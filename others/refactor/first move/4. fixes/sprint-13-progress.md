# Sprint 13 Progress: Pipeline Extraction

## Prerequisite: Sprints 11-12
Status: SATISFIED

Sprint 11 eliminated all bare cross-thread reads in pipeline methods (all go through ProcessingParams/PipelineConfig snapshots or atomics). Sprint 12 added test coverage.

## Task 13.1: Create PipelineProcessor class
Status: DONE

**Changes:**
- Created `src/pipeline/pipelineprocessor.h` with SharedState struct, all pipeline method declarations, and signal declarations
- Created `src/pipeline/pipelineprocessor.cpp` with all 12 pipeline method bodies moved from userpanel.cpp
- Methods moved: run (was grab_thread_process), acquire_frame, preprocess_frame, detect_yolo, frame_average_and_3d, enhance_frame, render_and_display, advance_scan, detect_fishnet, record_frame, save_to_file, save_scan_img, draw_yolo_boxes
- All direct member access converted to SharedState pointer access (m_s.*)

## Task 13.2: Define PipelineProcessor interface
Status: DONE

**Changes:**
- SharedState struct holds pointers to all shared resources: per-thread arrays, atomic flags, queues, mutexes, snapshots, controllers, displays
- device_type and frame_rate_edit stored as pointers (not value copies) so changes after construction are visible to pipeline
- save_to_file signature extended with `const ProcessingParams& params` to replace `pref->split` cross-thread read with `params.split`
- Added task_queue_full signal (was on UserPanel)

## Task 13.3: Move FrameAverageState and ECCState
Status: DONE

**Changes:**
- Moved both structs from userpanel.h to pipelineprocessor.h
- userpanel.h now includes pipelineprocessor.h

## Task 13.4: Move grab_thread_process
Status: DONE

**Changes:**
- Main loop becomes PipelineProcessor::run(int* idx)
- GrabThread holds PipelineProcessor* instead of UserPanel*
- GrabThread::run() calls m_pipeline->run(&_display_idx)
- Removed grab_thread_process declaration from UserPanel

## Task 13.5: Rewire signals
Status: DONE

**Changes:**
- PipelineProcessor signals: packet_lost_updated, update_mcp_in_thread, set_hist_pixmap, finish_scan_signal, save_screenshot_signal, update_delay_in_thread, update_dist_mat, task_queue_full, set_model_list_enabled (LVTONG), update_fishnet_result (LVTONG)
- UserPanel constructor creates PipelineProcessor with SharedState and connects all signals
- Removed 7 old signal connections from UserPanel (packet_lost_updated, update_delay_in_thread, update_mcp_in_thread, save_screenshot_signal, set_model_list_enabled, task_queue_full, set_hist_pixmap, finish_scan_signal)
- Removed corresponding signal declarations from UserPanel (kept set_src_pixmap, update_scan, video_stopped, etc. that are still used)

## Task 13.6: Extract TimedQueue
Status: DONE

**Changes:**
- Extracted TimedQueue class from userpanel.h into new `src/util/timedqueue.h`
- pipelineprocessor.h includes timedqueue.h instead of forward-declaring TimedQueue
- userpanel.h includes timedqueue.h instead of defining TimedQueue inline

## Task 13.7: Update CMakeLists.txt
Status: DONE

**Changes:**
- Added src/pipeline/pipelineprocessor.h, src/pipeline/pipelineprocessor.cpp, src/util/timedqueue.h
