#ifndef SCANPARAMS_H
#define SCANPARAMS_H

#include <vector>
#include <utility>

struct PanoramaZoomFovPoint {
    float zoom;
    float focal_length_mm;
    float fov_h;
    float fov_v;

    PanoramaZoomFovPoint(float z = 1.0f, float focal = 10.0f, float h = 38.4f, float v = 22.5f)
        : zoom(z), focal_length_mm(focal), fov_h(h), fov_v(v) {}
};

// Plain-data scan configuration. Replaces the data members that used to
// live on the legacy ScanConfig QDialog (S22). Owned by ScanController;
// borrowed by the pipeline worker thread via ProcessingParams::SharedState.
// Same defaults the old ScanConfig() constructor used.
struct ScanParams {
    enum PtzScanMode {
        PTZ_SCAN_GRID = 0,
        PTZ_SCAN_PANORAMA = 1,
    };

    enum PanoramaFusionMode {
        PANORAMA_FUSION_NONE = 0,
        PANORAMA_FUSION_GAIN = 1,
        PANORAMA_FUSION_MATCHING = 2,
    };

    // TCU side
    float dist_ns          = 3e8f / 2e9f;
    int   hz_unit          = 0;     // 0: kHz, 1: Hz
    int   base_unit        = 0;     // 0: ns, 1: μs, 2: m
    int   starting_delay   = 0;
    int   ending_delay     = 0;
    int   starting_gw      = 0;
    int   ending_gw        = 0;
    float step_size_delay  = 1;
    float step_size_gw     = 1;
    int   frame_count      = 1;
    float rep_freq         = 10;

    // Grid scan dimensions. Disabled dimensions retain the device value that
    // is active when the scan starts.
    bool  scan_delay_enabled = true;
    bool  scan_gate_width_enabled = true;
    bool  scan_ptz_enabled = true;

    // Capture / record
    bool  capture_scan_ori = true;
    bool  capture_scan_res = false;
    bool  record_scan_ori  = false;
    bool  record_scan_res  = true;

    // PTZ
    float starting_h       = 0;
    float ending_h         = 0;
    float starting_v       = 0;
    float ending_v         = 0;
    float step_h           = 0;
    float step_v           = 0;
    int   count_h          = 1;
    int   count_v          = 1;
    unsigned int ptz_direction = 0;
    int   ptz_wait_time    = 4;
    int   num_single_pos   = 8;

    // PTZ panorama mode. Zoom/focal/FOV calibration comes from default.json;
    // FOV is estimated from zoom using the lens formula and then used as the
    // default horizontal step.
    int   ptz_scan_mode    = PTZ_SCAN_PANORAMA;
    float panorama_start_h = 0;
    bool  panorama_use_end_h = false;
    float panorama_end_h   = 0;
    float panorama_fixed_v = 0;
    float panorama_step_h  = 30;
    int   panorama_count   = 12;
    unsigned int panorama_direction = 0;
    int   panorama_wait_time = 2;
    float panorama_zoom_level = 1.0f;
    float panorama_fov_h = 38.4f;
    float panorama_fov_v = 22.5f;
    int   panorama_fusion_mode = PANORAMA_FUSION_GAIN;
    std::vector<PanoramaZoomFovPoint> panorama_zoom_fov = {
        PanoramaZoomFovPoint(1.0f, 10.0f, 38.4f, 22.5f),
        PanoramaZoomFovPoint(86.0f, 860.0f, 0.49f, 0.28f)
    };
};

// Derived routes. Free functions so they're testable without constructing
// a controller. Logic copied verbatim from ScanConfig::get_*_route().
std::vector<std::pair<float, float>> get_ptz_route(const ScanParams &p);
std::vector<std::pair<float, float>> get_panorama_route(const ScanParams &p);
std::vector<float>                   get_tcu_route(const ScanParams &p);
std::vector<float>                   get_gate_width_route(const ScanParams &p);
std::pair<float, float>              get_panorama_fov(float zoom, const std::vector<PanoramaZoomFovPoint> &points);
void                                 update_panorama_fov_from_zoom(ScanParams &p);
float                                get_panorama_sweep_degrees(const ScanParams &p);
int                                  get_panorama_stop_count(const ScanParams &p);
const char*                          get_panorama_fusion_name(int mode);

#endif // SCANPARAMS_H
