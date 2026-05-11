# Sprint 22 Progress: Legacy Dialog Removal

## Prerequisite: Sprint 21
Status: SATISFIED

Sprint 21 closed the UI polish work; the two legacy `QDialog`s (`Preferences`, `ScanConfig`) remained alive as invisible state holders. The exhaustive coupling inventory in [sprint-22-plan.md](sprint-22-plan.md) drove this sprint.

## Branch
All Sprint 22 work stays on `refactor`.

## Strategy

Plan's recommended order: smaller `ScanConfig` first, then `Preferences`. Sprint 22 deletes both classes in a single commit.

Sliced into 9 tasks:

- 22.1 Create `ScanParams` struct (new home for `ScanConfig` data fields)
- 22.2 Wire `ScanController` to own `ScanParams`; expose `params()` accessor
- 22.3 Repoint pipeline coupling (`ProcessingParams::scan_config` -> `scan_params`)
- 22.4 Repoint preset save/load + all other `scan_config->X` data reads
- 22.5 `ScanOverlay` takes over `ScanConfig`'s widget-sync slots; `ScanConfig` class deleted
- 22.6 `SetupOverlay` takes over `Preferences`'s widget setup, signals, `data_exchange`, helper methods
- 22.7 Drop `Preferences` member, instantiation, includes (topbar + userpanel); reroute 24 signal connects
- 22.8 Remove legacy source files from CMakeLists + disk
- 22.9 Manual regression pass

## Out of scope

- New features
- Light theme parity
- `tr()` wrapping / i18n sweep (separate focused patch)

---

## Task 22.1: Create `ScanParams` struct
Status: DONE

[src/pipeline/scanparams.h](../../../../src/pipeline/scanparams.h) + `.cpp` — plain-data struct with all 27 fields from `ScanConfig` (same defaults the legacy constructor used). Two free functions `get_ptz_route(const ScanParams&)` and `get_tcu_route(const ScanParams&)` ported verbatim from `ScanConfig::get_*_route()`. Registered in [CMakeLists.txt](../../../../CMakeLists.txt) alongside `processingparams.h`.

---

## Task 22.2: `ScanController` owns `ScanParams`
Status: DONE

[scancontroller.h](../../../../src/controller/scancontroller.h) gains `ScanParams m_params` + `params()` accessors (const / mutable). Scan logic (`on_SCAN_BUTTON_clicked`, route building, rep-freq setup, capture flags, param-file output) reads from `m_params` instead of the legacy dialog.

---

## Task 22.3: Pipeline repoint
Status: DONE

`ProcessingParams::SharedState::scan_config` (`ScanConfig*`) -> `scan_params` (`ScanParams*`). 4 access sites in [pipelineprocessor.cpp](../../../../src/pipeline/pipelineprocessor.cpp) repointed; include swapped from `visual/scanconfig.h` to `pipeline/scanparams.h`. `userpanel.cpp` SharedState assignment now reads `&m_scan_ctrl->params()`.

---

## Task 22.4: Preset save/load + scan_config data reads
Status: DONE

- **Preset SAVE** ([userpanel.cpp](../../../../src/visual/userpanel.cpp)) reads from `ScanController::params()`. Output binary format unchanged.
- **Preset LOAD** deserializes directly into `ScanController::params()`, then calls `ScanOverlay::loadFromParams()` to push the new values to the widgets.
- **`TCUController`** no longer holds a `ScanConfig*`; `init(Aliasing*)` signature simplified. `dist_ns_changed` signal propagates to `ScanController::params().dist_ns` (and `SetupOverlay::set_dist_ns`) via the lambda in userpanel.cpp.
- **`TCUThread` (dead code)** in [controlportthread.h/cpp](../../../../src/thread/controlportthread.h) — `ScanConfig*` member -> `ScanParams*` to keep the file compiling.

---

## Task 22.5: `ScanOverlay` takes over widget sync; `ScanConfig` deleted
Status: DONE

[scanoverlay.h](../../../../src/widgets/overlays/scanoverlay.h) gains:
- `Widgets` struct (~46 widget pointers cached after hand-creation in userpanel.cpp)
- `bind(ScanController*, const Widgets&)`, `loadFromParams()`, `setup_hz()`, `setup_unit()`
- Slot ports of `update_scan_tcu`, `update_scan_ptz`, checkbox + radio + wait/pos handlers, with the unit-mode (ns / μs / m) toggling and derivation rules (`frame_count = (ending - starting) / step + 1`, etc.)

`userpanel.cpp` hand-creates the ~40 SCAN-card widgets (`QLineEdit`, `QCheckBox`, `QRadioButton`, `QLabel`) directly into the 4 group cards and binds them to `m_scan_ctrl`. The old `.ui`-based widget tree and the `ScanConfig` class are gone.

---

## Task 22.6: `SetupOverlay` takes over `Preferences`'s responsibilities
Status: DONE

[setupoverlay.h](../../../../src/widgets/overlays/setupoverlay.h) + `.cpp` — SetupOverlay now owns `Ui::Preferences*` + a hidden `QDialog` form root. New methods:
- `setup_form()` calls `Ui::Preferences::setupUi(form_root)` and re-parents the 5 top-level groups (`SAVE_LOAD_GRP`, `SERIAL_PORTS_GRP`, `TCU_GRP`, `DEVICES_GRP`, `IMG_PROC_GRP`) into the overlay's rows
- `bind()` ports ~280 lines of widget setup + signal-handler lambdas from the old `Preferences` constructor (combo-box population, event filters, all the `connect(...)` calls)
- `data_exchange`, `config_ip`, `enable_ip_editing`, `set_pixel_format`, `update_distance_display`, `display_baudrate`, `set_ptz_type_enabled`, `switch_language`, `update_device_list`, `toggle_laser` — all ported verbatim
- Re-emits the same 24 signals (`search_for_devices`, `change_pixel_format`, `tcu_type_changed`, ...) UserPanel listens for
- Owns runtime state previously on `Preferences`: `m_split`, `m_cameralink`, `m_dist_ns` (dead members `port_idx`, `use_tcp` not carried over)

---

## Task 22.7: Drop `Preferences` from topbar + userpanel
Status: DONE

[topbar.h/cpp](../../../../src/widgets/topbar/topbar.cpp) — removed `Preferences` forward decl, `preferences` member + init-list, `preferences.h` include, `new Preferences()` instantiation, Alt+S `-- preferences` menu action, and 24 signal-forwarding `connect()`s.

[userpanel.h/cpp](../../../../src/visual/userpanel.cpp) — dropped the `Preferences*` member + include. `SetupOverlay` now constructed early (right after `Config`) so widgets exist before any other code touches them. All ~50 `pref->` sites rewired: `pref->ui->X` -> `m_setup_overlay->w()->X`, helper methods (`data_exchange`, `set_pixel_format`, `config_ip`, ...) -> `m_setup_overlay->...`, state reads (`cameralink`, `split`, `dist_ns`) -> accessor methods, `&Preferences::set_auto_rep_freq`/`set_ab_lock` -> `&SetupOverlay::...`. The 24 signal connects moved from topbar.cpp into userpanel.cpp where `m_setup_overlay` lives. Dead `pref->show()/raise()` (Alt+S shortcut) and `pref->reject()` (closeEvent) removed.

---

## Task 22.8: Remove from CMakeLists + disk
Status: DONE

[CMakeLists.txt](../../../../CMakeLists.txt) — `src/visual/scanconfig.{h,cpp,ui}` and `src/visual/preferences.{h,cpp}` dropped from `PROJECT_SOURCES`. `src/visual/preferences.ui` stays so AUTOUIC still generates `ui_preferences.h` (needed by `SetupOverlay::setup_form()`). Files deleted from disk after build verification.

---

## Task 22.9: Manual regression
Status: USER-DRIVEN

Functional pass against the new code paths: SETUP overlay widget behaviour, SCAN overlay parameter derivation, preset save/load round-trip.

---

## Definition of Done for Sprint 22

- [x] `ScanParams` struct + free routes own scan parameter data
- [x] `ScanController` owns `ScanParams`; pipeline + preset save/load read from `params()`
- [x] `ScanConfig` class + `.ui` deleted; `ScanOverlay` owns widget sync
- [x] `SetupOverlay` owns `Ui::Preferences` widget tree + setup + 24 signals + `data_exchange` + helpers
- [x] `Preferences` class + .h/.cpp deleted; `preferences.ui` retained for AUTOUIC
- [x] All `pref->` / `scan_config->` references removed from active code
- [x] Build clean
- [ ] Manual regression pass (user-driven)
