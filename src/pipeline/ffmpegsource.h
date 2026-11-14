#ifndef FFMPEGSOURCE_H
#define FFMPEGSOURCE_H

#include <atomic>
#include <thread>

#include <QObject>
#include <QString>
#include <opencv2/core.hpp>

#include "pipeline/routing.h"  // StreamId

// Reads one network stream or local file with FFmpeg and feeds one StreamSlot.
// start() spawns the worker and returns; opened() or failed() reports the result.
// Signals come from the worker thread: frame_decoded() is connected directly, the rest queued.
class FfmpegSource : public QObject
{
    Q_OBJECT
public:
    struct VideoInfo {
        int    width    = 0;
        int    height   = 0;
        double fps      = 0.0;
        bool   is_color = true;  // false for format_gray=true
    };

    explicit FfmpegSource(QObject *parent = nullptr);
    ~FfmpegSource() override;

    // Starts the worker, stopping a previous run first. url is any FFmpeg input.
    // format_gray routes decoded frames through a yuv to gray filter graph.
    void start(StreamId target, QString url, bool format_gray);

    // Signal the worker to stop and wait up to timeout_ms for it to exit.
    // Default timeout matches the FFmpeg interrupt cadence (~1 s).
    void stop(int timeout_ms = 2000);

    // Called by UserPanel after opened() has finished display/queue setup.
    // The worker waits for this ack before it starts emitting frames.
    void mark_display_ready(int session);

    bool is_running() const { return m_running.load(); }
    bool is_stopped() const { return m_stopped.load(); }
    StreamId get_target() const { return m_target; }
    QString get_current_url() const { return m_url; }
    bool get_current_format_gray() const { return m_format_gray; }
    // Bumped on every start(). The worker tags each signal with the value it started
    // with, and receivers drop any other, so events still queued from a stopped
    // worker cannot leak into the new stream.
    int get_current_session() const { return m_session.load(); }

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
    std::atomic<bool> m_stopped     {true};
    std::atomic<int>  m_session     {0};
    std::atomic<int>  m_ready_session {0};
    std::thread       m_thread;
    StreamId          m_target = StreamId::NONE;
    QString           m_url;
    bool              m_format_gray = false;
};

Q_DECLARE_METATYPE(FfmpegSource::VideoInfo)

#endif // FFMPEGSOURCE_H
