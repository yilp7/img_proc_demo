# Review 1 — Fix Progress

> Tracks implementation of the 5 urgent thread safety fixes from review-1-retrospective.md
> and the PTZ data logging bug discovered during testing.

---

## Status Summary

| # | Fix | Status | Commit |
|---|-----|--------|--------|
| 1 | Fishnet signal `Qt::QueuedConnection` | DONE | pending |
| 2 | DeviceManager `angle_h`/`angle_v` → `std::atomic<float>` | DONE | pending |
| 3 | TCUController `frame_a_3d`/`delay_dist` → `std::atomic` | DONE | pending |
| 4 | ScanController `QMutex` for scan state | DONE | pending |
| 5 | Config `PipelineConfig` snapshot struct | DONE | pending |
| 6 | PTZ data not appearing in DATA_EXCHANGE | DONE | pending |

---

## Fix 1: Fishnet signal connection type — DONE

**File:** userpanel.cpp (line 678)

Changed `connect(this, SIGNAL(update_fishnet_result(int)), SLOT(display_fishnet_result(int)));`
to add explicit `Qt::QueuedConnection`, preventing `display_fishnet_result()` from executing
in the worker thread context.

---

## Fix 2: DeviceManager angle atomics — DONE

**Files:** devicemanager.h, devicemanager.cpp

- Changed `float angle_h; float angle_v;` → `std::atomic<float> angle_h{0.0f}; std::atomic<float> angle_v{0.0f};`
- Updated all getters/setters to use `.load()`/`.store()`
- Fixed `point_ptz_to_target()`: `angle_h +=` → `angle_h.store(angle_h.load() + ...)`
- Fixed `fmod(angle_h + 360.0, 360.0)` → `fmod(angle_h.load() + 360.0, 360.0)`
- Fixed `update_ptz_angle()`: `angle_h = _h` → `angle_h.store(_h)`
- Fixed `set_ptz_angle()`: `ptz_set_angle(angle_h, angle_v)` → `ptz_set_angle(angle_h.load(), angle_v.load())`

---

## Fix 3: TCUController atomics — DONE

**Files:** tcucontroller.h, tcucontroller.cpp

- Changed `bool frame_a_3d` → `std::atomic<bool> frame_a_3d{false}`
- Changed `float delay_dist` → `std::atomic<float> delay_dist{0.0f}`
- Updated getters to use `.load()`, setters to use `.store()`
- Added `toggle_frame_a_3d()` with `compare_exchange_weak` loop (fixes TOCTOU race)
- Updated userpanel.cpp line 1118: `set_frame_a_3d(!get_frame_a_3d())` → `toggle_frame_a_3d()`
- Fixed all MSVC compatibility issues with `std::atomic<float>`:
  - `QString::asprintf("%.2f", delay_dist)` → `delay_dist.load()` (variadic)
  - `qBound(0.0f, delay_dist, max)` → `delay_dist.load()` (template deduction)
  - `delay_dist = val` → `delay_dist.store(val)` throughout
  - `abs(delay_dist - val)` → `abs(delay_dist.load() - val)`

---

## Fix 4: ScanController mutex — DONE

**Files:** scancontroller.h, scancontroller.cpp

- Added `#include <QMutex>`, `#include <QMutexLocker>`
- Added `mutable QMutex m_scan_mutex;` private member
- All getters protected with `QMutexLocker lk(&m_scan_mutex)`
- All setters protected with `QMutexLocker lk(&m_scan_mutex)`
- Changed `get_scan_name()` return from `const QString&` → `QString` (by value, safe cross-thread)
- Changed `get_scan_ptz_route()` / `get_scan_tcu_route()` from `const vector&` → return by value
- Fixed internal direct writes in scancontroller.cpp to use mutex:
  - `on_SCAN_BUTTON_clicked()`: grouped scan_idx/ptz_idx/tcu_idx/routes/step writes under single lock;
    scan_name set under separate lock; stop path locks for `scan = false`
  - `on_CONTINUE_SCAN_BUTTON_clicked()` / `on_RESTART_SCAN_BUTTON_clicked()`: `scan = true` under lock
  - `auto_scan_for_target()`: scan_stopping_delay/scan_step under lock
  - `enable_scan_options()`: reads `scan` via `is_scanning()` getter instead of direct access

---

## Fix 5: PipelineConfig snapshot — DONE

**Files:** NEW pipelineconfig.h, processingparams.h, userpanel.h, userpanel.cpp, CMakeLists.txt

### New struct: `src/pipeline/pipelineconfig.h`
- Contains all Config fields read by pipeline methods:
  - `DeviceSettings::flip`
  - All `ImageProcSettings` fields (accu_base, gamma, thresholds, ECC params, fishnet, etc.)
  - All `SaveSettings` fields (save_info, custom_topleft_info, save_in_grayscale, etc.)
  - All `YoloSettings` fields including QString paths (model paths, classes files)
  - `custom_3d_delay`, `custom_3d_gate_width` (moved from ProcessingParams)

### ProcessingParams changes
- Removed `custom_3d_delay` and `custom_3d_gate_width` (now in PipelineConfig)

### UserPanel changes
- Added `PipelineConfig m_pipeline_config;` member (under existing `m_params_mutex`)
- `update_processing_params()` now also snapshots all Config fields into `m_pipeline_config`
- `grab_thread_process()` main loop snapshots both structs under one mutex lock:
  ```cpp
  ProcessingParams params;
  PipelineConfig pcfg;
  { QMutexLocker lk(&m_params_mutex); params = m_processing_params; pcfg = m_pipeline_config; }
  ```
- All 7 pipeline methods updated with `const PipelineConfig& pcfg` parameter:
  `preprocess_frame`, `detect_yolo`, `frame_average_and_3d`, `enhance_frame`,
  `render_and_display`, `detect_fishnet`, `record_frame`
- Also updated `save_to_file` and `save_scan_img` with pcfg parameter
- Replaced ~50+ `config->get_data().*` reads in pipeline methods with `pcfg.*`
- Fixed 3 GUI-thread false positives from bulk replace (lines 314, 315, 2344)
- Fixed LVTONG init code to read `m_pipeline_config.model_idx` under mutex

---

## Fix 6: PTZ data not appearing in DATA_EXCHANGE — DONE

**Bug:** After Sprint 6 PTZ controller unification, `active_ptz->ptz_move/stop/set_angle()`
were called directly from the GUI thread (and worker thread in `advance_scan`), but the PTZ
object lives on `th_ptz`. This caused `ControlPort::communicate()` to run on the wrong thread,
preventing `port_io_log` signals from being delivered to DATA_EXCHANGE. Additionally,
`active_ptz` was only set when the serial port connected, blocking the signal path when no
COM port was open (unlike TCU/Lens which emit unconditionally).

**Root cause:** Direct cross-thread method calls bypassed the existing `send_ptz_msg` signal
(connected via `Qt::QueuedConnection` to `p_ptz->ptz_control()` on `th_ptz`).

### Files modified

**devicemanager.h:**
- Added `send_ptz_angle(float h, float v)`, `send_ptz_angle_h(float h)`, `send_ptz_angle_v(float v)` helper methods

**devicemanager.cpp:**
- `ptz_button_pressed()`: for PTZ type 0, emit `send_ptz_msg(PTZ::SPEED, speed)` + `send_ptz_msg(direction)` instead of direct `active_ptz->ptz_move()`
- `ptz_button_released()`: for PTZ type 0, emit `send_ptz_msg(PTZ::STOP)` instead of direct `active_ptz->ptz_stop()`
- `set_ptz_angle()`: for PTZ type 0, emit `send_ptz_msg(PTZ::SET_H/SET_V)` instead of direct `active_ptz->ptz_set_angle()`
- `STOP_BTN` lambda: same pattern
- New methods `send_ptz_angle()`, `send_ptz_angle_h()`, `send_ptz_angle_v()`: type-switched dispatch
- Removed `active_ptz` null guard from PTZ type 0 paths (signal goes to `p_ptz` which always exists, matching TCU/Lens behavior)
- USBCAN/UDPPTZ paths keep direct calls (they handle threading internally)

**userpanel.cpp:**
- `advance_scan()` line 1397: replaced `active_ptz_ctrl()->ptz_set_angle()` → `m_device_mgr->send_ptz_angle()`
- `keyPressEvent` lines 3583, 3589: replaced `active_ptz_ctrl()->ptz_set_angle_h/v()` → `m_device_mgr->send_ptz_angle_h/v()`
