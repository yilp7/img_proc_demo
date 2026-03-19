#ifndef PIPELINEPROCESSOR_H
#define PIPELINEPROCESSOR_H

#include <QObject>
#include <QMutex>
#include <QMutexLocker>
#include <QString>
#include <QPixmap>
#include <deque>
#include <queue>
#include <vector>
#include <atomic>
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#ifdef LVTONG
#include <opencv2/dnn.hpp>
#endif

#include "pipeline/processingparams.h"
#include "pipeline/pipelineconfig.h"
#include "yolo/yolo_detector.h"

class TCUController;
class ScanController;
class DeviceManager;
class AuxPanelManager;
class ScanConfig;
class Display;
class FloatingWindow;
class ThreadPool;
#include "util/timedqueue.h"

// Persistent frame-averaging state for pipeline methods
struct FrameAverageState {
    cv::Mat seq[8], seq_sum, frame_a_sum, frame_b_sum;
    cv::Mat prev_img, prev_3d;
    int seq_idx = 0;
    bool frame_a = true;
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

// Persistent ECC temporal denoising state
struct ECCState {
    std::deque<cv::Mat> fusion_buf, fusion_warps, fusion_warps_inv;
    cv::Mat fusion_last_warp;
    int prev_ecc_warp_mode = -1;
    void clear() {
        fusion_buf.clear(); fusion_warps.clear(); fusion_warps_inv.clear();
        fusion_last_warp.release();
    }
};

class PipelineProcessor : public QObject
{
    Q_OBJECT

public:
    // Shared resource pointers — all owned by UserPanel, PipelineProcessor borrows them.
    struct SharedState {
        // Per-thread arrays (size 3)
        cv::Mat*            img_mem;
        cv::Mat*            modified_result;
        QMutex*             image_mutex;
        cv::VideoWriter*    vid_out;         // size 4
        int*                w;
        int*                h;
        int*                pixel_depth;
        bool*               is_color;
        cv::Mat*            user_mask;

        // Atomic flags (size 3)
        bool*               grab_image;      // non-atomic, checked per-loop
        std::atomic<bool>*  save_original;
        std::atomic<bool>*  save_modified;
        std::atomic<bool>*  record_original;
        std::atomic<bool>*  record_modified;
        std::atomic<bool>*  grab_thread_state;

        // Queues
        std::queue<cv::Mat>* q_img;          // size 3
        std::queue<int>*    q_frame_info;
        TimedQueue*         q_fps_calc;
        QMutex*             frame_info_mutex;

        // Snapshot state
        QMutex*             m_params_mutex;
        ProcessingParams*   m_processing_params;
        PipelineConfig*     m_pipeline_config;

        // ROI
        QMutex*             m_roi_mutex;
        std::vector<cv::Rect>* list_roi;

        // YOLO
        YoloDetector**      m_yolo_detector; // size 3
        QMutex*             m_yolo_init_mutex;
        int*                m_yolo_last_model; // size 3

        // Controllers
        TCUController*      tcu_ctrl;
        ScanController*     scan_ctrl;
        DeviceManager*      device_mgr;
        AuxPanelManager*    aux_panel;

        // Display
        Display**           displays;        // size 3
        FloatingWindow*     secondary_display;

        // Other
        ScanConfig*         scan_config;
        QString*            save_location;
        ThreadPool*         tp;
        int*                device_type;
        float*              frame_rate_edit;
    };

    explicit PipelineProcessor(SharedState state, QObject* parent = nullptr);

    // Main pipeline entry point — called by GrabThread::run()
    int run(int* display_idx);

signals:
    void packet_lost_updated(int packets_lost);
    void update_mcp_in_thread(int new_mcp);
    void set_hist_pixmap(QPixmap pm);
    void finish_scan_signal();
    void save_screenshot_signal(QString path);
    void update_delay_in_thread();
    void update_dist_mat(cv::Mat, double, double);
    void task_queue_full();
#ifdef LVTONG
    void set_model_list_enabled(bool enabled);
    void update_fishnet_result(int result);
#endif

private:
    SharedState m_s;

    // Pipeline stage methods
    bool acquire_frame(int thread_idx, cv::Mat& prev_img, bool& updated, int& packets_lost);
    void preprocess_frame(int thread_idx, const ProcessingParams& params,
        const PipelineConfig& pcfg,
        FrameAverageState& avg_state, int& _w, int& _h, uint hist[256]);
    std::vector<YoloResult> detect_yolo(int thread_idx, bool updated,
        const PipelineConfig& pcfg, YoloDetector*& yolo);
#ifdef LVTONG
    double detect_fishnet(int thread_idx, const PipelineConfig& pcfg, cv::dnn::Net& net);
#endif
    void frame_average_and_3d(int thread_idx, bool updated, const ProcessingParams& params,
        const PipelineConfig& pcfg,
        FrameAverageState& avg_state, ECCState& ecc_state, int _w, int _h, int _pixel_depth,
        int& ww, int& hh);
    void enhance_frame(int thread_idx, const ProcessingParams& params,
        const PipelineConfig& pcfg);
    void render_and_display(int thread_idx, int display_idx,
        const ProcessingParams& params, const PipelineConfig& pcfg,
        const std::vector<YoloResult>& yolo_results,
        double is_net, int ww, int hh, int _w, int _h, float weight,
        uint hist[256], const QString& info_tcu, const QString& info_time);
    void advance_scan(int thread_idx, bool updated, const PipelineConfig& pcfg,
        int& scan_img_count,
        QString& scan_save_path_a, QString& scan_save_path);
    void record_frame(int thread_idx, int display_idx, bool updated,
        const ProcessingParams& params, const PipelineConfig& pcfg,
        double is_net,
        const QString& info_tcu, const QString& info_time,
        int ww, int hh, float weight);
    void save_to_file(bool save_result, int idx, const ProcessingParams& params, const PipelineConfig& pcfg);
    void save_scan_img(QString path, QString name, const PipelineConfig& pcfg);
    void draw_yolo_boxes(cv::Mat& image, const std::vector<YoloResult>& results);
};

#endif // PIPELINEPROCESSOR_H
