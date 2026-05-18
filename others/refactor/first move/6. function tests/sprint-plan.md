# Function Tests Sprint Plan (Sprints 23–24)

End-to-end manual verification of GLI_user_panel after the full refactor (Sprints 1–22). The refactor touched signal routing, controller decoupling, and the legacy-dialog deletion (ScanConfig + Preferences), so every feature path is in scope.

**Prerequisites**: Sprints 1–22 complete (analysis → refactor → review → fixes → UI redesign).
**Approach**: Split into two sprints by environment dependency — software-only tests first (anywhere), hardware tests second (lab setup). Within each sprint, steps group items by risk and verification approach so failures cluster by cause.

**Per-feature tracking**: see [feature-test-matrix.md](feature-test-matrix.md) — 181 features × Sprint × Status. The matrix is the source of truth for pass/fail. This plan describes the structure; the matrix records the results.

---

## Sprint 23 — Software Function Tests

### Prerequisites
- Sprint 22 build is green
- Dev box only — no camera, TCU, lens, laser, PTZ, or rangefinder
- Sample media: a few video files (mp4 / avi / h264), at least one BMP, TIFF, JPEG
- Optional: one YOLO ONNX model file for inference tests
- Sample streaming endpoints: an RTSP source (or local test server), and a webcam (for `cv:0` path). HTTP/HLS with TLS renegotiation is a known blocker (covered as F-030c ↷ in matrix)

### Step 1 — Regression-critical (highest refactor risk)

These are the paths the refactor actively rewired. If anything broke, it's most likely here. Run automated tests first; manual second.

1.1 **Automated test gate**
- Run `ctest` — all 13 existing tests must pass before manual testing starts
- Add `test_scanparams.cpp` to Tier 1 covering Sprint 22's `get_ptz_route()` / `get_tcu_route()` / `frame_count` rewrites
- Fix or update any test that fails due to legitimate API change; do not disable

1.2 **SETUP overlay (Sprint 22 absorbed Preferences dialog)**
- All 5 cards (Save-Load, Serial Ports, TCU, Devices, Image Proc) render with their controls
- Every combo populates with the right items
- Every checkbox writes to Config and emits the corresponding SetupOverlay signal (24 signals total)
- TCU unit switch (ns/μs/m) updates MAX_DIST / MAX_DOV / DELAY_OFFSET / GATE_WIDTH_OFFSET via `update_distance_display`
- `PS_CONFIG_GRP` shows only on TCU_LIST == "40ps step"; PS_STEPPING / MAX_PS_STEP round-trip
- Laser group bits (LASER_CHK_1..4) write to `tcu.laser_on`
- `data_exchange(false)` → widgets from Config; `data_exchange(true)` → Config from widgets; full round-trip stable

1.3 **SCAN overlay (Sprint 22 absorbed ScanConfig dialog)**
- 4 cards + action column render per prototype v3
- Delay / GW edits recompute `frame_count`
- PTZ card: H/V angle derivation from start/end; direction radios; PTZ wait + num_single_pos
- Capture+Record 4 checkboxes update `ScanController::params()`
- Preset SAVE/LOAD round-trip preserves all scan parameters

1.4 **Controller signal routing (Sprint 14 decoupling)**

Sprint 14 decoupled all 7 controllers from `Ui::UserPanel*`; each one now communicates via signals/slots. The widget-side wiring is S23-testable per controller (no hardware needed); device-side effect is S24.

**TCU controller**:
- F-042 / F-043: DELAY_A/B_EDIT_U/N/P → controller → device write (round-trip)
- F-044 / F-045: GATE_WIDTH_A/B_EDIT_U/N/P → controller → device write
- F-046: LASER_WIDTH_EDIT_U/N/P → controller
- F-047: FREQ_EDIT + hz_unit display swap (kHz / Hz / m via `setup_hz`)
- F-048 / F-049 / F-050: MCP / DELAY / GW sliders ↔ edit fields, two-way
- ~~F-051: DIST_BTN math — input distance produces expected delay + GW + PRF~~ → marked `–` (DIST_BTN hidden per UI redesign Part 2 Deferred #6; `apply_distance()` math retained for future re-exposure)
- F-052: Auto PRF formula `1e6 / (delay + DOV + 1000)` at boundary inputs
- F-053: AB Lock — Delay B = A + offset_N
- F-054: Auto MCP adapts to frame brightness (verify via file playback)
- F-055: TCU type switch (`TCU_LIST` 0/1/2) — picosecond `_P` field show/hide on `idx >= 2`; layout reflow restored — **fixed** at [userpanel.cpp:1394-1410](src/visual/userpanel.cpp#L1394-L1410)
- F-157: SWITCH_TCU_UI_BTN B↔DIFF swap (B_GRP / GATE_WIDTH_B_GRP hide when diff shown) — **fixed** at [userpanel.cpp:1406-1411](src/visual/userpanel.cpp#L1406-L1411)

**Lens controller**: ZOOM_IN/OUT_BTN, FOCUS_FAR/NEAR_BTN, RADIUS_INC/DEC_BTN pressed/released → controller slots (F-060…F-063, F-068); ZOOM_EDIT / FOCUS_EDIT enter → set position (F-066, F-067)

**Laser controller**: LASER_BTN click → toggle slot (F-072); CURRENT_EDIT → current value (F-075); DUAL_LIGHT_BTN click (F-077); FIRE_LASER_BTN gating via UnlockBar (F-073, also covered in 2.1)

**IPTZ controller** (active_ptz dispatch): 8-direction buttons (F-078), STOP_BTN (F-079), GET_ANGLE_BTN (F-080), angle edit fields → set_angle (F-081), self-check at init (F-085), screen-px → angle coordinate transform (F-084 software side, device move S24)

**UDPPTZ controller** (Console VID tab widgets): VL/IR zoom/focus, defog, IR power, IR-AF, LDM, video-source, OSD click signals reach controller (F-086…F-095, F-097 software side)

**Scan controller**: SCAN_BUTTON / CONTINUE_SCAN_BUTTON / RESTART_SCAN_BUTTON click signals reach controller (F-121, F-122, F-123)

**Device manager**: F-001/F-002/F-003 button signals route by `device_type` (S24 walks each SDK); F-005 GET_PARAMS click fills edits without device; F-011/F-012 radio toggle propagates

**Port init paths**: F-161 / F-162 / F-167 — boot-time port construction must be crash-free with disconnected hardware

1.5 **Mode switching**
- Setup ↔ Monitor ↔ Scan; right-stack + floating panels hide in overlay modes
- Overlays raise above siblings; pbar / FPS label visibility follows mode
- F-031 FPS Counter: TimedQueue updates the readout while frames flow; readout hides in overlay modes
- F-074 Simple Laser Mode: `SIMPLE_LASER_CHK` toggles simple-UI vs Guru-UI of the laser block (LVTONG/SIMPLE_UI builds)

1.6 **Preview thumbnails (Sprint 20)**
- 3 thumbs in SETUP + 3 in SCAN mirror MAIN / ALT1 / ALT2 via `Display::set_pixmap`

1.7 **Config persistence**
- Auto-load `default.json` on startup; auto-save on change
- Manually-saved JSON round-trips
- Preset SAVE → close → reopen → LOAD: parameters identical (uses `ScanParams`)

### Step 2 — UI redesign verification

Less refactor-exposed but unverified after the redesign. Many items here are "does the widget actually behave like the prototype showed".

2.1 **Custom widget behavior (Sprint 15, not just paint)**
- JoystickWidget: drag → continuous `(dx, dy)` signal emission
- ArcSliderWidget: drag arc → value signal
- UnlockBar: drag-to-unlock state change
- F-073 Fire Laser: FIRE_LASER_BTN disabled until UnlockBar reaches the unlocked state (S24 verifies the device fire)
- TimingBarWidget: drag any of 4 bars → corresponding TCU edit field updates
- DotIndicator: visual color/glow states (state-drive deferred to S24)

2.2 **Panel mode toggles**
- Optics panel Classic ↔ Modern (button grid vs. joystick + arc sliders)
- TCU panel Normal ↔ Guru (timing bars vs. full param grid)

2.3 **Console panel tabs (UI redesign 1.7)**
- 7 tabs: DATA / HIST / ALT / ADDON / VID / YOLO / 3D
- Tab switching swaps content; DATA_EXCHANGE text area present (data flow verified in S24)
- HIST renders from file-playback frames
- ADDON tab — confirm content renders (currently plugin-stub territory per F-180 inventory note; verify the tab is present even if empty)
- VID tab widget clicks reach `UDPPTZController` (F-086…F-095 software side; device effect S24)
- F-159 Clean Log clears DATA_EXCHANGE

2.4 **Right-stack widgets (UI redesign 1.8)**
- IMG PROC widget: MODELS tab (3 model lists, detect + fishnet) and GENERAL tab (enhance / frame-avg / pseudocolor / 3D)
- Save Options widget: FILE_PATH_EDIT + 4 ORI/RES buttons (replaces SAVE_BMP/AVI/RESULT/FINAL)
- Vision widget: per-display labels render; thumbnails populate (YOLO results verified in 3.3)
- F-120 Fishnet Detection rendering: when LVTONG build is active, FISHNET_RESULT label updates from detection signal (the inference path itself is in 3.3)

2.5 **Side widgets + display overlays (UI redesign 1.2, 1.9, 1.10)**
- Side Display (inline FloatingWindow) — fixed 207px slot
- Mapbox placeholder — fixed 207px square
- Coordinate readouts (START / SHIFT / SHAPE) top-left
- Distance readout top-right — live rangefinder in S24 (manual entry via DIST_BTN now `–` per UI redesign Part 2 Deferred #6)
- Center display toolbar (SEL / ZOOM / PTZ + INFO) — F-022 / F-023 / F-024 / F-025 / F-026 tool toggles
- F-031 FPS Counter bottom-left readout (also tested per-mode in 1.5)
- F-082 PTZ Speed Control: slider value display (device effect S24)
- F-084 Point PTZ to Target: software-side screen-px → pan/tilt-angle coordinate transform (device move S24)

2.6 **Floating panels + window controls**
- Optics / TCU / Console panels: open via pbar, drag-move, pin/unpin, close
- Panels survive mode switch
- FloatingWindow 0/1 open and dock back; secondary display opens
- F-152 min/max/restore; F-153 / F-154 TopBar + dot indicators; F-156 frameless border resize
- F-152b Display full-screen: double-click on any Display widget toggles it to fullscreen (and back). PTZ tool mode suppresses this — verify guard

2.7 **Theme + language + shortcuts**
- F-149: dark/light theme repaints all custom widgets
- F-150: English ↔ Chinese retranslateUi
- F-178: keyboard shortcuts (Alt+0/1/2 floating, mode-tab hotkeys, V for 3D view, etc.)
- F-181: Multi-platform build — Windows verified; Linux build attempt (CMake config + compile, no full feature run required)

### Step 3 — Pipeline + I/O correctness (independent of refactor)

These paths weren't directly touched by the refactor but need verification — the data flows through new widget homes now.

3.1 **Image processing pipeline (file source)**
- F-027 / F-028: image rotation (0/90/180/270) + flip — Preferences setting now in SETUP overlay, pipeline cv::Mat transform
- F-007: pixel format selection effect — switching Mono8↔Mono10/12 must change the pipeline's cv::Mat bit-depth handling correctly (sample frames in each format needed)
- Enhancement options (F-098 to F-101): histogram-eq / adaptive / DCP / sky tolerance / fast_gf
- F-103: gamma adjustment (edit field only — slider was discarded in redesign)
- F-104 / F-105: frame averaging + ECC modes (window × warp × fusion; spot-check key combos, not all 36)
- F-106: pseudocolor toggle
- F-107: image splitting (2×2 quadrants via SPLIT_CHK)
- F-108: aliasing recommendations

3.2 **3D gated imaging (file source / synthetic data)**
- F-109: 3D image building (multi-frame depth fusion)
- F-110: range threshold clipping
- F-111: Reset 3D Parameters (`RESET_3D_BTN` clears state — pure UI op)
- F-114: distance → colormap rendering
- ~~F-112 / F-113: Distance3DView dialog~~ → marked `–` (deferred per UI redesign Part 2 Deferred #13; dialog source files retained for future re-add)

3.3 **YOLO inference**
- F-115/F-116/F-117: model selection per display (combo populated from on-disk models)
- F-118: inference runs end-to-end on file-playback frames
- F-119: detection boxes overlay on the displayed frame
- F-120: fishnet result count (LVTONG build only)

3.4 **Capture & record**
- F-032/F-033: ORI / RES save buttons write files
- F-034/F-035: ORI / RES record produces valid AVI
- F-036/F-037: ALT capture/record (via Console panel ALT tab)
- F-038: file path browse dialog
- F-039: full-window screenshot
- F-040: export video (background thread, valid output)
- F-041: BMP NOTE metadata block (covered by `test_imageio`)

3.5 **File I/O + Streaming sources**
- F-170 / F-171 / F-172: BMP / TIFF / JPEG read+write (covered by `test_imageio`)
- F-029: drag-drop image files
- F-030: video sources — four distinct code paths in [load_video_file](src/visual/userpanel.cpp#L4441):
  - **F-030a** Local file (mp4 / avi / h264): drag-drop or dialog → FFmpeg → PTS throttle. HW-decode pts + throttle unit-mismatch already fixed (see F-031 notes)
  - **F-030b** RTSP stream: dialog → FFmpeg with `rtsp_transport=tcp` + 2 MB buffer, no PTS throttle (network-paced). Verify reconnect on transient disconnect
  - **F-030c** HTTP / HLS stream: known blocker (TLS handshake failure on CloudFront/wurl.tv endpoints — needs FFmpeg ≥ 6.x). Track as ↷ until FFmpeg upgrade
  - **F-030d** Webcam (`cv:N`): OpenCV `VideoCapture` + DSHOW — separate from FFmpeg path; fixed 1920×1080 setup
- F-173: crash dump (Windows) — trigger via debug crash, verify .dmp file

3.6 **Other config dialogs (deferred per redesign but still functional)**
- F-141: config file browse (TopBar settings menu)
- F-142: Developer Options dialog opens, edit-round-trip works
- F-143: Serial Server Config dialog
- F-144: Gatewidth Lookup Table file load

3.7 **Distance + misc**
- ~~F-132: manual distance input dialog math~~ → marked `–` (DIST_BTN hidden)
- F-133: underwater mode toggle adjusts light speed

### End state
- All Step 1 / Step 2 / Step 3 items have a Status entry in the matrix
- Regressions found are fixed in-sprint; commit hash recorded in matrix Notes
- Items deferred have an explicit reason + target sprint

---

## Sprint 24 — Hardware Function Tests

### Prerequisites
- Sprint 23 complete (all software regressions fixed)
- Lab setup: GigE / USB3 / CameraLink camera, TCU, lens, laser, PTZ, rangefinder
- Known-good cable + power for each device
- Optional: Pleora eBUS camera, Euresys CameraLink camera, gamepad

### Step 1 — Camera + grabbing
- F-001 to F-003 device management — run separately for each available SDK: Hikvision MVS (mvcam), Pleora eBUS (ebuscam), Euresys CameraLink (euresyscam, if build flag enabled), HQV (hqvcam). Each enumeration produces its own device list
- F-004 / F-005 / F-007 / F-008 / F-009 / F-010 camera params, ROI, sensor taps, IP — run against whichever camera is on the bench
- F-011 to F-016 grab control (continuous / trigger / start / stop)
- F-017 live frame rendering at expected FPS

### Step 2 — TCU + laser
- F-042 to F-048 timing parameter writes verified on device (delay / GW / laser / freq / MCP)
- F-049 / F-050 slider drag reflects on hardware
- F-054 / F-055 / F-058 auto-MCP, TCU type switch, individual laser bits reach device
- F-059 parameter feedback poll
- F-072 to F-077 laser on/off / fire (via UnlockBar) / current / preset positions / dual light

### Step 3 — Lens + PTZ + rangefinder
- F-060 to F-071 lens zoom / focus / radius / iris / speed / position feedback
- F-078 to F-085 PTZ directional / stop / get-angle / set-angle / speed / feedback / point-to-target / self-check
- F-086 to F-097 UDP PTZ (VL/IR zoom/focus, defog, IR power, LDM, video source toggle, OSD)
- F-131 rangefinder distance polling

### Step 4 — Joystick + serial
- F-146 to F-148 gamepad PTZ + button mappings + 8-direction
- F-161 to F-169 serial port init / TCP init / shared port / data logging / transparent transmission / USB CAN / serial server bridge / port health

### Step 5 — Full scan end-to-end
- F-121 to F-128: start / continue / restart scan with all four devices live
- PTZ walks computed route; TCU sets delay/GW per step; camera captures
- Files written per `m_params.capture_scan_ori` / `capture_scan_res` and `record_scan_ori` / `record_scan_res`
- Verify captured `frame_count` matches `get_ptz_route(...)` × `get_tcu_route(...)` size
- F-176 scan image output directory structure

### Step 6 — Cross-device integration
- F-024 / F-084 mouse-to-PTZ pointing (screen → angle → device move)
- F-051 / F-132 distance dialog → automatically configures TCU on device
- F-164 port status DotIndicators reflect real port states

### End state
- All hardware test items in the matrix have a Status entry
- Device-specific quirks documented (firmware versions, port settings, baudrates)
- Any device-blocking issues logged with reproduction steps

---

## Tracking

- `feature-test-matrix.md` is the per-feature pass/fail record. Update Status + Notes columns as tests run.
- Each sprint gets a `sprint-XX-progress.md` for narrative notes (what was unblocked, what was deferred, fix commits).
- Defer only if a fix is structural and out of scope; log the target sprint or open issue.
