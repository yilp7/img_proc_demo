# Feature × Sprint Test Matrix

Cross-reference of the 181 features from [phase1-feature-inventory.md](../1. analysis/phase1-feature-inventory.md) against the two function-test sprints. Fill in **Status** as tests are executed.

**Legend**
- **Sprint** — `23` = software (Sprint 23, no hardware), `24` = hardware (Sprint 24, lab setup required), `23+24` = both layers (software side in S23, device-effect verification in S24), `–` = not in scope (deprecated / disabled / dropped from new UI / stub)
- **Status** — `☐` not yet tested, `✓` pass, `✗` fail (with fix-commit hash in Notes), `↷` skipped (reason in Notes)
- **Notes** — automated test coverage, deferral context, redesign status

---

## A. Camera / Device Management

| ID | Feature | Sprint | Status | Notes |
|---|---|---|---|---|
| F-001 | Enumerate Devices | 23+24 | ✓ | S23 1.4 DM: ENUM_BUTTON click signal routing verified; S24: per-SDK enumeration verified at bench |
| F-002 | Start Device | 23+24 | ✓ | S23 1.4 DM: START_BUTTON signal routing verified; S24: device starts on bench |
| F-003 | Shutdown Device | 23+24 | ✓ | S23 1.4 DM: SHUTDOWN_BUTTON signal routing verified; S24: device shuts down cleanly on bench |
| F-004 | Set Camera Parameters | 24 | ✓ | SET_PARAMS_BUTTON dropped per UI redesign Part 2 Deferred #3; initial params applied at device start — verified at bench |
| F-005 | Get Camera Parameters | 23+24 | ✓ | S23 1.4 DM: GET_PARAMS_BUTTON signal routing verified; S24: SDK read verified at bench |
| F-006 | Camera Binning Mode | – | ☐ | Deferred per UI redesign Part 2 #4 (hardware-specific, removed from new UI) |
| F-007 | Set Pixel Format | 23+24 | ✓ | S23: combo populates Mono8/10/12/RGB8/BayerRG8; S24: format-switch effect on `cv::Mat` verified at bench |
| F-008 | Set Image Region (ROI) | 23+24 | ✓ | S23 1.2.1: dialog + round-trip; S24: ROI write verified at bench |
| F-009 | Set Sensor Taps | 23+24 | ✓ | S23 1.2.1: dialog + round-trip; S24: tap-mode write verified at bench |
| F-010 | Device IP Configuration | 23+24 | ✓ | S23: IP encode/decode math; S24: write to camera verified at bench |

## B. Image Grabbing & Display

| ID | Feature | Sprint | Status | Notes |
|---|---|---|---|---|
| F-011 | Set Continuous Mode | 23+24 | ✓ | S23 1.4 DM: routing verified; S24: continuous mode applied on bench |
| F-012 | Set Trigger Mode | 23+24 | ✓ | S23 1.4 DM: routing verified; S24: trigger mode applied on bench |
| F-013 | Software Trigger Enable | – | ☐ | Deferred per UI redesign Part 2 #2 |
| F-014 | Manual Software Trigger | – | ☐ | Deferred per UI redesign Part 2 #2 |
| F-015 | Start Grabbing | 24 | ✓ | S24: grab start verified at bench |
| F-016 | Stop Grabbing | 24 | ✓ | S24: grab stop verified at bench |
| F-017 | Main Display Rendering | 23+24 | ✓ | S23: file playback path; S24: live camera frames render at expected fps |
| F-018 | Histogram Display | 23 | ✓ | Renders from cv::Mat; verified during display sweep |
| F-019 | Alt Display 1 | 23 | ✓ | Display routing verified (Side Display mirrors ALT1 thumb) |
| F-020 | Alt Display 2 | 23 | ✓ | Display routing verified (Side Display mirrors ALT2 thumb) |
| F-021 | Dual Display Mode | 23 | ✓ | Verified during display sweep |
| F-022 | Mouse Zoom Tool | 23 | ✓ | Center toolbar ZOOM button verified |
| F-023 | Mouse Select Tool (ROI) | 23 | ✓ | Center toolbar SEL button verified |
| F-024 | Mouse PTZ Tool | 23+24 | ✓ | S23: tool toggle + click event; S24: PTZ moves on bench |
| F-025 | Center on Screen | 23 | ✓ | Verified during display sweep |
| F-026 | Info Overlay | 23 | ✓ | INFO checkbox in center toolbar verified |
| F-027 | Image Rotation | 23 | ✓ | cv::Mat transform verified via file source |
| F-028 | Image Flip | 23 | ✓ | cv::Mat flip verified via file source |
| F-029 | Static Image Display | 23 | ✓ | Drag-drop image file verified |
| F-030 | Video File Playback | 23 | ✓ | Umbrella row — sub-paths a/b passed, c deferred (TLS), d still ☐ (separate cv:N path) |
| F-030a | &nbsp;&nbsp;Local video (mp4/avi/h264) | 23 | ✓ | Verified during F-031 fix pass (drag-drop mp4 ~24 fps now plays at real-time speed; HW-decode pts + throttle unit-mismatch corrected at [userpanel.cpp:4695-4710](src/visual/userpanel.cpp#L4695-L4710)). Same code path handles avi/h264 — extending coverage requires testing those file types explicitly |
| F-030b | &nbsp;&nbsp;RTSP stream | 23 | ✓ | Dialog input `rtsp://...`; FFmpeg path with `rtsp_transport=tcp` + 2 MB buffer; no PTS throttle (network-paced). **LAN pass** + **WAN pass** (Step 3.5) — minor lag on internet streams, acceptable since streaming is non-core. **Known gap**: no `reconnect=1` flag, so a transient WAN drop kills the stream silently — flagged for future polish, not in scope this sprint |
| F-030c | &nbsp;&nbsp;HTTP / HLS stream | 23 | ↷ | Dialog input `http(s)://.../*.m3u8`; **known blocker**: TLS handshake failure (`[tls @ ...] Failed to read handshake response`) on CloudFront/wurl.tv style endpoints — needs FFmpeg ≥ 6.x; carry as deferred until upgraded |
| F-030d | &nbsp;&nbsp;Webcam (cv:N) | 23 | ☐ | Dialog input `cv:N`; OpenCV `VideoCapture` + DSHOW backend (separate from FFmpeg); fixed 1920×1080 setup |
| F-031 | FPS Counter | 23 | ✓ | Step 1.5.5: per-display refactor verified — `q_fps_calc[3]`, `frame_rate_edit[3]`, `m_fps_label[3]`; alt labels parented to `displays[1/2]` via eventFilter. Three bugs fixed in same pass: (1) `r_frame_rate` → `avg_frame_rate` + `av_q2d` for video load (CCD_FREQ_EDIT was 48 for 23.97 source); (2) HW decoding pts: throttle was using `sw_frame->pts` (uninitialized after `av_hwframe_transfer_data`), now uses `frame->pts` (root cause of 2× playback speed); (3) unit mismatch in throttle elapsed correction — `nsecsElapsed()/1e6` (ms) → `/1000` (µs) to match av_rescale_q output |

## C. Image / Video Capture & Recording

| ID | Feature | Sprint | Status | Notes |
|---|---|---|---|---|
| F-032 | Save Original Image | 23 | ✓ | S23 3.4: Save Options widget ORI button writes original frame |
| F-033 | Save Processed Image | 23 | ✓ | S23 3.4: Save Options widget RES button writes processed frame |
| F-034 | Record Original Video | 23 | ✓ | S23 3.4: Save Options widget ORI record toggle starts/stops |
| F-035 | Record Processed Video | 23 | ✓ | S23 3.4: Save Options widget RES record toggle starts/stops |
| F-036 | Alt Display Capture | 23 | ✓ | S23 3.4: Console panel ALT tab capture |
| F-037 | Alt Display Record | 23 | ✓ | S23 3.4: Console panel ALT tab record |
| F-038 | File Path Browse | 23 | ✓ | S23 3.4: Save Options widget FILE_PATH_BROWSE dialog round-trip |
| F-039 | Screenshot | 23 | ✓ | S23 3.4: TopBar capture button |
| F-040 | Export Video | 23 | ✓ | S23 3.4: background thread; export rate matches main camera rate after per-display refactor (`vid_out_proc` + `frame_rate_edit[0]`) |
| F-041 | BMP Metadata (NOTE) | 23 | ✓ | Covered by `test_imageio` (automated) |

## D. TCU (Timing Control Unit)

| ID | Feature | Sprint | Status | Notes |
|---|---|---|---|---|
| F-042 | Set Delay A | 23+24 | ✓ | S23 1.4: us/ns/ps unit split + edit-field sync; S24: TCU register written, verified at bench |
| F-043 | Set Delay B | 23+24 | ✓ | S23 1.4: pass incl. AB-lock derivation; S24: verified at bench |
| F-044 | Set Gate Width A | 23+24 | ✓ | S23 1.4: pass; S24: verified at bench |
| F-045 | Set Gate Width B | 23+24 | ✓ | S23 1.4: pass incl. AB-lock; S24: verified at bench |
| F-046 | Set Laser Width | 23+24 | ✓ | S23 1.4: pass; S24: verified at bench |
| F-047 | Set Frequency (PRF) | 23+24 | ✓ | S23 1.4: hz_unit display switching; S24: PRF register verified at bench |
| F-048 | Set MCP (Gain) | 23+24 | ✓ | S23 1.4: slider ↔ edit binding; S24: MCP register verified at bench |
| F-049 | Delay Slider | 23+24 | ✓ | S23 1.4: slider→ns mapping + edit sync; S24: verified at bench |
| F-050 | Gate Width Slider | 23+24 | ✓ | S23 1.4: pass; S24: verified at bench |
| F-051 | Distance / Gate Width Dialog | – | ☐ | DIST_BTN entry point hidden per UI redesign Part 2 Deferred #6 (rangefinder distance trigger button deferred); `apply_distance()` math still exists in TCUController, can be re-exposed when manual entry returns — automated test recommended if path is revived |
| F-052 | Auto PRF | 23 | ✓ | S23 1.4: formula `1e6 / (delay + DOV + 1000)` pass (prior dev testing) |
| F-053 | AB Lock | 23 | ✓ | S23 1.4: B = A + offset_N pass |
| F-054 | Auto MCP | 23+24 | ✓ | S23 1.4: adaptive gain from frame brightness (file source); S24: device write verified at bench |
| F-055 | TCU Type Selection | 23+24 | ✓ | S23 1.4.13/1.4.15: layout reflow restored — `update_tcu_param_pos` helper re-introduced in UserPanel; `tcu_type_layout_changed` signal extended with `laser_diff` arg; spurious DIFF_GRP toggle removed (DIFF visibility is SWITCH_TCU_UI_BTN's job only). S24: protocol switch verified at bench |
| F-056 | PS Stepping Config | 23 | ✓ | UI round-trip via PS_STEPPING_EDT / MAX_PS_STEP confirmed |
| F-057 | Offset Configuration | 23 | ✓ | Config storage round-trip confirmed |
| F-058 | Individual Laser Enable | 23+24 | ✓ | S23: LASER_CHK_1..4 bit packing confirmed; S24: verified at bench |
| F-059 | TCU Parameter Feedback | 24 | ✓ | 1s polling read from device verified at bench |

## E. Lens Control

| ID | Feature | Sprint | Status | Notes |
|---|---|---|---|---|
| F-060 | Zoom In | 23+24 | ✓ | S23 1.4 Lens: signal routing; S24: motor in verified at bench |
| F-061 | Zoom Out | 23+24 | ✓ | S23 1.4 Lens: signal routing; S24: motor out verified at bench |
| F-062 | Focus Far | 23+24 | ✓ | S23 1.4 Lens: signal routing; S24: motor far verified at bench |
| F-063 | Focus Near | 23+24 | ✓ | S23 1.4 Lens: signal routing; S24: motor near verified at bench |
| F-064 | Auto Focus | – | ☐ | Deferred per UI redesign Part 2 #5 |
| F-065 | Get Lens Parameters | – | ☐ | Deferred per UI redesign Part 2 #5 |
| F-066 | Set Zoom Position | 23+24 | ✓ | S23 1.4 Lens: signal routing; S24: motor moves to value verified at bench |
| F-067 | Set Focus Position | 23+24 | ✓ | S23 1.4 Lens: signal routing; S24: motor moves verified at bench |
| F-068 | Laser Radius Inc/Dec | 23+24 | ✓ | S23 1.4 Lens: signal routing; S24: radius change verified at bench |
| F-069 | Focus Speed Control | 23+24 | ✓ | S23: slider value display; S24: motor speed verified at bench |
| F-070 | Iris Open/Close | – | ☐ | Removed per UI redesign Phase 1.5 (duplicate of radius) |
| F-071 | Lens Position Feedback | 24 | ✓ | S24: position feedback poll verified at bench |

## F. Laser Control

| ID | Feature | Sprint | Status | Notes |
|---|---|---|---|---|
| F-072 | Laser On/Off Toggle | 23+24 | ✓ | S23 1.4 Laser: signal routing; S24: laser toggles on TCU bus verified at bench |
| F-073 | Fire Laser | 23+24 | ✓ | S23 1.4 Laser: UnlockBar gates fire signal; S24: laser fires verified at bench |
| F-074 | Simple Laser Mode | 23 | ✓ | Step 1.5.6: SIMPLE_LASER_CHK toggle swaps simple ↔ Guru laser block (LVTONG/SIMPLE_UI builds) |
| F-075 | Laser Current Control | 23+24 | ✓ | S23 1.4 Laser: signal routing; S24: current setpoint verified at bench |
| F-076 | Laser Preset Positions | – | ☐ | LaserControl dialog deferred per UI redesign Part 2 Deferred #10 — entry point not in new UI; dialog source files still exist for future re-add |
| F-077 | Dual Light Control | 23+24 | ✓ | S23 1.4 Laser: signal routing; S24: illumination toggle verified at bench |

## G. PTZ (Pan-Tilt-Zoom)

| ID | Feature | Sprint | Status | Notes |
|---|---|---|---|---|
| F-078 | PTZ Directional Move | 23+24 | ✓ | S23 1.4 IPTZ: 8-direction routing; S24: PTZ moves verified at bench |
| F-079 | PTZ Stop | 23+24 | ✓ | S23 1.4 IPTZ: STOP_BTN routing; S24: PTZ stops verified at bench |
| F-080 | Get PTZ Angle | 23+24 | ✓ | S23 1.4 IPTZ: GET_ANGLE_BTN routing; S24: SDK read verified at bench |
| F-081 | Set PTZ Angle | 23+24 | ✓ | S23 1.4 IPTZ: angle edit routing; S24: PTZ moves to value verified at bench |
| F-082 | PTZ Speed Control | 23+24 | ✓ | S23: slider value; S24: PTZ motor speed verified at bench |
| F-083 | PTZ Angle Feedback | 24 | ✓ | Hardware polling verified at bench |
| F-084 | Point PTZ to Target | 23+24 | ✓ | S23 1.4 IPTZ: PTZ_TOOL screen→angle transform; S24: PTZ moves verified at bench |
| F-085 | PTZ Self-Check | 23+24 | ✓ | S23 1.4 IPTZ: init routing crash-free; S24: PTZ home cycle verified at bench |

## H. UDP PTZ (VID_PAGE Camera Control)

Widget click signals are S23-testable (reach UDPPTZController slot). Actual UDP frames + device response require S24 hardware.

| ID | Feature | Sprint | Status | Notes |
|---|---|---|---|---|
| F-086 | VL Zoom In/Out | 23+24 | ✓ | S23 1.4 UDPPTZ: routing; S24: VL camera zoom verified at bench |
| F-087 | VL Focus Far/Near | 23+24 | ✓ | S23 1.4 UDPPTZ: routing; S24: VL focus verified at bench |
| F-088 | IR Zoom In/Out | 23+24 | ✓ | S23 1.4 UDPPTZ: routing; S24: IR zoom verified at bench |
| F-089 | IR Focus Far/Near | 23+24 | ✓ | S23 1.4 UDPPTZ: routing; S24: IR focus verified at bench |
| F-090 | VL Defog | 23+24 | ✓ | S23 1.4 UDPPTZ: routing; S24: defog filter verified at bench |
| F-091 | IR Power | 23+24 | ✓ | S23 1.4 UDPPTZ: routing; S24: illuminator verified at bench |
| F-092 | IR Auto Focus | 23+24 | ✓ | S23 1.4 UDPPTZ: routing; S24: IR AF verified at bench |
| F-093 | Low-Distance Mode | 23+24 | ✓ | S23 1.4 UDPPTZ: routing; S24: device mode verified at bench |
| F-094 | Video Source Toggle | 23+24 | ✓ | S23 1.4 UDPPTZ: routing; S24: source switch verified at bench |
| F-095 | OSD Toggle | 23+24 | ✓ | S23 1.4 UDPPTZ: routing; S24: OSD on/off verified at bench. Frame bytes confirmed against 530 protocol: `EB 90 01 35/36/37 00 00 00 00 7F 7F 00 00 00 cksum` |
| F-096 | UDP PTZ Angle Feedback | 24 | ✓ | S24: rx status frames parsed (angles + servo status) verified at bench |
| F-097 | UDP PTZ Movement | 23+24 | ✓ | S23 1.4 UDPPTZ: routing; S24: servo movement verified at bench (per-axis h_vel/v_vel via ptz_move_2d) |

## I. Image Processing

All software — exercised through the file-playback pipeline.

| ID | Feature | Sprint | Status | Notes |
|---|---|---|---|---|
| F-098 | Image Enhancement | 23 | ✓ | IMG PROC widget GENERAL tab; covered by `test_imageproc` for pure functions |
| F-099 | Histogram Equalization | 23 | ✓ | One of ENHANCE_OPTIONS |
| F-100 | Adaptive Enhancement | 23 | ✓ | Adaptive sub-group in SETUP overlay |
| F-101 | Haze Removal (DCP) | 23 | ✓ | dehaze_pct + sky_tolerance + fast_gf in SETUP overlay |
| F-102 | Brightness/Contrast | – | ☐ | Discarded per UI redesign Part 2 (discarded #2) |
| F-103 | Gamma Adjustment | 23 | ✓ | Slider discarded; GAMMA_EDIT in SETUP overlay still present |
| F-104 | Frame Averaging | 23 | ✓ | FRAME_AVG_CHECK + 4/8-frame options |
| F-105 | Temporal Denoise (ECC) | 23 | ✓ | ECC modes + warp + fusion combinations |
| F-106 | Pseudocolor | 23 | ✓ | PSEUDOCOLOR_CHK |
| F-107 | Image Splitting | 23 | ✓ | SPLIT_CHK (2×2 quadrants) |
| F-108 | Aliasing Recommendations | – | ☐ | Deprecated for now — patch-in feature kept on the backlog for possible later revival |

## J. 3D Gated Imaging

| ID | Feature | Sprint | Status | Notes |
|---|---|---|---|---|
| F-109 | 3D Image Building | 23+24 | ✓ (s23) | S23 3.2: multi-frame fusion verified via file-source path. **S24 full hardware-fed scan deferred** alongside F-125-F-128 |
| F-110 | 3D Range Threshold | 23 | ✓ | S23 3.2: lower/upper 3D threshold round-trip pass |
| F-111 | Reset 3D Parameters | 23 | ✓ | S23 3.2: CUSTOM_3D_PARAM_CHK reset pass |
| F-112 | 3D Scatter Visualization | – | ☐ | Distance3DView dialog deferred per UI redesign Part 2 Deferred #13 — entry point not in new UI; build flag still exists, dialog files retained |
| F-113 | 3D View Controls | – | ☐ | Same — deferred with Distance3DView dialog |
| F-114 | Paint 3D (Colormap) | 23 | ✓ | Colormap applied to distance data — verified during 3D sweep |

## K. AI / Object Detection (YOLO)

| ID | Feature | Sprint | Status | Notes |
|---|---|---|---|---|
| F-115 | YOLO Model Selection (Main) | 23 | ✓ | S23 3.3: IMG PROC widget MODELS tab — main combo populated + selection wired |
| F-116 | YOLO Model Selection (Alt1) | 23 | ✓ | S23 3.3: same |
| F-117 | YOLO Model Selection (Alt2) | 23 | ✓ | S23 3.3: same |
| F-118 | YOLO Inference | 23 | ✓ | S23 3.3: ONNX runtime + loaded model produces detections |
| F-119 | Draw Detection Boxes | 23 | ✓ | S23 3.3: `draw_yolo_boxes` at pipelineprocessor.cpp:606 renders overlays on display |
| F-120 | Fishnet Detection Result | 23 | ☐ | LVTONG build only — patch-in |

## L. Scanning System

| ID | Feature | Sprint | Status | Notes |
|---|---|---|---|---|
| F-121 | Start Scan | 23+24 | ✓ | S23 1.4 Scan: SCAN_BUTTON click signal routing verified; S24: full scan with hardware |
| F-122 | Continue Scan | 23+24 | ✓ | S23 1.4 Scan: CONTINUE_SCAN_BUTTON signal routing verified; S24: scan resumes |
| F-123 | Restart Scan | 23+24 | ✓ | S23 1.4 Scan: RESTART_SCAN_BUTTON signal routing verified; S24: scan restarts |
| F-124 | Scan Configuration | 23 | ✓ | SCAN overlay 4 cards + action column verified during 1.3 |
| F-125 | TCU Parameter Scan | 24 | ☐ | **Deferred** — full-bench scan end-to-end not core for this iteration; revisit when scheduled |
| F-126 | PTZ Route Scan | 24 | ☐ | **Deferred** — same as F-125 |
| F-127 | Scan Image Capture | 24 | ☐ | **Deferred** — same as F-125 |
| F-128 | Scan Video Recording | 24 | ☐ | **Deferred** — same as F-125 |
| F-129 | Auto-Scan (CLI) | – | ☐ | DISABLED in code |
| F-130 | Scan Preset Validation | – | ☐ | DISABLED (part of Auto-Scan) |
| *new* | `get_ptz_route(ScanParams&)` correctness | 23 | ✓ | Covered by `test_scanparams` TC-SP-001, TC-SP-020..025 (all pass) |
| *new* | `get_tcu_route(ScanParams&)` correctness | 23 | ✓ | Covered by `test_scanparams` TC-SP-001, TC-SP-010..013 (all pass) |
| *new* | `frame_count` derivation in ScanOverlay | 23 | ✓ | Step 1.3 manual: Delay / GW / PTZ edits recompute `frame_count` correctly; SCAN overlay 4-card layout matches prototype after Capture/Record desc_row equalized-width fix |

## M. Rangefinder & Distance

| ID | Feature | Sprint | Status | Notes |
|---|---|---|---|---|
| F-131 | Distance Measurement | 24 | ☐ | **Deferred** — rangefinder not on bench this iteration; revisit when device available |
| F-132 | Manual Distance Input | – | ☐ | DIST_BTN hidden per UI redesign Part 2 Deferred #6 + user direction (no client demand). `apply_distance()` math still in TCUController for future re-exposure |
| F-133 | Underwater Mode | 23 | ✓ | S23 3.7 audit: signal chain `UNDERWATER_CHK::stateChanged → device_underwater → update_light_speed` preserved 1:1 from sprint-9 original. Math: `c_water = 0.75·c_air` (n≈1.33), `dist_ns = c/2`. Port-side write order safe (LIGHT_SPEED before EST_DIST/EST_DOV). **Two latent fixes applied**: (1) [tcucontroller.cpp:349-356](src/controller/tcucontroller.cpp#L349-L356) bypass slider-debounce throttle by pushing `EST_DIST`/`EST_DOV` directly instead of `update_delay()`/`update_gate_width()` — avoids dropped re-push under rapid toggle; (2) [userpanel.cpp:2031-2042](src/visual/userpanel.cpp#L2031-L2042) LVTONG startup `UNDERWATER_CHK->click()` / `SHARE_CHK->click()` → `if (!chk->isChecked()) chk->click()` — single-fire, idempotent against a persisted-true config, immune to future .ui default flips |

## N. Preset Management

| ID | Feature | Sprint | Status | Notes |
|---|---|---|---|---|
| F-134 | Save Preset | 23 | ✓ | Verified during SCAN overlay sweep (preset save round-trip) |
| F-135 | Apply Preset | 23 | ✓ | TCU + lens + camera state restore verified during SCAN sweep |
| F-136 | Preset Table Display | 23 | ✓ | Preset table populated and displayed correctly |

## O. Configuration & Settings

| ID | Feature | Sprint | Status | Notes |
|---|---|---|---|---|
| F-137 | Preferences Dialog | 23 | ✓ | Absorbed into SETUP overlay; S23 1.2.1–1.2.7 pass after Row-3 horizontal + Camera-card fixes |
| F-138 | JSON Config Load | 23 | ✓ | Covered by `test_config` (automated) + S23 1.7 manual: A1 auto-load `default.json` on startup, D10/D11 manual `Load` round-trips. Fix: auto-load path now uses `applicationDirPath()` (userpanel.cpp:1858-1860) to match `auto_save` — previously was relative "default.json" which broke round-trip when CWD ≠ binary dir. |
| F-139 | JSON Config Save | 23 | ✓ | Covered by `test_config` (automated) + S23 1.7 manual: D8/D9 manual SAVE → D12 byte-identical round-trip. |
| F-140 | Config Auto-Save | 23 | ✓ | Covered by `test_config` (automated) + S23 1.7 manual: B3/B4 COM/PTZ port connect, B5 theme switch, B6 language switch all persist across runs. YOLO model selection lambdas at userpanel.cpp:1876-1887 intentionally NOT auto-saved (serializer also drops the fields). |
| F-141 | Config File Browse | 23 | ✓ | TopBar settings menu — load/save round-trip verified |
| F-142 | Developer Options | – | ☐ | Deprecated — separate dialog still in build but out of test scope per user direction; revisit if scope changes |
| F-143 | Serial Server Config | 23 | ✓ | Separate dialog kept per UI redesign deferred #12; dialog round-trip verified |
| F-144 | Gatewidth Lookup Table | 23 | ✓ | File reader (camera-serial indexed) verified |
| F-145 | Legacy Binary Config | – | ☐ | DEPRECATED, replaced by JSON |

## P. Joystick / Gamepad Input

All require a physical gamepad — Sprint 24 only.

| ID | Feature | Sprint | Status | Notes |
|---|---|---|---|---|
| F-146 | Joystick PTZ Control | 24 | ☐ | **Deferred** — gamepad not on bench this iteration; revisit when device available |
| F-147 | Joystick Button Actions | 24 | ☐ | **Deferred** — same as F-146 |
| F-148 | Joystick Direction | 24 | ☐ | **Deferred** — same as F-146 |

## Q. UI / Display Management

| ID | Feature | Sprint | Status | Notes |
|---|---|---|---|---|
| F-149 | Dark/Light Theme Toggle | 23 | ✓ | TopBar theme button — repaints all custom widgets |
| F-150 | Language Switch (EN/ZH) | 23 | ✓ | TopBar language button — retranslateUi |
| F-151 | Hide Left Panel | – | ☐ | N/A — left panel removed in UI redesign |
| F-152 | Window Maximize/Restore | 23 | ✓ | TopBar max/restore button + Alt+Enter verified |
| F-152b | Display Full-Screen (per-display) | 23 | ✓ (main) / known-bug (alts) | **MAIN display**: double-click toggle (Display becomes Qt::Dialog + showFullScreen). PTZ-tool mode (`mode == 2`) suppresses. Three rendering bugs fixed: (1) stale `display_region` — `Display::resizeEvent` now re-calls `update_roi()` + emits `widget_resized`; UserPanel resnapshots ProcessingParams; (2) crop-math axis bug at pipelineprocessor.cpp:632 — was using `ww/display_width` for both axes, now per-axis ratios; (3) display-size resize at pipelineprocessor.cpp:652 — plain cv::resize stretched aspect, now letterbox/pillarbox preserving source aspect. **ALT1/ALT2 (FloatingWindow)**: chrome opacity fixed at floatingwindow.cpp — frame's inline stylesheet set `background: transparent; border: none;` to override dark theme's `.QFrame { background-color: #12100E }` rule. **Known issue (deferred)**: alt fullscreen via double-click does not work — the chrome QFrame is z-above the Display and intercepts the second click of the double-click sequence (Qt event order Press→Release→DblClick reshows frame on first Release, which then steals the dblclick). Alt fullscreen is effectively disabled by current behavior; left as-is per user decision. Revisit if alts need fullscreen — would require either making frame `Qt::WA_TransparentForMouseEvents` (loses window-drag) or forwarding `mouseDoubleClickEvent` from FloatingWindow→Display. |
| F-153 | Custom Title Bar | 23 | ✓ | Now TopBar (Sprint 16) — verified |
| F-154 | Custom Status Bar | 23 | ✓ | Now DotIndicators in TopBar (Sprint 16) — note: state-driven indicator not fully implemented yet, visible-only |
| F-155 | Floating Windows | 23 | ✓ | Alt+0/1/2 shortcuts verified during 2.6 |
| F-156 | Window Resize (Custom) | 23 | ✓ | Frameless border hit-testing verified during 2.6 |
| F-157 | Switch TCU UI Layout | 23 | ✓ | 1.4.6/1.4.14: SWITCH_TCU_UI_BTN B↔DIFF swap restored at userpanel.cpp:1406-1411 (was one-sided after Sprint 14 decoupling — only DIFF toggled, B stayed visible). Now B_GRP / GATE_WIDTH_B_GRP hide when diff shown, restore when diff hidden. (Note: Normal/Guru tab toggle in TCUPanel is separate; both pages share the same controller state, fine.) |
| F-158 | COM Data Display | 23+24 | ✓ | S23: DATA_EXCHANGE in Console DATA tab renders; S24: serial traffic display verified at bench |
| F-159 | Clean Log | 23 | ✓ | TopBar CLS button verified |
| F-160 | Misc Radio Selection | – | ☐ | Replaced by Console panel tabs |

## R. Communication / Serial

| ID | Feature | Sprint | Status | Notes |
|---|---|---|---|---|
| F-161 | Serial Port Init | 23+24 | ✓ | S23 1.4 Port: 5 port objects boot crash-free; S24: open + handshake verified at bench |
| F-162 | TCP Port Init | 23+24 | ✓ | S23 1.4 Port: init crash-free; S24: connect verified at bench (proxy bypass fix applied at controlport.cpp:21) |
| F-163 | Shared Serial Port | 24 | ✓ | S24: patch-in flag exercised at bench |
| F-164 | Port Status Display | 23+24 | ✓ (s23) | S23 2.1: DotIndicator::update_status wiring + color states. **S24 known gap**: state-driven indicator not fully implemented (visible-only); revisit when full status driver lands |
| F-165 | Serial Data Logging | 24 | ✓ | S24: data log verified at bench |
| F-166 | Transparent Transmission | 24 | ✓ | S24: transparent pass-through verified at bench |
| F-167 | USB CAN Communication | 23+24 | ✓ | S23 1.4 Port: GCAN init crash-free without dongle; S24: CAN traffic verified at bench |
| F-168 | Serial Server TCP | 23+24 | ✓ | S23: dialog config; S24: TCP bridge verified at bench (after proxy bypass fix + all-three-ports success check at devicemanager.cpp:586-600) |
| F-169 | Port Health Monitoring | 24 | ✓ | S24: port health poll verified at bench |

## S. File I/O & Data Management

| ID | Feature | Sprint | Status | Notes |
|---|---|---|---|---|
| F-170 | Custom BMP Write | 23 | ✓ | Covered by `test_imageio` (automated) |
| F-171 | Custom TIFF Read | 23 | ✓ | Covered by `test_imageio` (automated) |
| F-172 | JPEG Read/Write | 23 | ✓ | Covered by `test_imageio` (automated) |
| F-173 | Crash Dump (Windows) | – | ☐ | Deferred — `.dmp` generation path not exercised this sprint; revisit when a crash-repro test is needed |
| F-174 | Log Messages | – | ☐ | Currently disabled in code |
| F-175 | Config JSON Persistence | 23 | ✓ | Covered by `test_config` (automated) + S23 1.7 manual full pass (see F-138/F-139/F-140). |
| F-176 | Scan Image Output | 24 | ☐ | **Deferred** alongside scan end-to-end (F-125-F-128) |

## T. System / Platform

| ID | Feature | Sprint | Status | Notes |
|---|---|---|---|---|
| F-177 | Thread Pool Execution | 23 | ✓ | Indirectly tested via F-040 export-video path |
| F-178 | Keyboard Shortcuts | 23 | ✓ | Alt+0/1/2 + mode tabs verified (shortcuts implemented via keyPressEvent, not QShortcut) |
| F-179 | Drag & Drop File Load | 23 | ✓ | Covered as part of F-029/F-030 |
| F-180 | Plugin Interface | – | ☐ | Stub only — not fully implemented |
| F-181 | Multi-Platform Build | – | ↷ | Windows verified; Linux build skipped per user direction this sprint |

---

## New tests recommended (additions to automated suite)

These are pure functions that benefit more from an automated Tier 1 test than from a manual checklist. Suggest adding before/during Sprint 23.

| Proposed test | Covers | Why |
|---|---|---|
| `test_scanparams.cpp` | `get_ptz_route()`, `get_tcu_route()`, `frame_count` derivation | High refactor risk (Sprint 22 rewrote these); pure functions over a struct — easy to assert against known inputs |
| Extend `test_config` | Any new Config fields added during refactor | Ensure round-trip stays intact as Config grows |

## Summary statistics

Counts after the Sprint 23 + Sprint 24 sweep:

| Status | Count | Notes |
|---|---|---|
| ✓ Passed | ~138 | software-only + bench-verified items |
| ↷ Skipped (with reason) | 2 | F-030c (HLS TLS blocker), F-181 (Linux build) |
| ☐ Deferred this iteration | 9 | scan end-to-end (F-125-F-128, F-176), rangefinder (F-131), joystick (F-146-F-148) |
| ☐ Out-of-scope (`–`) | ~22 | dropped per UI redesign or otherwise non-core |
| ☐ Non-build / build-flag specific | 2 | F-030d (cv:N webcam path), F-120 (LVTONG fishnet) |
| **Total** | **181** | |

**Deferred-this-iteration inventory** — to revisit when the relevant hardware is on the bench:
F-125 / F-126 / F-127 / F-128 / F-176 (scan end-to-end — needs full bench: camera + TCU + lens + PTZ live), F-131 (rangefinder), F-146 / F-147 / F-148 (gamepad).

**Out-of-scope inventory** (post UI-redesign-plan reconciliation, 2026-05-14):
F-006 (binning), F-013 / F-014 (SW trigger), F-051 (DIST_BTN math entry), F-064 (auto focus), F-065 (get lens params), F-070 (iris), F-076 (laser preset dialog), F-102 (B/C slider), F-108 (aliasing — back on backlog), F-112 / F-113 (Distance3DView), F-129 / F-130 (auto-scan CLI), F-132 (manual distance), F-142 (developer options), F-145 (legacy bin config), F-151 (hide LEFT), F-160 (misc radio), F-173 (crash dump), F-174 (log msgs), F-180 (plugin stub).
