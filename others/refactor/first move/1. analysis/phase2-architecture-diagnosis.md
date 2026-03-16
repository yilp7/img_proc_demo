# Phase 2 — Architecture Diagnosis: Smell Detection

> **Project:** GLI_user_panel (v0.10.2.0)
> **Date:** 2026-02-28
> **Cross-references:** Feature IDs from [phase1-feature-inventory.md](phase1-feature-inventory.md)

---

## Table of Contents

1. [Diagnostic Checklist Summary](#diagnostic-checklist-summary)
2. [Critical Issues (Must Refactor)](#critical-issues-must-refactor)
3. [Moderate Issues (Should Refactor)](#moderate-issues-should-refactor)
4. [Minor Issues (Nice to Have)](#minor-issues-nice-to-have)
5. [Overall Severity Map](#overall-severity-map)

---

## Diagnostic Checklist Summary

| # | Smell | Detected? | Severity | Instances |
|---|---|---|---|---|
| 1 | God Class / God Module | **YES** | CRITICAL | 1 primary, 1 secondary |
| 2 | Shotgun Surgery | **YES** | HIGH | Multiple areas |
| 3 | Feature Envy | **YES** | MEDIUM | Preferences ↔ UserPanel |
| 4 | Spaghetti Code | **YES** | CRITICAL | GUI + business + comm tangled |
| 5 | Copy-Paste Programming | **YES** | HIGH | 6+ identified clusters |
| 6 | Magic Numbers / Strings | **YES** | HIGH | 50+ instances |
| 7 | Missing Abstraction Layer | **YES** | CRITICAL | PTZ controllers, camera callbacks |
| 8 | Circular Dependencies | **YES** | MEDIUM | automation → visual |
| 9 | Excessive Coupling | **YES** | CRITICAL | Global state, void* casts |
| 10 | Patch Artifacts | **YES** | HIGH | #ifdef LVTONG, disabled AutoScan |
| 11 | Dead Code | **YES** | MEDIUM | ~300 lines of #if 0 / commented code |
| 12 | Inconsistent Error Handling | **YES** | HIGH | Mixed strategies throughout |

---

## Critical Issues (Must Refactor)

### S-001: God Class — `UserPanel` (6,994 + 710 lines)

- **Description:** `UserPanel` is the single largest anti-pattern in the codebase. With **200+ member variables**, **64+ private slots**, **15 signals**, and a 6,994-line implementation, it owns virtually every responsibility in the application: camera management, device communication (TCU, lens, laser, PTZ), image processing pipeline, scanning logic, video recording, file I/O, UI state management, joystick input handling, YOLO detection model lifecycle, preset management, and configuration sync.

- **Files Involved:**
  - `src/visual/userpanel.h:92-710` — 200+ member variables, 100-line constructor initializer
  - `src/visual/userpanel.cpp:1-6994` — All business logic in one file

- **Concrete Evidence:**
  - Constructor initializer list spans **lines 91–193** (103 lines of member initialization)
  - Member variables include: 3-element arrays for displays (`w[3]`, `h[3]`, `pixel_depth[3]`, `img_mem[3]`, etc.), 5-element port arrays (`serial_port[5]`, `tcp_port[5]`), scan state (`scan_ptz_route`, `scan_tcu_route`, `scan_tcu_idx`), joystick button state (`joybtn_A/B/X/Y/L1/L2/R1/R2`), physics constants (`c`, `dist_ns`), and more
  - The class simultaneously acts as: **View** (display management), **Controller** (event handlers), **Model** (state: `delay_dist`, `depth_of_view`, `rep_freq`), **Service** (device I/O via signal emissions), and **Coordinator** (scan orchestration)

- **Impact Scope:** Touches 77% of all 181 features (F-001 through F-181). Any modification risks cascading side effects.

- **Suggested Approach:** Extract into:
  1. `DeviceManager` — camera lifecycle, device enumeration
  2. `TCUController` — timing control logic and UI ↔ TCU sync
  3. `ImagePipeline` — grab thread processing, enhancement, YOLO
  4. `ScanController` — scanning state machine, route management
  5. `IOManager` — file save/load, video recording
  6. `PTZCoordinator` — unified PTZ interface across serial/CAN/UDP

---

### S-002: God Method — `grab_thread_process()` (~900 lines)

- **Description:** The method at `userpanel.cpp:947-1845` is a monolithic image processing pipeline running in a background thread. It handles frame acquisition, queue management, rotation/flipping, histogram calculation, auto-MCP, YOLO model lifecycle, YOLO inference, LVTONG fishnet detection, frame averaging (basic + ECC temporal denoising), 3D gated imaging, image enhancement (10 algorithms), pseudocolor, display rendering, scan state advancement, image saving, and video recording — all in a single `while` loop.

- **Files Involved:**
  - `src/visual/userpanel.cpp:947-1845`

- **Concrete Evidence:**
  - **Lines 947–992:** Variable initialization (45 lines of local state including `cv::Mat` arrays, deques, counters)
  - **Lines 999–1008:** Queue overflow management with magic `> 5` limit
  - **Lines 1010–1022:** Queue desynchronization detection with `device_type == 1` check
  - **Lines 1070–1107:** Image rotation (4 `switch` cases with duplicated buffer release logic)
  - **Lines 1109–1128:** Histogram calculation + auto-MCP, with magic `200` divisor and `0.94`/`0.39` thresholds
  - **Lines 1131–1200:** YOLO detector lifecycle management (model load/unload/swap)
  - **Lines 1216–1260:** `#ifdef LVTONG` fishnet detection (customer-specific DNN inference)
  - **Lines 1729–1788:** Scan state machine advancement (calls `on_SCAN_BUTTON_clicked()` from thread!)
  - **Lines 1796–1834:** Image/video save logic with format conversion

- **Impact Scope:** Features F-015 through F-031, F-098 through F-120, F-121 through F-128

- **Suggested Approach:**
  1. Extract `ImageRotator` — handles rotation/flip/resize logic
  2. Extract `FrameProcessor` — enhancement + pseudocolor + histogram
  3. Extract `YoloManager` — model lifecycle + inference
  4. Extract `ScanEngine` — scan state machine (must not call UI slots from thread)
  5. Extract `RecordingManager` — video writing + image saving

---

### S-003: Spaghetti Code — GUI ↔ Business Logic ↔ Communication Tangled

- **Description:** UI slot handlers directly contain business logic, state mutations, and device communication commands with no intermediate layer. There is no Model, ViewModel, Service, or Controller abstraction separating concerns.

- **Files Involved:**
  - `src/visual/userpanel.cpp` — throughout

- **Concrete Evidence:**

  **Example 1 — `change_mcp()` at line 4604:**
  ```cpp
  void UserPanel::change_mcp(int val) {
      if (val < 0) val = 0;                              // Business logic: clamping
      if (val > mcp_max) val = mcp_max;                  // Business logic: clamping
      ui->MCP_EDIT->setText(QString::number(val));        // UI update
      ui->MCP_SLIDER->setValue(val);                      // UI update
      if (val == (int)std::round(p_tcu->get(TCU::MCP))) return;  // State query
      ui->MCP_LABEL->setText(QString::number(val));       // UI update
      emit send_double_tcu_msg(TCU::MCP, val);            // Device communication
  }
  ```

  **Example 2 — Scan logic called from grab thread at line 1773:**
  ```cpp
  // Inside grab_thread_process() — a BACKGROUND THREAD
  on_SCAN_BUTTON_clicked();  // Calling a UI SLOT from a worker thread!
  ```
  This is a thread-safety violation: a UI slot (which accesses UI widgets) called from a non-GUI thread.

  **Example 3 — `on_DIST_BTN_clicked()` mixes dialog display + physics calculation + TCU command:**
  The handler reads user input, computes `delay = distance / dist_ns`, constrains it, updates UI sliders, and emits TCU commands — all in one function.

- **Impact Scope:** Every device control feature (F-042 through F-097)

- **Suggested Approach:** Introduce a **Service Layer** pattern:
  - UI slots only: read input → call service → update UI from service result
  - Services: contain business rules, emit device commands
  - Device ports: receive commands, return status via signals

---

### S-004: Missing Abstraction — Three PTZ Controllers Without Common Interface

- **Description:** `PTZ` (serial/Pelco-D), `USBCAN` (CAN bus), and `UDPPTZ` (UDP) all control the same type of hardware (pan-tilt-zoom mount) but have no shared abstract interface. They use different base classes (`ControlPort` vs `QObject`), different operation enums, different frame formats, and different signal signatures. `UserPanel` must handle each one separately with different connection logic, different slot handlers (`update_ptz_params`, `update_usbcan_angle`, `update_udpptz_angle`), and hardcoded calls like `p_usbcan->emit control(USBCAN::POSITION, ...)` directly in the scan loop.

- **Files Involved:**
  - `src/port/ptz.h/cpp` — Inherits `ControlPort`, Pelco-D protocol
  - `src/port/usbcan.h/cpp` — Inherits `QObject`, CAN bus protocol
  - `src/port/udpptz.h/cpp` — Inherits `QObject`, UDP protocol
  - `src/visual/userpanel.h:548-554` — All three instantiated as separate members

- **Concrete Evidence:**
  - Three separate slot handlers in `userpanel.h`:
    - Line 178: `update_ptz_params(qint32 ptz_param, double val)` — for serial PTZ
    - Line 180: `update_usbcan_angle(float _h, float _v)` — for CAN PTZ
    - Line 181: `update_udpptz_angle(float _h, float _v)` — for UDP PTZ
  - Scan logic at line 1758 hardcodes `p_usbcan`:
    ```cpp
    p_usbcan->emit control(USBCAN::POSITION, scan_ptz_route[scan_ptz_idx].first);
    p_usbcan->emit control(USBCAN::PITCH, scan_ptz_route[scan_ptz_idx].second);
    ```
  - No way to switch PTZ type at runtime without modifying `UserPanel`

- **Impact Scope:** Features F-078 through F-097 (all PTZ features), F-121 through F-128 (scanning)

- **Suggested Approach:** Create `IPTZController` abstract interface:
  ```cpp
  class IPTZController : public QObject {
      virtual void move(Direction dir, double speed) = 0;
      virtual void stop() = 0;
      virtual void setAngle(double h, double v) = 0;
      virtual QPair<double,double> getAngle() const = 0;
  signals:
      void angleUpdated(double h, double v);
  };
  ```
  Have PTZ, USBCAN, and UDPPTZ implement it. UserPanel holds a single `IPTZController*`.

---

### S-005: Excessive Coupling via Global State and Unsafe Casts

- **Description:** The system relies on global variables for shared state (themes, fonts, cursors) defined in `util.h`, and uses `void*` pointers with unsafe C-style casts for thread communication. The `GrabThread` and `JoystickThread` classes pass `UserPanel*` as `void*` and cast it back without any type safety.

- **Files Involved:**
  - `src/util/util.h:36-48` — 8 global externs (fonts, theme, cursors, mutex)
  - `src/visual/userpanel.h:53,62` — `GrabThread(void *info, int idx)` + `void *p_info`
  - `src/visual/userpanel.cpp:26` — `((UserPanel*)p_info)->grab_thread_process(&_display_idx)`
  - `src/thread/joystick.h:15,62` — Same pattern
  - `src/cam/mvcam.h:25,51` — `void* user_data` in frame callback

- **Concrete Evidence:**
  ```cpp
  // userpanel.cpp:26 — unsafe cast from void* to UserPanel*
  void GrabThread::run() {
      ((UserPanel*)p_info)->grab_thread_process(&_display_idx);
  }
  ```
  If `p_info` is ever corrupted or points to a deleted object, this is undefined behavior with no compile-time or runtime check.

  Camera SDK callbacks use the same pattern:
  ```cpp
  // mvcam.h:51
  static void __stdcall frame_cb(...void* user_data);
  ```

- **Impact Scope:** Thread safety across all grab threads (F-015 through F-031), joystick (F-146 through F-148)

- **Suggested Approach:**
  - Replace `void*` in `GrabThread` with typed pointer: `UserPanel* m_panel`
  - For camera callbacks (SDK-mandated `void*`): use `static_cast` with assertions
  - Move global theme/font state into a `ThemeManager` singleton or pass via dependency injection

---

## Moderate Issues (Should Refactor)

### M-001: Copy-Paste Programming — Duplicated Throttling/Validation Patterns

- **Description:** Multiple functions share identical throttling (static `QElapsedTimer`, 40ms check) and value-clamping logic but are copy-pasted rather than abstracted.

- **Files Involved:**
  - `src/visual/userpanel.cpp:3926-3992` — `update_laser_width()`, `update_delay()`, `update_gate_width()`
  - `src/port/tcu.cpp:108-150` — `connect_to_serial_port()` and `connect_to_tcp_port()` duplicate 8-line init sequence

- **Concrete Evidence:**

  **Cluster 1 — TCU parameter update functions (userpanel.cpp):**
  ```cpp
  // Line 3926: update_laser_width()
  static QElapsedTimer t;
  if (t.elapsed() < 40) return;     // SAME throttle
  t.start();
  if (laser_width < 0) laser_width = 0;                    // SAME clamp pattern
  if (laser_width > pref->max_laser_width) laser_width = pref->max_laser_width;
  emit send_double_tcu_msg(TCU::LASER_USR, laser_width);   // SAME emit pattern

  // Line 3941: update_delay() — IDENTICAL structure
  // Line 3969: update_gate_width() — IDENTICAL structure
  ```

  **Cluster 2 — Rotation handling (userpanel.cpp:1070-1107):**
  Cases 0, 1, 2, 3 all contain identical buffer-release blocks:
  ```cpp
  prev_img.release(), prev_3d.release();
  seq_sum.release(), frame_a_sum.release(), frame_b_sum.release();
  for (auto& m: seq) m.release();
  seq_idx = 0;
  ```
  This 4-line block is repeated 3 times in the switch.

  **Cluster 3 — Slider setup (userpanel.cpp:319-347):**
  Four sliders (MCP, DELAY, GW, FOCUS_SPEED) follow identical `setMinimum/setMaximum/setSingleStep/setPageStep/setValue/connect` pattern.

  **Cluster 4 — TCU connection init (tcu.cpp:108-150):**
  `connect_to_serial_port()` and `connect_to_tcp_port()` both call the same 8 `set_user_param()` sequence after successful connection.

  **Cluster 5 — Video record writing (userpanel.cpp:1804-1834):**
  Original and modified video recording blocks are near-identical (clone, putText if save_info, cvtColor, write).

- **Impact Scope:** Maintainability of F-042 through F-059 (TCU), F-034/F-035 (recording)

- **Suggested Approach:** Extract into helper functions:
  - `throttled_tcu_update(param, &value, min, max)` for TCU parameters
  - `release_frame_buffers(...)` for rotation buffer cleanup
  - `setup_slider(slider, min, max, step, page, initial)` for slider init
  - `init_tcu_on_connect()` for post-connection initialization

---

### M-002: Patch Artifacts — `#ifdef LVTONG` Scattered Throughout

- **Description:** The `LVTONG` build variant adds customer-specific fishnet detection logic through 15+ `#ifdef LVTONG` blocks scattered across userpanel.cpp. These blocks embed DNN inference, threshold comparison, and text rendering directly in the main image pipeline, making the code hard to follow and maintaining two code paths simultaneously.

- **Files Involved:**
  - `src/visual/userpanel.cpp` — Lines 782, 974, 1026, 1216, 1423, 1440, 1453, 1459, 1681, 1821, 1840, 2480, 5140, 6015, 6908
  - `src/visual/preferences.cpp` — Lines 135, 386, 464, 538

- **Concrete Evidence (userpanel.cpp:1216-1260):**
  ```cpp
  #ifdef LVTONG
      double is_net = 0;
      static double min, max;
      if (pref->fishnet_recog) {
          switch (pref->model_idx) {
          case 0: {
              cv::cvtColor(img_mem[thread_idx], fishnet_res, cv::COLOR_GRAY2RGB);
              fishnet_res.convertTo(fishnet_res, CV_32FC3, 1.0 / 255);  // Magic number
              cv::resize(fishnet_res, fishnet_res, cv::Size(224, 224));  // Magic size
              cv::Mat blob = cv::dnn::blobFromImage(fishnet_res, 1.0, cv::Size(224, 224));
              net.setInput(blob);
              cv::Mat prob = net.forward("195");  // Magic layer name!
              // ... softmax computation inline ...
          }
          // ... 3 more cases with different sizes (256, 256) ...
          }
      }
  #endif
  ```
  This is 60+ lines of DNN inference logic directly in the grab thread, guarded by a compile flag.

- **Impact Scope:** F-120 (fishnet detection), readability of F-015–F-031 (image pipeline)

- **Suggested Approach:** Extract LVTONG fishnet as a **plugin** or separate `FishnetDetector` class that implements a common detection interface (same as YOLO). Remove compile-time branching; use runtime configuration.

---

### M-003: Magic Numbers and Hardcoded Strings

- **Description:** Numeric literals and string constants are used directly in logic without named constants, making the code hard to understand and maintain.

- **Files Involved:**
  - `src/visual/userpanel.cpp` — Throughout
  - `src/port/ptz.cpp` — Protocol bytes
  - `src/port/udpptz.cpp` — IP addresses, port numbers
  - `src/port/tcu.cpp` — Protocol format bytes

- **Concrete Evidence:**

  | Location | Value | What It Means |
  |---|---|---|
  | userpanel.cpp:999 | `> 5` | Max queue size before dropping frames |
  | userpanel.cpp:1118 | `/ 200` | Auto-MCP pixel threshold divisor |
  | userpanel.cpp:1121 | `0.94` | Upper brightness threshold for MCP |
  | userpanel.cpp:1126 | `0.39` | Lower brightness threshold for MCP |
  | userpanel.cpp:1224 | `1.0 / 255` | Normalization constant |
  | userpanel.cpp:1246 | `cv::Size(256, 256)` | Model input size |
  | userpanel.h:155 | `w{640, 480, 480}` | Default display dimensions |
  | userpanel.cpp:967 | `/ 1024.0` | Font scale factor |
  | userpanel.cpp:185 | `tp(40)` | Thread pool size of 40 |
  | ptz.cpp:118–127 | `0xFF, 0x0C, 0x08, ...` | Pelco-D protocol bytes |
  | ptz.cpp:158 | `-40., 40.` | Vertical angle limits |
  | ptz.cpp:89,101 | `36000, 4000, 100` | Angle scale factors |
  | ptz.cpp:172 | `-11111` | Unknown error return code |
  | udpptz.cpp:6,8 | `"192.168.1.10"`, `30000` | Default IP and port |
  | userpanel.cpp:5287 | `< 99` | Max drag-and-drop file count |
  | userpanel.cpp:325 | `40` (in lambda) | Throttle timer in ms |

- **Impact Scope:** Readability and maintainability across the entire codebase

- **Suggested Approach:**
  - Define protocol constants in protocol header files (e.g., `pelco_d.h`)
  - Define UI constants in a `constants.h` (queue sizes, thresholds, default sizes)
  - Move network defaults to config with named fallback constants

---

### M-004: Missing Virtual Destructor in `Cam` Base Class

- **Description:** The `Cam` abstract base class has a non-virtual destructor. Since `Cam*` pointers are used to hold derived class instances (`MvCam`, `EbusCam`, `EuresysCam`, `HQVCam`), deleting through the base pointer causes undefined behavior — derived destructors won't run, leaking SDK handles and resources.

- **Files Involved:**
  - `src/cam/cam.h:14-16`
  - `src/visual/userpanel.h:529` — `Cam* curr_cam;`

- **Concrete Evidence:**
  ```cpp
  // cam.h:14-16
  public:
      Cam() {};
      ~Cam() {};  // NOT VIRTUAL — derived destructors will NOT be called
  ```
  All derived camera classes hold SDK handles (`void* dev_handle` in MvCam, HQVCam) that must be released in their destructors.

- **Impact Scope:** F-001 through F-010 (camera management) — resource leaks on shutdown

- **Suggested Approach:** Change to `virtual ~Cam() {}` (one-line fix, high impact).

---

### M-005: `#if 0` Dead Code and Commented-Out Code

- **Description:** The codebase contains ~300 lines of disabled code via `#if 0` blocks and commented-out sections. These represent abandoned approaches, old implementations, and debug traces that add cognitive load without providing value.

- **Files Involved:**
  - `src/visual/userpanel.cpp` — Lines 69-87 (alt FPS calc), 82-87, 2886-2898 (old COM write), 1202-1214 (auto-focus), 6122+ (old PTZ)
  - `src/visual/userpanel.h` — Lines 117-120 (old thread pointers), 299-304 (disabled radio buttons), 542-546 (old Thread classes), 623-635 (commented Mat arrays)
  - `src/cam/cam.h:46` — `#if 1` block (always-on, unusual)
  - `src/thread/controlportthread.cpp:471` — Disabled 8-byte protocol variant
  - `src/visual/lasercontrol.cpp:125` — Old button handler
  - `src/visual/serialserver.cpp:11` — Commented init (TODO)
  - `src/port/lens.cpp:158` — Alternate speed setting

- **Concrete Evidence:**
  ```cpp
  // userpanel.h:542-546 — Old architecture left as comments
  // TODO: implement by inheriting QThread
  //    TCUThread*    ptr_tcu;
  //    InclinThread* ptr_inc;
  //    LensThread*   ptr_lens;
  //    LaserThread*  ptr_laser;
  //    PTZThread*    ptr_ptz;
  ```

  ```cpp
  // userpanel.cpp:1202-1214 — Entire auto-focus algorithm commented out
  /*
      // tenengrad (sobel) auto-focus
      cv::Sobel(img_mem, sobel, CV_16U, 1, 1);
      clarity[2] = clarity[1], clarity[1] = clarity[0], clarity[0] = cv::mean(sobel)[0] * 1000;
      if (focus_direction) {
          if (clarity[2] > clarity[1] && clarity[1] > clarity[0]) {
              focus_direction *= -2;
              // FIXME auto focus by traversing
              ...
          }
      }
  */
  ```

- **Impact Scope:** Readability, cognitive load during maintenance

- **Suggested Approach:** Delete all `#if 0` blocks and commented-out code. These are preserved in git history if ever needed.

---

### M-006: Inconsistent Error Handling

- **Description:** The codebase mixes four different error handling strategies with no consistency: return codes, `qDebug`/`qWarning`, silent failures, and (rarely) exceptions.

- **Files Involved:**
  - Throughout `src/port/` — return codes
  - `src/port/ptz.cpp:172` — Returns `-11111` for unknown param
  - `src/port/usbcan.cpp:63-107` — `qDebug` for connection events
  - `src/port/ptz.cpp:223-235` — try/catch for JSON only
  - `src/visual/userpanel.cpp:26` — Unsafe cast, no error check
  - `src/cam/mvcam.cpp:287` — Frame callback, no null check

- **Concrete Evidence:**

  | Strategy | Example | Location |
  |---|---|---|
  | Silent failure | Queue empty → use previous frame | userpanel.cpp:1032 |
  | Magic return code | `return -11111` | ptz.cpp:172 |
  | qDebug only | `qDebug() << "GCAN connected"` | usbcan.cpp:67 |
  | No null check | `((UserPanel*)p_info)->...` | userpanel.cpp:26 |
  | try/catch | JSON parsing only | ptz.cpp:223-235 |
  | Bool success | `bool success = ControlPort::connect_to_serial_port(...)` | ptz.cpp:46 |

- **Impact Scope:** Debugging and reliability across all features

- **Suggested Approach:** Define error handling policy:
  - Device communication → return `ErrorCode` enum + `qWarning`
  - Configuration parsing → exceptions (already done in ptz.cpp JSON)
  - Thread safety → assertions in debug builds
  - User-facing errors → signal to status bar

---

### M-007: Feature Envy — Preferences ↔ UserPanel Bidirectional Data Flow

- **Description:** `UserPanel` and `Preferences` have excessive bidirectional coupling. `UserPanel::init()` reads from `pref->` member variables directly (over 30 references), and `Preferences` holds raw copies of device state that must be manually synced. Changes to preferences require updating both the Preferences dialog and UserPanel state, creating a "shotgun surgery" pattern.

- **Files Involved:**
  - `src/visual/userpanel.cpp` — 30+ references to `pref->max_dist`, `pref->auto_mcp`, `pref->symmetry`, `pref->fishnet_recog`, `pref->save_info`, etc.
  - `src/visual/preferences.h/cpp` — Holds 60+ member variables mirroring UserPanel state
  - `src/visual/userpanel.cpp` — `syncConfigToPreferences()`, `syncPreferencesToConfig()`

- **Concrete Evidence:**
  ```cpp
  // userpanel.cpp:1117-1118 — Accessing preferences from grab thread
  if (pref->auto_mcp && !ui->MCP_SLIDER->hasFocus()) {
      int thresh_num = img_mem[thread_idx].total() / 200;
  ```
  The grab thread (background) directly reads `pref->auto_mcp` — a value owned by a dialog that can be open/modified simultaneously.

- **Impact Scope:** F-137 (preferences dialog), all features that have configurable parameters

- **Suggested Approach:** Make `Config` the single source of truth. Preferences writes to `Config`, UserPanel reads from `Config`. Remove direct cross-references.

---

### M-008: God File — `mywidget.h` Contains 13 Unrelated Classes

- **Description:** `mywidget.h` (305 lines) packs 13 distinct widget classes into one header, and `mywidget.cpp` (749 lines) contains all their implementations. The classes have no logical relationship: `Display` (image viewer), `Ruler` (measurement), `TitleBar` (window chrome), `StatusBar` (device indicators), `FloatingWindow` (secondary display), `MiscSelection` (combo box), etc.

- **Files Involved:**
  - `src/widgets/mywidget.h:1-305` — 13 class definitions
  - `src/widgets/mywidget.cpp` — All implementations

- **Concrete Evidence:**
  Classes in the file:
  1. `Display` (line 9) — Image display with zoom/select/PTZ modes
  2. `Ruler` (line 58) — Measurement ruler
  3. `InfoLabel` (line 79) — Styled label
  4. `TitleButton` (line 87) — Title bar button
  5. `TitleBar` (line 98) — Window title bar with Preferences and ScanConfig pointers
  6. `AnimationLabel` (line 143) — Sprite animation
  7. `Tools` (line 163) — Radio button tool
  8. `Coordinate` (line 170) — Coordinate display
  9. `IndexLabel` (line 188) — Clickable indexed label
  10. `StatusIcon` (line 207) — Device status indicator
  11. `StatusBar` (line 238) — 8-icon status bar
  12. `FloatingWindow` (line 256) — Draggable secondary window
  13. `MiscSelection` (line 285) — Custom combo box

  Note: `TitleBar` (line 138) holds `Preferences*` and `ScanConfig*` pointers, creating a dependency from a generic widget to specific dialog classes.

- **Impact Scope:** Compilation time, code navigation, separation of concerns

- **Suggested Approach:** Split into individual files: `display.h/cpp`, `titlebar.h/cpp`, `statusbar.h/cpp`, `floatingwindow.h/cpp`, etc.

---

## Minor Issues (Nice to Have)

### L-001: Disabled AutoScan Feature — Incomplete Integration

- **Description:** The AutoScan feature (`src/automation/autoscan.h/cpp`, `scanpreset.h/cpp`) exists as a complete implementation but is commented out in `main.cpp` and `userpanel.h`. Five separate `// NOTE: AutoScan feature temporarily disabled` markers are scattered across the codebase with TODO-style instructions.

- **Files Involved:**
  - `src/main.cpp:3,228-240` — Integration code commented
  - `src/visual/userpanel.h:28-29,112-115,369-371,696-699` — Includes and members commented
  - `src/automation/autoscan.h/cpp` — Full state machine implementation
  - `src/automation/scanpreset.h/cpp` — Preset data structures

- **Impact Scope:** F-129, F-130 (disabled features)

- **Suggested Approach:** Either complete the integration properly or remove the files from the project (they're in git history). The current half-state adds confusion.

---

### L-002: Conditional Compilation Overuse (42+ `#ifdef` blocks)

- **Description:** Heavy reliance on preprocessor conditionals creates multiple implicit code variants: `WIN32`, `DISTANCE_3D_VIEW`, `LVTONG`, `ICMOS`, `USING_CAMERALINK`, `ENABLE_PORT_JSON`, `SIMPLE_UI`, `ENABLE_USER_DEFAULT`, `DEPRECATED_PRESET_STRUCT`, `benchmark`, `CLALLSERIAL`, `BUILD_STATIC`. Some of these (`LVTONG`, `ICMOS`) represent product variants that should be plugins, not compile-time flags.

- **Files Involved:**
  - `src/visual/userpanel.cpp` — 30+ blocks
  - `src/visual/preferences.cpp` — 6+ blocks
  - `src/util/util.h` — Feature flag definitions
  - Multiple port and camera files

- **Impact Scope:** Maintainability — each flag doubles the number of code paths to reason about

- **Suggested Approach:** Replace product-variant flags (`LVTONG`, `ICMOS`) with runtime plugin system. Keep platform flags (`WIN32`) but minimize their scope. Remove `ENABLE_USER_DEFAULT` (already deprecated).

---

### L-003: `goto` Labels in Protocol Code

- **Description:** `ptz.cpp:174-176` uses `goto chksm` and `goto send` labels for control flow in the `ptz_control()` switch statement. While functional, this is fragile and makes the flow hard to follow.

- **Files Involved:**
  - `src/port/ptz.cpp:118-213`

- **Concrete Evidence:**
  ```cpp
  case STOP:       command = ...; goto chksm;
  case UP_LEFT:    command = ...; goto chksm;
  // ...
  chksm:
      command[6] = checksum(command);
  send:
      switch (ptz_param) { ... }
  ```

- **Impact Scope:** F-078 through F-085 (PTZ control)

- **Suggested Approach:** Replace with early return pattern or helper function.

---

### L-004: `static` Local Variables in Thread Context

- **Description:** Several functions use `static` local variables inside methods called from multiple threads, creating shared mutable state without synchronization.

- **Files Involved:**
  - `src/visual/userpanel.cpp:971` — `static int packets_lost;` inside grab_thread_process()
  - `src/visual/userpanel.cpp:1218` — `static double min, max;` in LVTONG block
  - `src/visual/userpanel.cpp:1734` — `static QString scan_save_path_a, scan_save_path;` in scan logic
  - `src/visual/userpanel.cpp:1822` — `static int baseline = 0;` in text rendering

- **Impact Scope:** Thread safety — data races if multiple grab threads run simultaneously

- **Suggested Approach:** Convert to per-thread local variables (non-static) or use thread-local storage.

---

### L-005: Deprecated Code Still Compiled

- **Description:** The `_deprecated/` folder is excluded from compilation, but the `ENABLE_USER_DEFAULT` code path (default 0) and `DEPRECATED_PRESET_STRUCT` blocks are still compiled behind feature flags set to 0 or guarded by `#ifdef`. The deprecated path in `main.cpp:116-147` adds complexity to the entry point.

- **Files Involved:**
  - `src/main.cpp:116-147` — Legacy binary config loading
  - `src/visual/userpanel.cpp:245-256` — Legacy language loading
  - `src/visual/presetpanel.cpp:57,147,206` — `DEPRECATED_PRESET_STRUCT`
  - `src/_deprecated/` — Not compiled but present

- **Suggested Approach:** Remove all `ENABLE_USER_DEFAULT` code blocks and the `_deprecated/` folder.

---

### L-006: Memory Allocation in Protocol Handlers

- **Description:** `ptz.cpp:118-127` allocates arrays with `new uchar[7]{...}` inside `generate_ba()` calls within the switch statement, but it's unclear if `generate_ba()` owns the memory. If it only copies, these are memory leaks (10 allocations per call).

- **Files Involved:**
  - `src/port/ptz.cpp:10-13` — Constructor also uses `new uchar[7]`
  - `src/port/ptz.cpp:118-127` — 10 `new uchar[7]` in switch cases

- **Concrete Evidence:**
  ```cpp
  case STOP: command = generate_ba(new uchar[7]{0xFF, address, ...}, 7); goto chksm;
  ```
  If `generate_ba()` creates a `QByteArray` from the pointer without deleting it, this leaks 7 bytes per call.

- **Impact Scope:** Memory leaks in F-078 through F-085

- **Suggested Approach:** Use stack-allocated arrays: `uchar buf[7] = {...}; command = generate_ba(buf, 7);`

---

## Overall Severity Map

```
┌─────────────────────────────────────────────────────────────┐
│                   CRITICAL (Must Fix)                       │
│                                                             │
│  S-001  UserPanel God Class (6994 lines, 200+ members)      │
│  S-002  grab_thread_process God Method (~900 lines)         │
│  S-003  GUI ↔ Business ↔ Comm tangled (no layers)          │
│  S-004  Three PTZ controllers, zero shared interface        │
│  S-005  void* casts + global state coupling                 │
│                                                             │
├─────────────────────────────────────────────────────────────┤
│                   HIGH (Should Fix)                          │
│                                                             │
│  M-001  Copy-paste programming (6+ clusters)                │
│  M-002  #ifdef LVTONG patch artifacts (15+ blocks)          │
│  M-003  Magic numbers (50+ instances)                       │
│  M-004  Missing virtual destructor in Cam                   │
│  M-005  Dead code (~300 lines of #if 0 / comments)          │
│  M-006  Inconsistent error handling (4 strategies)          │
│  M-007  Preferences ↔ UserPanel bidirectional coupling      │
│  M-008  mywidget.h God File (13 classes)                    │
│                                                             │
├─────────────────────────────────────────────────────────────┤
│                   LOW (Nice to Have)                         │
│                                                             │
│  L-001  Disabled AutoScan half-integrated                   │
│  L-002  Conditional compilation overuse (42+ ifdefs)        │
│  L-003  goto labels in PTZ protocol code                    │
│  L-004  static locals in threaded context                   │
│  L-005  Deprecated code still compiled                      │
│  L-006  Memory leaks in protocol new[] allocations          │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

> **Next Phase:** [phase3-dependency-map.md](phase3-dependency-map.md) will trace inter-module coupling and quantify the hotspots identified here with Fan-in/Fan-out metrics.
