# Sprint 20 Progress: QSS Theme + Signal Audit

## Prerequisite: Sprints 16–19
Status: SATISFIED

Sprint 19 shipped the SCAN overlay; all 3 modes (SETUP / MONITOR / SCAN) now have their own surfaces and switch cleanly. The legacy `style_dark.qss` / `style_light.qss` (~11 KB each, hardcoded greys, references to the now-removed `TitleBar`) no longer matches the new widget hierarchy. Sprint 20 replaces the dark theme with a prototype-faithful QSS, audits controller signal wiring, and surfaces the deferred Display Previews sidebar.

## Branch
All Sprint 20 work stays on `refactor`.

## Strategy

**Why not delete the legacy dialogs in this sprint** — `Preferences` has 21 active signals to `UserPanel` and is the parent of widgets re-parented into `SetupOverlay` (Sprint 18). Deleting it now would either invalidate those widget pointers or require a `setParent(nullptr)` dance that risks leaking the underlying QObjects. The plan calls for removal "after Sprint 20" — pushed to Sprint 21 once theming is stable.

**QSS approach** — Qt's QSS does not support CSS custom properties, so prototype `:root` variables are translated to literal hex values, organized by section header comments. The new file replaces `resources/style_dark.qss` (resource path unchanged → `:/style/style_dark.qss` keeps loading via `main.cpp:138`). `style_light.qss` is left unchanged for now; the theme toggle still works but only dark mode matches v3.

Sliced into 4 tasks:

- 20.1 ports prototype color tokens + styles standard Qt widgets (QPushButton / QLineEdit / QCheckBox / QRadioButton / QComboBox / QSlider / QScrollBar / QGroupBox / QTabWidget).
- 20.2 styles custom widgets (`TopBar`, `FloatingPanel`, overlay cards, scan-card headers).
- 20.3 signal audit — 7 controllers (RF / Scan / AuxPanel / Lens / Laser / DeviceManager / TCU) verified end-to-end.
- 20.4 Display Previews sidebar (carry-over from Sprints 18.5 / 19.5).

## Out of scope for Sprint 20

- Legacy dialog removal (`Preferences`, `ScanConfig`) — Sprint 21.
- m-mode (meter) widget cleanup — Sprint 21.
- Translation sweep (`lupdate`) — Sprint 21.
- Light-theme parity — future patch.
- Cross-mode input consistency audit — Sprint 21.

---

## Task 20.1: QSS color tokens + standard widget styling
Status: DONE

**Landed:** [resources/style_dark.qss](resources/style_dark.qss) replaced. The legacy 521-line file with hardcoded greys (`#282828`, `#3C3C3C`, etc.) is gone; the new file leads with a header comment block listing the prototype `:root` tokens for reference (no QSS-side variable system since QSS doesn't support CSS custom properties), followed by sectioned rules.

**Tokens included in the reference header:**
- Surfaces: `bg-deepest #040303`, `bg-base #080707`, `bg-raised #12100E`, `bg-elevated #1C1815`, `bg-header #261B14`, `bg-active #2E221A`
- Borders: `border-subtle #241E1A`, `border-default #382D25`, `border-strong #8F6B4D`
- Text: `text-primary #F5E1CE`, `text-secondary #CDB299`, `text-muted #8A7461`, `text-dim #5C4A3D`, `text-ghost #3D2E24`
- Accent: `accent #E0924A`, `accent-glow-33 #E0924A55`
- Semantic: `positive #00ff7f`, `negative #E6455D`, `info #4DA6E6`, `warning #E6B345`, `readout #579972`
- Misc: `slider-thumb #E0924A`, `slider-track #040303`, `crosshair #3cb371`

**Standard widgets covered:**
- `QMainWindow`, `QWidget`, `QFrame`, `QLabel`, `QGroupBox` (with title bar accent)
- `QPushButton` / `QToolButton` — default, hover, pressed, disabled, checked
- `QLineEdit`, `QPlainTextEdit`, `QTextEdit` — base, focus (accent glow), disabled
- `QCheckBox`, `QRadioButton` — bg/border colors plus existing icon assets (`:/tools/dark/check`, `:/tools/dark/dot`)
- `QComboBox`, `MiscSelection`, `QSpinBox` — including dropdown arrow assets
- `QSlider` — horizontal + vertical with accent thumb, dark track, hover/pressed/disabled
- `QScrollBar` — both axes (legacy file only had vertical), accent-strong thumb on hover
- `QTabWidget` / `QTabBar` — selected tab uses accent text color
- `QTableView` / `QHeaderView` — for `Aliasing` dialog
- `QMenuBar` / `QMenu`, `QToolTip`

**Preserved icon assets:** all `:/tools/dark/...` references unchanged (verified against `resources.qrc` aliases). The TopBar action cluster (URL / SETTINGS / CAPTURE / CLS / LANGUAGE / THEME) and window controls (MIN / MAX / EXIT) keep their existing icons.

**Removed:** dead `TitleBar { ... }` and `TitleButton { ... }` rules — those widgets were replaced by `TopBar` in Sprint 16.

**`style_light.qss`:** untouched. Theme toggle still works; only dark mode matches v3.

---

## Task 20.2: Custom-widget styling
Status: DONE

**Landed:** appended a "Sprint 20.2 — custom widgets" section to [resources/style_dark.qss](resources/style_dark.qss). New rules:

- **TopBar** (`#TOP_BAR`) — vertical gradient `bg-header → bg-elevated`, bottom border `border-default`. Slots (`#TOP_BAR_LEFT/CENTER/RIGHT`) transparent. Clock (`#TOP_BAR_CLOCK`) uses readout-green Consolas. Pixel-depth + resolution labels muted. `#SYSTEM_SELECTOR` styled as a thin combo.
- **Mode tabs** (`#MODE_SETUP/MONITOR/SCAN`) — base dark, hover lighter. `:checked` state shows accent gradient (`bg-active → bg-elevated` 135°) with accent border + accent text — matches prototype `.mode-btn.on`.
- **FloatingPanel** (`#FloatingPanel`, `#FloatingPanelHeader`, `#FloatingPanelTitle`, `#FloatingPanelPin`, `#FloatingPanelHide`) — bg-raised body, gradient header (`bg-header → bg-header-end`), accent uppercase title, transparent pin/hide buttons; pin highlights accent when checked, hide highlights `negative-hover-bg-soft` red on hover.
- **OpticsPanel / TCUPanel mode toggles** (`#OPTICS_CLASSIC_BTN`, `#OPTICS_MODERN_BTN`, `#TCU_NORMAL_BTN`, `#TCU_GURU_BTN`) — same checked-accent styling as TopBar mode tabs but smaller padding.
- **Pbar** (`#PANEL_TOGGLE_BAR`, `#PBAR_OPTICS_BTN`, `#PBAR_TCU_BTN`, `#PBAR_CONSOLE_BTN`) — semi-transparent bg-raised container; buttons match the mode-tab checked-accent style.
- **Center toolbar** (`#CENTER_TOOLBAR`) — semi-transparent bg-raised, subtle border.
- **Right stack** (`#RIGHT_STACK`, `#IMG_PROC_WIDGET`, `#SAVE_OPTIONS_WIDGET`, `#VISION_WIDGET`) — bg-raised container, sections separated by 1px subtle bottom border (last section borderless).
- **Side zone** (`#SIDE_DISPLAY_WIDGET`, `#MAPBOX_WIDGET`) — bg-deepest panels with subtle borders.
- **SETUP overlay** (`#SETUP_OVERLAY`, `#SETUP_SCROLL`, `#SETUP_ROW_1/2/3`, `#SETUP_PREVIEW_SIDEBAR`) — bg-base canvas, transparent rows, bordered preview sidebar.
- **SCAN overlay** (`#SCAN_OVERLAY`, `#SCAN_SCROLL`, `#SCAN_PREVIEW_SIDEBAR`) — same canvas pattern as SETUP.
- **SCAN cards** (`#SCAN_CARD`, `#SCAN_CARD_TTL`, `#SCAN_CARD_BODY`) — bg-raised body with `border-default`. Title bar uses gradient `bg-header → bg-header-end` with accent uppercase label, matches prototype `.ov-grp` + `.ov-ttl`.
- **SCAN action column** (`#SCAN_ACTION_COL`) — `#SCAN_BUTTON` styled with `positive-bg`/`positive-border`/`readout` (start-state green); other action buttons use the standard dark-card look. The "Stop" state visual swap is deferred (needs a dynamic property hook in `ScanController`).

**Code tweaks landed alongside QSS:**
- [userpanel.cpp](src/visual/userpanel.cpp) `make_card` lambda: scan-card objectName changed from `"SCAN_CARD_<title>"` (titles like "GATE WIDTH" / "CAPTURE / RECORD" contained spaces and slashes that QSS doesn't accept as ID selectors) to a fixed `"SCAN_CARD"`. The original title is preserved on a `scan_card_title` dynamic property for diagnostics. `SCAN_CARD_TTL` and `SCAN_CARD_BODY` (`QWidget`s, not `QFrame`s) get `WA_StyledBackground=true` so QSS background rules paint.
- [setupoverlay.cpp](src/widgets/overlays/setupoverlay.cpp), [scanoverlay.cpp](src/widgets/overlays/scanoverlay.cpp): preview-sidebar `QWidget`s get `WA_StyledBackground=true`.

**Deferred to Sprint 21 polish:**
- Radial glows around accent buttons / DotIndicators (QSS can't do `box-shadow`).
- SCAN_BUTTON visual swap to the negative/red "Stop" style during a running scan — needs `ScanController` to set a dynamic property the QSS can react to.
- Custom-painted widgets (`TimingBarWidget`, `JoystickWidget`, `ArcSliderWidget`, `UnlockBar`) use their own `paintEvent` colors and are not affected by QSS — Sprint 21 will sync them to the new tokens.
- Scan-card body color (`bg-raised #12100E`) reads slightly disconnected from the surrounding canvas (`bg-base #080707`) — the contrast is by design (matches prototype `bg-raised-93` on `bg-base`) but feels muted; Sprint 21 will tighten the palette or add a subtle border glow.

### Post-20.2 fixes

**Fix A: SETUP overlay showed system-palette grey background.** The legacy `style_dark.qss` had a catch-all `.QWidget { background-color: #282828 }` rule that gave plain `QWidget`/`QFrame` instances a dark bg. I dropped it in 20.1, exposing the system palette through the re-parented Preferences groups (`DEVICES_GRP`, `TCU_GRP`, etc., which are plain `QFrame`s without their own QSS rule). Added baseline `.QWidget` and `.QFrame` rules using `bg-raised #12100E` — these match only exact-class instances, so subclass selectors (e.g. `#SETUP_OVERLAY` for `SetupOverlay : QFrame`) still win.

**Fix B: FPS label rendered on top of the Optics floating panel.** `m_fps_label` was a direct child of `QMainWindow`; per Sprint 18 z-order rules, those paint above `ui->CENTRAL`'s children — including `m_optics_panel` (which lives in `CENTRAL`). Re-parented `m_fps_label` to `ui->CENTRAL`, and the position math now uses `mapTo(ui->CENTRAL, …)` to convert the SOURCE_DISPLAY-relative bottom-left point. Added `raise()` calls on the 3 floating panels at the end of `resizeEvent` so they sit above the fps label after every layout change.

---

## Task 20.3: Signal audit
Status: DONE

Walked through each of the 7 controllers' `init()` / wiring section plus the corresponding `userpanel.cpp` signal-routing block. **No fixes needed** — Sprint 14's controller decoupling already isolated controllers from widgets: each controller emits signals (`*_changed`, `*_updated`, etc.) and `UserPanel` routes them to widgets via lambdas. Re-parenting widgets through Sprints 16–19 didn't break any wiring because Qt's signal/slot connections are object-to-object, not position-dependent.

| Controller | Internal connects | UI-directed (UserPanel) | Verdict |
|---|---|---|---|
| `DeviceManager` (`devicemanager.cpp`) | ~7 (port objects) | 11 (TCU_COM/PTZ/VL/IR widgets, `pref->set_ptz_type_enabled`) | OK |
| `TCUController` (`tcucontroller.cpp`) | 0 | 17 (TCU sliders/displays + `pref->ui->PS_*_EDT`) | OK |
| `LensController` (`lenscontroller.cpp`) | 0 | 4 (zoom/focus widgets — re-parented into `OpticsPanel` but pointers valid) | OK |
| `LaserController` (`lasercontroller.cpp`) | 0 | 5 (`LASER_BTN`/`FIRE_LASER_BTN` — re-parented into `TCUPanel` but pointers valid) | OK |
| `RFController` (`rfcontroller.cpp`) | 0 | 1 (`ui->DISTANCE`) | OK |
| `ScanController` (`scancontroller.cpp`) | 1 (self-queued) | 2 (`SCAN_BUTTON`/`CONTINUE_SCAN_BUTTON`/`RESTART_SCAN_BUTTON` — re-parented into SCAN action column but pointers valid) | OK |
| `AuxPanelManager` (`auxpanelmanager.cpp`) | 0 | 1 (`display_page_changed` → page-switch lambda) | OK |

**Total:** ~45 UI-directed signals, ~15 internal port-object signals; zero references to removed or hidden widgets (`ui->LEFT`, `ui->RIGHT`, `LASER_STATIC`, `IMG_PROC_STATIC`, `IMG_SAVE_STATIC`, legacy `STATUS`).

`Preferences` and `ScanConfig` dialogs are still alive (they own children that were re-parented into the overlays); accessed via `ui->TITLE->preferences` / `->scan_config`. Removal of these QObject shells is Sprint 21 work.

---

## Task 20.4: Display Previews sidebar
Status: DONE

**Landed:** [userpanel.cpp](src/visual/userpanel.cpp) — added a "Preview sidebars" block right after the SCAN overlay creation. 3 thumbnail `QLabel`s per overlay (`m_setup_thumb[3]`, `m_scan_thumb[3]`), each labeled MAIN / ALT 1 / ALT 2.

**Approach — no pipeline-tee needed.** `Display` already exposes `set_pixmap(QPixmap)` as a Qt signal (internally connected to its own `setPixmap` slot — see [display.cpp:15](src/widgets/display.cpp)). The pipeline emits this signal queued from the `GrabThread`. To mirror frames into thumbnails, the new block adds another listener to each `displays[i]->set_pixmap` that scales the pixmap to fit the thumbnail (`Qt::KeepAspectRatio`, `Qt::SmoothTransformation`). One additional scale per frame per visible thumbnail — negligible cost on the GUI thread.

**Layout details:**
- Both overlay constructors construct the preview sidebar with a placeholder `QLabel`. The new `populate_thumbs` lambda clears the layout (placeholder + stretch) and adds 3 [name-label, thumbnail-label] pairs followed by a trailing stretch.
- Thumbnails: 180 × 150 minimum, dark `bg-deepest #040303` background with subtle border. Scale-to-fit preserves the source aspect ratio inside that frame.
- Section labels: accent-colored uppercase ("MAIN" / "ALT 1" / "ALT 2") above each thumbnail.

**Empty-state behavior:** when the camera isn't grabbing, no `set_pixmap` signal fires; thumbnails stay as plain dark rectangles. No additional null-handling needed beyond a `pm.isNull()` guard inside the scaler lambda.

**Memory model:** thumbnails are children of the preview-sidebar `QWidget` (which is a child of the overlay), so they get auto-deleted with the overlay. The Qt `connect()` calls are auto-disconnected on `displays[i]` destruction (which happens when UserPanel is destroyed — same lifetime).

---

## Definition of Done for Sprint 20

- [x] Dark theme matches prototype v3 at the standard-widget level (20.1).
- [x] Custom widgets (TopBar, FloatingPanel, overlay cards) match prototype (20.2).
- [x] All 7 controllers wired end-to-end; no signals dangling on removed widgets (20.3).
- [x] Display Previews sidebar shows 3 live thumbnails in SETUP and SCAN overlays (20.4).
- [x] Sprint 20 progress doc updated.
