#ifndef ROUTING_H
#define ROUTING_H

#include <QMetaType>
#include <QMutex>
#include <QMutexLocker>
#include <QRect>
#include <QWaitCondition>
#include <QString>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>

#include "pipeline/pipelineconfig.h"
#include "pipeline/processingparams.h"
#include "yolo/targettracker.h"
#include "yolo/gatetracker.h"
#include "util/targetestimator.h"

class Display;
class QLabel;
class TimedQueue;
class YoloDetector;
class NafNetEnhancer;
class ConsumerThread;

// Strong-enum ids: indexing with the wrong one is a compile error.

enum class StreamId : int {
    NONE  = -1,
    S0    = 0,
    S1    = 1,
    S2    = 2,
    COUNT = 3,
};

enum class DisplayId : int {
    MAIN = 0,
    ALT1 = 1,
    ALT2 = 2,
    COUNT = 3,
};

Q_DECLARE_METATYPE(StreamId)
Q_DECLARE_METATYPE(DisplayId)

constexpr std::size_t STREAM_COUNT  = static_cast<std::size_t>(StreamId::COUNT);
constexpr std::size_t DISPLAY_COUNT = static_cast<std::size_t>(DisplayId::COUNT);

enum class SourceType : int {
    NONE           = 0,
    INDUSTRIAL_CAM = 1,  // hik / ebus / euresys
    LOCAL_FILE     = 2,  // ffmpeg-decoded file
    NETWORK_STREAM = 3,  // RTSP / HTTP
    WEBCAM         = 4,  // cv::VideoCapture DSHOW, transitional
    STATIC_IMAGE   = 5,  // single image displayed without a producer loop
};

// Today only two types drive special pipeline branches: INDUSTRIAL_CAM (hik path
// has the frame_info packet-loss queue), and LOCAL_FILE (paused playback repushes
// the popped frame to keep the display alive).
inline bool source_has_frame_info(SourceType t) { return t == SourceType::INDUSTRIAL_CAM; }
inline bool source_repush_on_pause(SourceType t) { return t == SourceType::LOCAL_FILE; }

// Fixed-size array keyed by a strong enum.

template <typename Id, typename T, std::size_t N>
class IdArray
{
public:
    T       &operator[](Id id)       { return m_data[static_cast<std::size_t>(id)]; }
    const T &operator[](Id id) const { return m_data[static_cast<std::size_t>(id)]; }

    T       *begin()       { return m_data; }
    T       *end()         { return m_data + N; }
    const T *begin() const { return m_data; }
    const T *end()   const { return m_data + N; }

    constexpr std::size_t size() const { return N; }

private:
    T m_data[N];
};

// Drop-oldest queue. push() never blocks and drops the oldest value when full.
// pop_or_timeout() returns false on timeout or when closed and empty; is_closed()
// tells which. close() wakes every blocked pop and makes later pushes no-ops.

template <typename T>
class BoundedQueue
{
public:
    explicit BoundedQueue(std::size_t cap) : m_cap(cap) {}

    void push(T value)
    {
        QMutexLocker lk(&m_mtx);
        if (m_closed) return;
        while (m_q.size() >= m_cap) m_q.pop_front();
        m_q.push_back(std::move(value));
        m_cv.wakeOne();
    }

    bool pop_or_timeout(T &out, unsigned long timeout_ms)
    {
        QMutexLocker lk(&m_mtx);
        if (m_q.empty() && !m_closed) m_cv.wait(&m_mtx, timeout_ms);
        if (m_q.empty()) return false;
        out = std::move(m_q.front());
        m_q.pop_front();
        return true;
    }

    bool try_pop(T &out)
    {
        QMutexLocker lk(&m_mtx);
        if (m_q.empty()) return false;
        out = std::move(m_q.front());
        m_q.pop_front();
        return true;
    }

    void close()
    {
        QMutexLocker lk(&m_mtx);
        m_closed = true;
        m_cv.wakeAll();
    }

    void reopen()
    {
        QMutexLocker lk(&m_mtx);
        m_closed = false;
    }

    void clear()
    {
        QMutexLocker lk(&m_mtx);
        m_q.clear();
    }

    bool is_closed() const
    {
        QMutexLocker lk(&m_mtx);
        return m_closed;
    }

    std::size_t size() const
    {
        QMutexLocker lk(&m_mtx);
        return m_q.size();
    }

    bool empty() const
    {
        QMutexLocker lk(&m_mtx);
        return m_q.empty();
    }

private:
    mutable QMutex   m_mtx;
    QWaitCondition   m_cv;
    std::deque<T>    m_q;
    std::size_t      m_cap;
    bool             m_closed = false;
};

// Stream metadata, owned by the source; consumers take a snapshot.

struct StreamMeta {
    int        width        = 0;
    int        height       = 0;
    int        pixel_depth  = 8;
    int        pixel_format = 0;     // hik-cam only
    bool       is_color     = false;
    float      fps          = 0.0f;
    SourceType source_type  = SourceType::NONE;
};

// Per-display runtime caches. They persist across loop iterations and are reset
// when the route generation changes.

struct FrameAverageRuntime {
    cv::Mat seq[8], seq_sum, frame_a_sum, frame_b_sum;
    cv::Mat prev_img, prev_3d;
    int     seq_idx = 0;
    bool    frame_a = true;

    void release_avg_buffers()
    {
        seq_sum.release(); frame_a_sum.release(); frame_b_sum.release();
        for (auto &m : seq) m.release();
        seq_idx = 0;
    }
    void release_all()
    {
        prev_img.release(); prev_3d.release();
        release_avg_buffers();
    }
};

struct ECCRuntime {
    std::deque<cv::Mat> fusion_buf, fusion_warps, fusion_warps_inv;
    cv::Mat fusion_last_warp;
    int     prev_ecc_warp_mode = -1;

    void clear()
    {
        fusion_buf.clear(); fusion_warps.clear(); fusion_warps_inv.clear();
        fusion_last_warp.release();
    }
};

// What a producer pushes through. The camera SDK's user pointer aims at one;
// the other producers hold one and call frames->push().

struct StreamFrameSink {
    BoundedQueue<cv::Mat> *frames     = nullptr;
    BoundedQueue<int>     *frame_info = nullptr;  // packets-lost per frame (hik only)
    QMutex                *meta_mutex = nullptr;
    StreamMeta            *meta       = nullptr;
    cv::Mat               *latest_raw = nullptr;  // post-pop scratch (ROI/scan reads it)
    SourceType             source_type = SourceType::NONE;
};

// Producer side.

struct StreamSlot {
    StreamId   id           = StreamId::NONE;
    bool       active       = false;  // a producer is feeding this slot
    StreamMeta meta;
    cv::Mat    latest_raw;             // last popped frame (consumer writes after pop)
    QMutex     meta_mutex;             // protects meta + latest_raw access
    BoundedQueue<cv::Mat> frames{static_cast<std::size_t>(5)};      // MAX_QUEUE_SIZE
    BoundedQueue<int>     frame_info{static_cast<std::size_t>(5)};  // MAX_QUEUE_SIZE
    std::atomic<bool>     producer_run{false};   // producer should keep pushing

    StreamFrameSink sink()
    {
        StreamFrameSink s;
        s.frames      = &frames;
        s.frame_info  = &frame_info;
        s.meta_mutex  = &meta_mutex;
        s.meta        = &meta;
        s.latest_raw  = &latest_raw;
        s.source_type = meta.source_type;
        return s;
    }
};

// Consumer side.

struct DisplaySlot {
    DisplayId  id            = DisplayId::MAIN;
    Display   *widget        = nullptr;     // owned by UserPanel/Qt parent tree
    QLabel    *fps_label     = nullptr;     // owned by Qt parent tree
    QLabel    *source_dead_label = nullptr; // bottom-right "source died" overlay; owned by Qt parent

    QMutex     paint_mutex;                 // guards modified_result <-> widget paint
    QMutex     setup_mutex;                 // held by a loader while it sets up this slot, never in the pipeline loop
    cv::Mat    modified_result;             // last pipeline output (also read by export pipeline)
    cv::Mat    user_mask;                   // ROI overlay drawn on this display
    TimedQueue *fps_calc     = nullptr;     // owned by UserPanel

    // YOLO, one detector per display
    YoloDetector *yolo       = nullptr;
    int           yolo_last_model = -1;
    // Learned enhance option, created on first selection so the ONNX session
    // is not paid for by installations that never pick it.
    NafNetEnhancer *nafnet   = nullptr;
    TargetTracker tracker;
    // Source-frame size the tracker's boxes are expressed in. Range and
    // bearing need it; the display geometry cannot supply it after cropping.
    cv::Size      tracker_frame_size;

    // Range-on-label snapshot, pushed from the GUI thread. Heights are indexed
    // by class id for the currently selected model, so the pipeline needs no
    // class-name lookup and no access to the table.
    QMutex        label_mutex;
    bool          range_label_enabled = false;
    std::vector<double> class_heights;
    TargetEstimator::Optics label_optics;

    // Operator-designated follow gate, for closed-loop follow with no detector.
    // The request crosses from the GUI thread, so it is guarded; the tracker
    // itself is only ever touched by the pipeline thread.
    GateTracker   gate_tracker;
    QMutex        gate_mutex;
    QRect         gate_request;          // display-space; empty request clears
    bool          gate_request_pending = false;
    GateTracker::Settings gate_settings; // method and options, from the setup panel

    // Per-display pipeline config (not persisted to config file).
    PipelineConfig pcfg;

    // Pipeline-runtime caches reset on route-generation bump.
    FrameAverageRuntime avg_state;
    ECCRuntime          ecc_state;
    uint                hist[256] = {0};

    // Recording / one-shot save flags driven by UI buttons.
    std::atomic<bool> record_raw  {false};
    std::atomic<bool> record_proc {false};
    std::atomic<bool> save_raw    {false};
    std::atomic<bool> save_proc   {false};

    // Recorders, finalised on route change, source death or display blank.
    // raw_path / proc_path are the temp files of the open writers; the consumer thread
    // reports them through recorder_finalized so the GUI can move them.
    cv::VideoWriter raw_writer;
    cv::VideoWriter proc_writer;
    QString         raw_path;
    QString         proc_path;

    // Consumer thread state.
    ConsumerThread   *consumer       = nullptr;
    std::atomic<bool> consumer_run   {false};   // consumer should keep iterating
    std::atomic<bool> consumer_state {false};   // true while consumer alive (status)
    std::uint64_t     last_route_gen = 0;       // detects route changes mid-loop

    // True while the routed stream's queue is closed and empty, false once a frame arrives.
    // Transitions emit PipelineProcessor::source_state_changed for the corner overlay.
    std::atomic<bool> source_dead    {false};
};

// The single source of truth for routing. One stream per display and one display
// per stream. A display goes blank only when its stream stops and is unrouted.
// snapshot() carries a generation; consumers reset per-route caches when it changes.

class DisplayRouter
{
public:
    struct RouteSnapshot {
        StreamId      stream = StreamId::NONE;
        std::uint64_t gen    = 0;
    };

    DisplayRouter()
    {
        for (std::size_t i = 0; i < DISPLAY_COUNT; ++i) {
            m_route[static_cast<DisplayId>(i)] = StreamId::NONE;
            m_displays[static_cast<DisplayId>(i)].id = static_cast<DisplayId>(i);
        }
        for (std::size_t i = 0; i < STREAM_COUNT; ++i) {
            m_streams[static_cast<StreamId>(i)].id = static_cast<StreamId>(i);
        }
    }

    // Binds a stream to a display; starting the producer is the caller's job.
    // Returns false when the stream is already routed elsewhere.
    bool route(DisplayId d, StreamId s)
    {
        QMutexLocker lk(&m_mtx);
        if (s == StreamId::NONE) return false;
        // bijection check: stream must not already be routed elsewhere
        for (std::size_t i = 0; i < DISPLAY_COUNT; ++i) {
            DisplayId od = static_cast<DisplayId>(i);
            if (od != d && m_route[od] == s) return false;
        }
        m_route[d] = s;
        ++m_gen;
        return true;
    }

    // Called when a stream stops, for any reason. Clears the display routed to it.
    // Does not stop the producer.
    void unroute(StreamId s)
    {
        if (s == StreamId::NONE) return;
        QMutexLocker lk(&m_mtx);
        for (std::size_t i = 0; i < DISPLAY_COUNT; ++i) {
            DisplayId d = static_cast<DisplayId>(i);
            if (m_route[d] == s) m_route[d] = StreamId::NONE;
        }
        ++m_gen;
    }

    // Atomic swap of which streams the two displays show.
    void swap(DisplayId a, DisplayId b)
    {
        if (a == b) return;
        QMutexLocker lk(&m_mtx);
        StreamId tmp = m_route[a];
        m_route[a] = m_route[b];
        m_route[b] = tmp;
        ++m_gen;
    }

    RouteSnapshot snapshot(DisplayId d) const
    {
        QMutexLocker lk(&m_mtx);
        RouteSnapshot s;
        s.stream = m_route[d];
        s.gen    = m_gen;
        return s;
    }

    // Returns StreamId::NONE if no stream is currently routed to this display.
    StreamId stream_routed_to(DisplayId d) const
    {
        QMutexLocker lk(&m_mtx);
        return m_route[d];
    }

    // Returns DisplayId::COUNT if the stream is not currently routed.
    DisplayId display_for(StreamId s) const
    {
        QMutexLocker lk(&m_mtx);
        for (std::size_t i = 0; i < DISPLAY_COUNT; ++i) {
            DisplayId d = static_cast<DisplayId>(i);
            if (m_route[d] == s) return d;
        }
        return DisplayId::COUNT;
    }

    std::uint64_t get_generation() const
    {
        QMutexLocker lk(&m_mtx);
        return m_gen;
    }

    StreamSlot        &get_stream(StreamId s)         { return m_streams[s]; }
    const StreamSlot  &get_stream(StreamId s) const   { return m_streams[s]; }
    DisplaySlot       &get_display(DisplayId d)       { return m_displays[d]; }
    const DisplaySlot &get_display(DisplayId d) const { return m_displays[d]; }

private:
    mutable QMutex m_mtx;
    IdArray<DisplayId, StreamId, DISPLAY_COUNT> m_route;
    IdArray<StreamId,  StreamSlot,  STREAM_COUNT>  m_streams;
    IdArray<DisplayId, DisplaySlot, DISPLAY_COUNT> m_displays;
    std::uint64_t  m_gen = 1;
};

#endif // ROUTING_H
