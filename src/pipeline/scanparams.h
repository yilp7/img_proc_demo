#ifndef SCANPARAMS_H
#define SCANPARAMS_H

#include <vector>
#include <utility>

// Plain-data scan configuration. Replaces the data members that used to
// live on the legacy ScanConfig QDialog (S22). Owned by ScanController;
// borrowed by the pipeline worker thread via ProcessingParams::SharedState.
// Same defaults the old ScanConfig() constructor used.
struct ScanParams {
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
};

// Derived routes. Free functions so they're testable without constructing
// a controller. Logic copied verbatim from ScanConfig::get_*_route().
std::vector<std::pair<float, float>> get_ptz_route(const ScanParams &p);
std::vector<float>                   get_tcu_route(const ScanParams &p);

#endif // SCANPARAMS_H
