# Sprint 9 Progress: UserPanel Decomposition

## Prerequisite: Sprints 0-8
Status: SATISFIED

All previous sprints complete. Sprint 8 extracted grab_thread_process() into 10 pipeline methods. Sprint 6 unified PTZ via IPTZController. Sprint 7 made Config the single source of truth.

## Analysis

### Current architecture

`src/visual/userpanel.cpp` (6504 lines, ~110 private member variables) is a God Class handling everything: camera lifecycle, TCU/Lens/Laser/PTZ device control, scanning, file I/O, display management, and the image pipeline.

### The problem

1. **Size**: 6504 lines with ~110 private member variables
2. **Responsibility**: Handles 8+ distinct concerns (camera, TCU, lens, laser, PTZ, scan, file I/O, display)
3. **Coupling**: Every concern accesses every other concern's state directly via `this->`
4. **Constructor**: 731 lines creating all objects, connecting all signals
5. **Member variable sprawl**: 14 port/thread objects, 13 TCU-related fields, 12 scan fields, 10 camera fields, etc.

### Approach: controller extraction

Extract QObject controller classes into `src/controller/`. Each controller:
- Owns its member variables and methods (moved from UserPanel)
- Receives UI widget pointers via `bind_ui()` — widgets stay in the `.ui` file
- Communicates cross-controller via signals/slots
- Takes `Config*` and `DeviceManager*` in constructor as needed

Pipeline methods (Sprint 8) stay in UserPanel for now — tightly coupled to img_mem[], GrabThread, and image_mutex[].

### Design decisions

1. RFController kept as separate class (planned to grow with multiple RF hardware versions)
2. keyPressEvent (332 lines) NOT extracted — dispatch table that naturally slims once controllers exist
3. VID camera functions moved to DeviceManager (all route through p_udpptz)
4. Joystick handlers stay in UserPanel as thin dispatch wrappers
5. Constructor signal connections absorbed by each controller's `bind_ui()`

## Task breakdown

### Task 9.0: Create controller directory and CMake infrastructure
Status: DONE

- Create `src/controller/` directory
- Add controller source files to CMakeLists.txt incrementally (one pair per task)
- `src/` is already in include_directories, so `#include "controller/xxx.h"` will resolve

Files: `CMakeLists.txt`

### Task 9.1: Extract DeviceManager
Status: DONE

Create `src/controller/devicemanager.h/cpp`. Foundation — all other controllers depend on it.

Member variables moving:
- 7 port objects (p_tcu, p_lens, p_laser, p_ptz, p_rf, p_usbcan, p_udpptz)
- IPTZController* active_ptz
- 7 QThread* objects (th_tcu..th_udpptz)
- serial_port[5], serial_port_connected[5], tcp_port[5], use_tcp[5], share_serial_port
- com_label[5], com_edit[5]
- ptz_grp, vid_camera_grp, ptz_speed, angle_h, angle_v

Methods moving:
- init_control_port() → DeviceManager::init()
- connect_port_edit(), setup_serial_port(), setup_tcp_port()
- update_port_status() (both overloads), display_port_info(), update_ptz_status()
- set_baudrate(), set_tcp_status_on_port(), set_tcu_as_shared_port(), com_write_data()
- connect_to_serial_server_tcp(), disconnect_from_serial_server_tcp()
- PTZ button handlers, VID camera handlers
- update_ptz_angle(), update_ptz_params()
- Destructor cleanup for threads/ports

Constructor code moving: PTZ/VID button groups, PTZ speed slider, serial/TCP creation, COM label/edit init

Depends on: 9.0
Files: `src/controller/devicemanager.h` (NEW), `src/controller/devicemanager.cpp` (NEW), `src/visual/userpanel.h`, `src/visual/userpanel.cpp`, `CMakeLists.txt`

### Task 9.2: Extract TCUController
Status: DONE

Create `src/controller/tcucontroller.h/cpp`. Most complex controller.

Member variables moved (14): stepping, hz_unit, base_unit, rep_freq, laser_width, delay_dist, depth_of_view, mcp_max, aliasing_mode, aliasing_level, c, dist_ns, auto_mcp, frame_a_3d, distance

Methods moved (17 bodies + 2 private helpers):
- update_tcu_params, change_mcp, change_delay, change_gatewidth
- update_delay, update_gate_width, update_laser_width, update_current
- setup_hz, setup_stepping, setup_max_dist, setup_max_dov
- update_delay_offset, update_gate_width_offset, update_laser_offset
- setup_laser, update_light_speed, set_tcu_type, update_ps_config
- set_auto_mcp, set_distance_set, convert_to_send_tcu
- on_SWITCH_TCU_UI_BTN_clicked, on_SIMPLE_LASER_CHK_clicked, on_AUTO_MCP_CHK_clicked
- split_value_by_unit, update_tcu_param_pos (private)

Design: on_DIST_BTN_clicked stays in UserPanel (uses communicate_display + QInputDialog), calls m_tcu_ctrl->apply_distance() for TCU parameter logic. UserPanel retains inline delegations for all moved slots (old-style SIGNAL/SLOT compatibility). Pipeline methods access TCU state via m_tcu_ctrl->get_*() accessors.

Signal wiring: DeviceManager::tcu_param_updated → TCUController::update_tcu_params (direct). TCUController::send_double_tcu_msg/send_uint_tcu_msg/send_laser_msg → DeviceManager (forwarded).

CMake: added AUTOUIC_SEARCH_PATHS on GLI_params target for cross-directory ui file resolution.

Depends on: 9.1
Files: `src/controller/tcucontroller.h` (NEW), `src/controller/tcucontroller.cpp` (NEW), `src/visual/userpanel.h`, `src/visual/userpanel.cpp`, `CMakeLists.txt`

### Task 9.3: Extract LensController
Status: DONE

Create `src/controller/lenscontroller.h/cpp`.

Member variables moved (5): zoom, focus, focus_direction, clarity[3], lens_adjust_ongoing

Methods moved (24): 6 button pressed handlers (ZOOM_IN/OUT, FOCUS_FAR/NEAR, RADIUS_INC/DEC), 6 button released handlers, 4 IRIS handlers (OPEN/CLOSE pressed/released), on_GET_LENS_PARAM_BTN_clicked, on_AUTO_FOCUS_BTN_clicked, change_focus_speed, update_lens_params, lens_stop, focus_far, focus_near, set_zoom_from_ui (was set_zoom), set_focus_from_ui (was set_focus)

Design: LensController receives `bool* simple_ui` and `uchar* lang` pointers via init() for button released text localization. Focus speed slider SIGNAL/SLOT connection stays in constructor (routes through UserPanel inline delegation). Joystick handlers stay in UserPanel as thin wrappers using m_lens_ctrl->get/set_lens_adjust_ongoing(). send_lens_msg signal forwarded from LensController to DeviceManager.

Depends on: 9.1
Files: `src/controller/lenscontroller.h` (NEW), `src/controller/lenscontroller.cpp` (NEW), `src/visual/userpanel.h`, `src/visual/userpanel.cpp`, `CMakeLists.txt`

### Task 9.4: Extract LaserController
Status: DONE

Created `src/controller/lasercontroller.h/cpp`.

Member variables moved (1): curr_laser_idx
Dead variables removed (2): laser_on (real value in Config), multi_laser_lenses (never read)

Methods moved (7): on_LASER_BTN_clicked, on_FIRE_LASER_BTN_clicked, start_laser, init_laser, laser_preset_reached, set_laser_preset_target (private), goto_laser_preset

Design: LaserController receives DeviceManager*, LensController* (for lens_stop in preset system), TCUController* (for update_current in init_laser). send_laser_msg signal forwarded to DeviceManager. on_DUAL_LIGHT_BTN_clicked stays in UserPanel (plugin system, not laser-specific).

Depends on: 9.1, 9.3
Files: `src/controller/lasercontroller.h` (NEW), `src/controller/lasercontroller.cpp` (NEW), `src/visual/userpanel.h`, `src/visual/userpanel.cpp`, `CMakeLists.txt`

### Task 9.5: Extract RFController
Status: DONE

Created `src/controller/rfcontroller.h/cpp`.

Member variables moved (0): `distance` was already moved to TCUController in Task 9.2.

Methods moved (1): update_distance (updates DISTANCE label from rangefinder reading)

Design: Minimal controller. `on_DIST_BTN_clicked()` stays in UserPanel because it uses `communicate_display()` (UserPanel serial I/O helper) and `QInputDialog` (needs parent QWidget). DeviceManager::distance_updated signal now connects directly to RFController::update_distance instead of through UserPanel.

Depends on: 9.1
Files: `src/controller/rfcontroller.h` (NEW), `src/controller/rfcontroller.cpp` (NEW), `src/visual/userpanel.h`, `src/visual/userpanel.cpp`, `CMakeLists.txt`

### Task 9.6: Extract CameraController
Status: SKIPPED

Architectural decision: camera state (device_on, start_grabbing, trigger_mode_on, etc.) is inseparable from the grab thread lifecycle owned by UserPanel. Extracting would create split-brain ownership — CameraController owns the state but UserPanel controls the transitions (on_START_GRABBING sets start_grabbing=true, shut_down sets device_on=false). The Cam class hierarchy (MvCam, EbusCam, EuresysCam) already serves as the SDK controller layer. Adding a UI wrapper on top creates indirection without meaningful decoupling, splitting camera logic across two files.

Camera methods and state remain in UserPanel. The real fix is a future sprint refactoring GrabThread to be independent of UserPanel.

### Task 9.7: Extract ScanController
Status: DONE

Created `src/controller/scancontroller.h/cpp`.

Member variables moved (13): auto_scan_mode, scan, scan_distance, scan_step, scan_stopping_delay, scan_name, scan_3d, scan_sum, scan_idx, scan_ptz_route, scan_tcu_route, scan_ptz_idx, scan_tcu_idx

Methods moved (6): on_SCAN_BUTTON_clicked, on_CONTINUE_SCAN_BUTTON_clicked, on_RESTART_SCAN_BUTTON_clicked, on_SCAN_CONFIG_BTN_clicked, enable_scan_options, auto_scan_for_target

Design: ScanController receives TCUController*, DeviceManager*, and pointers to UserPanel's save_location, w[0], h[0] via init(). filter_scan stays in UserPanel (uses img_mem[0]). advance_scan (pipeline method) stays in UserPanel and accesses scan state via getters/setters. update_scan signal internal to ScanController (wired in init()). finish_scan_signal emitted by UserPanel (from advance_scan in grab thread) connected to ScanController::on_SCAN_BUTTON_clicked via QueuedConnection.

Depends on: 9.1, 9.2
Files: `src/controller/scancontroller.h` (NEW), `src/controller/scancontroller.cpp` (NEW), `src/visual/userpanel.h`, `src/visual/userpanel.cpp`, `CMakeLists.txt`

Pipeline coupling: advance_scan() (Sprint 8) stays in UserPanel but calls ScanController getters/mutators

Depends on: 9.1, 9.2, 9.5
Files: `src/controller/scancontroller.h` (NEW), `src/controller/scancontroller.cpp` (NEW), `src/visual/userpanel.h`, `src/visual/userpanel.cpp`, `CMakeLists.txt`

### Task 9.8: Extract AuxPanelManager
Status: DONE

Created `src/controller/auxpanelmanager.h/cpp`.

Member variables moved (2): alt_display_option, display_grp
Member variables kept in UserPanel: alt_ctrl_grp (used by alt_display_control() which stays in UserPanel)

Methods moved (6): on_MISC_RADIO_1/2/3_clicked(), on_MISC_OPTION_1/2/3_currentIndexChanged()

Constructor code moved: MISC_OPTION combo setup (addItems, setCurrentIndex, selected() signals, font), display_grp button group setup (exclusive radio, MISC_DISPLAY index)

Design: AuxPanelManager::init() absorbs ~40 lines of constructor code. alt_display_control() stays in UserPanel — too coupled to grab thread state, video writers, and image buffers. Pipeline method render_and_display() accesses alt_display_option via m_aux_panel->get_alt_display_option().

Depends on: 9.0
Files: `src/controller/auxpanelmanager.h` (NEW), `src/controller/auxpanelmanager.cpp` (NEW), `src/visual/userpanel.h`, `src/visual/userpanel.cpp`, `CMakeLists.txt`

### Task 9.9: Rewire constructor, update data_exchange and sync functions
Status: DONE (completed incrementally during 9.1-9.8)

All rewiring was done as part of each controller extraction task:
1. Constructor creates controllers, calls init(), wires cross-controller signals — done per task
2. data_exchange() — already uses m_tcu_ctrl getters/setters (task 9.2)
3. syncConfigToPreferences/syncPreferencesToConfig — already uses m_device_mgr->com_edit_widget(), udpptz() (task 9.1)
4. generate_preset_data/apply_preset — already uses m_device_mgr->tcu/lens/ptz() (task 9.1)
5. convert_write/convert_read — no moved variables referenced
6. keyPressEvent — all references delegate via controllers (tasks 9.1-9.5)
7. Joystick handlers — joystick button state is local UI state (stays in UserPanel appropriately)
8. Destructor — DeviceManager handles thread/port cleanup (task 9.1)

No additional changes required.

Depends on: 9.1-9.8
Files: no changes needed

### Task 9.10: Final cleanup and verification
Status: DONE

1. All moved member variables already removed from UserPanel (done per task)
2. All moved method declarations replaced with inline delegations (done per task)
3. Cleaned up includes: removed 7 redundant port headers from userpanel.h (already included via controller/devicemanager.h)
4. Removed "moved to" marker comments from userpanel.h (12 comment blocks)
5. Verified CMakeLists.txt has all 14 controller source files (7 controllers × 2; CameraController skipped)
6. Verified all files in src/controller/ match CMakeLists.txt entries

Line counts:
- userpanel.cpp: 6504 → 4771 (−1733 lines, 26.6% reduction)
- userpanel.h: reduced by ~25 lines (marker comments removed)
- Controllers total: 1688 lines across 7 .cpp files

Note: target of <1500 lines was based on extracting CameraController (~300 lines of camera methods) which was skipped. Camera methods, pipeline methods (Sprint 8), keyPressEvent, file I/O, and display methods remain in UserPanel as designed.

Depends on: 9.9
Files: `src/visual/userpanel.h`, `CMakeLists.txt`

## Execution order

```
9.0  CMake setup ──→ 9.1  DeviceManager ──→ 9.2  TCUController
                     │                      │
                     ├──→ 9.3  LensController ──→ 9.4  LaserController
                     │
                     ├──→ 9.5  RFController
                     │
                     ├──→ 9.6  CameraController
                     │
                     └──→ 9.7  ScanController (also needs 9.2, 9.5)

9.0 ──→ 9.8  AuxPanelManager (independent)

ALL ──→ 9.9  Rewire ──→ 9.10  Cleanup
```

Sequential: 9.0 → 9.1 → 9.2 → 9.3 → 9.4 → 9.5 → 9.6 → 9.7 → 9.8 → 9.9 → 9.10

## Summary of files to create/modify

| File | Action |
|------|--------|
| `src/controller/devicemanager.h/cpp` | NEW |
| `src/controller/tcucontroller.h/cpp` | NEW |
| `src/controller/lenscontroller.h/cpp` | NEW |
| `src/controller/lasercontroller.h/cpp` | NEW |
| `src/controller/rfcontroller.h/cpp` | NEW |
| `src/controller/cameracontroller.h/cpp` | NEW |
| `src/controller/scancontroller.h/cpp` | NEW |
| `src/controller/auxpanelmanager.h/cpp` | NEW |
| `src/visual/userpanel.h` | MODIFY (remove moved members/methods) |
| `src/visual/userpanel.cpp` | MODIFY (extract ~3000 lines) |
| `CMakeLists.txt` | MODIFY (add 16 new source files) |

## Risks

- **DeviceManager thread lifecycle** (HIGH): 7 threads with moveToThread pattern. Must preserve exact init/destroy order.
- **TCU state accessed from pipeline** (MEDIUM): Pipeline methods use delay_dist, depth_of_view. Expose via getters on TCUController.
- **Scan state race from grab thread** (MEDIUM): Pre-existing issue from Sprint 8. Document for future mutex addition.
- **Signal wiring order** (MEDIUM): Build after each task; test incrementally.
- **Camera start creates GrabThread** (MEDIUM): CameraController emits signal, UserPanel handles thread creation.

## Bug fixes during Sprint 9

- 9.1: AUTOUIC could not find user_panel.ui for source files in src/controller/. Fixed by adding `AUTOUIC_SEARCH_PATHS` property on GLI_params target pointing to src/visual/.
- 9.1: devicemanager.cpp lambdas captured local `ui` parameter instead of `m_ui` member. Fixed 3 lambda bodies.
- 9.1: devicemanager.cpp needed `#include "ui_preferences.h"` for `m_pref->ui->` access.
