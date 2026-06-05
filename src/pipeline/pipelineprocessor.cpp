#include "pipeline/pipelineprocessor.h"
#include "controller/tcucontroller.h"
#include "controller/scancontroller.h"
#include "controller/devicemanager.h"
#include "controller/auxpanelmanager.h"
#include "pipeline/scanparams.h"
#include "widgets/display.h"
#include "widgets/floatingwindow.h"
#include "image/imageio.h"
#include "image/imageproc.h"
#include "util/threadpool.h"
#include "util/constants.h"
#include "util/timedqueue.h"
#include "port/tcu.h"

#include <QThread>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QCoreApplication>
#include <cmath>

PipelineProcessor::PipelineProcessor(PipelineGlobals globals, QObject* parent)
    : QObject(parent), m_g(globals)
{
}

int PipelineProcessor::run(DisplayRouter& router, DisplaySlot& display) {
    display.consumer_state = true;
    bool updated = false;
    int packets_lost = 0;
    int _w = 0, _h = 0, _pixel_depth = 8;
    float weight = 0.0f;
    int scan_img_count = -1;
    QString scan_save_path_a, scan_save_path;

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
            display.last_route_gen = snap.gen;
            // Route change clears the dead-source observation. Subsequent
            // iterations re-set it from the new stream's queue state.
            if (display.source_dead) {
                display.source_dead = false;
                emit source_state_changed(static_cast<int>(display.id), false);
            }

            if (snap.stream != StreamId::None) {
                StreamSlot& s = router.stream(snap.stream);
                QMutexLocker mlk(&s.meta_mutex);
                _w = s.meta.width;
                _h = s.meta.height;
                _pixel_depth = s.meta.pixel_depth;
                weight = _h / FONT_SCALE_DIVISOR;
                if (_w > 0 && _h > 0) {
                    display.avg_state.prev_3d = cv::Mat(_h, _w, CV_8UC3);
                    display.avg_state.prev_img = cv::Mat(_h, _w, CV_MAKETYPE(_pixel_depth == 8 ? CV_8U : CV_16U, s.meta.is_color ? 3 : 1));
                    display.user_mask = cv::Mat::zeros(_h, _w, CV_8UC1);
                }
            }
        }

        if (snap.stream == StreamId::None) {
            QThread::msleep(QUEUE_EMPTY_SLEEP_MS);
            continue;
        }

        StreamSlot& stream = router.stream(snap.stream);

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
        { QMutexLocker lk(m_g.params_mutex); params = *m_g.processing_params; pcfg = display.pcfg; }

        if (!acquire_frame(stream, display, display.avg_state.prev_img, updated, packets_lost)) continue;

        if (display.source_dead) {
            display.source_dead = false;
            emit source_state_changed(static_cast<int>(display.id), false);
        }

        {
            QMutexLocker processing_locker(&display.paint_mutex);

        if (updated) {
            preprocess_frame(stream, display, params, pcfg, _w, _h);
        }

        std::vector<YoloResult> yolo_results = detect_yolo(stream, display, updated, pcfg);

        double is_net = 0.0;
#ifdef LVTONG
        is_net = detect_fishnet(stream, display, pcfg, net);
#endif

        int ww = 0, hh = 0;
        frame_average_and_3d(stream, display, updated, params, pcfg, _w, _h, _pixel_depth, ww, hh);

        enhance_frame(stream, display, params, pcfg);

        QString info_tcu = pcfg.custom_topleft_info ?
                    params.custom_info_text :
                    m_g.tcu_ctrl->get_base_unit() == 2 ? QString::asprintf("DIST %05d m  DOV %04d m", (int)m_g.tcu_ctrl->get_delay_dist(), (int)m_g.tcu_ctrl->get_depth_of_view()) :
                                     QString::asprintf("DELAY %06d ns  GATE %04d ns", (int)std::round(m_g.tcu_ctrl->get_delay_dist() / m_g.tcu_ctrl->get_dist_ns()), (int)std::round(m_g.tcu_ctrl->get_depth_of_view() / m_g.tcu_ctrl->get_dist_ns()));
        QString info_time = QDateTime::currentDateTime().toString("hh:mm:ss:zzz");

        render_and_display(stream, display, params, pcfg, yolo_results, is_net, ww, hh, _w, _h, weight, info_tcu, info_time);

        if (display.id == DisplayId::Main) {
            advance_scan_main(router, stream, display, updated, pcfg, scan_img_count, scan_save_path_a, scan_save_path);
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
    display.consumer_state = false;
    return 0;
}

bool PipelineProcessor::acquire_frame(StreamSlot& stream, DisplaySlot& display,
    cv::Mat& prev_img, bool& updated, int& packets_lost)
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

    {
        QMutexLocker mlk(&stream.meta_mutex);
        stream.latest_raw = frame;
    }

    int packets_lost_frame = 0;
    if (source_has_frame_info(stream.meta.source_type)) {
        int info = 0;
        if (stream.frame_info.try_pop(info)) {
            packets_lost_frame = info;
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

void PipelineProcessor::preprocess_frame(StreamSlot& stream, DisplaySlot& display,
    const ProcessingParams& params, const PipelineConfig& pcfg,
    int& _w, int& _h)
{
    cv::Mat& latest = stream.latest_raw;
    int meta_w = 0, meta_h = 0;
    bool meta_color = false;
    int meta_depth = 8;
    { QMutexLocker mlk(&stream.meta_mutex);
      meta_w = stream.meta.width; meta_h = stream.meta.height;
      meta_color = stream.meta.is_color; meta_depth = stream.meta.pixel_depth; }
    (void)meta_color; (void)meta_depth;

    switch (params.image_rotate) {
    case 0:
        if (_w != meta_w) display.avg_state.release_all();
        _w = meta_w; _h = meta_h;
        break;
    case 2:
        cv::flip(latest, latest, -1);
        if (_w != meta_w) display.avg_state.release_all();
        _w = meta_w; _h = meta_h;
        break;
    case 1:
        cv::flip(latest, latest, -1);
    case 3:
        cv::flip(latest, latest, 0);
        cv::transpose(latest, latest);
        if (_w != meta_h) display.avg_state.release_all();
        _w = meta_h; _h = meta_w;
        break;
    default:
        break;
    }

    // calc histogram (grayscale)
    memset(display.hist, 0, 256 * sizeof(uint));
    if (!meta_color) for (int i = 0; i < _h; i++) for (int j = 0; j < _w; j++) display.hist[(latest.data + i * latest.cols)[j]]++;

    // if the image needs flipping
    { int sym = pcfg.flip; if (sym) cv::flip(latest, latest, sym - 2); }

    // mcp self-adaptive
    if (m_g.tcu_ctrl->get_auto_mcp() && !params.mcp_slider_focused) {
        int thresh_num = latest.total() / AUTO_MCP_DIVISOR, thresh = (1 << meta_depth) - 1;
        while (thresh && thresh_num > 0) thresh_num -= display.hist[thresh--];
        if (thresh > (1 << meta_depth) * 0.94) emit update_mcp_in_thread(m_g.device_mgr->tcu()->get(TCU::MCP) - sqrt(thresh - (1 << meta_depth) * 0.94));
        if (thresh < (1 << meta_depth) * 0.39) emit update_mcp_in_thread(m_g.device_mgr->tcu()->get(TCU::MCP) + sqrt((1 << meta_depth) * 0.39 - thresh));
    }
}

std::vector<YoloResult> PipelineProcessor::detect_yolo(StreamSlot& stream, DisplaySlot& display,
    bool updated, const PipelineConfig& pcfg)
{
    // Model is per-display since T3 — pcfg.model_idx is this display's selection.
    int current_model_selection = pcfg.model_idx;
    YoloDetector* yolo = display.yolo;

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
            } else {
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
    if (yolo && yolo->isInitialized() && updated) {
        cv::Mat det_input;
        if (stream.latest_raw.channels() == 1) {
            cv::cvtColor(stream.latest_raw, det_input, cv::COLOR_GRAY2BGR);
        } else {
            det_input = stream.latest_raw;
        }
        yolo->run(det_input, results);
    }
    return results;
}

void PipelineProcessor::frame_average_and_3d(StreamSlot& stream, DisplaySlot& display,
    bool updated, const ProcessingParams& params, const PipelineConfig& pcfg,
    int _w, int _h, int _pixel_depth, int& ww, int& hh)
{
    cv::Mat& latest = stream.latest_raw;
    cv::Mat& result = display.modified_result;
    FrameAverageRuntime& avg_state = display.avg_state;
    ECCRuntime& ecc_state = display.ecc_state;
    bool is_color = false;
    { QMutexLocker mlk(&stream.meta_mutex); is_color = stream.meta.is_color; }

    if (avg_state.seq_sum.empty()) avg_state.seq_sum = cv::Mat::zeros(_h, _w, CV_MAKETYPE(CV_16U, is_color ? 3 : 1));
    if (avg_state.frame_a_sum.empty()) avg_state.frame_a_sum = cv::Mat::zeros(_h, _w, CV_MAKETYPE(CV_16U, is_color ? 3 : 1));
    if (avg_state.frame_b_sum.empty()) avg_state.frame_b_sum = cv::Mat::zeros(_h, _w, CV_MAKETYPE(CV_16U, is_color ? 3 : 1));
    if (pcfg.frame_avg_enabled) {
        int avg_mode = pcfg.frame_avg_mode;

        if (updated) {
            if (avg_state.seq[7].empty()) for (auto& m: avg_state.seq) m = cv::Mat::zeros(_h, _w, CV_MAKETYPE(CV_16U, is_color ? 3 : 1));

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
            avg_state.seq_sum.convertTo(result, CV_MAKETYPE(CV_8U, is_color ? 3 : 1), 1. / (4 * (1 << (_pixel_depth - 8))));
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
                    latest.convertTo(tmp, CV_MAKETYPE(CV_8U, 3), 1. / (1 << (_pixel_depth - 8)));
                    cv::cvtColor(tmp, gray, cv::COLOR_BGR2GRAY);
                    gray.convertTo(gray, CV_32F);
                } else {
                    latest.convertTo(gray, CV_32F, 255.0 / ((1 << _pixel_depth) - 1));
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
                } else {
                    int show_idx = std::max(0, (int)ecc_state.fusion_buf.size() - 1 - ecc_fwd);
                    ecc_state.fusion_buf[show_idx].convertTo(result, CV_8U);
                }
            }
        }
    }
    else {
        if (!avg_state.seq_sum.empty()) avg_state.release_avg_buffers();
        if (!ecc_state.fusion_buf.empty()) ecc_state.clear();
        latest.convertTo(result, CV_MAKETYPE(CV_8U, is_color ? 3 : 1), 1. / (1 << (_pixel_depth - 8)));
    }

    if (params.split) ImageProc::split_img(result, result);

    // 3D image construction — main display only, grayscale only.
    if (pcfg.enable_3d && display.id == DisplayId::Main && !is_color) {
        ww = _w + 104;
        hh = _h;
        cv::Mat dist_mat;
        double dist_min = 0, dist_max = 0;
        if (updated) {
            if (pcfg.frame_avg_enabled) {
                cv::Mat frame_a_avg, frame_b_avg;
                avg_state.frame_a_sum.convertTo(frame_a_avg, _pixel_depth > 8 ? CV_16U : CV_8U, 0.25);
                avg_state.frame_b_sum.convertTo(frame_b_avg, _pixel_depth > 8 ? CV_16U : CV_8U, 0.25);
#ifdef LVTONG
                ImageProc::gated3D_v2(m_g.tcu_ctrl->get_frame_a_3d() ? frame_b_avg : frame_a_avg, m_g.tcu_ctrl->get_frame_a_3d() ? frame_a_avg : frame_b_avg, result,
                                      pcfg.custom_3d_param ? pcfg.custom_3d_delay : (m_g.tcu_ctrl->get_delay_dist() - m_g.device_mgr->tcu()->get(TCU::OFFSET_DELAY) * m_g.tcu_ctrl->get_dist_ns()) / m_g.tcu_ctrl->get_dist_ns(),
                                      pcfg.custom_3d_param ? pcfg.custom_3d_gate_width : (m_g.tcu_ctrl->get_depth_of_view() - m_g.device_mgr->tcu()->get(TCU::OFFSET_GW) * m_g.tcu_ctrl->get_dist_ns()) / m_g.tcu_ctrl->get_dist_ns(),
                                      pcfg.colormap, pcfg.lower_3d_thresh, pcfg.upper_3d_thresh, pcfg.truncate_3d);
#else
                ImageProc::gated3D_v2(m_g.tcu_ctrl->get_frame_a_3d() ? frame_b_avg : frame_a_avg, m_g.tcu_ctrl->get_frame_a_3d() ? frame_a_avg : frame_b_avg, result,
                                      pcfg.custom_3d_param ? pcfg.custom_3d_delay : (m_g.tcu_ctrl->get_delay_dist() - m_g.device_mgr->tcu()->get(TCU::OFFSET_DELAY) * m_g.tcu_ctrl->get_dist_ns()) / m_g.tcu_ctrl->get_dist_ns(),
                                      pcfg.custom_3d_param ? pcfg.custom_3d_gate_width : (m_g.tcu_ctrl->get_depth_of_view() - m_g.device_mgr->tcu()->get(TCU::OFFSET_GW) * m_g.tcu_ctrl->get_dist_ns()) / m_g.tcu_ctrl->get_dist_ns(),
                                      pcfg.colormap, pcfg.lower_3d_thresh, pcfg.upper_3d_thresh, pcfg.truncate_3d, &dist_mat, &dist_min, &dist_max);
#endif
            }
            else {
#ifdef LVTONG
                ImageProc::gated3D_v2(m_g.tcu_ctrl->get_frame_a_3d() ? avg_state.prev_img : latest, m_g.tcu_ctrl->get_frame_a_3d() ? latest : avg_state.prev_img, result,
                                      pcfg.custom_3d_param ? pcfg.custom_3d_delay : (m_g.tcu_ctrl->get_delay_dist() - m_g.device_mgr->tcu()->get(TCU::OFFSET_DELAY) * m_g.tcu_ctrl->get_dist_ns()) / m_g.tcu_ctrl->get_dist_ns(),
                                      pcfg.custom_3d_param ? pcfg.custom_3d_gate_width : (m_g.tcu_ctrl->get_depth_of_view() - m_g.device_mgr->tcu()->get(TCU::OFFSET_GW) * m_g.tcu_ctrl->get_dist_ns()) / m_g.tcu_ctrl->get_dist_ns(),
                                      pcfg.colormap, pcfg.lower_3d_thresh, pcfg.upper_3d_thresh, pcfg.truncate_3d);
#else
                ImageProc::gated3D_v2(m_g.tcu_ctrl->get_frame_a_3d() ? avg_state.prev_img : latest, m_g.tcu_ctrl->get_frame_a_3d() ? latest : avg_state.prev_img, result,
                                      pcfg.custom_3d_param ? pcfg.custom_3d_delay : (m_g.tcu_ctrl->get_delay_dist() - m_g.device_mgr->tcu()->get(TCU::OFFSET_DELAY) * m_g.tcu_ctrl->get_dist_ns()) / m_g.tcu_ctrl->get_dist_ns(),
                                      pcfg.custom_3d_param ? pcfg.custom_3d_gate_width : (m_g.tcu_ctrl->get_depth_of_view() - m_g.device_mgr->tcu()->get(TCU::OFFSET_GW) * m_g.tcu_ctrl->get_dist_ns()) / m_g.tcu_ctrl->get_dist_ns(),
                                      pcfg.colormap, pcfg.lower_3d_thresh, pcfg.upper_3d_thresh, pcfg.truncate_3d, &dist_mat, &dist_min, &dist_max);
#endif
                m_g.tcu_ctrl->toggle_frame_a_3d();
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
        ww = _w; hh = _h;
    }
}

#ifdef LVTONG
double PipelineProcessor::detect_fishnet(StreamSlot& stream, DisplaySlot& display,
    const PipelineConfig& pcfg, cv::dnn::Net& net)
{
    (void)display;
    cv::Mat& latest = stream.latest_raw;
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

void PipelineProcessor::enhance_frame(StreamSlot& stream, DisplaySlot& display,
    const ProcessingParams& params, const PipelineConfig& pcfg)
{
    cv::Mat& result = display.modified_result;
    bool is_color = false;
    { QMutexLocker mlk(&stream.meta_mutex); is_color = stream.meta.is_color; }

    // Skip enhance when 3D is active on main display with grayscale source.
    if (!pcfg.enable_3d || display.id != DisplayId::Main || is_color) {
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

        // brightness / contrast / gamma sliders not in current GUI — re-enable when restored.
        // ImageProc::brightness_and_contrast(result, result, std::exp(params.contrast / 10.), params.brightness * 12.8);
        // ImageProc::brightness_and_contrast(result, result, tan((20 - params.gamma_slider) / 40. * M_PI));
    }

    // Pseudocolor on main display only.
    if (pcfg.pseudocolor_enabled && display.id == DisplayId::Main && result.channels() == 1) {
        cv::Mat mask;
        cv::normalize(result, result, 0, 255, cv::NORM_MINMAX, CV_8UC1, mask);
        cv::Mat temp_mask = result, result_3d;
        cv::applyColorMap(result, result_3d, pcfg.colormap);
        result_3d.copyTo(result, temp_mask);
    }
}

void PipelineProcessor::render_and_display(StreamSlot& stream, DisplaySlot& display,
    const ProcessingParams& params, const PipelineConfig& pcfg,
    const std::vector<YoloResult>& yolo_results,
    double is_net, int ww, int hh, int _w, int _h, float weight,
    const QString& info_tcu, const QString& info_time)
{
    (void)info_tcu;
    Display* disp = display.widget;
    cv::Mat& result = display.modified_result;
    bool is_color = false;
    { QMutexLocker mlk(&stream.meta_mutex); is_color = stream.meta.is_color; }

    // YOLO detection boxes.
    if (!yolo_results.empty()) draw_yolo_boxes(display, result, yolo_results);

    // Histogram side-output: gray-value or 3D distance hist.
    if (m_g.aux_panel->get_alt_display_option() == 2) {
        cv::Mat mat_for_hist;
        if (!is_color && !pcfg.pseudocolor) mat_for_hist = result.clone();
        else                                   cv::cvtColor(result, mat_for_hist, cv::COLOR_RGB2GRAY);
        for (int i = 0; i < _h; i++) for (int j = 0; j < _w; j++) display.hist[(mat_for_hist.data + i * mat_for_hist.cols)[j]]++;

        uchar* img = mat_for_hist.data;
        int step = mat_for_hist.step;
        memset(display.hist, 0, 256 * sizeof(uint));
        for (int i = 0; i < hh; i++) for (int j = 0; j < ww; j++) display.hist[(img + i * step)[j]]++;
        uint max = 0;
        for (int i = 1; i < 256; i++) if (display.hist[i] > max) max = display.hist[i];
        cv::Mat hist_mat = cv::Mat(225, 256, CV_8UC3, cv::Scalar(56, 64, 72));
        for (int i = 1; i < 256; i++) {
            cv::rectangle(hist_mat, cv::Point(i, 225), cv::Point(i + 1, 225 - display.hist[i] * 225.0 / max), cv::Scalar(202, 225, 255));
        }
        emit set_hist_pixmap(QPixmap::fromImage(QImage(hist_mat.data, hist_mat.cols, hist_mat.rows, hist_mat.step, QImage::Format_RGB888).scaled(params.hist_display_size, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
    }

    cv::Rect region(cv::Point(params.display_region.tl().x * ww / params.display_width,
                              params.display_region.tl().y * hh / params.display_height),
                    cv::Point(params.display_region.br().x * ww / params.display_width,
                              params.display_region.br().y * hh / params.display_height));
    if (region.height + region.y > result.rows) region.height = result.rows - region.y;
    if (region.width + region.x > result.cols) region.width = result.cols - region.x;
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

    QImage stream_img = QImage(img_display.data, img_display.cols, img_display.rows, img_display.step, is_color || pcfg.pseudocolor ? QImage::Format_RGB888 : QImage::Format_Indexed8);
    if (disp) disp->emit set_pixmap(QPixmap::fromImage(stream_img.scaled(disp->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation)));

    if (display.id == DisplayId::Main && params.dual_display && m_g.secondary_display) {
        cv::Mat ori_img_display;
        Display* ori_display = m_g.secondary_display->get_display_widget();
        cv::resize(stream.latest_raw, ori_img_display, cv::Size(ori_display->width(), ori_display->height()), 0, 0, cv::INTER_AREA);
        QImage ori_img = QImage(ori_img_display.data, ori_img_display.cols, ori_img_display.rows, ori_img_display.step, is_color ? QImage::Format_RGB888 : QImage::Format_Indexed8);
        ori_display->emit set_pixmap(QPixmap::fromImage(ori_img).scaled(ori_display->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

// Main-display-only scan step. Reaches across slots via the router because
// scan walks main+alt1 in lockstep.
void PipelineProcessor::advance_scan_main(DisplayRouter& router, StreamSlot& main_stream, DisplaySlot& main_display,
    bool updated, const PipelineConfig& pcfg,
    int& scan_img_count,
    QString& scan_save_path_a, QString& scan_save_path)
{
    (void)main_display;
    if (!m_g.scan_ctrl->is_scanning() || !updated) return;

    scan_img_count -= 1;
    if (scan_img_count < 0) scan_img_count = 0;

    int save_img_num = m_g.scan_params->num_single_pos;
    if (scan_img_count > 0 && scan_img_count <= save_img_num) {
        save_scan_img(router, main_stream, scan_save_path, QString::number(save_img_num - scan_img_count + 1) + ".bmp", pcfg);
    }
    if (scan_img_count == save_img_num) {
        emit save_screenshot_signal(scan_save_path + "/screenshot_" + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".png");
    }

    if (!scan_img_count) {
        float fps = 0.0f;
        { QMutexLocker mlk(&main_stream.meta_mutex); fps = main_stream.meta.fps; }
        if (fps <= 0.0f) fps = 1.0f; // safety floor
        scan_img_count = fps * (m_g.scan_params->ptz_wait_time + save_img_num / fps);

        int tcu_idx = m_g.scan_ctrl->get_scan_tcu_idx();
        int ptz_idx = m_g.scan_ctrl->get_scan_ptz_idx();
        const auto tcu_route = m_g.scan_ctrl->get_scan_tcu_route();
        const auto ptz_route = m_g.scan_ctrl->get_scan_ptz_route();

        if (tcu_idx == (int)tcu_route.size() - 1 || tcu_idx == -1) {
            m_g.scan_ctrl->set_scan_tcu_idx(-1);
            tcu_idx = -1;
            ptz_idx++;
            m_g.scan_ctrl->set_scan_ptz_idx(ptz_idx);

            if (ptz_idx < (int)ptz_route.size()) {
                m_g.device_mgr->send_ptz_angle(ptz_route[ptz_idx].first, ptz_route[ptz_idx].second);

                scan_save_path_a = *m_g.save_location + "/" + m_g.scan_ctrl->get_scan_name() + "/" + QString::number(ptz_idx);
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
                emit finish_scan_signal();
            }
        }

        tcu_idx = m_g.scan_ctrl->get_scan_tcu_idx() + 1;
        m_g.scan_ctrl->set_scan_tcu_idx(tcu_idx);
        m_g.tcu_ctrl->set_delay_dist(tcu_route[tcu_idx] * m_g.tcu_ctrl->get_dist_ns());
        scan_save_path = scan_save_path_a + "/" + QString::number(tcu_route[tcu_idx], 'f', 2) + " ns";
        QDir().mkdir(scan_save_path);
        QDir().mkdir(scan_save_path + "/ori_bmp");
        QDir().mkdir(scan_save_path + "/res_bmp");

        // Alt1 contributes a side capture if it's currently routed.
        StreamId alt1_stream = router.stream_routed_to(DisplayId::Alt1);
        if (alt1_stream != StreamId::None) QDir().mkdir(scan_save_path + "/alt_bmp");

        emit update_delay_in_thread();
    }
}

void PipelineProcessor::record_frame(StreamSlot& stream, DisplaySlot& display, bool updated,
    const ProcessingParams& params, const PipelineConfig& pcfg,
    double is_net,
    const QString& info_tcu, const QString& info_time,
    int ww, int hh, float weight)
{
    (void)info_tcu;
    bool is_color = false;
    { QMutexLocker mlk(&stream.meta_mutex); is_color = stream.meta.is_color; }

    if (updated && display.save_raw) {
        save_to_file(stream, display, false, params, pcfg);
        if (source_repush_on_pause(stream.meta.source_type) || !pcfg.consecutive_capture || display.id != DisplayId::Main) display.save_raw = false;
    }
    if (updated && display.save_proc) {
        save_to_file(stream, display, true, params, pcfg);
        if (source_repush_on_pause(stream.meta.source_type) || !pcfg.consecutive_capture || display.id != DisplayId::Main) display.save_proc = false;
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

void PipelineProcessor::save_to_file(StreamSlot& stream, DisplaySlot& display,
    bool save_result, const ProcessingParams& params, const PipelineConfig& pcfg)
{
    cv::Mat* temp = save_result ? &display.modified_result : &stream.latest_raw;
    int pixel_depth = 8;
    { QMutexLocker mlk(&stream.meta_mutex); pixel_depth = stream.meta.pixel_depth; }

    cv::Mat result_image;
    if (pcfg.save_in_grayscale) cv::cvtColor(*temp, result_image, cv::COLOR_RGB2GRAY);
    else                         result_image = temp->clone();

    if (save_result) {
        QString dt = QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz");
        if (params.split) {
            if (pcfg.integrate_info) {
                QString tcu_note = QString::fromStdString(m_g.device_mgr->tcu()->to_json().dump());
                if (!m_g.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString, QString)>(&ImageIO::save_image_bmp), result_image(cv::Rect(             0,              0, temp->cols / 2, temp->rows / 2)).clone(), *m_g.save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + dt + "_0" + ".bmp", tcu_note))) emit task_queue_full();
                if (!m_g.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString, QString)>(&ImageIO::save_image_bmp), result_image(cv::Rect(temp->cols / 2,              0, temp->cols / 2, temp->rows / 2)).clone(), *m_g.save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + dt + "_1" + ".bmp", tcu_note))) emit task_queue_full();
                if (!m_g.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString, QString)>(&ImageIO::save_image_bmp), result_image(cv::Rect(             0, temp->rows / 2, temp->cols / 2, temp->rows / 2)).clone(), *m_g.save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + dt + "_2" + ".bmp", tcu_note))) emit task_queue_full();
                if (!m_g.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString, QString)>(&ImageIO::save_image_bmp), result_image(cv::Rect(temp->cols / 2, temp->rows / 2, temp->cols / 2, temp->rows / 2)).clone(), *m_g.save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + dt + "_3" + ".bmp", tcu_note))) emit task_queue_full();
            } else {
                if (!m_g.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString)>(&ImageIO::save_image_bmp), result_image(cv::Rect(             0,              0, temp->cols / 2, temp->rows / 2)).clone(), *m_g.save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + dt + "_0" + ".bmp"))) emit task_queue_full();
                if (!m_g.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString)>(&ImageIO::save_image_bmp), result_image(cv::Rect(temp->cols / 2,              0, temp->cols / 2, temp->rows / 2)).clone(), *m_g.save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + dt + "_1" + ".bmp"))) emit task_queue_full();
                if (!m_g.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString)>(&ImageIO::save_image_bmp), result_image(cv::Rect(             0, temp->rows / 2, temp->cols / 2, temp->rows / 2)).clone(), *m_g.save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + dt + "_2" + ".bmp"))) emit task_queue_full();
                if (!m_g.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString)>(&ImageIO::save_image_bmp), result_image(cv::Rect(temp->cols / 2, temp->rows / 2, temp->cols / 2, temp->rows / 2)).clone(), *m_g.save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + dt + "_3" + ".bmp"))) emit task_queue_full();
            }
        }
        switch (pcfg.img_format) {
        case 0:
            if (pcfg.integrate_info) {
                QString tcu_note = QString::fromStdString(m_g.device_mgr->tcu()->to_json().dump());
                if (!m_g.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString, QString)>(&ImageIO::save_image_bmp), result_image, *m_g.save_location + "/res_bmp/" + dt + ".bmp", tcu_note))) emit task_queue_full();
            } else {
                if (!m_g.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString)>(&ImageIO::save_image_bmp), result_image, *m_g.save_location + "/res_bmp/" + dt + ".bmp"))) emit task_queue_full();
            }
            break;
        case 1: if (!m_g.tp->append_task(std::bind(ImageIO::save_image_jpg, result_image, *m_g.save_location + "/res_bmp/" + dt + ".jpg"))) emit task_queue_full(); break;
        default: break;
        }
    } else {
        switch (pixel_depth) {
        case 8:
            switch (pcfg.img_format) {
            case 0:
                if (pcfg.integrate_info) {
                    QString tcu_note = QString::fromStdString(m_g.device_mgr->tcu()->to_json().dump());
                    if (!m_g.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString, QString)>(&ImageIO::save_image_bmp), result_image, *m_g.save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".bmp", tcu_note))) emit task_queue_full();
                } else {
                    if (!m_g.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString)>(&ImageIO::save_image_bmp), result_image, *m_g.save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".bmp"))) emit task_queue_full();
                }
                break;
            case 1: if (!m_g.tp->append_task(std::bind(ImageIO::save_image_jpg, result_image, *m_g.save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".jpg"))) emit task_queue_full(); break;
            default: break;
            }
            break;
        case 10:
        case 12:
        case 16:
            switch (pcfg.img_format) {
            case 0: if (!m_g.tp->append_task(std::bind(ImageIO::save_image_tif, result_image * (1 << (16 - pixel_depth)), *m_g.save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".tif"))) emit task_queue_full(); break;
            case 1: if (!m_g.tp->append_task(std::bind(ImageIO::save_image_jpg, result_image * (1 << (16 - pixel_depth)), *m_g.save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".jpg"))) emit task_queue_full(); break;
            default: break;
            }
            break;
        default: break;
        }
    }
}

void PipelineProcessor::save_scan_img(DisplayRouter& router, StreamSlot& main_stream,
    QString path, QString name, const PipelineConfig& pcfg)
{
    DisplaySlot& main_display = router.display(DisplayId::Main);

    if (m_g.scan_params->capture_scan_ori) {
        if (pcfg.integrate_info) {
            QString tcu_note = QString::fromStdString(m_g.device_mgr->tcu()->to_json().dump());
            m_g.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString, QString)>(&ImageIO::save_image_bmp), main_stream.latest_raw.clone(), path + "/ori_bmp/" + name, tcu_note));
        } else {
            m_g.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString)>(&ImageIO::save_image_bmp), main_stream.latest_raw.clone(), path + "/ori_bmp/" + name));
        }
    }
    if (m_g.scan_params->capture_scan_res) {
        if (pcfg.integrate_info) {
            QString tcu_note = QString::fromStdString(m_g.device_mgr->tcu()->to_json().dump());
            m_g.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString, QString)>(&ImageIO::save_image_bmp), main_display.modified_result.clone(), path + "/res_bmp/" + name, tcu_note));
        } else {
            m_g.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString)>(&ImageIO::save_image_bmp), main_display.modified_result.clone(), path + "/res_bmp/" + name));
        }
    }

    StreamId alt1_stream = router.stream_routed_to(DisplayId::Alt1);
    if (alt1_stream != StreamId::None) {
        StreamSlot& alt1 = router.stream(alt1_stream);
        if (pcfg.integrate_info) {
            QString tcu_note = QString::fromStdString(m_g.device_mgr->tcu()->to_json().dump());
            m_g.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString, QString)>(&ImageIO::save_image_bmp), alt1.latest_raw.clone(), path + "/alt_bmp/" + name, tcu_note));
        } else {
            m_g.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString)>(&ImageIO::save_image_bmp), alt1.latest_raw.clone(), path + "/alt_bmp/" + name));
        }
    }
}

void PipelineProcessor::draw_yolo_boxes(DisplaySlot& display,
    cv::Mat& image, const std::vector<YoloResult>& results)
{
    if (image.empty() || results.empty()) return;

    cv::Mat draw_image;
    if (image.channels() == 1) cv::cvtColor(image, draw_image, cv::COLOR_GRAY2BGR);
    else                       draw_image = image;

    YoloDetector* det_for_labels = display.yolo;
    for (const auto& det : results) {
        cv::Scalar color(0, 255, 0);
        cv::rectangle(draw_image, det.box, color, 2);

        std::string label;
        if (det_for_labels && det.classId < static_cast<int>(det_for_labels->getClassNames().size())) {
            label = det_for_labels->getClassNames()[det.classId];
        } else {
            label = "class_" + std::to_string(det.classId);
        }
        label += ": " + std::to_string(static_cast<int>(det.confidence * 100)) + "%";

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
