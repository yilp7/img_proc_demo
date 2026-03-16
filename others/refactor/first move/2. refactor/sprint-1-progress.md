# Sprint 1 Progress: Dead Code Removal

## Task 1.1: Delete all `#if 0` blocks
Status: DONE

10 blocks removed:
- userpanel.cpp:82-87 — old FPS calculation alternative
- userpanel.cpp:2886-2898 — old com_write_data using ControlPortThread
- userpanel.cpp:6122-6146 — old display radio button handlers (on_COM_DATA_RADIO_clicked, on_PTZ_RADIO_clicked, on_PLUGIN_RADIO_clicked)
- userpanel.cpp:6277-6296 — old ptz_button_pressed using send_ctrl_cmd
- userpanel.cpp:6512-6532 — old on_GET_ANGLE_BTN_clicked using ptr_ptz
- userpanel.cpp:6541-6572 — old set_ptz_angle using ptr_ptz
- userpanel.h:301-306 — old display radio button slot declarations
- lasercontrol.cpp:125-172 — old adjustment_btn_pressed/released using raw hex commands
- lens.cpp:158-173 — old STEPPING implementation using 9-byte command
- imageproc.cpp:634-643 — old histogram calculation using cv::calcHist (also removed surrounding `#if 1` wrapper)

## Task 1.2: Delete commented-out code blocks
Status: DONE

- userpanel.h:537-548 — removed commented-out TCUThread/InclinThread/LensThread/LaserThread/PTZThread declarations; fixed typo "thriugh" -> "through" in remaining comment

## Task 1.3: Remove ENABLE_USER_DEFAULT code path
Status: DONE

Removed `#define ENABLE_USER_DEFAULT 0` from util.h. For blocks with `#else` branches, kept the `#else` code (JSON config path). Blocks without `#else` were removed entirely.

Files modified:
- util.h — removed define
- main.cpp — removed user_default file init block (no #else); kept theme loading #else branch
- userpanel.cpp — kept #else branches for language loading, COM port loading, theme saving, language saving, COM port saving
- controlport.cpp — removed user_default COM port saving block; kept #else comment
- controlportthread.cpp — removed user_default COM port saving block; kept #else comment
- preferences.cpp — removed TCP server IP saving lambda, 3 offset saving blocks inside lambdas, and user_default loading block (all had no #else)

## Task 1.4: Remove disabled AutoScan references
Status: DONE

Removed all commented-out AutoScan integration code from:
- main.cpp — removed include comment, AutoScan object creation block, signal connections, command line args
- userpanel.h — removed include comment, method declarations, signal, member variables
- userpanel.cpp — removed initializer list entry, initialization_complete emit, set_command_line_args/set_auto_scan_controller method bodies

The automation/ directory (autoscan.h, autoscan.cpp, scanpreset.cpp) is preserved as standalone classes.

## Task 1.5: Remove goto labels in ptz_control()
Status: DONE

ptz.cpp ptz_control(): replaced `goto chksm;` with `break;` in all 10 direction cases. Removed `chksm:` and `send:` labels. Control flow is identical because `chksm:` was immediately after the switch closing brace, so `break` exits to the same position. `send:` was never targeted by any goto.

## Task 1.6: Fix static locals in grab_thread_process()
Status: DONE

grab_thread_process() runs in multiple threads (up to 3 GrabThread instances). Static local variables were shared across all threads, causing potential data races.

Changes:
- `static int packets_lost` — removed `static` (reset to 0 immediately after declaration)
- `static double min, max` (LVTONG) — removed `static` (output params from cv::minMaxLoc, written before read)
- `static bool frame_a = true` — moved declaration before while loop (needs to persist across iterations)
- `static int baseline = 0` (2 instances, LVTONG) — removed `static` (output param from cv::getTextSize)
- `static cv::Mat temp` — moved to `scan_temp` declared before while loop (needs to persist across scan iterations)
- `static QString scan_save_path_a, scan_save_path` — moved declaration before while loop (set in one iteration, read in subsequent)
- `static cv::Mat frame_a_avg, frame_b_avg` — removed `static` (written by convertTo() before each read, no persistence needed)

## Build Verification
Status: PASS

Post-build analysis: drag&drop image loading is unaffected by the frame_a change (separate code path). The "frame_average + 3D works with 4 images but not 6/8/10" behavior is pre-existing — caused by the queue cap of 5 reducing even counts to an odd count, which rotates A/B phase assignment each cycle.
