# Sprint 10 Progress: Thread Safety — Critical Fixes

## Prerequisite: Sprints 0-9 + Post-Sprint Review
Status: SATISFIED

Post-sprint review (review-3-thread-safety.md) identified 4 CRITICAL data races (C1–C4). Sprint 10 addresses all four.

## Pre-Sprint Bug Fix (committed separately)

Before sprint 10 began, a scan PTZ transmission bug was discovered and fixed:
- IPTZController methods (ptz_set_angle, ptz_move, ptz_stop) in UDPPTZ/USBCAN called device_control()/transmit_data() directly from grab thread and main thread; QUdpSocket::writeDatagram() from wrong thread silently failed
- Fix: all IPTZController methods now use emit control()/transmit() queued signals
- Also fixed: UDPPTZ ptz_stop MANUAL_SEARCH → STANDBY for 530; DeviceManager self-check transmit_data() → transmit() signal
- This fix also completed task 10.3 (window()->grab() → save_screenshot_signal)

## Task 10.1: Protect list_roi + user_mask
Status: DONE

**Problem (C1):** `list_roi` is a `std::vector<cv::Rect>` modified by `push_back()` on the GUI thread (line 238, add_roi lambda) and read by `.size()` on the worker thread (line 1135, frame_average_and_3d). A reallocation during push_back while the worker reads is heap corruption. `user_mask[0]` is a `cv::Mat` written by `setTo()`/assignment on the GUI thread (lines 239, 245) and read by `copyTo()` on the worker thread (line 1135).

**Changes:**
- Added `QMutex m_roi_mutex` to UserPanel (protects both list_roi and user_mask)
- Locked in add_roi lambda (lines 235-240): `QMutexLocker lk(&m_roi_mutex)` before push_back and setTo
- Locked in clear_roi lambda (lines 241-246): `QMutexLocker lk(&m_roi_mutex)` before swap and assignment
- Locked in frame_average_and_3d (line 1135): narrow lock scope around `list_roi.size()` check and `copyTo(masked_dist, user_mask)`; emit update_dist_mat after lock released
- Distance3DView reads list_roi via pointer (lines 165, 168) but runs on GUI thread (connected via QueuedConnection) — same thread as modifications, no race

Files: `src/visual/userpanel.h`, `src/visual/userpanel.cpp`

## Task 10.2: Protect vid_out[] VideoWriters
Status: DONE (already protected)

**Finding:** All vid_out[] accesses are already properly mutexed via `image_mutex[]`:
- Worker thread writes (record_frame lines 1453-1473): inside `QMutexLocker processing_locker(&image_mutex[thread_idx])` at line 1521
- GUI open/release (lines 2206, 2219, 4365, 4377, 4575, 4585): each inside `QMutexLocker locker(&image_mutex[...])` at corresponding lines

No additional work needed.

## Task 10.3: Replace window()->grab() with signal
Status: DONE (in pre-sprint bug fix)

- Added `save_screenshot_signal(QString path)` signal to UserPanel
- Connected via QueuedConnection to lambda calling `window()->grab().save(path)`
- Replaced direct `window()->grab().save(...)` call in advance_scan (line 1379) with `emit save_screenshot_signal(...)`

Files: `src/visual/userpanel.h`, `src/visual/userpanel.cpp`

## Task 10.4: Fix grab_thread_state spin-wait
Status: DONE

**Problem (C4 from busy-wait):** `bool grab_thread_state[3]` is written by the grab thread (lines 1479, 1573) and read in a bare spin-wait on the GUI thread (line 4310: `while (grab_thread_state[select_display_thread]) ;`). Without atomic/volatile, the compiler may optimize the loop into an infinite loop. Also burns CPU.

**Changes:**
- Changed `bool grab_thread_state[3]` to `std::atomic<bool> grab_thread_state[3]` in userpanel.h
- Replaced bare spin-wait `while (grab_thread_state[...]) ;` with `while (grab_thread_state[...].load()) QThread::msleep(1);` — yields CPU and ensures atomic read
- Writes at lines 1485, 1579 and read at line 4559 work as-is via std::atomic's implicit conversion operators

Files: `src/visual/userpanel.h`, `src/visual/userpanel.cpp`
