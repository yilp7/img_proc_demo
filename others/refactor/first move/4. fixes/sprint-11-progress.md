# Sprint 11 Progress: Thread Safety — HIGH Fixes + Snapshot Expansion

## Prerequisite: Sprint 10
Status: SATISFIED

Sprint 10 fixed all 4 CRITICAL data races (C1-C4). Sprint 11 addresses HIGH-severity issues (H1-H7).

## Task 11.1: Add display config to ProcessingParams
Status: TODO

**Problem (H1-H3):** `image_rotate[0]`, `pseudocolor[i]`, `image_3d[i]` are toggled by user on GUI thread while pipeline reads them. Mid-frame inconsistency possible.

**Pipeline read sites:**
- `image_rotate[0]`: line 867 (preprocess_frame)
- `pseudocolor[i]`: lines 1306, 1362 (render_and_display), 1476 (record_frame)
- `image_3d[i]`: lines 1351 (render_and_display), 1464 (record_frame)

**Plan:**
- Add `int image_rotate`, `bool pseudocolor`, `bool image_3d` to ProcessingParams
- Populate in update_processing_params() — indexed by thread_idx 0 (main display)
- Replace bare reads in pipeline methods with params.*
- Connect image_rotate/pseudocolor/image_3d change signals to update_processing_params()

Files: `src/pipeline/processingparams.h`, `src/visual/userpanel.cpp`

## Task 11.2: Atomicize save/record flags
Status: TODO

**Problem (H4-H5):** `save_original[3]`, `save_modified[3]`, `record_original[3]`, `record_modified[3]` are set by GUI button clicks and read/cleared by the worker thread. Without atomics, compiler may reorder or optimize.

**Pipeline read/write sites:**
- `save_original[i]`: line 1443 (read), 1445 (clear) in record_frame
- `save_modified[i]`: line 1447 (read), 1449 (clear) in record_frame
- `record_original[i]`: line 1451 (read) in record_frame
- `record_modified[i]`: line 1462 (read) in record_frame

**Plan:**
- Change all four to `std::atomic<bool>[3]`
- Worker thread uses `.load()` for reads and `.store(false)` for clears
- GUI thread uses `.store(true)` for sets

Files: `src/visual/userpanel.h`, `src/visual/userpanel.cpp`

## Task 11.3: Snapshot Display selection rect
Status: TODO

**Problem (H7):** `displays[...]->selection_v1`, `selection_v2`, `display_region` read from worker thread in render_and_display (lines 1326, 1355-1356) while mouse events modify them on GUI thread.

**Plan:**
- Add `cv::Point selection_v1`, `selection_v2` and `cv::Rect display_region` to ProcessingParams
- Populate in update_processing_params() from displays[0]
- Replace bare reads in render_and_display with params.*

Files: `src/pipeline/processingparams.h`, `src/visual/userpanel.cpp`

## Task 11.4: Add device properties to ProcessingParams (LOW, opportunistic)
Status: TODO

**Problem (L1-L4):** `is_color[i]`, `pixel_depth[i]`, `w[i]`, `h[i]`, `device_type` are stable during grab (set at camera connect), but formally UB per C++ standard.

**Plan:**
- Skip — these are set once before grab starts, with a happens-before relationship (grab_image[i] = false stops the thread before any change). Adding them would bloat ProcessingParams substantially (~15 read sites) for negligible safety gain on x86. Defer to Sprint 13 (pipeline extraction) where they become constructor parameters.

## Task 11.5: Write test_processing_params.cpp
Status: TODO

**Plan:**
- Verify all ProcessingParams and PipelineConfig fields are populated by update_processing_params()
- Test that snapshot values match source widget/config values

Files: `tests/test_processing_params.cpp`, `CMakeLists.txt`
