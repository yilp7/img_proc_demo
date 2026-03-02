# Phase 5 — Refactoring Roadmap

> **Project:** GLI_user_panel (v0.10.2.0)
> **Date:** 2026-02-28
> **Cross-references:** [phase2-architecture-diagnosis.md](phase2-architecture-diagnosis.md), [phase3-dependency-map.md](phase3-dependency-map.md), [phase4-test-safety-net.md](phase4-test-safety-net.md)
> **Principle:** Zero feature loss, zero behavior change, incremental structural improvement.

---

## Scoring System

Each task is scored on three axes (1–5 scale):

| Score | Impact (I) | Risk (R) | Effort (E) |
|-------|-----------|----------|------------|
| 1 | Cosmetic improvement | Near-zero risk of regression | < 30 minutes |
| 2 | Measurable code quality gain | Low risk, easily reversible | 1–2 hours |
| 3 | Significant maintainability gain | Moderate risk, needs test coverage | 2–4 hours |
| 4 | Enables future changes | High risk, affects multiple modules | 4–8 hours |
| 5 | Architectural transformation | Very high risk, structural change | 1–2 days |

**Priority Score** = Impact × (6 − Risk) / Effort — higher is better.

---

## Sprint 0: Quick Wins — Zero-Risk Bug Fixes

**Goal:** Fix obvious defects that require no architectural change and cannot cause regressions.
**Prerequisite:** None (can be done before characterization tests).

| # | Task | Smell | Files | I | R | E | Priority | Notes |
|---|------|-------|-------|---|---|---|----------|-------|
| 0.1 | Add `virtual` to `Cam::~Cam()` | M-004 | `cam/cam.h:16` | 4 | 1 | 1 | **20.0** | One-word change: `~Cam() {}` → `virtual ~Cam() {}`. Prevents undefined behavior on delete through base pointer. |
| 0.2 | Fix memory leaks in PTZ `new uchar[7]` | L-006 | `port/ptz.cpp:10-13, 118-127` | 3 | 1 | 1 | **15.0** | Replace `generate_ba(new uchar[7]{...}, 7)` with stack array: `uchar buf[7] = {...}; generate_ba(buf, 7)`. 12 instances. |
| 0.3 | Fix memory leak in TCU constructor | L-006 | `port/tcu.cpp:32` | 2 | 1 | 1 | **10.0** | Same pattern as 0.2. |
| 0.4 | Replace `void*` in GrabThread with typed pointer | S-005 | `userpanel.cpp:5-7,26` | 3 | 1 | 1 | **15.0** | Change `void *p_info` → `UserPanel *m_panel`. Update constructor and `run()`. |
| 0.5 | Replace `void*` in JoystickThread with typed pointer | S-005 | `thread/joystick.h:15,62` | 2 | 1 | 1 | **10.0** | Same as 0.4. |
| 0.6 | Fix `get(GATE_WIDTH_A)` unreachable code | — | `port/tcu.cpp:57-62` | 2 | 1 | 1 | **10.0** | Move `return gate_width_a;` (line 57) inside the switch-case to restore type-dependent behavior. **Behavior change — verify with user.** |

**Sprint 0 total estimated effort:** ~1 hour

---

## Sprint 1: Dead Code Removal & Hygiene

**Goal:** Remove noise that impedes readability. Pure deletion — no new code added.
**Prerequisite:** Git history preserves all deleted code.

| # | Task | Smell | Files | I | R | E | Priority | Notes |
|---|------|-------|-------|---|---|---|----------|-------|
| 1.1 | Delete all `#if 0` blocks | M-005 | `userpanel.cpp` (6 blocks), `userpanel.h` (3 blocks), `cam.h:46`, `controlportthread.cpp:471`, `lasercontrol.cpp:125`, `serialserver.cpp:11`, `lens.cpp:158` | 3 | 1 | 2 | **7.5** | ~300 lines of dead code. All preserved in git. |
| 1.2 | Delete commented-out code blocks | M-005 | `userpanel.h:542-546`, `userpanel.cpp` (scattered), `preferences.cpp` | 2 | 1 | 2 | **5.0** | Remove `// TODO: implement by inheriting QThread`, old `TCUThread*` etc. |
| 1.3 | Remove `ENABLE_USER_DEFAULT` code path | L-005 | `main.cpp:116-147`, `userpanel.cpp:245-256`, `util.h:4` | 2 | 1 | 1 | **10.0** | Already deprecated (flag = 0). Remove the blocks and the flag definition. |
| 1.4 | Remove or decide on disabled AutoScan | L-001 | `main.cpp:228-240`, `userpanel.h:28-29,112-115,369-371,696-699` | 2 | 1 | 1 | **10.0** | Either remove the `// NOTE: AutoScan feature temporarily disabled` comments and dead includes, or complete the integration. User decision needed. |
| 1.5 | Remove `goto` labels in `ptz_control()` | L-003 | `port/ptz.cpp:118-176` | 2 | 2 | 2 | **4.0** | Refactor switch to: direction cases call `apply_checksum_and_send(command)` helper. |
| 1.6 | Fix `static` locals in `grab_thread_process()` | L-004 | `userpanel.cpp:971,1218,1734,1822` | 3 | 2 | 1 | **12.0** | Change `static int packets_lost` → `int packets_lost` etc. These are incorrectly shared across thread invocations. |

**Sprint 1 total estimated effort:** ~3 hours

---

## Sprint 2: Leaf Module Extraction — MyWidget Breakup

**Goal:** Split the 13-class monolith `mywidget.h/cpp` into individual files and remove the upward dependency on Preferences/ScanConfig.
**Prerequisite:** Characterization tests for Display, TitleBar, StatusBar.

| # | Task | Smell | Files | I | R | E | Priority | Notes |
|---|------|-------|-------|---|---|---|----------|-------|
| 2.1 | Extract `Display` to `widgets/display.h/cpp` | M-008 | `widgets/mywidget.h:9-56` | 3 | 2 | 2 | **6.0** | Move class + implementation. Update includes in `userpanel.h`. |
| 2.2 | Extract `TitleBar` to `widgets/titlebar.h/cpp` | M-008 | `widgets/mywidget.h:87-141` | 3 | 2 | 2 | **6.0** | Includes TitleButton, InfoLabel (used only by TitleBar). |
| 2.3 | Extract `StatusBar` + `StatusIcon` to `widgets/statusbar.h/cpp` | M-008 | `widgets/mywidget.h:207-254` | 2 | 2 | 2 | **4.0** | |
| 2.4 | Extract `FloatingWindow` to `widgets/floatingwindow.h/cpp` | M-008 | `widgets/mywidget.h:256-283` | 2 | 2 | 1 | **8.0** | Self-contained, no cross-deps. |
| 2.5 | Remove TitleBar → Preferences/ScanConfig dependency | V-003 | `widgets/mywidget.h:5-6, 138-139` | 4 | 3 | 3 | **5.3** | TitleBar holds `Preferences*` and `ScanConfig*`. Replace with signals: TitleBar emits `settings_requested()`, `scan_config_requested()`. UserPanel connects them. |
| 2.6 | Remove THREAD → WIDGETS dependency | V-005 | `thread/controlportthread.h` → `widgets/mywidget.h` | 2 | 2 | 1 | **8.0** | Pass StatusIcon* via signal/slot instead of include. |
| 2.7 | Update CMakeLists.txt with new files | — | `CMakeLists.txt` | 1 | 1 | 1 | **5.0** | Replace `src/widgets/mywidget.h/cpp` with individual files. |

**Sprint 2 total estimated effort:** ~6 hours

---

## Sprint 3: Magic Numbers & Constants Extraction

**Goal:** Replace hardcoded values with named constants for readability and maintainability.
**Prerequisite:** None (purely additive — no behavior change).

| # | Task | Smell | Files | I | R | E | Priority | Notes |
|---|------|-------|-------|---|---|---|----------|-------|
| 3.1 | Create `constants.h` for pipeline constants | M-003 | New: `util/constants.h` | 3 | 1 | 2 | **7.5** | `MAX_QUEUE_SIZE = 5`, `AUTO_MCP_DIVISOR = 200`, `THROTTLE_MS = 40`, `FONT_SCALE_DIVISOR = 1024.0`, `THREAD_POOL_SIZE = 40`, `MAX_MCP = 255`, etc. |
| 3.2 | Create Pelco-D protocol constants | M-003 | New: `port/pelco_d.h` | 2 | 1 | 1 | **10.0** | `PELCO_SYNC = 0xFF`, `CMD_STOP = 0x00`, `CMD_UP = 0x08`, `ANGLE_SCALE = 100`, `FULL_ROTATION = 36000`, `VERTICAL_LIMIT = 4000`, etc. |
| 3.3 | Create TCU protocol constants | M-003 | New: `port/tcu_protocol.h` | 2 | 1 | 1 | **10.0** | `TCU_HEAD = 0x88`, `TCU_TAIL = 0x99`, `TYPE0_CLOCK = 8`, `TYPE1_CLOCK = 4`, `MAX_PRF_KHZ = 30`, etc. |
| 3.4 | Replace magic numbers in `grab_thread_process()` | M-003 | `userpanel.cpp:999,1118,1121,1126,967` | 2 | 2 | 2 | **4.0** | Use constants from 3.1. |
| 3.5 | Replace magic numbers in PTZ angle calculations | M-003 | `ptz.cpp:89,101,144,158` | 2 | 1 | 1 | **10.0** | Use constants from 3.2. |

**Sprint 3 total estimated effort:** ~3 hours

---

## Sprint 4: Copy-Paste Elimination

**Goal:** Extract duplicated patterns into helper functions.
**Prerequisite:** Sprint 3 constants available.

| # | Task | Smell | Files | I | R | E | Priority | Notes |
|---|------|-------|-------|---|---|---|----------|-------|
| 4.1 | Extract `throttled_tcu_update()` helper | M-001 | `userpanel.cpp:3926-3992` | 3 | 2 | 2 | **6.0** | `update_laser_width()`, `update_delay()`, `update_gate_width()` all share: static timer check → clamp → emit pattern. |
| 4.2 | Extract `setup_slider()` helper | M-001 | `userpanel.cpp:319-347` | 2 | 1 | 1 | **10.0** | 4 identical slider setups → `setup_slider(slider, min, max, step, page, initial)`. |
| 4.3 | Extract `init_tcu_on_connect()` in TCU | M-001 | `port/tcu.cpp:108-150` | 3 | 2 | 1 | **12.0** | `connect_to_serial_port()` and `connect_to_tcp_port()` share identical 14-line init block. |
| 4.4 | Extract rotation buffer release helper | M-001 | `userpanel.cpp:1070-1107` | 2 | 2 | 1 | **8.0** | 4-line buffer release block repeated 3 times in rotation switch. |
| 4.5 | Extract COM port connection lambda pattern | M-001 | `userpanel.cpp:2131-2198` | 3 | 2 | 2 | **6.0** | 5 nearly-identical COM edit `returnPressed` lambdas. Extract `connect_port_edit(edit, port, config_field)`. |

**Sprint 4 total estimated effort:** ~3 hours

---

## Sprint 5: Thread Safety Fixes

**Goal:** Eliminate the 3 identified thread-safety violations.
**Prerequisite:** Characterization tests for scan logic (TC-TCU-* tests).

| # | Task | Smell | Files | I | R | E | Priority | Notes |
|---|------|-------|-------|---|---|---|----------|-------|
| 5.1 | Replace `on_SCAN_BUTTON_clicked()` direct call from GrabThread | S-003 | `userpanel.cpp:1773` | 5 | 3 | 2 | **8.3** | Current: direct function call from worker thread to UI slot. Fix: emit signal → queued connection → slot executes on GUI thread. `emit advance_scan_signal();` with `Qt::QueuedConnection`. |
| 5.2 | Remove `pref->auto_mcp` direct read from GrabThread | S-003 | `userpanel.cpp:1117` | 4 | 2 | 2 | **8.0** | Copy `auto_mcp` to a local `std::atomic<bool>` or pass it via signal when value changes. |
| 5.3 | Remove `pref->ui->MODEL_LIST` access from GrabThread (LVTONG) | S-003 | `userpanel.cpp:974` | 3 | 2 | 1 | **12.0** | Move `setEnabled()` call to signal emitted from thread, caught on GUI thread. |
| 5.4 | Review all `pref->` reads from `grab_thread_process` | S-003 | `userpanel.cpp:1117-1200+` | 4 | 3 | 3 | **5.3** | Audit every `pref->` access in the grab loop. Replace with thread-safe copies or atomic flags updated via signals. |

**Sprint 5 total estimated effort:** ~4 hours

---

## Sprint 6: PTZ Controller Unification

**Goal:** Create a common `IPTZController` interface for PTZ (serial), USBCAN (CAN), and UDPPTZ (UDP).
**Prerequisite:** Characterization tests TC-PTZ-*.
**Note:** These are different controllers for the same PTZ hardware type, not separate device types.

| # | Task | Smell | Files | I | R | E | Priority | Notes |
|---|------|-------|-------|---|---|---|----------|-------|
| 6.1 | Define `IPTZController` abstract interface | S-004 | New: `port/iptzcontroller.h` | 4 | 2 | 2 | **8.0** | Minimal interface: `move(dir, speed)`, `stop()`, `setAngle(h, v)`, `getAngle()`, signals: `angleUpdated(double h, double v)`, `connectionChanged(bool connected)`. |
| 6.2 | Adapt `PTZ` to implement `IPTZController` | S-004 | `port/ptz.h/cpp` | 3 | 3 | 3 | **4.0** | Add `IPTZController` as additional base. Map existing `ptz_control()` to interface methods. Keep `ControlPort` inheritance. |
| 6.3 | Adapt `USBCAN` to implement `IPTZController` | S-004 | `port/usbcan.h/cpp` | 3 | 3 | 3 | **4.0** | Add `IPTZController` as base. Map CAN commands to interface. |
| 6.4 | Adapt `UDPPTZ` to implement `IPTZController` | S-004 | `port/udpptz.h/cpp` | 3 | 3 | 3 | **4.0** | Add `IPTZController` as base. Map UDP commands to interface. |
| 6.5 | Replace 3 angle-update slots with one | S-004 | `userpanel.h:178-181` | 3 | 3 | 2 | **4.0** | Remove `update_ptz_params`, `update_usbcan_angle`, `update_udpptz_angle`. Add single `update_ptz_angle(double h, double v)` connected to `IPTZController::angleUpdated`. |
| 6.6 | Replace PTZ type switch in `init_control_port()` | S-004 | `userpanel.cpp:2178-2191` | 3 | 3 | 2 | **4.0** | Hold `IPTZController* active_ptz`. Switch assigns the right controller. Connection logic unified. |
| 6.7 | Update scan loop to use `IPTZController` | S-004 | `userpanel.cpp:1758` | 4 | 4 | 2 | **4.0** | Replace hardcoded `p_usbcan->emit control(USBCAN::POSITION, ...)` with `active_ptz->setAngle(...)`. |

**Sprint 6 total estimated effort:** ~8 hours

---

## Sprint 7: Config as Single Source of Truth

**Goal:** Eliminate the bidirectional sync between UserPanel, Preferences, and Config. Make Config authoritative.
**Prerequisite:** Sprint 5 (thread safety), characterization tests TC-CF-*.

| # | Task | Smell | Files | I | R | E | Priority | Notes |
|---|------|-------|-------|---|---|---|----------|-------|
| 7.1 | Remove duplicate state from Preferences | M-007 | `visual/preferences.h/cpp` | 4 | 4 | 4 | **2.0** | Preferences currently holds 60+ member variables mirroring UserPanel state. Replace with `Config*` reference. Preferences reads from and writes to Config directly. |
| 7.2 | Remove `syncConfigToPreferences()` | M-007 | `userpanel.cpp:3604-3670` | 3 | 3 | 2 | **4.0** | No longer needed if Preferences reads from Config. |
| 7.3 | Remove `syncPreferencesToConfig()` | M-007 | `userpanel.cpp:3543-3602` | 3 | 3 | 2 | **4.0** | No longer needed if Preferences writes to Config. |
| 7.4 | UserPanel reads from Config, not Preferences | M-007 | `userpanel.cpp` (30+ `pref->` references) | 4 | 4 | 4 | **2.0** | Replace `pref->max_dist` → `config->get_data().tcu.max_dist`, etc. Audit all `pref->` in userpanel.cpp. |
| 7.5 | Add `Config::watch()` observer pattern | — | `util/config.h/cpp` | 3 | 2 | 3 | **4.0** | Emit `config_changed(section)` signal when values change. UI components observe Config. |

**Sprint 7 total estimated effort:** ~8 hours

---

## Sprint 8: Extract `grab_thread_process()` Pipeline

**Goal:** Break the 900-line God Method into composable pipeline stages.
**Prerequisite:** Sprint 5 (thread safety), characterization tests TC-IP-*.
**Note:** This is the highest-impact, highest-risk sprint. Each sub-task should be a separate commit.

| # | Task | Smell | Files | I | R | E | Priority | Notes |
|---|------|-------|-------|---|---|---|----------|-------|
| 8.1 | Extract `FrameAcquisition` stage | S-002 | `userpanel.cpp:995-1060` | 4 | 3 | 3 | **5.3** | Queue management, overflow handling, frame info sync. Input: queues. Output: `cv::Mat frame + metadata`. |
| 8.2 | Extract `FramePreprocessor` stage | S-002 | `userpanel.cpp:1060-1130` | 4 | 3 | 3 | **5.3** | Rotation, flip, resize, histogram calculation, auto-MCP. |
| 8.3 | Extract `YoloManager` class | S-002 | `userpanel.cpp:1131-1215` | 4 | 2 | 3 | **5.3** | YOLO model lifecycle (load/unload/swap) + inference. Already partially isolated. Move to `yolo/yolo_manager.h/cpp`. |
| 8.4 | Extract `FrameEnhancer` stage | S-002 | `userpanel.cpp:1400-1680` | 4 | 3 | 3 | **5.3** | 10 enhancement algorithms dispatch. Input: frame + enhance option index. Output: enhanced frame. |
| 8.5 | Extract `FrameRecorder` stage | S-002 | `userpanel.cpp:1796-1845` | 3 | 2 | 2 | **6.0** | Image save + video recording logic. Already has `save_to_file()` — extend to include vid_out writes. |
| 8.6 | Extract `ScanEngine` state machine | S-002 | `userpanel.cpp:1729-1788` | 5 | 4 | 4 | **2.5** | Scan advancement logic. Must NOT call UI slots directly. Uses signals + `IPTZController`. |
| 8.7 | Extract LVTONG fishnet to `FishnetDetector` class | M-002 | `userpanel.cpp:1216-1260` | 3 | 2 | 3 | **4.0** | Move `#ifdef LVTONG` DNN inference into `FishnetDetector` class. Keep compile-time `#ifdef` (runtime overhead unacceptable). Class is compiled only in LVTONG builds. |
| 8.8 | Wire pipeline stages in `grab_thread_process()` | S-002 | `userpanel.cpp:947` | 5 | 4 | 3 | **3.3** | After all extractions, the loop becomes: `acquire → preprocess → detect → enhance → display → record → scan_advance`. ~50 lines. |

**Sprint 8 total estimated effort:** ~12 hours

---

## Sprint 9: UserPanel Decomposition

**Goal:** Reduce UserPanel from 200+ members to a coordination shell.
**Prerequisite:** All previous sprints. **Requires discussion with user** (user noted the extraction strategy "could be a little different").

> **This sprint's exact scope should be discussed before implementation.** The tasks below represent one possible decomposition strategy. The user may prefer different module boundaries.

| # | Task | Smell | Files | I | R | E | Priority | Notes |
|---|------|-------|-------|---|---|---|----------|-------|
| 9.1 | Extract `DeviceManager` | S-001 | `userpanel.cpp:2108-2237` | 5 | 4 | 5 | **2.0** | Owns: `init_control_port()`, device thread creation, IPTZController selection, port status. Other controllers depend on it — extract first. |
| 9.2 | Extract `CameraController` | S-001 | `userpanel.cpp` (on_ENUM/START/SHUTDOWN/GRABBING) | 4 | 4 | 4 | **2.0** | Camera enumeration, start/stop, grabbing, trigger mode. Owns `Cam* curr_cam`. |
| 9.3 | Extract `TCUController` | S-001 | `userpanel.cpp:3926-4100+` | 4 | 4 | 4 | **2.0** | TCU parameter management, delay/gatewidth/laser UI sync, slider handlers. Owns TCU signal wiring. |
| 9.4 | Extract `LensController` | S-001 | `userpanel.cpp: update_lens_params, focus_speed` | 3 | 3 | 3 | **4.0** | Lens zoom/focus control, focus speed slider, position feedback. Exposes lens data for future algorithms. |
| 9.5 | Extract `LaserController` | S-001 | `userpanel.cpp: send_laser_msg, lasercontrol.cpp` | 3 | 3 | 2 | **6.0** | Laser command formatting, type-specific logic. Absorbs relevant parts of lasercontrol.cpp dialog. |
| 9.6 | Extract `RFController` | S-001 | `userpanel.cpp: update_distance` | 2 | 2 | 2 | **4.0** | Distance display, RF→TCU auto-delay feed. |
| 9.7 | Extract `ScanController` | S-001 | `userpanel.cpp:5810-5930` | 4 | 4 | 4 | **2.0** | Scan state machine, route management, param file writing. Uses `ScanEngine` from Sprint 8. |
| 9.8 | Extract `AuxPanelManager` | S-001 | `userpanel.cpp:6084-6121` | 3 | 2 | 2 | **6.0** | Alt display switching (DATA/HIST/PTZ/ALT/ADDON/VID/YOLO), content routing. |
| 9.9 | Extract `FileIOManager` | S-001 | `userpanel.cpp:2239-2440, 5325-5490` | 3 | 3 | 3 | **4.0** | Image/video save, file browsing, drag-and-drop loading. |
| 9.10 | Flatten remaining UserPanel | S-001 | `userpanel.h/cpp` | 5 | 5 | 5 | **1.0** | After extractions: UserPanel becomes a thin shell wiring extracted components. Target: < 1500 lines. |

**Sprint 9 total estimated effort:** ~20 hours

**Extraction order within Sprint 9:**
1. DeviceManager (9.1) — other controllers depend on it
2. TCUController (9.3) — most complex, highest value
3. LensController (9.4), LaserController (9.5), RFController (9.6) — can be parallel
4. CameraController (9.2) — depends on DeviceManager
5. ScanController (9.7) — depends on TCUController + IPTZController
6. AuxPanelManager (9.8) — depends on pipeline outputs
7. FileIOManager (9.9) — independent, can be done anytime
8. Flatten (9.10) — last

---

## Execution Summary

```
Sprint  Focus                          Est. Effort  Risk   Cumulative LOC Impact
──────  ────────────────────────────   ──────────   ─────  ─────────────────────
  0     Bug fixes & safety             ~1h          NONE   Fix 6 defects
  1     Dead code removal              ~3h          NONE   −300 LOC
  2     MyWidget breakup               ~6h          LOW    −0 LOC, +6 files
  3     Magic numbers → constants      ~3h          NONE   +3 files, readability++
  4     Copy-paste elimination         ~3h          LOW    −80 LOC, −5 duplications
  5     Thread safety fixes            ~4h          MED    Fix 3 data races
  6     PTZ unification                ~8h          MED    −2 slots, +1 interface
  7     Config as truth                ~8h          HIGH   −120 LOC, −60 Pref members
  8     Pipeline extraction            ~12h         HIGH   −800 LOC from God Method
  9     UserPanel decomposition        ~20h         V.HIGH −3000+ LOC from God Class
                                       ─────
                                       ~68h total
```

### Recommended Execution Order

```
┌─────────────────────────────────────────────────────────────────┐
│                                                                   │
│  Sprint 0 ──→ Sprint 1 ──→ Sprint 3                              │
│  (bug fixes)  (dead code)  (constants)                           │
│                    │              │                                │
│                    ▼              ▼                                │
│              Sprint 2       Sprint 4                              │
│              (mywidget)     (dedup)                               │
│                    │              │                                │
│                    └──────┬───────┘                                │
│                           ▼                                       │
│                      Sprint 5                                     │
│                      (thread safety)                              │
│                           │                                       │
│                    ┌──────┴───────┐                                │
│                    ▼              ▼                                │
│              Sprint 6       Sprint 7                              │
│              (PTZ unify)    (Config truth)                        │
│                    │              │                                │
│                    └──────┬───────┘                                │
│                           ▼                                       │
│                      Sprint 8                                     │
│                      (pipeline extraction)                        │
│                           │                                       │
│                           ▼                                       │
│                     Sprint 9 ◄── REQUIRES DISCUSSION              │
│                     (UserPanel decomposition)                     │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
```

- Sprints 0–1 can be done immediately without tests
- Sprints 2–4 can run in parallel after Sprint 1
- Sprint 5 is a gate — all subsequent sprints depend on thread safety being resolved
- Sprints 6 and 7 can run in parallel
- Sprint 8 depends on both 6 and 7
- Sprint 9 depends on everything and should be planned collaboratively

---

## Risk Mitigation Strategy

### Per-Sprint Safety Protocol

1. **Before each sprint:** Run full characterization test suite (Phase 4). All tests must pass.
2. **Each extracted class:** Write its characterization test BEFORE moving code. Run after.
3. **Commit granularity:** One logical change per commit. Each commit should compile and pass tests.
4. **Rollback plan:** Each sprint is independently revertable via `git revert` on the sprint's merge commit.

### High-Risk Sprint Precautions

| Sprint | Risk | Mitigation |
|--------|------|------------|
| 6 (PTZ) | Protocol encoding change | Test wire format byte-by-byte before/after |
| 7 (Config) | Lost setting on load/save | Test JSON round-trip for every field |
| 8 (Pipeline) | Image output regression | Pixel-by-pixel comparison of pipeline output before/after extraction |
| 9 (UserPanel) | Everything | Feature-by-feature smoke test against Phase 1 inventory |

### Feature Freeze During Refactoring

- No new features should be added to modules being refactored
- Bug fixes in active sprint modules should go through the same test protocol
- New features targeting extracted modules can proceed on separate branches

---

## Decision Points — Resolved

| # | Question | Decision | Sprint |
|---|----------|----------|--------|
| D-1 | Fix `get(GATE_WIDTH_A)` bug? | **YES — Fix it.** Different TCU types have different precisions; the switch must be reachable. | Sprint 0, task 0.6 |
| D-2 | Remove or complete AutoScan? | **Remove now.** Rebuild after refactor against new interfaces (ScanController, DeviceManager). ScanPresetData struct preserved. | Sprint 1, task 1.4 |
| D-3 | LVTONG runtime or compile-time? | **Keep compile-time** (`#ifdef`). Processing time is critical; runtime dispatch adds overhead. | Sprint 8, task 8.7 |
| D-4 | UserPanel decomposition strategy? | **Resolved.** Extract: DeviceManager, CameraController, TCUController, LensController, LaserController, RFController, ScanController, AuxPanelManager, FileIOManager. See Sprint 9 and Phase 6 for details. | Sprint 9 |
| D-5 | ControlPort for USBCAN/UDPPTZ? | **Extract new `DevicePort` base** with virtual `communicate()`. USBCAN/UDPPTZ inherit DevicePort (not ControlPort). No serial/TCP baggage on CAN/UDP classes. | Sprint 6 |

---

**STOP — Phase 5 Complete**
Please review the refactoring roadmap. When you're ready, confirm to proceed to Phase 6 (Target Architecture Blueprint).
