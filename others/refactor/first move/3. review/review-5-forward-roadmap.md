# Phase 5 — Forward Roadmap

> Based on findings from Phases 1–4 | 2026-03-16

---

## Sprint Sequencing Rationale

```
Sprint 10 ── Thread Safety (CRITICAL)     ← crashes/corruption in production
Sprint 11 ── Thread Safety (HIGH) + Tests  ← UB elimination + regression guards
Sprint 12 ── Test Expansion                ← coverage for controllers + protocols
Sprint 13 ── Pipeline Extraction           ← largest remaining structural debt
Sprint 14 ── Controller UI Decoupling      ← enables unit testing of controllers
```

**Why this order**: CRITICALs first (Sprint 10) because they cause observable crashes. HIGHs next (Sprint 11) bundled with tests because the fixes are small and tests lock them in. Test expansion (Sprint 12) before pipeline extraction (Sprint 13) because tests create a safety net for the extraction. Controller decoupling (Sprint 14) last because it's the largest effort with the least crash risk.

---

## Sprint 10: Thread Safety — Critical Fixes

**Rationale:** Phase 3 identified 4 CRITICAL data races (C1–C4) with observable crash/corruption risk. `list_roi` heap corruption (C1) and `vid_out` VideoWriter race (C3) can crash in production. `window()->grab()` (C4) is a QWidget call from a worker thread.

**Scope:**

- Task 10.1: **Protect `list_roi` + `user_mask`** — Add a `QMutex roi_mutex`. Lock in mousePress/Release handlers (lines 238-245) and in `frame_average_and_3d` (line 1132). Copy `list_roi.size()` under lock, use the copy outside.
- Task 10.2: **Protect `vid_out[]` VideoWriters** — Add `QMutex video_mutex[4]`. Lock around `vid_out[].write()` in `record_frame` (lines 1450-1470) and around `vid_out[].open()`/`.release()` in the GUI button handlers (lines 2203, 2216, 4362, 4374, 4572, 4582).
- Task 10.3: **Replace `window()->grab()` with signal** — In `advance_scan` (line 1379), emit a new `request_screenshot(QString path)` signal. Connect with `Qt::QueuedConnection` to a GUI-thread slot that calls `window()->grab().save(path)`.
- Task 10.4: **Fix `grab_thread_state` spin-wait** — Change `bool grab_thread_state[3]` to `std::atomic<bool>`. Replace the bare spin-wait at line 4307 (`while (grab_thread_state[...]) ;`) with a loop that includes `QThread::yieldCurrentThread()` or `QThread::msleep(1)`.

**Prerequisites:** None (independent of other sprints)

**Risk level:** Medium — mutex additions can introduce deadlocks if scoped incorrectly; keep locks narrow

**Estimated effort:** S

**Success criteria:**
- No `list_roi`/`user_mask`/`vid_out` access exists outside mutex protection
- `window()->grab()` never called from non-GUI thread
- `grab_thread_state` is `std::atomic<bool>` with no bare spin-wait
- Application builds and runs without deadlock under normal scan + record workflow

---

## Sprint 11: Thread Safety — HIGH Fixes + Snapshot Expansion

**Rationale:** Phase 3 identified 7 HIGH issues (H1–H7) — undefined behavior per C++ standard on x86, with mid-frame inconsistency risk. Most fixes are adding fields to the existing ProcessingParams snapshot or converting to `std::atomic`.

**Scope:**

- Task 11.1: **Add display config to ProcessingParams** — Add `image_rotate`, `pseudocolor[3]`, `image_3d[3]` to `ProcessingParams`. Update `update_processing_params()` to populate them. Replace all bare reads in pipeline methods with `params.*`.
- Task 11.2: **Atomicize save/record flags** — Change `save_original[3]`, `save_modified[3]`, `record_original[3]`, `record_modified[3]` to `std::atomic<bool>`. Update all read/write sites to use `.load()`/`.store()`.
- Task 11.3: **Snapshot Display selection rect** — Add `selection_v1`, `selection_v2`, `display_region` to ProcessingParams (or a small struct). Snapshot at loop start, use in `render_and_display` instead of reading Display widget members.
- Task 11.4: **Add device properties to ProcessingParams** (LOW items, opportunistic) — Add `is_color[3]`, `pixel_depth[3]`, `w[3]`, `h[3]`, `device_type` to ProcessingParams. These are stable during grab, but adding them eliminates the last formal UB in the pipeline. Skip if it bloats the snapshot struct excessively.
- Task 11.5: **Write test_processing_params.cpp** — Verify all ProcessingParams and PipelineConfig fields are populated by `update_processing_params()`. This test catches regressions when new params are added.

**Prerequisites:** Sprint 10 (C1-C4 must be fixed first)

**Risk level:** Low — additive changes, snapshot pattern already proven

**Estimated effort:** M

**Success criteria:**
- Zero bare member variable reads from pipeline methods (all go through params/pcfg snapshots or atomics)
- test_processing_params passes
- No visual regressions (rotation, pseudocolor, 3D mode, save/record all work)

---

## Sprint 12: Test Expansion

**Rationale:** Phase 4 found 25+ untested modules. The existing test infrastructure (QTest + CTest + stub pattern) makes expansion straightforward. This sprint adds tests for the highest-risk untested modules before the pipeline extraction in Sprint 13.

**Scope:**

- Task 12.1: **test_laser.cpp** — Clone test_ptz pattern. Validate on/off/fire command frames, preset parameter encoding, checksum. Tier 2 (stubbed communicate). ~10 cases.
- Task 12.2: **test_rangefinder.cpp** — Stub communicate, validate distance parsing for supported RF types. Tier 2. ~5 cases.
- Task 12.3: **test_scan_controller.cpp** — Test state transitions (start/stop/continue), index advancement, route boundary conditions. Test concurrent get/set from two threads. Tier 2 (needs stubbed TCUController + DeviceManager). ~10 cases.
- Task 12.4: **test_tcu_controller.cpp** — Test set_delay_dist clamping, dist_ns calculation, toggle_frame_a_3d atomicity, config reads. Tier 2 (real Config with load_defaults() + stubbed TCU). ~8 cases.
- Task 12.5: **Extend test_config.cpp** — Add 4 cases for ImageProcSettings and SaveSettings field-level mutation round-trips. Tier 1. Catches Sprint 7 regressions.
- Task 12.6: **CI setup** — GitHub Actions workflow: Windows runner, MSVC build, ctest run on push to master/refactor branches.

**Prerequisites:** Sprint 11 (snapshot expansion must be testable)

**Risk level:** Low — tests don't modify production code

**Estimated effort:** M

**Success criteria:**
- All new tests pass in CI
- Laser, RangeFinder, ScanController, TCUController, Config extensions all green
- CI runs automatically on push

---

## Sprint 13: Pipeline Extraction

**Rationale:** Phase 2 identified the pipeline methods (788 lines) as the ripest extraction target from UserPanel. After Sprint 11 eliminates all bare member variable reads, pipeline methods depend only on snapshot structs + image buffers — making extraction feasible.

**Scope:**

- Task 13.1: **Create `PipelineProcessor` class** in `src/pipeline/pipelineprocessor.h/cpp`. Move all 11 pipeline methods: `acquire_frame`, `preprocess_frame`, `detect_yolo`, `frame_average_and_3d`, `detect_fishnet`, `enhance_frame`, `render_and_display`, `advance_scan`, `record_frame`, `save_to_file`, `save_scan_img`.
- Task 13.2: **Define `PipelineProcessor` interface** — Constructor takes pointers to shared resources (image_mutex, img_mem, modified_result, YoloDetector, ThreadPool). Pipeline methods take `ProcessingParams` + `PipelineConfig` by const ref (already the pattern).
- Task 13.3: **Move `FrameAverageState` and `ECCState`** from userpanel.h to pipelineprocessor.h. These are pipeline-internal state.
- Task 13.4: **Move `grab_thread_process()`** — The 99-line main loop becomes `PipelineProcessor::run_frame()` or similar. GrabThread calls this instead of `UserPanel::grab_thread_process()`.
- Task 13.5: **Rewire signals** — Pipeline signals (set_pixmap, set_hist_pixmap, packet_lost_updated, finish_scan_signal, etc.) now emitted by PipelineProcessor. UserPanel connects them in its constructor.
- Task 13.6: **Write test_pipeline_stages.cpp** — Feed synthetic cv::Mat through individual pipeline methods with known ProcessingParams. Validate output dimensions, channel count, format.

**Prerequisites:** Sprint 11 (all pipeline reads must go through snapshots), Sprint 12 (safety net tests in place)

**Risk level:** High — largest code move since Sprint 9. Many signal rewirings. Regression risk in display, recording, scanning.

**Estimated effort:** L

**Success criteria:**
- UserPanel no longer contains any pipeline method
- `userpanel.cpp` drops to ~4000 lines
- All existing functionality works: live view, recording, scanning, YOLO, 3D, pseudocolor
- test_pipeline_stages passes
- All Sprint 12 tests still pass

---

## Sprint 14: Controller UI Decoupling

**Rationale:** Phase 2 found that 5/7 controllers hold `Ui::UserPanel*` and directly manipulate widgets (layering violations L1–L4). This blocks unit testing of controllers and creates fragile coupling. This sprint introduces a signal-based boundary between controllers and UI.

**Scope:**

- Task 14.1: **DeviceManager signal-based UI updates** — Replace direct `m_ui->` widget manipulation with signals: `port_status_changed(int port_id, bool connected)`, `port_label_updated(int port_id, QString text)`. UserPanel slots handle the widget updates. Remove `Ui::UserPanel*` from DeviceManager.
- Task 14.2: **TCUController signal-based UI updates** — Replace direct slider/label manipulation with signals: `delay_updated(float)`, `gate_width_updated(float)`, `tcu_type_changed(int)`. UserPanel handles widget layout changes. Remove `Ui::UserPanel*` from TCUController.
- Task 14.3: **LensController signal-based UI updates** — Replace button text changes with signals: `zoom_state_changed(bool active)`, `focus_state_changed(bool active)`. Remove `Ui::UserPanel*`.
- Task 14.4: **LaserController refactor** — Replace `m_pref->ui->LASER_ENABLE_CHK->click()` with a signal `laser_enable_requested(bool)`. UserPanel or Preferences handles the click. Remove both `Ui::UserPanel*` and `Preferences*` from LaserController.
- Task 14.5: **Write controller unit tests** — With UI decoupled, controllers can be tested without a running GUI. Add test_device_manager.cpp, extend test_tcu_controller.cpp, test_laser_controller.cpp.

**Prerequisites:** Sprint 13 (pipeline extraction reduces UserPanel complexity, making rewiring easier)

**Risk level:** High — large surface area of signal rewiring across all controllers

**Estimated effort:** XL

**Success criteria:**
- No controller holds `Ui::UserPanel*` or `Preferences*`
- All controller ↔ UI communication is via signals/slots
- Controller unit tests pass without QApplication (headless)
- All existing functionality works

---

## Summary

| Sprint | Theme | Risk | Effort | Outcome |
|--------|-------|------|--------|---------|
| 10 | Thread Safety CRITICAL | Medium | S | 4 crash-risk races eliminated |
| 11 | Thread Safety HIGH + Snapshot | Low | M | Zero bare cross-thread reads in pipeline |
| 12 | Test Expansion | Low | M | +37 test cases, CI pipeline |
| 13 | Pipeline Extraction | High | L | PipelineProcessor class, UserPanel −788 lines |
| 14 | Controller UI Decoupling | High | XL | Controllers testable headless, no Ui:: dependency |