# UI Redesign Plan

## Overview
Redesign the GLI_user_panel main window from a fixed 3-column layout to a display-centric layout with floating panels, mode-based tabs, and a modernized dark theme. Based on prototype.html v3 (updated) — now considered the final target for this version.

---

## Part 1: Conversion — Old UI to New UI

### Phase 1: Structural Overhaul

#### 1.1 Remove fixed 3-column layout
- **Remove**: LEFT (210px), MID, RIGHT (240px) frames from user_panel.ui
- **Replace with**: Single full-window container holding main display + overlay widgets
- The .ui file will be largely rewritten or reduced to a minimal shell; most layout moves to code

#### 1.2 Main Display Area
- SOURCE_DISPLAY becomes the dominant widget, filling available space (16:9 fitted)
- **Stays**: Crosshair overlay, fishnet result indicator
- **FPS readout**: display bottom-left corner
- **Coordinate readouts**: display top-left corner — full set (START, SHIFT, SHAPE from START_COORD, SHIFT_INFO, SHAPE_INFO)
- **Distance readout**: display top-right corner
- **Keep but defer to Part 2**: Rulers (RULER_H, RULER_V)

#### 1.3 Top Bar (replaces TitleBar + StatusBar)
- **From TitleBar**: Min/Max/Exit buttons stay
- **From TitleBar**: URL, CLS, CAP (capture), theme toggle, language toggle, settings (⚙️) buttons — right side cluster
- **From StatusBar**: cam/tcu/lens/laser/ptz status → colored dots with tooltips (left side)
- **From StatusBar**: pixel depth readout, image resolution readout — Consolas-style text readouts next to action buttons
- **New**: System selector dropdown, mode tabs (SETUP/MONITOR/SCAN), clock display
- **Remove**: Old TitleBar button row, old StatusBar widget bar, wavelength display
- **Discarded** (will not be implemented): packet loss indicator

#### 1.3.1 Center Display Toolbar (new)
Floating horizontal toolbar centered at top of display area:
- **Tool selectors**: SEL / ZOOM / PTZ buttons (from SELECT_TOOL, ZOOM_TOOL, PTZ_TOOL custom widgets)
- **INFO checkbox**: toggle display overlay info (from INFO_CHECK)

#### 1.3.2 Panel Toggle Bar (pbar)
- Positioned top-right of window, above the IMG PROC widget (stacked aligned)
- 3 toggle buttons: Optics & PTZ / TCU & Laser / Console
- Shows/hides the three floating panels

#### 1.4 Floating Panel Framework (new infrastructure)
Create a reusable FloatingPanel QWidget base class with:
- Draggable header bar (grab to move)
- Pin/unpin toggle (pinned = fixed position, unpinned = free drag)
- Show/hide with toggle buttons in panel bar
- Close button
- Z-order management
- Semi-transparent background

#### 1.5 Floating Panel: Optics & PTZ (bottom-left)
- **From LENS_STATIC**: ZOOM_IN/OUT_BTN, FOCUS_NEAR/FAR_BTN, RADIUS_INC/DEC_BTN, ZOOM_EDIT, FOCUS_EDIT, FOCUS_SPEED_SLIDER
- **From PTZ controls**: UP/DOWN/LEFT/RIGHT/diagonal BTNs, PTZ_SPEED_SLIDER
- **From LASER_STATIC**: RADIUS_INC/DEC_BTN (already lens)
- **New**: H/V angle readouts (from ANGLE_H/V_EDIT, read-only display)
- **New**: Classic/Modern mode toggle
  - Classic: button grid + lens +/- rows (as current, reorganized)
  - Modern: virtual joystick canvas + arc sliders for zoom/focus/radius (new custom widgets)
- **Remove**: IRIS_OPEN/CLOSE_BTN (duplicate of radius)

#### 1.6 Floating Panel: TCU & Laser (bottom-center)
- **From TCU_STATIC**: All delay A/B edits, gate width A/B edits, laser width edits, MCP_SLIDER/EDIT, FREQ_EDIT, STEPPING_EDIT, DELAY_SLIDER, GW_SLIDER
- **From TCU_STATIC**: EST_DIST/ESTIMATED labels, delay diff/GW diff groups
- **From LASER_STATIC**: LASER_BTN, FIRE_LASER_BTN
- **New**: Normal/Guru mode toggle
  - Normal: visual timing bar diagram (distance/depth/laser/MCP bars — custom painted), fire button, unlock bar
  - Guru: full parameter grid (all edits exposed), B/N toggle for delay diff display
- **New**: Unlock safety bar for laser fire
- **Stays but renamed**: SWITCH_TCU_UI_BTN → Normal/Guru mode toggle

#### 1.7 Floating Panel: Console Data (bottom-right)
- **From MISC_DISPLAY stacked widget**: All 7 pages reorganized as tabs
  - DATA → DATA tab (DATA_EXCHANGE text area)
  - HIST → HIST tab (HIST_DISPLAY)
  - ALT_CTRL_PAGE → ALT tab (capture/record controls for alt displays — copy-paste from current, functional)
  - ADDON_PAGE → ADDON tab (plugin display)
  - VID_PAGE → VID tab (VL/IR camera controls — copy-paste from current, functional)
  - YOLO_PAGE → YOLO tab (model selector per display)
  - New: 3D tab (point cloud placeholder)
- **From MISC_DISPLAY_GRP**: MISC_OPTION_1/2/3 and MISC_RADIO_1/2/3 absorbed or replaced by tab switching
- Tab bar replaces the radio+dropdown selection mechanism

#### 1.8 Right-Side Widgets (fixed position, stacked vertically)

**IMG PROC widget (top-right)**:
- **MODELS tab**:
  - From user_panel.ui: MAIN_MODEL_LIST, ALT1_MODEL_LIST, ALT2_MODEL_LIST
  - Detection enable checkbox + confidence edit
  - Fishnet enable checkbox + fishnet model combo
- **GENERAL tab**:
  - From user_panel.ui: IMG_ENHANCE_CHECK + ENHANCE_OPTIONS combo
  - From user_panel.ui: FRAME_AVG_CHECK + FRAME_AVG_OPTIONS combo
  - From user_panel.ui: PSEUDOCOLOR_CHK
  - From user_panel.ui: IMG_3D_CHECK
  - **Discarded** (will not be implemented): BRIGHTNESS/CONTRAST/GAMMA_SLIDER — dropped from new UI

**Save Options widget (below IMG PROC)**:
- From user_panel.ui: FILE_PATH_EDIT + FILE_PATH_BROWSE
- New: ORI/RES toggle buttons for capture and record (4 buttons replacing SAVE_BMP/AVI/RESULT/FINAL)

**Vision widget (below Save Options)**:
- New widget showing per-display YOLO results (MAIN, ALT1, ALT2)
- From user_panel.ui: YOLO_MAIN_LABEL, YOLO_ALT1_LABEL, YOLO_ALT2_LABEL
- Thumbnail + model name + confidence stats
- Hidden when ALTs are inactive

#### 1.9 Side Display widget (top-left)
- **From FloatingWindow[0]**: First floating window displayed inline as side display
- Fixed position (207px wide), shows secondary camera feed
- FloatingWindow infrastructure stays — side display is just the first one rendered in a fixed widget slot
- Second FloatingWindow remains available but not shown as a widget (deferred)

#### 1.10 Mapbox widget (below Side Display)
- New placeholder widget (207px wide, 1:1 aspect)
- Map display for future GPS/positioning integration
- Static placeholder for now

### Phase 2: Mode System

#### 2.1 Mode switching infrastructure
- Top bar mode tabs: SETUP / MONITOR / SCAN
- MONITOR = default view (main display + floating panels + right widgets)
- SETUP = full-window overlay replacing display area
- SCAN = full-window overlay replacing display area
- Mode switch hides/shows floating panels and overlay containers

#### 2.2 SETUP overlay (absorbs left panel + Preferences dialog)
Row-based layout with scrollable config area + fixed preview sidebar.

**Row 1 — Connection & Config:**
- **Presets group**: Export/Import buttons (from TitleBar settings menu export/import pref)
- **Ports group**: TCU/Rangefinder/Lens/Laser/PTZ COM edits (from INITIALIZATION COM tab + Preferences SERIAL_TAB)
- **Serial Ports group**: Port/baudrate/available selects, refresh, TCP server checkbox + IP, share port, custom hex data (from Preferences SERIAL_TAB)
- **Save/Load group**: Frame info, custom info, integrate, save gray, consecutive capture, img format (from Preferences SAVE_LOAD_TAB)
- **TCU Configuration group**: TCU type, base unit, rep freq unit, auto PRF/MCP/AB lock, delay/GW/laser max+offset, PS config, laser toggle enables (from Preferences TCU_TAB)

**Row 2 — Hardware & Acquisition:**
- **Devices group**: Local IP (read-only), Device IP, pixel type, PTZ type, rotate, flip, device select, split/underwater/CL/eBUS checkboxes (from Preferences DEVICES_TAB)
- **Camera group**: Enumerate/Turn On/Turn Off, continuous/trigger radios, CMOS freq, time expo, gain + slider, start/stop grabbing, get params (from PARAMS_STATIC + IMG_GRAB_STATIC + INITIALIZATION DEVICE tab)
- **Camera group extras**: Image Region button (⛶) and Sensor Taps button (⚄) — compact icon buttons appended to Enumerate row (from IMG_REGION_BTN, SENSOR_TAPS_BTN)

**Row 3 — Image Processing (full width):**
- Sub-groups: Accumulative, Adaptive, DCP, 3D Mapping, Fishnet, ECC Denoise (from Preferences IMG_PROC_TAB)

**Preview sidebar (right 20%):**
- Live preview thumbnails for Main, ALT1, ALT2 displays

#### 2.3 SCAN overlay (absorbs ScanConfig dialog)
Single-row layout with config groups + action sidebar.

- **Delay group**: Start/end delay (us+ns), step size, rep freq (from ScanConfig)
- **Gate Width group**: Start/end GW (us+ns), step size (from ScanConfig)
- **PTZ group**: Start/end/step angles H+V, direction N/Z radios, wait time (from ScanConfig)
- **Capture/Record group**: Original/result checkboxes for capture+record, profiles count, imgs/profile (from ScanConfig)
- **Action buttons**: Start, Continue, Restart, Stop (from SCAN_BUTTON, CONTINUE_SCAN_BUTTON, RESTART_SCAN_BUTTON + stop logic)
- **Preview sidebar**: Same as Setup

### Phase 3: Theming

#### 3.1 QSS rewrite
- Replace style_dark.qss and style_light.qss with new theme system
- CSS variables from prototype translated to QSS properties
- ~100+ color tokens organized by: surfaces, borders, text, accent, semantic (positive/negative/info/warning), TCU bars, lens, PTZ, console, laser
- Gradient headers, glow effects (via box-shadow → QSS border + background tricks)
- Custom painting needed for: timing bars, joystick, arc sliders, dot indicators

#### 3.2 Font system
- Primary: Segoe UI 11px
- Monospace readouts: Consolas
- Widget headers: 9px bold uppercase with letter-spacing

### Phase 4: Signal Rewiring

#### 4.1 Controller signal connections
Sprint 14 decoupled all 7 controllers from UI via signals. The new UI needs to:
- Reconnect all controller signals to the new widget instances
- Widget names change but signal semantics stay the same
- Key mapping: old widget → new widget location (same slot, different parent)

#### 4.2 Widget identity mapping (key examples)
| Old Widget | New Location |
|---|---|
| ZOOM_IN/OUT_BTN | Optics panel → Classic mode |
| FOCUS_NEAR/FAR_BTN | Optics panel → Classic mode |
| PTZ direction BTNs | Optics panel → Classic mode grid |
| DELAY_A/B edits | TCU panel → Guru mode |
| GW_A/B edits | TCU panel → Guru mode |
| LASER_BTN, FIRE_LASER_BTN | TCU panel → both modes |
| ENHANCE_OPTIONS | IMG PROC widget → GENERAL tab |
| MAIN_MODEL_LIST | IMG PROC widget → MODELS tab |
| GAIN_EDIT/SLIDER | Setup overlay → Camera group |
| FILE_PATH_EDIT | Save Options widget |
| DATA_EXCHANGE | Console panel → DATA tab |
| HIST_DISPLAY | Console panel → HIST tab |

### Phase 5: Custom Widgets (new)

| Widget | Description | Paint method |
|---|---|---|
| FloatingPanel | Base class for drag/pin/hide panels | Standard QWidget + custom header |
| TimingBarWidget | Draggable horizontal bars for TCU normal mode | paintEvent with gradients |
| JoystickWidget | Virtual joystick for PTZ modern mode | paintEvent + mouse tracking |
| ArcSliderWidget | Vertical arc sliders for zoom/focus/radius | paintEvent on canvas |
| DotIndicator | Colored status dot with tooltip | paintEvent (circle + glow) |
| UnlockBar | Striped safety toggle for laser fire | paintEvent with repeating gradient |

---

## Part 2: Feature Status — Discarded & Deferred

Prototype v3 (updated) absorbs most previously-deferred items into Part 1 of the conversion. This section tracks what will **not** be in the first version of the new UI.

### Discarded features (will not be implemented)

These were evaluated and dropped from scope. They may be reconsidered later but are not planned for re-addition.

| # | Feature | Current Widget(s) | Reason |
|---|---|---|---|
| 1 | **Packet loss indicator** | StatusBar::packet_lost | Dropped from top bar status area |
| 2 | **Brightness/Contrast/Gamma sliders** | BRIGHTNESS/CONTRAST/GAMMA_SLIDER | Dropped from IMG PROC widget |

### Deferred features (future patches)

These are intentionally postponed to a later patch. Placement and design already discussed.

| # | Feature | Reason |
|---|---|---|
| 1 | Record button + autopilot badge in top bar | CSS ready in prototype, just needs placement |
| 2 | Software trigger button | Niche use; trigger mode radio is present |
| 3 | Set Params button | Get Params covers most needs |
| 4 | Binning checkbox (Hik cam) | Hardware-specific |
| 5 | Auto Focus / Get Lens Params buttons | Can add to Optics panel later |
| 6 | Rangefinder distance trigger button | Distance display exists; manual trigger is rare |
| 7 | Rulers (RULER_H, RULER_V) | Add as display overlay toggle |
| 8 | OSD button, LDM button | Stay in their respective console tab pages |
| 9 | Second side display widget | FloatingWindow infra stays; only first shown as widget |
| 10 | Laser Control dialog (4 individual lasers) | Separate dialog, can open from TCU panel or Setup |
| 11 | Developer Options dialog (lens calibration per intensity) | Separate dialog, accessible from Settings menu |
| 12 | Serial Server dialog (4-port TCP config) | Separate dialog, accessible from Setup |
| 13 | Distance 3D View dialog (3D visualization) | Separate dialog, accessible from Console → 3D tab |

### Prototype v3 update — layout refinements

Layout adjustments made during prototype v3 that deviate from the original plan (and should be followed during implementation):

- **Panel toggle bar (pbar)** moved from top-center to **top-right** above IMG PROC, aligning with the right-side widget stack
- **Center display toolbar** (new) holds tool selectors + INFO checkbox, replacing pbar's original top-center slot
- **IMG PROC / Save Options / Vision** stack positioned dynamically below pbar (JS layout logic → Qt resize logic)
- **FPS readout** moved from top-left to **bottom-left** of display
- **GAIN readout** removed from both main display and side display overlays (gain control lives in Setup)
- **Coordinate widgets** (START/SHIFT/SHAPE) placed in display top-left; distance readout top-right

---

## Key Decisions
- IRIS_OPEN/CLOSE removed (duplicate of RADIUS_INC/DEC)
- Bottom status bar removed; port lights move to top bar as dots
- Preferences/ScanConfig dialogs absorbed into SETUP/SCAN mode overlays
- Floating panels use pin/free/hide states (not Qt docking framework)
- VID and ALT console tabs: copy-paste current functionality, only restyle
- SWITCH_TCU_UI_BTN renamed to Normal/Guru mode toggle in TCU panel

## Files
- `prototype.html` — Interactive HTML/CSS/JS prototype for layout validation
- `UI_REDESIGN_PLAN.md` — This document
