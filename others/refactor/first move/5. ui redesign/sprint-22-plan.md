# Sprint 22 Plan: Legacy Dialog Removal

Carry-over work from Sprint 21.1, deferred because the coupling survey revealed deeper integration than the original sprint-plan anticipated. This document is a complete inventory — it should be possible to execute Sprint 22 without going back to re-discover any of these.

## Goal

Remove the `Preferences` and `ScanConfig` `QDialog` classes (currently invisible shells, kept alive only for state). Migrate their data + signal contracts into proper homes (`Config`, `ScanController`, `TCUController`, or new `ScanParams` struct).

## Why this is non-trivial

The dialogs are not just UI; they are **the app's persistent state holder**. Deleting them naively will:
- invalidate ~30 widget pointers used as configuration state in `userpanel.cpp`
- break preset serialization (reads/writes 12+ `scan_config` members)
- break the pipeline (`ProcessingParams::scan_config` is a live pointer)
- break ~24 signal connections in `topbar.cpp`

## Pre-flight check (verify these before starting Sprint 22)

The exact line numbers below reflect the codebase at end of Sprint 21. They may shift; use the expression as the canonical reference and `grep` for it before editing.

---

## Part 1: ScanConfig removal (smaller, do first)

### 1A. Data members to migrate

`ScanConfig` holds 27 plain-data members ([scanconfig.h](src/visual/scanconfig.h):44–70). Plan: introduce `struct ScanParams` (new file e.g. `src/model/scanparams.h`) holding these fields. Owned by `ScanController` (currently it just holds a pointer to the dialog).

Members:
- TCU side: `dist_ns`, `hz_unit`, `base_unit`, `starting_delay`, `ending_delay`, `starting_gw`, `ending_gw`, `step_size_delay`, `step_size_gw`, `frame_count`, `rep_freq`
- Capture/record: `capture_scan_ori`, `capture_scan_res`, `record_scan_ori`, `record_scan_res`
- PTZ: `starting_h`, `ending_h`, `starting_v`, `ending_v`, `step_h`, `step_v`, `count_h`, `count_v`, `ptz_direction`, `ptz_wait_time`, `num_single_pos`

### 1B. Methods on ScanConfig that need replacement

- `ScanConfig::data_exchange(bool read)` — bidirectional widget↔state sync. Replacement: `ScanOverlay::loadFromState(const ScanParams&)` + `ScanOverlay::commitToState(ScanParams&)`. Used at [userpanel.cpp:3370](src/visual/userpanel.cpp#L3370).
- `ScanConfig::get_ptz_route()` / `get_tcu_route()` — derived data. Move to `ScanController` (or free functions on `ScanParams`). Used at [scancontroller.cpp:55–57](src/controller/scancontroller.cpp).
- `ScanConfig::setup_hz(int)` / `setup_unit(int)` — change unit display. Move to `ScanOverlay` (it owns the widgets now).

### 1C. ScanConfig data-member call sites (each must be re-pointed)

| File:Line | Expression | Migrates to |
|---|---|---|
| `userpanel.cpp:3302–3305` | preset SAVE writing 12 scan_config members | `m_scan_ctrl->params()` |
| `userpanel.cpp:3360–3369` | preset LOAD reading 12 scan_config members | `m_scan_ctrl->params()` |
| `userpanel.cpp:3370` | `scan_config->data_exchange(false)` | `m_scan_overlay->loadFromState(...)` |
| `scancontroller.cpp:55–61` | scan execution reading `step_size_delay`, `rep_freq`, `starting_delay`, `capture_*` | `m_params.X` (internal) |
| `scancontroller.cpp:80` | debug output of scan_config members | `m_params` |
| `scancontroller.cpp:114–119` | `m_scan_config->isVisible/show/hide/raise` | DELETE — overlay handles visibility |
| `tcucontroller.cpp:344` | `m_scan_config->dist_ns = dist_ns` | `m_scan_ctrl->set_dist_ns(...)` or move to TCUController itself |
| `controlportthread.cpp:388` | `scan_config->rep_freq` | param-pointer or callback |
| `pipelineprocessor.cpp:689` | `m_s.scan_config->num_single_pos` | `m_s.scan_params->num_single_pos` |
| `pipelineprocessor.cpp:698` | `m_s.scan_config->ptz_wait_time` | same |
| `pipelineprocessor.cpp:856` | `m_s.scan_config->capture_scan_ori` | same |
| `pipelineprocessor.cpp:864` | `m_s.scan_config->capture_scan_res` | same |

### 1D. Pipeline coupling

`ProcessingParams::SharedState::scan_config` ([pipelineprocessor.h:118](src/pipeline/pipelineprocessor.h#L118)) → rename to `ScanParams* scan_params`. Set up in [userpanel.cpp:1119](src/visual/userpanel.cpp#L1119) where `ss.scan_config = scan_config` is currently assigned. Change to `ss.scan_params = &m_scan_ctrl->params()`.

### 1E. ScanConfig widget accesses (already migrated in S19 — confirm)

All 27 widgets accessed via `scan_config->ui->*` in [userpanel.cpp:626–882](src/visual/userpanel.cpp) were re-parented into `ScanOverlay` in Sprint 19. They survive dialog deletion. Sprint 22 needs: change the access path from `scan_config->ui->X` to `m_scan_overlay->X()` getters (or store the pointers directly on `ScanOverlay`).

### 1F. Lifecycle calls to remove

- `topbar.cpp:157` — `new ScanConfig()` instantiation
- `topbar.h:68` — `ScanConfig* scan_config` member
- `userpanel.h:458` — `ScanConfig* scan_config` member
- `userpanel.cpp:284` — `scan_config = ui->TITLE->scan_config`
- `userpanel.cpp:1977` — `scan_config->reject()` in closeEvent

---

## Part 2: Preferences removal (larger, more risk)

### 2A. Re-parented groups (children survive)

Sprint 18 re-parented these 5 groups into `SetupOverlay`. Their children survive `Preferences` deletion:
- `SAVE_LOAD_GRP` → SetupOverlay row 0
- `SERIAL_PORTS_GRP` → SetupOverlay row 0
- `TCU_GRP` → SetupOverlay row 0
- `DEVICES_GRP` → SetupOverlay row 1
- `IMG_PROC_GRP` → SetupOverlay row 2

Important nested groups (also survive because their parent group was re-parented):
- `PS_CONFIG_GRP` (nested in TCU_GRP)
- `ACCUMULATIVE_GRP`, `ADAPTIVE_GRP`, `DCP_GRP`, `IMAGE_3D_GRP`, `FISHNET`, `ECC_GRP` (all nested in IMG_PROC_GRP)

### 2B. Orphaned widgets in Preferences (need attention)

`PREFERENCE_TAB::SCAN_TAB` — the legacy scan tab inside Preferences. Children: `START_DELAY_EDIT_U/N`, `END_DELAY_EDIT_U/N`, `STEP_SIZE_DELAY_EDIT`, `START_GW_EDIT_U/N`, `END_GW_EDIT_U/N`, `STEP_SIZE_GW_EDIT`, `REP_FREQ_EDIT`, `FRAME_COUNT_EDIT`, `SAVE_SCAN_ORI_CHK`, `SAVE_SCAN_RES_CHK`, `RECONSTRUCTION_CHK`, plus unit labels.

**Migration:** these are duplicates of `ScanConfig` widgets that were never wired to the rest of the app post-S16 refactor. Safe to **delete** from `preferences.ui` and remove any associated code.

### 2C. Preferences widget accesses in userpanel.cpp (~30, all SURVIVE)

Every `pref->ui->X` reference in [userpanel.cpp] points to a widget inside one of the 5 re-parented groups. Migration: change `pref->ui->X` to `m_setup_overlay->X()` getters (add getters to SetupOverlay), or store widget pointers directly on UserPanel.

Affected expressions (verify each by grep before editing — line numbers may drift):
- `pref->ui->LOWER_3D_THRESH_EDT` — IMAGE_3D_GRP (in IMG_PROC_GRP)
- `pref->ui->CUSTOM_3D_DELAY_EDT` / `CUSTOM_3D_GW_EDT` — IMAGE_3D_GRP
- `pref->ui->CUSTOM_INFO_EDT` — SAVE_LOAD_GRP
- `pref->ui->REFRESH_AVAILABLE_PORTS_BTN` — SERIAL_PORTS_GRP
- `pref->ui->TCP_SERVER_CHK` / `TCP_SERVER_IP_EDIT` — SERIAL_PORTS_GRP
- `pref->ui->PS_STEPPING_EDT` / `MAX_PS_STEP_EDT` — PS_CONFIG_GRP
- `pref->ui->LASER_ENABLE_CHK` — TCU_GRP
- `pref->ui->MODEL_LIST` — FISHNET
- `pref->ui->PTZ_TYPE_LIST` — DEVICES_GRP
- `pref->ui->AUTO_MCP_CHK` — TCU_GRP
- `pref->ui->UNIT_LIST` — TCU_GRP
- `pref->ui->MAX_DIST_EDT` / `MAX_DOV_EDT` — TCU_GRP
- `pref->ui->UNDERWATER_CHK` — DEVICES_GRP
- `pref->ui->SHARE_CHK` — SERIAL_PORTS_GRP
- `pref->ui->IMG_FORMAT_LST` — SAVE_LOAD_GRP
- `pref->ui->CAMERALINK_CHK` — DEVICES_GRP
- `pref->ui->CONSECUTIVE_CAPTURE_CHK` — SAVE_LOAD_GRP

### 2D. Preferences data members to migrate

`Preferences` holds 6 plain-data members ([preferences.h:96–103](src/visual/preferences.h)):

| Member | Used at | Migrate to |
|---|---|---|
| `split` | `userpanel.cpp:2886` | `Config::display::split` or directly read at point-of-use |
| `cameralink` | `userpanel.cpp:2072,2083` | `Config::device::cameralink_enabled` |
| `port_idx` | internal serial selection | `DeviceManager` state |
| `use_tcp` | internal | `Config::network::use_tcp_server` |
| `dist_ns` | `userpanel.cpp:1217` (set from TCU) | `TCUController` member (already has `get_dist_ns()`) |
| `laser_grp` | QButtonGroup for laser checkboxes | rebuild on `SetupOverlay` or `TCUController` |

### 2E. Methods called on Preferences

| Call | Replacement |
|---|---|
| `pref->init()` | DELETE (was UI setup) |
| `pref->set_config(config)` | DELETE (config now passed to relevant controllers) |
| `pref->set_ptz_type_enabled(bool)` | move to `DeviceManager` or `SetupOverlay` |
| `pref->display_baudrate(idx, baud)` | move to `DeviceManager` |
| `pref->enable_ip_editing(...)` | move to `DeviceManager` |
| `pref->config_ip(...)` | move to `DeviceManager` |
| `pref->data_exchange(bool)` | replaced by SetupOverlay state sync |
| `pref->ui->retranslateUi(pref)` | DELETE (overlay handles its own retranslate) |
| `pref->show()` / `pref->raise()` (userpanel.cpp:3771–3772, keyboard shortcut) | DELETE — switch the SETUP mode tab instead |
| `pref->reject()` (userpanel.cpp:1975) | DELETE |

### 2F. Signal connections to re-route

`Preferences` declares 24 signals ([preferences.h:52–86](src/visual/preferences.h)). These are connected in [topbar.cpp:194–217](src/widgets/topbar/topbar.cpp). Each must be re-routed:

| Signal | New source |
|---|---|
| `search_for_devices` | DEVICES_GRP button → `DeviceManager` slot direct |
| `query_dev_ip`, `set_dev_ip`, `change_pixel_format`, `rotate_image`, `device_underwater` | DEVICES_GRP → `DeviceManager` |
| `get_port_info`, `change_baudrate`, `set_tcp_status`, `share_tcu_port`, `com_write` | SERIAL_PORTS_GRP → `DeviceManager` |
| `tcu_type_changed`, `set_auto_rep_freq`, `set_auto_mcp`, `set_ab_lock`, `rep_freq_unit_changed`, `base_unit_changed`, `max_dist_changed`, `max_dov_changed`, `max_laser_changed`, `delay_offset_changed`, `gate_width_offset_changed`, `laser_offset_changed`, `laser_toggled`, `ps_config_updated` | TCU_GRP → `TCUController` |
| `lower_3d_thresh_updated`, `query_tcu_param` | IMG_PROC_GRP / TCU_GRP |

The cleanest pattern: have `SetupOverlay` connect each widget directly to its target controller (no Preferences re-emit chain).

### 2G. Lifecycle calls to remove

- `topbar.cpp:156` — `new Preferences()`
- `topbar.cpp:190–192` — `pref->setShortcut(...)` + the `connect(pref, triggered, ...)` action wiring
- `topbar.h:67` — `Preferences* preferences` member
- `userpanel.h:450` — `Preferences* pref` member
- `userpanel.cpp:282–283` — `pref = ui->TITLE->preferences; pref->init()`

---

## Part 3: Header & file cleanup

### 3A. Files to DELETE (after parts 1+2 done)

- `src/visual/preferences.h`, `preferences.cpp`, `preferences.ui`
- `src/visual/scanconfig.h`, `scanconfig.cpp`, `scanconfig.ui`
- Auto-generated `ui_preferences.h` and `ui_scanconfig.h` will stop being generated when the `.ui` files are removed from the project.

### 3B. Files to update `#include`s in

- `src/visual/userpanel.cpp` — remove `#include "ui_preferences.h"`, `#include "ui_scanconfig.h"`
- `src/visual/userpanel.h` — remove `#include "visual/preferences.h"`, `#include "visual/scanconfig.h"` (members are gone)
- `src/widgets/topbar/topbar.{h,cpp}` — remove both includes; remove `Preferences*`/`ScanConfig*` members
- `src/controller/scancontroller.{h,cpp}` — remove `#include "visual/scanconfig.h"`; replace `ScanConfig* m_scan_config` member with `ScanParams m_params`
- `src/controller/tcucontroller.{h,cpp}` — remove include; replace `m_scan_config->dist_ns` with own member
- `src/pipeline/pipelineprocessor.h` — replace `ScanConfig* scan_config` field in SharedState with `ScanParams* scan_params`
- `src/thread/controlportthread.cpp` — re-target `scan_config->rep_freq` access

### 3C. CMakeLists.txt

Remove the lines listing `src/visual/preferences.{h,cpp,ui}` and `src/visual/scanconfig.{h,cpp,ui}` from `set(SOURCE_FILES ...)`.

---

## Part 4: Suggested execution order

1. **Create `ScanParams` struct** — new file, no behavior change yet.
2. **Add `params()` accessor on `ScanController`** that returns a `ScanParams&` — initially copies from `scan_config` so behavior is identical.
3. **Repoint pipeline coupling** (`m_s.scan_config` → `m_s.scan_params`) — verify scan still runs.
4. **Repoint preset save/load** — verify round-trip with old preset files.
5. **Repoint other `scan_config->X` reads** in scancontroller / tcucontroller / controlportthread.
6. **Add SetupOverlay getters** for the 30+ Preferences widgets — purely additive.
7. **Repoint `pref->ui->X` reads** to use the getters.
8. **Re-route Preferences signals** from `topbar.cpp` to direct widget→controller connections.
9. **Migrate Preferences data members** (`split`, `cameralink`, etc.) to Config / controllers.
10. **Delete dialog instantiations + class files + ui files**.
11. **Remove includes**, update `CMakeLists.txt`.
12. **Test**: scan run, preset save/load, theme toggle, preset apply, language switch.

## Part 5: High-risk areas to test

- **Preset save/load** — binary serialization reads ~12 scan_config members per preset
- **Scan execution** — `get_ptz_route()` / `get_tcu_route()` derived from data members
- **Pipeline capture/record gating** — `capture_scan_ori/res` flags read every frame
- **TCU `dist_ns` propagation** — set in many places, read in scan_step calculation
- **i18n** — `retranslateUi` chain across SetupOverlay's re-parented widgets

---

## Estimated effort

Multi-day refactor. Worth its own sprint cycle (Sprint 22) rather than squeezing into a polish sprint. The work is mechanical once the migration types (`ScanParams`, SetupOverlay getters) are designed; the volume of edits is ~50 call-sites.
