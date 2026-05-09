# Sprint 19 Progress: SCAN Overlay

## Prerequisite: Sprints 16, 17, 18
Status: SATISFIED

Sprint 18 shipped the SETUP overlay and permanently hid `ui->LEFT`. `ui->RIGHT` was hidden in Sprint 17; its only surviving content (`SCAN_GRP` + `scan_config` dialog) was left orphaned behind the hidden column. SCAN mode produced no visual change from MONITOR mode. Sprint 19 builds the SCAN overlay that matches the prototype.html v3 layout.

## Branch
All Sprint 19 work stays on `refactor`.

## Strategy

The prototype specifies a **single horizontal row of 4 titled group cards** (Delay / Gate Width / PTZ / Capture+Record) plus a **90 px vertical action column** (Start/Continue/Restart), all inside a `QScrollArea`. A fixed 220 px right sidebar hosts Display Previews (deferred to 19.5).

`ScanConfig` is a flat `QDialog` with all widgets at absolute positions — no named sub-groups. Re-parenting the whole dialog as a card (as initially attempted) produces the legacy layout unchanged and doesn't match the prototype. Instead, individual `scan_config->ui->*` widgets are extracted and re-parented into the 4 new group cards with proper `QGridLayout` / `QHBoxLayout` sub-layouts.

The re-parent sequence follows the Sprint 18 pattern:
1. `w->setParent(new_parent)` — Qt automatically hides the widget on parent change.
2. `w->show()` — makes it visible in the new location.
3. `layout->addWidget(w)` — `QLayout::addChildWidget` detects `pw == mw` and skips re-parenting, so the widget stays visible.

`m`-mode widgets (`START_DELAY_EDIT_M` etc.) are not included in the new cards — they remain as children of the now-empty `scan_config` shell and will be handled in the Sprint 20 QSS / cleanup pass.

Sliced into 5 tasks:

- 19.1 scaffolds `src/widgets/overlays/scanoverlay.{h,cpp}` — single cards row + preview sidebar.
- 19.2–19.5 build cards + action column + mode wiring + deferred previews.

## Layout
- `ScanOverlay` lives under `src/widgets/overlays/`.
- Child of `ui->CENTRAL`; fills central area below TopBar (y=40).
- Horizontal `QScrollArea` (cards row, flex) + fixed 220 px preview sidebar.

---

## Task 19.1: Scaffolding + ScanOverlay shell
Status: DONE

**Created:** [src/widgets/overlays/scanoverlay.h](src/widgets/overlays/scanoverlay.h) / [.cpp](src/widgets/overlays/scanoverlay.cpp).

**Public API:**
- `contentWidget()` — the `QWidget` whose layout holds the cards row (use as `parent_w` target when calling `setParent` on widgets being re-parented in).
- `cardsLayout()` — the `QHBoxLayout`; callers `addWidget()` cards then `addStretch(1)`.
- `previewLayout()` — the `QVBoxLayout` of the 220 px right sidebar (task 19.5).

**Layout:** root `QHBoxLayout` with a horizontally-scrollable `QScrollArea` (left, stretches) and a fixed 220 px preview sidebar (right). The scroll area contains `m_cards_container` whose `QHBoxLayout` receives all cards + the action column at run time.

**[CMakeLists.txt](CMakeLists.txt):** appended `widgets/overlays/scanoverlay.{h,cpp}` after the setupoverlay block.

---

## Tasks 19.2–19.4: 4 Group Cards + Action Column + Mode Wiring
Status: DONE

**Landed:** [userpanel.cpp](src/visual/userpanel.cpp) — SCAN overlay block builds the following structure, matching prototype.html v3 §scan-ov:

### Delay card
`QGridLayout` with 5 columns (label | edit_μs | unit_μs | edit_ns | unit_ns):
- Starting: `START_DELAY_EDIT_U` / `UNIT_US_1` / `START_DELAY_EDIT_N` / `UNIT_NS_1`
- Ending: `END_DELAY_EDIT_U` / `UNIT_US_2` / `END_DELAY_EDIT_N` / `UNIT_NS_2`
- Separator + second grid for Step (`STEP_SIZE_DELAY_EDIT` / `UNIT_1`) and Rep Freq (`REP_FREQ_EDIT` / `FREQ_UNIT`).

### Gate Width card
Same 5-column `QGridLayout`:
- Starting: `START_GW_EDIT_U` / `UNIT_US_3` / `START_GW_EDIT_N` / `UNIT_NS_3`
- Ending: `END_GW_EDIT_U` / `UNIT_US_4` / `END_GW_EDIT_N` / `UNIT_NS_4`
- Separator + Step: `STEP_SIZE_GW_EDIT` / `UNIT_2`.

### PTZ card
Two sub-panels in an `QHBoxLayout`:
- Left: H/V `QGridLayout` with header row (`H` / `V`) and Starting / Ending / Step / Pos-count rows using `START_ANGLE_H/V_EDT`, `END_ANGLE_H/V_EDT`, `STEP_ANGLE_H/V_EDT`, `FRAME_H/V_EDT`.
- Right: Direction radio buttons (`RADIO_PTZ_DIRECTION_N` / `_Z`) + Wait field (`PTZ_WAIT_TIME_EDT`).

### Capture / Record card
Two sub-panels in a `QHBoxLayout`:
- Left: CAPTURE section (`CAPTURE_SCAN_ORI_CHK` / `_RES_CHK`) + RECORD section (`RECORD_SCAN_ORI_CHK` / `_RES_CHK`).
- Right: PROFILES label + `FRAME_COUNT_EDIT`; IMGS/PROFILE label + `NUM_SINGLE_POS_EDT`.

### Action column (90 px, `QVBoxLayout`)
- `SCAN_BUTTON` — re-parented and shown; text toggles "Scan"/"Stop" via `ScanController`.
- `CONTINUE_SCAN_BUTTON` — re-parented only (not forced show); `ScanController` owns visibility.
- `RESTART_SCAN_BUTTON` — same.

**Mode wiring:** `modeChanged` lambda updated with `scan_mode` + `overlay_mode` variables:
- `Scan` → `m_scan_overlay->show()`, raise + resize; right-stack, pbar, floating panels hidden.
- `Setup` → unchanged from Sprint 18.
- `Monitor` → both overlays hidden; right-stack/pbar/panels restored to pbar toggle state.

`resizeEvent` updated to resize `m_scan_overlay` when visible (same pattern as `m_setup_overlay`).

**`scan_config` object status:** remains as an empty shell (no visible children); all `data_exchange()` / `setup_hz()` / `setup_unit()` methods still work because they access child widgets via their still-valid pointers.

**m-mode limitation:** `START_DELAY_EDIT_M`, `END_DELAY_EDIT_M`, `START_GW_EDIT_M`, `END_GW_EDIT_M`, and their unit labels are **not** included in the new card layouts. If `scan_config->setup_unit(2)` is called, those widgets will appear at (0,0) on the empty `scan_config` shell (invisible). Deferred to Sprint 20 cleanup.

---

## Task 19.5: Display Previews sidebar
Status: DEFERRED

Right sidebar shows a placeholder label ("Display Previews — populated in 19.5"). Wiring 3 live thumbnails requires the pixmap-tee mechanism also deferred in Sprint 18.5. Deferred to Sprint 20.

---

## Definition of Done for Sprint 19

- [x] Pressing SCAN shows a full-window overlay with 4 titled group cards matching the prototype.html v3 layout.
- [x] All scan parameters (delay, gate-width, PTZ route, capture/record options) are always visible in the overlay.
- [x] `SCAN_BUTTON` (Start/Stop) wired to `ScanController`; `CONTINUE`/`RESTART` visibility managed by `ScanController::scan_options_changed`.
- [x] Mode switching SCAN ↔ MONITOR ↔ SETUP works; overlays are mutually exclusive.
- [x] Right-stack and pbar hidden in both overlay modes; restored on return to MONITOR.
- [~] m-mode (meter) widgets not included — legacy path deferred to Sprint 20 (see limitation above).
- [~] Display Previews sidebar deferred (placeholder only — see 19.5).
- [x] Sprint 19 progress doc updated.

---

## Out of scope for Sprint 19

- QSS theming — Sprint 20.
- Removing `Preferences` / `ScanConfig` dialogs entirely — Sprint 20.
- m-mode widget layout — Sprint 20.
- Display Previews sidebar — Sprint 20.
- Cross-mode input consistency audit — Sprint 21.
