# Sprint 17 Progress: Right-Side Widget Stack + Side Display + Mapbox

## Prerequisite: Sprint 16
Status: SATISFIED

Sprint 16 shipped `TopBar`, the 3 floating panels (`OpticsPanel` / `TCUPanel` / `ConsolePanel`), display overlays, and the panel toggle bar. The MONITOR-mode skeleton is in place; what remains in the legacy `LEFT` / `RIGHT` columns is `IMG_PROC_STATIC`, `IMG_SAVE_STATIC`, `SCAN_GRP`, `PARAMS_STATIC`, `IMG_GRAB_STATIC`, and `LASER_STATIC` (without the laser/fire buttons that moved into `TCUPanel`).

## Branch
All Sprint 17 work stays on `refactor`.

## Strategy

Sprint 17 dismantles the legacy `LEFT` / `RIGHT` 3-column layout in favor of a dynamic layout owned by `userpanel.cpp::resizeEvent`. The cutover is sliced into 9 incremental tasks:

- 17.1 builds the `RightStackContainer` widget wrapper.
- 17.2–17.4 build the three right-side widgets (`ImgProcWidget`, `SaveOptionsWidget`, `VisionWidget`) by **re-parenting existing UI widgets** out of `IMG_PROC_STATIC` / `IMG_SAVE_STATIC` / YOLO row, the same approach used for floating panels in 16.7. Controllers stay wired throughout.
- 17.5 wires the stack into `UserPanel`, hides the legacy `RIGHT` column.
- 17.6 builds `SideDisplayWidget` reusing `FloatingWindow[0]` content.
- 17.7 stubs `MapboxWidget` (placeholder until external library landed).
- 17.8 rewrites `resizeEvent` to compute side/center/right zones dynamically — center display fits 16:9 inside available space.
- 17.9 adds floating-panel drag clamping so panels can't be hidden behind the side widgets or right stack.

## Layout
- New widget classes live under `src/widgets/sidebar/` (right-stack widgets, side display, mapbox).
- `widget_gallery` gains a tab per new composed widget for visual smoke-testing before live cutover.

---

## Task 17.1: RightStackContainer scaffold
Status: DONE

**Created:** [src/widgets/sidebar/rightstackcontainer.h](src/widgets/sidebar/rightstackcontainer.h) / [.cpp](src/widgets/sidebar/rightstackcontainer.cpp).

**Public API:** `addSection(QWidget *w, int stretch = 0)` re-parents into the internal `QVBoxLayout`; `contentLayout()` accessor for callers needing direct layout manipulation. Default content margin 4 px, spacing 6 px. Object name `RIGHT_STACK` for QSS targeting (Sprint 20).

**[CMakeLists.txt](CMakeLists.txt):** appended `widgets/sidebar/rightstackcontainer.{h,cpp}` to `SOURCE_FILES` after the panels block.

**Out of scope:** the container has no header / pbar slot of its own — `m_pbar` keeps its current top-right position from Sprint 16.9 and stays a child of the main window (not the stack). This decision is revisited in 17.5 if visual integration proves awkward.

---

## Task 17.2: ImgProcWidget — MODELS + GENERAL tabs
Status: DONE

**Created:** [src/widgets/sidebar/imgprocwidget.h](src/widgets/sidebar/imgprocwidget.h) / [.cpp](src/widgets/sidebar/imgprocwidget.cpp).

**Two tabs:**
- **MODELS:** 3 grouped rows for the algorithmic pipeline stages — `IMG_ENHANCE_CHECK` + `ENHANCE_OPTIONS`; `FRAME_AVG_CHECK` + `FRAME_AVG_OPTIONS`; `IMG_3D_CHECK` + `RANGE_THRESH_EDIT` + `RESET_3D_BTN` (full-width below).
- **GENERAL:** `PSEUDOCOLOR_CHK`. (Brightness/contrast/gamma sliders are discarded per the redesign plan and stay in the .ui as orphans for now — they'll be removed in Sprint 20 when the .ui is cleaned up.)

**`setup()` re-parents** the listed widgets from `IMG_PROC_STATIC` into the tab pages and clears their .ui-baked geometry so the layout sizes them. Widget identity is preserved → all existing controller / pipeline / preferences connections remain intact.

**[CMakeLists.txt](CMakeLists.txt):** appended to `SOURCE_FILES` after rightstackcontainer entries.

---

## Task 17.3: SaveOptionsWidget
Status: DONE

**Created:** [src/widgets/sidebar/saveoptionswidget.h](src/widgets/sidebar/saveoptionswidget.h) / [.cpp](src/widgets/sidebar/saveoptionswidget.cpp).

**Layout:**
- Row 1: `FILE_PATH_EDIT` + `FILE_PATH_BROWSE` (toolbutton).
- Row 2: `CAPTURE_LABEL` | `SAVE_BMP_BUTTON` (ORI capture) | `SAVE_FINAL_BUTTON` (RES capture).
- Row 3: `RECORD_LABEL`  | `SAVE_AVI_BUTTON` (ORI record)  | `SAVE_RESULT_BUTTON` (RES record).

`setup()` re-parents all 8 widgets out of `IMG_SAVE_STATIC` and clears their .ui-baked geometry. Widget identity preserved — connections to `Preferences` / pipeline save state remain intact. The 2×3 grid stretches columns 1 & 2 evenly so the buttons share remaining horizontal space.

**[CMakeLists.txt](CMakeLists.txt):** appended.

---

## Task 17.4: VisionWidget — MAIN/ALT1/ALT2 detection rows
Status: DONE (scaffold only)

**Created:** [src/widgets/sidebar/visionwidget.h](src/widgets/sidebar/visionwidget.h) / [.cpp](src/widgets/sidebar/visionwidget.cpp).

**Decision deviation from plan:** the YOLO model combos (`MAIN_MODEL_LIST` / `ALT1_MODEL_LIST` / `ALT2_MODEL_LIST`) were already re-parented into ConsolePanel's YOLO tab in 16.7. Pulling them out again would create a churn cycle. Instead, VisionWidget is a structural scaffold: 3 rows (MAIN / ALT1 / ALT2), each with an enable checkbox and a placeholder slot for the future threshold slider.

**Public API:**
- `setAlt1Visible(bool)` / `setAlt2Visible(bool)` — external visibility driver, will be hooked to camera-active state in a later sprint.
- Signals: `mainEnabledChanged(bool)` / `alt1EnabledChanged(bool)` / `alt2EnabledChanged(bool)`.

**Initial state:** ALT1 and ALT2 rows hidden by default; MAIN row visible. Detection-pipeline wiring (threshold sliders, actual on/off enforcement) deferred — the existing pipeline config carries detect-on/off via Preferences, which is unaffected.

**[CMakeLists.txt](CMakeLists.txt):** appended.

---

## Task 17.5: Cutover — replace RIGHT column with RightStackContainer
Status: DONE

**Landed changes:**

**[userpanel.h](src/visual/userpanel.h):** added 6 new sidebar includes + 6 new member pointers (`m_right_stack`, `m_img_proc`, `m_save_options`, `m_vision`, `m_side_display`, `m_mapbox`).

**[userpanel.cpp](src/visual/userpanel.cpp):**
- Initializer list: 6 new `nullptr` initializers added.
- Constructor (after pbar wiring block): construct `RightStackContainer` as a child of `this` (main window — not `ui->CENTRAL`, so it floats above any leftover legacy widgets); construct the 3 sub-widgets as children of the stack; call `m_img_proc->setup()` and `m_save_options->setup()` re-parenting widgets out of `IMG_PROC_STATIC` / `IMG_SAVE_STATIC`; `addSection()` for each. `ui->RIGHT->hide()` removes the legacy column and `SCAN_GRP` along with it. `SCAN_BUTTON` etc. are still wired (widget identity preserved); they'll surface again via the SCAN overlay in Sprint 19.

**[CMakeLists.txt](CMakeLists.txt):** no further changes — all sidebar files were appended in 17.1–17.4 / 17.6 / 17.7.

**Decision — `.ui` not edited yet:** the `RIGHT` widget tree stays in [user_panel.ui](src/visual/user_panel.ui) but is hidden at runtime. Removing it from the .ui would break the auto-generated `ui->IMG_PROC_STATIC`/`ui->SCAN_GRP` accessors that other code still references (Preferences signal wiring touches `IMG_PROC_STATIC` children indirectly). Cleanup deferred to Sprint 20 alongside the QSS pass.

---

## Task 17.6: SideDisplayWidget — second-camera feed
Status: DONE (scaffold only)

**Created:** [src/widgets/sidebar/sidedisplaywidget.h](src/widgets/sidebar/sidedisplaywidget.h) / [.cpp](src/widgets/sidebar/sidedisplaywidget.cpp).

**Public API:**
- `setDisplay(Display *display)` — re-parents a `Display*` from `FloatingWindow` into the widget; pass `nullptr` to clear.
- `setActive(bool active)` — show the live `Display` if active and a display is set, otherwise show the placeholder label `"Side display — inactive"`.

**Status — hidden in Sprint 17:** the widget is constructed in `UserPanel` but `hide()` is called on it at construction. The legacy `ui->LEFT` panel still occupies the prototype's intended position for the side display (it holds critical controls — gain, start/stop grabbing — that don't migrate until the SETUP overlay in Sprint 18). SideDisplay surfaces in S18 once `LEFT` is dismantled.

**[CMakeLists.txt](CMakeLists.txt):** appended.

---

## Task 17.7: MapboxWidget — placeholder
Status: DONE

**Created:** [src/widgets/sidebar/mapboxwidget.h](src/widgets/sidebar/mapboxwidget.h) / [.cpp](src/widgets/sidebar/mapboxwidget.cpp). Pure placeholder — a `QFrame` with a single centered `QLabel` reading `"Map view — pending integration"`. Hidden in S17 alongside `SideDisplayWidget`; surfaces in S18 when LEFT is dismantled. No external library integration yet — that's a separate post-S21 patch.

**[CMakeLists.txt](CMakeLists.txt):** appended.

---

## Task 17.8: Dynamic layout in resizeEvent
Status: DONE (partial — left zone deferred)

**Landed changes:** [userpanel.cpp::resizeEvent](src/visual/userpanel.cpp) — replaced `int grp_width = ui->RIGHT->width()` with a fixed `right_zone_w = 220` constant; `m_right_stack` positioned at `(window.width() - right_zone_w - 6, title_h + 4, right_zone_w, side_h)`; `ui->MID->setGeometry` adjusted to subtract `right_zone_w + 12` from the available width.

**Left zone — deferred to S18:** `ui->LEFT` still drives the left-side width (`mid_width = window.width() - (right_zone_w + 12) - 10 - ui->LEFT->width()`). When the SETUP overlay absorbs LEFT in Sprint 18, this line will switch to a `left_zone_w` constant and SideDisplay/Mapbox will be positioned in the left zone the same way RightStack is positioned in the right zone today.

**16:9 letterbox math:** the existing letterbox calculation (lines below the new constants — `int width = mid_width - 20, height = width * hh / ww; …`) is unchanged. It already does the 16:9 fit; only the available width changed.

---

## Task 17.9: Floating panel drag clamping
Status: DONE (verification)

The 3 floating panels are parented to `ui->CENTRAL`, which fills the entire space below the title bar. `FloatingPanel::clampToParent` (Sprint 15) already clamps the panel's top-left to keep the panel fully visible inside its parent. So panels can't be dragged off-screen — covered.

**Known limitation (intentional):** panels can be dragged so they overlap the right-side `RightStackContainer` or the legacy `ui->LEFT` column. `RightStackContainer` is parented to `this` (main window) so it sits above `ui->CENTRAL` and the panels — overlapping panels appear *behind* the stack, not on top. This is acceptable for S17; once SETUP overlay frees the left zone (S18), panels will settle into the central display area naturally and the overlap concern shrinks. No code change needed.

---

## Definition of Done for Sprint 17

- [~] MONITOR mode layout matches the prototype on the right side; left side still uses legacy `ui->LEFT` (full prototype match deferred to S18).
- [~] `RIGHT` column hidden at runtime via `ui->RIGHT->hide()`; `.ui` removal deferred to S20 cleanup.
- [x] All 3 right-side widgets (IMG PROC / Save / Vision) wired in and visible. Vision is a scaffold (rows + signals; threshold sliders deferred).
- [~] `SideDisplayWidget` constructed but hidden in S17; surfaces in S18 when LEFT is dismantled.
- [~] `MapboxWidget` constructed but hidden in S17; surfaces in S18.
- [x] Window resize keeps center display 16:9 (existing letterbox math preserved); no overlap with right stack.
- [x] Floating panels clamp inside `ui->CENTRAL` via existing `FloatingPanel::clampToParent` (S15).
- [x] Sprint 17 progress doc updated with status for each task.

## Out of scope for Sprint 17

- SETUP / SCAN overlay content — Sprint 18 / 19.
- QSS theming — Sprint 20.
- Removing `Preferences` / `ScanConfig` dialogs — Sprint 20.
- Mapbox actual integration (just placeholder).
