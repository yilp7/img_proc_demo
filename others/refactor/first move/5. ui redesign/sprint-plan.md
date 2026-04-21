# UI Redesign Sprint Plan (Sprints 15–21)

Overall plan for porting the GLI_user_panel GUI to match `others/ui_redesign/prototype.html` v3.

**Duration**: ~12 weeks across 7 sprints
**Prerequisites**: Sprints 11–14 complete (thread safety, pipeline extraction, controller decoupling)
**Reference**: `others/ui_redesign/UI_REDESIGN_PLAN.md`

Per-sprint progress docs (`sprint-XX-progress.md`) will be created when each sprint starts. This file is the overall plan only.

---

## Sprint 15 — Custom Widget Infrastructure (2 weeks)

### Prerequisites
- Sprint 14 complete
- UI_REDESIGN_PLAN.md reviewed and locked
- `prototype.html` v3 available as visual reference

### What it does
Builds the reusable custom widgets that have no Qt equivalent — the foundation for the new UI. Each widget is self-contained, tested in isolation, and ready for composition in Sprint 16.

Widgets to build:
- **FloatingPanel** — base class for draggable/pinnable/hideable panels (z-order, transparency, bounded drag)
- **TimingBarWidget** — 4 horizontal draggable timing bars for TCU Normal mode
- **JoystickWidget** — virtual joystick for PTZ Modern mode
- **ArcSliderWidget** — vertical arc slider variant for zoom/focus/iris
- **DotIndicator** — colored status dot with glow for top bar
- **UnlockBar** — striped safety toggle for laser fire
- **CenterToolbar** — tool mode selector + info checkbox for display toolbar

Plus a gallery test app and unit tests.

### End state
- 7 widget classes compile under `src/widgets/` with no warnings
- Gallery app (`tests/widget_gallery/`) visually matches prototype elements
- FloatingPanel transparency strategy decided (QSS `rgba` vs. `WA_TranslucentBackground`)
- No changes yet to `userpanel.*`, controllers, or `user_panel.ui` — main app still runs as before

---

## Sprint 16 — TopBar + Display Overlays + Floating Panel Instances (2 weeks)

### Prerequisites
- Sprint 15 complete (all 7 custom widgets ready)

### What it does
Replaces the old TitleBar/StatusBar with the new TopBar. Wires display overlay widgets. Composes the 3 floating panel instances and populates them with their child controls.

Main work:
- New `TopBar` widget consolidates TitleBar actions (URL / CLS / CAP / theme / lang / settings / min / max / exit) plus StatusBar readouts (dots, pixel depth, resolution) plus system selector, mode tabs, clock
- Old `StatusBar` removed; its signals routed to TopBar
- Display overlay layer: FPS (bottom-left), coordinates (top-left START/SHIFT/SHAPE), distance (top-right), crosshair, center toolbar (SEL/ZOOM/PTZ + INFO)
- `OpticsPanel`: Classic PTZ grid + lens rows / Modern joystick + arc sliders (toggleable)
- `TCUPanel`: Normal (timing bars + fire + unlock) / Guru (full param grid) (toggleable)
- `ConsolePanel`: 7-tab bar (DATA / HIST / ALT / ADDON / VID / YOLO / 3D) reusing existing stacked-widget content
- Panel toggle bar (pbar) positioned top-right, wired to show/hide the 3 panels

### End state
- App launches with new TopBar instead of old TitleBar
- All 3 floating panels can be dragged, pinned, hidden, and re-shown
- Display shows FPS, coords, distance overlays correctly
- Mode toggles inside panels (Classic/Modern, Normal/Guru) swap their child widgets
- Right-side widgets (IMG PROC / Save / Vision) not yet present — display fills that space temporarily
- SETUP / SCAN mode tabs exist but overlays are stubs

---

## Sprint 17 — Right-Side Widget Stack + Side Display + Mapbox (2 weeks)

### Prerequisites
- Sprint 16 complete

### What it does
Implements the fixed right-side widget stack (IMG PROC / Save Options / Vision) with dynamic layout, plus the left-side widgets (Side Display, Mapbox). Finalizes the MONITOR-mode layout.

Main work:
- `RightStackContainer` with `QVBoxLayout`: pbar + IMG PROC + Save Options + Vision
- `ImgProcWidget` with MODELS and GENERAL tabs
- `SaveOptionsWidget`: path + browse + ORI/RES capture+record toggle buttons
- `VisionWidget`: MAIN/ALT1/ALT2 detection rows; auto-hide when ALTs inactive
- `SideDisplayWidget` reusing `FloatingWindow[0]` content
- `MapboxWidget` placeholder
- Dynamic layout in `userpanel.cpp::resizeEvent`: center display fits 16:9 after reserving left and right zones
- Floating panel drag clamping to avoid overlap with side widgets or right stack

### End state
- MONITOR mode layout matches prototype at full window resolution
- Window resize produces smooth layout without overlap
- All right-side widgets wired to existing controller signals
- Side Display shows second camera feed when activated
- SETUP / SCAN overlays still stubs — will be filled in Sprint 18/19

---

## Sprint 18 — SETUP Overlay (2 weeks)

### Prerequisites
- Sprints 16, 17 complete

### What it does
Absorbs the `Preferences` dialog and left-panel device controls into a single full-window SETUP overlay. Migrates all device/TCU/camera/save settings. Keeps the legacy `Preferences` dialog as a fallback for one sprint.

Main work:
- `SetupOverlay` container: row-based scrollable config area + fixed preview sidebar
- Row 1 groups: Presets / Ports (COM) / Serial Ports / Save-Load / TCU Configuration
- Row 2 groups: Devices / Camera (with Image Region and Sensor Taps icon buttons)
- Row 3: Image Processing sub-groups (Accumulative / Adaptive / DCP / 3D Mapping / Fishnet / ECC Denoise)
- Preview sidebar: Main / ALT1 / ALT2 live thumbnails
- Config load/save preserves identical JSON semantics
- Reconnect controller signals (Camera, TCU, Save, ImageProc) to SetupOverlay widgets

### End state
- Every setting that was reachable in `Preferences` dialog is reachable in SETUP overlay
- Config file round-trips produce identical values as before
- Preview thumbnails update live
- Legacy `Preferences` dialog still accessible via fallback flag — kept for one sprint as safety net
- SCAN mode tab still stubbed

---

## Sprint 19 — SCAN Overlay + Mode Switching (1 week)

### Prerequisites
- Sprint 18 complete

### What it does
Absorbs the `ScanConfig` dialog into a SCAN overlay. Finalizes mode switching (SETUP / MONITOR / SCAN) with per-mode panel visibility persistence.

Main work:
- `ScanOverlay` container: 4 config groups + action sidebar
- Groups: Delay (start/end/step + PRF) / Gate Width / PTZ (H/V start/end/step + direction + wait) / Capture-Record (ORI/RES + profiles + imgs/profile)
- Action buttons wired to existing `ScanController`: Start / Continue / Restart / Stop
- Mode switcher: hide/show floating panels and overlays on mode change, persist per-panel hidden state across mode switches
- Delete or stub `ScanConfig` dialog

### End state
- SCAN runs end-to-end from the new overlay, producing the same artifacts as the legacy dialog
- Mode tabs in TopBar switch cleanly: no flicker, no overlap artifacts
- Floating panel visibility state persists across mode switches (close Optics in MONITOR → goes to SETUP → returns to MONITOR: Optics still closed)
- Both legacy dialogs (`Preferences`, `ScanConfig`) can be fully removed after Sprint 20

---

## Sprint 20 — QSS Theme + Signal Rewiring (2 weeks)

### Prerequisites
- Sprints 16–19 complete

### What it does
Applies the prototype's color system via new QSS. Completes signal rewiring for any remaining controllers. Removes dead code from legacy UI.

Main work:
- Port prototype CSS variables to QSS: backgrounds, borders, text, accents, semantic colors, TCU bar colors, lens colors
- Style all standard Qt widgets (QPushButton / QCheckBox / QComboBox / QSlider / QLineEdit / QTabWidget / QScrollBar / …) to match prototype
- Gradient headers and striped backgrounds via QSS where possible; fall back to `paintEvent` for effects QSS can't do (glows, complex gradients)
- Delete old `style_light.qss` / `style_dark.qss` (or keep toggle if decided)
- Signal audit: verify all 7 controllers (RF / Scan / AuxPanel / Lens / Laser / DeviceManager / TCU) are wired to new widgets; fix gaps
- Remove dead code: old TitleBar, StatusBar, Preferences/ScanConfig dialogs, unused `.ui` sections
- Translation sweep (`lupdate`), verify new widget strings are in `.ts` files

### End state
- Screenshot comparison with prototype passes informal review (~95% match)
- All controller flows work end-to-end: connect → grab → process → scan → record
- Legacy dialog code removed from repo
- Translation files updated and loadable

---

## Sprint 21 — Testing, Polish, Release (1 week)

### Prerequisites
- Sprint 20 complete

### What it does
Ship-ready quality. Catches regressions, polishes interactions, prepares release artifacts.

Main work:
- Full manual test pass: every controller, every mode, every floating panel interaction
- Resize/DPI testing across 1080p / 1440p / 4K
- Visual polish: spacing, alignment, hover states, focus rings
- Fix regressions found during testing
- Update in-code comments where behavior changed
- Final screenshot comparison with prototype; capture before/after for release notes
- Version bump to 0.11.0 and release notes

### End state
- No P0/P1 bugs outstanding
- Version tagged at 0.11.0
- Release notes written
- `ui-redesign` branch ready to merge back to `refactor` / `master`
- Prototype parity confirmed via side-by-side review

---

## Cross-sprint notes

### Branching
- Work on a dedicated `ui-redesign` branch off `refactor`
- Merge back after Sprint 21

### Fallback strategy
- Preferences and ScanConfig dialogs kept as fallback through Sprint 19
- Removed only in Sprint 20 after SETUP/SCAN overlays are proven stable

### Parallelization (if multiple developers)
- Sprint 15 widgets are independent and can split across developers
- Sprint 17 right-side widgets can run in parallel with Sprint 18 SETUP overlay
- QSS theming (Sprint 20) can start earlier in parallel with Sprints 17 & 18

### Deferred / discarded
See `others/ui_redesign/UI_REDESIGN_PLAN.md` Part 2 for the full list. Summary:
- Discarded: packet loss indicator, brightness/contrast/gamma sliders
- Deferred to future patches: record button, software trigger, auto-focus, rulers, second side display, Laser Control / Developer Options / Serial Server / Distance 3D View dialogs, etc.
