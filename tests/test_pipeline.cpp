#include <QtTest>

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QPixmap>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <atomic>
#include <chrono>
#include <thread>
#include <utility>
#include <vector>
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>

#include "pipeline/pipelineprocessor.h"
#include "image/imageproc.h"
#include "util/threadpool.h"

// The frame path on its own. Frames go into a stream queue; pictures, scan steps and files
// come out. Everything above the pipeline is replaced by what this file holds: the values
// a controller would publish, and slots that record what the window would receive.

namespace {

const int FRAME_W = 64;
const int FRAME_H = 48;

// What the pipeline sent, written on the frame thread and read by the test thread.
struct Recorder {
    QMutex                mutex;
    int                   pictures = 0;   // display_frame_ready for the main display
    QSize                 picture_size;
    std::vector<ScanStep> steps;
    std::vector<quint64>  ends;           // generations whose end was announced
    std::vector<cv::Mat>  dist_mats;      // one per frame that reached the 3D stage
    std::vector<std::pair<int, QString>> closed_recorders;  // kind and path, main display
};

cv::Mat flat_frame(int grey)
{
    return cv::Mat(FRAME_H, FRAME_W, CV_8UC1, cv::Scalar(grey));
}

// Grey values from first to first + 99. gated_3d_v2 divides one frame by the other and
// drops values above 251, so the test frames stay inside 1..251.
cv::Mat patterned_frame(int first, int step_x, int step_y)
{
    cv::Mat frame(FRAME_H, FRAME_W, CV_8UC1);
    for (int y = 0; y < FRAME_H; y++) {
        for (int x = 0; x < FRAME_W; x++) {
            frame.at<uchar>(y, x) = static_cast<uchar>(first + (x * step_x + y * step_y) % 100);
        }
    }
    return frame;
}

// A grid of 2 by 2 positions with two delays at each, gate width left alone.
ScanJob grid_job(const QString &name, quint64 generation, int wait_time)
{
    ScanJob job;
    job.generation = generation;
    job.running    = true;
    job.name       = name;
    job.params.ptz_scan_mode           = ScanParams::PTZ_SCAN_GRID;
    job.params.scan_ptz_enabled        = true;
    job.params.scan_delay_enabled      = true;
    job.params.scan_gate_width_enabled = false;
    job.params.capture_scan_ori        = true;
    job.params.capture_scan_res        = false;
    job.params.num_single_pos          = 2;
    job.params.ptz_wait_time           = wait_time;
    job.ptz_route        = { {10.0f, 1.0f}, {20.0f, 1.0f}, {20.0f, 2.0f}, {10.0f, 2.0f} };
    job.tcu_route        = { 2000.0f, 2500.0f };
    job.gate_width_route = { 40.0f, 40.0f };
    return job;
}

// Motion JPEG in an AVI file: OpenCV writes and reads it itself, so the test needs no
// installed codec. The size and the single channel are those of the test frames.
bool open_recorder(cv::VideoWriter &writer, const QString &path)
{
    return writer.open(path.toLocal8Bit().constData(), cv::CAP_OPENCV_MJPEG,
                       cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), 10.0,
                       cv::Size(FRAME_W, FRAME_H), false);
}

int frames_in(const QString &path)
{
    cv::VideoCapture capture(path.toLocal8Bit().constData(), cv::CAP_OPENCV_MJPEG);
    if (!capture.isOpened()) return -1;
    int count = 0;
    cv::Mat frame;
    while (capture.read(frame)) count++;
    return count;
}

// Ends a helper thread on every way out of a test, a failed check included.
struct ThreadGuard {
    std::atomic<bool> &stop;
    std::thread       &thread;
    ~ThreadGuard()
    {
        stop = true;
        if (thread.joinable()) thread.join();
    }
};

// The pipeline with its inputs and one frame thread for the main display.
class Rig
{
public:
    Rig() :
        m_pool(2)
    {
        m_globals.params_mutex      = &m_params_mutex;
        m_globals.processing_params = &m_params;
        m_globals.scan_job          = &m_scan_job;
        m_globals.roi_mutex         = &m_roi_mutex;
        m_globals.list_roi          = &m_list_roi;
        m_globals.yolo_init_mutex   = &m_yolo_init_mutex;
        m_globals.tp                = &m_pool;
        m_pipeline = new PipelineProcessor(m_globals);
        m_pool.start();

        // direct: the slots run on the frame thread, as the emit does
        QObject::connect(m_pipeline, &PipelineProcessor::display_frame_ready, m_pipeline,
                         [this](int display_idx, QPixmap pm) {
            if (display_idx != 0) return;
            {
                QMutexLocker lk(&m_recorder.mutex);
                m_recorder.pictures++;
                m_recorder.picture_size = pm.size();
            }
            // a slow frame on demand: this runs on the frame thread, inside the frame
            const int hold_ms = m_hold_ms;
            if (hold_ms > 0) std::this_thread::sleep_for(std::chrono::milliseconds(hold_ms));
        }, Qt::DirectConnection);
        QObject::connect(m_pipeline, &PipelineProcessor::scan_step_requested, m_pipeline,
                         [this](ScanStep step) {
            QMutexLocker lk(&m_recorder.mutex);
            m_recorder.steps.push_back(step);
        }, Qt::DirectConnection);
        QObject::connect(m_pipeline, &PipelineProcessor::finish_scan_signal, m_pipeline,
                         [this](quint64 generation) {
            QMutexLocker lk(&m_recorder.mutex);
            m_recorder.ends.push_back(generation);
        }, Qt::DirectConnection);
        QObject::connect(m_pipeline, &PipelineProcessor::update_dist_mat, m_pipeline,
                         [this](cv::Mat dist_mat, double, double) {
            // a pass without a new frame sends an empty one
            if (dist_mat.empty()) return;
            QMutexLocker lk(&m_recorder.mutex);
            m_recorder.dist_mats.push_back(dist_mat.clone());
        }, Qt::DirectConnection);
        QObject::connect(m_pipeline, &PipelineProcessor::recorder_finalized, m_pipeline,
                         [this](int display_idx, int kind, QString temp_path) {
            if (display_idx != 0) return;
            QMutexLocker lk(&m_recorder.mutex);
            m_recorder.closed_recorders.push_back({kind, temp_path});
        }, Qt::DirectConnection);

        m_params.display_width   = FRAME_W;
        m_params.display_height  = FRAME_H;
        m_params.display_region  = cv::Rect(0, 0, FRAME_W, FRAME_H);
        m_params.display_size[0] = QSize(FRAME_W, FRAME_H);
        m_params.tcu.delay_dist    = 15.0f;
        m_params.tcu.depth_of_view = 6.0f;
        m_params.tcu.dist_ns       = 0.15f;
    }

    ~Rig()
    {
        stop();
        m_pool.stop();
        delete m_pipeline;
    }

    ProcessingParams &params() { return m_params; }
    DisplaySlot &display() { return m_router.get_display(DisplayId::MAIN); }
    StreamSlot &stream() { return m_router.get_stream(StreamId::S0); }
    QMutex &params_mutex() { return m_params_mutex; }
    QMutex &roi_mutex() { return m_roi_mutex; }

    // Every frame then takes at least this long: the frame thread waits where it sends
    // the picture, in the middle of the frame.
    void hold_each_frame(int ms) { m_hold_ms = ms; }

    // A LOCAL_FILE source puts each frame back into the queue, as a still image does, so
    // one pushed frame keeps the pipeline supplied. Any other type delivers each frame once.
    void start(SourceType source_type, float fps)
    {
        stream().meta.width       = FRAME_W;
        stream().meta.height      = FRAME_H;
        stream().meta.pixel_depth = 8;
        stream().meta.is_color    = false;
        stream().meta.fps         = fps;
        stream().meta.source_type = source_type;
        m_router.route(DisplayId::MAIN, StreamId::S0);

        display().consumer_run = true;
        m_worker = std::thread([this]() { m_pipeline->run(m_router, display()); });
    }

    void stop()
    {
        display().consumer_run = false;
        if (m_worker.joinable()) m_worker.join();
    }

    void push(const cv::Mat &frame) { stream().frames.push(frame.clone()); }

    // A route change, as a swap or a stopped source makes one: the display goes blank,
    // and takes its stream back.
    void unroute() { m_router.unroute(StreamId::S0); }
    void reroute() { m_router.route(DisplayId::MAIN, StreamId::S0); }

    void set_job(ScanJob job)
    {
        QMutexLocker lk(&m_params_mutex);
        job.revision = m_scan_job.revision + 1;
        m_scan_job = job;
    }

    // True once every pushed frame has been taken and three more pictures have gone out:
    // the last frame is then fully processed.
    bool wait_until_idle(int timeout_ms = 5000)
    {
        QElapsedTimer timer;
        timer.start();
        while (!stream().frames.empty()) {
            if (timer.elapsed() > timeout_ms) return false;
            QTest::qWait(5);
        }
        const int seen = picture_count();
        while (picture_count() < seen + 3) {
            if (timer.elapsed() > timeout_ms) return false;
            QTest::qWait(5);
        }
        return true;
    }

    // True once every pushed frame has been taken and no picture has gone out for 50 ms.
    // A free frame thread sends one about every 10 ms, so it then stands inside a frame,
    // at a mutex the test holds.
    bool wait_until_held(int timeout_ms = 5000)
    {
        QElapsedTimer timer;
        timer.start();
        while (!stream().frames.empty()) {
            if (timer.elapsed() > timeout_ms) return false;
            QTest::qWait(5);
        }
        int seen = picture_count();
        QElapsedTimer still;
        still.start();
        while (still.elapsed() < 50) {
            if (timer.elapsed() > timeout_ms) return false;
            QTest::qWait(5);
            const int now = picture_count();
            if (now != seen) {
                seen = now;
                still.restart();
            }
        }
        return true;
    }

    int picture_count() { QMutexLocker lk(&m_recorder.mutex); return m_recorder.pictures; }
    QSize picture_size() { QMutexLocker lk(&m_recorder.mutex); return m_recorder.picture_size; }
    int step_count() { QMutexLocker lk(&m_recorder.mutex); return static_cast<int>(m_recorder.steps.size()); }
    std::vector<ScanStep> steps() { QMutexLocker lk(&m_recorder.mutex); return m_recorder.steps; }
    std::vector<quint64> ends() { QMutexLocker lk(&m_recorder.mutex); return m_recorder.ends; }
    int end_count() { QMutexLocker lk(&m_recorder.mutex); return static_cast<int>(m_recorder.ends.size()); }
    std::vector<cv::Mat> dist_mats() { QMutexLocker lk(&m_recorder.mutex); return m_recorder.dist_mats; }
    std::vector<std::pair<int, QString>> closed_recorders() { QMutexLocker lk(&m_recorder.mutex); return m_recorder.closed_recorders; }
    int closed_recorder_count() { QMutexLocker lk(&m_recorder.mutex); return static_cast<int>(m_recorder.closed_recorders.size()); }

private:
    QMutex                m_params_mutex;
    ProcessingParams      m_params;
    ScanJob               m_scan_job;
    QMutex                m_roi_mutex;
    std::vector<cv::Rect> m_list_roi;
    QMutex                m_yolo_init_mutex;
    ThreadPool            m_pool;
    PipelineGlobals       m_globals;
    DisplayRouter         m_router;
    PipelineProcessor    *m_pipeline = nullptr;
    std::thread           m_worker;
    Recorder              m_recorder;
    std::atomic<int>      m_hold_ms {0};
};

int bmp_count(const QString &folder)
{
    return QDir(folder).entryList(QStringList() << "*.bmp", QDir::Files).size();
}

} // namespace

class TestPipeline : public QObject
{
    Q_OBJECT

private slots:
    // TC-PL-001: a picture goes out at the widget size the copy holds.
    void picture_leaves_at_the_copied_widget_size()
    {
        Rig rig;
        rig.params().display_size[0] = QSize(FRAME_W / 2, FRAME_H / 2);
        rig.start(SourceType::NETWORK_STREAM, 10.0f);
        rig.push(flat_frame(90));

        QVERIFY(rig.wait_until_idle());
        QCOMPARE(rig.picture_size(), QSize(FRAME_W / 2, FRAME_H / 2));
    }

    // TC-PL-002: frame average over four frames gives their mean.
    void frame_average_is_the_mean_of_four_frames()
    {
        Rig rig;
        rig.display().pcfg.frame_avg_enabled = true;
        rig.display().pcfg.frame_avg_mode    = 0;
        rig.start(SourceType::NETWORK_STREAM, 10.0f);
        const int greys[] = { 10, 20, 30, 40 };
        for (int grey : greys) rig.push(flat_frame(grey));

        QVERIFY(rig.wait_until_idle());
        rig.stop();

        const cv::Mat &result = rig.display().modified_result;
        QVERIFY(result.size() == cv::Size(FRAME_W, FRAME_H));
        QCOMPARE(result.type(), static_cast<int>(CV_8UC1));
        QCOMPARE(cv::countNonZero(result != 25), 0);
    }

    void gated_3d_pairs_previous_and_current_frame_data()
    {
        QTest::addColumn<bool>("flag_at_start");
        QTest::newRow("flag off at start") << false;
        QTest::newRow("flag on at start")  << true;
    }

    // TC-PL-003: without frame average the 3D stage pairs the frame before with the
    // current one, and the frame flag decides which of them comes first. The flag flips
    // after every frame, so the second frame sees the opposite of the start value.
    void gated_3d_pairs_previous_and_current_frame()
    {
        QFETCH(bool, flag_at_start);

        Rig rig;
        rig.display().pcfg.enable_3d = true;
        rig.display().frame_a_3d = flag_at_start;
        const cv::Mat before  = patterned_frame(20, 1, 1);
        const cv::Mat current = patterned_frame(60, 3, 1);
        rig.start(SourceType::NETWORK_STREAM, 10.0f);
        rig.push(before);
        rig.push(current);

        QVERIFY(rig.wait_until_idle());
        // gated_3d_v2 keeps its scratch images in statics, so it is called here only
        // after the frame thread has stopped
        rig.stop();

        const std::vector<cv::Mat> got = rig.dist_mats();
        QCOMPARE(static_cast<int>(got.size()), 2);
        QCOMPARE(rig.display().frame_a_3d.load(), flag_at_start);

        const bool flag_at_second_frame = !flag_at_start;
        cv::Mat first  = (flag_at_second_frame ? before : current).clone();
        cv::Mat second = (flag_at_second_frame ? current : before).clone();
        const TcuReadout &tcu = rig.params().tcu;
        PipelineConfig pcfg;
        // the same expressions, with the same types, as in frame_average_and_3d
        const double delay = (tcu.delay_dist - tcu.offset_delay * tcu.dist_ns) / tcu.dist_ns;
        const double gw    = (tcu.depth_of_view - tcu.offset_gw * tcu.dist_ns) / tcu.dist_ns;
        cv::Mat picture, expected;
        double dist_min = 0, dist_max = 0;
        ImageProc::gated_3d_v2(first, second, picture, delay, gw, pcfg.colormap,
                               pcfg.lower_3d_thresh, pcfg.upper_3d_thresh, pcfg.truncate_3d,
                               &expected, &dist_min, &dist_max);

        QVERIFY(!expected.empty());
        QVERIFY(cv::checkRange(expected));
        QVERIFY(got[1].size() == expected.size());
        QCOMPARE(got[1].type(), expected.type());
        QCOMPARE(cv::norm(got[1], expected, cv::NORM_INF), 0.0);
    }

    // TC-PL-004: a grid scan asks for every step once, in order, saves the pictures of
    // every profile and announces its end once.
    void grid_scan_requests_each_step_and_saves_each_profile()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const ScanJob job = grid_job("scan_grid", 1, 1);
        // ScanController makes the scan's own folder; the pipeline makes those inside it
        QVERIFY(QDir(dir.path()).mkdir(job.name));

        Rig rig;
        rig.params().save_location = dir.path();
        rig.set_job(job);
        // at 1 fps a step is 1 frame of wait and 2 saved frames
        rig.start(SourceType::LOCAL_FILE, 1.0f);
        rig.push(flat_frame(90));

        QTRY_COMPARE_WITH_TIMEOUT(rig.end_count(), 1, 10000);

        const std::vector<ScanStep> steps = rig.steps();
        QCOMPARE(static_cast<int>(steps.size()), 8);
        for (int i = 0; i < 8; i++) {
            const ScanStep &step = steps[static_cast<size_t>(i)];
            const size_t position = static_cast<size_t>(i / 2);
            const size_t profile  = static_cast<size_t>(i % 2);
            QCOMPARE(step.generation, static_cast<quint64>(1));
            QCOMPARE(step.ptz_idx, i / 2);
            QCOMPARE(step.tcu_idx, i % 2);
            // the PTZ moves when a position begins, not for its second profile
            QCOMPARE(step.move_ptz, i % 2 == 0);
            if (step.move_ptz) {
                QCOMPARE(step.angle_h, job.ptz_route[position].first);
                QCOMPARE(step.angle_v, job.ptz_route[position].second);
            }
            QVERIFY(step.set_delay);
            QCOMPARE(step.delay_ns, job.tcu_route[profile]);
            QVERIFY(!step.set_gate_width);
        }

        const QString root = dir.path() + "/" + job.name;
        const QString profiles[] = { "2000.00 ns", "2500.00 ns" };
        for (int position = 0; position < 4; position++) {
            QVERIFY(QFile::exists(root + QString("/%1/params").arg(position)));
            for (const QString &profile : profiles) {
                const QString folder = root + QString("/%1/%2/ori_bmp").arg(position).arg(profile);
                // the files are written by the pool, after the step that asked for them
                QTRY_COMPARE_WITH_TIMEOUT(bmp_count(folder), 2, 5000);
                QVERIFY(QFile::exists(folder + "/1.bmp"));
                QVERIFY(QFile::exists(folder + "/2.bmp"));
            }
        }

        // the scan still counts as running here, since nothing has stopped it
        QTest::qWait(200);
        QCOMPARE(rig.end_count(), 1);
        QCOMPARE(rig.step_count(), 8);
    }

    // TC-PL-005: a new scan begins at its first position at once, and nothing more is
    // saved for the scan before it.
    void new_scan_begins_at_once_at_its_first_position()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        // the first scan never gets past the wait of its first profile
        const ScanJob first  = grid_job("scan_first", 1, 100000);
        const ScanJob second = grid_job("scan_second", 2, 1);
        QVERIFY(QDir(dir.path()).mkdir(first.name));
        QVERIFY(QDir(dir.path()).mkdir(second.name));

        Rig rig;
        rig.params().save_location = dir.path();
        rig.set_job(first);
        rig.start(SourceType::LOCAL_FILE, 1.0f);
        rig.push(flat_frame(90));
        QTRY_COMPARE_WITH_TIMEOUT(rig.step_count(), 1, 5000);

        rig.set_job(second);
        QTRY_COMPARE_WITH_TIMEOUT(rig.end_count(), 1, 10000);

        const std::vector<ScanStep> steps = rig.steps();
        QCOMPARE(static_cast<int>(steps.size()), 9);
        QCOMPARE(steps[0].generation, static_cast<quint64>(1));
        QCOMPARE(steps[1].generation, static_cast<quint64>(2));
        QCOMPARE(steps[1].ptz_idx, 0);
        QCOMPARE(steps[1].tcu_idx, 0);
        QVERIFY(steps[1].move_ptz);
        QCOMPARE(rig.ends()[0], static_cast<quint64>(2));

        const QString last = dir.path() + "/scan_second/3/2500.00 ns/ori_bmp";
        QTRY_COMPARE_WITH_TIMEOUT(bmp_count(last), 2, 5000);
        QCOMPARE(bmp_count(dir.path() + "/scan_first/0/2000.00 ns/ori_bmp"), 0);
    }

    // TC-PL-006: a route change closes an open recorder and announces it once, with its
    // path, and what was recorded until then is in the file.
    void route_change_closes_an_open_recorder()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.path() + "/proc.avi";

        Rig rig;
        rig.start(SourceType::NETWORK_STREAM, 10.0f);
        // One frame goes through first. The frame thread closes the recorders when it
        // meets its route for the first time, and this one must not be open by then.
        rig.push(flat_frame(20));
        QVERIFY(rig.wait_until_idle());

        DisplaySlot &display = rig.display();
        {
            // as the window starts a recording: the writer under its mutex, then the flag
            QMutexLocker lk(&display.recorder_mutex);
            display.proc_path = path;
            QVERIFY(open_recorder(display.proc_writer, path));
        }
        display.record_proc = true;
        const int greys[] = { 40, 80, 120, 160 };
        for (int grey : greys) rig.push(flat_frame(grey));
        QVERIFY(rig.wait_until_idle());
        QCOMPARE(rig.closed_recorder_count(), 0);

        rig.unroute();
        QTRY_COMPARE_WITH_TIMEOUT(rig.closed_recorder_count(), 1, 5000);
        QCOMPARE(rig.closed_recorders()[0].first, 1);
        QCOMPARE(rig.closed_recorders()[0].second, path);
        {
            QMutexLocker lk(&display.recorder_mutex);
            QVERIFY(!display.proc_writer.isOpened());
        }
        QCOMPARE(frames_in(path), 4);

        // the display stays blank, and the recorder is not announced a second time
        QTest::qWait(100);
        QCOMPARE(rig.closed_recorder_count(), 1);
    }

    // TC-PL-007: a second thread starts and stops recordings the way the window does,
    // while frames flow and the route keeps changing. It cannot prove that no race is
    // left; it is here to catch a deadlock or a crash in these paths.
    void recorder_holds_against_window_and_route_changes()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        Rig rig;
        // a frame that repeats, so the frame thread always has work
        rig.start(SourceType::LOCAL_FILE, 10.0f);
        rig.push(flat_frame(90));
        QTRY_VERIFY_WITH_TIMEOUT(rig.picture_count() > 0, 5000);

        DisplaySlot &display = rig.display();
        std::atomic<bool> stop {false};
        std::atomic<bool> window_done {false};
        std::atomic<int>  recordings {0};
        std::atomic<int>  failed_opens {0};
        std::thread window([&]() {
            while (!stop) {
                const QString path = dir.path() + QString("/%1.avi").arg(recordings.load());
                {
                    QMutexLocker lk(&display.recorder_mutex);
                    display.proc_path = path;
                    if (!open_recorder(display.proc_writer, path)) failed_opens++;
                }
                display.record_proc = true;
                // long enough for the frame thread to write a few frames into it
                std::this_thread::sleep_for(std::chrono::milliseconds(3));
                {
                    QMutexLocker lk(&display.recorder_mutex);
                    display.proc_path.clear();
                    display.proc_writer.release();
                }
                display.record_proc = false;
                recordings++;
            }
            window_done = true;
        });
        ThreadGuard guard { stop, window };

        QElapsedTimer timer;
        timer.start();
        while (timer.elapsed() < 400) {
            rig.unroute();
            QTest::qWait(2);
            rig.reroute();
            QTest::qWait(2);
        }
        stop = true;
        QTRY_VERIFY_WITH_TIMEOUT(window_done.load(), 5000);
        window.join();

        QVERIFY(recordings.load() > 0);
        QCOMPARE(failed_opens.load(), 0);
        {
            QMutexLocker lk(&display.recorder_mutex);
            QVERIFY(!display.proc_writer.isOpened());
        }
        for (int i = 0; i < recordings.load(); i++) {
            QVERIFY(QFileInfo(dir.path() + QString("/%1.avi").arg(i)).size() > 0);
        }

        // the frame thread is still at work
        const int seen = rig.picture_count();
        QTRY_VERIFY_WITH_TIMEOUT(rig.picture_count() > seen, 5000);
    }

    // TC-PL-008: apart from paint_mutex, no lock that the window shares with the frame
    // thread is held for a frame's work. Every frame is made to take 80 ms, and the test
    // takes each lock in turn, as the window would. None may keep it waiting for half a frame.
    void no_lock_is_held_for_a_frame()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.path() + "/proc.avi";

        Rig rig;
        // a frame that repeats, so every pass does a frame's work
        rig.start(SourceType::LOCAL_FILE, 10.0f);
        rig.push(flat_frame(90));
        QTRY_VERIFY_WITH_TIMEOUT(rig.picture_count() > 0, 5000);

        DisplaySlot &display = rig.display();
        {
            // a recording, so that the frame thread takes the recorder's mutex as well
            QMutexLocker lk(&display.recorder_mutex);
            display.proc_path = path;
            QVERIFY(open_recorder(display.proc_writer, path));
        }
        display.record_proc = true;

        const int hold_ms = 80;
        rig.hold_each_frame(hold_ms);
        // two pictures later, the frame in progress is a slow one
        const int before = rig.picture_count();
        QTRY_VERIFY_WITH_TIMEOUT(rig.picture_count() >= before + 2, 5000);

        struct Probe {
            const char *name;
            QMutex     *mutex;
            qint64      longest_ms;
        };
        Probe probes[] = {
            { "params mutex",   &rig.params_mutex(),      0 },
            { "ROI mutex",      &rig.roi_mutex(),         0 },
            { "label_mutex",    &display.label_mutex,     0 },
            { "gate_mutex",     &display.gate_mutex,      0 },
            { "recorder_mutex", &display.recorder_mutex,  0 },
            { "meta_mutex",     &rig.stream().meta_mutex, 0 },
        };
        const int first = rig.picture_count();
        QElapsedTimer total;
        total.start();
        while (total.elapsed() < 800) {
            for (Probe &probe : probes) {
                QElapsedTimer wait;
                wait.start();
                { QMutexLocker lk(probe.mutex); }
                probe.longest_ms = qMax(probe.longest_ms, wait.elapsed());
            }
            QTest::qSleep(2);
        }
        const qint64 elapsed_ms = total.elapsed();
        const int frames = rig.picture_count() - first;
        rig.hold_each_frame(0);

        for (const Probe &probe : probes) {
            QVERIFY2(probe.longest_ms < hold_ms / 2,
                     qPrintable(QString("%1 kept the test waiting for %2 ms")
                                .arg(QLatin1String(probe.name)).arg(probe.longest_ms)));
        }
        // the frames were slow ones, and the frame thread went on working
        QVERIFY2(frames >= 3 && frames <= elapsed_ms / hold_ms + 2,
                 qPrintable(QString("%1 frames in %2 ms").arg(frames).arg(elapsed_ms)));
    }

    // TC-PL-009: a source that ends closes an open recorder, also when it ends while the
    // frame thread is inside a frame. The window marks the source as dead before it closes
    // the queue, so the frame thread cannot go by that mark.
    void source_end_closes_an_open_recorder()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.path() + "/proc.avi";

        Rig rig;
        rig.start(SourceType::NETWORK_STREAM, 10.0f);
        // the first pass closes the recorders, so one frame goes through before one is opened
        rig.push(flat_frame(20));
        QVERIFY(rig.wait_until_idle());

        DisplaySlot &display = rig.display();
        {
            QMutexLocker lk(&display.recorder_mutex);
            display.proc_path = path;
            QVERIFY(open_recorder(display.proc_writer, path));
        }
        display.record_proc = true;
        const int greys[] = { 40, 80, 120 };
        for (int grey : greys) rig.push(flat_frame(grey));
        QVERIFY(rig.wait_until_idle());
        QCOMPARE(rig.closed_recorder_count(), 0);

        {
            // With the recorder's mutex taken, the frame thread stops where it comes to
            // write the next frame. The source then ends in the order the window uses.
            QMutexLocker lk(&display.recorder_mutex);
            rig.push(flat_frame(160));
            QVERIFY(rig.wait_until_held());
            display.source_dead = true;
            rig.stream().frames.close();
        }

        QTRY_COMPARE_WITH_TIMEOUT(rig.closed_recorder_count(), 1, 5000);
        QCOMPARE(rig.closed_recorders()[0].first, 1);
        QCOMPARE(rig.closed_recorders()[0].second, path);
        {
            QMutexLocker lk(&display.recorder_mutex);
            QVERIFY(!display.proc_writer.isOpened());
        }
        QCOMPARE(frames_in(path), 4);

        // the source stays dead, and the recorder is not announced a second time
        QTest::qWait(100);
        QCOMPARE(rig.closed_recorder_count(), 1);
    }
};

QTEST_MAIN(TestPipeline)
#include "test_pipeline.moc"
