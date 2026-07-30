#ifndef PIPELINEPROCESSOR_H
#define PIPELINEPROCESSOR_H

#include <QObject>
#include <QMutex>
#include <QMutexLocker>
#include <QString>
#include <QPixmap>
#include <QRect>
#include <QVector>
#include <vector>
#include <opencv2/core.hpp>
#ifdef LVTONG
#include <opencv2/dnn.hpp>
#endif

#include "pipeline/processingparams.h"
#include "pipeline/pipelineconfig.h"
#include "pipeline/routing.h"
#include "yolo/yolo_detector.h"

class TCUController;
class ScanController;
class DeviceManager;
class AuxPanelManager;
struct ScanParams;
class FloatingWindow;
class ThreadPool;

// Singletons + globally shared state. Things that don't belong to any one
// stream or display.
struct PipelineGlobals {
    QMutex*           params_mutex      = nullptr;
    ProcessingParams* processing_params = nullptr;  // UI snapshot

    QMutex*                  roi_mutex = nullptr;
    std::vector<cv::Rect>*   list_roi  = nullptr;

    QMutex* yolo_init_mutex = nullptr;

    TCUController*   tcu_ctrl   = nullptr;
    ScanController*  scan_ctrl  = nullptr;
    DeviceManager*   device_mgr = nullptr;
    AuxPanelManager* aux_panel  = nullptr;

    FloatingWindow* secondary_display = nullptr;
    ScanParams*     scan_params       = nullptr;
    QString*        save_location     = nullptr;
    ThreadPool*     tp                = nullptr;
};

struct PanoramaCanvas {
    cv::Mat accum;
    cv::Mat weights;
    std::vector<double> angles;
    double left_deg = 0.0;
    double extent_deg = 0.0;
    double px_per_deg = 0.0;
    float fov_h = 0.0f;
    int target_width = 0;
    int canvas_width = 0;
    int canvas_height = 0;
    int route_size = 0;

    void reset()
    {
        accum.release();
        weights.release();
        angles.clear();
        left_deg = 0.0;
        extent_deg = 0.0;
        px_per_deg = 0.0;
        fov_h = 0.0f;
        target_width = 0;
        canvas_width = 0;
        canvas_height = 0;
        route_size = 0;
    }
};

class PipelineProcessor : public QObject
{
    Q_OBJECT

public:
    explicit PipelineProcessor(PipelineGlobals globals, QObject* parent = nullptr);

    // Main pipeline entry. Called by ConsumerThread::run() with this display's
    // slot and the router (needed for route snapshots). Returns when the slot's
    // consumer_run flag becomes false.
    int run(DisplayRouter& router, DisplaySlot& display);

signals:
    void packet_lost_updated(int packets_lost);
    void update_mcp_in_thread(int new_mcp);
    void set_hist_pixmap(QPixmap pm);
    void panorama_preview_updated(QPixmap pm);
    void main_detection_boxes_updated(QVector<QRect> boxes);
    void finish_scan_signal();
    void save_screenshot_signal(QString path);
    void update_delay_in_thread();
    void update_gate_width_in_thread();
    void update_dist_mat(cv::Mat, double, double);
    void task_queue_full();
    // state: 0=none (no model), 1=loaded (running), 2=failed (load error, no retry).
    void yolo_status_changed(int display_idx, int state);
    // Edge-triggered: emitted when the routed stream's "is the source dead"
    // state flips. dead=true when the queue went closed-and-empty; dead=false
    // when frames flow again (route changed to a live stream, or producer
    // resumed). Steady-state truth lives in DisplaySlot::source_dead.
    void source_state_changed(int display_idx, bool dead);
    // Auto-finalize handoff: emitted when the consumer thread releases a
    // recorder (route change / source death). UserPanel's slot moves the
    // temp file to save_location, flips DisplaySlot::record_raw/proc to
    // false, and resets the button text. kind: 0 = raw, 1 = proc.
    void recorder_finalized(int display_idx, int kind, QString temp_path);
#ifdef LVTONG
    void set_model_list_enabled(bool enabled);
    void update_fishnet_result(int result);
#endif

private:
    PipelineGlobals m_g;

    // Pipeline stage methods. Each reads from the passed StreamSlot (producer
    // metadata + queues) and DisplaySlot (per-display pcfg + recorders +
    // runtime caches) directly; no SharedState indirection.
    bool acquire_frame(StreamSlot& stream, DisplaySlot& display,
        const PipelineConfig& pcfg, cv::Mat& prev_img, bool& updated, int& packets_lost);
    void preprocess_frame(StreamSlot& stream, DisplaySlot& display,
        const ProcessingParams& params, const PipelineConfig& pcfg,
        int& _w, int& _h);
    std::vector<YoloResult> detect_yolo(StreamSlot& stream, DisplaySlot& display,
        bool updated, const PipelineConfig& pcfg);
#ifdef LVTONG
    double detect_fishnet(StreamSlot& stream, DisplaySlot& display,
        const PipelineConfig& pcfg, cv::dnn::Net& net);
#endif
    void frame_average_and_3d(StreamSlot& stream, DisplaySlot& display,
        bool updated, const ProcessingParams& params, const PipelineConfig& pcfg,
        int _w, int _h, int _pixel_depth, int& ww, int& hh);
    void enhance_frame(StreamSlot& stream, DisplaySlot& display,
        const ProcessingParams& params, const PipelineConfig& pcfg);
    void render_and_display(StreamSlot& stream, DisplaySlot& display,
        const ProcessingParams& params, const PipelineConfig& pcfg,
        const std::vector<YoloResult>& yolo_results,
        bool updated, double is_net, int ww, int hh, int _w, int _h, float weight,
        const QString& info_tcu, const QString& info_time);
    // Main-display-only. Walks main + alt1 in lockstep; needs router for alt1.
    void advance_scan_main(DisplayRouter& router,
        StreamSlot& main_stream, DisplaySlot& main_display,
        bool updated, const PipelineConfig& pcfg,
        int& scan_img_count,
        QString& scan_save_path_a, QString& scan_save_path,
        PanoramaCanvas& panorama_canvas,
        qint64& panorama_position_started_ms);
    void advance_panorama_scan_main(DisplayRouter& router,
        StreamSlot& main_stream, DisplaySlot& main_display,
        const PipelineConfig& pcfg,
        int& scan_img_count,
        QString& scan_save_path,
        PanoramaCanvas& panorama_canvas,
        qint64& panorama_position_started_ms);
    void append_panorama_preview(DisplaySlot& main_display,
        int ptz_idx, const std::vector<std::pair<float, float>>& route,
        PanoramaCanvas& panorama_canvas);
    void save_panorama_preview(const PanoramaCanvas& panorama_canvas,
        const PipelineConfig& pcfg);
    void record_frame(StreamSlot& stream, DisplaySlot& display,
        bool updated, const ProcessingParams& params, const PipelineConfig& pcfg,
        double is_net,
        const QString& info_tcu, const QString& info_time,
        int ww, int hh, float weight);
    void save_to_file(StreamSlot& stream, DisplaySlot& display,
        bool save_result, const ProcessingParams& params, const PipelineConfig& pcfg);
    void save_scan_img(DisplayRouter& router, StreamSlot& main_stream,
        QString path, QString name, const PipelineConfig& pcfg);
    void draw_yolo_boxes(DisplaySlot& display,
        cv::Mat& image, const std::vector<YoloResult>& results);
};

#endif // PIPELINEPROCESSOR_H
