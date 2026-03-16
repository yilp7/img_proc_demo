# Review 1 — Urgent Fix Plan

> Fixes for 5 urgent thread safety issues identified in review-1-retrospective.md.
> Strategy: extend the ProcessingParams snapshot pattern to Config/controller state.

---

## Overview

| # | Fix | Severity | Effort | Files |
|---|-----|----------|--------|-------|
| 1 | Fishnet signal connection type | MEDIUM | 1 line | userpanel.cpp |
| 2 | DeviceManager angle_h/angle_v race | MEDIUM | ~5 lines | devicemanager.h |
| 3 | TCUController cross-thread writes | CRITICAL | ~10 lines | tcucontroller.h |
| 4 | ScanController state race | CRITICAL | ~30 lines | scancontroller.h |
| 5 | Config get_data() thread safety | CRITICAL | ~150 lines | NEW pipelineconfig.h, processingparams.h, userpanel.h/cpp |

---

## Fix 1: `update_fishnet_result` signal connection type

**Problem:** Line 678 in userpanel.cpp:
```cpp
connect(this, SIGNAL(update_fishnet_result(int)), SLOT(display_fishnet_result(int)));
```
Uses default `AutoConnection`. Signal is emitted from worker thread (detect_fishnet, lines 1161/1176/1190/1196). `AutoConnection` resolves to `DirectConnection` when sender and receiver are the same QObject and the emit happens from a non-owner thread. This causes `display_fishnet_result()` — which calls `ui->FISHNET_RESULT->setText()` and `setStyleSheet()` — to execute in the worker thread context.

**Fix:** Add explicit `Qt::QueuedConnection`:
```cpp
connect(this, SIGNAL(update_fishnet_result(int)),
        SLOT(display_fishnet_result(int)), Qt::QueuedConnection);
```

---

## Fix 2: DeviceManager `angle_h`/`angle_v` race

**Problem:** `angle_h` and `angle_v` (float) in DeviceManager are:
- Written by GUI thread: `keyPressEvent()` (lines 3527, 3533), preset application (lines 2906-2907)
- Written by PTZ port thread: `update_ptz_angle()` slot
- Read by GUI thread: `get_angle_h()`/`get_angle_v()`
Non-atomic float writes can tear on some architectures.

**Fix:** Change to `std::atomic<float>`:
```cpp
// devicemanager.h
std::atomic<float> angle_h{0.0f};
std::atomic<float> angle_v{0.0f};
```
Getters/setters use simple load/store — no API change needed.

---

## Fix 3: TCUController cross-thread writes

**Problem:** Two fields written from worker thread without synchronization:
- `frame_a_3d`: read-modify-write in `frame_average_and_3d()` line 1117: `set_frame_a_3d(!get_frame_a_3d())`
- `delay_dist`: written in `advance_scan()` line 1410: `set_delay_dist(tcu_route[tcu_idx] * get_dist_ns())`
Both also written from GUI thread via `keyPressEvent()` (15+ locations).

**Fix:**
1. Change `frame_a_3d` to `std::atomic<bool>`.
2. Change `delay_dist` to `std::atomic<float>`.
3. Add `toggle_frame_a_3d()` method using atomic exchange to fix the TOCTOU race:
```cpp
void toggle_frame_a_3d() {
    bool expected = frame_a_3d.load();
    while (!frame_a_3d.compare_exchange_weak(expected, !expected));
}
```
4. Replace line 1117 call with `m_tcu_ctrl->toggle_frame_a_3d()`.

---

## Fix 4: ScanController state race

**Problem:** `advance_scan()` (worker thread) writes `scan_ptz_idx`, `scan_tcu_idx` via setters. GUI thread reads them during `on_SCAN_BUTTON_clicked()` scan setup. `scan_name` (QString) returned by const ref — unsafe for cross-thread access.

**Fix:** Add `QMutex` to ScanController:
1. Add `mutable QMutex m_scan_mutex;` member.
2. Protect setters with `QMutexLocker`:
   - `set_scan_ptz_idx`, `set_scan_tcu_idx`, `set_scan_idx`, `set_scanning`
3. Protect corresponding getters.
4. Change `get_scan_name()` to return `QString` by value (not const ref).
5. Change `get_scan_ptz_route()` / `get_scan_tcu_route()` to return by value.

---

## Fix 5: Config `get_data()` thread safety

**Problem:** `config->get_data()` returns a mutable reference to `ConfigData`. Worker thread reads ~50 fields per frame — including `QString` fields (yolo model paths). GUI thread writes these concurrently. `QString` is NOT safe for concurrent read/write.

**Strategy:** Extend the proven `ProcessingParams` snapshot pattern. Create a `PipelineConfig` struct that snapshots all Config fields read by pipeline methods.

### Steps:

**5.1** Create `src/pipeline/pipelineconfig.h`:
```cpp
struct PipelineConfig {
    // from DeviceSettings
    int flip = 0;

    // from ImageProcSettings (all fields)
    float accu_base, gamma, low_in, high_in, low_out, high_out, dehaze_pct;
    int colormap, ecc_window_mode, ecc_warp_mode, ecc_fusion_method;
    int ecc_backward, ecc_forward, ecc_levels, ecc_max_iter;
    double ecc_eps;
    bool ecc_half_res_reg, ecc_half_res_fuse;
    double lower_3d_thresh, upper_3d_thresh;
    bool truncate_3d, custom_3d_param;
    float custom_3d_delay, custom_3d_gate_width;
    int model_idx;
    bool fishnet_recog;
    float fishnet_thresh;

    // from SaveSettings (all fields)
    bool save_info, custom_topleft_info, save_in_grayscale, consecutive_capture, integrate_info;
    int img_format;

    // from YoloSettings (all fields including QStrings)
    QString config_path;
    int main_display_model, alt1_display_model, alt2_display_model;
    QString visible_model_path, visible_classes_file;
    QString thermal_model_path, thermal_classes_file;
    QString gated_model_path, gated_classes_file;
};
```

**5.2** Add to `UserPanel`:
- `PipelineConfig m_pipeline_config;` (under existing `m_params_mutex`)
- `void update_pipeline_config();` — copies from `config->get_data()`

**5.3** Call `update_pipeline_config()` from `update_processing_params()` (same mutex).

**5.4** In `grab_thread_process()` main loop, snapshot both:
```cpp
PipelineConfig pcfg;
ProcessingParams params;
{ QMutexLocker lk(&m_params_mutex); params = m_processing_params; pcfg = m_pipeline_config; }
```

**5.5** Add `const PipelineConfig&` parameter to all pipeline methods:
- `preprocess_frame()`, `detect_yolo()`, `frame_average_and_3d()`, `enhance_frame()`
- `render_and_display()`, `detect_fishnet()`, `record_frame()`

**5.6** Replace all `config->get_data().*` reads in these methods with `pcfg.*`.

**5.7** Move `custom_3d_delay`/`custom_3d_gate_width` from `ProcessingParams` to `PipelineConfig` (they're Config fields, not UI widgets).

---

## Implementation Order

1. Fix 1 (fishnet signal) — trivial, immediate safety win
2. Fix 2 (angle atomics) — trivial, no API change
3. Fix 3 (TCU atomics) — small, isolated
4. Fix 4 (scan mutex) — careful getter return-type changes
5. Fix 5 (Config snapshot) — largest; touches all pipeline methods

## Verification

- **Build:** `cmake --build build` — no compilation errors
- **Smoke test:** Launch app, start grab thread, verify frame processing (no crashes/regressions)
- **Scan test:** Start/stop a scan to exercise ScanController mutex paths
- **Fishnet test:** Enable fishnet detection — verify FISHNET_RESULT label updates correctly
