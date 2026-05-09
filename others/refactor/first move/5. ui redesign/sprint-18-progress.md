# Sprint 18 Progress: SETUP Overlay

## Prerequisite: Sprints 16, 17
Status: SATISFIED

Sprint 16 shipped `TopBar` with 3 mode tabs (Setup/Monitor/Scan). Sprint 17 left `ui->LEFT` in place because `PARAMS_STATIC` + `IMG_GRAB_STATIC` (gain / start-stop grabbing) had no home. Sprint 18 builds that home — the SETUP overlay — then dismantles `ui->LEFT` and surfaces `SideDisplayWidget` + `MapboxWidget`.

## Branch
All Sprint 18 work stays on `refactor`.

## Strategy

The legacy `Preferences` dialog has 187 named widgets across 5 top-level groups (`DEVICES_GRP`, `SERIAL_PORTS_GRP`, `TCU_GRP`, `IMG_PROC_GRP`, `SAVE_LOAD_GRP`). Recreating them all from scratch is out of scope; instead we **re-parent the 5 group frames** into `SetupOverlay`'s row layout. All existing `pref->ui->*` accessors and signal connections continue to work — the widgets keep their object identity, only their parent changes.

The legacy `Preferences` dialog remains technically accessible via the `SETTINGS` action in `TopBar`, but it becomes a near-empty shell (only widgets that didn't migrate stay inside it). A fallback flag in `Config` lets users force the legacy path if the overlay breaks. Removed in Sprint 20.

Sliced into 7 incremental tasks:

- 18.1 scaffolds `src/widgets/overlays/setupoverlay.{h,cpp}` and the `SetupOverlay` shell.
- 18.2 builds row 1 (Presets / Ports / Serial Ports / Save-Load / TCU Configuration) by re-parenting `SAVE_LOAD_GRP`, `SERIAL_PORTS_GRP`, `TCU_GRP` and adding the Preset section header.
- 18.3 builds row 2 (Devices / Camera) by re-parenting `DEVICES_GRP` from `Preferences` plus `PARAMS_STATIC` + `IMG_GRAB_STATIC` from `ui->LEFT`. Image Region and Sensor Taps stay as icon buttons exposed on the camera card.
- 18.4 builds row 3 (Image Processing) by re-parenting `IMG_PROC_GRP` and laying its sub-groups (`ACCUMULATIVE_GRP`, `ADAPTIVE_GRP`, `DCP_GRP`, `ECC_GRP`, `FISHNET_GRP`, `3D_MAPPING_GRP`) horizontally.
- 18.5 builds the preview sidebar: 3 stacked thumbnail `Display` widgets sharing camera frames with the existing `displays[]` array.
- 18.6 wires mode switching: `TopBar::modeChanged(Setup)` → `SetupOverlay::show()`; `Monitor`/`Scan` → `hide()`. Hides `ui->LEFT` while overlay is visible.
- 18.7 final cutover — `ui->LEFT` permanently hidden in MONITOR mode too; `SideDisplayWidget` + `MapboxWidget` surfaced via `setActive(true)` and `show()`.

## Layout
- Overlay class lives under `src/widgets/overlays/`.
- `SetupOverlay` is a child of `ui->CENTRAL`, fills the entire central area (above any other overlays in z-order while visible).
- Scrollable content via `QScrollArea` for window heights below the prototype's 1080-px target.

---

## Task 18.1: Scaffolding + SetupOverlay shell
Status: DONE

**Created:** [src/widgets/overlays/setupoverlay.h](src/widgets/overlays/setupoverlay.h) / [.cpp](src/widgets/overlays/setupoverlay.cpp).

**Public API:** `contentRow(idx)` returns the QWidget for row 0/1/2; `rowLayout(idx)` returns the QHBoxLayout for inserting cards; `previewSidebarLayout()` returns the QVBoxLayout for preview thumbnails.

**Layout:** root `QHBoxLayout` with a `QScrollArea` (left, stretches) and a fixed-width `QWidget` preview sidebar (right, 220 px). The scroll area hosts a vertical container with 3 named rows (`SETUP_ROW_1` / `SETUP_ROW_2` / `SETUP_ROW_3`); each row is a `QWidget` with a `QHBoxLayout` ready for cards. Preview sidebar shows a placeholder label until 18.5.

**[CMakeLists.txt](CMakeLists.txt):** appended `widgets/overlays/setupoverlay.{h,cpp}` after the sidebar block.

---

## Task 18.2: Row 1 — Presets / Ports / Serial Ports / Save-Load / TCU Configuration
Status: DONE (re-parents only — Preset card deferred)

**Landed:** [userpanel.cpp](src/visual/userpanel.cpp) SETUP overlay block re-parents `pref->ui->SAVE_LOAD_GRP`, `SERIAL_PORTS_GRP`, `TCU_GRP` into row 0 via `add_card` lambda (sets parent, clears .ui-baked geometry, addWidget into `rowLayout(0)`). Trailing stretch added so cards align left.

**Deferred:** the planned Preset card (named slots with rename / save / delete) is not yet on the overlay — `PresetPanel` is still accessed via the existing TopBar settings menu. Adding it later is purely additive: drop a new `QFrame` into row 0's layout.

**Wiring intact:** all `pref->ui->TCU_GRP` child widgets (e.g. `AB_LOCK_CHK`, `AUTO_REP_FREQ_CHK`) keep their object identity through re-parent; existing signal/slot connections continue working without changes.

---

## Task 18.3: Row 2 — Devices / Camera
Status: DONE

**Landed:** [userpanel.cpp](src/visual/userpanel.cpp) row-1 setup re-parents `pref->ui->DEVICES_GRP`, `ui->PARAMS_STATIC`, and `ui->IMG_GRAB_STATIC` into row 1 (in that order). PARAMS + IMG_GRAB sit side-by-side as the "camera" half of the row; `DEVICES_GRP` sits to their left.

`IMG_REGION_BTN` and `SENSOR_TAPS_BTN` ride along inside `IMG_GRAB_STATIC` as before — no separate handling needed.

---

## Task 18.4: Row 3 — Image Processing sub-groups
Status: DONE

**Landed:** `pref->ui->IMG_PROC_GRP` re-parented into row 2. Its existing internal layout (the `ACCUMULATIVE_GRP` / `ADAPTIVE_GRP` / `DCP_GRP` / `ECC_GRP` sub-groups plus 3D mapping widgets) is preserved by Qt's re-parent semantics — the whole subtree moves together. Trailing stretch added.

**Future polish:** if visual density warrants, the sub-groups could be lifted out of `IMG_PROC_GRP` and laid out directly in row 2 as side-by-side cards. Deferred to Sprint 20 QSS pass.

---

## Task 18.5: Preview sidebar — Main / ALT1 / ALT2 thumbnails
Status: DEFERRED

The SETUP overlay's preview sidebar is currently a placeholder `QLabel` ("Preview — populated in 18.5") added in 18.1. Wiring 3 live thumbnails sharing the existing `displays[]` array requires a thread-safe pixmap-tee (the pipeline already pushes frames to `displays[]` from the GrabThread; an additional `QImage::scaled` per-frame cost is small but needs measurement).

Deferred to a focused follow-up patch — does not block S18's core goal (every Preferences setting reachable in the SETUP overlay) and does not block S19/S20.

---

## Task 18.6: Mode switching wiring
Status: DONE

**Landed:** [userpanel.cpp](src/visual/userpanel.cpp) — `connect(ui->TITLE, &TopBar::modeChanged, …)` lambda toggles overlay visibility based on mode:
- `Setup` → `m_setup_overlay->show()`, raise + resize to `ui->CENTRAL->rect()`; right-stack, pbar, and all 3 floating panels hidden.
- `Monitor` / `Scan` → `m_setup_overlay->hide()`; right-stack and pbar restored; floating panels restored to **the visibility state stored in their corresponding pbar button** (per-panel persistence).

`Scan` mode currently behaves like `Monitor` from the overlay's perspective — Sprint 19 will add the SCAN overlay and refine the right-stack visibility there.

Per-panel persistence is implemented via `pbar_checked` lambda that reads each `PBAR_*_BTN`'s `isChecked()` state at the moment of mode change.

---

## Task 18.7: Final cutover — hide ui->LEFT permanently, surface SideDisplay + Mapbox
Status: DONE

**Landed:**
- `ui->LEFT->hide()` called once in the constructor (after the SetupOverlay block); permanent across all modes.
- `m_side_display->show()` and `m_mapbox->show()` called immediately after — they now occupy the left zone in MONITOR mode.
- [userpanel.cpp::resizeEvent](src/visual/userpanel.cpp): replaced `ui->LEFT->width()` driver with a fixed `left_zone_w = 220` constant. SideDisplay positioned at `(6, side_top, left_zone_w, side_h/2 - 4)`; Mapbox positioned below at `(6, side_top + side_h/2 + 4, left_zone_w, side_h/2 - 8)`. `m_setup_overlay` resizes to `ui->CENTRAL->rect()` when visible.
- `LASER_STATIC` remaining widgets stay inside the now-hidden `ui->LEFT` — `LASER_BTN`, `FIRE_LASER_BTN` already moved into `TCUPanel` in 16.7; the rest (`DISTANCE` overlay et al.) are unused in MONITOR mode and stay attached to LEFT until Sprint 20 cleanup.

---

## Definition of Done for Sprint 18

- [x] Pressing the SETUP mode tab in `TopBar` shows a full-window overlay with all device / TCU / camera / save settings.
- [x] Every setting that was reachable in `Preferences` dialog is reachable in SETUP overlay (re-parent preserves widget identity → all signal/slot connections continue).
- [x] Config file round-trips produce identical values as before — JSON layer is untouched.
- [x] `ui->LEFT` is permanently hidden; `SideDisplayWidget` + `MapboxWidget` are visible in MONITOR mode.
- [x] Mode switching SETUP ↔ MONITOR works via `TopBar::modeChanged` lambda.
- [~] Legacy `Preferences` dialog still opens via SETTINGS — but it's mostly an empty shell now (5 of its top-level groups re-parented out). Functional fallback if needed.
- [~] Preview sidebar deferred (placeholder label only — see 18.5).
- [x] Sprint 18 progress doc updated.

---

## Post-sprint bug fixes

### Fix 1: SETUP overlay rows appeared empty
**Root cause:** `add_card` called `card->setGeometry(QRect())`, resizing each card to (0, 0). `QGroupBox::sizeHint()` without an inner `QLayout` returns only the title-bar height, so the row `QHBoxLayout` collapsed every card to zero height. Only the group-box title strings were visible.

**Fix:** Capture `orig_size = card->size()` before `setParent`. Replace `setGeometry(QRect())` with `card->move(0, 0)` (clears baked position without touching size). For widgets with no `QLayout`, call `card->setMinimumSize(orig_size)` so the row layout respects the original .ui-designed dimensions.

### Fix 2: OpticsPanel could not be dragged
**Root cause:** `m_side_display`, `m_mapbox`, `m_right_stack`, and `m_pbar` were direct children of `this` (QMainWindow). Qt paints direct QMainWindow children on top of everything inside `ui->CENTRAL`; they also intercept all mouse events in the overlapping area. `OpticsPanel` starts at (10, 50) in CENTRAL which falls inside `m_side_display`'s left-zone rectangle, so every click on its header went to `SideDisplayWidget` instead. `TCUPanel` (x=300) and `ConsolePanel` (x=560) start outside the 220 px left zone and were unaffected.

**Fix:** Changed the four widgets' parent from `this` to `ui->CENTRAL` — they are now siblings of the floating panels. Added `raise()` calls on the floating panels immediately after `m_side_display->show()` / `m_mapbox->show()`, and in the modeChanged lambda whenever switching away from SETUP mode, so panels always sit above the side-zone widgets in CENTRAL's z-order.

---

## Out of scope for Sprint 18

- SCAN overlay — Sprint 19.
- QSS theming — Sprint 20.
- Removing `Preferences` / `ScanConfig` dialogs entirely — Sprint 20.
- Cross-mode input consistency audit — Sprint 21.
