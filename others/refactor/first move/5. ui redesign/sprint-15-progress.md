# Sprint 15 Progress: Custom Widget Infrastructure

## Prerequisite: Sprint 14
Status: SATISFIED

Sprint 14 decoupled the 7 controllers from `Ui::UserPanel*` / `Preferences*`. Widgets no longer have hard dependencies on the old `.ui` layout, so new widgets can be introduced without touching controller code.

## Branch
All Sprint 15 work stays on `refactor`. No separate `ui-redesign` branch.

## Layout
- New widgets live under `src/widgets/` (alongside existing `display`, `floatingwindow`, `titlebar`, `statusbar`, `mywidget`).
- Unit tests under `tests/` following the existing `test_<topic>.cpp` convention and CMake `BUILD_TESTS` block.
- Gallery app under `tests/widget_gallery/` as a separate executable.

---

## Task 15.1: Scaffolding — directory + CMake wiring
Status: DONE

**Approach:** incremental. No pre-allocated empty shells; each widget appends to two places in `CMakeLists.txt` when it lands — one in `SOURCE_FILES` (main app) and one in the new Tier 3 block (its test). This keeps the build graph honest — no target references files that don't exist yet.

**Changes to [CMakeLists.txt](CMakeLists.txt):**
- Main target `SOURCE_FILES` (line ~142): new widget `.h/.cpp` pairs are inserted alongside existing `src/widgets/*` entries (e.g. `floatingpanel.h/.cpp` added at line 159–160 in task 15.2).
- `BUILD_TESTS` block: added a new "Tier 3: UI widget tests (Qt::Widgets required)" section after `test_rangefinder`. Each widget test appends one `add_executable` + `target_include_directories` + `target_link_libraries` + `add_test` block here, mirroring the existing `test_ptz` / `test_lens` pattern but linking only `Qt::Test` + `Qt::Core` + `Qt::Widgets`.
- `src/widgets/` was already on the include path — no include change needed.

**Deferred to Task 15.9:** the `widget_gallery` executable. Created when there are widgets to display; empty-shell target would be useless.

---

## Task 15.2: FloatingPanel (base class)
Status: DONE

**Created:** [src/widgets/floatingpanel.h](src/widgets/floatingpanel.h), [.cpp](src/widgets/floatingpanel.cpp), [tests/test_floatingpanel.cpp](tests/test_floatingpanel.cpp).
**Modified:** [CMakeLists.txt](CMakeLists.txt) — widget sources appended to `SOURCE_FILES`; new Tier 3 block adds `test_floatingpanel`.

**Public API:** `setTitle/title`, `setPinned/isPinned`, `contentLayout()` for subclass content, `static clampToParent(desired, panelSize, parentSize)` as a pure function. Signals: `pinToggled(bool)`, `hideRequested()`, `dragged(QPoint)`.

**Behavior:**
- Header (fixed 28 px) holds title label + pin button (checkable) + hide button. Labels are placeholder text; styled in Sprint 20 via exposed object names (`FloatingPanelHeader`, `FloatingPanelTitle`, `FloatingPanelPin`, `FloatingPanelHide`).
- Any mouse press in the panel calls `raise()` — z-order lift on focus.
- Drag starts only when left-press is on the header, not pinned, and not on a header button. Uses `globalPos` mapped through `parentWidget()` (DPI/multi-monitor handled by Qt).
- Movement clamped via `clampToParent`; degenerate parent-smaller-than-panel pins to origin.
- `dragged(QPoint)` emitted only when `pos()` actually changes.
- `hideRequested()` emitted on hide-button click; widget calls `hide()` itself.
- Transparency not applied — deferred to Sprint 20 QSS.

**Tests ([test_floatingpanel.cpp](tests/test_floatingpanel.cpp)):**
| Test | Verifies |
|---|---|
| `clamp_inside_parent` | `clampToParent` returns desired point inside, clamps to 0 / `parent - panel` at edges. |
| `clamp_parent_smaller_than_panel` | Degenerate case → result is origin. |
| `drag_moves_panel` | Press + move + release → `pos()` equals `(target - press_offset)`; `dragged()` emitted. |
| `pin_disables_drag` | After `setPinned(true)`, identical drag sequence leaves `pos()` unchanged. |
| `hide_emits_signal` | Click hide button → `hideRequested()` fires once + `isHidden()` true. |
| `pin_toggle_signal` | Click pin button twice → `pinToggled(true)` then `pinToggled(false)`. |
| `click_raises_z_order` | Two overlapping sibling panels; click lower → it becomes last in `findChildren` list (Qt's z-order). |

---

## Task 15.3: TimingBarWidget
Status: DONE

**Created:** [src/widgets/timingbarwidget.h](src/widgets/timingbarwidget.h), [.cpp](src/widgets/timingbarwidget.cpp), [tests/test_timingbarwidget.cpp](tests/test_timingbarwidget.cpp).
**Modified:** [CMakeLists.txt](CMakeLists.txt) — widget sources appended to `SOURCE_FILES`; `test_timingbarwidget` added in Tier 3 block.

**Public API:** `BarId` enum (`LaserWidth`, `Distance`, `Dov`, `Mcp`, `BarCount`); per-bar `setBarValue` / `barValue`, `setBarScale` / `barScale`, `setBarLabel`, `setBarUnit`, `setBarColor`; `static pixelToValue` / `valueToPixel` as pure helpers. Signal: `barValueChanged(BarId, quint64 value)`.

**Behavior:**
- 4 stacked horizontal bars. Each bar's start is fixed at 0 (the trigger origin) and is not draggable — only the right edge (end) can be dragged.
- Each bar has its own scale, label, and unit string. Defaults: `LaserWidth` (0–1000 ns), `Distance` (0–5000 m), `DOV` (0–1000 m), `MCP` (0–255, unitless). Left strip renders `"<Label>: <value> <unit>"`.
- Widget stays dumb: renders raw values. Normal-mode B-follows-A offset rule is enforced by TCUController.
- End-handle hit-zone is ±8 px around the right edge; cursor switches to `SizeHorCursor` on hover.
- `setBarValue` clamps to that bar's scale; `setBarScale` shrinking below current value re-clamps and emits once for the affected bar.
- `barValueChanged` emitted only when the stored value actually changes.
- Placeholder per-bar colors (blue / green / cyan / lime) — restyled in Sprint 20.

**Tests ([test_timingbarwidget.cpp](tests/test_timingbarwidget.cpp)):**
| Test | Verifies |
|---|---|
| `pixel_value_roundtrip` | `valueToPixel` then `pixelToValue` recovers original within 1 unit/px tolerance across the scale. |
| `pixel_value_degenerate` | Zero width or zero scale returns 0 — no div-by-zero. |
| `pixel_saturates_at_scale` | `valueToPixel(v ≥ scale, …)` returns `widthPx`. |
| `setBarValue_clamps_to_scale` | Value above the bar's scale is clamped to scale. |
| `setBarValue_signal_only_on_change` | Identical re-write does not re-emit. |
| `bars_are_independent` | Writing one bar leaves the other three untouched and does not emit for them. |
| `setBarScale_clamps_existing_value` | Shrinking a bar's scale clamps its value and emits exactly once for that bar. |
| `scales_are_independent` | Changing one bar's scale does not affect others or emit value-change signals. |

---

## Task 15.4: JoystickWidget
Status: DONE

**Created:** [src/widgets/joystickwidget.h](src/widgets/joystickwidget.h), [.cpp](src/widgets/joystickwidget.cpp), [tests/test_joystickwidget.cpp](tests/test_joystickwidget.cpp).
**Modified:** [CMakeLists.txt](CMakeLists.txt) — widget sources appended to `SOURCE_FILES`; `test_joystickwidget` added in Tier 3 block.

**Public API:** `setDeadzone(fraction)` / `deadzone()`, `direction()` returns current `(dx, dy)`. Signal: `directionChanged(double dx, double dy)`. Pure statics: `clampToCircle(v, radius)`, `computeDirection(stickLocal, center, radius, deadzoneFrac)`.

**Behavior:**
- Output is continuous `(dx, dy)` in `[-1, +1]`, each axis. No enum — Classic-mode direction buttons continue to use the existing `IPTZController::Direction` at [src/port/iptzcontroller.h:14](src/port/iptzcontroller.h#L14).
- Press + drag moves the stick cap, visually clamped to the base radius. Release snaps the stick back to center and emits `(0, 0)`.
- Deadzone: inside `deadzoneFrac × radius` from center, reported direction is `(0, 0)`. Default deadzone 0.1. Clamped to `[0, 1)`.
- `directionChanged` emitted only when the reported `(dx, dy)` actually changes.
- Base radius derived from widget size (`min(w, h) / 2 - 2` px). Stick cap is 25% of base radius.

**Tests ([test_joystickwidget.cpp](tests/test_joystickwidget.cpp)):**
| Test | Verifies |
|---|---|
| `clamp_passthrough` | Vector shorter than radius is returned unchanged. |
| `clamp_shrinks_to_radius` | Vector longer than radius is reduced to radius, same direction. |
| `clamp_zero_radius` | Degenerate: zero radius → zero vector. |
| `direction_center_is_zero` | Stick at center → `(0, 0)`. |
| `direction_right_edge_is_plus_x` | Stick on right edge → `(~+1, ~0)`. |
| `direction_clamped_to_unit_magnitude` | Stick far beyond base → magnitude saturates at 1. |
| `direction_deadzone_reports_zero` | Inside deadzone radius → `(0, 0)`. |
| `direction_just_outside_deadzone` | Just past deadzone → non-zero, y≈0 along +x axis. |
| `release_snaps_center_and_emits_zero` | Press + drag + release → last signal is `(0, 0)` and `direction()` is `(0, 0)`. |

---

## Task 15.5: ArcSliderWidget
Status: DONE

**Created:** [src/widgets/arcsliderwidget.h](src/widgets/arcsliderwidget.h), [.cpp](src/widgets/arcsliderwidget.cpp), [tests/test_arcsliderwidget.cpp](tests/test_arcsliderwidget.cpp).
**Modified:** [CMakeLists.txt](CMakeLists.txt) — widget sources appended to `SOURCE_FILES`; `test_arcsliderwidget` added in Tier 3 block.

**Public API:** `setRange(min, max)` / `minimum()` / `maximum()`, `setValue(int)` / `value()`. Signal: `valueChanged(int)`. Pure statics: `pixelToValue(y, trackHeight, min, max)`, `valueToPixel(value, trackHeight, min, max)`.

**Behavior:**
- Convention matches `QSlider` vertical: top = max, bottom = min.
- Value↔pixel mapping is 1D linear over the track height (`height - topMargin - bottomMargin`). The arc is purely cosmetic — `paintEvent` draws a curved track via `QPainterPath::cubicTo` with a 22 px horizontal bulge.
- Thumb is positioned on the arc itself: for this Bézier (P1.x = P2.x = cx + bulge), the x-offset from the centerline reduces to `3·t·(1−t)·bulge`, where `t = (thumb_y − top) / trackHeight`.
- Track renders dim; the portion from the bottom up to the thumb is re-stroked in accent color under a `setClipRect` so the active region is visible. Thumb has a white outline for contrast.
- Press + drag anywhere on the widget moves the thumb to the y-coordinate under the cursor (click-to-position covered by the same path).
- `setValue` clamps to `[min, max]` and emits only when the stored value changes.
- `setRange` with a narrower range re-clamps the current value and emits once if affected.
- Degenerate inputs (zero track height, `min == max`) return safe defaults — no div-by-zero.

**Tests ([test_arcsliderwidget.cpp](tests/test_arcsliderwidget.cpp)):**
| Test | Verifies |
|---|---|
| `value_pixel_roundtrip` | `valueToPixel` → `pixelToValue` recovers every integer value across the full range. |
| `top_is_max_bottom_is_min` | Convention: `y=0 → max`, `y=trackHeight → min`. |
| `pixel_out_of_bounds_clamps` | Negative or oversized y clamps to max / min respectively. |
| `pixel_degenerate` | Zero height / `min == max` return safe values. |
| `setValue_clamps_below_min` | `setValue(-50)` on `[0, 100]` → value is 0. |
| `setValue_clamps_above_max` | `setValue(9999)` on `[0, 100]` → value is 100. |
| `setValue_signal_only_on_change` | Identical re-writes do not re-emit; out-of-range requests that resolve to current value do not re-emit. |
| `setRange_reclamps_value` | Shrinking range below current value re-clamps and emits once. |

---

## Task 15.6: DotIndicator
Status: DONE

**Created:** [src/widgets/dotindicator.h](src/widgets/dotindicator.h), [.cpp](src/widgets/dotindicator.cpp).
**Modified:** [CMakeLists.txt](CMakeLists.txt) — widget sources appended to `SOURCE_FILES`.

**Public API:** `State` enum (`Off`, `Ok`, `Warn`, `Err`), `setState(State)` / `state()`, `setColorForState(State, QColor)`. `Q_ENUM` + `Q_PROPERTY` for QSS targeting.

**Behavior:**
- Fixed 12×12 px. Draws an antialiased filled circle using the per-state color.
- `Ok` state adds a soft radial-gradient glow at 140 alpha around the dot core.
- `setState` calls `update()` only when the state actually changes.
- Default colors: `Off` grey, `Ok` green, `Warn` amber, `Err` red. Override per state via `setColorForState` so Sprint 20 QSS/theme can remap without subclassing.

**Tests:** none — pure rendering. Verified via widget gallery (Task 15.9).

---

## Task 15.7: UnlockBar
Status: DONE

**Created:** [src/widgets/unlockbar.h](src/widgets/unlockbar.h), [.cpp](src/widgets/unlockbar.cpp), [tests/test_unlockbar.cpp](tests/test_unlockbar.cpp).
**Modified:** [CMakeLists.txt](CMakeLists.txt) — widget sources appended to `SOURCE_FILES`; `test_unlockbar` added in Tier 3 block.

**Public API:** `setUnlockThreshold(fraction)` / `unlockThreshold()` (default 0.9), `setRelockMs(ms)` / `relockMs()` (default 3000; 0 disables auto-relock), `isUnlocked()`. Slot: `relock()`. Signals: `unlocked()`, `locked()`.

**Behavior:**
- Gates the laser-ON transition only. Turning the laser OFF bypasses the bar — the caller (LaserController) handles that, so the widget has no awareness of laser state.
- Thumb starts at the track-left. Press inside the thumb starts a drag; release decides the outcome:
  - If final position past `threshold × max_travel` → snap to right end, emit `unlocked()` once (repeated gestures while already unlocked do not re-emit), start auto-relock timer if `relockMs > 0`.
  - Else → spring back to left, no signal.
- `relock()` slot forces locked state: stops the timer, returns the thumb, emits `locked()` only if the state actually changed. Auto-relock timeout follows the same path.
- Striped amber-on-black background painted as 45° polygons; thumb turns green when unlocked. Placeholder visuals — styled in Sprint 20.

**Tests ([test_unlockbar.cpp](tests/test_unlockbar.cpp)):**
| Test | Verifies |
|---|---|
| `drag_past_threshold_unlocks` | Full-width drag → `unlocked()` fires once, `isUnlocked()` true. |
| `drag_short_springs_back` | Drag to ~40% → no signal, state still locked. |
| `auto_relock` | After unlock with `relockMs=50`, state returns to locked and `locked()` fires. |
| `unlocked_signal_single_fire` | Second unlock gesture while already unlocked does not re-emit `unlocked()`. |
| `manual_relock_slot` | `relock()` emits `locked()` and cancels the pending auto-relock timer (no second `locked()` after the window passes). |

---

## Task 15.8: Center display toolbar — NO dedicated widget class
Status: DEFERRED TO SPRINT 16

The SEL / ZOOM / PTZ + INFO toolbar is 3 exclusive buttons + 1 checkbox. A dedicated widget class adds no value — it will be composed inline in Sprint 16 from a `QHBoxLayout` + `QButtonGroup` sitting directly on the display overlay layer. No Sprint 15 deliverable.

---

## Task 15.9: Widget Gallery app
Status: DONE

**Created:** [tests/widget_gallery/main.cpp](tests/widget_gallery/main.cpp).
**Modified:** [CMakeLists.txt](CMakeLists.txt) — `widget_gallery` executable added to the Tier 3 block, linked against `Qt::Core` + `Qt::Gui` + `Qt::Widgets` plus the 6 widget sources. Not registered with `ctest` — launch manually.

**Layout:** single `QMainWindow` with a `QTabWidget`; one tab per widget:
- **FloatingPanel** — two panels on a dark canvas so drag, pin, hide, and z-order-on-click are all visible. `Show hidden panels` button re-shows anything hidden.
- **TimingBar** — all four bars pre-populated (LaserWidth / Distance / DOV / MCP) with appropriate scales and units; log pane prints `barValueChanged` events.
- **Joystick** — centered joystick with a live `(dx, dy)` readout and signal log.
- **ArcSlider** — three sliders labelled ZOOM / FOCUS / IRIS, each wired to the log.
- **DotIndicator** — 6 named status rows (TCU / Camera / Laser / Lens / PTZ / Rangefinder); `Cycle all states` button walks through Off → Ok → Warn → Err.
- **UnlockBar** — bar + state label + `QSpinBox` for auto-relock ms + `relock()` button; signal log.

Not shipped — only built when `BUILD_TESTS=ON`.

---

## Task 15.10: Unit test executable wiring
Status: TODO

**Scope:** CMake `BUILD_TESTS` block additions + a single `test_widgets` suite executable (or per-widget executables following existing convention).

**Decision:** follow existing convention of one executable per tested unit (`test_ptz`, `test_lens`, …). So:
- `test_floatingpanel`
- `test_timingbarwidget`
- `test_joystickwidget`
- `test_arcsliderwidget`
- `test_unlockbar`

Each links `Qt::Test` + `Qt::Widgets` + the widget source files it tests (no main-app dependency).

DotIndicator intentionally has no test executable — gallery covers it. The center toolbar has no widget class and therefore no test.

**End state:** `ctest` lists 5 new tests alongside the 8 existing ones; all pass.

---

## Definition of Done for Sprint 15

- [x] 6 widget classes under `src/widgets/` compile clean: FloatingPanel, TimingBarWidget, JoystickWidget, ArcSliderWidget, DotIndicator, UnlockBar.
- [x] Main app still launches and behaves exactly as before (no UI rewire yet — Sprint 16's job).
- [x] `widget_gallery` executable launches; each widget renders and responds to input.
- [x] 5 new `test_*` executables build and pass under `BUILD_TESTS=ON`.
- [x] FloatingPanel transparency recorded in the header comment — cross-platform QSS `rgba` is the chosen approach; actual styling deferred to Sprint 20.
- [ ] Sprint 15 work committed on `refactor`; progress doc updated with `Status: DONE` for each task.

## Out of scope for Sprint 15

- Actual panel composition (OpticsPanel / TCUPanel / ConsolePanel subclasses) — Sprint 16.
- Wiring widgets to controllers — Sprint 16/17.
- Center display toolbar composition (SEL / ZOOM / PTZ + INFO) — Sprint 16, built inline, no widget class.
- QSS theming pass — Sprint 20.
- Any changes to `userpanel.cpp`, `user_panel.ui`, or controller code.
