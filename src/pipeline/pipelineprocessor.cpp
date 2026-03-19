#include "pipeline/pipelineprocessor.h"
#include "controller/tcucontroller.h"
#include "controller/scancontroller.h"
#include "controller/devicemanager.h"
#include "controller/auxpanelmanager.h"
#include "visual/scanconfig.h"
#include "widgets/display.h"
#include "widgets/floatingwindow.h"
#include "image/imageio.h"
#include "image/imageproc.h"
#include "util/threadpool.h"
#include "util/constants.h"
#include "port/tcu.h"

#include <QThread>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QCoreApplication>
#include <cmath>

PipelineProcessor::PipelineProcessor(SharedState state, QObject* parent)
    : QObject(parent), m_s(state)
{
}

int PipelineProcessor::run(int *idx) {
    int thread_idx = *idx;
    m_s.grab_thread_state[thread_idx] = true;
    int _w = m_s.w[thread_idx], _h = m_s.h[thread_idx], _pixel_depth = m_s.pixel_depth[thread_idx];
    bool updated;
    FrameAverageState avg_state;
    ECCState ecc_state;
    uint hist[256];
    cv::Mat sobel;
    int ww, hh, scan_img_count = -1;
    float weight = _h / FONT_SCALE_DIVISOR; // font scale & thickness
    avg_state.prev_3d = cv::Mat(_h, _w, CV_8UC3);
    avg_state.prev_img = cv::Mat(_h, _w, CV_MAKETYPE(_pixel_depth == 8 ? CV_8U : CV_16U, m_s.is_color[thread_idx] ? 3 : 1));
    m_s.user_mask[thread_idx] = cv::Mat::zeros(_h, _w, CV_8UC1);
    int packets_lost = 0;
#ifdef LVTONG
    emit set_model_list_enabled(false);
    QString model_name;
    { QMutexLocker lk(m_s.m_params_mutex); switch (m_s.m_pipeline_config->model_idx) {
    case 0: model_name = "models/fishnet_lvtong_legacy.onnx"; break;
    case 1: model_name = "models/fishnet_pix2pix_maxpool.onnx"; break;
    case 2: model_name = "models/fishnet_maskguided.onnx"; break;
    case 3: model_name = "models/fishnet_semantic_static_sim.onnx"; break;
    default: break;
    } }

    cv::dnn::Net net = cv::dnn::readNet(model_name.toLatin1().constData());
#endif

    // YOLO detector pointer (initialized dynamically in the loop below)
    YoloDetector* yolo = nullptr;

    // variables that persist across loop iterations (per-thread, not static)
    QString scan_save_path_a, scan_save_path;

    while (m_s.grab_image[thread_idx]) {
        ProcessingParams params;
        PipelineConfig pcfg;
        { QMutexLocker lk(m_s.m_params_mutex); params = *m_s.m_processing_params; pcfg = *m_s.m_pipeline_config; }

        if (!acquire_frame(thread_idx, avg_state.prev_img, updated, packets_lost)) continue;

        {
            QMutexLocker processing_locker(&m_s.image_mutex[thread_idx]);

        if (updated) {
            preprocess_frame(thread_idx, params, pcfg, avg_state, _w, _h, hist);
        }

        std::vector<YoloResult> yolo_results = detect_yolo(thread_idx, updated, pcfg, yolo);

        double is_net = 0.0;
#ifdef LVTONG
        is_net = detect_fishnet(thread_idx, pcfg, net);
#endif

        frame_average_and_3d(thread_idx, updated, params, pcfg, avg_state, ecc_state, _w, _h, _pixel_depth, ww, hh);

        enhance_frame(thread_idx, params, pcfg);

        // compute info strings (needed by both render_and_display and record_frame)
        QString info_tcu = pcfg.custom_topleft_info ?
                    params.custom_info_text :
                    m_s.tcu_ctrl->get_base_unit() == 2 ? QString::asprintf("DIST %05d m  DOV %04d m", (int)m_s.tcu_ctrl->get_delay_dist(), (int)m_s.tcu_ctrl->get_depth_of_view()) :
                                     QString::asprintf("DELAY %06d ns  GATE %04d ns", (int)std::round(m_s.tcu_ctrl->get_delay_dist() / m_s.tcu_ctrl->get_dist_ns()), (int)std::round(m_s.tcu_ctrl->get_depth_of_view() / m_s.tcu_ctrl->get_dist_ns()));
        QString info_time = QDateTime::currentDateTime().toString("hh:mm:ss:zzz");

        render_and_display(thread_idx, *idx, params, pcfg, yolo_results, is_net, ww, hh, _w, _h, weight, hist, info_tcu, info_time);

        advance_scan(thread_idx, updated, pcfg, scan_img_count, scan_save_path_a, scan_save_path);

        record_frame(thread_idx, *idx, updated, params, pcfg, is_net, info_tcu, info_time, ww, hh, weight);

        if (updated) avg_state.prev_img = m_s.img_mem[thread_idx].clone();
        } // QMutexLocker automatically unlocks here
    }
#ifdef LVTONG
    emit set_model_list_enabled(true);
#endif
    m_s.grab_thread_state[thread_idx] = false;
    return 0;
}

bool PipelineProcessor::acquire_frame(int thread_idx, cv::Mat& prev_img, bool& updated, int& packets_lost)
{
    QMutexLocker locker(&m_s.image_mutex[thread_idx]);

    // Limit queue size safely
    while (m_s.q_img[thread_idx].size() > MAX_QUEUE_SIZE) {
        m_s.q_img[thread_idx].pop();
        if (*m_s.device_type == 1) {
            QMutexLocker frame_locker(m_s.frame_info_mutex);
            if (!m_s.q_frame_info->empty()) {
                m_s.q_frame_info->pop();
            }
        }
    }

    // Detect and handle queue desynchronization for device_type == 1
    if (*m_s.device_type == 1) {
        QMutexLocker frame_locker(m_s.frame_info_mutex);
        if (m_s.q_img[thread_idx].size() != m_s.q_frame_info->size()) {
            qDebug() << "WARNING: Queue desynchronization detected!"
                     << "q_img size:" << m_s.q_img[thread_idx].size()
                     << "q_frame_info size:" << m_s.q_frame_info->size();
            while (!m_s.q_frame_info->empty() && m_s.q_frame_info->size() > m_s.q_img[thread_idx].size()) {
                m_s.q_frame_info->pop();
            }
        }
    }

    // Safe check and access
    if (m_s.q_img[thread_idx].empty()) {
#ifdef LVTONG
        locker.unlock();
        QThread::msleep(QUEUE_EMPTY_SLEEP_MS);
        return false; // caller should continue (skip iteration)
#else
        m_s.img_mem[thread_idx] = prev_img.clone();
        updated = false;
#endif
    }
    else {
        m_s.img_mem[thread_idx] = m_s.q_img[thread_idx].front();
        m_s.q_img[thread_idx].pop();
        int packets_lost_frame = 0;
        if (*m_s.device_type == 1) {
            QMutexLocker frame_locker(m_s.frame_info_mutex);
            if (!m_s.q_frame_info->empty()) {
                packets_lost_frame = m_s.q_frame_info->front();
                m_s.q_frame_info->pop();
            }
        }
        packets_lost += packets_lost_frame;
        if (packets_lost_frame) emit packet_lost_updated(packets_lost);
        updated = true;
        if (*m_s.device_type == -1) m_s.q_img[thread_idx].push(m_s.img_mem[thread_idx].clone());
        m_s.q_fps_calc->add_to_q();
    }
    return true;
}

void PipelineProcessor::preprocess_frame(int thread_idx, const ProcessingParams& params,
    const PipelineConfig& pcfg,
    FrameAverageState& avg_state, int& _w, int& _h, uint hist[256])
{
    switch (params.image_rotate) {
    case 0:
        if (_w != m_s.w[thread_idx]) avg_state.release_all();
        _w = m_s.w[thread_idx];
        _h = m_s.h[thread_idx];
        break;
    case 2:
        cv::flip(m_s.img_mem[thread_idx], m_s.img_mem[thread_idx], -1);
        if (_w != m_s.w[thread_idx]) avg_state.release_all();
        _w = m_s.w[thread_idx];
        _h = m_s.h[thread_idx];
        break;
    case 1:
        cv::flip(m_s.img_mem[thread_idx], m_s.img_mem[thread_idx], -1);
    case 3:
        cv::flip(m_s.img_mem[thread_idx], m_s.img_mem[thread_idx], 0);
        cv::transpose(m_s.img_mem[thread_idx], m_s.img_mem[thread_idx]);
        if (_w != m_s.h[thread_idx]) avg_state.release_all();
        _w = m_s.h[thread_idx];
        _h = m_s.w[thread_idx];
        break;
    default:
        break;
    }

    // calc histogram (grayscale)
    memset(hist, 0, 256 * sizeof(uint));
    if (!m_s.is_color[thread_idx]) for (int i = 0; i < _h; i++) for (int j = 0; j < _w; j++) hist[(m_s.img_mem[thread_idx].data + i * m_s.img_mem[thread_idx].cols)[j]]++;

    // if the image needs flipping
    { int sym = pcfg.flip; if (sym) cv::flip(m_s.img_mem[thread_idx], m_s.img_mem[thread_idx], sym - 2); }

    // mcp self-adaptive
    if (m_s.tcu_ctrl->get_auto_mcp() && !params.mcp_slider_focused) {
        int thresh_num = m_s.img_mem[thread_idx].total() / AUTO_MCP_DIVISOR, thresh = (1 << m_s.pixel_depth[thread_idx]) - 1;
        while (thresh && thresh_num > 0) thresh_num -= hist[thresh--];
        if (thresh > (1 << m_s.pixel_depth[thread_idx]) * 0.94) emit update_mcp_in_thread(m_s.device_mgr->tcu()->get(TCU::MCP) - sqrt(thresh - (1 << m_s.pixel_depth[thread_idx]) * 0.94));
        if (thresh < (1 << m_s.pixel_depth[thread_idx]) * 0.39) emit update_mcp_in_thread(m_s.device_mgr->tcu()->get(TCU::MCP) + sqrt((1 << m_s.pixel_depth[thread_idx]) * 0.39 - thresh));
    }
}

std::vector<YoloResult> PipelineProcessor::detect_yolo(int thread_idx, bool updated,
    const PipelineConfig& pcfg, YoloDetector*& yolo)
{
    // Check if YOLO model selection has changed and update detector if needed
    int current_model_selection = 0;
    if (thread_idx == 0) {
        current_model_selection = pcfg.main_display_model;
    } else if (thread_idx == 1) {
        current_model_selection = pcfg.alt1_display_model;
    } else if (thread_idx == 2) {
        current_model_selection = pcfg.alt2_display_model;
    }

    // If model changed, reinitialize
    if (current_model_selection != m_s.m_yolo_last_model[thread_idx]) {
        QMutexLocker locker(m_s.m_yolo_init_mutex);

        if (m_s.m_yolo_detector[thread_idx]) {
            qDebug() << "YOLO model changed for thread" << thread_idx << "- reinitializing";
            delete m_s.m_yolo_detector[thread_idx];
            m_s.m_yolo_detector[thread_idx] = nullptr;
            yolo = nullptr;
        }

        if (current_model_selection > 0) {
            m_s.m_yolo_detector[thread_idx] = new YoloDetector();
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

            if (!model_path.isEmpty() && m_s.m_yolo_detector[thread_idx]->initialize(cfg_path, model_path, classes_file)) {
                yolo = m_s.m_yolo_detector[thread_idx];
                qDebug() << "YOLO detector reloaded for thread" << thread_idx << "with model" << current_model_selection;
            } else {
                qWarning() << "Failed to reload YOLO for thread" << thread_idx << "- will not retry";
                delete m_s.m_yolo_detector[thread_idx];
                m_s.m_yolo_detector[thread_idx] = nullptr;
                yolo = nullptr;
            }
        }

        m_s.m_yolo_last_model[thread_idx] = current_model_selection;
    }

    // YOLO detection on original image
    std::vector<YoloResult> results;
    if (yolo && yolo->isInitialized() && updated) {
        cv::Mat det_input;
        if (m_s.img_mem[thread_idx].channels() == 1) {
            cv::cvtColor(m_s.img_mem[thread_idx], det_input, cv::COLOR_GRAY2BGR);
        } else {
            det_input = m_s.img_mem[thread_idx];
        }
        yolo->run(det_input, results);
    }
    return results;
}

void PipelineProcessor::frame_average_and_3d(int thread_idx, bool updated, const ProcessingParams& params,
    const PipelineConfig& pcfg,
    FrameAverageState& avg_state, ECCState& ecc_state, int _w, int _h, int _pixel_depth,
    int& ww, int& hh)
{
    // process frame average
    if (avg_state.seq_sum.empty()) avg_state.seq_sum = cv::Mat::zeros(_h, _w, CV_MAKETYPE(CV_16U, m_s.is_color[thread_idx] ? 3 : 1));
    if (avg_state.frame_a_sum.empty()) avg_state.frame_a_sum = cv::Mat::zeros(_h, _w, CV_MAKETYPE(CV_16U, m_s.is_color[thread_idx] ? 3 : 1));
    if (avg_state.frame_b_sum.empty()) avg_state.frame_b_sum = cv::Mat::zeros(_h, _w, CV_MAKETYPE(CV_16U, m_s.is_color[thread_idx] ? 3 : 1));
    if (params.frame_avg_enabled) {
        int avg_mode = params.frame_avg_mode;

        // A/B running sums maintained for 3D reconstruction regardless of mode
        if (updated) {
            if (avg_state.seq[7].empty()) for (auto& m: avg_state.seq) m = cv::Mat::zeros(_h, _w, CV_MAKETYPE(CV_16U, m_s.is_color[thread_idx] ? 3 : 1));

            avg_state.seq_sum -= avg_state.seq[(avg_state.seq_idx + 4) & 7];
            if (avg_state.frame_a) avg_state.frame_a_sum -= avg_state.seq[avg_state.seq_idx];
            else                   avg_state.frame_b_sum -= avg_state.seq[avg_state.seq_idx];
            m_s.img_mem[thread_idx].convertTo(avg_state.seq[avg_state.seq_idx], CV_MAKETYPE(CV_16U, m_s.is_color[thread_idx] ? 3 : 1));
            avg_state.seq_sum += avg_state.seq[avg_state.seq_idx];
            if (avg_state.frame_a) avg_state.frame_a_sum += avg_state.seq[avg_state.seq_idx];
            else                   avg_state.frame_b_sum += avg_state.seq[avg_state.seq_idx];

            avg_state.seq_idx = (avg_state.seq_idx + 1) & 7;
            avg_state.frame_a ^= 1;
        }

        if (avg_mode == 0) {
            // --- option "a": display from 4-frame running sum ---
            avg_state.seq_sum.convertTo(m_s.modified_result[thread_idx], CV_MAKETYPE(CV_8U, m_s.is_color[thread_idx] ? 3 : 1), 1. / (4 * (1 << (_pixel_depth - 8))));
        }
        else {
            // --- option "ECC": display from temporal denoising ---
            if (updated) {
                // clear state on warp mode change (matrix shape mismatch)
                if (pcfg.ecc_warp_mode != ecc_state.prev_ecc_warp_mode) {
                    ecc_state.clear();
                    ecc_state.prev_ecc_warp_mode = pcfg.ecc_warp_mode;
                }

                // convert current frame to grayscale CV_32F [0,255]
                cv::Mat gray;
                if (m_s.is_color[thread_idx]) {
                    cv::Mat tmp;
                    m_s.img_mem[thread_idx].convertTo(tmp, CV_MAKETYPE(CV_8U, 3), 1. / (1 << (_pixel_depth - 8)));
                    cv::cvtColor(tmp, gray, cv::COLOR_BGR2GRAY);
                    gray.convertTo(gray, CV_32F);
                } else {
                    m_s.img_mem[thread_idx].convertTo(gray, CV_32F, 255.0 / ((1 << _pixel_depth) - 1));
                }

                // register with previous frame
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

                // compute effective forward
                int ecc_fwd = (pcfg.ecc_window_mode == 1) ? pcfg.ecc_backward
                            : (pcfg.ecc_window_mode == 2) ? pcfg.ecc_forward : 0;

                // push frame to buffer
                ecc_state.fusion_buf.push_back(gray);
                int max_buf = pcfg.ecc_backward + 1 + ecc_fwd;
                while ((int)ecc_state.fusion_buf.size() > max_buf) {
                    ecc_state.fusion_buf.pop_front();
                    if (!ecc_state.fusion_warps.empty()) ecc_state.fusion_warps.pop_front();
                    if (!ecc_state.fusion_warps_inv.empty()) ecc_state.fusion_warps_inv.pop_front();
                }

                // fuse when we have enough frames
                int min_frames = (ecc_fwd > 0) ? pcfg.ecc_backward + 1 + ecc_fwd : 2;
                if ((int)ecc_state.fusion_buf.size() >= min_frames) {
                    int target = (int)ecc_state.fusion_buf.size() - 1 - ecc_fwd;
                    ImageProc::temporal_denoise_fuse(
                        ecc_state.fusion_buf, ecc_state.fusion_warps, ecc_state.fusion_warps_inv,
                        target, pcfg.ecc_backward, ecc_fwd,
                        m_s.modified_result[thread_idx],
                        pcfg.ecc_half_res_fuse,
                        pcfg.ecc_warp_mode,
                        pcfg.ecc_fusion_method);
                } else {
                    int show_idx = std::max(0, (int)ecc_state.fusion_buf.size() - 1 - ecc_fwd);
                    ecc_state.fusion_buf[show_idx].convertTo(m_s.modified_result[thread_idx], CV_8U);
                }
            }
        }
    }
    else {
        if (!avg_state.seq_sum.empty()) {
            avg_state.release_avg_buffers();
        }
        if (!ecc_state.fusion_buf.empty()) {
            ecc_state.clear();
        }
        m_s.img_mem[thread_idx].convertTo(m_s.modified_result[thread_idx], CV_MAKETYPE(CV_8U, m_s.is_color[thread_idx] ? 3 : 1), 1. / (1 << (_pixel_depth - 8)));
    }

    if (params.split) ImageProc::split_img(m_s.modified_result[thread_idx], m_s.modified_result[thread_idx]);

    // process 3d image construction from ABN frames
    if (params.enable_3d && thread_idx == 0 && !m_s.is_color[thread_idx]) {
        ww = _w + 104;
        hh = _h;
        cv::Mat dist_mat;
        double dist_min = 0, dist_max = 0;
        if (updated) {
            if (params.frame_avg_enabled) {
                cv::Mat frame_a_avg, frame_b_avg;
                avg_state.frame_a_sum.convertTo(frame_a_avg, _pixel_depth > 8 ? CV_16U : CV_8U, 0.25);
                avg_state.frame_b_sum.convertTo(frame_b_avg, _pixel_depth > 8 ? CV_16U : CV_8U, 0.25);
#ifdef LVTONG
                ImageProc::gated3D_v2(m_s.tcu_ctrl->get_frame_a_3d() ? frame_b_avg : frame_a_avg, m_s.tcu_ctrl->get_frame_a_3d() ? frame_a_avg : frame_b_avg, m_s.modified_result[thread_idx],
                                      pcfg.custom_3d_param ? pcfg.custom_3d_delay : (m_s.tcu_ctrl->get_delay_dist() - m_s.device_mgr->tcu()->get(TCU::OFFSET_DELAY) * m_s.tcu_ctrl->get_dist_ns()) / m_s.tcu_ctrl->get_dist_ns(),
                                      pcfg.custom_3d_param ? pcfg.custom_3d_gate_width : (m_s.tcu_ctrl->get_depth_of_view() - m_s.device_mgr->tcu()->get(TCU::OFFSET_GW) * m_s.tcu_ctrl->get_dist_ns()) / m_s.tcu_ctrl->get_dist_ns(),
                                      pcfg.colormap, pcfg.lower_3d_thresh, pcfg.upper_3d_thresh, pcfg.truncate_3d);
#else //LVTONG
                ImageProc::gated3D_v2(m_s.tcu_ctrl->get_frame_a_3d() ? frame_b_avg : frame_a_avg, m_s.tcu_ctrl->get_frame_a_3d() ? frame_a_avg : frame_b_avg, m_s.modified_result[thread_idx],
                                      pcfg.custom_3d_param ? pcfg.custom_3d_delay : (m_s.tcu_ctrl->get_delay_dist() - m_s.device_mgr->tcu()->get(TCU::OFFSET_DELAY) * m_s.tcu_ctrl->get_dist_ns()) / m_s.tcu_ctrl->get_dist_ns(),
                                      pcfg.custom_3d_param ? pcfg.custom_3d_gate_width : (m_s.tcu_ctrl->get_depth_of_view() - m_s.device_mgr->tcu()->get(TCU::OFFSET_GW) * m_s.tcu_ctrl->get_dist_ns()) / m_s.tcu_ctrl->get_dist_ns(),
                                      pcfg.colormap, pcfg.lower_3d_thresh, pcfg.upper_3d_thresh, pcfg.truncate_3d, &dist_mat, &dist_min, &dist_max);
#endif //LVTONG
            }
            else {
#ifdef LVTONG
                ImageProc::gated3D_v2(m_s.tcu_ctrl->get_frame_a_3d() ? avg_state.prev_img : m_s.img_mem[thread_idx], m_s.tcu_ctrl->get_frame_a_3d() ? m_s.img_mem[thread_idx] : avg_state.prev_img, m_s.modified_result[thread_idx],
                                      pcfg.custom_3d_param ? pcfg.custom_3d_delay : (m_s.tcu_ctrl->get_delay_dist() - m_s.device_mgr->tcu()->get(TCU::OFFSET_DELAY) * m_s.tcu_ctrl->get_dist_ns()) / m_s.tcu_ctrl->get_dist_ns(),
                                      pcfg.custom_3d_param ? pcfg.custom_3d_gate_width : (m_s.tcu_ctrl->get_depth_of_view() - m_s.device_mgr->tcu()->get(TCU::OFFSET_GW) * m_s.tcu_ctrl->get_dist_ns()) / m_s.tcu_ctrl->get_dist_ns(),
                                      pcfg.colormap, pcfg.lower_3d_thresh, pcfg.upper_3d_thresh, pcfg.truncate_3d);
#else //LVTONG
                ImageProc::gated3D_v2(m_s.tcu_ctrl->get_frame_a_3d() ? avg_state.prev_img : m_s.img_mem[thread_idx], m_s.tcu_ctrl->get_frame_a_3d() ? m_s.img_mem[thread_idx] : avg_state.prev_img, m_s.modified_result[thread_idx],
                                      pcfg.custom_3d_param ? pcfg.custom_3d_delay : (m_s.tcu_ctrl->get_delay_dist() - m_s.device_mgr->tcu()->get(TCU::OFFSET_DELAY) * m_s.tcu_ctrl->get_dist_ns()) / m_s.tcu_ctrl->get_dist_ns(),
                                      pcfg.custom_3d_param ? pcfg.custom_3d_gate_width : (m_s.tcu_ctrl->get_depth_of_view() - m_s.device_mgr->tcu()->get(TCU::OFFSET_GW) * m_s.tcu_ctrl->get_dist_ns()) / m_s.tcu_ctrl->get_dist_ns(),
                                      pcfg.colormap, pcfg.lower_3d_thresh, pcfg.upper_3d_thresh, pcfg.truncate_3d, &dist_mat, &dist_min, &dist_max);
#endif //LVTONG
                m_s.tcu_ctrl->toggle_frame_a_3d();
            }
#ifdef LVTONG
#else //LVTONG
            avg_state.prev_3d = m_s.modified_result[thread_idx].clone();
#endif //LVTONG
        }
        else m_s.modified_result[thread_idx] = avg_state.prev_3d;
#ifdef LVTONG
#else
        cv::Mat masked_dist;
        {
            QMutexLocker lk(m_s.m_roi_mutex);
            if (m_s.list_roi->size()) dist_mat.copyTo(masked_dist, m_s.user_mask[thread_idx]);
        }
        if (!masked_dist.empty()) emit update_dist_mat(masked_dist, dist_min, dist_max);
        else emit update_dist_mat(dist_mat, dist_min, dist_max);
#endif
    }
    else {
        ww = _w;
        hh = _h;
    }
}

#ifdef LVTONG
double PipelineProcessor::detect_fishnet(int thread_idx, const PipelineConfig& pcfg, cv::dnn::Net& net)
{
    double is_net = 0;
    double min, max;
    if (pcfg.fishnet_recog) {
        cv::Mat fishnet_res;
        switch (pcfg.model_idx) {
        case 0:
        {
            cv::cvtColor(m_s.img_mem[thread_idx], fishnet_res, cv::COLOR_GRAY2RGB);
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
            m_s.img_mem[thread_idx].convertTo(fishnet_res, CV_32FC1, 1.0 / 255);
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
            m_s.img_mem[thread_idx].convertTo(fishnet_res, CV_32FC1, 1.0 / 255);
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

void PipelineProcessor::enhance_frame(int thread_idx, const ProcessingParams& params,
    const PipelineConfig& pcfg)
{
    // process ordinary image enhance (skip when 3D is active on thread 0 with grayscale)
    if (!params.enable_3d || thread_idx != 0 || m_s.is_color[thread_idx]) {
        if (params.enhance_enabled) {
            switch (params.enhance_option) {
            // histogram
            case 1: {
                ImageProc::hist_equalization(m_s.modified_result[thread_idx], m_s.modified_result[thread_idx]);
                break;
            }
            // laplace
            case 2: {
                cv::Mat kernel = (cv::Mat_<float>(3, 3) << 0, -1, 0, 0, 5, 0, 0, -1, 0);
                cv::filter2D(m_s.modified_result[thread_idx], m_s.modified_result[thread_idx], CV_8U, kernel);
                break;
            }
            // SP
            case 3: {
                ImageProc::plateau_equl_hist(m_s.modified_result[thread_idx], m_s.modified_result[thread_idx], 4);
                break;
            }
            // accumulative
            case 4: {
                ImageProc::accumulative_enhance(m_s.modified_result[thread_idx], m_s.modified_result[thread_idx], pcfg.accu_base);
                break;
            }
            // guided image filter
            case 5: {
                ImageProc::guided_image_filter(m_s.modified_result[thread_idx], m_s.modified_result[thread_idx], 60, 0.01, 1);
                break;
            }
            // adaptive
            case 6: {
                ImageProc::adaptive_enhance(m_s.modified_result[thread_idx], m_s.modified_result[thread_idx], pcfg.low_in, pcfg.high_in, pcfg.low_out, pcfg.high_out, pcfg.gamma);
                break;
            }
            // enhance_dehaze
            case 7: {
                m_s.modified_result[thread_idx] = ~m_s.modified_result[thread_idx];
                ImageProc::haze_removal(m_s.modified_result[thread_idx], m_s.modified_result[thread_idx], 7, pcfg.dehaze_pct, 0.1, 60, 0.01);
                m_s.modified_result[thread_idx] = ~m_s.modified_result[thread_idx];
                break;
            }
            // dcp
            case 8: {
                ImageProc::haze_removal(m_s.modified_result[thread_idx], m_s.modified_result[thread_idx], 7, pcfg.dehaze_pct, 0.1, 60, 0.01);
                break;
            }
            // aindane
            case 9: {
                ImageProc::aindane(m_s.modified_result[thread_idx], m_s.modified_result[thread_idx]);
                break;
            }
            // none
            default:
                break;
            }
        }

        // brightness & contrast
        ImageProc::brightness_and_contrast(m_s.modified_result[thread_idx], m_s.modified_result[thread_idx], std::exp(params.contrast / 10.), params.brightness * 12.8);
        // map [0, 20] to [0, +inf) using tan
        ImageProc::brightness_and_contrast(m_s.modified_result[thread_idx], m_s.modified_result[thread_idx], tan((20 - params.gamma_slider) / 40. * M_PI));
    }

    if (params.pseudocolor_enabled && thread_idx == 0 && m_s.modified_result[thread_idx].channels() == 1) {
        cv::Mat mask;
        cv::normalize(m_s.modified_result[thread_idx], m_s.modified_result[thread_idx], 0, 255, cv::NORM_MINMAX, CV_8UC1, mask);
        cv::Mat temp_mask = m_s.modified_result[thread_idx], result_3d;
        cv::applyColorMap(m_s.modified_result[thread_idx], result_3d, pcfg.colormap);
        result_3d.copyTo(m_s.modified_result[thread_idx], temp_mask);
    }
}

void PipelineProcessor::render_and_display(int thread_idx, int display_idx,
    const ProcessingParams& params, const PipelineConfig& pcfg,
    const std::vector<YoloResult>& yolo_results,
    double is_net, int ww, int hh, int _w, int _h, float weight,
    uint hist[256], const QString& info_tcu, const QString& info_time)
{
    Display *disp = m_s.displays[display_idx];

    // Draw YOLO detection boxes on display image
    if (!yolo_results.empty()) {
        draw_yolo_boxes(m_s.modified_result[thread_idx], yolo_results);
    }

    // display the gray-value histogram of the current grayscale image, or the distance histogram of the current 3D image
    if (m_s.aux_panel->get_alt_display_option() == 2) {
        cv::Mat mat_for_hist;
        if (!m_s.is_color[thread_idx] && !params.pseudocolor) mat_for_hist = m_s.modified_result[thread_idx].clone();
        else                       cv::cvtColor(m_s.modified_result[thread_idx], mat_for_hist, cv::COLOR_RGB2GRAY);
        for (int i = 0; i < _h; i++) for (int j = 0; j < _w; j++) hist[(mat_for_hist.data + i * mat_for_hist.cols)[j]]++;

        uchar *img = mat_for_hist.data;
        int step = mat_for_hist.step;
        memset(hist, 0, 256 * sizeof(uint));
        for (int i = 0; i < hh; i++) for (int j = 0; j < ww; j++) hist[(img + i * step)[j]]++;
        uint max = 0;
        for (int i = 1; i < 256; i++) {
            if (hist[i] > max) max = hist[i];
        }
        cv::Mat hist_mat = cv::Mat(225, 256, CV_8UC3, cv::Scalar(56, 64, 72));
        for (int i = 1; i < 256; i++) {
            cv::rectangle(hist_mat, cv::Point(i, 225), cv::Point(i + 1, 225 - hist[i] * 225.0 / max), cv::Scalar(202, 225, 255));
        }
        emit set_hist_pixmap(QPixmap::fromImage(QImage(hist_mat.data, hist_mat.cols, hist_mat.rows, hist_mat.step, QImage::Format_RGB888).scaled(params.hist_display_size, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
    }

    // crop the region to display
    cv::Rect region = cv::Rect(params.display_region.tl() * ww / params.display_width, params.display_region.br() * ww / params.display_width);
    if (region.height + region.y > m_s.modified_result[thread_idx].rows) region.height = m_s.modified_result[thread_idx].rows - region.y;
    if (region.width + region.x > m_s.modified_result[thread_idx].cols) region.width = m_s.modified_result[thread_idx].cols - region.x;
    m_s.modified_result[thread_idx] = m_s.modified_result[thread_idx](region);
    cv::resize(m_s.modified_result[thread_idx], m_s.modified_result[thread_idx], cv::Size(ww, hh));

    // put info (dist, dov, time) as text on image
    if (params.show_info) {
        cv::putText(m_s.modified_result[thread_idx], info_tcu.toLatin1().data(), cv::Point(10, 50 * weight), cv::FONT_HERSHEY_SIMPLEX, weight, cv::Scalar(255), weight * 2);
        cv::putText(m_s.modified_result[thread_idx], info_time.toLatin1().data(), cv::Point(ww - 240 * weight, 50 * weight), cv::FONT_HERSHEY_SIMPLEX, weight, cv::Scalar(255), weight * 2);
#ifdef LVTONG
        int baseline = 0;
        if (pcfg.fishnet_recog) {
            cv::putText(m_s.modified_result[thread_idx], is_net > pcfg.fishnet_thresh ? "FISHNET FOUND" : "FISHNET NOT FOUND", cv::Point(ww - 40 - cv::getTextSize(is_net > pcfg.fishnet_thresh ? "FISHNET FOUND" : "FISHNET NOT FOUND", cv::FONT_HERSHEY_SIMPLEX, weight, weight * 2, &baseline).width, hh - 100 + 50 * weight), cv::FONT_HERSHEY_SIMPLEX, weight, cv::Scalar(255), weight * 2);
        }
#endif
    }

    // resize to display size
    cv::Mat img_display;
    cv::resize(m_s.modified_result[thread_idx], img_display, cv::Size(params.display_width, params.display_height), 0, 0, cv::INTER_AREA);
    bool use_mask = m_s.modified_result[thread_idx].channels() == 1;
    cv::Mat info_mask(cv::Size(params.display_width, params.display_height), CV_8U, cv::Scalar(0)), thresh_result;
    if (use_mask) cv::threshold(img_display, thresh_result, 128, 255, cv::THRESH_BINARY);
    // draw the center cross
    if (!params.image_3d && params.show_center) {
        cv::line(use_mask ? info_mask : img_display, cv::Point(info_mask.cols / 2 - 9, info_mask.rows / 2), cv::Point(info_mask.cols / 2 + 10, info_mask.rows / 2), cv::Scalar(255), 1);
        cv::line(use_mask ? info_mask : img_display, cv::Point(info_mask.cols / 2, info_mask.rows / 2 - 9), cv::Point(info_mask.cols / 2, info_mask.rows / 2 + 10), cv::Scalar(255), 1);
    }
    if (params.select_tool && params.selection_v1 != params.selection_v2) {
        cv::rectangle(use_mask ? info_mask : img_display, params.selection_v1, params.selection_v2, cv::Scalar(255));
        cv::circle(use_mask ? info_mask : img_display, (params.selection_v1 + params.selection_v2) / 2, 1, cv::Scalar(255), -1);
    }
    if (use_mask) img_display = (img_display & (~info_mask)) + ((~thresh_result) & info_mask);

    // image display
    QImage stream = QImage(img_display.data, img_display.cols, img_display.rows, img_display.step, m_s.is_color[thread_idx] || params.pseudocolor ? QImage::Format_RGB888 : QImage::Format_Indexed8);
    disp->emit set_pixmap(QPixmap::fromImage(stream.scaled(disp->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation)));

    if (display_idx == 0 && params.dual_display) {
        cv::Mat ori_img_display;
        Display *ori_display = m_s.secondary_display->get_display_widget();
        cv::resize(m_s.img_mem[thread_idx], ori_img_display, cv::Size(ori_display->width(), ori_display->height()), 0, 0, cv::INTER_AREA);
        stream = QImage(ori_img_display.data, ori_img_display.cols, ori_img_display.rows, ori_img_display.step, m_s.is_color[thread_idx] ? QImage::Format_RGB888 : QImage::Format_Indexed8);
        ori_display->emit set_pixmap(QPixmap::fromImage(stream).scaled(ori_display->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void PipelineProcessor::advance_scan(int thread_idx, bool updated, const PipelineConfig& pcfg,
    int& scan_img_count,
    QString& scan_save_path_a, QString& scan_save_path)
{
    if (thread_idx != 0 || !m_s.scan_ctrl->is_scanning() || !updated) return;

    scan_img_count -= 1;
    if (scan_img_count < 0) scan_img_count = 0;

    int save_img_num = m_s.scan_config->num_single_pos;
    if (scan_img_count > 0 && scan_img_count <= save_img_num) {
        save_scan_img(scan_save_path, QString::number(save_img_num - scan_img_count + 1) + ".bmp", pcfg);
    }
    if (thread_idx == 0 && scan_img_count == save_img_num) {
        emit save_screenshot_signal(scan_save_path + "/screenshot_" + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".png");
    }

    if (!scan_img_count) {
        scan_img_count = *m_s.frame_rate_edit * (m_s.scan_config->ptz_wait_time + save_img_num / *m_s.frame_rate_edit);

        int tcu_idx = m_s.scan_ctrl->get_scan_tcu_idx();
        int ptz_idx = m_s.scan_ctrl->get_scan_ptz_idx();
        const auto tcu_route = m_s.scan_ctrl->get_scan_tcu_route();
        const auto ptz_route = m_s.scan_ctrl->get_scan_ptz_route();

        if (tcu_idx == (int)tcu_route.size() - 1 || tcu_idx == -1) {
            m_s.scan_ctrl->set_scan_tcu_idx(-1);
            tcu_idx = -1;
            ptz_idx++;
            m_s.scan_ctrl->set_scan_ptz_idx(ptz_idx);

            if (ptz_idx < (int)ptz_route.size()) {
                m_s.device_mgr->send_ptz_angle(ptz_route[ptz_idx].first, ptz_route[ptz_idx].second);

                scan_save_path_a = *m_s.save_location + "/" + m_s.scan_ctrl->get_scan_name() + "/" + QString::number(ptz_idx);
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

        tcu_idx = m_s.scan_ctrl->get_scan_tcu_idx() + 1;
        m_s.scan_ctrl->set_scan_tcu_idx(tcu_idx);
        m_s.tcu_ctrl->set_delay_dist(tcu_route[tcu_idx] * m_s.tcu_ctrl->get_dist_ns());
        scan_save_path = scan_save_path_a + "/" + QString::number(tcu_route[tcu_idx], 'f', 2) + " ns";
        QDir().mkdir(scan_save_path);
        QDir().mkdir(scan_save_path + "/ori_bmp");
        QDir().mkdir(scan_save_path + "/res_bmp");
        if (m_s.grab_image[1]) QDir().mkdir(scan_save_path + "/alt_bmp");

        emit update_delay_in_thread();
    }
}

void PipelineProcessor::record_frame(int thread_idx, int display_idx, bool updated,
    const ProcessingParams& params, const PipelineConfig& pcfg,
    double is_net,
    const QString& info_tcu, const QString& info_time,
    int ww, int hh, float weight)
{
    if (updated && m_s.save_original[thread_idx]) {
        save_to_file(false, thread_idx, params, pcfg);
        if (*m_s.device_type == -1 || !pcfg.consecutive_capture || display_idx) m_s.save_original[thread_idx] = 0;
    }
    if (updated && m_s.save_modified[thread_idx]) {
        save_to_file(true, thread_idx, params, pcfg);
        if (*m_s.device_type == -1 || !pcfg.consecutive_capture || display_idx) m_s.save_modified[thread_idx] = 0;
    }
    if (updated && m_s.record_original[thread_idx]) {
        cv::Mat temp = m_s.img_mem[thread_idx].clone();
        if (pcfg.save_info) {
            cv::putText(temp, info_tcu.toLatin1().data(), cv::Point(10, 50 * weight), cv::FONT_HERSHEY_SIMPLEX, weight, cv::Scalar(255), weight * 2);
            cv::putText(temp, info_time.toLatin1().data(), cv::Point(ww - 240 * weight, 50 * weight), cv::FONT_HERSHEY_SIMPLEX, weight, cv::Scalar(255), weight * 2);
        }
        if (m_s.is_color[thread_idx]) cv::cvtColor(temp, temp, cv::COLOR_RGB2BGR);
        // inc by 1 because main display uses two videowriter
        if (display_idx) m_s.vid_out[thread_idx + 1].write(temp);
        else             m_s.vid_out[0].write(temp);
    }
    if (updated && m_s.record_modified[thread_idx]) {
        cv::Mat temp = m_s.modified_result[thread_idx].clone();
        if (!params.image_3d && !params.show_info) {
            if (pcfg.save_info) {
                cv::putText(m_s.modified_result[thread_idx], info_tcu.toLatin1().data(), cv::Point(10, 50 * weight), cv::FONT_HERSHEY_SIMPLEX, weight, cv::Scalar(255), weight * 2);
                cv::putText(m_s.modified_result[thread_idx], info_time.toLatin1().data(), cv::Point(ww - 240 * weight, 50 * weight), cv::FONT_HERSHEY_SIMPLEX, weight, cv::Scalar(255), weight * 2);
#ifdef LVTONG
                int baseline = 0;
                if (pcfg.fishnet_recog) {
                    cv::putText(m_s.modified_result[thread_idx], is_net > pcfg.fishnet_thresh ? "FISHNET FOUND" : "FISHNET NOT FOUND", cv::Point(ww - 40 - cv::getTextSize(is_net > pcfg.fishnet_thresh ? "FISHNET FOUND" : "FISHNET NOT FOUND", cv::FONT_HERSHEY_SIMPLEX, weight, weight * 2, &baseline).width, hh - 100 + 50 * weight), cv::FONT_HERSHEY_SIMPLEX, weight, cv::Scalar(255), weight * 2);
                }
#endif
            }
        }
        if (m_s.is_color[thread_idx] || params.pseudocolor) cv::cvtColor(temp, temp, cv::COLOR_RGB2BGR);
        // inc by 1 because main display uses two videowriter
        if (display_idx) m_s.vid_out[thread_idx + 1].write(temp);
        else             m_s.vid_out[1].write(temp);
    }
}

void PipelineProcessor::save_to_file(bool save_result, int idx, const ProcessingParams& params, const PipelineConfig& pcfg) {
    cv::Mat *temp = save_result ? &m_s.modified_result[idx] : &m_s.img_mem[idx];

    // TODO implement 16bit result image processing/writing
    cv::Mat result_image;
    if (pcfg.save_in_grayscale) cv::cvtColor(*temp, result_image, cv::COLOR_RGB2GRAY);
    else                         result_image = temp->clone();

    if (save_result) {
        QString dt = QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz");
        if (params.split) {
            if (pcfg.integrate_info) {
                QString tcu_note = QString::fromStdString(m_s.device_mgr->tcu()->to_json().dump());
                if (!m_s.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString, QString)>(&ImageIO::save_image_bmp), result_image(cv::Rect(             0,              0, temp->cols / 2, temp->rows / 2)).clone(), *m_s.save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + dt + "_0" + ".bmp", tcu_note))) emit task_queue_full();
                if (!m_s.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString, QString)>(&ImageIO::save_image_bmp), result_image(cv::Rect(temp->cols / 2,              0, temp->cols / 2, temp->rows / 2)).clone(), *m_s.save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + dt + "_1" + ".bmp", tcu_note))) emit task_queue_full();
                if (!m_s.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString, QString)>(&ImageIO::save_image_bmp), result_image(cv::Rect(             0, temp->rows / 2, temp->cols / 2, temp->rows / 2)).clone(), *m_s.save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + dt + "_2" + ".bmp", tcu_note))) emit task_queue_full();
                if (!m_s.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString, QString)>(&ImageIO::save_image_bmp), result_image(cv::Rect(temp->cols / 2, temp->rows / 2, temp->cols / 2, temp->rows / 2)).clone(), *m_s.save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + dt + "_3" + ".bmp", tcu_note))) emit task_queue_full();
            } else {
                if (!m_s.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString)>(&ImageIO::save_image_bmp), result_image(cv::Rect(             0,              0, temp->cols / 2, temp->rows / 2)).clone(), *m_s.save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + dt + "_0" + ".bmp"))) emit task_queue_full();
                if (!m_s.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString)>(&ImageIO::save_image_bmp), result_image(cv::Rect(temp->cols / 2,              0, temp->cols / 2, temp->rows / 2)).clone(), *m_s.save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + dt + "_1" + ".bmp"))) emit task_queue_full();
                if (!m_s.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString)>(&ImageIO::save_image_bmp), result_image(cv::Rect(             0, temp->rows / 2, temp->cols / 2, temp->rows / 2)).clone(), *m_s.save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + dt + "_2" + ".bmp"))) emit task_queue_full();
                if (!m_s.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString)>(&ImageIO::save_image_bmp), result_image(cv::Rect(temp->cols / 2, temp->rows / 2, temp->cols / 2, temp->rows / 2)).clone(), *m_s.save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + dt + "_3" + ".bmp"))) emit task_queue_full();
            }
        }
        switch (pcfg.img_format){
        case 0:
            if (pcfg.integrate_info) {
                QString tcu_note = QString::fromStdString(m_s.device_mgr->tcu()->to_json().dump());
                if (!m_s.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString, QString)>(&ImageIO::save_image_bmp), result_image, *m_s.save_location + "/res_bmp/" + dt + ".bmp", tcu_note))) emit task_queue_full();
            } else {
                if (!m_s.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString)>(&ImageIO::save_image_bmp), result_image, *m_s.save_location + "/res_bmp/" + dt + ".bmp"))) emit task_queue_full();
            }
            break;
        case 1: if (!m_s.tp->append_task(std::bind(ImageIO::save_image_jpg, result_image, *m_s.save_location + "/res_bmp/" + dt + ".jpg"))) emit task_queue_full(); break;
        default: break;
        }
    } else {
        switch (m_s.pixel_depth[0]) {
        case  8:
            switch (pcfg.img_format){
            case 0:
                if (pcfg.integrate_info) {
                    QString tcu_note = QString::fromStdString(m_s.device_mgr->tcu()->to_json().dump());
                    if (!m_s.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString, QString)>(&ImageIO::save_image_bmp), result_image, *m_s.save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".bmp", tcu_note))) emit task_queue_full();
                } else {
                    if (!m_s.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString)>(&ImageIO::save_image_bmp), result_image, *m_s.save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".bmp"))) emit task_queue_full();
                }
                break;
            case 1: if (!m_s.tp->append_task(std::bind(ImageIO::save_image_jpg, result_image, *m_s.save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".jpg"))) emit task_queue_full(); break;
            default: break;
            }
            break;
        case 10:
        case 12:
        case 16:
            switch (pcfg.img_format){
            case 0: if (!m_s.tp->append_task(std::bind(ImageIO::save_image_tif, result_image * (1 << (16 - m_s.pixel_depth[0])), *m_s.save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".tif"))) emit task_queue_full(); break;
            case 1: if (!m_s.tp->append_task(std::bind(ImageIO::save_image_jpg, result_image * (1 << (16 - m_s.pixel_depth[0])), *m_s.save_location + (save_result ? "/res_bmp/" : "/ori_bmp/") + QDateTime::currentDateTime().toString("MMdd_hhmmss_zzz") + ".jpg"))) emit task_queue_full(); break;
            default: break;
            }
            break;
        default: break;
        }
    }
}

void PipelineProcessor::save_scan_img(QString path, QString name, const PipelineConfig& pcfg) {
    if (m_s.scan_config->capture_scan_ori) {
        if (pcfg.integrate_info) {
            QString tcu_note = QString::fromStdString(m_s.device_mgr->tcu()->to_json().dump());
            m_s.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString, QString)>(&ImageIO::save_image_bmp), m_s.img_mem[0].clone(), path + "/ori_bmp/" + name, tcu_note));
        } else {
            m_s.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString)>(&ImageIO::save_image_bmp), m_s.img_mem[0].clone(), path + "/ori_bmp/" + name));
        }
    }
    if (m_s.scan_config->capture_scan_res) {
        if (pcfg.integrate_info) {
            QString tcu_note = QString::fromStdString(m_s.device_mgr->tcu()->to_json().dump());
            m_s.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString, QString)>(&ImageIO::save_image_bmp), m_s.modified_result[0].clone(), path + "/res_bmp/" + name, tcu_note));
        } else {
            m_s.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString)>(&ImageIO::save_image_bmp), m_s.modified_result[0].clone(), path + "/res_bmp/" + name));
        }
    }

    if (m_s.grab_image[1]) {
        if (pcfg.integrate_info) {
            QString tcu_note = QString::fromStdString(m_s.device_mgr->tcu()->to_json().dump());
            m_s.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString, QString)>(&ImageIO::save_image_bmp), m_s.img_mem[1].clone(), path + "/alt_bmp/" + name, tcu_note));
        } else {
            m_s.tp->append_task(std::bind(static_cast<void(*)(cv::Mat, QString)>(&ImageIO::save_image_bmp), m_s.img_mem[1].clone(), path + "/alt_bmp/" + name));
        }
    }
}

void PipelineProcessor::draw_yolo_boxes(cv::Mat& image, const std::vector<YoloResult>& results)
{
    if (image.empty() || results.empty()) {
        return;
    }

    // Ensure image is 3-channel for colored boxes
    cv::Mat draw_image;
    if (image.channels() == 1) {
        cv::cvtColor(image, draw_image, cv::COLOR_GRAY2BGR);
    } else {
        draw_image = image;
    }

    for (const auto& det : results) {
        // Use green color for detection boxes
        cv::Scalar color(0, 255, 0);

        // Draw bounding box
        cv::rectangle(draw_image, det.box, color, 2);

        // Prepare label text
        std::string label;
        if (m_s.m_yolo_detector[0] && det.classId < static_cast<int>(m_s.m_yolo_detector[0]->getClassNames().size())) {
            label = m_s.m_yolo_detector[0]->getClassNames()[det.classId];
        } else {
            label = "class_" + std::to_string(det.classId);
        }
        label += ": " + std::to_string(static_cast<int>(det.confidence * 100)) + "%";

        // Calculate text size for background rectangle
        int baseline = 0;
        cv::Size text_size = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseline);

        // Draw background rectangle for text
        cv::rectangle(draw_image,
                     cv::Point(det.box.x, det.box.y - text_size.height - 5),
                     cv::Point(det.box.x + text_size.width, det.box.y),
                     color, -1);

        // Draw label text
        cv::putText(draw_image, label,
                   cv::Point(det.box.x, det.box.y - 5),
                   cv::FONT_HERSHEY_SIMPLEX, 0.5,
                   cv::Scalar(0, 0, 0), 1);
    }

    // Copy back if we converted
    if (image.channels() == 1) {
        cv::cvtColor(draw_image, image, cv::COLOR_BGR2GRAY);
    }
}
