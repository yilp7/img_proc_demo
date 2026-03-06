# Sprint 8 Progress: Extract `grab_thread_process()` Pipeline

## Prerequisite: Characterization tests TC-IP-*
Status: SATISFIED

Phase 4 test_imageproc (12 cases) covers all enhancement algorithms called from grab_thread_process. Sprint 5 thread safety fixes completed. Sprint 7 Config as single source of truth completed.

## Analysis

### Current architecture

`grab_thread_process()` in `src/visual/userpanel.cpp` (lines 913-1806, ~894 lines) is the main worker thread loop. It runs in `GrabThread` (QThread) and handles the entire image pipeline:

```
acquire → rotate/flip → histogram → auto-MCP → YOLO → fishnet →
frame_avg/ECC → 3D_reconstruct → enhance → overlay → display →
scan_advance → save/record
```

### The problem

1. **Size**: 894 lines with 5+ levels of nesting, 3 switch statements, ~40 if/else branches
2. **Thread safety**: 18 direct UI widget reads from the worker thread (not thread-safe)
3. **Persistent state**: 12+ local variables persist across loop iterations (frame averaging buffers, ECC state, scan state)
4. **Coupling**: Reads config, pref, ui, p_tcu, active_ptz, displays, and ~30 UserPanel member variables

### UI reads from worker thread (thread-safety violations)

| Widget | Lines | Type |
|--------|-------|------|
| `ui->STATUS->packet_lost->set_text(...)` | 1018 | **MUTATION** |
| `ui->MCP_SLIDER->hasFocus()` | 1079 | Read |
| `ui->FRAME_AVG_CHECK->isChecked()` | 1254, 1373 | Read |
| `ui->FRAME_AVG_OPTIONS->currentIndex()` | 1255 | Read |
| `ui->IMG_3D_CHECK->isChecked()` | 1365 | Read |
| `ui->IMG_ENHANCE_CHECK->isChecked()` | 1433 | Read |
| `ui->ENHANCE_OPTIONS->currentIndex()` | 1434 | Read |
| `ui->BRIGHTNESS/CONTRAST/GAMMA_SLIDER->value()` | 1569-1571 | Read |
| `ui->PSEUDOCOLOR_CHK->isChecked()` | 1574 | Read |
| `ui->HIST_DISPLAY->size()` | 1618 | Read |
| `ui->INFO_CHECK->isChecked()` | 1640, 1778 | Read |
| `ui->CENTER_CHECK->isChecked()` | 1658 | Read |
| `ui->SELECT_TOOL->isChecked()` | 1664 | Read |
| `ui->DUAL_DISPLAY_CHK->isChecked()` | 1680 | Read |
| `pref->split` | 1362 | Read |
| `pref->ui->CUSTOM_3D_*_EDT->text()` | 1387-1410 | Read |
| `pref->ui->CUSTOM_INFO_EDT->text()` | 1636 | Read |

### Approach: method extraction with explicit parameters

Extract code into private methods of UserPanel. Methods take explicit parameters to avoid hidden `this->` coupling, making future class promotion (Sprint 9) straightforward. Each sub-task is a separate commit.

**ProcessingParams prerequisite**: Create a plain struct that snapshots all UI widget state once per frame. GUI thread writes to it (via signals), grab thread copies it at loop start. Eliminates all 18 ui->/ pref-> reads from the worker thread.

**State bundling**: Persistent loop variables are bundled into named structs (`FrameAverageState`, `ECCState`) passed by reference to methods.

## Task breakdown

### Task 8.0: Create ProcessingParams snapshot struct
Status: DONE

New file `src/pipeline/processingparams.h` (header-only):

```cpp
struct ProcessingParams {
    // main window checkboxes/sliders
    bool frame_avg_enabled = false;     // ui->FRAME_AVG_CHECK
    int  frame_avg_mode = 0;            // ui->FRAME_AVG_OPTIONS
    bool enable_3d = false;             // ui->IMG_3D_CHECK
    bool enhance_enabled = false;       // ui->IMG_ENHANCE_CHECK
    int  enhance_option = 0;            // ui->ENHANCE_OPTIONS
    int  brightness = 0;                // ui->BRIGHTNESS_SLIDER
    int  contrast = 0;                  // ui->CONTRAST_SLIDER
    int  gamma_slider = 10;             // ui->GAMMA_SLIDER
    bool pseudocolor_enabled = false;   // ui->PSEUDOCOLOR_CHK
    bool show_info = false;             // ui->INFO_CHECK
    bool show_center = false;           // ui->CENTER_CHECK
    bool select_tool = false;           // ui->SELECT_TOOL
    bool dual_display = false;          // ui->DUAL_DISPLAY_CHK
    bool mcp_slider_focused = false;    // ui->MCP_SLIDER->hasFocus()
    QSize hist_display_size;            // ui->HIST_DISPLAY->size()
    // from pref->
    bool split = false;                 // pref->split
    float custom_3d_delay = 0.0f;       // pref->ui->CUSTOM_3D_DELAY_EDT
    float custom_3d_gate_width = 0.0f;  // pref->ui->CUSTOM_3D_GW_EDT
    QString custom_info_text;           // pref->ui->CUSTOM_INFO_EDT
};
```

Changes:
1. Add `QMutex m_params_mutex` and `ProcessingParams m_processing_params` to UserPanel
2. Add `update_processing_params()` that reads all widget values under mutex
3. Connect widget signals (toggled, valueChanged, currentIndexChanged) to `update_processing_params()`
4. At top of grab_thread_process main loop: copy snapshot under lock
5. Replace every `ui->` and `pref->` read with `params.*`
6. Replace `ui->STATUS->packet_lost->set_text(...)` with `emit packet_lost_signal(packets_lost)`

Files: `src/pipeline/processingparams.h` (NEW), `src/visual/userpanel.h`, `src/visual/userpanel.cpp`, `CMakeLists.txt`

### Task 8.1: Extract `acquire_frame()` method
Status: DONE

Extract lines 965-1031 (queue drain block inside image_mutex lock).

```cpp
struct AcquiredFrame { cv::Mat frame; bool updated; int packets_lost_frame; };
AcquiredFrame acquire_frame(int thread_idx, cv::Mat& prev_img);
```

Handles: queue overflow drain (MAX_QUEUE_SIZE), device_type==1 desync detection, frame dequeue with packet loss tracking, q_fps_calc, device_type==-1 re-push, LVTONG empty-queue sleep.

Depends on: 8.0
Files: `src/visual/userpanel.h`, `src/visual/userpanel.cpp`

### Task 8.2: Extract `preprocess_frame()` method
Status: DONE

Extract lines 1035-1091 (rotation switch, histogram, flip, auto-MCP).

```cpp
void preprocess_frame(int thread_idx, const ProcessingParams& params,
    FrameAverageState& avg_state, int& _w, int& _h, uint hist[256]);
```

Introduces `FrameAverageState` struct to bundle persistent frame averaging variables:

```cpp
struct FrameAverageState {
    cv::Mat seq[8], seq_sum, frame_a_sum, frame_b_sum;
    cv::Mat prev_img, prev_3d;
    int seq_idx = 0;
    bool frame_a = true;
    void release_buffers();
};
```

Handles: 4 rotation cases with buffer release on dimension change, grayscale histogram calculation, config-based flip, auto-MCP adaptive exposure (emit update_mcp_in_thread).

Depends on: 8.0, 8.1
Files: `src/visual/userpanel.h`, `src/visual/userpanel.cpp`

### Task 8.3: Extract `detect_yolo()` method
Status: DONE

Extract lines 1093-1162 (YOLO model lifecycle + inference).

```cpp
std::vector<YoloResult> detect_yolo(int thread_idx, bool updated, YoloDetector*& yolo);
```

Handles: per-thread model selection from config->get_data().yolo.*, model change detection via m_yolo_last_model[], detector delete/create under m_yolo_init_mutex, inference with grayscale→BGR conversion.

Depends on: 8.0
Files: `src/visual/userpanel.h`, `src/visual/userpanel.cpp`

### Task 8.7: Extract `detect_fishnet()` method
Status: DONE

Extract lines 1178-1248 (`#ifdef LVTONG` DNN inference block).

```cpp
#ifdef LVTONG
double detect_fishnet(int thread_idx, cv::dnn::Net& net);
#endif
```

Handles: 4 model types (legacy 224x224 RGB, pix2pix/maskguided 256x256 gray, semantic 224x224 gray), softmax/comparison, emit update_fishnet_result(). Returns is_net value consumed by display overlay and recording.

Depends on: 8.0
Files: `src/visual/userpanel.h`, `src/visual/userpanel.cpp`

### Task 8.4a: Extract `frame_average_and_3d()` method
Status: DONE

Extract lines 1250-1428 (frame averaging, ECC temporal denoising, 3D gated reconstruction). **Highest-risk extraction.**

```cpp
struct FrameAvg3DResult { int ww, hh; bool is_3d; };
FrameAvg3DResult frame_average_and_3d(int thread_idx, bool updated,
    const ProcessingParams& params, FrameAverageState& avg_state,
    ECCState& ecc_state, int _w, int _h);
```

Introduces `ECCState` struct:

```cpp
struct ECCState {
    std::deque<cv::Mat> fusion_buf, fusion_warps, fusion_warps_inv;
    cv::Mat fusion_last_warp;
    int prev_ecc_warp_mode = -1;
    void clear();
};
```

Three code paths:
1. Frame avg disabled: direct convertTo to modified_result (line 1359)
2. Frame avg mode 0: 4-frame running sum → convertTo (lines 1274-1276)
3. Frame avg mode != 0: ECC temporal denoising (lines 1278-1342) — ecc_register_consecutive + temporal_denoise_fuse

3D reconstruction (lines 1364-1428): gated3D_v2 from A/B frame sums, with #ifdef LVTONG/#else for dist_mat computation (non-LVTONG computes distance matrix, emits update_dist_mat).

Depends on: 8.0, 8.1, 8.2
Files: `src/visual/userpanel.h`, `src/visual/userpanel.cpp`

### Task 8.4b: Extract `enhance_frame()` method
Status: DONE

Extract lines 1429-1588 (the `else` branch of the 3D check).

```cpp
void enhance_frame(int thread_idx, const ProcessingParams& params,
    int _w, int _h, int& ww, int& hh);
```

10 enhancement cases (all stateless, calling ImageProc:: static methods):
0=none, 1=hist_equalization, 2=laplace, 3=plateau_equl_hist, 4=accumulative_enhance, 5=guided_image_filter, 6=adaptive_enhance, 7=haze_removal(inverted), 8=haze_removal(DCP), 9=aindane

Plus: brightness_and_contrast (alpha/beta from sliders), gamma correction, pseudocolor mapping (thread_idx==0, grayscale only).

Depends on: 8.0, 8.4a
Files: `src/visual/userpanel.h`, `src/visual/userpanel.cpp`

### Task 8.5: Extract `render_and_display()` method
Status: DONE

Extract lines 1590-1686 (overlay, resize, QImage emit, dual display).

```cpp
void render_and_display(int thread_idx, int display_idx,
    const ProcessingParams& params, const std::vector<YoloResult>& yolo_results,
    double is_net, int ww, int hh, float weight);
```

Handles: YOLO bounding box drawing, alt-display histogram (emit set_hist_pixmap), display region cropping, TCU info text (delay/gate or dist/dov depending on base_unit), timestamp overlay, center crosshair, selection rectangle, info mask, QImage conversion (RGB888 or Indexed8), set_pixmap emission, dual display (original image on secondary display).

Depends on: 8.0, 8.3, 8.7
Files: `src/visual/userpanel.h`, `src/visual/userpanel.cpp`

### Task 8.6: Extract `advance_scan()` method
Status: DONE

Extract lines 1690-1749 (scan state machine).

```cpp
void advance_scan(int thread_idx, bool updated, int& scan_img_count,
    QString& scan_save_path_a, QString& scan_save_path);
```

Handles: scan countdown (scan_img_count--), image saving at configured intervals, PTZ/TCU coordinate advancement (scan_ptz_idx, scan_tcu_idx), directory creation, emit finish_scan_signal() when route exhausted, emit update_delay_in_thread() for TCU delay update. Uses active_ptz->ptz_set_angle() (Sprint 6).

Mutates shared members: scan_ptz_idx, scan_tcu_idx, delay_dist (existing thread-safety issue, documented for Sprint 9).

Depends on: 8.0
Files: `src/visual/userpanel.h`, `src/visual/userpanel.cpp`

### Task 8.8: Extract `record_frame()` method
Status: DONE

Extract lines 1756-1795 (BMP save + video recording).

```cpp
void record_frame(int thread_idx, int display_idx, bool updated,
    const ProcessingParams& params, double is_net,
    const QString& info_tcu, const QString& info_time,
    int ww, int hh, float weight);
```

Handles: save_original/save_modified → save_to_file(bool, int), clear flags if not consecutive_capture. record_original/record_modified → overlay info text if configured, color-convert, write to vid_out[]. Preserves #ifdef LVTONG fishnet overlay on recording.

Depends on: 8.0
Files: `src/visual/userpanel.h`, `src/visual/userpanel.cpp`

### Task 8.9: Wire pipeline — shrink main loop
Status: DONE

After all extractions, replace 800+ lines of inline code with method call sequence (~60-70 lines):

```
while (grab_image[thread_idx]) {
    params = snapshot ProcessingParams
    acq = acquire_frame()
    {
        QMutexLocker processing_lock
        preprocess_frame()
        yolo_results = detect_yolo()
        is_net = detect_fishnet()       // LVTONG only
        avg_result = frame_average_and_3d()
        if (!avg_result.is_3d) enhance_frame()
        compute info_tcu, info_time
        render_and_display()
        advance_scan()
        record_frame()
        update prev_img
    }
}
```

Clean up local variable declarations, bundle into FrameAverageState/ECCState structs at function scope.

Depends on: ALL previous tasks
Files: `src/visual/userpanel.h`, `src/visual/userpanel.cpp`

## Execution order

```
8.0  ProcessingParams ──→ 8.1 acquire ──→ 8.2 preprocess ──→ 8.4a avg+3D ──→ 8.4b enhance
         │                                                         │
         ├──→ 8.3 yolo ─────────────────────────────────────────────┤
         ├──→ 8.7 fishnet ──────────────────────────────────────────┤
         ├──→ 8.6 scan ────────────────────────────────────────────→├──→ 8.5 display
         └──→ 8.8 record ─────────────────────────────────────────→ 8.9 wire
```

Recommended sequential order: 8.0 → 8.1 → 8.2 → 8.3 → 8.7 → 8.4a → 8.4b → 8.5 → 8.6 → 8.8 → 8.9

## Summary of files to create/modify

| File | Action |
|------|--------|
| `src/pipeline/processingparams.h` | NEW: ProcessingParams struct definition |
| `src/visual/userpanel.h` | Add 10 method declarations, ProcessingParams/FrameAverageState/ECCState/AcquiredFrame structs, m_params_mutex, packet_lost_signal |
| `src/visual/userpanel.cpp` | Extract 10 methods from grab_thread_process(), add update_processing_params(), connect widget signals |
| `CMakeLists.txt` | Add src/pipeline/ to include path |

## Risks

- **ECC state corruption** (CRITICAL): ECC temporal denoising has complex buffer management (deques of cv::Mat, warp matrices). Extraction must preserve exact buffer push/pop ordering. Mitigation: pixel-identical test on ECC output frames.
- **ProcessingParams snapshot staleness**: Widget changes delayed by up to 1 frame. Same latency as current code — acceptable.
- **MCP_SLIDER hasFocus()**: In snapshot, may always be false if focusChanged not connected. Mitigation: connect QWidget::focusChanged signal.
- **Scan shared state mutation**: advance_scan() mutates scan_ptz_idx, scan_tcu_idx, delay_dist from worker thread. Existing issue, documented for Sprint 9 ScanController.
- **info_tcu/info_time cross-cutting**: Needed by both render_and_display() and record_frame(). Solution: compute once in main loop, pass to both.
- **#ifdef LVTONG**: Multiple code paths in fishnet detection and 3D reconstruction differ between LVTONG and non-LVTONG builds. Must preserve all guards exactly.

## Bug fixes during Sprint 8

- **3D colorbar missing without avg** (FIXED): `FrameAverageState::release_buffers()` incorrectly released `prev_img`/`prev_3d` when called from the avg-disable path. The original code only released averaging buffers (seq_sum, seq[], frame_a_sum, frame_b_sum) in that path. Fix: split into `release_avg_buffers()` (avg-disable path) and `release_all()` (rotation dimension-change path). Root cause: `gated3D_v2()` silently returned early when `prev_img` was empty (size mismatch guard on line 381).
- **`update_processing_params()` reading `params` instead of config** (FIXED): `custom_3d_delay`/`custom_3d_gate_width` were referencing a nonexistent local `params` variable. Fixed to read from `config->get_data().image_proc.*`.
