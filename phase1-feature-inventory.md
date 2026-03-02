# Phase 1 - Feature Inventory

> **Project:** GLI_user_panel (v0.10.2.0)
> **Tech Stack:** C++11 / Qt5 / OpenCV 4.5.4 / FFmpeg 5.1.3 / ONNX Runtime
> **Build:** CMake 3.5+ / MSVC 2015-2022 (Windows primary), GCC (Linux secondary)
> **Date:** 2026-02-28

---

## Table of Contents

1. [Project Structure Annotation](#1-project-structure-annotation)
2. [Entry Points](#2-entry-points)
3. [Feature Inventory Table](#3-feature-inventory-table)
   - [A. Camera / Device Management](#a-camera--device-management)
   - [B. Image Grabbing & Display](#b-image-grabbing--display)
   - [C. Image / Video Capture & Recording](#c-image--video-capture--recording)
   - [D. TCU (Timing Control Unit)](#d-tcu-timing-control-unit)
   - [E. Lens Control](#e-lens-control)
   - [F. Laser Control](#f-laser-control)
   - [G. PTZ (Pan-Tilt-Zoom)](#g-ptz-pan-tilt-zoom)
   - [H. UDP PTZ (VID_PAGE Camera Control)](#h-udp-ptz-vid_page-camera-control)
   - [I. Image Processing](#i-image-processing)
   - [J. 3D Gated Imaging](#j-3d-gated-imaging)
   - [K. AI / Object Detection (YOLO)](#k-ai--object-detection-yolo)
   - [L. Scanning System](#l-scanning-system)
   - [M. Rangefinder & Distance](#m-rangefinder--distance)
   - [N. Preset Management](#n-preset-management)
   - [O. Configuration & Settings](#o-configuration--settings)
   - [P. Joystick / Gamepad Input](#p-joystick--gamepad-input)
   - [Q. UI / Display Management](#q-ui--display-management)
   - [R. Communication / Serial](#r-communication--serial)
   - [S. File I/O & Data Management](#s-file-io--data-management)
   - [T. System / Platform](#t-system--platform)
4. [Summary Statistics](#4-summary-statistics)

---

## 1. Project Structure Annotation

```
GLI_user_panel/
├── CMakeLists.txt                  # Build config (430 lines) - multi-platform, multi-SDK
├── README.md                       # Chinese documentation
├── zh_cn.ts / zh_cn.qm            # Chinese translation source / compiled
│
├── src/
│   ├── main.cpp                    # Entry point: app init, fonts, themes, crash handler
│   │
│   ├── automation/                 # [PARTIALLY DISABLED] Scanning automation
│   │   ├── autoscan.h/cpp          #   State-machine based auto-scan controller
│   │   └── scanpreset.h/cpp        #   Preset data structures and validation
│   │
│   ├── cam/                        # Camera drivers (polymorphic via Cam base class)
│   │   ├── cam.h                   #   Abstract camera interface
│   │   ├── mvcam.h/cpp             #   Hikvision MVS SDK driver
│   │   ├── ebuscam.h/cpp           #   Pleora eBUS SDK driver (Windows only)
│   │   ├── euresyscam.h/cpp        #   Euresys CameraLink driver (optional build)
│   │   └── hqvcam.h/cpp            #   HQV camera driver
│   │
│   ├── image/                      # Image processing & file I/O
│   │   ├── imageio.h/cpp           #   BMP/TIFF/JPEG save/load (custom implementations)
│   │   └── imageproc.h/cpp         #   Enhancement, temporal denoise, 3D gating, haze removal
│   │
│   ├── port/                       # Hardware device communication
│   │   ├── controlport.h/cpp       #   Abstract base: serial + TCP, timer-based polling
│   │   ├── tcu.h/cpp               #   Timing Control Unit protocol (3 device types)
│   │   ├── lens.h/cpp              #   Motorized lens (zoom, focus, radius)
│   │   ├── laser.h/cpp             #   Laser power control
│   │   ├── ptz.h/cpp               #   Pan-Tilt-Zoom dome (Pelco-style protocol)
│   │   ├── rangefinder.h/cpp       #   Distance measurement device
│   │   ├── huanyu.h/cpp            #   Huanyu device interface
│   │   ├── usbcan.h/cpp            #   CAN bus (GCAN - ECanVci64)
│   │   └── udpptz.h/cpp            #   UDP-based PTZ (0xEB90 protocol)
│   │
│   ├── thread/                     # Threading
│   │   ├── joystick.h/cpp          #   Gamepad input polling (Windows multimedia API)
│   │   └── controlportthread.h/cpp #   Serial/TCP worker threads (legacy, partially used)
│   │
│   ├── util/                       # Utilities
│   │   ├── util.h/cpp              #   Globals: fonts, cursors, themes, mutex
│   │   ├── config.h/cpp            #   JSON config (nlohmann/json)
│   │   ├── threadpool.h/cpp        #   Async task execution
│   │   └── version.h               #   Version constants (0.10.2.0)
│   │
│   ├── visual/                     # GUI layer
│   │   ├── userpanel.h/cpp         #   MAIN WINDOW (710 + 6994 lines) — core orchestrator
│   │   ├── user_panel.ui           #   Main window layout (4317 lines)
│   │   ├── preferences.h/cpp/ui    #   Settings dialog (5 tabs)
│   │   ├── scanconfig.h/cpp/ui     #   Scan parameter configuration
│   │   ├── lasercontrol.h/cpp      #   Laser control panel (24 buttons)
│   │   ├── laser_control.ui        #   Laser control layout
│   │   ├── developeroptions.h/cpp/ui # Debug/developer settings
│   │   ├── serialserver.h/cpp/ui   #   TCP serial server config
│   │   ├── presetpanel.h/cpp       #   Preset save/load interface
│   │   ├── aliasing.h/cpp          #   Aliasing parameter recommendations
│   │   └── distance3dview.h/cpp/ui #   3D scatter visualization (optional build)
│   │
│   ├── widgets/                    # Custom Qt widgets
│   │   └── mywidget.h/cpp          #   Display, TitleBar, StatusBar, FloatingWindow, etc.
│   │
│   ├── yolo/                       # AI object detection
│   │   ├── yolo_detector.h/cpp     #   Detection wrapper
│   │   ├── inference.h/cpp         #   ONNX Runtime engine
│   │   └── yolo_app_config.h       #   INI-based model configuration
│   │
│   ├── plugins/                    # Plugin system (stub)
│   │   └── plugininterface.h       #   Plugin interface definition
│   │
│   └── _deprecated/               # Legacy code (not compiled)
│       ├── videosurface.h/cpp      #   Old QMediaPlayer surface
│       ├── progsettings.h/cpp      #   Old settings system
│       └── settings.ui             #   Old settings UI
│
├── resources/                      # Assets
│   ├── resources.qrc               #   Qt resource index
│   ├── style_dark.qss / style_light.qss  # Theme stylesheets
│   ├── fonts/                      #   Monaco, Consolas
│   ├── cursor/                     #   Custom cursors (dark/light variants)
│   └── tools/                      #   Toolbar icons
│
└── others/                         # Supplementary tools & docs
    ├── moving_average/             #   Standalone frame averaging tool
    ├── python-udp-ptz/             #   UDP PTZ simulator (Python)
    ├── frame_averager/             #   Temporal denoising tool
    ├── bmp_note_reader/            #   BMP metadata reader
    └── refactor/                   #   Existing refactoring notes
```

---

## 2. Entry Points

| Entry Point | File | Description |
|---|---|---|
| **Application Main** | `src/main.cpp:110` | QApplication init, font/theme loading, crash handler, `UserPanel` creation |
| **Main Window Init** | `src/visual/userpanel.cpp` (UserPanel::init) | Device threads, signal-slot wiring, config loading, UI setup |
| **Grab Thread** | `src/visual/userpanel.h:50` (GrabThread::run) | Per-display image acquisition loop |
| **Control Port Timer** | `src/port/controlport.cpp` (ControlPort::try_communicate) | Periodic device polling (1s interval) |
| **UDP PTZ Timers** | `src/port/udpptz.cpp` | TX (20ms) and RX (5ms) polling loops |

---

## 3. Feature Inventory Table

### A. Camera / Device Management

| ID | Feature Name | Trigger | Files Involved | Input | Output / Effect | Complexity | Notes |
|---|---|---|---|---|---|---|---|
| F-001 | Enumerate Devices | `ENUM_BUTTON` click | userpanel.cpp, mvcam.cpp, ebuscam.cpp | None | Populates device list in Preferences | Medium | Supports Hikvision, eBUS, Euresys, HQV |
| F-002 | Start Device | `START_BUTTON` click | userpanel.cpp, cam.h implementations | Device index | Camera initialized, controls enabled | High | Creates camera instance based on `device_type` |
| F-003 | Shutdown Device | `SHUTDOWN_BUTTON` click | userpanel.cpp, cam.h implementations | None | Camera stopped, resources released | Medium | Calls `shut_down()` |
| F-004 | Set Camera Parameters | `SET_PARAMS_BUTTON` click | userpanel.cpp, cam.h | Exposure, gain, FPS, pixel type | Camera hardware configured | Medium | Writes to camera SDK |
| F-005 | Get Camera Parameters | `GET_PARAMS_BUTTON` click | userpanel.cpp, cam.h | None | UI fields populated with current values | Low | Reads from camera SDK |
| F-006 | Camera Binning Mode | `BINNING_CHECK` toggle | userpanel.cpp, mvcam.cpp | Checkbox state | 2x2 pixel binning on/off | Low | Hikvision cameras only |
| F-007 | Set Pixel Format | Preferences dialog | userpanel.cpp, preferences.cpp | Pixel format index | Mono8/Mono12/RGB8 selection | Low | |
| F-008 | Set Image Region (ROI) | `IMG_REGION_BTN` click | userpanel.cpp, cam.h | Region coordinates | Camera ROI configured | Medium | |
| F-009 | Set Sensor Taps | `SENSOR_TAPS_BTN` click | userpanel.cpp | Tap mode | Sensor readout configuration | Low | |
| F-010 | Device IP Configuration | Preferences dialog | userpanel.cpp, preferences.cpp | IP, gateway | Camera network reconfigured | Low | GigE cameras only |

### B. Image Grabbing & Display

| ID | Feature Name | Trigger | Files Involved | Input | Output / Effect | Complexity | Notes |
|---|---|---|---|---|---|---|---|
| F-011 | Set Continuous Mode | `CONTINUOUS_RADIO` click | userpanel.cpp, cam.h | None | Camera in free-run mode | Low | |
| F-012 | Set Trigger Mode | `TRIGGER_RADIO` click | userpanel.cpp, cam.h | None | Camera waits for trigger | Low | |
| F-013 | Software Trigger Enable | `SOFTWARE_CHECK` toggle | userpanel.cpp | Checkbox state | Enables manual SW trigger | Low | |
| F-014 | Manual Software Trigger | `SOFTWARE_TRIGGER_BUTTON` click | userpanel.cpp, cam.h | None | Single frame acquired | Low | |
| F-015 | Start Grabbing | `START_GRABBING_BUTTON` click | userpanel.cpp, GrabThread | None | Grab threads started, display active | High | Up to 3 concurrent grab threads |
| F-016 | Stop Grabbing | `STOP_GRABBING_BUTTON` click | userpanel.cpp, GrabThread | None | Grab threads stopped | Medium | |
| F-017 | Main Display Rendering | Continuous (GrabThread) | userpanel.cpp, mywidget.cpp | cv::Mat frames | QPixmap displayed in SOURCE_DISPLAY | High | Includes processing pipeline |
| F-018 | Histogram Display | Continuous (GrabThread) | userpanel.cpp | cv::Mat frames | Histogram shown in HIST_DISPLAY | Medium | |
| F-019 | Alt Display 1 | `MISC_RADIO_1` + options | userpanel.cpp, mywidget.cpp | Display selection | Content in FloatingWindow 1 | Medium | |
| F-020 | Alt Display 2 | `MISC_RADIO_2` + options | userpanel.cpp, mywidget.cpp | Display selection | Content in FloatingWindow 2 | Medium | |
| F-021 | Dual Display Mode | `DUAL_DISPLAY_CHK` toggle | userpanel.cpp | Checkbox state | Side-by-side display | Low | |
| F-022 | Mouse Zoom Tool | `ZOOM_TOOL` click | userpanel.cpp, mywidget.cpp | Mouse wheel/drag | Image zoom in/out | Medium | |
| F-023 | Mouse Select Tool (ROI) | `SELECT_TOOL` click | userpanel.cpp, mywidget.cpp | Mouse rectangle | ROI selected on image | Medium | |
| F-024 | Mouse PTZ Tool | `PTZ_TOOL` click | userpanel.cpp, mywidget.cpp | Mouse click on image | PTZ pointed to click location | Medium | |
| F-025 | Center on Screen | `CENTER_CHECK` toggle | userpanel.cpp | Checkbox state | Image centered in display | Low | |
| F-026 | Info Overlay | `INFO_CHECK` toggle | userpanel.cpp | Checkbox state | Frame info overlaid on display | Low | |
| F-027 | Image Rotation | Preferences setting | userpanel.cpp | Angle (0/90/180/270) | Rotated display output | Low | |
| F-028 | Image Flip | Preferences setting | userpanel.cpp | Flip flag | Vertically flipped display | Low | |
| F-029 | Static Image Display | Drag-and-drop file | userpanel.cpp, imageio.cpp | BMP/TIFF/JPEG file | Image shown in display | Medium | |
| F-030 | Video File Playback | Drag-and-drop file | userpanel.cpp | Video file path | Video played in display | High | Uses FFmpeg/OpenCV VideoCapture |
| F-031 | FPS Counter | Continuous (TimedQueue) | userpanel.h, mywidget.cpp | Frame timestamps | FPS shown in status bar | Low | |

### C. Image / Video Capture & Recording

| ID | Feature Name | Trigger | Files Involved | Input | Output / Effect | Complexity | Notes |
|---|---|---|---|---|---|---|---|
| F-032 | Save Original Image | `SAVE_BMP_BUTTON` click | userpanel.cpp, imageio.cpp | None | BMP file saved to `save_location` | Medium | Includes metadata (NOTE block) |
| F-033 | Save Processed Image | `SAVE_RESULT_BUTTON` click | userpanel.cpp, imageio.cpp | None | Processed BMP saved | Medium | |
| F-034 | Record Original Video | `SAVE_AVI_BUTTON` click | userpanel.cpp | None | AVI recording started/stopped | High | cv::VideoWriter, up to 4 streams |
| F-035 | Record Processed Video | `SAVE_FINAL_BUTTON` click | userpanel.cpp | None | Processed AVI recording | High | |
| F-036 | Alt Display Capture | `CAPTURE_ALT1/2_BTN` click | userpanel.cpp | None | Alt display image saved | Medium | |
| F-037 | Alt Display Record | `RECORD_ALT1/2_BTN` click | userpanel.cpp | None | Alt display video recording | Medium | |
| F-038 | File Path Browse | `FILE_PATH_BROWSE` click | userpanel.cpp | None | QFileDialog for save location | Low | |
| F-039 | Screenshot | TitleBar button | userpanel.cpp | None | Full window screenshot saved | Low | |
| F-040 | Export Video | Preferences option | userpanel.cpp | Video file | Processed video export with effects | High | Background thread processing |
| F-041 | BMP Metadata (NOTE) | Automatic with F-032/F-033 | imageio.cpp | Timestamp + string | Metadata appended to BMP file | Low | Custom format |

### D. TCU (Timing Control Unit)

| ID | Feature Name | Trigger | Files Involved | Input | Output / Effect | Complexity | Notes |
|---|---|---|---|---|---|---|---|
| F-042 | Set Delay A | `DELAY_A_EDIT_*` editing | userpanel.cpp, tcu.cpp | us/ns/ps values | TCU delay A register updated | Medium | 3 unit fields (us, ns, ps) |
| F-043 | Set Delay B | `DELAY_B_EDIT_*` editing | userpanel.cpp, tcu.cpp | us/ns/ps values | TCU delay B register updated | Medium | Linked to A via AB-lock |
| F-044 | Set Gate Width A | `GATE_WIDTH_A_EDIT_*` editing | userpanel.cpp, tcu.cpp | us/ns/ps values | TCU gate width A updated | Medium | |
| F-045 | Set Gate Width B | `GATE_WIDTH_B_EDIT_*` editing | userpanel.cpp, tcu.cpp | us/ns/ps values | TCU gate width B updated | Medium | Linked to A via AB-lock |
| F-046 | Set Laser Width | `LASER_WIDTH_EDIT_*` editing | userpanel.cpp, tcu.cpp | us/ns/ps values | Laser pulse width updated | Medium | |
| F-047 | Set Frequency (PRF) | `FREQ_EDIT` editing | userpanel.cpp, tcu.cpp | Frequency value | PRF register updated | Medium | Unit depends on hz_unit setting |
| F-048 | Set MCP (Gain) | `MCP_SLIDER` / `MCP_EDIT` | userpanel.cpp, tcu.cpp | 0-31 value | MCP gain register updated | Low | |
| F-049 | Delay Slider | `DELAY_SLIDER` drag | userpanel.cpp, tcu.cpp | Slider position | Delay A (and B if locked) updated | Medium | Maps slider range to ns |
| F-050 | Gate Width Slider | `GW_SLIDER` drag | userpanel.cpp, tcu.cpp | Slider position | Gate width A (and B if locked) updated | Medium | |
| F-051 | Distance / Gate Width Dialog | `DIST_BTN` click | userpanel.cpp, tcu.cpp | Distance in meters | Auto-calculates delay, GW, PRF | High | Core ranging calculation |
| F-052 | Auto PRF | Preferences toggle | userpanel.cpp, tcu.cpp | Checkbox state | PRF auto-adjusts based on delay | Medium | `1e6 / (delay + DOV + 1000)` |
| F-053 | AB Lock | Preferences toggle | userpanel.cpp, tcu.cpp | Checkbox state | Delay B = A + offset_N | Low | |
| F-054 | Auto MCP | `AUTO_MCP_CHK` toggle | userpanel.cpp, tcu.cpp | Checkbox state | MCP auto-adjusts for brightness | Medium | Adaptive gain |
| F-055 | TCU Type Selection | Preferences / `SWITCH_TCU_UI_BTN` | userpanel.cpp, tcu.cpp | Type 0/1/2 | Protocol and UI layout changes | High | 3 device types with different protocols |
| F-056 | PS Stepping Config | Preferences dialog | userpanel.cpp, tcu.cpp | ps values per step | Picosecond precision timing | Medium | For Type 1/2 TCUs |
| F-057 | Offset Configuration | Preferences dialog | userpanel.cpp, tcu.cpp | Delay/GW/Laser offsets | Calibration offsets applied | Low | |
| F-058 | Individual Laser Enable | TCU commands | userpanel.cpp, tcu.cpp | Laser 1-4 on/off | Individual lasers toggled | Low | |
| F-059 | TCU Parameter Feedback | Continuous (timer) | tcu.cpp, controlport.cpp | None | TCU params read and displayed | Medium | 1s polling |

### E. Lens Control

| ID | Feature Name | Trigger | Files Involved | Input | Output / Effect | Complexity | Notes |
|---|---|---|---|---|---|---|---|
| F-060 | Zoom In | `ZOOM_IN_BTN` press/release | userpanel.cpp, lens.cpp | Hold duration | Lens zoom motor in | Low | |
| F-061 | Zoom Out | `ZOOM_OUT_BTN` press/release | userpanel.cpp, lens.cpp | Hold duration | Lens zoom motor out | Low | |
| F-062 | Focus Far | `FOCUS_FAR_BTN` press/release | userpanel.cpp, lens.cpp | Hold duration | Lens focus motor far | Low | |
| F-063 | Focus Near | `FOCUS_NEAR_BTN` press/release | userpanel.cpp, lens.cpp | Hold duration | Lens focus motor near | Low | |
| F-064 | Auto Focus | `AUTO_FOCUS_BTN` click | userpanel.cpp, lens.cpp | None | Automated focus sweep | High | Clarity-based algorithm |
| F-065 | Get Lens Parameters | `GET_LENS_PARAM_BTN` click | userpanel.cpp, lens.cpp | None | Zoom/focus/radius shown | Low | |
| F-066 | Set Zoom Position | Direct input | userpanel.cpp, lens.cpp | Zoom value | Motor moves to position | Medium | Blocking wait loop |
| F-067 | Set Focus Position | Direct input | userpanel.cpp, lens.cpp | Focus value | Motor moves to position | Medium | Blocking wait loop |
| F-068 | Laser Radius Inc/Dec | `RADIUS_INC/DEC_BTN` press/release | userpanel.cpp, lens.cpp | Hold duration | Laser radius adjusted | Low | |
| F-069 | Focus Speed Control | `FOCUS_SPEED_SLIDER` | userpanel.cpp, lens.cpp | Speed 1-63 | Lens motor speed set | Low | |
| F-070 | Iris Open/Close | `IRIS_OPEN/CLOSE_BTN` press/release | userpanel.cpp, lens.cpp | Hold duration | Iris aperture adjusted | Low | |
| F-071 | Lens Position Feedback | Continuous (timer) | lens.cpp, controlport.cpp | None | Zoom/focus/radius polled | Medium | 3-query rotation, 1s interval |

### F. Laser Control

| ID | Feature Name | Trigger | Files Involved | Input | Output / Effect | Complexity | Notes |
|---|---|---|---|---|---|---|---|
| F-072 | Laser On/Off Toggle | `LASER_BTN` click | userpanel.cpp, tcu.cpp | None | Laser toggled via TCU | Low | |
| F-073 | Fire Laser | `FIRE_LASER_BTN` click | userpanel.cpp, laser.cpp | None | Single laser fire command | Low | |
| F-074 | Simple Laser Mode | `SIMPLE_LASER_CHK` toggle | userpanel.cpp | Checkbox | Simplified laser control UI | Low | |
| F-075 | Laser Current Control | `CURRENT_EDIT` | userpanel.cpp, laser.cpp | Current value | Laser power level set | Medium | |
| F-076 | Laser Preset Positions | LaserControl dialog | lasercontrol.cpp, lens.cpp | 24 buttons (6x4 grid) | Motor moves to preset lens positions | High | 4 laser groups with speed control |
| F-077 | Dual Light Control | `DUAL_LIGHT_BTN` click | userpanel.cpp | None | Toggles dual illumination | Low | |

### G. PTZ (Pan-Tilt-Zoom)

| ID | Feature Name | Trigger | Files Involved | Input | Output / Effect | Complexity | Notes |
|---|---|---|---|---|---|---|---|
| F-078 | PTZ Directional Move | 8 direction buttons | userpanel.cpp, ptz.cpp | Button press/release | PTZ moves in direction | Medium | Pelco protocol, 8 directions |
| F-079 | PTZ Stop | `STOP_BTN` click | userpanel.cpp, ptz.cpp | None | PTZ movement stopped | Low | |
| F-080 | Get PTZ Angle | `GET_ANGLE_BTN` click | userpanel.cpp, ptz.cpp | None | H/V angles displayed | Low | |
| F-081 | Set PTZ Angle | Angle edit fields + enter | userpanel.cpp, ptz.cpp | H (0-360), V (-40 to 40) | PTZ moves to position | Medium | |
| F-082 | PTZ Speed Control | `PTZ_SPEED_SLIDER` | userpanel.cpp, ptz.cpp | Speed 1-63 | PTZ movement speed set | Low | |
| F-083 | PTZ Angle Feedback | Continuous (timer) | ptz.cpp, controlport.cpp | None | H/V angles polled | Medium | Alternates H/V queries, 1s |
| F-084 | Point PTZ to Target | Mouse click on image (PTZ tool) | userpanel.cpp, ptz.cpp | Screen coordinates | PTZ redirected to clicked point | High | Coordinate transform |
| F-085 | PTZ Self-Check | PTZ initialization | ptz.cpp | None | PTZ goes to home position | Low | |

### H. UDP PTZ (VID_PAGE Camera Control)

| ID | Feature Name | Trigger | Files Involved | Input | Output / Effect | Complexity | Notes |
|---|---|---|---|---|---|---|---|
| F-086 | VL Zoom In/Out | `VL_ZOOM_IN/OUT_BTN` press/release | userpanel.cpp, udpptz.cpp | Hold duration | Visible light camera zoom | Low | UDP 0xEB90 protocol |
| F-087 | VL Focus Far/Near | `VL_FOCUS_FAR/NEAR_BTN` press/release | userpanel.cpp, udpptz.cpp | Hold duration | VL camera focus | Low | |
| F-088 | IR Zoom In/Out | `IR_ZOOM_IN/OUT_BTN` press/release | userpanel.cpp, udpptz.cpp | Hold duration | Infrared camera zoom | Low | |
| F-089 | IR Focus Far/Near | `IR_FOCUS_FAR/NEAR_BTN` press/release | userpanel.cpp, udpptz.cpp | Hold duration | IR camera focus | Low | |
| F-090 | VL Defog | `VL_DEFOG_BTN` toggle | userpanel.cpp, udpptz.cpp | Toggle state | Defog filter on/off | Low | |
| F-091 | IR Power | `IR_POWER_BTN` toggle | userpanel.cpp, udpptz.cpp | Toggle state | IR illuminator on/off | Low | |
| F-092 | IR Auto Focus | `IR_AUTO_FOCUS_BTN` click | userpanel.cpp, udpptz.cpp | None | IR auto-focus triggered | Low | |
| F-093 | Low-Distance Mode | `LDM_BTN` toggle | userpanel.cpp, udpptz.cpp | Toggle state | Close-range mode on/off | Low | |
| F-094 | Video Source Toggle | `VIDEO_SOURCE_BTN` toggle | userpanel.cpp, udpptz.cpp | Toggle state | Switch VL/IR video source | Low | |
| F-095 | OSD Toggle | `OSD_BTN` toggle | userpanel.cpp, udpptz.cpp | Toggle state | On-screen display on/off | Low | |
| F-096 | UDP PTZ Angle Feedback | Continuous (timer, 5ms RX) | udpptz.cpp | None | H/V angles from UDP device | Medium | Status frame parsing |
| F-097 | UDP PTZ Movement | PTZ buttons when UDP mode | udpptz.cpp | Direction command | PTZ servo positioned | Medium | |

### I. Image Processing

| ID | Feature Name | Trigger | Files Involved | Input | Output / Effect | Complexity | Notes |
|---|---|---|---|---|---|---|---|
| F-098 | Image Enhancement | `IMG_ENHANCE_CHECK` toggle + options | userpanel.cpp, imageproc.cpp | Enhancement type | Enhanced image displayed | Medium | Multiple algorithms |
| F-099 | Histogram Equalization | Enhancement option | imageproc.cpp | cv::Mat | Equalized image | Low | Per-channel |
| F-100 | Adaptive Enhancement | Enhancement option | imageproc.cpp | cv::Mat + gamma | Contrast-stretched image | Medium | |
| F-101 | Haze Removal (DCP) | Enhancement option | imageproc.cpp | cv::Mat | Dehazed image | High | Dark channel prior + guided filter |
| F-102 | Brightness/Contrast | `BRIGHTNESS/CONTRAST_SLIDER` | userpanel.cpp, imageproc.cpp | Slider values | Image B/C adjusted | Low | |
| F-103 | Gamma Adjustment | `GAMMA_SLIDER` | userpanel.cpp, imageproc.cpp | Slider value | Gamma-corrected image | Low | |
| F-104 | Frame Averaging | `FRAME_AVG_CHECK` toggle + options | userpanel.cpp, imageproc.cpp | 4 or 8 frames | Temporally denoised image | High | ECC registration + fusion |
| F-105 | Temporal Denoise (ECC) | Frame avg with motion compensation | imageproc.cpp | Frame buffer | Aligned and fused frames | High | Translation/Euclidean/Affine/Homography |
| F-106 | Pseudocolor | `PSEUDOCOLOR_CHK` toggle | userpanel.cpp, imageproc.cpp | Checkbox state | Colormap applied to grayscale | Low | |
| F-107 | Image Splitting | Processing pipeline | imageproc.cpp | 2x2 image | 4 separate quadrant images | Low | |
| F-108 | Aliasing Recommendations | Aliasing panel selection | aliasing.cpp, userpanel.cpp | Selection ID | Delay preset applied | Low | Suspected patch-in |

### J. 3D Gated Imaging

| ID | Feature Name | Trigger | Files Involved | Input | Output / Effect | Complexity | Notes |
|---|---|---|---|---|---|---|---|
| F-109 | 3D Image Building | `IMG_3D_CHECK` toggle | userpanel.cpp, imageproc.cpp | Consecutive frames | Depth map generated | High | Delay-weighted frame fusion |
| F-110 | 3D Range Threshold | `RANGE_THRESH_EDIT` | userpanel.cpp, imageproc.cpp | Threshold value | Depth clipping applied | Low | |
| F-111 | Reset 3D Parameters | `RESET_3D_BTN` click | userpanel.cpp | None | 3D params reset to defaults | Low | |
| F-112 | 3D Scatter Visualization | Distance3DView widget | distance3dview.cpp | Distance matrix | 3D scatter plot rendered | High | Qt Data Visualization (optional build) |
| F-113 | 3D View Controls | Angle/zoom/projection controls | distance3dview.cpp | Spinbox/slider/combo | View angle and projection changed | Medium | |
| F-114 | Paint 3D (Colormap) | Processing pipeline | imageproc.cpp | Distance data | Colorized depth visualization | Medium | |

### K. AI / Object Detection (YOLO)

| ID | Feature Name | Trigger | Files Involved | Input | Output / Effect | Complexity | Notes |
|---|---|---|---|---|---|---|---|
| F-115 | YOLO Model Selection (Main) | `MAIN_MODEL_LIST` combo | userpanel.cpp, yolo_detector.cpp | Model index | Detection model loaded | High | Visible/Thermal/Gated models |
| F-116 | YOLO Model Selection (Alt1) | `ALT1_MODEL_LIST` combo | userpanel.cpp, yolo_detector.cpp | Model index | Detection model for alt1 | High | |
| F-117 | YOLO Model Selection (Alt2) | `ALT2_MODEL_LIST` combo | userpanel.cpp, yolo_detector.cpp | Model index | Detection model for alt2 | High | |
| F-118 | YOLO Inference | Per-frame in grab thread | inference.cpp, yolo_detector.cpp | cv::Mat frame | Bounding boxes + class labels | High | ONNX Runtime, optional CUDA |
| F-119 | Draw Detection Boxes | Post-inference | userpanel.cpp | YoloResult vector | Annotated image with boxes | Low | |
| F-120 | Fishnet Detection Result | Detection callback | userpanel.cpp | Detection count | Result displayed in FISHNET_RESULT | Low | LVTONG build only |

### L. Scanning System

| ID | Feature Name | Trigger | Files Involved | Input | Output / Effect | Complexity | Notes |
|---|---|---|---|---|---|---|---|
| F-121 | Start Scan | `SCAN_BUTTON` click | userpanel.cpp, scanconfig.cpp | Scan config | TCU+PTZ scanning initiated | High | Combined parameter sweep |
| F-122 | Continue Scan | `CONTINUE_SCAN_BUTTON` click | userpanel.cpp | None | Paused scan resumed | Low | |
| F-123 | Restart Scan | `RESTART_SCAN_BUTTON` click | userpanel.cpp | None | Scan restarted from beginning | Low | |
| F-124 | Scan Configuration | `SCAN_CONFIG_BTN` click | scanconfig.cpp | None | ScanConfig dialog opened | Medium | TCU + PTZ params |
| F-125 | TCU Parameter Scan | Scan system | userpanel.cpp, tcu.cpp | Delay/GW range + step | Parameters swept automatically | High | |
| F-126 | PTZ Route Scan | Scan system | userpanel.cpp, ptz.cpp | Angle range + step | PTZ raster scan | High | Combined with TCU scan |
| F-127 | Scan Image Capture | During scan | userpanel.cpp, imageio.cpp | None | Images saved per scan step | Medium | |
| F-128 | Scan Video Recording | During scan | userpanel.cpp | None | Video recorded during scan | Medium | |
| F-129 | Auto-Scan (CLI) | Command-line `--auto-scan` | autoscan.cpp, scanpreset.cpp | Preset file path | Automated scan execution | High | Currently DISABLED |
| F-130 | Scan Preset Validation | Auto-scan system | scanpreset.cpp | Preset JSON | Multi-part parameter validation | Medium | Currently DISABLED |

### M. Rangefinder & Distance

| ID | Feature Name | Trigger | Files Involved | Input | Output / Effect | Complexity | Notes |
|---|---|---|---|---|---|---|---|
| F-131 | Distance Measurement | Continuous (rangefinder) | rangefinder.cpp | None | Distance displayed in UI | Medium | 0x5C protocol, checksum validation |
| F-132 | Manual Distance Input | `DIST_BTN` dialog | userpanel.cpp | Meters value | TCU params calculated from distance | Medium | |
| F-133 | Underwater Mode | Preferences toggle | userpanel.cpp, config.cpp | Checkbox | Light speed adjusted for water | Low | |

### N. Preset Management

| ID | Feature Name | Trigger | Files Involved | Input | Output / Effect | Complexity | Notes |
|---|---|---|---|---|---|---|---|
| F-134 | Save Preset | PresetPanel save button | presetpanel.cpp, userpanel.cpp | Preset slot index | Current params saved to JSON | Medium | |
| F-135 | Apply Preset | PresetPanel select row | presetpanel.cpp, userpanel.cpp | Preset JSON data | All device params restored | High | TCU + lens + camera state |
| F-136 | Preset Table Display | PresetPanel open | presetpanel.cpp | None | Preset table shown | Low | |

### O. Configuration & Settings

| ID | Feature Name | Trigger | Files Involved | Input | Output / Effect | Complexity | Notes |
|---|---|---|---|---|---|---|---|
| F-137 | Preferences Dialog | TitleBar settings button | preferences.cpp | None | 5-tab settings dialog | High | 40+ configurable fields |
| F-138 | JSON Config Load | App startup / file browse | config.cpp, userpanel.cpp | default.json file | All settings restored | Medium | |
| F-139 | JSON Config Save | App close / manual | config.cpp, userpanel.cpp | None | Settings persisted to JSON | Medium | |
| F-140 | Config Auto-Save | App close event | config.cpp | None | default.json updated | Low | |
| F-141 | Config File Browse | TitleBar button | userpanel.cpp | None | QFileDialog for config file | Low | |
| F-142 | Developer Options | Preferences sub-dialog | developeroptions.cpp | None | Lens calibration + intensity params | Low | |
| F-143 | Serial Server Config | Preferences sub-dialog | serialserver.cpp | None | Port type (TCP/UDP/RS232/RS485) | Low | |
| F-144 | Gatewidth Lookup Table | Config file loading | userpanel.cpp | Lookup table file | GW values indexed by serial number | Medium | Suspected patch-in |
| F-145 | Legacy Binary Config | `ENABLE_USER_DEFAULT` flag | main.cpp, userpanel.cpp | user_default file | COM ports, theme, language | Low | DEPRECATED, replaced by JSON |

### P. Joystick / Gamepad Input

| ID | Feature Name | Trigger | Files Involved | Input | Output / Effect | Complexity | Notes |
|---|---|---|---|---|---|---|---|
| F-146 | Joystick PTZ Control | Joystick D-pad/stick | joystick.cpp, userpanel.cpp | Direction input | PTZ movement commands | Medium | Windows multimedia API |
| F-147 | Joystick Button Actions | Joystick A/B/X/Y/L/R | joystick.cpp, userpanel.cpp | Button press/release | Mapped to lens/laser/zoom actions | Medium | |
| F-148 | Joystick Direction | Joystick hat/stick | joystick.cpp, userpanel.cpp | 8 directions | PTZ movement in direction | Medium | |

### Q. UI / Display Management

| ID | Feature Name | Trigger | Files Involved | Input | Output / Effect | Complexity | Notes |
|---|---|---|---|---|---|---|---|
| F-149 | Dark/Light Theme Toggle | TitleBar theme button | main.cpp, userpanel.cpp, util.h | None | Stylesheet + cursors switched | Low | |
| F-150 | Language Switch (EN/ZH) | TitleBar language button | userpanel.cpp | None | UI text translated | Low | zh_cn.qm translation |
| F-151 | Hide Left Panel | `HIDE_BTN` click | userpanel.cpp | None | Parameter panel hidden/shown | Low | |
| F-152 | Window Maximize/Restore | TitleBar button or Alt+Enter | mywidget.cpp, userpanel.cpp | None | Window maximized or restored | Low | |
| F-153 | Custom Title Bar | Always visible | mywidget.cpp | None | Custom drag, buttons, status | Medium | TitleBar widget |
| F-154 | Custom Status Bar | Always visible | mywidget.cpp | None | 8 device status icons | Medium | StatusBar widget |
| F-155 | Floating Windows | Alt+0/1/2 shortcuts | mywidget.cpp, userpanel.cpp | Key combination | Secondary displays shown/hidden | Medium | |
| F-156 | Window Resize (Custom) | Mouse drag on borders | userpanel.cpp | Mouse position | Frameless window resizing | Medium | Custom hit-testing |
| F-157 | Switch TCU UI Layout | `SWITCH_TCU_UI_BTN` click | userpanel.cpp | None | TCU parameter display reorganized | Low | |
| F-158 | COM Data Display | `DATA_EXCHANGE` text edit | userpanel.cpp | Serial data | Raw data log shown | Low | |
| F-159 | Clean Log | TitleBar clean button | userpanel.cpp | None | DATA_EXCHANGE cleared | Low | |
| F-160 | Misc Radio Selection | `MISC_RADIO_1/2/3` | userpanel.cpp | Radio index | Bottom panel content switched | Low | |

### R. Communication / Serial

| ID | Feature Name | Trigger | Files Involved | Input | Output / Effect | Complexity | Notes |
|---|---|---|---|---|---|---|---|
| F-161 | Serial Port Init | App init / preferences | controlport.cpp, userpanel.cpp | COM port, baudrate | Serial ports opened | Medium | Up to 5 ports |
| F-162 | TCP Port Init | Preferences toggle | controlport.cpp, userpanel.cpp | IP, port | TCP connections established | Medium | |
| F-163 | Shared Serial Port | Preferences toggle | userpanel.cpp | Checkbox | TCU port shared with other devices | Medium | Suspected patch-in |
| F-164 | Port Status Display | Continuous | mywidget.cpp, userpanel.cpp | Port states | Status icons updated | Low | |
| F-165 | Serial Data Logging | Continuous | controlport.cpp, userpanel.cpp | TX/RX data | Data shown in DATA_EXCHANGE | Low | |
| F-166 | Transparent Transmission | Menu or hotkey | userpanel.cpp | File data | Raw data sent to COM port | Medium | |
| F-167 | USB CAN Communication | Init + continuous | usbcan.cpp | None | CAN bus angle data received | Medium | GCAN ECanVci64 library |
| F-168 | Serial Server TCP | SerialServer dialog | serialserver.cpp | Port types | TCP-to-serial bridge configured | Medium | |
| F-169 | Port Health Monitoring | Continuous (timer) | controlport.cpp | Successive count | Connection health tracked | Low | |

### S. File I/O & Data Management

| ID | Feature Name | Trigger | Files Involved | Input | Output / Effect | Complexity | Notes |
|---|---|---|---|---|---|---|---|
| F-170 | Custom BMP Write | Save operations | imageio.cpp | cv::Mat + metadata | BMP file with optional NOTE block | Medium | Custom DIB implementation |
| F-171 | Custom TIFF Read | Drag-and-drop | imageio.cpp | TIFF file | cv::Mat loaded with IFD parsing | Medium | Custom IFD reader |
| F-172 | JPEG Read/Write | File operations | imageio.cpp | JPEG file | cv::Mat via OpenCV | Low | |
| F-173 | Crash Dump (Windows) | Unhandled exception | main.cpp | Exception info | .dmp file in dmp/ folder | Medium | MiniDumpWriteDump |
| F-174 | Log Messages | qDebug/qWarning/etc. | main.cpp | Qt messages | Log file in GLI_logs/ | Low | Currently disabled |
| F-175 | Config JSON Persistence | Load/Save | config.cpp | JSON file | Full config serialized/deserialized | Medium | |
| F-176 | Scan Image Output | During scan | userpanel.cpp | Scan frames | Timestamped images in directories | Medium | |

### T. System / Platform

| ID | Feature Name | Trigger | Files Involved | Input | Output / Effect | Complexity | Notes |
|---|---|---|---|---|---|---|---|
| F-177 | Thread Pool Execution | Image save, video export | threadpool.cpp | Callable tasks | Async task execution | Medium | |
| F-178 | Keyboard Shortcuts | Various key combos | userpanel.cpp | Key events | Feature triggers (Alt+0/1/2, etc.) | Low | |
| F-179 | Drag & Drop File Load | File dropped on window | userpanel.cpp | File path | Image/video loaded and displayed | Medium | |
| F-180 | Plugin Interface | Plugin system (stub) | plugininterface.h | Plugin DLL | Extended functionality | Low | Not fully implemented |
| F-181 | Multi-Platform Build | CMake configuration | CMakeLists.txt | Platform detection | Windows/Linux builds | Medium | |

---

## 4. Summary Statistics

### Total Feature Count: **181 features**

### Features by Module / Area

| Module | Feature Count | Primary Files |
|---|---|---|
| Camera / Device Management | 10 (F-001 to F-010) | userpanel.cpp, cam/ |
| Image Grabbing & Display | 21 (F-011 to F-031) | userpanel.cpp, mywidget.cpp |
| Image / Video Capture | 10 (F-032 to F-041) | userpanel.cpp, imageio.cpp |
| TCU Control | 18 (F-042 to F-059) | userpanel.cpp, tcu.cpp |
| Lens Control | 12 (F-060 to F-071) | userpanel.cpp, lens.cpp |
| Laser Control | 6 (F-072 to F-077) | userpanel.cpp, laser.cpp, lasercontrol.cpp |
| PTZ Control | 8 (F-078 to F-085) | userpanel.cpp, ptz.cpp |
| UDP PTZ | 12 (F-086 to F-097) | userpanel.cpp, udpptz.cpp |
| Image Processing | 11 (F-098 to F-108) | userpanel.cpp, imageproc.cpp |
| 3D Gated Imaging | 6 (F-109 to F-114) | userpanel.cpp, imageproc.cpp, distance3dview.cpp |
| AI / Object Detection | 6 (F-115 to F-120) | userpanel.cpp, yolo_detector.cpp, inference.cpp |
| Scanning System | 10 (F-121 to F-130) | userpanel.cpp, scanconfig.cpp, autoscan.cpp |
| Rangefinder & Distance | 3 (F-131 to F-133) | rangefinder.cpp, userpanel.cpp |
| Preset Management | 3 (F-134 to F-136) | presetpanel.cpp, userpanel.cpp |
| Configuration & Settings | 9 (F-137 to F-145) | config.cpp, preferences.cpp, userpanel.cpp |
| Joystick / Gamepad | 3 (F-146 to F-148) | joystick.cpp, userpanel.cpp |
| UI / Display Management | 12 (F-149 to F-160) | userpanel.cpp, mywidget.cpp |
| Communication / Serial | 9 (F-161 to F-169) | controlport.cpp, userpanel.cpp |
| File I/O & Data | 7 (F-170 to F-176) | imageio.cpp, config.cpp, main.cpp |
| System / Platform | 5 (F-177 to F-181) | threadpool.cpp, userpanel.cpp, CMakeLists.txt |

### Complexity Distribution

| Complexity | Count | Percentage |
|---|---|---|
| Low | 82 | 45.3% |
| Medium | 69 | 38.1% |
| High | 30 | 16.6% |

### Features Flagged as "Suspected Patch-In"

| ID | Feature | Evidence |
|---|---|---|
| F-108 | Aliasing Recommendations | Isolated widget with single signal, loosely coupled |
| F-120 | Fishnet Detection Result | Guarded by `#ifdef LVTONG`, customer-specific patch |
| F-129 | Auto-Scan (CLI) | Entire feature commented out, incomplete integration |
| F-130 | Scan Preset Validation | Part of auto-scan, disabled |
| F-133 | Underwater Mode | Single boolean flag changing light speed constant |
| F-144 | Gatewidth Lookup Table | Separate config file reader, serial-number indexed |
| F-145 | Legacy Binary Config | `ENABLE_USER_DEFAULT` flag, explicitly marked DEPRECATED |
| F-163 | Shared Serial Port | Flag-based port sharing, adds conditional logic throughout |
| F-167 | USB CAN Communication | Alternative PTZ input path, parallel to serial PTZ |
| F-180 | Plugin Interface | Stub only, no implementation |

### File Concentration (Lines of Code)

| File | Lines | Feature Count (touches) | Risk Assessment |
|---|---|---|---|
| userpanel.cpp | 6,994 | ~140 (77%) | **Critical** - God Module |
| userpanel.h | 710 | ~140 | **Critical** - God Header |
| user_panel.ui | 4,317 | ~60 | **High** - Monolithic UI |
| imageproc.cpp | 1,152 | 11 | Medium |
| preferences.cpp | 758 | 9 | Medium |
| scanconfig.cpp | 482 | 10 | Medium |
| tcu.cpp | 481 | 18 | Medium |
| udpptz.cpp | 394 | 12 | Medium |
| imageio.cpp | 440 | 7 | Low |
| lens.cpp | 311 | 12 | Low |
| inference.cpp | 295 | 6 | Low |
| ptz.cpp | 260 | 8 | Low |

---

> **Cross-references:**
> - Phase 2 will diagnose architectural issues identified here (especially the userpanel.cpp God Module)
> - Phase 3 will map the dependency relationships between the modules listed
> - Phase 4 tests will reference Feature IDs (F-xxx) from this table
> - Phase 5 refactoring tasks will target the high-concentration files identified in the summary
