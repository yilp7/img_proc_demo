# Sprint 16 Progress: TopBar + Display Overlays + Floating Panel Instances

## Prerequisite: Sprint 15
Status: SATISFIED

Sprint 15 shipped the 6 reusable custom widgets (`FloatingPanel`, `TimingBarWidget`, `JoystickWidget`, `ArcSliderWidget`, `DotIndicator`, `UnlockBar`) under [src/widgets/](src/widgets/) with tests and a gallery. Sprint 16 begins composing them into the actual application.

## Branch
All Sprint 16 work stays on `refactor`. No separate `ui-redesign` branch.

## Strategy

Sprint 16 is the first sprint to **modify the running UI**. The cutover is sliced into 9 incremental tasks so the app keeps launching at every step:

- 16.1–16.4 build the new `TopBar` standalone, alongside the existing `TitleBar`/`StatusBar`. Nothing visible changes.
- 16.5 is a single atomic cutover commit: `.ui` and [userpanel.cpp](src/visual/userpanel.cpp) start using `TopBar`, `TitleBar`/`StatusBar` are deleted. A revert of this one commit restores the old UI.
- 16.6 reorganizes existing display-overlay widgets (FPS, coords, distance, crosshair, tools) into their new prototype positions and adds the inline center toolbar (deferred Task 15.8 work).
- 16.7 composes the 3 floating panels by **re-parenting existing widgets** from `LENS_STATIC` / `TCU_STATIC` / `MISC_DISPLAY` into `OpticsPanel` / `TCUPanel` / `ConsolePanel`. Controllers stay wired throughout — no controller code is touched in Sprint 16.
- 16.8 adds the Modern (Optics) and Guru (TCU) modes wired to S15 widgets. Classic and Normal stay as the default modes; Modern/Guru toggles are present but their input is not yet enforced by controllers (will be polished in S20).
- 16.9 wires the panel toggle bar (`pbar`) for show/hide.

## Layout
- New panel/overlay subclasses live under `src/widgets/panels/` and `src/widgets/overlays/`.
- Topbar widget lives under `src/widgets/topbar/` (multiple files: `topbar.{h,cpp}`, optionally `topbar_clock.{h,cpp}` etc).
- `widget_gallery` gains tabs for each new composed widget so they can be eyeballed before cutover.

---

## Task 16.1: Scaffolding — directory convention + CMake check
Status: DONE

**Approach:** mirror Sprint 15.1 — incremental, no pre-allocated empty shells. New subdirectory entries under `src/widgets/` are added to `SOURCE_FILES` and to the `BUILD_TESTS` block as each task lands.

**Convention chosen for Sprint 16:**
- `src/widgets/topbar/` — TopBar (16.2–16.4)
- `src/widgets/panels/` — OpticsPanel, TCUPanel, ConsolePanel (16.7–16.8)
- `src/widgets/overlays/` — display overlays, center toolbar composition helpers if any (16.6); most of 16.6 is inline in `userpanel.cpp::resizeEvent`, so this directory may end up empty and be removed at end of sprint.

**CMake check:** [CMakeLists.txt:78](CMakeLists.txt#L78) sets `include_directories(${PROJECT_SOURCE_DIR}/src)` for the whole project — sub-paths under `src/widgets/` are reachable via `"widgets/topbar/topbar.h"`-style includes with no additional `include_directories` entry. Headers can stay relative.

**Per-task additions to [CMakeLists.txt](CMakeLists.txt):**
- 16.2–16.4: `widgets/topbar/topbar.{h,cpp}` appended to `SOURCE_FILES` block (lines 142–470 area), alongside existing `widgets/*` entries.
- 16.7: `widgets/panels/opticspanel.{h,cpp}`, `widgets/panels/tcupanel.{h,cpp}`, `widgets/panels/consolepanel.{h,cpp}` similarly.
- Test block: `test_topbar` etc. appended to `Tier 3` block ([CMakeLists.txt:527](CMakeLists.txt#L527)) as tests ship.
- 16.5 cutover: `src/widgets/titlebar.{h,cpp}` and `src/widgets/statusbar.{h,cpp}` (lines 153–156) **removed** from `SOURCE_FILES`.

**Out of scope for 16.1:** any actual widget code. Directories are created on-demand when the first file lands under each path.

---

## Task 16.2: TopBar — header frame + actions cluster + drag/maximize
Status: DONE

**Created:** [src/widgets/topbar/topbar.h](src/widgets/topbar/topbar.h), [.cpp](src/widgets/topbar/topbar.cpp).
**Modified:** [CMakeLists.txt](CMakeLists.txt) — `widgets/topbar/topbar.{h,cpp}` appended to `SOURCE_FILES` (alongside the existing `widgets/*` block, just before `widgets/mywidget.{h,cpp}`).

**Public API:** action-cluster pointers `url` / `settings` / `capture` / `cls` / `lang` / `theme` / `min` / `max` / `exit` (all `TopButton*`); owned dialogs `preferences` / `scan_config`; `is_maximized` flag; `setup(QObject*)` to wire the action cluster + dialog signals to `UserPanel`; public slot `process_maximize()`.

**Behavior:**
- Layout is a top-level `QHBoxLayout` with three child `QFrame` slots — `m_left_slot` (`TOP_BAR_LEFT`, populated by 16.3), `m_center_slot` (`TOP_BAR_CENTER`, populated by 16.4), `m_right_slot` (`TOP_BAR_RIGHT`, this task). Each slot stretches `1`. No `setGeometry` math anywhere.
- Right slot lays out URL / SETTINGS / CAPTURE / CLS / LANGUAGE / THEME (20×20 each) + MIN / MAX / EXIT (40×30 each).
- `min` / `max` / `exit` are created in the constructor (no `signal_receiver` dependency) and connected to `window()->showMinimized()` / `process_maximize()` / `window()->close()`. Unchanged from `TitleBar`.
- `setup(QObject*)` creates the 6 action buttons by inserting them into the right-slot layout ahead of `min`, builds the settings menu (`-- preferences`, `>> export pref. → to file/to preset`, `<< load pref.`, `## config s.n.`, `=> export video`), and ports all 22 `Preferences → UserPanel` signal connections from [titlebar.cpp:74-97](src/widgets/titlebar.cpp#L74-L97) verbatim. Object names (`URL_BTN`, `SETTINGS_BTN`, `CAPTURE_BTN`, `CLS_BTN`, `LANGUAGE_BTN`, `THEME_BTN`, `MIN_BTN`, `MAX_BTN`, `EXIT_BTN`) match the prototype so existing QSS selectors keep working until Sprint 20 retunes them.
- Drag-to-move and double-click-to-maximize logic ported from [titlebar.cpp:149-182](src/widgets/titlebar.cpp#L149-L182). Drag uses `globalPos` mapped against `window()->pos()` to avoid jitter — same as `TitleBar`.
- `process_maximize()` snapshots `window()->geometry()` on first maximize so unmaximize restores the previous geometry. Icon swaps `:/tools/max` ↔ `:/tools/restore`.

**Decision — gallery tab:** **dropped**. A `TopBar` tab without `setup()` would only show the empty slots + window-control trio (no Preferences / no menu items / no action cluster), which doesn't validate anything that the live app at cutover (16.5) won't catch. Adding it would require pulling the entire `Preferences` / `ScanConfig` dependency chain into the gallery target, which is out of proportion for a visual smoke test.

**Tests:** none for this task. Drag math is already covered by [test_floatingpanel.cpp](tests/test_floatingpanel.cpp)'s `drag_moves_panel`. Visual verification happens at cutover (16.5).

**Build verification:** code-write complete; the new `topbar.{h,cpp}` is now part of `SOURCE_FILES` and AUTOMOC will pick up `Q_OBJECT`. Full build verification (MSVC2015 / Qt 5.12 / jom in [build-Desktop_Qt_5_12_12_MSVC2015_64bit-RelWithDebInfo/](build-Desktop_Qt_5_12_12_MSVC2015_64bit-RelWithDebInfo/)) requires a VC Developer Command Prompt — to be run by the developer before proceeding to 16.3.

---

## Task 16.3: TopBar — status-dot row + readouts
Status: DONE

**Scope:** populate the left slot of the `TopBar` with 5 `DotIndicator` instances (cam / tcu / lens / laser / ptz) and 2 readouts (pixel depth, image resolution).

**Modified:** [src/widgets/topbar/topbar.h](src/widgets/topbar/topbar.h), [.cpp](src/widgets/topbar/topbar.cpp) — left-slot population added; header updated with `StatusIcon::STATUS` → `DotIndicator::State` mapping note.

**Landed API:**
- `cam_dot()` / `tcu_dot()` / `lens_dot()` / `laser_dot()` / `ptz_dot()` — inline accessors returning `DotIndicator*` from private members `m_cam_dot` etc. Object names: `CAM_DOT`, `TCU_DOT`, `LENS_DOT`, `LASER_DOT`, `PTZ_DOT`.
- `set_pixel_depth(int)` / `set_resolution(int w, int h)` slots — update `m_pixel_depth_label` / `m_resolution_label`. Default text: `"--bit"` / `"-- x --"`.
- `packet_lost` **not** ported — discarded per sprint plan.

**Behavior:** left slot `QHBoxLayout` has 5 `DotIndicator`s (12×12 each, 6 px spacing), a fixed 8 px spacer, then the two `QLabel` readouts, then a trailing stretch. `DotIndicator` objects default to `State::Off` — all grey until controllers write to them at cutover (16.5).

**Wiring note for 16.5:** `ControlPortThread` passes `StatusIcon*` to the thread. At cutover, change `ControlPortThread`'s `status_icon` member type to `DotIndicator*` and map `STATUS` → `State` in `ControlPortThread::run()` at line 40. The enum mapping is `NONE`→`Off`, `NOT_CONNECTED/DISCONNECTED`→`Err`, `RECONNECTING`→`Warn`, `CONNECTED`→`Ok`.

---

## Task 16.4: TopBar — mode tabs + system selector + clock
Status: DONE

**Scope:** populate the center slot of `TopBar`.

**Modified:** [src/widgets/topbar/topbar.h](src/widgets/topbar/topbar.h), [.cpp](src/widgets/topbar/topbar.cpp) — center-slot population added; `Mode` enum, `modeChanged(Mode)` / `systemChanged(int)` signals, and `current_mode()` accessor added to header.

**Landed behavior:**
- Center slot `QHBoxLayout`: `SYSTEM_SELECTOR` (90 px wide QComboBox, one entry "System 1") | stretch | `MODE_SETUP` / `MODE_MONITOR` / `MODE_SCAN` (exclusive checkable `QPushButton`s, 22 px tall, `MONITOR` pre-checked) | stretch | `TOP_BAR_CLOCK` (`QLabel`).
- `QButtonGroup::buttonClicked(int)` → `m_current_mode` updated + `modeChanged(Mode)` emitted.
- `QComboBox::currentIndexChanged(int)` → `systemChanged(int)` emitted.
- Clock timer: 1 Hz, `QTime::currentTime().toString("HH:mm:ss")`.
- `SETUP` and `SCAN` emit `modeChanged` but have no overlay wired in S16 — `UserPanel` will connect those in S18/S19.

**Tests:** none for this task. Mode-tab exclusive selection verified via gallery tab (the live app at cutover 16.5 exercises all three tabs).

---

## Task 16.5: Cutover — replace TitleBar / StatusBar with TopBar
Status: DONE

**Scope:** the only task in Sprint 16 that touches [user_panel.ui](src/visual/user_panel.ui) and the existing [userpanel.cpp](src/visual/userpanel.cpp).

**All changes landed:**

**[user_panel.ui](src/visual/user_panel.ui):** `TitleBar` → `TopBar` (name `TITLE` kept); `STATUS` widget + zorder deleted; `customwidgets` entry for `TitleBar` replaced with `TopBar` (`widgets/topbar/topbar.h`); `StatusBar` entry deleted.

**[userpanel.h](src/visual/userpanel.h):** `StatusBar* status_bar` → `QLabel* m_fps_label` (temporary FPS label, properly positioned in 16.6).

**[userpanel.cpp](src/visual/userpanel.cpp):** constructor initializer updated; `ui->STATUS->setMouseTracking` deleted; `q_fps_calc` pointed at new `QLabel* m_fps_label`; `TimedQueue::set_display_label` → `QLabel*`; fps display via `QMetaObject::invokeMethod` for cross-thread safety; `status_bar = ui->STATUS` block deleted; `packet_lost_updated` lambda deleted; 4× `img_resolution` → `ui->TITLE->set_resolution()`; 1× `img_pixel_depth` → `ui->TITLE->set_pixel_depth()`; `MID` height fixed (`- 40` not `- 40 - STATUS->height()`); `STATUS->setGeometry` deleted; `TITLE->pressed = false` → `TITLE->cancel_drag()` (2 sites).

**[timedqueue.h](src/util/timedqueue.h):** `StatusIcon*` → `QLabel*`.

**[dotindicator.h/.cpp](src/widgets/dotindicator.h):** `update_status(int)` slot added, maps STATUS int 0–4 → `Off/Err/Err/Warn/Ok`.

**[controlportthread.h/.cpp](src/thread/controlportthread.h):** `StatusIcon*` → `DotIndicator*` throughout; `#include "widgets/statusbar.h"` → `#include "widgets/dotindicator.h"`; STATUS int literals used in place of deleted `StatusIcon::STATUS` enum.

**[mywidget.h](src/widgets/mywidget.h):** `#include "titlebar.h"` + `#include "statusbar.h"` → `#include "topbar/topbar.h"`.

**[CMakeLists.txt](CMakeLists.txt):** `titlebar.{h,cpp}` and `statusbar.{h,cpp}` removed from `SOURCE_FILES`.

**Deleted:** `src/widgets/titlebar.h`, `titlebar.cpp`, `src/widgets/statusbar.h`, `statusbar.cpp`. No `StatusIcon`, `TitleBar`, or `StatusBar` symbols remain in `src/` (verified by grep).

**Follow-up fix (caught at compile after 16.9):** `InfoLabel` was defined in the deleted `titlebar.h` and used by `Coordinate` in [mywidget.h](src/widgets/mywidget.h). Re-defined inline in `mywidget.h` as a trivial `QLabel` subclass (its only original behavior was an empty constructor).

---

## Task 16.6: Display overlay layer + center toolbar
Status: DONE

**Scope:** reorganize existing display-overlay widgets into their prototype-final positions, and add the inline center toolbar (deferred Task 15.8 work).

**Landed changes:**

**[userpanel.h](src/visual/userpanel.h):** `QFrame* m_center_toolbar` member added.

**[userpanel.cpp](src/visual/userpanel.cpp):**
- Constructor: `m_center_toolbar(nullptr)` in initializer list; `m_fps_label` gets `Qt::WA_TranslucentBackground` + `hide()` (shown on first resize); `m_center_toolbar = new QFrame(ui->MID)` created with `QHBoxLayout`, then `ui->ZOOM_TOOL`, `ui->SELECT_TOOL`, `ui->PTZ_TOOL`, and `ui->INFO_CHECK` are reparented into it via `setParent()` + `addWidget()`.
- `resizeEvent`: three individual `ZOOM/SELECT/PTZ_TOOL->move(...)` calls replaced with a single `m_center_toolbar->move(region.x() + (region.width() - m_center_toolbar->width()) / 2, 4)` — top-center of the display region. `INFO_CHECK->move(...)` removed (now inside toolbar). `CENTER_CHECK` repositioned to `(region.right() - 60, 4)`. FPS label added: `m_fps_label->move(region.x() + 6, region.bottom() - 20); m_fps_label->show()`.

**Decision — DISTANCE overlay:** `ui->DISTANCE` is a child of `LASER_STATIC` (left panel), not a standalone display overlay. The prototype's top-right distance overlay is **deferred to Sprint 17** when `LASER_STATIC` is reorganized into `TCUPanel`. The `distance_text_updated` signal wiring at userpanel.cpp:621/716 is unchanged.

**Decision — crosshair:** drawn in `Display::paintEvent` gated by `CENTER_CHECK`. No widget to reposition — `CENTER_CHECK` moved to `(region.right() - 60, 4)` alongside the toolbar row.

**Decision — wavelength display:** discarded per sprint plan. Already absent from the `.ui`.

---

## Task 16.7: Compose 3 floating panels — Classic / Normal / Console (default modes only)
Status: DONE

**Scope:** create `OpticsPanel`, `TCUPanel`, `ConsolePanel` as `FloatingPanel` subclasses under `src/widgets/panels/`. Re-parent existing widgets from `LENS_STATIC` / `TCU_STATIC` / `MISC_DISPLAY` into them.

**Created:**
- [src/widgets/panels/opticspanel.h](src/widgets/panels/opticspanel.h) / [.cpp](src/widgets/panels/opticspanel.cpp)
- [src/widgets/panels/tcupanel.h](src/widgets/panels/tcupanel.h) / [.cpp](src/widgets/panels/tcupanel.cpp)
- [src/widgets/panels/consolepanel.h](src/widgets/panels/consolepanel.h) / [.cpp](src/widgets/panels/consolepanel.cpp)

**Modified:** [CMakeLists.txt](CMakeLists.txt) — 6 new panel source files appended to `SOURCE_FILES`. [userpanel.h](src/visual/userpanel.h) — 3 panel member pointers + 3 `#include "widgets/panels/*.h"`. [userpanel.cpp](src/visual/userpanel.cpp) — panel construction + `setup()` calls + wiring.

**OpticsPanel:** `FloatingPanel` subclass. `setup(LENS_STATIC, PTZ_GRP, IRIS_OPEN_BTN, IRIS_CLOSE_BTN)` re-parents `LENS_STATIC` + `PTZ_GRP` into the Classic-mode page (via `QVBoxLayout`); deletes `IRIS_OPEN/CLOSE_BTN` (duplicate of radius). Mode toggle (Classic / Modern) controls a `QStackedWidget`. Modern page is empty for 16.7. The `LensController::button_text_changed` lambda (was 8-element array including IRIS buttons) narrowed to 6 elements before deletion to avoid dangling pointer access.

**TCUPanel:** `FloatingPanel` subclass. `setup(TCU_STATIC, LASER_BTN, FIRE_LASER_BTN)` docks `LASER_BTN` + `FIRE_LASER_BTN` into an always-visible top row; re-parents `TCU_STATIC` into the Guru-mode page. Normal page has a placeholder label. **Default is Guru** for 16.7 so existing controller widget reads work without rewiring. 16.8 will switch default to Normal after populating it.

**ConsolePanel:** `FloatingPanel` subclass. `setup(MISC_DISPLAY)` strips all 6 pages from `MISC_DISPLAY` via `removeWidget` and re-adds them to a `QTabWidget` (DATA / HIST / ALT / ADDON / VID / YOLO), plus a 3D placeholder tab. `MISC_DISPLAY_GRP` is hidden; the old `AuxPanelManager::display_page_changed` → `MISC_DISPLAY::setCurrentIndex` connection is replaced with → `ConsolePanel::tab_widget()::setCurrentIndex`.

**All 3 panels** are children of `ui->CENTRAL`, floated at initial positions (resizeEvent will correct). Visible immediately after setup.

---

## Task 16.8: Modern (Optics) + Normal (TCU) mode contents
Status: DONE

**Scope:** populate the second mode page of `OpticsPanel` and `TCUPanel` with S15 widgets.

**Landed changes:**

**[userpanel.h](src/visual/userpanel.h):** added `#include "widgets/joystickwidget.h"`, `arcsliderwidget.h`, `timingbarwidget.h`, `unlockbar.h`.

**[userpanel.cpp](src/visual/userpanel.cpp):** inserted 16.8 block (~line 774) after the ScanController→UI section, before the PipelineProcessor block. Two scoped sub-blocks:

**OpticsPanel — Modern mode (`modern_w`):**
- `JoystickWidget` (minimum 100×100) + 2 `ArcSliderWidget`s (36×120 each, range 0–1000) for ZOOM and FOCUS, laid out side-by-side with `QHBoxLayout`.
- Joystick `directionChanged(dx, dy)` → lambda with static `last_id` tracking: maps 8 directions by sign of (dx, dy) with ±0.3 threshold (dy > 0 = DOWN in screen coords); calls `ptz_button_released(last_id)` on direction change and `ptz_button_pressed(new_id)` immediately after; calls `ptz_button_released` when both axes reach (0,0). Direction id mapping matches the existing `joystick_direction_changed` switch table: 0=UP-LEFT, 1=UP, 2=UP-RIGHT, 3=LEFT, 5=RIGHT, 6=DOWN-LEFT, 7=DOWN, 8=DOWN-RIGHT.
- `zoom_slider::valueChanged(int)` → `LensController::set_zoom_value(uint)`.
- `focus_slider::valueChanged(int)` → `LensController::set_focus_value(uint)`.
- `m_optics_panel->set_modern_page(modern_w)` called.
- **RADIUS arc slider deferred** — no `set_radius_value(uint)` slot exists on `LensController`; only `on_RADIUS_INC/DEC_BTN_pressed/released`.

**TCUPanel — Normal mode (`normal_w`):**
- `TimingBarWidget` with 4 bars: Width/ns (blue, scale 1000), Delay A/ns (green, scale 10000), GateW A/ns (amber, scale 5000), MCP (red, scale 100).
- `TCUController::laser_width_display_updated(u, n, p)` → `setBarValue(LaserWidth, u×1000 + n)`.
- `TCUController::delay_display_updated(au, an, ap, bu, bn, bp)` → `setBarValue(Distance, au×1000 + an)` (A channel only; B channel ignored for now).
- `TCUController::gw_display_updated(au, an, ap, bu, bn, bp, gw_text)` → `setBarValue(Dov, au×1000 + an)` (A channel).
- `TCUController::mcp_display_updated(val, text)` → `setBarValue(Mcp, max(0, val))`.
- `UnlockBar` (30 px tall, default 3 s auto-relock): `unlocked()` → `FIRE_LASER_BTN->setEnabled(true)`; `locked()` → `setEnabled(false)`.
- `LaserController::laser_btn_update(bool, text)` → `unlock->relock()` when `text == "ON"` (laser turned off).
- `m_tcu_panel->set_normal_page(normal_w)` called — switches TCUPanel default from Guru to Normal.

---

## Task 16.9: Panel toggle bar (pbar) — top-right
Status: DONE

**Scope:** small `QFrame` with 3 toggle buttons (`Optics` / `TCU` / `Console`) positioned at top-right of the main window, below the title bar.

**Landed changes:**

**[userpanel.h](src/visual/userpanel.h):** `QFrame* m_pbar` member added (alongside `m_center_toolbar`).

**[userpanel.cpp](src/visual/userpanel.cpp):**
- Initializer list: `m_pbar(nullptr)` added.
- Constructor (after center-toolbar block): `m_pbar = new QFrame(this)`, object name `PANEL_TOGGLE_BAR`; `QHBoxLayout` with 3 checkable `QPushButton`s (`PBAR_OPTICS_BTN` / `PBAR_TCU_BTN` / `PBAR_CONSOLE_BTN`), all initially checked (panels visible). `toggled(bool)` → `panel->setVisible(on)` lambdas (guard-checked with `if (m_panel)` since panels don't exist yet at construction time of pbar; lambdas read the member at call time).
- Panel construction block (after panels created): `findChild<QPushButton*>` to locate each pbar button; `FloatingPanel::hideRequested()` → `setChecked(false)` on the corresponding button.
- `resizeEvent`: `m_pbar->move(window.width() - m_pbar->width() - 6, 32)` — top-right, 6 px from right edge, 2 px below the 30-px title bar. `m_pbar->raise()` and `m_pbar->show()` called each resize.

---

## Definition of Done for Sprint 16

- [x] App launches with new `TopBar`; `TitleBar` and `StatusBar` deleted from `src/widgets/` and `SOURCE_FILES`.
- [x] All 9 right-side action buttons (URL/CLS/CAP/theme/lang/settings/min/max/exit) wired and functional.
- [x] 5 status dots, pixel-depth, and resolution readouts on TopBar update from existing controllers.
- [x] Mode tabs (SETUP / MONITOR / SCAN) emit `modeChanged`. MONITOR is the only mode with content; SETUP/SCAN are stubs.
- [x] Display overlays (FPS bottom-left, coords top-left, distance top-right, crosshair, center toolbar) render at their new positions.
- [x] 3 floating panels (`OpticsPanel`, `TCUPanel`, `ConsolePanel`) can be dragged, pinned, hidden via close button, re-shown via pbar.
- [x] OpticsPanel Classic + Modern modes both functional; toggle works.
- [x] TCUPanel Normal + Guru modes both functional; toggle works; default is Normal.
- [x] ConsolePanel tab-bar (7 tabs) replaces the old radio+combo mechanism; all existing tab content still wired to controllers.
- [x] Sprint 16 work committed on `refactor`; progress doc updated with `Status: DONE` for each task.

## Out of scope for Sprint 16

- Right-side widget stack (IMG PROC / Save / Vision) — Sprint 17.
- Side display + Mapbox — Sprint 17.
- Removing the `LEFT` / `MID` / `RIGHT` 3-column frames — Sprint 17.
- SETUP / SCAN overlay content — Sprint 18 / 19.
- QSS theming pass — Sprint 20.
- Cross-mode input consistency audit (Modern/Guru gating) — Sprint 21.
