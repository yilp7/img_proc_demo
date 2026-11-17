#include "pipeline/pipelineprocessor.h"
#include "pipeline/scanparams.h"
#include "image/imageio.h"
#include "image/imageproc.h"
#include "image/nafnetenhancer.h"
#include "util/threadpool.h"
#include "util/constants.h"
#include "util/timedqueue.h"

#include <QThread>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QCoreApplication>
#include <QDebug>
#include <QImage>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <utility>
#include <opencv2/imgproc.hpp>

PipelineProcessor::PipelineProcessor(PipelineGlobals globals, QObject *parent) :
    QObject(parent),
    m_g(globals)
{
}

// Maps a source-frame box through crop / resize / letterbox into widget coordinates.
// Returns an empty rect when the box is outside the visible region: the target
// still exists, it is just not on screen.
static QRect map_box_to_display(const cv::Rect &box,
    const ProcessingParams &params,
    const cv::Size &result_size,
    const cv::Size &render_size,
    const cv::Rect &region)
{
    if (result_size.width <= 0 || result_size.height <= 0
            || render_size.width <= 0 || render_size.height <= 0
            || params.display_width <= 0 || params.display_height <= 0
            || region.width <= 0 || region.height <= 0) {
        return QRect();
    }

    const cv::Rect image_rect(0, 0, result_size.width, result_size.height);
    const cv::Rect visible_region = region & image_rect;
    if (visible_region.width <= 0 || visible_region.height <= 0) return QRect();

    const cv::Rect clipped = box & visible_region;
    if (clipped.width <= 0 || clipped.height <= 0) return QRect();

    const double crop_scale_x = static_cast<double>(render_size.width) / visible_region.width;
    const double crop_scale_y = static_cast<double>(render_size.height) / visible_region.height;

    const double fit_scale = std::min(static_cast<double>(params.display_width) / render_size.width,
                                      static_cast<double>(params.display_height) / render_size.height);
    const int fit_w = static_cast<int>(std::round(render_size.width * fit_scale));
    const int fit_h = static_cast<int>(std::round(render_size.height * fit_scale));
    const int offset_x = (params.display_width - fit_w) / 2;
    const int offset_y = (params.display_height - fit_h) / 2;
    const double display_scale_x = static_cast<double>(fit_w) / render_size.width;
    const double display_scale_y = static_cast<double>(fit_h) / render_size.height;

    const double crop_x1 = (clipped.x - visible_region.x) * crop_scale_x;
    const double crop_y1 = (clipped.y - visible_region.y) * crop_scale_y;
    const double crop_x2 = (clipped.x + clipped.width - visible_region.x) * crop_scale_x;
    const double crop_y2 = (clipped.y + clipped.height - visible_region.y) * crop_scale_y;

    const int x1 = offset_x + static_cast<int>(std::floor(crop_x1 * display_scale_x));
    const int y1 = offset_y + static_cast<int>(std::floor(crop_y1 * display_scale_y));
    const int x2 = offset_x + static_cast<int>(std::ceil(crop_x2 * display_scale_x));
    const int y2 = offset_y + static_cast<int>(std::ceil(crop_y2 * display_scale_y));
    const QRect rect(x1, y1, x2 - x1, y2 - y1);
    const QRect display_rect(0, 0, params.display_width, params.display_height);
    if (rect.isEmpty() || !rect.intersects(display_rect)) return QRect();
    return rect;
}

// Inverse of map_box_to_display: widget coordinates back to source-frame.
// Needed because the operator designates the follow gate on screen but the
// tracker and the follow loop both work in the source frame.
static cv::Rect map_display_to_source(const QRect &rect,
    const ProcessingParams &params,
    const cv::Size &result_size,
    const cv::Size &render_size,
    const cv::Rect &region)
{
    if (rect.isEmpty()
            || result_size.width <= 0 || result_size.height <= 0
            || render_size.width <= 0 || render_size.height <= 0
            || params.display_width <= 0 || params.display_height <= 0
            || region.width <= 0 || region.height <= 0) {
        return cv::Rect();
    }

    const cv::Rect image_rect(0, 0, result_size.width, result_size.height);
    const cv::Rect visible_region = region & image_rect;
    if (visible_region.width <= 0 || visible_region.height <= 0) return cv::Rect();

    const double fit_scale = std::min(static_cast<double>(params.display_width) / render_size.width,
                                      static_cast<double>(params.display_height) / render_size.height);
    const int fit_w = static_cast<int>(std::round(render_size.width * fit_scale));
    const int fit_h = static_cast<int>(std::round(render_size.height * fit_scale));
    const int offset_x = (params.display_width - fit_w) / 2;
    const int offset_y = (params.display_height - fit_h) / 2;
    if (fit_w <= 0 || fit_h <= 0) return cv::Rect();

    const double render_x1 = (rect.left() - offset_x) * static_cast<double>(render_size.width) / fit_w;
    const double render_y1 = (rect.top() - offset_y) * static_cast<double>(render_size.height) / fit_h;
    const double render_x2 = (rect.right() - offset_x) * static_cast<double>(render_size.width) / fit_w;
    const double render_y2 = (rect.bottom() - offset_y) * static_cast<double>(render_size.height) / fit_h;

    const double crop_scale_x = static_cast<double>(visible_region.width) / render_size.width;
    const double crop_scale_y = static_cast<double>(visible_region.height) / render_size.height;

    const int x1 = visible_region.x + static_cast<int>(std::floor(render_x1 * crop_scale_x));
    const int y1 = visible_region.y + static_cast<int>(std::floor(render_y1 * crop_scale_y));
    const int x2 = visible_region.x + static_cast<int>(std::ceil(render_x2 * crop_scale_x));
    const int y2 = visible_region.y + static_cast<int>(std::ceil(render_y2 * crop_scale_y));

    return cv::Rect(x1, y1, x2 - x1, y2 - y1) & image_rect;
}

static QVector<QRect> map_yolo_boxes_to_display(const std::vector<YoloResult> &results,
    const ProcessingParams &params,
    const cv::Size &result_size,
    const cv::Size &render_size,
    const cv::Rect &region)
{
    QVector<QRect> boxes;
    if (results.empty()
            || result_size.width <= 0 || result_size.height <= 0
            || render_size.width <= 0 || render_size.height <= 0
            || params.display_width <= 0 || params.display_height <= 0
            || region.width <= 0 || region.height <= 0) {
        return boxes;
    }

    const cv::Rect image_rect(0, 0, result_size.width, result_size.height);
    const cv::Rect visible_region = region & image_rect;
    if (visible_region.width <= 0 || visible_region.height <= 0) return boxes;

    const double crop_scale_x = static_cast<double>(render_size.width) / visible_region.width;
    const double crop_scale_y = static_cast<double>(render_size.height) / visible_region.height;

    const double fit_scale = std::min(static_cast<double>(params.display_width) / render_size.width,
                                      static_cast<double>(params.display_height) / render_size.height);
    const int fit_w = static_cast<int>(std::round(render_size.width * fit_scale));
    const int fit_h = static_cast<int>(std::round(render_size.height * fit_scale));
    const int offset_x = (params.display_width - fit_w) / 2;
    const int offset_y = (params.display_height - fit_h) / 2;
    const double display_scale_x = static_cast<double>(fit_w) / render_size.width;
    const double display_scale_y = static_cast<double>(fit_h) / render_size.height;
    const QRect display_rect(0, 0, params.display_width, params.display_height);

    boxes.reserve(static_cast<int>(results.size()));
    for (const auto &det : results) {
        const cv::Rect box = det.box & visible_region;
        if (box.width <= 0 || box.height <= 0) continue;

        const double crop_x1 = (box.x - visible_region.x) * crop_scale_x;
        const double crop_y1 = (box.y - visible_region.y) * crop_scale_y;
        const double crop_x2 = (box.x + box.width - visible_region.x) * crop_scale_x;
        const double crop_y2 = (box.y + box.height - visible_region.y) * crop_scale_y;

        const int x1 = offset_x + static_cast<int>(std::floor(crop_x1 * display_scale_x));
        const int y1 = offset_y + static_cast<int>(std::floor(crop_y1 * display_scale_y));
        const int x2 = offset_x + static_cast<int>(std::ceil(crop_x2 * display_scale_x));
        const int y2 = offset_y + static_cast<int>(std::ceil(crop_y2 * display_scale_y));
        const QRect rect(x1, y1, x2 - x1, y2 - y1);
        if (!rect.isEmpty() && rect.intersects(display_rect)) boxes.push_back(rect);
    }
    return boxes;
}

int PipelineProcessor::run(DisplayRouter &router, DisplaySlot &display)
{
    display.consumer_state = true;
    bool updated = false;
    int packets_lost = 0;
    int w = 0, h = 0, pixel_depth = 8;
    float weight = 0.0f;
    int scan_img_count = -1;
    QString scan_save_path_a, scan_save_path;
    PanoramaCanvas panorama_canvas;
    qint64 panorama_position_started_ms = -1;
    ScanJob scan_job;   // this thread's copy; only the main display steps a scan
    ScanProgress scan_progress;

#ifdef LVTONG
    emit set_model_list_enabled(false);
    QString model_name;
    { QMutexLocker lk(m_g.params_mutex); switch (display.pcfg.fishnet_model_idx) {
    case 0: model_name = "models/fishnet_lvtong_legacy.onnx"; break;
    case 1: model_name = "models/fishnet_pix2pix_maxpool.onnx"; break;
    case 2: model_name = "models/fishnet_maskguided.onnx"; break;
    case 3: model_name = "models/fishnet_semantic_static_sim.onnx"; break;
    default: break;
    } }
    cv::dnn::Net net = cv::dnn::readNet(model_name.toLatin1().constData());
#endif

    while (display.consumer_run) {
        DisplayRouter::RouteSnapshot snap = router.snapshot(display.id);

        // Route change: reset per-route runtime caches. User-config (pcfg
        // toggles, model selection, thresholds) is preserved across the bump.
        if (snap.gen != display.last_route_gen) {
            display.avg_state.release_all();
            display.ecc_state.clear();
            memset(display.hist, 0, sizeof(display.hist));
            if (display.raw_writer.isOpened()) {
                display.raw_writer.release();
                emit recorder_finalized(static_cast<int>(display.id), 0, display.raw_path);
            }
            if (display.proc_writer.isOpened()) {
                display.proc_writer.release();
                emit recorder_finalized(static_cast<int>(display.id), 1, display.proc_path);
            }
            display.modified_result.release();
            // A route change means a different scene: existing tracks cannot
            // survive it, and their ids must not be reused for whatever the
            // new stream shows.
            display.tracker.reset();
            display.tracker_frame_size = cv::Size();
            display.gate_tracker.clear();
            if (display.id == DisplayId::MAIN) {
                emit main_detection_boxes_updated(QVector<QRect>());
                emit main_targets_updated(QVector<DetectedTarget>());
            }
            display.last_route_gen = snap.gen;
            // Route change clears the dead-source observation. Subsequent
            // iterations re-set it from the new stream's queue state.
            if (display.source_dead) {
                display.source_dead = false;
                emit source_state_changed(static_cast<int>(display.id), false);
            }

            if (snap.stream != StreamId::NONE) {
                StreamSlot &s = router.get_stream(snap.stream);
                QMutexLocker mlk(&s.meta_mutex);
                w = s.meta.width;
                h = s.meta.height;
                pixel_depth = s.meta.pixel_depth;
                weight = h / FONT_SCALE_DIVISOR;
                if (w > 0 && h > 0) {
                    display.avg_state.prev_3d = cv::Mat(h, w, CV_8UC3);
                    display.avg_state.prev_img = cv::Mat(h, w, CV_MAKETYPE(pixel_depth == 8 ? CV_8U : CV_16U, s.meta.is_color ? 3 : 1));
                    display.user_mask = cv::Mat::zeros(h, w, CV_8UC1);
                }
            }
        }

        if (snap.stream == StreamId::NONE) {
            QThread::msleep(QUEUE_EMPTY_SLEEP_MS);
            continue;
        }

        StreamSlot &stream = router.get_stream(snap.stream);

        // Source-died observation: closed-and-empty queue. Emit only on
        // transition; finalize the recorders the first time we see death so
        // the resulting files close cleanly.
        if (stream.frames.is_closed() && stream.frames.empty()) {
            if (!display.source_dead) {
                display.source_dead = true;
                if (display.raw_writer.isOpened()) {
                    display.raw_writer.release();
                    emit recorder_finalized(static_cast<int>(display.id), 0, display.raw_path);
                }
                if (display.proc_writer.isOpened()) {
                    display.proc_writer.release();
                    emit recorder_finalized(static_cast<int>(display.id), 1, display.proc_path);
                }
                emit source_state_changed(static_cast<int>(display.id), true);
            }
            QThread::msleep(QUEUE_EMPTY_SLEEP_MS);
            continue;
        }

        ProcessingParams params;
        PipelineConfig pcfg;
        {
            QMutexLocker lk(m_g.params_mutex);
            params = *m_g.processing_params;
            pcfg = display.pcfg;
            // the job holds routes, so it is copied only when ScanController has changed it
            if (display.id == DisplayId::MAIN && m_g.scan_job->revision != scan_job.revision) scan_job = *m_g.scan_job;
        }

        if (!acquire_frame(stream, display, pcfg, display.avg_state.prev_img, updated, packets_lost)) continue;

        if (display.source_dead) {
            display.source_dead = false;
            emit source_state_changed(static_cast<int>(display.id), false);
        }

        {
            QMutexLocker processing_locker(&display.paint_mutex);

        if (updated) {
            preprocess_frame(stream, display, params, pcfg, w, h);
        }

        std::vector<YoloResult> yolo_results = detect_yolo(stream, display, updated, pcfg);

        double is_net = 0.0;
#ifdef LVTONG
        is_net = detect_fishnet(stream, display, pcfg, net);
#endif

        int ww = 0, hh = 0;
        frame_average_and_3d(stream, display, updated, params, pcfg, w, h, pixel_depth, ww, hh);

        enhance_frame(stream, display, params, pcfg);

        QString info_tcu = pcfg.custom_topleft_info ?
                    params.custom_info_text :
                    params.tcu.base_unit == 2 ? QString::asprintf("DIST %05d m  DOV %04d m", (int)params.tcu.delay_dist, (int)params.tcu.depth_of_view) :
                                     QString::asprintf("DELAY %06d ns  GATE %04d ns", (int)std::round(params.tcu.delay_dist / params.tcu.dist_ns), (int)std::round(params.tcu.depth_of_view / params.tcu.dist_ns));
        QString info_time = QDateTime::currentDateTime().toString("hh:mm:ss:zzz");

        render_and_display(stream, display, params, pcfg, yolo_results, updated, is_net, ww, hh, w, h, weight, info_tcu, info_time);

        if (display.id == DisplayId::MAIN) {
            advance_scan_main(router, stream, display, updated, params, pcfg, scan_job, scan_progress,
                              scan_img_count, scan_save_path_a, scan_save_path,
                              panorama_canvas, panorama_position_started_ms);
        }

        record_frame(stream, display, updated, params, pcfg, is_net, info_tcu, info_time, ww, hh, weight);

        if (updated) {
            QMutexLocker mlk(&stream.meta_mutex);
            display.avg_state.prev_img = stream.latest_raw.clone();
        }
        } // QMutexLocker(paint_mutex) auto-unlocks
    }
#ifdef LVTONG
    emit set_model_list_enabled(true);
#endif
    // The CUDA session holds device memory, so it is released with the thread
    // that created it rather than left for process exit.
    delete display.nafnet;
    display.nafnet = nullptr;
    display.consumer_state = false;
    return 0;
}

bool PipelineProcessor::acquire_frame(StreamSlot &stream, DisplaySlot &display,
    const PipelineConfig &pcfg, cv::Mat &prev_img, bool &updated, int &packets_lost)
{
    // BoundedQueue handles drop-oldest internally on push.
    cv::Mat frame;
    if (!stream.frames.pop_or_timeout(frame, QUEUE_EMPTY_SLEEP_MS)) {
#ifdef LVTONG
        return false; // caller continues (skip iteration)
#else
        QMutexLocker mlk(&stream.meta_mutex);
        stream.latest_raw = prev_img.clone();
        updated = false;
        return true;
#endif
    }

    int popped_frames = 1;
    const bool low_latency_ecc = pcfg.frame_avg_enabled
        && pcfg.frame_avg_mode == 1
        && !source_repush_on_pause(stream.meta.source_type);
    if (low_latency_ecc) {
        cv::Mat newer_frame;
        while (stream.frames.try_pop(newer_frame)) {
            frame = std::move(newer_frame);
            ++popped_frames;
        }
    }

    {
        QMutexLocker mlk(&stream.meta_mutex);
        stream.latest_raw = frame;
    }

    int packets_lost_frame = 0;
    if (source_has_frame_info(stream.meta.source_type)) {
        for (int i = 0; i < popped_frames; ++i) {
            int info = 0;
            if (!stream.frame_info.try_pop(info)) break;
            packets_lost_frame += info;
        }
    }
    packets_lost += packets_lost_frame;
    if (packets_lost_frame) emit packet_lost_updated(packets_lost);
    updated = true;
    if (source_repush_on_pause(stream.meta.source_type)) {
        stream.frames.push(frame.clone());
    }
    if (display.fps_calc) display.fps_calc->add_to_q();
    return true;
}

void PipelineProcessor::preprocess_frame(StreamSlot &stream, DisplaySlot &display,
    const ProcessingParams &params, const PipelineConfig &pcfg,
    int &w, int &h)
{
    cv::Mat &latest = stream.latest_raw;
    int meta_w = 0, meta_h = 0;
    bool meta_color = false;
    int meta_depth = 8;
    { QMutexLocker mlk(&stream.meta_mutex);
      meta_w = stream.meta.width; meta_h = stream.meta.height;
      meta_color = stream.meta.is_color; meta_depth = stream.meta.pixel_depth; }
    (void)meta_color; (void)meta_depth;

    switch (params.image_rotate) {
    case 0:
        if (w != meta_w) display.avg_state.release_all();
        w = meta_w; h = meta_h;
        break;
    case 2:
        cv::flip(latest, latest, -1);
        if (w != meta_w) display.avg_state.release_all();
        w = meta_w; h = meta_h;
        break;
    case 1:
        cv::flip(latest, latest, -1);
    case 3:
        cv::flip(latest, latest, 0);
        cv::transpose(latest, latest);
        if (w != meta_h) display.avg_state.release_all();
        w = meta_h; h = meta_w;
        break;
    default:
        break;
    }

    // calc histogram (grayscale)
    memset(display.hist, 0, 256 * sizeof(uint));
    if (!meta_color) for (int i = 0; i < h; i++) for (int j = 0; j < w; j++) display.hist[(latest.data + i * latest.cols)[j]]++;

    // if the image needs flipping
    { int sym = pcfg.flip; if (sym) cv::flip(latest, latest, sym - 2); }

    // mcp self-adaptive
    if (params.tcu.auto_mcp && !params.mcp_slider_focused) {
        int thresh_num = latest.total() / AUTO_MCP_DIVISOR, thresh = (1 << meta_depth) - 1;
        while (thresh && thresh_num > 0) thresh_num -= display.hist[thresh--];
        if (thresh > (1 << meta_depth) * 0.94) emit update_mcp_in_thread(params.tcu.mcp - sqrt(thresh - (1 << meta_depth) * 0.94));
        if (thresh < (1 << meta_depth) * 0.39) emit update_mcp_in_thread(params.tcu.mcp + sqrt((1 << meta_depth) * 0.39 - thresh));
    }
}

std::vector<YoloResult> PipelineProcessor::detect_yolo(StreamSlot &stream, DisplaySlot &display,
    bool updated, const PipelineConfig &pcfg)
{
    int current_model_selection = pcfg.model_idx;
    YoloDetector *yolo = display.yolo;

    if (current_model_selection != display.yolo_last_model) {
        QMutexLocker locker(m_g.yolo_init_mutex);

        if (display.yolo) {
            qDebug() << "YOLO model changed for display" << static_cast<int>(display.id) << "- reinitializing";
            delete display.yolo;
            display.yolo = nullptr;
            yolo = nullptr;
        }

        if (current_model_selection > 0) {
            display.yolo = new YoloDetector();
            QString app_dir = QCoreApplication::applicationDirPath() + "/";
            QString cfg_path = app_dir + pcfg.config_path;

            QString model_path, classes_file;
            switch (current_model_selection) {
            case 1:
                model_path = app_dir + pcfg.visible_model_path;
                classes_file = app_dir + pcfg.visible_classes_file;
                break;
            case 2:
                model_path = app_dir + pcfg.thermal_model_path;
                classes_file = app_dir + pcfg.thermal_classes_file;
                break;
            case 3:
                model_path = app_dir + pcfg.gated_model_path;
                classes_file = app_dir + pcfg.gated_classes_file;
                break;
            }

            if (!model_path.isEmpty() && display.yolo->initialize(cfg_path, model_path, classes_file)) {
                yolo = display.yolo;
                qDebug() << "YOLO detector reloaded for display" << static_cast<int>(display.id) << "with model" << current_model_selection;
            }
            else {
                qWarning() << "Failed to reload YOLO for display" << static_cast<int>(display.id) << "- will not retry";
                delete display.yolo;
                display.yolo = nullptr;
                yolo = nullptr;
            }
        }

        display.yolo_last_model = current_model_selection;
        int state;
        if      (current_model_selection == 0) state = 0;
        else if (display.yolo)                 state = 1;
        else                                   state = 2;
        emit yolo_status_changed(static_cast<int>(display.id), state);
    }

    std::vector<YoloResult> results;
    if (yolo && yolo->is_initialized() && updated) {
        cv::Mat det_input;
        if (stream.latest_raw.channels() == 1) {
            cv::cvtColor(stream.latest_raw, det_input, cv::COLOR_GRAY2BGR);
        }
        else {
            det_input = stream.latest_raw;
        }
        yolo->run(det_input, results);
        display.tracker.update(results);
        display.tracker_frame_size = cv::Size(det_input.cols, det_input.rows);
    }
    return results;
}

void PipelineProcessor::frame_average_and_3d(StreamSlot &stream, DisplaySlot &display,
    bool updated, const ProcessingParams &params, const PipelineConfig &pcfg,
    int w, int h, int pixel_depth, int &ww, int &hh)
{
    cv::Mat &latest = stream.latest_raw;
    cv::Mat &result = display.modified_result;
    FrameAverageRuntime &avg_state = display.avg_state;
    ECCRuntime &ecc_state = display.ecc_state;
    bool is_color = false;
    { QMutexLocker mlk(&stream.meta_mutex); is_color = stream.meta.is_color; }

    if (avg_state.seq_sum.empty()) avg_state.seq_sum = cv::Mat::zeros(h, w, CV_MAKETYPE(CV_16U, is_color ? 3 : 1));
    if (avg_state.frame_a_sum.empty()) avg_state.frame_a_sum = cv::Mat::zeros(h, w, CV_MAKETYPE(CV_16U, is_color ? 3 : 1));
    if (avg_state.frame_b_sum.empty()) avg_state.frame_b_sum = cv::Mat::zeros(h, w, CV_MAKETYPE(CV_16U, is_color ? 3 : 1));
    if (pcfg.frame_avg_enabled) {
        int avg_mode = pcfg.frame_avg_mode;

        if (updated) {
            if (avg_state.seq[7].empty()) for (auto &m: avg_state.seq) m = cv::Mat::zeros(h, w, CV_MAKETYPE(CV_16U, is_color ? 3 : 1));

            avg_state.seq_sum -= avg_state.seq[(avg_state.seq_idx + 4) & 7];
            if (avg_state.frame_a) avg_state.frame_a_sum -= avg_state.seq[avg_state.seq_idx];
            else                   avg_state.frame_b_sum -= avg_state.seq[avg_state.seq_idx];
            latest.convertTo(avg_state.seq[avg_state.seq_idx], CV_MAKETYPE(CV_16U, is_color ? 3 : 1));
            avg_state.seq_sum += avg_state.seq[avg_state.seq_idx];
            if (avg_state.frame_a) avg_state.frame_a_sum += avg_state.seq[avg_state.seq_idx];
            else                   avg_state.frame_b_sum += avg_state.seq[avg_state.seq_idx];

            avg_state.seq_idx = (avg_state.seq_idx + 1) & 7;
            avg_state.frame_a ^= 1;
        }

        if (avg_mode == 0) {
            avg_state.seq_sum.convertTo(result, CV_MAKETYPE(CV_8U, is_color ? 3 : 1), 1. / (4 * (1 << (pixel_depth - 8))));
        }
        else if (avg_mode == 2) {
            // Learned single-frame denoise: it does not accumulate, so it runs on the current
            // frame alone. The sequence sums above are still kept because the 3D stage reads them.
            if (updated) {
                if (!display.nafnet) {
                    display.nafnet = new NafNetEnhancer();
                    // The GUI hides this option unless the model and the CUDA
                    // provider are both present, so a failure here means they
                    // went away after startup or the CUDA libraries do not load.
                    if (!display.nafnet->ensure_loaded(NafNetEnhancer::default_model_path())) {
                        qWarning() << "LightNAFNet unavailable for display"
                                   << static_cast<int>(display.id)
                                   << "-" << display.nafnet->get_last_error();
                    }
                }
                latest.convertTo(result, CV_MAKETYPE(CV_8U, is_color ? 3 : 1),
                                 1. / (1 << (pixel_depth - 8)));
                display.nafnet->enhance(result);
            }
        }
        else {
            if (updated) {
                if (pcfg.ecc_warp_mode != ecc_state.prev_ecc_warp_mode) {
                    ecc_state.clear();
                    ecc_state.prev_ecc_warp_mode = pcfg.ecc_warp_mode;
                }

                cv::Mat gray;
                if (is_color) {
                    cv::Mat tmp;
                    latest.convertTo(tmp, CV_MAKETYPE(CV_8U, 3), 1. / (1 << (pixel_depth - 8)));
                    cv::cvtColor(tmp, gray, cv::COLOR_BGR2GRAY);
                    gray.convertTo(gray, CV_32F);
                }
                else {
                    latest.convertTo(gray, CV_32F, 255.0 / ((1 << pixel_depth) - 1));
                }

                if (!ecc_state.fusion_buf.empty()) {
                    cv::Mat warp, warp_inv;
                    ImageProc::ecc_register_consecutive(
                        ecc_state.fusion_buf.back(), gray,
                        warp, warp_inv,
                        ecc_state.fusion_last_warp,
                        pcfg.ecc_levels, pcfg.ecc_max_iter, pcfg.ecc_eps,
                        pcfg.ecc_half_res_reg,
                        pcfg.ecc_warp_mode);
                    ecc_state.fusion_warps.push_back(warp);
                    ecc_state.fusion_warps_inv.push_back(warp_inv);
                    ecc_state.fusion_last_warp = warp.clone();
                }

                int ecc_fwd = (pcfg.ecc_window_mode == 1) ? pcfg.ecc_backward
                            : (pcfg.ecc_window_mode == 2) ? pcfg.ecc_forward : 0;

                ecc_state.fusion_buf.push_back(gray);
                int max_buf = pcfg.ecc_backward + 1 + ecc_fwd;
                while ((int)ecc_state.fusion_buf.size() > max_buf) {
                    ecc_state.fusion_buf.pop_front();
                    if (!ecc_state.fusion_warps.empty()) ecc_state.fusion_warps.pop_front();
                    if (!ecc_state.fusion_warps_inv.empty()) ecc_state.fusion_warps_inv.pop_front();
                }

                int min_frames = (ecc_fwd > 0) ? pcfg.ecc_backward + 1 + ecc_fwd : 2;
                if ((int)ecc_state.fusion_buf.size() >= min_frames) {
                    int target = (int)ecc_state.fusion_buf.size() - 1 - ecc_fwd;
                    ImageProc::temporal_denoise_fuse(
                        ecc_state.fusion_buf, ecc_state.fusion_warps, ecc_state.fusion_warps_inv,
                        target, pcfg.ecc_backward, ecc_fwd,
                        result,
                        pcfg.ecc_half_res_fuse,
                        pcfg.ecc_warp_mode,
                        pcfg.ecc_fusion_method);
                }
                else {
                    int show_idx = std::max(0, (int)ecc_state.fusion_buf.size() - 1 - ecc_fwd);
                    ecc_state.fusion_buf[show_idx].convertTo(result, CV_8U);
                }
            }
        }
    }
    else {
        if (!avg_state.seq_sum.empty()) avg_state.release_avg_buffers();
        if (!ecc_state.fusion_buf.empty()) ecc_state.clear();
        latest.convertTo(result, CV_MAKETYPE(CV_8U, is_color ? 3 : 1), 1. / (1 << (pixel_depth - 8)));
    }

    if (params.split) ImageProc::split_img(result, result);

    // 3D image construction: main display only, grayscale only.
    if (pcfg.enable_3d && display.id == DisplayId::MAIN && !is_color) {
        ww = w + 104;
        hh = h;
        cv::Mat dist_mat;
        double dist_min = 0, dist_max = 0;
        if (updated) {
            const TcuReadout &tcu = params.tcu;
            // read once, so both images of a frame are chosen by the same value
            const bool frame_a_3d = display.frame_a_3d;
            if (pcfg.frame_avg_enabled) {
                cv::Mat frame_a_avg, frame_b_avg;
                avg_state.frame_a_sum.convertTo(frame_a_avg, pixel_depth > 8 ? CV_16U : CV_8U, 0.25);
                avg_state.frame_b_sum.convertTo(frame_b_avg, pixel_depth > 8 ? CV_16U : CV_8U, 0.25);
#ifdef LVTONG
                ImageProc::gated_3d_v2(frame_a_3d ? frame_b_avg : frame_a_avg, frame_a_3d ? frame_a_avg : frame_b_avg, result,
                                       pcfg.custom_3d_param ? pcfg.custom_3d_delay : (tcu.delay_dist - tcu.offset_delay * tcu.dist_ns) / tcu.dist_ns,
                                       pcfg.custom_3d_param ? pcfg.custom_3d_gate_width : (tcu.depth_of_view - tcu.offset_gw * tcu.dist_ns) / tcu.dist_ns,
                                       pcfg.colormap, pcfg.lower_3d_thresh, pcfg.upper_3d_thresh, pcfg.truncate_3d);
#else
                ImageProc::gated_3d_v2(frame_a_3d ? frame_b_avg : frame_a_avg, frame_a_3d ? frame_a_avg : frame_b_avg, result,
                                       pcfg.custom_3d_param ? pcfg.custom_3d_delay : (tcu.delay_dist - tcu.offset_delay * tcu.dist_ns) / tcu.dist_ns,
                                       pcfg.custom_3d_param ? pcfg.custom_3d_gate_width : (tcu.depth_of_view - tcu.offset_gw * tcu.dist_ns) / tcu.dist_ns,
                                       pcfg.colormap, pcfg.lower_3d_thresh, pcfg.upper_3d_thresh, pcfg.truncate_3d, &dist_mat, &dist_min, &dist_max);
#endif
            }
            else {
#ifdef LVTONG
                ImageProc::gated_3d_v2(frame_a_3d ? avg_state.prev_img : latest, frame_a_3d ? latest : avg_state.prev_img, result,
                                       pcfg.custom_3d_param ? pcfg.custom_3d_delay : (tcu.delay_dist - tcu.offset_delay * tcu.dist_ns) / tcu.dist_ns,
                                       pcfg.custom_3d_param ? pcfg.custom_3d_gate_width : (tcu.depth_of_view - tcu.offset_gw * tcu.dist_ns) / tcu.dist_ns,
                                       pcfg.colormap, pcfg.lower_3d_thresh, pcfg.upper_3d_thresh, pcfg.truncate_3d);
#else
                ImageProc::gated_3d_v2(frame_a_3d ? avg_state.prev_img : latest, frame_a_3d ? latest : avg_state.prev_img, result,
                                       pcfg.custom_3d_param ? pcfg.custom_3d_delay : (tcu.delay_dist - tcu.offset_delay * tcu.dist_ns) / tcu.dist_ns,
                                       pcfg.custom_3d_param ? pcfg.custom_3d_gate_width : (tcu.depth_of_view - tcu.offset_gw * tcu.dist_ns) / tcu.dist_ns,
                                       pcfg.colormap, pcfg.lower_3d_thresh, pcfg.upper_3d_thresh, pcfg.truncate_3d, &dist_mat, &dist_min, &dist_max);
#endif
                // compare-exchange, so a swap the GUI makes during this frame is not lost
                bool expected = display.frame_a_3d.load();
                while (!display.frame_a_3d.compare_exchange_weak(expected, !expected));
            }
#ifndef LVTONG
            avg_state.prev_3d = result.clone();
#endif
        }
        else result = avg_state.prev_3d;
#ifndef LVTONG
        cv::Mat masked_dist;
        {
            QMutexLocker lk(m_g.roi_mutex);
            if (m_g.list_roi->size()) dist_mat.copyTo(masked_dist, display.user_mask);
        }
        if (!masked_dist.empty()) emit update_dist_mat(masked_dist, dist_min, dist_max);
        else emit update_dist_mat(dist_mat, dist_min, dist_max);
#endif
    }
    else {
        ww = w; hh = h;
    }
}

#ifdef LVTONG
double PipelineProcessor::detect_fishnet(StreamSlot &stream, DisplaySlot &display,
    const PipelineConfig &pcfg, cv::dnn::Net &net)
{
    (void)display;
    cv::Mat &latest = stream.latest_raw;
    double is_net = 0;
    double min, max;
    if (pcfg.fishnet_recog) {
        cv::Mat fishnet_res;
        switch (pcfg.fishnet_model_idx) {
        case 0:
        {
            cv::cvtColor(latest, fishnet_res, cv::COLOR_GRAY2RGB);
            fishnet_res.convertTo(fishnet_res, CV_32FC3, 1.0 / 255);
            cv::resize(fishnet_res, fishnet_res, cv::Size(224, 224));

            cv::Mat blob = cv::dnn::blobFromImage(fishnet_res, 1.0, cv::Size(224, 224));
            net.setInput(blob);
            cv::Mat prob = net.forward("195");

            cv::minMaxLoc(prob, &min, &max);
            prob -= max;
            is_net = exp(prob.at<float>(1)) / (exp(prob.at<float>(0)) + exp(prob.at<float>(1)));

            emit update_fishnet_result(is_net > pcfg.fishnet_thresh);
            break;
        }
        case 1:
        case 2:
        {
            latest.convertTo(fishnet_res, CV_32FC1, 1.0 / 255);
            cv::resize(fishnet_res, fishnet_res, cv::Size(256, 256));
            cv::Mat blob = cv::dnn::blobFromImage(fishnet_res, 1.0, cv::Size(256, 256));
            net.setInput(blob);
            cv::Mat prob = net.forward();
            cv::minMaxLoc(prob, &min, &max);
            emit update_fishnet_result(prob.at<float>(1) > prob.at<float>(0));
            break;
        }
        case 3:
        {
            latest.convertTo(fishnet_res, CV_32FC1, 1.0 / 255);
            cv::resize(fishnet_res, fishnet_res, cv::Size(224, 224));
            cv::Mat blob = cv::dnn::blobFromImage(fishnet_res, 1.0, cv::Size(224, 224), cv::Scalar(0.5));
            net.setInput(blob);
            cv::Mat prob = net.forward();
            cv::minMaxLoc(prob, &min, &max);
            emit update_fishnet_result(prob.at<float>(1) > prob.at<float>(0));
            break;
        }
        default: break;
        }
    }
    else emit update_fishnet_result(-1);
    return is_net;
}
#endif

void PipelineProcessor::enhance_frame(StreamSlot &stream, DisplaySlot &display,
    const ProcessingParams &params, const PipelineConfig &pcfg)
{
    cv::Mat &result = display.modified_result;
    bool is_color = false;
    { QMutexLocker mlk(&stream.meta_mutex); is_color = stream.meta.is_color; }

    // Skip enhance when 3D is active on main display with grayscale source.
    if (!pcfg.enable_3d || display.id != DisplayId::MAIN || is_color) {
        if (pcfg.enhance_enabled) {
            switch (pcfg.enhance_option) {
            case 1: ImageProc::hist_equalization(result, result); break;
            case 2: {
                cv::Mat kernel = (cv::Mat_<float>(3, 3) << 0, -1, 0, 0, 5, 0, 0, -1, 0);
                cv::filter2D(result, result, CV_8U, kernel);
                break;
            }
            case 3: ImageProc::plateau_equl_hist(result, result, 4); break;
            case 4: ImageProc::accumulative_enhance(result, result, pcfg.accu_base); break;
            case 5: ImageProc::guided_image_filter(result, result, 60, 0.01, 1); break;
            case 6: ImageProc::adaptive_enhance(result, result, pcfg.low_in, pcfg.high_in, pcfg.low_out, pcfg.high_out, pcfg.gamma); break;
            case 7:
                result = ~result;
                ImageProc::haze_removal(result, result, 7, pcfg.dehaze_pct, 0.1, 60, 0.01);
                result = ~result;
                break;
            case 8: ImageProc::haze_removal(result, result, 7, pcfg.dehaze_pct, 0.1, 60, 0.01); break;
            case 9: ImageProc::aindane(result, result); break;
            default: break;
            }
        }

        // brightness / contrast / gamma sliders are not in the current GUI; re-enable when restored.
        // ImageProc::brightness_and_contrast(result, result, std::exp(params.contrast / 10.), params.brightness * 12.8);
        // ImageProc::brightness_and_contrast(result, result, tan((20 - params.gamma_slider) / 40. * M_PI));
    }

    // Pseudocolor on main display only.
    if (pcfg.pseudocolor_enabled && display.id == DisplayId::MAIN && result.channels() == 1) {
        cv::Mat mask;
        cv::normalize(result, result, 0, 255, cv::NORM_MINMAX, CV_8UC1, mask);
        cv::Mat temp_mask = result, result_3d;
        cv::applyColorMap(result, result_3d, pcfg.colormap);
        result_3d.copyTo(result, temp_mask);
    }
}

void PipelineProcessor::render_and_display(StreamSlot &stream, DisplaySlot &display,
    const ProcessingParams &params, const PipelineConfig &pcfg,
    const std::vector<YoloResult> &yolo_results,
    bool updated, double is_net, int ww, int hh, int w, int h, float weight,
    const QString &info_tcu, const QString &info_time)
{
    (void)info_tcu;
    cv::Mat &result = display.modified_result;
    bool is_color = false;
    { QMutexLocker mlk(&stream.meta_mutex); is_color = stream.meta.is_color; }

    if (!yolo_results.empty()) draw_yolo_boxes(display, result, yolo_results);

    // Histogram side-output: gray-value or 3D distance hist.
    if (params.alt_display_option == 2) {
        cv::Mat mat_for_hist;
        if (!is_color && !pcfg.pseudocolor) mat_for_hist = result.clone();
        else                                   cv::cvtColor(result, mat_for_hist, cv::COLOR_RGB2GRAY);
        for (int i = 0; i < h; i++) for (int j = 0; j < w; j++) display.hist[(mat_for_hist.data + i * mat_for_hist.cols)[j]]++;

        uchar *img = mat_for_hist.data;
        const std::size_t step = mat_for_hist.step;
        memset(display.hist, 0, 256 * sizeof(uint));
        for (int i = 0; i < hh; i++) for (int j = 0; j < ww; j++) display.hist[(img + i * step)[j]]++;
        uint max = 0;
        for (int i = 1; i < 256; i++) if (display.hist[i] > max) max = display.hist[i];
        cv::Mat hist_mat = cv::Mat(225, 256, CV_8UC3, cv::Scalar(56, 64, 72));
        for (int i = 1; i < 256; i++) {
            cv::rectangle(hist_mat, cv::Point(i, 225), cv::Point(i + 1, 225 - display.hist[i] * 225.0 / max), cv::Scalar(202, 225, 255));
        }
        emit set_hist_pixmap(QPixmap::fromImage(QImage(hist_mat.data, hist_mat.cols, hist_mat.rows, static_cast<int>(hist_mat.step), QImage::Format_RGB888).scaled(params.hist_display_size, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
    }

    // The snapshot carries a zero size while the main display is being laid
    // out or is hidden. The crop below divides by it.
    if (params.display_width <= 0 || params.display_height <= 0) return;

    cv::Rect region(cv::Point(params.display_region.tl().x * ww / params.display_width,
                              params.display_region.tl().y * hh / params.display_height),
                    cv::Point(params.display_region.br().x * ww / params.display_width,
                              params.display_region.br().y * hh / params.display_height));
    if (region.height + region.y > result.rows) region.height = result.rows - region.y;
    if (region.width + region.x > result.cols) region.width = result.cols - region.x;
    if (display.id == DisplayId::MAIN && updated) {
        // Follow gate. The request is in widget coordinates because the operator drew it
        // there; it is converted here, where the render geometry is known, and tracked in
        // source-frame coordinates like the rest of the follow loop.
        QRect requested;
        bool has_request = false;
        GateTracker::Settings gate_settings;
        {
            QMutexLocker lk(&display.gate_mutex);
            gate_settings = display.gate_settings;
            if (display.gate_request_pending) {
                requested = display.gate_request;
                has_request = true;
                display.gate_request_pending = false;
                display.gate_request = QRect();
            }
        }
        // Applied before the seed so a request lands on the selected method.
        // Switching method drops any live track (set_settings does that).
        display.gate_tracker.set_settings(gate_settings);

        cv::Mat gate_gray;
        if (!stream.latest_raw.empty()) {
            if (stream.latest_raw.channels() == 1) gate_gray = stream.latest_raw;
            else cv::cvtColor(stream.latest_raw, gate_gray, cv::COLOR_BGR2GRAY);
        }

        if (has_request) {
            if (requested.isEmpty()) {
                display.gate_tracker.clear();
            }
            else {
                const cv::Rect seed = map_display_to_source(
                    requested, params, cv::Size(result.cols, result.rows),
                    cv::Size(ww, hh), region);
                display.gate_tracker.seed(gate_gray, seed);
            }
        }
        else if (display.gate_tracker.is_active()) {
            display.gate_tracker.update(gate_gray);
        }

        QRect gate_display;
        if (display.gate_tracker.is_active()) {
            const cv::Rect gate = display.gate_tracker.get_box();
            gate_display = map_box_to_display(gate, params, cv::Size(result.cols, result.rows),
                                              cv::Size(ww, hh), region);
        }
        emit follow_gate_updated(gate_display,
                                 QRect(display.gate_tracker.get_box().x, display.gate_tracker.get_box().y,
                                       display.gate_tracker.get_box().width,
                                       display.gate_tracker.get_box().height),
                                 QSize(gate_gray.cols, gate_gray.rows),
                                 display.gate_tracker.is_active());
    }

    QVector<QRect> main_detection_boxes;
    QVector<DetectedTarget> main_targets;
    if (display.id == DisplayId::MAIN && updated) {
        main_detection_boxes = map_yolo_boxes_to_display(
            yolo_results, params, cv::Size(result.cols, result.rows),
            cv::Size(ww, hh), region);

        const cv::Size source_size = display.tracker_frame_size;
        for (const TargetTrack &track : display.tracker.get_tracks()) {
            DetectedTarget target;
            target.trackId = track.id;
            target.classId = track.class_id;
            target.modelIdx = pcfg.model_idx;
            if (display.yolo && track.class_id >= 0
                    && track.class_id < static_cast<int>(display.yolo->get_class_names().size())) {
                target.className = QString::fromStdString(
                            display.yolo->get_class_names()[track.class_id]);
            }
            target.confidence = track.confidence;
            target.confirmed = track.confirmed;
            target.lost = track.just_lost;
            target.sourceBox = QRect(track.box.x, track.box.y, track.box.width, track.box.height);
            target.sourceSize = QSize(source_size.width, source_size.height);
            target.displayBox = map_box_to_display(track.box, params,
                                                   cv::Size(result.cols, result.rows),
                                                   cv::Size(ww, hh), region);
            main_targets.push_back(target);
        }
    }
    result = result(region);
    cv::resize(result, result, cv::Size(ww, hh));

    if (params.show_info) {
        cv::putText(result, info_time.toLatin1().data(), cv::Point(ww - 240 * weight, 50 * weight), cv::FONT_HERSHEY_SIMPLEX, weight, cv::Scalar(255), weight * 2);
#ifdef LVTONG
        int baseline = 0;
        if (pcfg.fishnet_recog) {
            cv::putText(result, is_net > pcfg.fishnet_thresh ? "FISHNET FOUND" : "FISHNET NOT FOUND", cv::Point(ww - 40 - cv::getTextSize(is_net > pcfg.fishnet_thresh ? "FISHNET FOUND" : "FISHNET NOT FOUND", cv::FONT_HERSHEY_SIMPLEX, weight, weight * 2, &baseline).width, hh - 100 + 50 * weight), cv::FONT_HERSHEY_SIMPLEX, weight, cv::Scalar(255), weight * 2);
        }
#endif
    }

    cv::Mat img_display;
    {
        const int src_w = result.cols;
        const int src_h = result.rows;
        const int disp_w = params.display_width;
        const int disp_h = params.display_height;
        const double r = std::min((double)disp_w / src_w, (double)disp_h / src_h);
        const int fit_w = (int)std::round(src_w * r);
        const int fit_h = (int)std::round(src_h * r);
        const int chans = result.channels();
        img_display = cv::Mat::zeros(disp_h, disp_w, CV_MAKETYPE(CV_8U, chans));
        cv::Mat fit_roi = img_display(cv::Rect((disp_w - fit_w) / 2, (disp_h - fit_h) / 2, fit_w, fit_h));
        cv::resize(result, fit_roi, cv::Size(fit_w, fit_h), 0, 0, cv::INTER_AREA);
    }
    bool use_mask = result.channels() == 1;
    cv::Mat info_mask(cv::Size(params.display_width, params.display_height), CV_8U, cv::Scalar(0)), thresh_result;
    if (use_mask) cv::threshold(img_display, thresh_result, 128, 255, cv::THRESH_BINARY);
    if (!pcfg.image_3d && params.show_center) {
        cv::line(use_mask ? info_mask : img_display, cv::Point(info_mask.cols / 2 - 9, info_mask.rows / 2), cv::Point(info_mask.cols / 2 + 10, info_mask.rows / 2), cv::Scalar(255), 1);
        cv::line(use_mask ? info_mask : img_display, cv::Point(info_mask.cols / 2, info_mask.rows / 2 - 9), cv::Point(info_mask.cols / 2, info_mask.rows / 2 + 10), cv::Scalar(255), 1);
    }
    if (params.select_tool && params.selection_v1 != params.selection_v2) {
        cv::rectangle(use_mask ? info_mask : img_display, params.selection_v1, params.selection_v2, cv::Scalar(255));
        cv::circle(use_mask ? info_mask : img_display, (params.selection_v1 + params.selection_v2) / 2, 1, cv::Scalar(255), -1);
    }
    if (use_mask) img_display = (img_display & (~info_mask)) + ((~thresh_result) & info_mask);

    QImage stream_img = QImage(img_display.data, img_display.cols, img_display.rows, static_cast<int>(img_display.step), is_color || pcfg.pseudocolor ? QImage::Format_RGB888 : QImage::Format_Indexed8);
    // a display without a widget shows nothing, so nothing is sent for it
    const int display_idx = static_cast<int>(display.id);
    if (display.widget) emit display_frame_ready(display_idx, QPixmap::fromImage(stream_img.scaled(params.display_size[display_idx], Qt::KeepAspectRatio, Qt::SmoothTransformation)));
    if (display.id == DisplayId::MAIN && updated) {
        emit main_detection_boxes_updated(main_detection_boxes);
        emit main_targets_updated(main_targets);
    }

    if (display.id == DisplayId::MAIN && params.dual_display) {
        cv::Mat ori_img_display;
        const QSize ori_size = params.secondary_size;
        cv::resize(stream.latest_raw, ori_img_display, cv::Size(ori_size.width(), ori_size.height()), 0, 0, cv::INTER_AREA);
        QImage ori_img = QImage(ori_img_display.data, ori_img_display.cols, ori_img_display.rows, static_cast<int>(ori_img_display.step), is_color ? QImage::Format_RGB888 : QImage::Format_Indexed8);
        emit secondary_frame_ready(QPixmap::fromImage(ori_img).scaled(ori_size, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

namespace {

const int PANORAMA_MAX_OUTPUT_WIDTH = 24000;
const int PANORAMA_FEATHER_PX = 48;
const int PANORAMA_GAIN_MIN_OVERLAP = 64;
const double PANORAMA_WEIGHT_EPS = 1e-6;
const double PANORAMA_GAIN_MIN = 0.7;
const double PANORAMA_GAIN_MAX = 1.3;

double normalize_angle_mod(double value)
{
    double result = std::fmod(value, 360.0);
    if (result < 0.0) result += 360.0;
    return result;
}

double normalize_angle_delta(double delta)
{
    while (delta > 180.0) delta -= 360.0;
    while (delta <= -180.0) delta += 360.0;
    return delta;
}

std::vector<double> unwrap_panorama_angles(const std::vector<std::pair<float, float>> &route,
                                           unsigned int direction)
{
    std::vector<double> angles;
    if (route.empty()) return angles;

    angles.reserve(route.size());
    angles.push_back(static_cast<double>(route.front().first));
    for (size_t i = 1; i < route.size(); ++i) {
        double delta = normalize_angle_delta(static_cast<double>(route[i].first)
                                             - normalize_angle_mod(angles.back()));
        if (direction == 0 && delta < 0.0) delta += 360.0;
        if (direction != 0 && delta > 0.0) delta -= 360.0;
        angles.push_back(angles.back() + delta);
    }
    return angles;
}

cv::Mat panorama_source_rgb8(const cv::Mat &input)
{
    if (input.empty()) return cv::Mat();

    cv::Mat src = input.clone();
    if (src.depth() != CV_8U) {
        double min_val = 0.0;
        double max_val = 0.0;
        cv::Mat flat = src.reshape(1);
        cv::minMaxLoc(flat, &min_val, &max_val);
        const double scale = (max_val > min_val) ? (255.0 / (max_val - min_val)) : 1.0;
        src.convertTo(src, CV_MAKETYPE(CV_8U, src.channels()), scale, -min_val * scale);
    }

    cv::Mat rgb;
    if (src.channels() == 1) {
        cv::cvtColor(src, rgb, cv::COLOR_GRAY2RGB);
    }
    else if (src.channels() == 3) {
        rgb = src;
    }
    else if (src.channels() == 4) {
        cv::cvtColor(src, rgb, cv::COLOR_RGBA2RGB);
    }
    return rgb;
}

cv::Mat make_panorama_feather_mask(int width, int height)
{
    cv::Mat mask(height, width, CV_32FC1, cv::Scalar(1.0f));
    const int feather = std::max(0, std::min(PANORAMA_FEATHER_PX, width / 2));
    if (feather <= 0) return mask;

    for (int y = 0; y < height; ++y) {
        float *row = mask.ptr<float>(y);
        for (int x = 0; x < feather; ++x) {
            const float value = feather > 1
                ? static_cast<float>(x) / static_cast<float>(feather - 1)
                : 0.0f;
            row[x] *= value;
            row[width - 1 - x] *= value;
        }
    }
    return mask;
}

bool clipped_panorama_rect(const PanoramaCanvas &canvas, const cv::Mat &src,
                           int x, int y, cv::Rect &dst_rect, cv::Rect &src_rect)
{
    const int x0 = std::max(0, x);
    const int y0 = std::max(0, y);
    const int x1 = std::min(canvas.canvas_width, x + src.cols);
    const int y1 = std::min(canvas.canvas_height, y + src.rows);
    if (x1 <= x0 || y1 <= y0) return false;

    dst_rect = cv::Rect(x0, y0, x1 - x0, y1 - y0);
    src_rect = cv::Rect(x0 - x, y0 - y, x1 - x0, y1 - y0);
    return true;
}

cv::Mat overlap_mask_from_weights(const cv::Mat &mask_roi, const cv::Mat &weights_roi)
{
    cv::Mat incoming_mask;
    cv::Mat existing_mask;
    cv::Mat overlap;
    cv::compare(mask_roi, PANORAMA_WEIGHT_EPS, incoming_mask, cv::CMP_GT);
    cv::compare(weights_roi, PANORAMA_WEIGHT_EPS, existing_mask, cv::CMP_GT);
    cv::bitwise_and(incoming_mask, existing_mask, overlap);
    return overlap;
}

cv::Mat existing_panorama_roi(const cv::Mat &accum_roi, const cv::Mat &weights_roi)
{
    cv::Mat denom;
    cv::max(weights_roi, PANORAMA_WEIGHT_EPS, denom);

    std::vector<cv::Mat> denom_channels;
    denom_channels.push_back(denom);
    denom_channels.push_back(denom);
    denom_channels.push_back(denom);

    cv::Mat denom3;
    cv::Mat existing;
    cv::merge(denom_channels, denom3);
    cv::divide(accum_roi, denom3, existing);
    return existing;
}

void apply_panorama_gain(cv::Mat &src, const cv::Mat &mask_roi,
                         const cv::Mat &accum_roi, const cv::Mat &weights_roi)
{
    const cv::Mat overlap = overlap_mask_from_weights(mask_roi, weights_roi);
    if (cv::countNonZero(overlap) < PANORAMA_GAIN_MIN_OVERLAP) return;

    const cv::Mat existing = existing_panorama_roi(accum_roi, weights_roi);
    const cv::Scalar incoming_mean = cv::mean(src, overlap);
    const cv::Scalar existing_mean = cv::mean(existing, overlap);

    std::vector<cv::Mat> channels;
    cv::split(src, channels);
    for (int c = 0; c < 3; ++c) {
        const double gain = std::max(PANORAMA_GAIN_MIN, std::min(PANORAMA_GAIN_MAX,
            existing_mean[c] / std::max(incoming_mean[c], 1.0)));
        channels[c] *= gain;
    }
    cv::merge(channels, src);
    cv::min(src, 255.0, src);
    cv::max(src, 0.0, src);
}

bool score_panorama_match(const PanoramaCanvas &canvas, const cv::Mat &src_float,
                          const cv::Mat &mask, int x, int y,
                          double &score, int &count)
{
    cv::Rect dst_rect;
    cv::Rect src_rect;
    if (!clipped_panorama_rect(canvas, src_float, x, y, dst_rect, src_rect)) return false;

    const cv::Mat mask_roi = mask(src_rect);
    const cv::Mat weights_roi = canvas.weights(dst_rect);
    const cv::Mat overlap = overlap_mask_from_weights(mask_roi, weights_roi);
    count = cv::countNonZero(overlap);
    if (count < PANORAMA_GAIN_MIN_OVERLAP) return false;

    const cv::Mat src_roi = src_float(src_rect);
    const cv::Mat existing = existing_panorama_roi(canvas.accum(dst_rect), weights_roi);
    cv::Mat diff;
    cv::absdiff(src_roi, existing, diff);
    const cv::Scalar mean_diff = cv::mean(diff, overlap);
    score = (mean_diff[0] + mean_diff[1] + mean_diff[2]) / 3.0;
    return true;
}

int refine_panorama_x_by_matching(const PanoramaCanvas &canvas, const cv::Mat &src_float,
                                  const cv::Mat &mask, int nominal_x, int y)
{
    if (canvas.accum.empty() || canvas.weights.empty()) return nominal_x;

    double nominal_score = 0.0;
    int nominal_count = 0;
    const bool nominal_ok = score_panorama_match(canvas, src_float, mask, nominal_x, y,
                                                 nominal_score, nominal_count);

    // nominal_x comes from the turret angle; search a fifth of the frame width around it
    // (12 to 160 px) in steps of 2, then refine within 2 px of the best
    const int radius = std::min(160, std::max(12, src_float.cols / 5));
    int best_x = nominal_x;
    int best_count = 0;
    double best_score = std::numeric_limits<double>::max();

    for (int dx = -radius; dx <= radius; dx += 2) {
        double score = 0.0;
        int count = 0;
        if (!score_panorama_match(canvas, src_float, mask, nominal_x + dx, y, score, count)) continue;
        if (score < best_score) {
            best_score = score;
            best_count = count;
            best_x = nominal_x + dx;
        }
    }

    for (int x = best_x - 2; x <= best_x + 2; ++x) {
        double score = 0.0;
        int count = 0;
        if (!score_panorama_match(canvas, src_float, mask, x, y, score, count)) continue;
        if (score < best_score) {
            best_score = score;
            best_count = count;
            best_x = x;
        }
    }

    // Scores are mean absolute differences in grey levels, lower is better.
    // Where the nominal position cannot be scored, a match must be good in absolute terms.
    // Otherwise it must beat the nominal one by 2% and one grey level, so noise moves nothing.
    if (best_count < PANORAMA_GAIN_MIN_OVERLAP || best_x == nominal_x) return nominal_x;
    if (!nominal_ok) return best_score <= 45.0 ? best_x : nominal_x;
    if (best_score + 1.0 < nominal_score * 0.98) return best_x;
    return nominal_x;
}

bool initialize_panorama_canvas(PanoramaCanvas &canvas, const cv::Mat &rgb,
                                const std::vector<std::pair<float, float>> &route,
                                const ScanParams &params)
{
    if (route.empty() || rgb.empty() || rgb.cols <= 0 || rgb.rows <= 0) return false;

    float fov_h = params.panorama_fov_h;
    // no FOV given: assume the frames abut, so the step is the FOV; 1 degree keeps the scale finite
    if (fov_h <= 1e-4f) fov_h = std::fabs(params.panorama_step_h);
    if (fov_h <= 1e-4f) fov_h = 1.0f;

    std::vector<double> angles = unwrap_panorama_angles(route, params.panorama_direction);
    if (angles.empty()) return false;

    double left_deg = angles.front() - static_cast<double>(fov_h) * 0.5;
    double right_deg = angles.front() + static_cast<double>(fov_h) * 0.5;
    for (double angle : angles) {
        left_deg = std::min(left_deg, angle - static_cast<double>(fov_h) * 0.5);
        right_deg = std::max(right_deg, angle + static_cast<double>(fov_h) * 0.5);
    }

    const double extent_deg = std::max(static_cast<double>(fov_h), right_deg - left_deg);
    // native resolution, reduced only when the whole sweep would exceed the output width limit
    double px_per_deg = static_cast<double>(rgb.cols) / static_cast<double>(fov_h);
    if (PANORAMA_MAX_OUTPUT_WIDTH > 0 && extent_deg * px_per_deg > PANORAMA_MAX_OUTPUT_WIDTH) {
        px_per_deg = static_cast<double>(PANORAMA_MAX_OUTPUT_WIDTH) / extent_deg;
    }

    const int target_width = std::max(1, static_cast<int>(std::round(fov_h * px_per_deg)));
    const double scale = static_cast<double>(target_width) / static_cast<double>(rgb.cols);
    const int target_height = std::max(1, static_cast<int>(std::round(rgb.rows * scale)));
    const int canvas_width = std::max(1, static_cast<int>(std::ceil(extent_deg * px_per_deg)));

    canvas.reset();
    canvas.angles = angles;
    canvas.left_deg = left_deg;
    canvas.extent_deg = extent_deg;
    canvas.px_per_deg = px_per_deg;
    canvas.fov_h = fov_h;
    canvas.target_width = target_width;
    canvas.canvas_width = canvas_width;
    canvas.canvas_height = target_height;
    canvas.route_size = static_cast<int>(route.size());
    canvas.accum = cv::Mat::zeros(canvas.canvas_height, canvas.canvas_width, CV_32FC3);
    canvas.weights = cv::Mat::zeros(canvas.canvas_height, canvas.canvas_width, CV_32FC1);
    return true;
}

cv::Mat render_panorama_canvas(const PanoramaCanvas &canvas, bool crop_to_content)
{
    if (canvas.accum.empty() || canvas.weights.empty()) return cv::Mat();

    cv::Mat denom;
    // weighted average: accumulated colour over accumulated weight; uncovered pixels stay black
    cv::max(canvas.weights, PANORAMA_WEIGHT_EPS, denom);
    std::vector<cv::Mat> denom_channels;
    denom_channels.push_back(denom);
    denom_channels.push_back(denom);
    denom_channels.push_back(denom);

    cv::Mat denom3;
    cv::Mat output32;
    cv::merge(denom_channels, denom3);
    cv::divide(canvas.accum, denom3, output32);

    cv::Mat content_mask;
    cv::compare(canvas.weights, PANORAMA_WEIGHT_EPS, content_mask, cv::CMP_GT);
    cv::Mat empty_mask;
    cv::bitwise_not(content_mask, empty_mask);
    output32.setTo(cv::Scalar(0, 0, 0), empty_mask);

    cv::Mat output8;
    output32.convertTo(output8, CV_8UC3);

    if (!crop_to_content) return output8;

    std::vector<cv::Point> content_points;
    cv::findNonZero(content_mask, content_points);
    if (content_points.empty()) return output8;

    const cv::Rect crop = cv::boundingRect(content_points) & cv::Rect(0, 0, output8.cols, output8.rows);
    return crop.width > 0 && crop.height > 0 ? output8(crop).clone() : output8;
}

} // namespace

void PipelineProcessor::append_panorama_preview(DisplaySlot &main_display,
    int ptz_idx, const std::vector<std::pair<float, float>> &route,
    const ScanParams &scan_params,
    PanoramaCanvas &panorama_canvas)
{
    if (ptz_idx < 0 || ptz_idx >= static_cast<int>(route.size()) || main_display.modified_result.empty()) return;

    const cv::Mat rgb = panorama_source_rgb8(main_display.modified_result);
    if (rgb.empty() || rgb.rows <= 0 || rgb.cols <= 0) return;

    if (ptz_idx == 0) panorama_canvas.reset();
    if (panorama_canvas.accum.empty()
            || panorama_canvas.route_size != static_cast<int>(route.size())
            || std::fabs(panorama_canvas.fov_h - scan_params.panorama_fov_h) > 1e-4f) {
        if (!initialize_panorama_canvas(panorama_canvas, rgb, route, scan_params)) return;
    }

    cv::Mat resized;
    cv::resize(rgb, resized, cv::Size(panorama_canvas.target_width, panorama_canvas.canvas_height),
               0, 0, cv::INTER_AREA);

    cv::Mat src_float;
    resized.convertTo(src_float, CV_32FC3);
    cv::Mat mask = make_panorama_feather_mask(src_float.cols, src_float.rows);

    int x = static_cast<int>(std::round(
        (panorama_canvas.angles[static_cast<size_t>(ptz_idx)]
         - static_cast<double>(panorama_canvas.fov_h) * 0.5
         - panorama_canvas.left_deg) * panorama_canvas.px_per_deg));
    const int y = (panorama_canvas.canvas_height - src_float.rows) / 2;

    if (scan_params.panorama_fusion_mode == ScanParams::PANORAMA_FUSION_MATCHING) {
        x = refine_panorama_x_by_matching(panorama_canvas, src_float, mask, x, y);
    }

    cv::Rect dst_rect;
    cv::Rect src_rect;
    if (!clipped_panorama_rect(panorama_canvas, src_float, x, y, dst_rect, src_rect)) return;

    cv::Mat src_roi = src_float(src_rect).clone();
    const cv::Mat mask_roi = mask(src_rect);
    cv::Mat accum_roi = panorama_canvas.accum(dst_rect);
    cv::Mat weights_roi = panorama_canvas.weights(dst_rect);

    if (scan_params.panorama_fusion_mode == ScanParams::PANORAMA_FUSION_GAIN
            || scan_params.panorama_fusion_mode == ScanParams::PANORAMA_FUSION_MATCHING) {
        apply_panorama_gain(src_roi, mask_roi, accum_roi, weights_roi);
    }

    std::vector<cv::Mat> mask_channels;
    mask_channels.push_back(mask_roi);
    mask_channels.push_back(mask_roi);
    mask_channels.push_back(mask_roi);
    cv::Mat mask3;
    cv::merge(mask_channels, mask3);

    cv::Mat weighted_src;
    cv::multiply(src_roi, mask3, weighted_src);
    accum_roi += weighted_src;
    weights_roi += mask_roi;

    const cv::Mat preview = render_panorama_canvas(panorama_canvas, true);
    if (preview.empty()) return;

    QImage preview_img(preview.data, preview.cols, preview.rows,
                       static_cast<int>(preview.step), QImage::Format_RGB888);
    emit panorama_preview_updated(QPixmap::fromImage(preview_img.copy()));
}

void PipelineProcessor::save_panorama_preview(const PanoramaCanvas &panorama_canvas,
    const QString &scan_root, const PipelineConfig &pcfg)
{
    if (panorama_canvas.accum.empty() || !m_g.tp) return;

    const cv::Mat panorama = render_panorama_canvas(panorama_canvas, false);
    if (panorama.empty()) return;

    QImage preview_img(panorama.data, panorama.cols, panorama.rows,
                       static_cast<int>(panorama.step), QImage::Format_RGB888);
    emit panorama_preview_updated(QPixmap::fromImage(preview_img.copy()));

    QDir().mkpath(scan_root);

    (void)pcfg;
    if (!m_g.tp->append_task(std::bind(ImageIO::save_image_jpg, panorama,
                                       scan_root + "/panorama.jpg"))) {
        emit task_queue_full();
    }
}

// Main-display-only scan step. Reaches across slots via the router because
// scan walks main+alt1 in lockstep.
void PipelineProcessor::advance_scan_main(DisplayRouter &router, StreamSlot &main_stream, DisplaySlot &main_display,
    bool updated, const ProcessingParams &params, const PipelineConfig &pcfg,
    const ScanJob &job, ScanProgress &progress,
    int &scan_img_count,
    QString &scan_save_path_a, QString &scan_save_path,
    PanoramaCanvas &panorama_canvas,
    qint64 &panorama_position_started_ms)
{
    (void)main_display;
    if (job.generation != progress.generation) {
        // A scan this thread has not stepped yet. A new one begins before its first
        // position; one that was under way when the thread started goes on from where
        // ScanController last saw it.
        progress = ScanProgress();
        progress.generation = job.generation;
        progress.ptz_idx = job.ptz_idx;
        progress.tcu_idx = job.tcu_idx;
        // Nothing of the scan before is finished or kept. Without this, the frames left
        // for its last position would still be counted and saved into its folder.
        scan_img_count = -1;
        scan_save_path_a.clear();
        scan_save_path.clear();
        panorama_canvas.reset();
        panorama_position_started_ms = -1;
    }
    // The end is announced once. The flag falls when the GUI has stopped the scan, so
    // continuing a finished scan ends it again, as before.
    if (!job.running) { progress.end_reported = false; return; }
    if (progress.end_reported || !updated) return;
    const ScanParams &scan_params = job.params;

    if (scan_params.ptz_scan_mode == ScanParams::PTZ_SCAN_PANORAMA) {
        advance_panorama_scan_main(router, main_stream, main_display, params, pcfg, job, progress, scan_img_count, scan_save_path,
                                   panorama_canvas,
                                   panorama_position_started_ms);
        return;
    }

    scan_img_count -= 1;
    if (scan_img_count < 0) scan_img_count = 0;

    int save_img_num = scan_params.num_single_pos;
    if (save_img_num < 1) save_img_num = 1;
    if (scan_img_count > 0 && scan_img_count <= save_img_num && !scan_save_path.isEmpty()) {
        save_scan_img(router, main_stream, scan_save_path, QString::number(save_img_num - scan_img_count + 1) + ".bmp", params, pcfg, scan_params);
    }
    if (scan_img_count == save_img_num && !scan_save_path.isEmpty()) {
        emit save_screenshot_signal(scan_save_path + "/screenshot_" + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".png");
    }

    if (!scan_img_count) {
        float fps = 0.0f;
        { QMutexLocker mlk(&main_stream.meta_mutex); fps = main_stream.meta.fps; }
        if (fps <= 0.0f) fps = 1.0f; // safety floor
        const int wait_time = scan_params.scan_ptz_enabled
            ? scan_params.ptz_wait_time : 0;
        scan_img_count = fps * (wait_time + save_img_num / fps);

        int tcu_idx = progress.tcu_idx;
        int ptz_idx = progress.ptz_idx;
        const auto &tcu_route = job.tcu_route;
        const auto &gate_width_route = job.gate_width_route;
        const auto &ptz_route = job.ptz_route;
        if (tcu_route.empty() || gate_width_route.size() != tcu_route.size() || ptz_route.empty()) {
            scan_img_count = -1;
            progress.end_reported = true;
            emit finish_scan_signal(job.generation);
            return;
        }

        // what this step has to command; sent once the profile is known
        ScanStep step;
        step.generation = job.generation;

        if (tcu_idx == (int)tcu_route.size() - 1 || tcu_idx == -1) {
            progress.tcu_idx = -1;
            tcu_idx = -1;
            ptz_idx++;
            progress.ptz_idx = ptz_idx;

            if (ptz_idx < (int)ptz_route.size()) {
                if (scan_params.scan_ptz_enabled) {
                    step.move_ptz = true;
                    step.angle_h = ptz_route[ptz_idx].first;
                    step.angle_v = ptz_route[ptz_idx].second;
                }

                scan_save_path_a = params.save_location + "/" + job.name + "/" + QString::number(ptz_idx);
                QDir().mkdir(scan_save_path_a);
                QFile params_file(scan_save_path_a + "/params");
                params_file.open(QIODevice::WriteOnly);
                params_file.write(QString::asprintf("angle: \n"
                                                   "    H: %06.2f\n"
                                                   "    V:  %05.2f\n",
                                                   ptz_route[ptz_idx].first,
                                                   ptz_route[ptz_idx].second).toUtf8());
            }
            else {
                scan_img_count = -1;
                progress.end_reported = true;
                emit finish_scan_signal(job.generation);
                return;
            }
        }

        tcu_idx = progress.tcu_idx + 1;
        progress.tcu_idx = tcu_idx;
        if (scan_params.scan_delay_enabled) {
            step.set_delay = true;
            step.delay_ns = tcu_route[tcu_idx];
        }
        if (scan_params.scan_gate_width_enabled) {
            step.set_gate_width = true;
            step.gate_width_ns = gate_width_route[tcu_idx];
        }
        step.ptz_idx = progress.ptz_idx;
        step.tcu_idx = progress.tcu_idx;
        emit scan_step_requested(step);

        QString profile_name;
        if (scan_params.scan_delay_enabled && scan_params.scan_gate_width_enabled) {
            profile_name = QString("%1 ns - gw %2 ns")
                .arg(tcu_route[tcu_idx], 0, 'f', 2)
                .arg(gate_width_route[tcu_idx], 0, 'f', 2);
        }
        else if (scan_params.scan_delay_enabled) {
            profile_name = QString::number(tcu_route[tcu_idx], 'f', 2) + " ns";
        }
        else if (scan_params.scan_gate_width_enabled) {
            profile_name = "gw " + QString::number(gate_width_route[tcu_idx], 'f', 2) + " ns";
        }
        else {
            profile_name = "current";
        }
        scan_save_path = scan_save_path_a + "/" + profile_name;
        QDir().mkdir(scan_save_path);
        QDir().mkdir(scan_save_path + "/ori_bmp");
        QDir().mkdir(scan_save_path + "/res_bmp");

#if 0
        // Alt1 contributes a side capture if it's currently routed.
        StreamId alt1_stream = router.stream_routed_to(DisplayId::ALT1);
        if (alt1_stream != StreamId::NONE) QDir().mkdir(scan_save_path + "/alt_bmp");
#endif
    }
}

void PipelineProcessor::advance_panorama_scan_main(DisplayRouter &router, StreamSlot &main_stream, DisplaySlot &main_display,
    const ProcessingParams &params, const PipelineConfig &pcfg,
    const ScanJob &job, ScanProgress &progress,
    int &scan_img_count,
    QString &scan_save_path,
    PanoramaCanvas &panorama_canvas,
    qint64 &panorama_position_started_ms)
{
    const ScanParams &scan_params = job.params;
    int save_img_num = scan_params.num_single_pos;
    if (save_img_num < 1) save_img_num = 1;

    if (progress.ptz_idx < 0 && scan_img_count >= 0) {
        scan_img_count = -1;
        scan_save_path.clear();
        panorama_canvas.reset();
        panorama_position_started_ms = -1;
    }

    if (scan_img_count > 0) {
        const int image_number = save_img_num - scan_img_count + 1;
        if (scan_img_count == save_img_num && !scan_save_path.isEmpty()) {
            const auto &ptz_route = job.ptz_route;
            append_panorama_preview(main_display, progress.ptz_idx,
                                    ptz_route, scan_params, panorama_canvas);
        }
        if (!scan_save_path.isEmpty()) {
            save_scan_img(router, main_stream, scan_save_path, QString::number(image_number) + ".bmp", params, pcfg, scan_params);
        }

        scan_img_count -= 1;
        if (scan_img_count <= 0) {
            scan_img_count = -1;
            panorama_position_started_ms = -1;
        }
        return;
    }

    if (scan_img_count == 0) {
        const int wait_time = scan_params.panorama_wait_time < 0 ? 0 : scan_params.panorama_wait_time;
        const qint64 now_ms = QDateTime::currentMSecsSinceEpoch();
        if (panorama_position_started_ms < 0) panorama_position_started_ms = now_ms;
        if (now_ms - panorama_position_started_ms < static_cast<qint64>(wait_time) * 1000) return;

        scan_img_count = save_img_num;
        return;
    }

    const auto &ptz_route = job.ptz_route;
    int ptz_idx = progress.ptz_idx + 1;
    progress.ptz_idx = ptz_idx;

    if (ptz_route.empty() || ptz_idx >= (int)ptz_route.size()) {
        save_panorama_preview(panorama_canvas, params.save_location + "/" + job.name, pcfg);
        panorama_canvas.reset();
        scan_img_count = -1;
        scan_save_path.clear();
        panorama_position_started_ms = -1;
        progress.end_reported = true;
        emit finish_scan_signal(job.generation);
        return;
    }

    const float angle_h = ptz_route[ptz_idx].first;
    const float angle_v = ptz_route[ptz_idx].second;
    ScanStep step;
    step.generation = job.generation;
    step.ptz_idx = ptz_idx;
    step.tcu_idx = progress.tcu_idx;
    step.move_ptz = true;
    step.angle_h = angle_h;
    step.angle_v = angle_v;
    emit scan_step_requested(step);

    const QString pos_name = QString("%1_h%2_v%3")
        .arg(ptz_idx, 3, 10, QChar('0'))
        .arg(angle_h, 0, 'f', 2)
        .arg(angle_v, 0, 'f', 2);
    scan_save_path = params.save_location + "/" + job.name + "/" + pos_name;
    QDir().mkdir(scan_save_path);

    QFile params_file(scan_save_path + "/params");
    params_file.open(QIODevice::WriteOnly);
    const QString ending = scan_params.panorama_use_end_h
        ? QString::number(scan_params.panorama_end_h, 'f', 2) + " deg"
        : QString("disabled");
    params_file.write(QString("mode: panorama\n"
                              "angle:\n"
                              "    H: %1\n"
                              "    V: %2\n"
                              "ending H: %3\n"
                              "zoom: %4 x\n"
                              "FOV H: %5 deg\n"
                              "FOV V: %6 deg\n"
                              "fusion: %7\n")
                          .arg(angle_h, 0, 'f', 2)
                          .arg(angle_v, 0, 'f', 2)
                          .arg(ending)
                          .arg(scan_params.panorama_zoom_level, 0, 'f', 2)
                          .arg(scan_params.panorama_fov_h, 0, 'f', 2)
                          .arg(scan_params.panorama_fov_v, 0, 'f', 2)
                          .arg(get_panorama_fusion_name(scan_params.panorama_fusion_mode))
                          .toUtf8());
    params_file.close();

    QDir().mkdir(scan_save_path + "/ori_bmp");
    QDir().mkdir(scan_save_path + "/res_bmp");

#if 0
    StreamId alt1_stream = router.stream_routed_to(DisplayId::ALT1);
    if (alt1_stream != StreamId::NONE) QDir().mkdir(scan_save_path + "/alt_bmp");
#endif

    panorama_position_started_ms = QDateTime::currentMSecsSinceEpoch();
    scan_img_count = 0;
}

void PipelineProcessor::record_frame(StreamSlot &stream, DisplaySlot &display, bool updated,
    const ProcessingParams &params, const PipelineConfig &pcfg,
    double is_net,
    const QString &info_tcu, const QString &info_time,
    int ww, int hh, float weight)
{
    (void)info_tcu;
    bool is_color = false;
    { QMutexLocker mlk(&stream.meta_mutex); is_color = stream.meta.is_color; }

    if (updated && display.save_raw) {
        save_to_file(stream, display, false, params, pcfg);
        if (source_repush_on_pause(stream.meta.source_type) || !pcfg.consecutive_capture || display.id != DisplayId::MAIN) display.save_raw = false;
    }
    if (updated && display.save_proc) {
        save_to_file(stream, display, true, params, pcfg);
        if (source_repush_on_pause(stream.meta.source_type) || !pcfg.consecutive_capture || display.id != DisplayId::MAIN) display.save_proc = false;
    }
    if (updated && display.record_raw) {
        cv::Mat temp = stream.latest_raw.clone();
        if (is_color) cv::cvtColor(temp, temp, cv::COLOR_RGB2BGR);
        if (display.raw_writer.isOpened()) display.raw_writer.write(temp);
    }
    if (updated && display.record_proc) {
        cv::Mat temp = display.modified_result.clone();
        if (!pcfg.image_3d && !params.show_info) {
            if (pcfg.save_info) {
                cv::putText(display.modified_result, info_time.toLatin1().data(), cv::Point(ww - 240 * weight, 50 * weight), cv::FONT_HERSHEY_SIMPLEX, weight, cv::Scalar(255), weight * 2);
#ifdef LVTONG
                int baseline = 0;
                if (pcfg.fishnet_recog) {
                    cv::putText(display.modified_result, is_net > pcfg.fishnet_thresh ? "FISHNET FOUND" : "FISHNET NOT FOUND", cv::Point(ww - 40 - cv::getTextSize(is_net > pcfg.fishnet_thresh ? "FISHNET FOUND" : "FISHNET NOT FOUND", cv::FONT_HERSHEY_SIMPLEX, weight, weight * 2, &baseline).width, hh - 100 + 50 * weight), cv::FONT_HERSHEY_SIMPLEX, weight, cv::Scalar(255), weight * 2);
                }
#endif
            }
        }
        if (is_color || pcfg.pseudocolor) cv::cvtColor(temp, temp, cv::COLOR_RGB2BGR);
        if (display.proc_writer.isOpened()) display.proc_writer.write(temp);
    }
}

void PipelineProcessor::save_to_file(StreamSlot &stream, DisplaySlot &display,
    bool save_result, const ProcessingParams &params, const PipelineConfig &pcfg)
{
    cv::Mat *temp = save_result ? &display.modified_result : &stream.latest_raw;
    int pixel_depth = 8;
    { QMutexLocker mlk(&stream.meta_mutex); pixel_depth = stream.meta.pixel_depth; }

    cv::Mat result_image;
    if (pcfg.save_in_grayscale) cv::cvtColor(*temp, result_image, cv::COLOR_RGB2GRAY);
    else                         result_image = temp->clone();

    if (save_result) {
        QString dt = QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz");
        if (params.split) {
            if (pcfg.integrate_info) {
                const QString &tcu_note = params.tcu.note;
                if (!m_g.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString, QString)>(&ImageIO::save_image_bmp), result_image(cv::Rect(             0,              0, temp->cols / 2, temp->rows / 2)).clone(), params.save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + dt + "_0" + ".bmp", tcu_note))) emit task_queue_full();
                if (!m_g.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString, QString)>(&ImageIO::save_image_bmp), result_image(cv::Rect(temp->cols / 2,              0, temp->cols / 2, temp->rows / 2)).clone(), params.save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + dt + "_1" + ".bmp", tcu_note))) emit task_queue_full();
                if (!m_g.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString, QString)>(&ImageIO::save_image_bmp), result_image(cv::Rect(             0, temp->rows / 2, temp->cols / 2, temp->rows / 2)).clone(), params.save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + dt + "_2" + ".bmp", tcu_note))) emit task_queue_full();
                if (!m_g.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString, QString)>(&ImageIO::save_image_bmp), result_image(cv::Rect(temp->cols / 2, temp->rows / 2, temp->cols / 2, temp->rows / 2)).clone(), params.save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + dt + "_3" + ".bmp", tcu_note))) emit task_queue_full();
            }
            else {
                if (!m_g.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString)>(&ImageIO::save_image_bmp), result_image(cv::Rect(             0,              0, temp->cols / 2, temp->rows / 2)).clone(), params.save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + dt + "_0" + ".bmp"))) emit task_queue_full();
                if (!m_g.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString)>(&ImageIO::save_image_bmp), result_image(cv::Rect(temp->cols / 2,              0, temp->cols / 2, temp->rows / 2)).clone(), params.save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + dt + "_1" + ".bmp"))) emit task_queue_full();
                if (!m_g.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString)>(&ImageIO::save_image_bmp), result_image(cv::Rect(             0, temp->rows / 2, temp->cols / 2, temp->rows / 2)).clone(), params.save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + dt + "_2" + ".bmp"))) emit task_queue_full();
                if (!m_g.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString)>(&ImageIO::save_image_bmp), result_image(cv::Rect(temp->cols / 2, temp->rows / 2, temp->cols / 2, temp->rows / 2)).clone(), params.save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + dt + "_3" + ".bmp"))) emit task_queue_full();
            }
        }
        switch (pcfg.img_format) {
        case 0:
            if (pcfg.integrate_info) {
                const QString &tcu_note = params.tcu.note;
                if (!m_g.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString, QString)>(&ImageIO::save_image_bmp), result_image, params.save_location + "/res_bmp/" + dt + ".bmp", tcu_note))) emit task_queue_full();
            }
            else {
                if (!m_g.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString)>(&ImageIO::save_image_bmp), result_image, params.save_location + "/res_bmp/" + dt + ".bmp"))) emit task_queue_full();
            }
            break;
        case 1: if (!m_g.tp->append_task(std::bind(ImageIO::save_image_jpg, result_image, params.save_location + "/res_bmp/" + dt + ".jpg"))) emit task_queue_full(); break;
        default: break;
        }
    }
    else {
        switch (pixel_depth) {
        case 8:
            switch (pcfg.img_format) {
            case 0:
                if (pcfg.integrate_info) {
                    const QString &tcu_note = params.tcu.note;
                    if (!m_g.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString, QString)>(&ImageIO::save_image_bmp), result_image, params.save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".bmp", tcu_note))) emit task_queue_full();
                }
                else {
                    if (!m_g.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString)>(&ImageIO::save_image_bmp), result_image, params.save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".bmp"))) emit task_queue_full();
                }
                break;
            case 1: if (!m_g.tp->append_task(std::bind(ImageIO::save_image_jpg, result_image, params.save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".jpg"))) emit task_queue_full(); break;
            default: break;
            }
            break;
        case 10:
        case 12:
        case 16:
            switch (pcfg.img_format) {
            case 0: if (!m_g.tp->append_task(std::bind(ImageIO::save_image_tif, result_image * (1 << (16 - pixel_depth)), params.save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".tif"))) emit task_queue_full(); break;
            case 1: if (!m_g.tp->append_task(std::bind(ImageIO::save_image_jpg, result_image * (1 << (16 - pixel_depth)), params.save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".jpg"))) emit task_queue_full(); break;
            default: break;
            }
            break;
        default: break;
        }
    }
}

void PipelineProcessor::save_scan_img(DisplayRouter &router, StreamSlot &main_stream,
    QString path, QString name,
    const ProcessingParams &params, const PipelineConfig &pcfg,
    const ScanParams &scan_params)
{
    DisplaySlot &main_display = router.get_display(DisplayId::MAIN);

    if (scan_params.capture_scan_ori) {
        if (pcfg.integrate_info) {
            const QString &tcu_note = params.tcu.note;
            m_g.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString, QString)>(&ImageIO::save_image_bmp), main_stream.latest_raw.clone(), path + "/ori_bmp/" + name, tcu_note));
        }
        else {
            m_g.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString)>(&ImageIO::save_image_bmp), main_stream.latest_raw.clone(), path + "/ori_bmp/" + name));
        }
    }
    if (scan_params.capture_scan_res) {
        if (pcfg.integrate_info) {
            const QString &tcu_note = params.tcu.note;
            m_g.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString, QString)>(&ImageIO::save_image_bmp), main_display.modified_result.clone(), path + "/res_bmp/" + name, tcu_note));
        }
        else {
            m_g.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString)>(&ImageIO::save_image_bmp), main_display.modified_result.clone(), path + "/res_bmp/" + name));
        }
    }

#if 0
    StreamId alt1_stream = router.stream_routed_to(DisplayId::ALT1);
    if (alt1_stream != StreamId::NONE) {
        StreamSlot &alt1 = router.get_stream(alt1_stream);
        if (pcfg.integrate_info) {
            const QString &tcu_note = params.tcu.note;
            m_g.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString, QString)>(&ImageIO::save_image_bmp), alt1.latest_raw.clone(), path + "/alt_bmp/" + name, tcu_note));
        }
        else {
            m_g.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString)>(&ImageIO::save_image_bmp), alt1.latest_raw.clone(), path + "/alt_bmp/" + name));
        }
    }
#endif
}

void PipelineProcessor::draw_yolo_boxes(DisplaySlot &display,
    cv::Mat &image, const std::vector<YoloResult> &results)
{
    if (image.empty() || results.empty()) return;

    cv::Mat draw_image;
    if (image.channels() == 1) cv::cvtColor(image, draw_image, cv::COLOR_GRAY2BGR);
    else                       draw_image = image;

    bool show_range = false;
    std::vector<double> class_heights;
    TargetEstimator::Optics optics;
    {
        QMutexLocker lk(&display.label_mutex);
        show_range = display.range_label_enabled;
        if (show_range) {
            class_heights = display.class_heights;
            optics = display.label_optics;
        }
    }

    // Range divides the frame diagonal by the box height, so both must be in the same
    // frame. The boxes are in detector input coordinates, which may differ from the
    // image drawn on, hence the detector's frame size and not image.size().
    const cv::Size detect_size = display.tracker_frame_size.area() > 0
            ? display.tracker_frame_size
            : image.size();

    YoloDetector *det_for_labels = display.yolo;
    for (const auto &det : results) {
        cv::Scalar color(0, 255, 0);
        cv::rectangle(draw_image, det.box, color, 2);

        std::string label;
        if (det_for_labels && det.class_id >= 0
                && det.class_id < static_cast<int>(det_for_labels->get_class_names().size())) {
            label = det_for_labels->get_class_names()[det.class_id];
        }
        else {
            label = "class_" + std::to_string(det.class_id);
        }
        label += ": " + std::to_string(static_cast<int>(det.confidence * 100)) + "%";

        if (show_range && det.class_id >= 0
                && det.class_id < static_cast<int>(class_heights.size())) {
            const QRect box(det.box.x, det.box.y, det.box.width, det.box.height);
            const double range = TargetEstimator::range_meters(
                class_heights[det.class_id], box,
                QSize(detect_size.width, detect_size.height), optics);
            // Zero means the class has no configured height, not a target at zero range,
            // so nothing is appended for it.
            if (range > 0.0) {
                char range_text[32];
                if (range >= 1000.0)
                    std::snprintf(range_text, sizeof(range_text), "  %.1fkm", range / 1000.0);
                else
                    std::snprintf(range_text, sizeof(range_text), "  %dm",
                                  static_cast<int>(std::lround(range)));
                label += range_text;
            }
        }

        int baseline = 0;
        cv::Size text_size = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseline);

        cv::rectangle(draw_image,
                     cv::Point(det.box.x, det.box.y - text_size.height - 5),
                     cv::Point(det.box.x + text_size.width, det.box.y),
                     color, -1);
        cv::putText(draw_image, label,
                   cv::Point(det.box.x, det.box.y - 5),
                   cv::FONT_HERSHEY_SIMPLEX, 0.5,
                   cv::Scalar(0, 0, 0), 1);
    }

    if (image.channels() == 1) cv::cvtColor(draw_image, image, cv::COLOR_BGR2GRAY);
}
