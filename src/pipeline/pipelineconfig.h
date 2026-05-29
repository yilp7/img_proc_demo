#ifndef PIPELINECONFIG_H
#define PIPELINECONFIG_H

#include <QString>

// Thread-safe snapshot of Config fields read by pipeline methods.
// GUI thread writes via update_pipeline_config() under m_params_mutex;
// worker thread copies at loop start alongside ProcessingParams.
struct PipelineConfig {
    // from DeviceSettings
    int flip = 0;

    // from ImageProcSettings
    float accu_base = 1.0f;
    float gamma = 1.2f;
    float low_in = 0.0f;
    float high_in = 0.05f;
    float low_out = 0.0f;
    float high_out = 1.0f;
    float dehaze_pct = 0.95f;
    int colormap = 2;
    double lower_3d_thresh = 0.0;
    double upper_3d_thresh = 0.981;
    bool truncate_3d = false;
    bool custom_3d_param = false;
    float custom_3d_delay = 0.0f;
    float custom_3d_gate_width = 0.0f;
    int fishnet_model_idx = 0;
    bool fishnet_recog = false;
    float fishnet_thresh = 0.99f;
    int ecc_window_mode = 0;
    int ecc_warp_mode = 2;
    int ecc_fusion_method = 2;
    int ecc_backward = 20;
    int ecc_forward = 0;
    int ecc_levels = 1;
    int ecc_max_iter = 8;
    double ecc_eps = 0.001;
    bool ecc_half_res_reg = true;
    bool ecc_half_res_fuse = false;

    // from SaveSettings
    bool save_info = true;
    bool custom_topleft_info = false;
    bool save_in_grayscale = false;
    bool consecutive_capture = true;
    bool integrate_info = true;
    int img_format = 0;

    // Per-display GENERAL-tab toggles. Session-only — never persisted to
    // config file (see [[feedback-per-display-toggles-not-persisted]]).
    // image_3d / pseudocolor are read in display-routing paths (layout +
    // pixel-format selection), enable_3d / pseudocolor_enabled in the
    // pipeline's 3D + colormap stages. Same checkbox drives both halves of
    // each pair — duplication preserved from the old ProcessingParams shape.
    bool frame_avg_enabled = false;
    int  frame_avg_mode    = 0;
    bool enable_3d         = false;
    bool image_3d          = false;
    bool enhance_enabled   = false;
    int  enhance_option    = 0;
    bool pseudocolor_enabled = false;
    bool pseudocolor       = false;

    // from YoloSettings
    QString config_path;
    int model_idx = 0;  // YOLO model index for this display (per-display since T3 C1a)
    QString visible_model_path;
    QString visible_classes_file;
    QString thermal_model_path;
    QString thermal_classes_file;
    QString gated_model_path;
    QString gated_classes_file;
};

#endif // PIPELINECONFIG_H
