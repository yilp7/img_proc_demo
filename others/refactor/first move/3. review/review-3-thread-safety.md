# Phase 3 — Thread Safety Audit

> Post-Sprint 9 + Thread Safety Fixes | Reviewed 2026-03-16

---

## Table of Contents

1. [ProcessingParams/PipelineConfig Effectiveness](#1-processingparamspipelineconfig-effectiveness)
2. [Remaining Cross-Thread Variable Access](#2-remaining-cross-thread-variable-access)
3. [Signal Emissions from Worker Thread](#3-signal-emissions-from-worker-thread)
4. [QMutex and Atomic Inventory](#4-qmutex-and-atomic-inventory)
5. [Thread Affinity Map](#5-thread-affinity-map)
6. [Specific Risk: Scan State Race](#6-specific-risk-scan-state-race)
7. [Specific Risk: Config Thread Safety](#7-specific-risk-config-thread-safety)
8. [Severity-Classified Finding Summary](#8-severity-classified-finding-summary)

---

## 1. ProcessingParams/PipelineConfig Effectiveness

The snapshot pattern (Sprint 8 + Fix 5) successfully eliminated:
- All `config->get_data().*` reads from pipeline → replaced with `pcfg.*`
- All `pref->ui->` reads from pipeline → replaced with `params.*`
- All `ui->` widget reads from pipeline → replaced with `params.*`

**Verification**: No remaining `config->`, `pref->`, or `ui->` accesses exist in pipeline methods.

**However**, ~20 UserPanel member variables are still read directly from pipeline methods without synchronization. These were documented in the Sprint 5 audit as "practically safe on x86" and deferred. They are categorized below.

---

## 2. Remaining Cross-Thread Variable Access

### Category A: Device Properties (set once at camera connect, stable during grab)

These are written by the GUI thread during camera enumeration/connect and read by the pipeline. They do NOT change while the grab loop is running.

| Variable | Pipeline Reads (line numbers) | GUI Writes | Severity |
|----------|-------------------------------|------------|----------|
| `is_color[i]` | 889, 982-984, 990, 995, 1006, 1019, 1081, 1087, 1209, 1297, 1353, 1360, 1448, 1467, 1486 | Lines 2083-2088, 2331-2336, 4315 (camera connect) | 🟢 LOW |
| `pixel_depth[i]` | 896, 1477, 1486 | update_pixel_depth() at camera connect | 🟢 LOW |
| `w[i]`, `h[i]` | 862-881, 1477 | Set at camera connect | 🟢 LOW |
| `device_type` | 806, 815, 842, 852, 1436, 1440 | Line 1966 (camera enumerate), 4318 (static display) | 🟢 LOW |

**Rationale for LOW**: These are written once during camera initialization (which stops the grab thread first via `grab_image[i] = false`), then remain constant throughout the grab loop. The grab loop checks `grab_image[i]` before proceeding, so there's a happens-before relationship by convention. Technically a data race per C++ standard, but no observable corruption risk.

**Recommendation**: Add these to ProcessingParams in a future sprint (straightforward, low risk). Or use `std::atomic` for the simple bool/int values.

### Category B: Display Configuration (changed by user during grab)

| Variable | Pipeline Reads | GUI Writes | Severity |
|----------|---------------|------------|----------|
| `image_rotate[0]` | 862 (preprocess_frame) | 4268 (data_exchange) | 🟡 HIGH |
| `pseudocolor[i]` | 1297, 1353, 1467 | 4769 (on_PSEUDOCOLOR_CHECK) | 🟡 HIGH |
| `image_3d[i]` | 1087, 1209, 1342, 1455 | on_IMG_3D_CHECK_stateChanged | 🟡 HIGH |

**Rationale for HIGH**: These are user-toggled bools/ints that can change *while the pipeline is processing a frame*. On x86, a torn read of a bool/int is effectively impossible, but this is **undefined behavior per C++ standard**. More importantly, if the value changes mid-frame, different pipeline stages see different values for the same frame, causing visual glitches (e.g., rotation applied but pseudocolor format mismatch).

**Recommendation**: Add to ProcessingParams snapshot. These are simple value types — minimal effort.

### Category C: Save/Record Flags (written by button clicks, read by pipeline)

| Variable | Pipeline Reads | GUI Writes | Severity |
|----------|---------------|------------|----------|
| `save_original[i]` | 1434, 1436 | 2187, button click | 🟡 HIGH |
| `save_modified[i]` | 1438, 1440 | 4401, button click | 🟡 HIGH |
| `record_original[i]` | 1442, 1450 | button click | 🟡 HIGH |
| `record_modified[i]` | 1453, 1469 | button click | 🟡 HIGH |
| `grab_thread_state[i]` | 1476 (write), 1570 (write) | 4307 (busy-wait read), 4550 (read) | 🟡 HIGH |

**Rationale for HIGH**: These are read-modify-write patterns. Line 1436: `save_original[thread_idx] = 0` in the pipeline clears a flag set by the GUI. Without atomics, the compiler may optimize or reorder this. `grab_thread_state` is used in a busy-wait spin loop at line 4307 (`while (grab_thread_state[select_display_thread]) ;`) which is UB without volatile/atomic.

**Recommendation**: Make all of these `std::atomic<bool>`. The busy-wait at line 4307 is particularly problematic — should use atomic + yield/sleep.

### Category D: Shared cv::Mat Objects

| Variable | Pipeline Access | GUI Access | Severity |
|----------|----------------|------------|----------|
| `list_roi` (vector) | 1132 (read in frame_average_and_3d) | 238, 244 (mousePress/Release push_back/swap) | 🔴 CRITICAL |
| `user_mask[i]` (cv::Mat) | 1132 (read), 1487 (write) | 239, 245 (mousePress setTo/clear) | 🔴 CRITICAL |
| `vid_out[0..3]` (VideoWriter) | 1450-1470 (write frames) | 2203, 2216, 4362, 4374 (open/release) | 🔴 CRITICAL |

**Rationale for CRITICAL**:
- **`list_roi`**: A `std::vector` modified by `push_back()` on the GUI thread (line 238) while being read by `.size()` on the worker thread (line 1132). A reallocation during push_back while the worker reads `.size()` is **heap corruption / crash**.
- **`user_mask`**: `cv::Mat::setTo()` on GUI thread while worker thread reads the same Mat. cv::Mat is not thread-safe for concurrent read+write.
- **`vid_out`**: `cv::VideoWriter::write()` on worker thread while GUI opens/releases the same writer. VideoWriter is not thread-safe.

### Category E: GUI Widget Access from Worker Thread

| Access | Line | Severity |
|--------|------|----------|
| `window()->grab()` | 1379 (advance_scan) | 🔴 CRITICAL |
| `displays[display_idx]->selection_v1` etc. | 1342-1348 (render_and_display) | 🟡 HIGH |

**`window()->grab()`** at line 1379: This is a `QWidget::grab()` call from the worker thread. QWidget methods must only be called from the GUI thread. This can cause crashes, rendering artifacts, or deadlocks. Must be replaced with a signal to capture the screenshot on the GUI thread.

**Display member reads** at 1342-1348: Reading `selection_v1`, `selection_v2`, `display_region` from Display widget. These are set by mouse events on the GUI thread. On x86 these are likely benign word-size reads, but formally UB.

---

## 3. Signal Emissions from Worker Thread

All signals emitted from pipeline methods have been verified for correct connection types:

| Signal | Emit Sites | Connection | Type | Status |
|--------|-----------|------------|------|--------|
| `packet_lost_updated(int)` | 850 | Line 350 lambda | QueuedConnection | ✅ |
| `update_mcp_in_thread(int)` | 898-899 | Line 301 | QueuedConnection | ✅ |
| `update_fishnet_result(int)` | 1165,1180,1194,1200 | Line 679 | QueuedConnection | ✅ |
| `set_hist_pixmap(QPixmap)` | 1313 | Line 247 | QueuedConnection | ✅ |
| `finish_scan_signal()` | 1411 | Line 458 | QueuedConnection | ✅ |
| `update_delay_in_thread()` | 1424 | Line 300 | QueuedConnection | ✅ |
| `set_model_list_enabled(bool)` | 1491, 1568 | Line 306 | QueuedConnection | ✅ |
| `task_queue_full()` | 1845+ (7 sites) | Line 310 | UniqueConnection | ✅ |
| `update_dist_mat(...)` | 1132-1133 | Distance3DView | QueuedConnection | ✅ |
| Display `set_pixmap(QPixmap)` | 1354, 1361 | Internal Display | QueuedConnection | ✅ |

**Verdict**: All cross-thread signals correctly use QueuedConnection. No violations found. Sprint 5 fixes (finish_scan_signal, set_model_list_enabled) and Fix 1 (update_fishnet_result) are properly in place.

---

## 4. QMutex and Atomic Inventory

### Mutexes

| Mutex | Class | Protects | Lock Sites | Status |
|-------|-------|----------|------------|--------|
| `image_mutex[3]` | UserPanel | img_mem queue drain | 801, 1518, 1581-1590 | ✅ Well-scoped |
| `frame_info_mutex` | UserPanel | q_frame_info queue | 807, 816, 843 | ✅ Well-scoped |
| `m_params_mutex` | UserPanel | ProcessingParams + PipelineConfig snapshot | 1493, 1513, 2753 | ✅ Well-scoped |
| `m_yolo_init_mutex` | UserPanel | YoloDetector reinitialization | 918 | ✅ Well-scoped |
| `port_mutex` | UserPanel | Serial/TCP communicate | 3058, 3068-3109 | ✅ |
| `display_mutex[3]` | UserPanel | Display handle lock | 3953-4247 (load_image/video) | ⚠️ Used only in file load, NOT in pipeline |
| `m_scan_mutex` | ScanController | All scan state variables | Header getters/setters + cpp | ✅ Fix 4 |
| `write/read/retrieve_mutex` | ControlPort | Port I/O operations | controlport.cpp | ✅ |
| `write/read/retrieve_mutex` | USBCAN | USBCAN device ops | usbcan.cpp | ✅ |
| `socket/retrieve_mutex` | UDPPTZ | UDP socket ops | udpptz.cpp | ✅ |
| `mat_mutex` | Distance3DView | dist_mat | distance3dview.cpp | ✅ |
| `cout_mutex` | Global (util.h) | Console output | lens/ptz/tcu.cpp | ✅ |
| `mtx` | TimedQueue | TimedQueue::q | userpanel.cpp:57-96 | ✅ |

**No nested lock patterns found** — each mutex is independently scoped. No deadlock risk identified.

### Atomics

| Variable | Class | Type | Purpose |
|----------|-------|------|---------|
| `angle_h`, `angle_v` | DeviceManager | `atomic<float>` | PTZ angles (Fix 2) |
| `delay_dist` | TCUController | `atomic<float>` | Delay distance (Fix 3) |
| `frame_a_3d` | TCUController | `atomic<bool>` | 3D toggle (Fix 3) |
| `hold`, `interupt` | Lens | `atomic<bool>` | Lens command flow control |
| `hold`, `interupt` | TCU | `atomic<bool>` | TCU command flow control |
| `tcu_type` | TCU | `atomic<uint>` | TCU type selection |
| `rf_type` | RangeFinder | `atomic<uint>` | Rangefinder type |
| `_is_running` | ThreadPool | `atomic_bool` | Pool lifecycle |

---

## 5. Thread Affinity Map

```
Main Thread (GUI)
├── UserPanel (all UI slots, controller calls, config updates)
├── DeviceManager (owns port/thread lifecycle)
├── TCUController, LensController, LaserController, RFController
├── ScanController, AuxPanelManager
├── Preferences, ScanConfig, Config
└── Display[3], TitleBar, StatusBar, FloatingWindow

GrabThread[0..2] (Worker)
├── grab_thread_process() main loop
├── acquire_frame(), preprocess_frame(), detect_yolo()
├── frame_average_and_3d(), enhance_frame()
├── render_and_display(), advance_scan()
├── detect_fishnet(), record_frame()
└── save_to_file(), save_scan_img()

th_tcu    → p_tcu (TCU port object)
th_lens   → p_lens (Lens port object)
th_laser  → p_laser (Laser port object)
th_ptz    → p_ptz (PTZ port object)
th_rf     → p_rf (RangeFinder port object)
th_usbcan → p_usbcan (USBCAN port object)
th_udpptz → p_udpptz (UDPPTZ port object)

JoystickThread → Joystick polling (Windows API)

ThreadPool → Background BMP/image save tasks
```

---

## 6. Specific Risk: Scan State Race — RESOLVED ✅

Fix 4 added `QMutex m_scan_mutex` to ScanController with proper per-getter/setter locking. `advance_scan()` in the worker thread accesses scan state exclusively through mutex-protected getters/setters. The scan start path (`on_SCAN_BUTTON_clicked`) groups index initialization under a single lock before setting `scan = true`.

**Remaining minor issue**: `scan_3d` and `scan_sum` (cv::Mat) are accessed via non-mutex reference getters (`get_scan_3d()`, `get_scan_sum()`). However, grep confirms these are **never called** from outside ScanController — they're only written at scan start (line 50-51) before scanning begins. No actual race exists.

---

## 7. Specific Risk: Config Thread Safety — RESOLVED ✅

Fix 5 introduced `PipelineConfig` snapshot struct. All `config->get_data().*` reads in pipeline methods have been replaced with `pcfg.*` reads from the snapshotted copy.

`Config::get_data()` returns `ConfigData&` (mutable reference, no mutex). This is safe because:
- All remaining `Config` reads/writes occur on the GUI thread
- The worker thread no longer calls `config->get_data()` — it uses the PipelineConfig snapshot
- `auto_save()` is called from the GUI thread only

**No action needed**, but documenting the invariant ("Config must only be accessed from GUI thread") would be good practice.

---

## 8. Severity-Classified Finding Summary

### 🔴 CRITICAL — Data race with observable crash/corruption risk

| # | Finding | Location | Impact | Recommended Fix |
|---|---------|----------|--------|-----------------|
| C1 | `list_roi` vector read from worker while GUI does push_back | 1132 vs 238 | Heap corruption, crash | Mutex or copy-on-write snapshot |
| C2 | `user_mask[0]` cv::Mat concurrent read+write | 1132 vs 239,245 | Mat data corruption | Mutex around both access sites |
| C3 | `vid_out[]` VideoWriter::write from worker while GUI opens/releases | 1450-1470 vs 2203,4362 | Crash or file corruption | Mutex per video writer |
| C4 | `window()->grab()` called from worker thread | 1379 | QWidget crash/deadlock | Emit signal, capture on GUI thread |

### 🟡 HIGH — UB per C++ standard, low crash probability on x86

| # | Finding | Location | Recommended Fix |
|---|---------|----------|-----------------|
| H1 | `image_rotate[0]` read without sync | 862 | Add to ProcessingParams |
| H2 | `pseudocolor[i]` read without sync | 1297,1353,1467 | Add to ProcessingParams |
| H3 | `image_3d[i]` read without sync | 1087,1209,1342,1455 | Add to ProcessingParams |
| H4 | `save_original/modified[i]` read-modify-clear | 1434-1440 | `std::atomic<bool>` |
| H5 | `record_original/modified[i]` read-modify-clear | 1442-1470 | `std::atomic<bool>` |
| H6 | `grab_thread_state[i]` spin-wait without atomic | 4307 | `std::atomic<bool>` + yield |
| H7 | Display widget member reads from worker | 1342-1348 | Snapshot selection rect in ProcessingParams |

### 🟢 LOW — Benign race, word-size read of stable value

| # | Finding | Location | Notes |
|---|---------|----------|-------|
| L1 | `is_color[i]` read without sync | 15+ sites | Set at camera connect, stable during grab |
| L2 | `pixel_depth[i]` read without sync | 896, 1477, 1486 | Same as above |
| L3 | `w[i]`, `h[i]` read without sync | 862-881, 1477 | Same as above |
| L4 | `device_type` read without sync | 806,815,842,852 | Same as above |

---

## Priority Fix Order

1. **C1+C2** (list_roi + user_mask): Add a mutex. Both are accessed in the same code path (line 1132) and the same GUI handler (lines 238-245), so a single mutex covers both.
2. **C3** (vid_out): Add per-writer mutex. GUI open/release and worker write must not overlap.
3. **C4** (window()->grab): Replace with signal + queued slot.
4. **H1-H3** (image_rotate, pseudocolor, image_3d): Add to ProcessingParams snapshot — minimal effort, eliminates mid-frame inconsistency.
5. **H4-H6** (save/record flags, grab_thread_state): Convert to `std::atomic<bool>`.
6. **H7** (display selection rect): Snapshot into ProcessingParams or use atomics.
7. **L1-L4**: Add to ProcessingParams in a future sprint, or leave as-is (genuinely low risk).
