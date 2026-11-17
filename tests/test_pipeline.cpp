#include <QtTest>

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QPixmap>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <thread>
#include <vector>
#include <opencv2/core.hpp>

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
            QMutexLocker lk(&m_recorder.mutex);
            m_recorder.pictures++;
            m_recorder.picture_size = pm.size();
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

        // The pipeline only tests this pointer and never follows it. Without it no
        // picture is sent.
        static char widget_stand_in;
        display().widget = reinterpret_cast<Display *>(&widget_stand_in);

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

    int picture_count() { QMutexLocker lk(&m_recorder.mutex); return m_recorder.pictures; }
    QSize picture_size() { QMutexLocker lk(&m_recorder.mutex); return m_recorder.picture_size; }
    int step_count() { QMutexLocker lk(&m_recorder.mutex); return static_cast<int>(m_recorder.steps.size()); }
    std::vector<ScanStep> steps() { QMutexLocker lk(&m_recorder.mutex); return m_recorder.steps; }
    std::vector<quint64> ends() { QMutexLocker lk(&m_recorder.mutex); return m_recorder.ends; }
    int end_count() { QMutexLocker lk(&m_recorder.mutex); return static_cast<int>(m_recorder.ends.size()); }
    std::vector<cv::Mat> dist_mats() { QMutexLocker lk(&m_recorder.mutex); return m_recorder.dist_mats; }

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
};

QTEST_MAIN(TestPipeline)
#include "test_pipeline.moc"
