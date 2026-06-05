#ifndef FFMPEGSOURCE_H
#define FFMPEGSOURCE_H

#include <atomic>
#include <thread>

#include <QObject>
#include <QString>
#include <opencv2/core.hpp>

#include "pipeline/routing.h"  // StreamId

// FfmpegSource — live network/local-file video reader driving one StreamSlot.
//
// Replaces the inline ~300-line lambda inside UserPanel::load_video_file's
// else-branch (FFmpeg path). Constructor-only, then start()/stop():
//   - start(target, url, format_gray) spawns a worker thread that runs the
//     FFmpeg open + decode loop. Returns immediately; success/failure is
//     reported via opened()/failed() signals.
//   - stop() sets the interrupt flag and waits with bounded timeout for the
//     worker to exit. Safe to call multiple times. Called from dtor.
//
// Signals are emitted from the worker thread. Lifecycle signals stay queued
// to the GUI thread; frame_decoded() is connected directly so frames enter
// BoundedQueue without building an unbounded GUI-event backlog.
class FfmpegSource : public QObject {
    Q_OBJECT
public:
    struct VideoInfo {
        int    width    = 0;
        int    height   = 0;
        double fps      = 0.0;
        bool   is_color = true;  // false for format_gray=true
    };

    explicit FfmpegSource(QObject* parent = nullptr);
    ~FfmpegSource() override;

    // Kick off the worker. If a previous run is still alive, stops it first.
    // url is the FFmpeg input (file path, rtsp://, http(s)://, dshow URL).
    // format_gray converts decoded frames through a yuv→gray filtergraph.
    void start(StreamId target, QString url, bool format_gray);

    // Signal the worker to stop and wait up to timeout_ms for it to exit.
    // Default timeout matches the FFmpeg interrupt cadence (~1 s).
    void stop(int timeout_ms = 2000);

    // Called by UserPanel after opened() has finished display/queue setup.
    // The worker waits for this ack before it starts emitting frames.
    void mark_display_ready(int session);

    bool is_running() const { return m_running.load(); }
    StreamId target() const { return m_target; }
    // Monotonic session counter. Bumped on every start(); worker captures the
    // value at thread launch and tags every emit. Receivers drop signals
    // whose session != current_session(), so events from a stop()ped worker
    // that are still queued in the GUI event loop don't bleed into the
    // freshly started worker's stream.
    int current_session() const { return m_session.load(); }

signals:
    void opened(StreamId target, int session, FfmpegSource::VideoInfo info);
    void frame_decoded(StreamId target, int session, cv::Mat frame);
    // reason: short error code string ("open_failed", "no_video_stream",
    // "codec_unsupported", "decoder_open_failed"). Used by UI to log + bail.
    void failed(StreamId target, int session, QString reason);
    void finished(StreamId target, int session);

private:
    void run_thread(StreamId target, int session, QString url, bool format_gray);

    std::atomic<bool> m_should_stop {false};
    std::atomic<bool> m_running     {false};
    std::atomic<int>  m_session     {0};
    std::atomic<int>  m_ready_session {0};
    std::thread       m_thread;
    StreamId          m_target = StreamId::None;
};

Q_DECLARE_METATYPE(FfmpegSource::VideoInfo)

#endif // FFMPEGSOURCE_H
