# Phase 4 — Test Gap Analysis

> Post-Sprint 9 + Thread Safety Fixes | Reviewed 2026-03-16

---

## Table of Contents

1. [Existing Test Inventory](#1-existing-test-inventory)
2. [Coverage Map](#2-coverage-map)
3. [Critical Untested Paths](#3-critical-untested-paths)
4. [Test Infrastructure Assessment](#4-test-infrastructure-assessment)
5. [Recommended Test Additions](#5-recommended-test-additions)

---

## 1. Existing Test Inventory

**7 test files** in `tests/`, using Qt Test (QTest) framework, integrated with CTest.

### Tier 1 — Pure Function Tests (no hardware dependencies)

| Test File | Cases | Module Tested | What's Covered |
|-----------|------:|---------------|----------------|
| `test_constants.cpp` | 5 | constants.h, pelco_protocol.h, tcu_protocol.h | Pelco constants, angle encoding, TCU protocol, light speed, pipeline constants |
| `test_config.cpp` | 9 | Config (util/config.h) | JSON round-trip, defaults, partial load, invalid JSON, ps_step array, YOLO paths, version |
| `test_imageproc.cpp` | 12 | ImageProc (image/imageproc.h) | hist_eq, laplace, plateau_equl, brightness/contrast, split_img, gated3D, gated3D_v2, accumulative, adaptive, haze_removal |
| `test_imageio.cpp` | 7 | ImageIO (image/imageio.h) | BMP 8/24-bit round-trip, TIF 8/16-bit, JPG lossy, metadata NOTE marker, row padding |

### Tier 2 — Protocol Tests (stubbed communicate())

| Test File | Cases | Module Tested | What's Covered |
|-----------|------:|---------------|----------------|
| `test_ptz.cpp` | 14 | PTZ (port/ptz.h) | STOP/direction frames, angle encoding/clamping, speed clamp, address propagation, checksum, angle feedback parsing |
| `test_lens.cpp` | 10 | Lens (port/lens.h) | STOP/zoom/focus/radius frames, set-param encoding, relay commands, stepping clamp, query frames, checksum |
| `test_tcu.cpp` | 13 | TCU (port/tcu.h) | Defaults, type 0/1/2 parameter remapping, frame encoding (rep_freq, delay_a), distance formula, AB lock, auto-PRF |

**Total: 70 test cases across 7 test files.**

---

## 2. Coverage Map

| Module | Test Suite | Cases | Coverage Assessment |
|--------|-----------|------:|---------------------|
| **Constants/Protocols** | test_constants | 5 | ✅ All protocol constants validated |
| **Config** | test_config | 9 | ✅ JSON persistence, defaults, error handling. ⚠️ ImageProcSettings field mutations only at high level |
| **ImageProc** | test_imageproc | 12 | ✅ All 12 enhancement algorithms covered |
| **ImageIO** | test_imageio | 7 | ✅ BMP/TIF/JPG, metadata, row padding |
| **PTZ (Pelco-D)** | test_ptz | 14 | ✅ Protocol encoding, angle logic. ❌ USBCAN, UDPPTZ implementations |
| **Lens (Pelco)** | test_lens | 10 | ✅ Protocol frames, parameter encoding |
| **TCU** | test_tcu | 13 | ✅ Type 0/1/2 remapping, frame encoding, distance formula |
| **Laser** | — | 0 | ❌ No tests. Serial protocol, preset system, fire/on/off untested |
| **RangeFinder** | — | 0 | ❌ No tests. Distance parsing, Huanyu protocol untested |
| **ControlPort** | — | 0 | ❌ Tested indirectly via PTZ/Lens/TCU stubs, but base class logic (read/write/reconnect) not directly tested |
| **USBCAN** | — | 0 | ❌ CAN bus protocol untested |
| **UDPPTZ** | — | 0 | ❌ UDP PTZ protocol untested |
| **IPTZController** | — | 0 | ❌ Interface contract not explicitly tested |
| **DeviceManager** | — | 0 | ❌ Port lifecycle, signal wiring, thread setup |
| **TCUController** | — | 0 | ❌ Parameter sync, atomic operations, config reads |
| **LensController** | — | 0 | ❌ Button handlers, parameter updates |
| **LaserController** | — | 0 | ❌ Laser on/off, preset system, cross-controller calls |
| **RFController** | — | 0 | ❌ Distance display update |
| **ScanController** | — | 0 | ❌ Scan state machine, mutex-protected getters/setters, route management |
| **AuxPanelManager** | — | 0 | ❌ Display option handlers |
| **ProcessingParams** | — | 0 | ❌ Snapshot struct field correctness not validated |
| **PipelineConfig** | — | 0 | ❌ Config snapshot completeness not validated |
| **Pipeline methods** | — | 0 | ❌ acquire_frame through record_frame: no integration tests |
| **GrabThread** | — | 0 | ❌ Main loop lifecycle untested |
| **ThreadPool** | — | 0 | ❌ Task dispatch, queue overflow |
| **Cam hierarchy** | — | 0 | ❌ Requires hardware; abstract interface testable with mock |
| **YoloDetector** | — | 0 | ❌ Model load, inference, class parsing |
| **UserPanel** | — | 0 | ❌ Signal routing, controller initialization |
| **Preferences** | — | 0 | ❌ Widget ↔ Config sync, data_exchange |
| **Display/widgets** | — | 0 | ❌ All widget classes untested |

**Summary: 7 modules tested, ~25 modules untested.**

---

## 3. Critical Untested Paths

Ranked by (regression likelihood × failure severity × test feasibility):

| Rank | What | Risk | Feasibility | Rationale |
|-----:|------|------|-------------|-----------|
| 1 | **ScanController state machine** | Scan mis-steps corrupt PTZ routes, lose scan data | Unit test (S) | Mutex-protected state, no UI dep needed for logic test. Scan bugs are silent and destructive. |
| 2 | **ProcessingParams + PipelineConfig snapshot completeness** | Missing field → stale data in pipeline → wrong output | Unit test (S) | Compare struct fields against all update_processing_params() writes. Catches regressions when new params added. |
| 3 | **TCUController parameter logic** | Wrong delay/gate/laser → incorrect imaging or hardware damage | Unit test (M) | Atomic operations, config reads, dist_ns calculations testable without hardware. TCU controls safety-relevant timing. |
| 4 | **Laser protocol** | Laser fire/on/off commands → safety-critical | Unit test (S) | Same stub pattern as test_ptz/lens/tcu. Laser commands affect eye safety. |
| 5 | **LaserController preset system** | Wrong preset → unintended laser fire + lens move | Unit test (M) | Cross-controller calls (LensController, TCUController) need stubs. Multi-device coordination. |
| 6 | **Config ImageProcSettings field mutations** | Add/remove field → JSON persistence breaks silently | Unit test (S) | Extend test_config with specific field mutation round-trips. Regression guard for Sprint 7 structs. |
| 7 | **Pipeline integration (preprocess → enhance → render)** | Frame format mismatch between stages → crash or garbage output | Integration test (L) | Feed synthetic cv::Mat through pipeline methods with known params. No hardware needed. |
| 8 | **USBCAN / UDPPTZ IPTZController conformance** | Interface contract violated → PTZ commands silently fail | Unit test (M) | Test against IPTZController interface contract. Stubs needed for CAN/UDP. |
| 9 | **DeviceManager port lifecycle** | Thread start/stop ordering → crash on connect/disconnect | Integration test (L) | Needs mock ControlPort. Tests moveToThread, signal wiring, shutdown sequence. |
| 10 | **ThreadPool queue overflow** | task_queue_full signal timing → image save silently dropped | Unit test (S) | Test append_task when pool is full. Currently no thread safety on append_task itself. |

---

## 4. Test Infrastructure Assessment

### Strengths

- **Framework**: Qt Test (QTest) is well-suited for this project — native Qt integration, signal spy capability, QTemporaryDir for file tests
- **Tiered design**: Tier 1 (pure) and Tier 2 (stubbed I/O) are cleanly separated in CMakeLists.txt
- **Stubbing pattern established**: TestPTZ/TestLens/TestTCU override `communicate()` to capture frames — proven pattern, easily extended to Laser and RangeFinder
- **CTest integration**: `enable_testing()` + `add_test()` in CMakeLists.txt — standard CMake test runner works
- **Low boilerplate**: Adding a new test requires ~15 lines of setup (class, slots, QTEST_MAIN)

### Gaps

| Gap | Impact | Effort to Fix |
|-----|--------|---------------|
| **No CI/CD pipeline** | Tests only run when developer remembers | M — GitHub Actions workflow |
| **No mock Config** | Can't test controllers without real Config object | S — Config has `load_defaults()`, usable as-is |
| **No mock ControlPort** | Can't test DeviceManager without real ports | M — Need base class stub |
| **No signal spy usage** | Can't verify signal emissions in tests | S — QSignalSpy is built into QTest |
| **No pipeline test harness** | Can't feed synthetic frames through pipeline | L — Need to extract pipeline from UserPanel first |
| **No coverage reporting** | Don't know what's actually exercised | S — gcov/lcov integration |

### What's Easy to Test Now (no infrastructure changes needed)

Using the existing stubbed-communicate pattern:
- **Laser protocol** (same pattern as PTZ/Lens/TCU)
- **RangeFinder protocol** (same pattern)
- **ScanController** (pure state logic, constructor-injectable deps)
- **ProcessingParams/PipelineConfig** (plain structs, field-by-field verification)
- **Config ImageProcSettings** (extend existing test_config.cpp)
- **TCUController** (needs Config + stubbed TCU, both available)

---

## 5. Recommended Test Additions

### Priority 1 — Quick Wins (extend existing patterns)

#### test_laser.cpp
- **Scope**: Laser serial protocol frame generation
- **Validates**: on/off/fire command frames, preset parameter encoding, checksum
- **Type**: Unit test (Tier 2, stubbed communicate)
- **Complexity**: S
- **Pattern**: Clone test_ptz.cpp structure with TestLaser : Laser override

#### test_config (extend)
- **Scope**: Add 3-4 cases for ImageProcSettings and SaveSettings field mutations
- **Validates**: Each new Sprint 7 struct field round-trips correctly through JSON
- **Type**: Unit test (Tier 1)
- **Complexity**: S

#### test_processing_params.cpp
- **Scope**: Verify ProcessingParams and PipelineConfig struct fields match update_processing_params() writes
- **Validates**: No field is forgotten when adding new params; default values are sensible
- **Type**: Unit test (Tier 1)
- **Complexity**: S

### Priority 2 — Controller Tests

#### test_scan_controller.cpp
- **Scope**: ScanController state transitions, mutex safety, route indexing
- **Validates**: Start/stop/continue scan, index advancement, route boundary conditions, concurrent get/set
- **Type**: Unit test (Tier 2)
- **Complexity**: M
- **Notes**: Needs stubbed TCUController and DeviceManager; test concurrent access from two threads

#### test_tcu_controller.cpp
- **Scope**: TCUController parameter calculations, atomic operations
- **Validates**: set_delay_dist range clamping, dist_ns calculation, toggle_frame_a_3d atomicity, config reads
- **Type**: Unit test (Tier 2)
- **Complexity**: M
- **Notes**: Needs real Config (use load_defaults()) + stubbed TCU

#### test_laser_controller.cpp
- **Scope**: LaserController preset system, cross-controller coordination
- **Validates**: Laser preset → TCU delay + lens position + laser fire sequence
- **Type**: Unit test (Tier 2)
- **Complexity**: M
- **Notes**: Needs stubbed DeviceManager, LensController, TCUController

### Priority 3 — Integration Tests

#### test_pipeline_stages.cpp
- **Scope**: Feed synthetic cv::Mat through individual pipeline methods
- **Validates**: preprocess_frame rotation/flip, enhance_frame algorithm selection, render_and_display output format
- **Type**: Integration test (Tier 3)
- **Complexity**: L
- **Notes**: Pipeline methods are currently UserPanel members; need to either friend-class the test or extract pipeline to standalone class first. Easier after pipeline extraction sprint.

#### test_device_manager_lifecycle.cpp
- **Scope**: DeviceManager port thread start/stop, signal wiring
- **Validates**: moveToThread ordering, port status callbacks, shutdown without crash
- **Type**: Integration test (Tier 3)
- **Complexity**: L
- **Notes**: Needs mock ControlPort subclass

### Priority 4 — Infrastructure

#### CI/CD pipeline
- **Scope**: GitHub Actions workflow to build + run ctest on push/PR
- **Validates**: Tests pass on clean build
- **Type**: Infrastructure
- **Complexity**: M
- **Notes**: Windows runner needed (MSVC + Qt + OpenCV deps); consider caching vcpkg/conan packages

#### Coverage reporting
- **Scope**: gcov/lcov integration in CMake, coverage report generation
- **Validates**: Quantifies actual line/branch coverage
- **Type**: Infrastructure
- **Complexity**: S (MSVC uses OpenCppCoverage instead of gcov)
