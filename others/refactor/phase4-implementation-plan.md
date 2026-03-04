# Phase 4 Implementation: Test Safety Net

## Context

Phase 4 was originally planned as a prerequisite before refactoring. Sprints 0-3 were executed without it because they involved no structural changes. Implementing tests now serves two purposes:
1. Retroactively verify sprints 0-3 didn't break wire format or state behavior
2. Provide the safety net for upcoming sprints 4-9 (copy-paste elimination through UserPanel decomposition)

Commit as "phase 4: test safety net implementation" — no sprint renumbering needed.

## Scope Exclusions

**UserPanel** (userpanel.h/cpp) and **Preferences** (preferences.h/cpp) are intentionally excluded:
- UserPanel is a 6994-line God class tightly coupled to GUI (QSliders, QLineEdits, QLabels), camera hardware, and serial ports. Testing it requires QApplication + .ui form + hardware stubs — impractical in current form.
- Preferences is a modal dialog with 60+ member variables mirroring UserPanel state. Testing the sync logic requires both classes plus the GUI form.
- Both will be heavily refactored in Sprints 7-9. Tests for extracted components (FramePreprocessor, TCUController, etc.) will be written when those components become independently testable.
- The strategy is to test the modules they depend on: ImageProc, Config, ImageIO, and the protocol classes. These modules ARE independently testable and will be touched by upcoming sprints.

## Prerequisite: Make `communicate()` virtual

`ControlPort::communicate()` (controlport.h:62) is a non-virtual protected slot. PTZ/Lens/TCU call it directly to send protocol frames. Without overriding it, tests cannot capture the bytes being sent — `communicate()` attempts real serial/TCP I/O on unopened ports and returns early, losing the frame.

**Change:** Add `virtual` to controlport.h:62:
```cpp
virtual void communicate(QByteArray write, uint read_size = 0, uint read_timeout = 40, bool heartbeat = false);
```

Zero runtime impact — all production calls go through the same single implementation.

## Test Infrastructure

- **Framework:** QtTest (Qt5, already available)
- **Directory:** `tests/`
- **Execution:** Each test is a standalone executable via `ctest --output-on-failure`

### CMake additions (bottom of CMakeLists.txt)

```cmake
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)
enable_testing()
```

Plus one `add_executable` + `add_test` per test suite. Each test links:
- `Qt5::Test`, `Qt5::Core`
- Protocol tests also link `Qt5::SerialPort`, `Qt5::Network` (ControlPort base needs them)
- ImageProc tests link OpenCV

## Test Suites

### Tier 1: Pure Functions (no mocking needed)

#### test_constants — 5 cases
Verifies Sprint 3 constant values match the magic numbers they replaced.

| ID | What | Expected |
|----|------|----------|
| TC-C-001 | Pelco shared: SYNC_BYTE, MIN_SPEED, MAX_SPEED, READ_TIMEOUT_MS | 0xFF, 0x01, 0x3F, 40 |
| TC-C-002 | Angle: ANGLE_SCALE, FULL_ROTATION, VERTICAL_LIMIT, VERTICAL_DEG | 100, 36000, 4000, 40.0 |
| TC-C-003 | TCU: TCU_HEAD, TCU_TAIL, TCU_QUERY_CMD, TCU_CLOCK_8, TCU_CLOCK_4 | 0x88, 0x99, 0x15, 8, 4 |
| TC-C-004 | Physics: LIGHT_SPEED_M_NS | 0.15 (3e8 * 1e-9 / 2) |
| TC-C-005 | Pipeline: MAX_QUEUE_SIZE, AUTO_MCP_DIVISOR, THROTTLE_MS, MAX_MCP_8BIT, MAX_MCP_12BIT | 5, 200, 40, 255, 4095 |

Files: `tests/test_constants.cpp`
Sources: `src/util/constants.h`, `src/port/pelco_protocol.h`, `src/port/tcu_protocol.h`
Protects: Sprint 3 (constant extraction), all future sprints that use these constants

#### test_imageproc — 12 cases
Pure static functions on cv::Mat. Safety net for Sprint 8 (pipeline extraction).

| ID | Function | Input | Assertion |
|----|----------|-------|-----------|
| TC-IP-001 | hist_equalization (grayscale) | 100x100 CV_8U value=50 | Same size/type, stddev increased |
| TC-IP-002 | hist_equalization (color) | 100x100 CV_8UC3 (50,50,50) | 3-channel output, differs from input |
| TC-IP-003 | laplace_transform (default) | 100x100 CV_8U horizontal edge | Edge pixels enhanced, flat suppressed |
| TC-IP-004 | laplace_transform (identity) | Same, kernel=[0,0,0;0,1,0;0,0,0] | Output == input |
| TC-IP-005 | plateau_equl_hist (methods 0-6) | 256x256 CV_8U gradient | Same size, output in [0,255], differs from input |
| TC-IP-006 | brightness_and_contrast | 100x100 CV_8U value=128, alpha=1.5, beta=10 | Pixels ≈ min(255, 1.5*128+10) |
| TC-IP-007 | split_img (2x2) | 200x200 CV_8U 4-quadrant | Same size, quadrant values preserved |
| TC-IP-008 | gated3D | Two 100x100 CV_8U, delay=100, gw=40 | Output CV_8U depth map, same size |
| TC-IP-009 | gated3D_v2 | Same as TC-IP-008 | Output CV_8UC3 when colormap applied |
| TC-IP-010 | accumulative_enhance | 100x100 CV_8U gradient | Dark pixels scaled up more |
| TC-IP-011 | adaptive_enhance | 100x100 CV_8U value=128, gamma=1.2 | Differs from input, in [0,255] |
| TC-IP-012 | haze_removal | 100x100 CV_8UC3 value=200 | Better contrast, darker darks |

Files: `tests/test_imageproc.cpp`
Sources: `src/image/imageproc.h/cpp` (links against ImgProc static lib)
Protects: Sprint 8 (pipeline extraction)

#### test_config — 9 cases
JSON round-trip. Safety net for Sprint 7 (Config as truth).

| ID | What | Assertion |
|----|------|-----------|
| TC-CF-001 | Default values after construction | All fields match struct initializer defaults |
| TC-CF-002 | JSON round-trip — all settings | Save -> load -> field-by-field comparison |
| TC-CF-003 | Partial JSON — missing fields | Modified fields changed, others at default |
| TC-CF-004 | Invalid JSON | load returns false |
| TC-CF-005 | Non-existent file | load returns false |
| TC-CF-006 | ps_step array round-trip | Array values match exactly |
| TC-CF-007 | YOLO paths round-trip | Paths preserved |
| TC-CF-008 | load_defaults() reset | All fields back to struct defaults |
| TC-CF-009 | Version field persistence | Version string preserved |

Files: `tests/test_config.cpp`
Sources: `src/util/config.h/cpp`
Protects: Sprint 7 (Config as single source of truth)

#### test_imageio — 7 cases
File I/O round-trip. Safety net for Sprint 8.

| ID | What | Assertion |
|----|------|-----------|
| TC-IO-001 | BMP save/load grayscale 8-bit | Pixel-by-pixel match |
| TC-IO-002 | BMP save/load color 3-channel | Color channels preserved |
| TC-IO-003 | BMP with NOTE metadata | NOTE block appended, text recoverable |
| TC-IO-004 | TIF save/load 16-bit | CV_16U type preserved, pixels match |
| TC-IO-005 | TIF save/load 8-bit | Pixels match |
| TC-IO-006 | JPG save (lossy) | Dimensions match, channels correct |
| TC-IO-007 | BMP row padding (non-multiple-of-4 width) | File size correct with padding |

Files: `tests/test_imageio.cpp`
Sources: `src/image/imageio.h/cpp`
Protects: Sprint 8 (pipeline extraction)

### Tier 2: Protocol Tests (stubbed communicate)

All protocol test suites use the same pattern — a test subclass that overrides `communicate()` to capture frames:
```cpp
class TestXXX : public XXX {
    Q_OBJECT
public:
    TestXXX(...) : XXX(-1, ...) {}
    QList<QByteArray> frames;
    void communicate(QByteArray write, uint, uint, bool) override {
        frames.append(write);
    }
};
```

#### test_ptz — 14 cases
Verifies PTZ Pelco-D protocol encoding. Retroactively confirms sprints 0 (generate_ba), 1 (goto->break), 3 (constants).

| ID | What | Assertion |
|----|------|-----------|
| TC-P-001 | STOP frame | `[FF, 01, 00, 00, 00, 00, chk]` |
| TC-P-002 | UP: speed in byte[5] only | byte[4]=0, byte[5]=speed |
| TC-P-003 | UP_RIGHT: both speeds | byte[4]=speed, byte[5]=speed |
| TC-P-004 | All 10 directions -> correct byte[3] | Maps to PTZ_DIR_* constants |
| TC-P-005 | SET_H(180.5) -> angle=18050 | Big-endian in bytes[4-5] |
| TC-P-006 | SET_H(-10) -> wraps to 35000 | Negative angle wrapping |
| TC-P-007 | SET_V(50) -> clamped to 40 deg | Angle=4000 |
| TC-P-008 | SET_V(-30) -> negative vertical | Correct encoding |
| TC-P-009 | SPEED(100) -> clamped to 63 | get(SPEED)==63 |
| TC-P-010 | SPEED(0) -> clamped to 1 | get(SPEED)==1 |
| TC-P-011 | ADDRESS(5) -> next frame byte[1]=5 | Address propagation |
| TC-P-012 | Checksum = (sum+1)&0xFF | Non-standard +1 preserved |
| TC-P-013 | Horizontal angle feedback | Mock response -> get(ANGLE_H)==180.0 |
| TC-P-014 | Vertical angle feedback with offset | Mock response -> correct angle_v |

Files: `tests/test_ptz.cpp`
Sources: `src/port/ptz.h/cpp`, `src/port/controlport.h/cpp`
Protects: Sprint 0.2, 1.5, 3.5, future Sprint 6 (PTZ unification)

#### test_lens — 10 cases
Verifies Lens protocol encoding. Confirms Sprint 3 (constants + generate_ba elimination).

| ID | What | Assertion |
|----|------|-----------|
| TC-L-001 | STOP frame | `[FF, 01, 00, 00, 00, 00, chk]` |
| TC-L-002 | ZOOM_IN: byte[3]=0x20, byte[5]=speed | Direction + speed position |
| TC-L-003 | FOCUS_NEAR: byte[2]=0x01 | Cmd1 position (byte 2, not 3) |
| TC-L-004 | RADIUS_UP/DOWN byte[2] | Byte 2 command encoding |
| TC-L-005 | SET_ZOOM(0x1234) | bytes[4-5] = 0x12, 0x34 |
| TC-L-006 | SET_FOCUS/SET_RADIUS | Same big-endian pattern |
| TC-L-007 | RELAY1_ON/OFF | byte[3] = 0x09 / 0x0B |
| TC-L-008 | STEPPING(100) -> clamped to 63 | Shared MAX_SPEED constant |
| TC-L-009 | Query frames | Correct LENS_QUERY_* bytes |
| TC-L-010 | Checksum | Same (sum+1)&0xFF algorithm |

Files: `tests/test_lens.cpp`
Sources: `src/port/lens.h/cpp`, `src/port/controlport.h/cpp`
Protects: Sprint 3.5, 3.6, future Sprint 4 (dedup)

#### test_tcu — 13 cases
Verifies TCU state machine + protocol. Confirms Sprint 0.6 (GATE_WIDTH_A fix) and Sprint 3 (constants).

| ID | What | Assertion |
|----|------|-----------|
| TC-T-001 | Defaults: rep_freq=30, laser_width=40, delay_a=100, gw_a=40, mcp=5 | Initial state |
| TC-T-002 | Type 0: get(GATE_WIDTH_A) returns gate_width_a | Fixed behavior (Sprint 0.6) |
| TC-T-003 | Type 2: get(GATE_WIDTH_A) returns gate_width_b | Type 2 remap |
| TC-T-004 | Type 2: get(LASER_WIDTH) returns gate_width_a | Type 2 remap |
| TC-T-005 | Type 2: get(DELAY_A) returns delay_b | Type 2 remap |
| TC-T-006 | Type 0: REPEATED_FREQ frame, data=(1e6/8)/20 | Protocol encoding |
| TC-T-007 | Type 0: DELAY_A frame, data=round(200) | Integer encoding |
| TC-T-008 | Type 1: DELAY_A -> 2 frames (int + ps) | Dual-frame stepping |
| TC-T-009 | Type 2: DELAY_A -> wire cmd remapped to 0x04 (DELAY_B) | Type 2 command remap |
| TC-T-010 | Frame structure: [0x88, cmd, d0, d1, d2, d3, 0x99] | TCU_HEAD/TCU_TAIL constants |
| TC-T-011 | EST_DIST -> delay_a = dist / LIGHT_SPEED_M_NS | Physics constant usage |
| TC-T-012 | AB_LOCK off -> only DELAY_A updated | Independent A/B control |
| TC-T-013 | Auto PRF: dist=3000 -> rep_freq calculated, <=30 kHz | TCU_MAX_PRF_KHZ clamping |

Files: `tests/test_tcu.cpp`
Sources: `src/port/tcu.h/cpp`, `src/port/controlport.h/cpp`
Protects: Sprint 0.6, 3.3, 3.5, future Sprint 5 (thread safety)

## Files to create/modify

| File | Action |
|------|--------|
| `src/port/controlport.h:62` | Add `virtual` to `communicate()` |
| `CMakeLists.txt` | Add Qt5::Test, enable_testing(), 7 test targets |
| `tests/test_constants.cpp` | New |
| `tests/test_imageproc.cpp` | New |
| `tests/test_config.cpp` | New |
| `tests/test_imageio.cpp` | New |
| `tests/test_ptz.cpp` | New |
| `tests/test_lens.cpp` | New |
| `tests/test_tcu.cpp` | New |

## Summary

| Suite | Cases | Tier | Safety net for |
|-------|-------|------|----------------|
| test_constants | 5 | 1 | Sprint 3, all future |
| test_imageproc | 12 | 1 | Sprint 8 |
| test_config | 9 | 1 | Sprint 7 |
| test_imageio | 7 | 1 | Sprint 8 |
| test_ptz | 14 | 2 | Sprints 0, 1, 3, 6 |
| test_lens | 10 | 2 | Sprint 3, 4 |
| test_tcu | 13 | 2 | Sprints 0, 3, 5 |
| **Total** | **70** | | |

## Implementation order

1. test_constants — simplest, no linking complexity
2. test_config — pure data, validates JSON persistence
3. test_ptz — protocol encoding, needs virtual communicate
4. test_lens — same pattern as PTZ
5. test_tcu — most complex state machine
6. test_imageproc — needs OpenCV linking
7. test_imageio — needs temp file management

## Verification

```bash
cmake --build build
cd build && ctest --output-on-failure
```

All 70 tests pass -> safe to proceed to Sprint 4 (Copy-Paste Elimination).
