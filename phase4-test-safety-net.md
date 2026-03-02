# Phase 4: Test Safety Net Plan

**Project**: GLI_user_panel v0.10.2.0
**Date**: 2026-02-28
**Purpose**: Define the characterization test plan that must be implemented before any refactoring begins.
**Principle**: Zero behavior change — tests lock current behavior, including known bugs.

---

## 1. Test Infrastructure

### Framework
- **QtTest (QTest)** — Already available via Qt5; provides `QTEST_MAIN`, `QCOMPARE`, `QVERIFY`, signal spy
- **Supplementary**: OpenCV `cv::Mat` assertions (size, type, pixel value comparison)
- **No additional dependencies** required beyond what the project already links

### CMake Integration
- Add `find_package(Qt5 COMPONENTS Test REQUIRED)` to top-level or new `tests/CMakeLists.txt`
- Link against: `Qt5::Test`, `Qt5::Core`, `Qt5::SerialPort`, `Qt5::Network`, OpenCV libs
- Compile source files under test directly into test executables (no need for separate library targets except `ImgProc` which already exists)

### Test Execution
- Each test file produces a standalone executable
- Run via `ctest` or directly: `./test_imageproc`, `./test_config`, etc.
- CI integration: `ctest --output-on-failure`

---

## 2. Testability Analysis

### Tier 1: Directly Testable (No Mocking Required)

| Module | File(s) | Reason |
|--------|---------|--------|
| **ImageProc** | `imageproc.h/cpp` | Pure static functions, cv::Mat in/out, no side effects |
| **Config** | `config.h/cpp` | JSON round-trip via file I/O, no hardware deps |
| **ImageIO** | `imageio.h/cpp` | File I/O with temp files, no hardware deps |
| **gated3D math** | `imageproc.cpp` (gated3D, gated3D_v2) | Pure math on cv::Mat |

### Tier 2: Testable with Subclass/Stub (Override `communicate()`)

| Module | File(s) | Strategy |
|--------|---------|----------|
| **TCU state logic** | `tcu.h/cpp` | Subclass TCU, override `communicate()` as no-op; test `get()`, parameter state, `to_json()` |
| **PTZ protocol** | `ptz.h/cpp` | Subclass PTZ, capture `communicate()` output; verify Pelco-D framing and checksum |
| **Lens protocol** | `lens.h/cpp` | Same subclass approach as PTZ |
| **Laser protocol** | `laser.h/cpp` | Same subclass approach |

### Tier 3: Testable with Significant Mocking

| Module | File(s) | Challenge |
|--------|---------|-----------|
| **ControlPort** | `controlport.h/cpp` | Needs mock QSerialPort/QTcpSocket for connection state tests |
| **USBCAN** | `usbcan.h/cpp` | Depends on ECanVci64 DLL (Windows-only CAN bus driver) |
| **UDPPTZ** | `udpptz.h/cpp` | Needs mock QUdpSocket |
| **ScanPreset** | `scanpreset.h/cpp` | Depends on TCU + PTZ state |
| **AutoScan** | `autoscan.h/cpp` | Depends on ScanPreset + timer state |

### Tier 4: Untestable in Isolation (Integration Tests Only)

| Module | File(s) | Reason |
|--------|---------|--------|
| **UserPanel** | `userpanel.h/cpp` | 6994-line God class, 200+ members, tight GUI coupling |
| **GrabThread** | `userpanel.cpp:947-1845` | 900-line loop, depends on camera hardware + all processing |
| **Display** | `mywidget.h/cpp` | Mouse/wheel event handling, QLabel rendering |
| **TitleBar** | `mywidget.h/cpp` | Window management, drag/resize |
| **Camera drivers** | `mvcam`, `ebuscam` | Require physical camera hardware or SDK simulator |

---

## 3. Test Specifications

### 3.1 test_imageproc — Image Processing Functions

**Source under test**: `src/image/imageproc.h`, `src/image/imageproc.cpp`
**Link**: `ImgProc` static library (already defined in CMakeLists.txt)

#### TC-IP-001: hist_equalization — Grayscale
- **Feature IDs**: F-070
- **Input**: 100x100 CV_8U Mat filled with value 50 (low contrast)
- **Expected**: Output same size/type; histogram is more spread (stddev > input stddev)
- **Assertion**: `res.size() == src.size()`, `res.type() == CV_8U`, pixel range wider

#### TC-IP-002: hist_equalization — Color (3-channel)
- **Feature IDs**: F-070
- **Input**: 100x100 CV_8UC3 with uniform color (50, 50, 50)
- **Expected**: Output CV_8UC3, same size; each channel equalized independently
- **Assertion**: `res.channels() == 3`, output differs from input

#### TC-IP-003: laplace_transform — Default kernel
- **Feature IDs**: F-071
- **Input**: 100x100 CV_8U with a horizontal edge (top half 0, bottom half 255)
- **Expected**: Edge pixels enhanced, flat areas suppressed
- **Assertion**: `res.size() == src.size()`, edge region has higher values than flat regions

#### TC-IP-004: laplace_transform — Custom kernel
- **Feature IDs**: F-071
- **Input**: Same as TC-IP-003, custom identity kernel `[0,0,0; 0,1,0; 0,0,0]`
- **Expected**: Output identical to input (identity transform)
- **Assertion**: `cv::countNonZero(res - src) == 0`

#### TC-IP-005: plateau_equl_hist — All 7 methods (method 0-6)
- **Feature IDs**: F-072
- **Input**: 256x256 CV_8U with gradient (row i = i)
- **Expected**: Output same size/type, values remapped per method curve
- **Assertion**: For each method 0-6: `res.size() == src.size()`, output differs from input, output values in [0, 255]
- **Note**: Characterization test — record exact output for regression

#### TC-IP-006: brightness_and_contrast — Alpha/Beta
- **Feature IDs**: F-075
- **Input**: 100x100 CV_8U filled with 128
- **Expected**: With alpha=1.5, beta=10: output ≈ 1.5*128+10 = 202 (clamped to 255)
- **Assertion**: All pixels approximately `min(255, alpha * 128 + beta)`

#### TC-IP-007: split_img — 2x2 split
- **Feature IDs**: F-082
- **Input**: 200x200 CV_8U with 4 quadrants of different values
- **Expected**: Output is the interleaved split (quad-pixel arrangement)
- **Assertion**: `res.size() == src.size()`, specific quadrant pixel values preserved

#### TC-IP-008: gated3D — Depth map calculation
- **Feature IDs**: F-079
- **Input**: Two 100x100 CV_8U images (src1 bright, src2 dim), delay=100, gw=40, range=[0, 500], range_thresh=100
- **Expected**: Output CV_8U depth map, values proportional to intensity ratio
- **Assertion**: `res.size() == src1.size()`

#### TC-IP-009: gated3D_v2 — Enhanced depth map with colormap
- **Feature IDs**: F-080
- **Input**: Same as TC-IP-008
- **Expected**: Output CV_8UC3 (colormapped), optional dist_mat filled
- **Assertion**: `res.channels() == 3` when colormap applied

#### TC-IP-010: accumulative_enhance — Piecewise scaling
- **Feature IDs**: F-073
- **Input**: 100x100 CV_8U with gradient 0-255
- **Expected**: Dark pixels (< 64) scaled up more than bright pixels
- **Assertion**: Output `res.at<uchar>(0,0)` (dark) scaled by ~2.4x base, bright pixels scaled less

#### TC-IP-011: adaptive_enhance — Gamma correction
- **Feature IDs**: F-074
- **Input**: 100x100 CV_8U with value 128
- **Expected**: With gamma=1.2, output values adjusted per imadjust curve
- **Assertion**: Output differs from input, stays within [0, 255]

#### TC-IP-012: haze_removal — Dark channel prior
- **Feature IDs**: F-076
- **Input**: 100x100 CV_8UC3 with hazy appearance (all channels ~200)
- **Expected**: Output has better contrast (darker darks, maintained brights)
- **Assertion**: `res.channels() == 3`, output min pixel < input min pixel

---

### 3.2 test_config — Configuration System

**Source under test**: `src/util/config.h`, `src/util/config.cpp`
**Dependencies**: nlohmann/json, Qt5::Core

#### TC-CF-001: Default values after construction
- **Feature IDs**: F-130
- **Input**: `Config config;`
- **Expected**: All defaults match struct initializer values:
  - `com_tcu.port == "COM0"`, `com_tcu.baudrate == 9600`
  - `network.tcp_server_address == "192.168.1.233"`
  - `ui.dark_theme == true`, `ui.english == true`
  - `camera.time_exposure == 95000.0f`
  - `tcu.type == 0`, `tcu.ab_lock == true`, `tcu.max_dist == 15000.0f`
  - `device.ptz_type == 0`
  - `yolo.config_path == "yolo_config.ini"`
- **Assertion**: `QCOMPARE` each field against expected default

#### TC-CF-002: JSON round-trip — All settings
- **Feature IDs**: F-130, F-131
- **Input**: Config with modified values: `com_tcu.port = "COM3"`, `com_tcu.baudrate = 115200`, `network.udp_target_port = 40000`, `ui.dark_theme = false`, `camera.frequency = 30`, `tcu.type = 2`, `tcu.ps_step = {10, 20, 30, 40}`, `device.flip = true`
- **Steps**: `save_to_file(temp)` → new Config → `load_from_file(temp)`
- **Expected**: All fields match original
- **Assertion**: Field-by-field `QCOMPARE` after round-trip

#### TC-CF-003: Partial JSON — Missing fields preserve defaults
- **Feature IDs**: F-130
- **Input**: JSON file with only `{"version": "1.0.0", "ui": {"dark_theme": false}}`
- **Expected**: `ui.dark_theme == false`, all other fields retain defaults
- **Assertion**: Modified fields changed, unmentioned fields at default

#### TC-CF-004: Invalid JSON — Parse error handling
- **Feature IDs**: F-130
- **Input**: File containing `{invalid json`
- **Expected**: `load_from_file()` returns `false`, `config_error` signal emitted
- **Assertion**: `QCOMPARE(result, false)`, `QSignalSpy` catches error signal

#### TC-CF-005: Non-existent file
- **Feature IDs**: F-130
- **Input**: `load_from_file("/nonexistent/path.json")`
- **Expected**: Returns `false`, `config_error` signal emitted
- **Assertion**: `QCOMPARE(result, false)`

#### TC-CF-006: TCU ps_step array round-trip
- **Feature IDs**: F-130
- **Input**: `tcu.ps_step = {10, 20, 30, 40}`
- **Expected**: After save/load, `ps_step[0]==10, [1]==20, [2]==30, [3]==40`
- **Assertion**: Array values match exactly

#### TC-CF-007: YOLO settings — Model paths round-trip
- **Feature IDs**: F-130
- **Input**: Custom model paths for visible/thermal/gated
- **Expected**: Paths preserved, but `main_display_model` etc. always default to 0 (runtime-only)
- **Assertion**: Model paths match, display model selections == 0

#### TC-CF-008: load_defaults() reset
- **Feature IDs**: F-130
- **Input**: Config with modified values, then `load_defaults()`
- **Expected**: All fields reset to struct defaults
- **Assertion**: Compare against freshly constructed ConfigData

#### TC-CF-009: Version field persistence
- **Feature IDs**: F-130
- **Input**: Save with version "0.10.2.0", load
- **Expected**: Version string preserved exactly
- **Assertion**: `QCOMPARE(config.get_version(), "0.10.2.0")`

---

### 3.3 test_imageio — Image File I/O

**Source under test**: `src/image/imageio.h`, `src/image/imageio.cpp`
**Dependencies**: OpenCV, Qt5::Core (for QString)

#### TC-IO-001: save/load BMP — Grayscale 8-bit
- **Feature IDs**: F-120
- **Input**: 100x100 CV_8U gradient image
- **Steps**: `save_image_bmp(img, tempfile)` → read back with OpenCV `imread`
- **Expected**: Loaded image matches original
- **Assertion**: Pixel-by-pixel comparison (accounting for BMP row padding)

#### TC-IO-002: save/load BMP — Color 3-channel
- **Feature IDs**: F-120
- **Input**: 100x100 CV_8UC3 color image
- **Expected**: Color channels preserved (note: save does BGR→RGB conversion)
- **Assertion**: After reading back, pixel values match

#### TC-IO-003: save BMP with NOTE metadata
- **Feature IDs**: F-120
- **Input**: 50x50 CV_8U image, note = "Test note 12345"
- **Expected**: BMP valid, NOTE block appended after pixel data
- **Assertion**: File size > standard BMP size, "NOTE" magic found in file, note text recoverable

#### TC-IO-004: save/load TIF — Grayscale 16-bit
- **Feature IDs**: F-121
- **Input**: 100x100 CV_16U image with values 0-65535
- **Steps**: `save_image_tif(img, tempfile)` → `load_image_tif(loaded, tempfile)`
- **Expected**: Loaded image matches original
- **Assertion**: `QCOMPARE(loaded.type(), CV_16U)`, pixel values match

#### TC-IO-005: save/load TIF — Grayscale 8-bit
- **Feature IDs**: F-121
- **Input**: 100x100 CV_8U image
- **Expected**: Round-trip preserves values
- **Assertion**: Pixel-by-pixel match

#### TC-IO-006: save JPG
- **Feature IDs**: F-122
- **Input**: 100x100 CV_8UC3 color image
- **Steps**: `save_image_jpg(img, tempfile)` → read back
- **Expected**: Image readable, dimensions match (lossy compression may change values)
- **Assertion**: `loaded.size() == img.size()`, `loaded.channels() == 3`

#### TC-IO-007: BMP row padding — Non-multiple-of-4 width
- **Feature IDs**: F-120
- **Input**: 97x50 CV_8U image (97 bytes per row, needs 3 bytes padding to 100)
- **Expected**: BMP file has correct padding, readable by other software
- **Assertion**: File size matches expected BMP size with padding

---

### 3.4 test_tcu_logic — TCU Parameter State Machine

**Source under test**: `src/port/tcu.h`, `src/port/tcu.cpp`
**Dependencies**: `src/port/controlport.h/cpp`, Qt5::Core, Qt5::SerialPort, Qt5::Network
**Strategy**: Subclass TCU, override `communicate()` to capture/discard protocol frames

```cpp
// Test helper subclass
class TestTCU : public TCU {
public:
    TestTCU(int type = 0) : TCU(-1, type) {}
    QList<QByteArray> sent_frames;
protected:
    // Override to capture frames without hardware
    void communicate(QByteArray write, uint, uint, bool) override {
        sent_frames.append(write);
    }
};
```

#### TC-TCU-001: Initial state after construction
- **Feature IDs**: F-030, F-031
- **Input**: `TestTCU tcu(0);` (type 0)
- **Expected**: `get(REPEATED_FREQ) == 30`, `get(LASER_WIDTH) == 40`, `get(DELAY_A) == 100`, `get(GATE_WIDTH_A) == 40`, `get(MCP) == 5`, `get(CCD_FREQ) == 10`
- **Assertion**: `QCOMPARE` each `get()` return value

#### TC-TCU-002: get(GATE_WIDTH_A) bug — Unreachable switch (KNOWN BUG)
- **Feature IDs**: F-031
- **Input**: `TestTCU tcu(2);` (type 2), `tcu.get(TCU::GATE_WIDTH_A)`
- **Expected**: Returns `gate_width_a` directly (line 57) — the `switch(tcu_type)` at lines 58-62 is UNREACHABLE dead code
- **Assertion**: For type 0, 1, 2: `get(GATE_WIDTH_A)` always returns `gate_width_a`, never `gate_width_b`
- **Note**: This characterizes a known bug (S-005 in Phase 2). The test locks the buggy behavior.

#### TC-TCU-003: Type 0 protocol frame — REPEATED_FREQ
- **Feature IDs**: F-033
- **Input**: `TestTCU tcu(0); tcu.set_user_param(REPEATED_FREQ, 20.0);`
- **Expected**: Frame = `[0x88, 0x00, <4-byte big-endian int of (1e6/8)/20 = 6250>, 0x99]`
- **Assertion**: `sent_frames.last()` matches expected bytes

#### TC-TCU-004: Type 0 protocol frame — DELAY_A
- **Feature IDs**: F-034
- **Input**: `TestTCU tcu(0); tcu.set_user_param(DELAY_A, 200.0);`
- **Expected**: Frame = `[0x88, 0x02, <round(200) as 4-byte big-endian>, 0x99]`
- **Assertion**: Frame bytes match

#### TC-TCU-005: Type 1 protocol — DELAY_A with ps_step
- **Feature IDs**: F-034
- **Input**: `TestTCU tcu(1); tcu.set_user_param(DELAY_A, 100.0);`
- **Expected**: Two frames sent: integer part (100/4 = 25) + decimal part (ps step)
- **Assertion**: `sent_frames.size()` increased by 2, frame bytes match calculation

#### TC-TCU-006: Type 2 protocol — Parameter remapping
- **Feature IDs**: F-034
- **Input**: `TestTCU tcu(2); tcu.set_user_param(DELAY_A, 100.0);`
- **Expected**: DELAY_A remapped to DELAY_B command byte (0x04), frame sent with 10ps stepping
- **Assertion**: Frame cmd byte == 0x04 (DELAY_B), not 0x02 (DELAY_A)

#### TC-TCU-007: EST_DIST — Distance-to-delay conversion
- **Feature IDs**: F-036
- **Input**: `TestTCU tcu(0); tcu.set_user_param(EST_DIST, 1500.0);` (1500m)
- **Expected**: `delay_a = 1500 / dist_ns`, `dist_ns = 3e8 * 1e-9 / 2 = 0.15 m/ns`, so `delay_a = 10000 ns`
- **Assertion**: `get(DELAY_A) ≈ 10000`, `get(EST_DIST) == 1500`

#### TC-TCU-008: EST_DIST — Auto PRF calculation
- **Feature IDs**: F-036, F-041
- **Input**: `TestTCU tcu(0);` with auto_rep_freq=true, `set_user_param(EST_DIST, 3000.0);`
- **Expected**: `rep_freq = 1e6 / (delay/dist_ns + dov/dist_ns + 1000)`, clamped to max 30
- **Assertion**: `get(REPEATED_FREQ)` matches calculated PRF

#### TC-TCU-009: EST_DOV — Depth of view with AB lock
- **Feature IDs**: F-037
- **Input**: `TestTCU tcu(0);` with ab_lock=true, `set_user_param(EST_DOV, 300.0);`
- **Expected**: `gate_width_a = 300 / dist_ns = 2000`, `gate_width_b = gate_width_a + gate_width_n`
- **Assertion**: Both gate widths updated

#### TC-TCU-010: LASER_ON — Bitfield laser control
- **Feature IDs**: F-042
- **Input**: `set_user_param(LASER_ON, 0b1010)` (lasers 2 and 4 on)
- **Expected**: Two LASER_x frames sent (for changed bits), with value 8 (on) or 4 (off)
- **Assertion**: Correct number of frames sent, correct laser indices

#### TC-TCU-011: to_json — Serialization
- **Feature IDs**: F-043
- **Input**: TCU with modified parameters
- **Expected**: JSON contains all keys: type, PRF, laser_width, delay_a, gw_a, delay_b, gw_b, ccd_freq, duty, mcp, delay_n, gw_n
- **Assertion**: All keys present, values match `get()` returns

#### TC-TCU-012: AB lock off — Independent A/B control
- **Feature IDs**: F-040
- **Input**: `set_user_param(AB_LOCK, 0u)`, then `set_user_param(EST_DIST, 1000.0);`
- **Expected**: Only DELAY_A updated, DELAY_B unchanged
- **Assertion**: `get(DELAY_B)` still at initial value (100)

#### TC-TCU-013: LASER_USR — Laser width with offset
- **Feature IDs**: F-042
- **Input**: `set_user_param(OFFSET_LASER, 5.0)`, then `set_user_param(LASER_USR, 40.0)`
- **Expected**: Internal laser_width = 40, but wire value = 40 + 5 = 45
- **Assertion**: `get(LASER_WIDTH)` reflects internal + offset

---

### 3.5 test_ptz_protocol — PTZ Pelco-D Protocol

**Source under test**: `src/port/ptz.h`, `src/port/ptz.cpp`
**Dependencies**: `src/port/controlport.h/cpp`, Qt5::Core, Qt5::SerialPort, Qt5::Network
**Strategy**: Subclass PTZ, override `communicate()` to capture frames

```cpp
class TestPTZ : public PTZ {
public:
    TestPTZ(uchar addr = 0x01, uint speed = 16) : PTZ(-1, addr, speed) {}
    QList<QByteArray> sent_frames;
    QByteArray mock_response;
protected:
    void communicate(QByteArray write, uint, uint, bool) override {
        sent_frames.append(write);
        retrieve_mutex.lock();
        last_read = mock_response;
        retrieve_mutex.unlock();
    }
};
```

#### TC-PTZ-001: Checksum calculation
- **Feature IDs**: F-050
- **Input**: `QByteArray{0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}` (STOP command)
- **Expected**: Checksum = `(0xFF + 0x01 + 0x00 + 0x00 + 0x00 + 0x00 + 0x00 + 1) & 0xFF = 0x01`
- **Assertion**: `checksum(data) == 0x01`
- **Note**: The Pelco-D checksum in this implementation adds 1 to the sum (non-standard — standard Pelco-D does not add 1). This is the current behavior and must be preserved.

#### TC-PTZ-002: STOP command encoding
- **Feature IDs**: F-051
- **Input**: `ptz_control(PTZ::STOP)`
- **Expected**: Frame = `[0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, chksm]`
- **Assertion**: Frame matches expected bytes

#### TC-PTZ-003: Direction commands — Speed encoding
- **Feature IDs**: F-052
- **Input**: PTZ with speed=16, `ptz_control(PTZ::UP)`
- **Expected**: Frame = `[0xFF, addr, 0x00, 0x08, 0x00, 0x10, chksm]` (speed in byte 5)
- **Assertion**: Byte 4 == 0x00 (no horizontal speed), byte 5 == 0x10 (vertical speed)

#### TC-PTZ-004: Direction commands — Diagonal encoding
- **Feature IDs**: F-052
- **Input**: `ptz_control(PTZ::UP_RIGHT)`
- **Expected**: Frame = `[0xFF, addr, 0x00, 0x0A, speed, speed, chksm]` (both H and V speed)
- **Assertion**: Bytes 4 and 5 both contain speed value

#### TC-PTZ-005: SET_H — Horizontal angle encoding
- **Feature IDs**: F-053
- **Input**: `ptz_control(PTZ::SET_H, 180.5)`
- **Expected**: Angle = `int(180.5 * 100) % 36000 = 18050`, encoded big-endian in bytes 4-5
- **Assertion**: `byte[4] == (18050 >> 8) & 0xFF`, `byte[5] == 18050 & 0xFF`

#### TC-PTZ-006: SET_H — Negative angle wrapping
- **Feature IDs**: F-053
- **Input**: `ptz_control(PTZ::SET_H, -10.0)`
- **Expected**: Angle = `(int(-10.0 * 100) % 36000 + 36000) % 36000 = 35000`
- **Assertion**: Encoded angle == 35000

#### TC-PTZ-007: SET_V — Vertical angle clamping
- **Feature IDs**: F-054
- **Input**: `ptz_control(PTZ::SET_V, 50.0)` (exceeds ±40° limit)
- **Expected**: Clamped to 40.0°, angle = `int(40.0 * 100) % 36000 = 4000`
- **Assertion**: Encoded angle == 4000

#### TC-PTZ-008: SET_V — Negative vertical angle
- **Feature IDs**: F-054
- **Input**: `ptz_control(PTZ::SET_V, -30.0)`
- **Expected**: angle_v = -30.0, angle = `(int(-30.0 * 100) % 36000 + 36000) % 36000`
- **Assertion**: Encoded angle matches calculation

#### TC-PTZ-009: SPEED — Speed clamping
- **Feature IDs**: F-055
- **Input**: `ptz_control(PTZ::SPEED, 100)` (exceeds max 0x3F=63)
- **Expected**: Speed clamped to 63 (0x3F)
- **Assertion**: `get(PTZ::SPEED) == 63`

#### TC-PTZ-010: SPEED — Minimum speed
- **Feature IDs**: F-055
- **Input**: `ptz_control(PTZ::SPEED, 0)` (below min 0x01)
- **Expected**: Speed clamped to 1 (0x01)
- **Assertion**: `get(PTZ::SPEED) == 1`

#### TC-PTZ-011: ADDRESS — Address change
- **Feature IDs**: F-056
- **Input**: `ptz_control(PTZ::ADDRESS, 5)`
- **Expected**: Subsequent commands use address 5 in byte 1
- **Assertion**: Next command frame byte[1] == 0x05

#### TC-PTZ-012: to_json — State serialization
- **Feature IDs**: F-057
- **Input**: PTZ with address=3, speed=32, angle_h=90.5, angle_v=-15.0
- **Expected**: JSON contains address, angle_h, angle_v, speed
- **Assertion**: All keys present with correct values

#### TC-PTZ-013: Angle feedback parsing — Horizontal
- **Feature IDs**: F-053
- **Input**: Mock response `[0xFF, addr, 0x00, 0x59, 0x46, 0x50, chksm]` (angle = 0x4650 = 18000 → 180.0°)
- **Expected**: `angle_h` updated to 180.0
- **Assertion**: After `try_communicate()`, `get(ANGLE_H) == 180.0`

#### TC-PTZ-014: Angle feedback parsing — Vertical with offset
- **Feature IDs**: F-054
- **Input**: Mock response with `[0xFF, addr, 0x00, 0x5B, <angle_bytes>, chksm]`
- **Expected**: `angle_v` calculated as `((fb + 4000) % 36000 - 4000) / 100.0`, clamped to ±40°
- **Assertion**: Correct angle_v for various feedback values

---

## 4. Coverage Map — Tests to Feature IDs

| Test Suite | Feature IDs Covered | Coverage Type |
|------------|-------------------|---------------|
| test_imageproc | F-070 to F-082 | Direct (Tier 1) |
| test_config | F-130, F-131 | Direct (Tier 1) |
| test_imageio | F-120 to F-122 | Direct (Tier 1) |
| test_tcu_logic | F-030 to F-043 | Stubbed (Tier 2) |
| test_ptz_protocol | F-050 to F-057 | Stubbed (Tier 2) |

### Features NOT Covered by Unit Tests

| Feature Range | Category | Reason | Mitigation |
|--------------|----------|--------|------------|
| F-001 to F-029 | Camera Control | Hardware-dependent, Cam virtual interface | Manual testing, future mock camera |
| F-044 to F-049 | Lens, Laser, Rangefinder | Serial protocol, similar to PTZ | Add when protocols are extracted |
| F-058 to F-069 | USBCAN, UDPPTZ | Platform-specific DLL/socket | Mock socket tests later |
| F-083 to F-099 | Video Recording, Streaming | FFmpeg pipeline | Integration tests after extraction |
| F-100 to F-119 | GUI Display, Mouse Events | QWidget rendering | Manual + QTest::mouseClick later |
| F-132 to F-145 | Preferences, ScanConfig dialogs | Modal dialogs | Manual testing |
| F-146 to F-170 | UserPanel orchestration | God class, 200+ member deps | After decomposition |
| F-171 to F-181 | Window management, themes | Platform-specific rendering | Manual testing |

---

## 5. Known Bugs Locked by Tests

| Test ID | Bug Description | Location |
|---------|----------------|----------|
| TC-TCU-002 | `get(GATE_WIDTH_A)` returns early before type-switch | tcu.cpp:57-62 |
| TC-PTZ-001 | Checksum adds 1 (non-standard Pelco-D) | ptz.cpp:257 |
| TC-IO-004 | `load_image_tif()` returns `-1` (int) for bool function | imageio.cpp:414,418,421 |
| TC-IO-004 | `load_image_tif()` uses static local variables (not thread-safe) | imageio.cpp:371-377 |
| — | Memory leak: `new uchar[7]{...}` in PTZ/TCU constructors | ptz.cpp:10-13, tcu.cpp:32 |
| — | Memory leak: `new uchar[7]{...}` in `ptz_control()` goto paths | ptz.cpp:118-127 |

---

## 6. Test Implementation Priority

**Order of implementation** (implement in this order when refactoring begins):

1. **test_config** — Lowest risk, pure data, validates JSON persistence layer
2. **test_imageproc** — Pure functions, validates core image processing pipeline
3. **test_imageio** — File I/O, validates image save/load cycle
4. **test_tcu_logic** — Complex state machine, high refactoring risk area
5. **test_ptz_protocol** — Protocol encoding, validates wire format preservation

### Estimated Test Count
- test_config: 9 test cases
- test_imageproc: 12 test cases
- test_imageio: 7 test cases
- test_tcu_logic: 13 test cases
- test_ptz_protocol: 14 test cases
- **Total: 55 characterization test cases**

---

## 7. Pre-Refactoring Checklist

Before starting any code modification:

- [ ] All 55 test cases written and passing
- [ ] Test executable builds alongside main project via CMake
- [ ] CI/CD pipeline (if any) runs tests automatically
- [ ] Baseline test results recorded (snapshot of current behavior)
- [ ] Each test clearly documents which Feature ID it covers
- [ ] Known bugs documented in tests with `// KNOWN BUG: ...` comments
- [ ] No test depends on hardware (all Tier 1 or Tier 2 with stubs)

---

**STOP — Phase 4 Complete**
Please review this test safety net plan. When you're ready, confirm to proceed to Phase 5 (Refactoring Roadmap).
