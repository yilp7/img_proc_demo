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

// =============================================================================
// Strong-enum IDs. Wrong-side indexing becomes a compile error.
// =============================================================================

enum class StreamId : int {
    None  = -1,
    S0    = 0,
    S1    = 1,
    S2    = 2,
    _Count = 3,
};

enum class DisplayId : int {
    Main = 0,
    Alt1 = 1,
    Alt2 = 2,
    _Count = 3,
};

Q_DECLARE_METATYPE(StreamId)
Q_DECLARE_METATYPE(DisplayId)

constexpr std::size_t kStreamCount  = static_cast<std::size_t>(StreamId::_Count);
constexpr std::size_t kDisplayCount = static_cast<std::size_t>(DisplayId::_Count);

// Source type (replaces magic-int `device_type` values 1 / -1 / -2 / 0).
enum class SourceType : int {
    None          = 0,
    IndustrialCam = 1,  // hik / ebus / euresys
    LocalFile     = 2,  // ffmpeg-decoded file
    NetworkStream = 3,  // RTSP / HTTP
    Webcam        = 4,  // cv::VideoCapture DSHOW (transitional, per [[project-video-no-opencv]])
    StaticImage   = 5,  // single image displayed without a producer loop
};

// Today only two types drive special pipeline branches: IndustrialCam (hik path
// has the frame_info packet-loss queue), and LocalFile (paused playback repushes
// the popped frame to keep the display alive).
inline bool source_has_frame_info(SourceType t) { return t == SourceType::IndustrialCam; }
inline bool source_repush_on_pause(SourceType t) { return t == SourceType::LocalFile; }

// =============================================================================
// IdArray<Id, T, N> — fixed-size array keyed by a strong enum.
// =============================================================================

template <typename Id, typename T, std::size_t N>
class IdArray {
public:
    T&       operator[](Id id)       { return m_data[static_cast<std::size_t>(id)]; }
    const T& operator[](Id id) const { return m_data[static_cast<std::size_t>(id)]; }

    T*       begin()       { return m_data; }
    T*       end()         { return m_data + N; }
    const T* begin() const { return m_data; }
    const T* end()   const { return m_data + N; }

    constexpr std::size_t size() const { return N; }

private:
    T m_data[N];
};

// =============================================================================
// BoundedQueue<T> — drop-oldest queue with close() and pop_or_timeout().
//   - push(v) is non-blocking; oldest is dropped when size exceeds capacity.
//   - pop_or_timeout(out, ms) blocks up to `ms` ms. Returns true if a value
//     was popped, false on timeout or closed-empty (caller distinguishes via
//     is_closed()).
//   - close() wakes all blocked pops; further pushes are dropped silently.
//   - reopen() resets the closed flag.
// =============================================================================

template <typename T>
class BoundedQueue {
public:
    explicit BoundedQueue(std::size_t cap) : m_cap(cap) {}

    void push(T value) {
        QMutexLocker lk(&m_mtx);
        if (m_closed) return;
        while (m_q.size() >= m_cap) m_q.pop_front();
        m_q.push_back(std::move(value));
        m_cv.wakeOne();
    }

    // Returns true and writes to `out` on success. Returns false on timeout or
    // closed-and-empty. Caller can disambiguate via is_closed().
    bool pop_or_timeout(T& out, unsigned long timeout_ms) {
        QMutexLocker lk(&m_mtx);
        if (m_q.empty() && !m_closed) m_cv.wait(&m_mtx, timeout_ms);
        if (m_q.empty()) return false;
        out = std::move(m_q.front());
        m_q.pop_front();
        return true;
    }

    bool try_pop(T& out) {
        QMutexLocker lk(&m_mtx);
        if (m_q.empty()) return false;
        out = std::move(m_q.front());
        m_q.pop_front();
        return true;
    }

    void close() {
        QMutexLocker lk(&m_mtx);
        m_closed = true;
        m_cv.wakeAll();
    }

    void reopen() {
        QMutexLocker lk(&m_mtx);
        m_closed = false;
    }

    void clear() {
        QMutexLocker lk(&m_mtx);
        m_q.clear();
    }

    bool is_closed() const {
        QMutexLocker lk(&m_mtx);
        return m_closed;
    }

    std::size_t size() const {
        QMutexLocker lk(&m_mtx);
        return m_q.size();
    }

    bool empty() const {
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

// =============================================================================
// Stream metadata — source-owned, copied to consumers via snapshot.
// =============================================================================

struct StreamMeta {
    int        width        = 0;
    int        height       = 0;
    int        pixel_depth  = 8;
    int        pixel_format = 0;     // hik-cam only
    bool       is_color     = false;
    float      fps          = 0.0f;
    SourceType source_type  = SourceType::None;
};

// =============================================================================
// Per-display pipeline-runtime caches (persist across loop iterations).
// Reset on route-generation bump in the consumer loop.
//   - Moved out of pipelineprocessor.h so DisplaySlot can own them by value.
// =============================================================================

struct FrameAverageRuntime {
    cv::Mat seq[8], seq_sum, frame_a_sum, frame_b_sum;
    cv::Mat prev_img, prev_3d;
    int     seq_idx = 0;
    bool    frame_a = true;

    void release_avg_buffers() {
        seq_sum.release(); frame_a_sum.release(); frame_b_sum.release();
        for (auto& m : seq) m.release();
        seq_idx = 0;
    }
    void release_all() {
        prev_img.release(); prev_3d.release();
        release_avg_buffers();
    }
};

struct ECCRuntime {
    std::deque<cv::Mat> fusion_buf, fusion_warps, fusion_warps_inv;
    cv::Mat fusion_last_warp;
    int     prev_ecc_warp_mode = -1;

    void clear() {
        fusion_buf.clear(); fusion_warps.clear(); fusion_warps_inv.clear();
        fusion_last_warp.release();
    }
};

// =============================================================================
// StreamFrameSink — bundle of pointers the producer pushes through. The cam
// SDK's user_pointer aims at one of these; the cv:// branch and FFmpeg lambda
// hold one and call sink.frames->push(...) instead of touching q_img[] directly.
// =============================================================================

struct StreamFrameSink {
    BoundedQueue<cv::Mat>* frames     = nullptr;
    BoundedQueue<int>*     frame_info = nullptr;  // packets-lost per frame (hik only)
    QMutex*                meta_mutex = nullptr;
    StreamMeta*            meta       = nullptr;
    cv::Mat*               latest_raw = nullptr;  // post-pop scratch (ROI/scan reads it)
    SourceType             source_type = SourceType::None;
};

// =============================================================================
// StreamSlot — producer-side ownership.
// =============================================================================

struct StreamSlot {
    StreamId   id           = StreamId::None;
    bool       active       = false;  // a producer is feeding this slot
    StreamMeta meta;
    cv::Mat    latest_raw;             // last popped frame (consumer writes after pop)
    QMutex     meta_mutex;             // protects meta + latest_raw access
    BoundedQueue<cv::Mat> frames{static_cast<std::size_t>(5)};      // MAX_QUEUE_SIZE
    BoundedQueue<int>     frame_info{static_cast<std::size_t>(5)};  // MAX_QUEUE_SIZE
    std::atomic<bool>     producer_run{false};   // producer should keep pushing

    StreamFrameSink sink() {
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

// =============================================================================
// DisplaySlot — consumer-side ownership.
// =============================================================================

struct DisplaySlot {
    DisplayId  id            = DisplayId::Main;
    Display*   widget        = nullptr;     // owned by UserPanel/Qt parent tree
    QLabel*    fps_label     = nullptr;     // owned by Qt parent tree
    QLabel*    source_dead_label = nullptr; // bottom-right "source died" overlay; owned by Qt parent

    QMutex     paint_mutex;                 // guards modified_result <-> widget paint
    QMutex     setup_mutex;                 // serialises external loaders (FFmpeg lambda /
                                            // load_image_file) setting up this display slot;
                                            // not held during the steady-state pipeline loop
    cv::Mat    modified_result;             // last pipeline output (also read by export pipeline)
    cv::Mat    user_mask;                   // ROI overlay drawn on this display
    TimedQueue* fps_calc     = nullptr;     // owned by UserPanel

    // YOLO (per-display since T1)
    YoloDetector* yolo       = nullptr;
    int           yolo_last_model = -1;
    // Learned enhance option, created on first selection so the ONNX session
    // is not paid for by installations that never pick it.
    NafNetEnhancer* nafnet   = nullptr;
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

    // Recorders. Auto-finalize on route change, source death, display blank.
    // raw_path / proc_path hold the temp filename for the currently-open
    // writer; consumer thread emits these via recorder_finalized so the GUI
    // thread can move the file to save_location and update button text.
    cv::VideoWriter raw_writer;
    cv::VideoWriter proc_writer;
    QString         raw_path;
    QString         proc_path;

    // Consumer thread state.
    ConsumerThread*   consumer       = nullptr;
    std::atomic<bool> consumer_run   {false};   // consumer should keep iterating
    std::atomic<bool> consumer_state {false};   // true while consumer alive (status)
    std::uint64_t     last_route_gen = 0;       // detects route changes mid-loop

    // Source-death observation. Set true when the routed stream's queue is
    // closed-and-empty; set false when a frame arrives. Edge transitions emit
    // PipelineProcessor::source_state_changed; UI consumes it to show / hide
    // the corner overlay.
    std::atomic<bool> source_dead    {false};
};

// =============================================================================
// DisplayRouter — single routing source of truth.
//
// Lifecycle model:
//   - A stream's lifetime is driven by its producer (cam SDK / FFmpeg / cv::).
//     When a producer starts, the user/UI calls route(display, stream) to bind
//     the stream to a display.
//   - When a producer stops — either because the user stopped it, or because
//     the source died naturally (file ended, network drop, cam disconnect) —
//     the caller calls unroute(stream). This is the ONLY way a display
//     becomes blank: as a consequence of its stream stopping. There is no
//     display-side "blank" command.
//   - swap(a, b) atomically exchanges which streams two displays show.
//
// Bijection invariant: at most one display routes to a given stream at a
// time; a display routes to at most one stream. Routes are uni-directional —
// blanking is observed, not commanded.
//
// snapshot(d) returns (stream, gen). Consumer loops detect route changes by
// gen bump and reset per-route caches (frame-avg, ECC buffers, etc.) before
// continuing.
// =============================================================================

class DisplayRouter {
public:
    struct RouteSnapshot {
        StreamId      stream = StreamId::None;
        std::uint64_t gen    = 0;
    };

    DisplayRouter() {
        for (std::size_t i = 0; i < kDisplayCount; ++i) {
            m_route[static_cast<DisplayId>(i)] = StreamId::None;
            m_displays[static_cast<DisplayId>(i)].id = static_cast<DisplayId>(i);
        }
        for (std::size_t i = 0; i < kStreamCount; ++i) {
            m_streams[static_cast<StreamId>(i)].id = static_cast<StreamId>(i);
        }
    }

    // Bind a stream to a display. Caller is responsible for starting the
    // producer; this just updates the routing table.
    // Returns false and does nothing if the stream is already routed elsewhere.
    bool route(DisplayId d, StreamId s) {
        QMutexLocker lk(&m_mtx);
        if (s == StreamId::None) return false;
        // bijection check: stream must not already be routed elsewhere
        for (std::size_t i = 0; i < kDisplayCount; ++i) {
            DisplayId od = static_cast<DisplayId>(i);
            if (od != d && m_route[od] == s) return false;
        }
        m_route[d] = s;
        ++m_gen;
        return true;
    }

    // Called when a stream stops (user-initiated or natural). Clears any
    // display that was routed to it. Does NOT stop the producer — caller has
    // already done that, or the producer died on its own.
    void unroute(StreamId s) {
        if (s == StreamId::None) return;
        QMutexLocker lk(&m_mtx);
        for (std::size_t i = 0; i < kDisplayCount; ++i) {
            DisplayId d = static_cast<DisplayId>(i);
            if (m_route[d] == s) m_route[d] = StreamId::None;
        }
        ++m_gen;
    }

    // Atomic swap of which streams the two displays show.
    void swap(DisplayId a, DisplayId b) {
        if (a == b) return;
        QMutexLocker lk(&m_mtx);
        StreamId tmp = m_route[a];
        m_route[a] = m_route[b];
        m_route[b] = tmp;
        ++m_gen;
    }

    RouteSnapshot snapshot(DisplayId d) const {
        QMutexLocker lk(&m_mtx);
        RouteSnapshot s;
        s.stream = m_route[d];
        s.gen    = m_gen;
        return s;
    }

    // Returns StreamId::None if no stream is currently routed to this display.
    StreamId stream_routed_to(DisplayId d) const {
        QMutexLocker lk(&m_mtx);
        return m_route[d];
    }

    // Returns DisplayId::_Count if the stream is not currently routed.
    DisplayId display_for(StreamId s) const {
        QMutexLocker lk(&m_mtx);
        for (std::size_t i = 0; i < kDisplayCount; ++i) {
            DisplayId d = static_cast<DisplayId>(i);
            if (m_route[d] == s) return d;
        }
        return DisplayId::_Count;
    }

    std::uint64_t generation() const {
        QMutexLocker lk(&m_mtx);
        return m_gen;
    }

    StreamSlot&        stream(StreamId s)         { return m_streams[s]; }
    const StreamSlot&  stream(StreamId s) const   { return m_streams[s]; }
    DisplaySlot&       display(DisplayId d)       { return m_displays[d]; }
    const DisplaySlot& display(DisplayId d) const { return m_displays[d]; }

private:
    mutable QMutex m_mtx;
    IdArray<DisplayId, StreamId, kDisplayCount> m_route;
    IdArray<StreamId,  StreamSlot,  kStreamCount>  m_streams;
    IdArray<DisplayId, DisplaySlot, kDisplayCount> m_displays;
    std::uint64_t  m_gen = 1;
};

#endif // ROUTING_H
