# Sprint 26 plan: free the pipeline from the layers above it

2026-11-14. Branch `refactor-2`, cut from `master` at `5481571`.

**Status: approved on 2026-11-14, with the decisions in section 7. Steps 1 to 3 are built and
checked; their sections give the results and what is still open. Step 4 was done in two
halves and is built and checked at the desk; the scans on the hardware are still open.
Steps 5 and 5b are built and checked. Step 6, the last step, is built and checked: the
pipeline test passes. What stays open is listed at the end of section 5.** Line numbers are
those of the tree at `5481571`.

The subject is finding S-3: the pipeline holds four controllers and a window, and steps the
scan from inside the frame loop. The findings are listed in section 10 of
`../1. analysis/structure-design.md`. This file says what will change, in which order, and how
each step is checked.

On the number: the sprint 25 document carries a "sprint 26: style unification". That work was
done as commit `5481571`, outside the sprint numbering, so the number was free.

## 1. Rules of this sprint

This code decides what the operator sees, so the work follows these rules.

- A step changes where a value comes from or where a result goes. No step changes what a stage
  computes.
- One concern per step. Each step builds and runs before the next one starts.
- I cannot build or run. After each step you build, run the checks listed for it, and tell me
  the result.
- I change only the files a step names. Anything else I find is reported, not fixed.
- No commit before the end: one commit for the sprint, with the message shown to you first.
  After each step you have checked, I save the whole diff as a patch file, a local working
  file outside the tracked folders, so the tree can be put back to any checked state without
  a commit.

## 2. Goal

When S-3 is done:

- `pipeline/pipelineprocessor.h` and `.cpp` include nothing from `controller/`, `widgets/`,
  `visual/` or `port/`.
- `PipelineGlobals` holds no controller, no window, and no pointer to a value that another
  thread writes without a lock.
- A frame thread calls no method of a controller or a widget.
- What the operator sees and what is saved stay the same, except for the points in section 6.

How it is checked: the two files contain no such `#include`, `PipelineGlobals` has the form
shown in 4.5, no call in the two files goes to a controller or a widget, and a test target
builds the pipeline without any controller or widget source (step 6).

## 3. What the pipeline takes from above today

All in `src/pipeline/pipelineprocessor.cpp`.

| # | Taken from above | Lines | Kind |
|---|---|---|---|
| 1 | TCU values for the overlay text: base unit, delay, gate width, `dist_ns` | 317-318 | read |
| 2 | Auto-MCP flag, and the current MCP from the TCU port | 441-445 | read |
| 3 | 3D: delay, gate width, `dist_ns`, and the port's delay and gate-width offsets | 658-678 | read |
| 4 | 3D frame A/B flag: read, and toggled after each frame | 658-681 | read and write |
| 5 | Whether the histogram panel is shown: `AuxPanelManager::get_alt_display_option()` | 824 | read |
| 6 | The display widget: its size, and its `set_pixmap` signal, emitted from the frame thread | 816, 979 | widget |
| 7 | The second window: its display widget, its size, its signal | 985-990 | widget |
| 8 | Scan state: running flag, name, three routes, two indices | 1373-1575 | read |
| 9 | Scan parameters: 17 fields through a bare pointer | 1314-1598, 1733-1742 | read, no lock |
| 10 | Scan progress: `set_scan_tcu_idx`, `set_scan_ptz_idx` | 1443-1555 | write |
| 11 | Scan commands: `send_ptz_angle`, `set_delay_dist`, `set_depth_of_view` | 1450-1569 | command |
| 12 | The save folder: a `QString*` into `UserPanel` | 1382-1719 | read, no lock |
| 13 | The TCU note written into saved BMP files: `get_tcu()->to_json()` | 1670-1757 | read |

What already goes the right way, and is the model for the rest:

- Each frame starts with a copy of `ProcessingParams` and `PipelineConfig`, taken under
  `params_mutex` (line 287).
- Results leave through 16 signals (18 counting two behind the `LVTONG` flag, which no build
  defines).
- One command already leaves as a signal: `update_mcp_in_thread`.

## 4. Design

Three mechanisms, all of them already in the file.

### 4.1 Values in: more fields in the per-frame copy

Rows 1, 2, 3, 5, 6, 7, 12 and 13. `ProcessingParams` gets:

```cpp
struct TcuReadout {
    int     base_unit     = 0;
    float   delay_dist    = 0;      // m
    float   depth_of_view = 0;      // m
    float   dist_ns       = 0;      // m per ns
    bool    auto_mcp      = false;
    double  mcp           = 0;      // as the TCU port reports it
    double  offset_delay  = 0;
    double  offset_gw     = 0;
    QString note;                   // TCU state as JSON text, for the BMP note
};

// in ProcessingParams
TcuReadout tcu;
int        alt_display_option = 0;
QSize      display_size[3];         // the three display widgets
QSize      secondary_size;          // the display of the second window
QString    save_location;
```

One function writes it, on the GUI thread: `UserPanel::update_processing_params()`, as today.
It re-reads every field each time it runs, so a missed trigger is repaired by the next trigger
of any kind. New triggers:

| Value | Trigger |
|---|---|
| Base unit, delay, gate width, `dist_ns`, auto MCP | A new signal `TCUController::pipeline_inputs_changed()`, emitted by `set_delay_dist`, `set_depth_of_view` and `set_auto_mcp`, and after each place in `tcucontroller.cpp` that writes one of the five members |
| MCP, the two offsets, the note | A new signal `TCU::state_changed()`, emitted at the end of both `set_user_param` overloads and forwarded as `DeviceManager::tcu_state_changed()`. See the correction below |
| Histogram option | `AuxPanelManager::display_page_changed`, emitted at the only place that changes it. `userpanel.cpp:1109` disconnects every listener of this signal, so the connection is made after that line |
| Save folder | The three live places that assign `m_save_location`. The fourth is in `convert_read`, which nothing calls |
| Size of the second window's display | `Display::widget_resized` of that widget. The three displays are connected already (`userpanel.cpp:698`) |

Each connection names the window as its context object. A call that still comes from a frame
thread during steps 1 to 3 is then queued to the GUI thread and never reads widgets there.

**Correction of 2026-11-14, made before step 1 was written.** The approved plan took MCP, the
offsets and the note from the end of `TCUController::update_tcu_params`. That was wrong: the
TCU port announces a change only for laser width, delay and gate width
(`tcu_param_updated`), and stores MCP and the offsets without telling anyone
(`tcu.cpp:231-232`, `243`). The copy would have kept an old MCP, and auto MCP could have
stalled in step 2. The port now has one more signal, `state_changed()`, which carries no
echo meaning and makes a listener read the state again. This adds four files to step 1:
`port/tcu.h` and `.cpp`, `controller/devicemanager.h` and `.cpp`.

### 4.2 The 3D frame flag becomes pipeline state

Row 4. `m_frame_a_3d` sits in `TCUController`, but nothing about it belongs to the TCU: the
pipeline toggles it after each frame while 3D runs without frame average (line 681), and the
GUI resets it when 3D is switched off
(`userpanel.cpp:5109`) and swaps it on request (`userpanel.cpp:7155`). It moves to `DisplaySlot`
as `std::atomic<bool> frame_a_3d`, next to the record and save flags, which work the same way.

- The pipeline reads the flag once per frame, where it read it twice, and flips it with the
  same compare-exchange as before.
- The GUI writes the flag of the main display, whichever display is picked. One flag served
  the whole program, and the pipeline builds 3D on the main display only, so this is what
  happened before.

### 4.3 Images out: the pipeline's own signals

Rows 6 and 7. Two new signals, `display_frame_ready(int display_idx, QPixmap pm)` and
`secondary_frame_ready(QPixmap pm)`. Today the frame thread emits the widget's own signal. In
both forms the image crosses to the GUI thread through one queued call. The sizes come from
the copy (4.1).

**Correction of 2026-11-15, made before step 3 was written.** The plan connected the new
signals to `setPixmap` of the widgets. That would have switched off the thumbnails: those of
the setup overlay, of the scan overlay and of the side panel are fed by the display's own
`set_pixmap` signal. `UserPanel` therefore receives the frame on the GUI thread and emits
that signal of the widget. The display takes the frame in the same GUI event; a thumbnail
takes it one event later, where today both are queued from the frame thread.

### 4.4 Scan: one value in, one signal out

Rows 8 to 11. The stepping logic stays in `advance_scan_main` and
`advance_panorama_scan_main`, line for line. Only its inputs and outputs change.

- **In: a `ScanJob`.** It holds the running flag, the scan name, a copy of `ScanParams`, the
  three routes, and two counters: `revision`, raised at every change, and `generation`, raised
  at every start. `ScanController` sends it as a signal value at start, pause, continue and
  stop, and when a scan parameter is edited, so that an edit during a scan takes effect as it
  does today. Every write to the parameters goes through `ScanController::get_params()`, which
  will schedule the re-send. `UserPanel` stores the job under `params_mutex`. The pipeline
  compares `revision` once per frame and copies the job only when it has changed.
- **The two indices** are counted by the pipeline, the only code that advances them. They
  restart at -1 when `generation` changes, which is the value `ScanController` gives them at a
  start today.
- **Out: `scan_step_requested(ScanStep)`.** It carries the new indices and what to command: the
  PTZ angles, the delay in ns, the gate width in ns. A new slot
  `ScanController::apply_scan_step` runs on the GUI thread. It stores the indices, calls
  `DeviceManager::send_ptz_angle`, and sets delay and gate width with today's arithmetic
  (route value times `dist_ns`).

Folder creation and the params files stay in the pipeline.

The work is done in two halves, 4a for what goes in and 4b for what comes out; see step 4.

### 4.5 `PipelineGlobals` afterwards

```cpp
struct PipelineGlobals {
    QMutex           *params_mutex      = nullptr;  // guards processing_params and scan_job
    ProcessingParams *processing_params = nullptr;
    ScanJob          *scan_job          = nullptr;

    QMutex                *roi_mutex = nullptr;
    std::vector<cv::Rect> *list_roi  = nullptr;

    QMutex     *yolo_init_mutex = nullptr;
    ThreadPool *tp              = nullptr;
};
```

Gone: `tcu_ctrl`, `scan_ctrl`, `device_mgr`, `aux_panel`, `secondary_display`, `scan_params`,
`save_location`.

## 5. Steps

### 5.1 Reference frames, before step 1

A still image gives the pipeline the same input every time, so saved results can be compared
byte for byte.

1. Load a still image file as the source. It has to be loaded as mono, because pseudocolor, 3D
   and the histogram run on mono images only. `load_image_file` takes a file as mono when it
   is a TIF, or an 8-bit BMP with a palette, which is what the program itself saves from a
   mono camera. A gray PNG or JPG is taken as colour. If PSEUDOCOLOR changes the picture, the
   image was loaded as mono.
2. Switch INFO off, because the time stamp is drawn into the saved result. Leave the display
   unzoomed and the window maximized as it starts, because the saved result is cropped to the
   visible region.
3. Leave delay and gate width as they are at program start. Keep the custom 3D parameters
   (`CUSTOM_3D_PARAM_CHK`) off, so that the 3D stage takes its values from the TCU. Keep 3D
   truncation (`TRUNCATE_3D_CHK`) off as well: with identical frames it removes every pixel,
   and the numbers on the colour bar read 0.00. With 3D on, the three numbers should all show
   the same value, not zero. In air and with zero offsets it is the delay distance plus half
   the gate width.
4. Save the processed result (RES) once for each setting, with the others off:
   - nothing switched on
   - enhance, option Histogram, which has no parameters
   - pseudocolor
   - 3D
   - nothing switched on, with the save option that writes the TCU info into the BMP file
     (`INTEGRATE_INFO_CHK`)
5. Keep the folder. After steps 2 and 3, save the same set in the same order into a new folder
   and give me both paths. I compare the files. Any difference is a change in the stages.

**The set as taken, 2026-11-14, with the build made before step 1.** Five files, saved between
23:35:55 and 23:36:47. It differs from the list above in two points, and both stay as they are
for the later sets:

- The TCU info is in the first four saves and not in the fifth. The option is on by default.
- INFO was on, so the time of the save is drawn into the top right corner of each picture.

| # | Setting | TCU info | Picture | What it shows |
|---|---|---|---|---|
| 1 | nothing on | yes | 1608 x 1104, 8 bit | the plain path |
| 2 | enhance, Histogram | yes | 1608 x 1104, 8 bit | mean grey 64.7 becomes 128.7 |
| 3 | pseudocolor | yes | 1608 x 1104, 24 bit | the colour map |
| 4 | 3D | yes | 1712 x 1104, 8 bit | the numbers on the bar read 18.00: 15 m delay plus half of 6 m |
| 5 | nothing on | no | 1608 x 1104, 8 bit | the same picture as 1 |

The note in saves 1 to 4 reads delay 100 ns, gate width 40 ns, laser width 40 ns, MCP 5.

How two sets are compared. Two things in a file change with every save and are left out:

- the time the note carries, 8 bytes behind the picture;
- the box the time stamp is drawn in: 33 rows by 263 columns in the top right corner, 0.5 % of
  the picture. The three numbers on the bar of the 3D save lie outside the box.

Everything else has to be equal byte for byte: the header, every pixel outside the box, and
the text of the note. Saves 1 and 5 were taken 52 s apart and are equal in this sense, which
shows that a still image gives the same picture every time.

A later set is taken the same way: the same image, right after the program starts and before
any other control is touched, the same five settings in the same order, INFO left on. It may
go into the same folder; the first five files a run saves are the set. A copy of the first
set is kept with the working files, outside the tracked folders.

**Changed on 2026-11-15: the image no longer has to be the same.** The set of step 2 came
from another frame of the same camera series, because it was not known which file the first
set had been taken from. Both source files were then found by their content: a plain save
equals its source outside the stamp box. Since then each set is checked against its own
source, which also lets sets from different frames be compared:

- plain: equals the source;
- Histogram: equals `cv::equalizeHist` of the source, computed again outside the program;
- pseudocolor: every pixel has the one colour of its grey level, and the table of colours is
  the same in both sets. This needs sources whose grey values run from 0 to 255, since the
  stage stretches the picture to that range first;
- 3D and the text of the note do not depend on the picture and are compared directly.

The first set passes these checks too, so the recomputation agrees with the build made
before step 1. Any 8-bit mono frame of that series can serve for a later set. The size of
the window has no effect on a save: the first set was taken maximized, the second was not.

What this set shows and what it does not:

- The first three saves show that nothing else moved. Steps 2 and 3 do not touch these stages.
- The 3D save does not test the reconstruction. One still image gives frame A and frame B the
  same content, so every pixel gets the same range and the picture is flat. It is kept for
  the three range numbers printed on the colour bar: `gated_3d_v2` computes them from delay
  and gate width, which the pipeline derives from delay, gate width, `dist_ns` and the two
  offsets. These are the values step 2 moves, and a wrong one changes the file.
- Frame average is left out. The average of identical frames is the frame itself, so the file
  would equal the first save.
- The A/B assignment and the averaging need frames that differ. They are checked on live
  frames in step 2, and by the test of step 6.

### 5.2 Reference scans, before step 4a

A scan saves into a folder of its own, so two runs of the same scan can be compared file by
file. The source is a still image as in 5.1, which makes every saved picture equal to the
source.

For all three scans: INFO off, so that no time stamp gets into the saved pictures and into
the panorama; capture of ORI and of RES on; 2 images per position; wait time 1 s.

| | Scan | Settings | Expected in its folder |
|---|---|---|---|
| A | grid, delay and PTZ | delay from 2000 ns in 3 values of step 500 ns; PTZ 2 by 2; gate width off | 4 positions with 3 profiles each, 2 ORI and 2 RES pictures per profile |
| B | grid, delay only | as A, with PTZ off | 1 position with 3 profiles |
| C | panorama | 3 positions | 3 positions with 2 ORI and 2 RES pictures each, and `panorama.jpg` |

The delays lie above the delay floor of 1000 ns on purpose; see finding 5 in section 8. The
number of delay values is not set in the delay card: it follows from start, end and step,
or is entered as PROFILES in the capture card.

Compared between two runs: the names of the folders and files, the text of `scan_params`
and of every `params` file, every picture and the text of its note. Left out: the time in
the name of the scan folder and of the screenshots, and the save time inside a note.

A scan may start again by itself when it has finished (finding 2). It is then stopped with
the SCAN button, and the first complete folder is the one compared.

### Step 1. Publish the new values; nothing reads them yet

- The fields and triggers of 4.1.
- A temporary check in the pipeline. Once per frame it compares each new field with the live
  value it will replace, and writes a `shadow:` warning to the log when a field has differed
  for 10 frames in a row. It proves that no trigger is missing before anything depends on the
  copy. Step 5 removes it.

Files: `pipeline/processingparams.h`, `controller/tcucontroller.h` and `.cpp`,
`visual/userpanel.cpp`, `pipeline/pipelineprocessor.cpp`, and, by the correction in 4.1,
`port/tcu.h` and `.cpp`, `controller/devicemanager.h` and `.cpp`.

You check: run with a camera or a video. Move the delay and gate sliders, change the MCP,
change the base unit, toggle auto MCP and the underwater switch, change a delay or gate-width
offset, show and hide the histogram, resize and maximize the window, open the second window,
change the save folder. The log has no `shadow:` line. The log is the newest file in
`GLI_logs`, next to the program.

The check does not cover the save folder and the TCU note. Comparing a `QString` that the GUI
thread may be replacing is unsafe from a frame thread, and the note is too costly to build for
every frame. Both are checked by reading the code: every assignment of the save folder is
followed by a refresh, and the note is built in the same call as the MCP value.

**Result, 2026-11-14.** The build succeeded and the log of the run has no `shadow:` line. The
log also shows how much that says. The source was a video file that played for 16 s
(23:41:30 to 23:41:46), and the check compares only while a source delivers frames. What was
changed after the video ended was not compared, the change of the window size at 23:42:32
included. So the result holds for those 16 s, and the log cannot tell which values were moved
in them. Two things follow:

- The check stays in the code through steps 2 to 4, and the run of step 2 uses a still image,
  which never ends.
- From step 2 on, the check writes one `shadow summary:` line when a frame thread ends: the
  number of comparisons, the values a difference stuck on, and how often each live value
  moved. A value that never moved was not tested by that run.

### Step 2. Read TCU values, histogram option and save folder from the copy; move the 3D flag

Rows 1 to 5, and rows 12 and 13 as far as `save_to_file` uses them.

**Changed against the approved plan, before the step was written:**

- The scan functions keep their own reads of the save folder and of the TCU note until
  step 4. They would need a new parameter, which changes `pipelineprocessor.h`, and only a
  scan can check them. Step 4 rewrites their inputs anyway and is checked by scans.
- One more file: the constructor in `controller/tcucontroller.cpp` loses the initialiser of
  the flag that moves.
- The temporary check gets the summary line described above.

Files: `pipeline/pipelineprocessor.cpp`, `pipeline/routing.h`, `visual/userpanel.cpp` (two
places), `controller/tcucontroller.h` and `.cpp`.

You check, with the still image of 5.1 as the source for the whole run:

1. First, before any other control is touched: the five reference saves of 5.1.
2. 3D on: the numbers on the bar read 18.00. Move the delay and the gate width: the numbers
   follow. Underwater on, from the start values: 24.00, and back to 18.00 when off. A delay
   offset lowers them by the offset.
3. Auto MCP on: the MCP runs from 5 down to 0. The picture is bright enough to ask for less
   gain, and a still image never gets darker. Stopping at 1 would mean that the frame path
   does not see the new MCP.
4. The histogram panel shows the histogram.
5. Set the delay to a value you will recognise and save one RES with the TCU info on. I read
   the note back from the file.
6. Change the save folder and save once: the file lands in the new folder.
7. Also change the base unit, resize and maximize the window, and open the second window.
   These feed the summary line.
8. Close the program. I read the log and the files.

With frames that differ, from a camera or a recording of gated frames, when one is at hand:

- The 3D picture, with and without frame average, and the swap of the two frames (RESET 3D).
- Auto MCP settles as before.

The text `DELAY ... GATE ...` that the frame path builds is not on this list. Nothing draws
it; see finding 4 in section 8.

**Result, 2026-11-15.** Built at 15:44. Three runs; the checks were made in the third, from
15:57 to 16:04.

- No `shadow:` line in any of the three logs. The summary of the third run: 29,766
  comparisons on the main display, no value stuck. Moved while compared: delay 117 times,
  gate width 21, `dist_ns` 2 (underwater on and off), base unit 1, auto MCP 1, MCP 2,
  histogram option 1, display size 8. Not moved: the two offsets and the size of the second
  window. The alt displays had no source, so their sizes were not compared.
- Reference frames: pass, by the checks of 5.1 as changed on 2026-11-15.
- 3D: 18.00 at the start, and 153 with the delay at 150 m, which is 150 plus half of 6.
  Underwater and gate width were found right.
- Auto MCP ran from 5 to 0 in two steps (5, 1, 0).
- The save made after the delay was set to 3369 m carries delay 22460 ns and MCP 0 in its
  note.
- After the change of the save folder, the next save landed in the new folder.
- Open: the checks that need frames that differ (the 3D picture, the swap of the two frames,
  auto MCP settling), and a delay or gate-width offset.
- Seen once in the log of the third run: `TCU param 259 did not echo within 500 ms`, the
  watchdog of the delay command. It is the first such line in the 19 logs of the build
  folder. No way from steps 1 and 2 to it was found. It is looked for in the next logs.

### Step 3. Images leave through the pipeline's own signals

Rows 6 and 7. The frame path sends each picture out through its own signal and takes the
sizes from the copy. It no longer reads a widget.

**Changed against the plan as it stood, before the step was written:**

- The frame goes to the display's `set_pixmap` signal, not to `setPixmap`; see the
  correction in 4.3.
- The temporary check keeps all its values. This step makes the frame path depend on the
  two sizes, so this is the run in which they have to be compared. The check keeps the
  three includes and the two pointers (`aux_panel`, `secondary_display`) it reads them
  through. The frame path does not use them any more. They go in step 5, with the check.

Files: `pipeline/pipelineprocessor.h` and `.cpp`, `visual/userpanel.cpp`.

You check:

1. First, before any other control is touched: the five reference saves of 5.1, from any
   frame of that camera series.
2. The main display: resize the window, maximize it and restore it. The picture fills the
   display each time, and no blank or wrongly sized picture stays.
3. The thumbnails still follow: in the setup overlay, in the scan overlay and in the side
   panel.
4. A source on ALT 1 and on ALT 2; a video file is enough. Each shows in its window and as a
   thumbnail. Resize one of the two windows.
5. The second window: on, resize it, off.
6. Swap two displays: no old picture stays behind.
7. Close the program. I read the log; this time the sizes of all three displays and of the
   second window should appear as moved.

**Result, 2026-11-16.** Built at 08:31. Two runs.

- No `shadow:` line and no warning in either log.
- Sizes while compared: the main display changed 6 times, ALT 1 changed 30 times in 5,563
  comparisons (its window was resized), ALT 2 was compared 5,270 times at one size. None
  stuck. A file cannot be opened on an alt display directly, so a network stream ran on
  ALT 1, and the main picture reached ALT 2 by a swap.
- The second run moved a delay offset once, auto MCP once and the MCP 36 times. None stuck.
  This closes the offset that step 2 left open.
- Reference frames: pass, from a third frame of the series.
- Not seen: a change of the size of the second window. Its path is the one part of this
  step the logs do not confirm. It turned out that the window had not been opened: it is
  the one Alt+0 shows, fed with the unprocessed picture while DUAL DISPLAY is ticked. It
  is checked with the next run.

### Step 4. Scan through the job and the step signal

Rows 8 to 11, and what step 2 left of rows 12 and 13.

**Changed against the plan as it stood: the step is split in two.** The scan cannot be run
on the hardware at the moment, so each half has to be checkable at the desk. Step 4a changes
only what the scan reads and must leave every folder and file as it is. Step 4b changes what
the scan commands, and with it the two things that are meant to change (D1 and D3).

The desk check for both halves is a set of reference scans, described in 5.2.

#### Step 4a. The scan reads from the job

Rows 8, 9, 12 and 13. `ScanController` announces a `ScanJob` at start, pause, continue and
stop, and after every write to its parameters. `UserPanel` keeps it under `params_mutex`,
and the frame thread copies it when its revision has moved. The scan functions take the
running flag, the name, the parameters and the three routes from that copy, and the save
folder and the TCU note from the per-frame copy.

Not changed in this half: the two indices are still read and written in `ScanController`,
and the PTZ and TCU calls are still made from the frame thread. A finished scan is still
reported by the toggle.

- Two more files than planned: `visual/userpanel.h` gets the member that holds the job, and
  `CMakeLists.txt` lists the new header.
- The writable `get_params()` schedules the new job for the end of the current GUI event,
  because its caller writes the fields after it has returned. Start and stop announce at
  once.
- The temporary check gains the scan: the running flag, the name, the mode, the three
  dimension switches, the two capture switches, the images per position and the wait times.
  `PipelineGlobals` keeps `scan_params` for it, and loses `save_location`.

Files: `pipeline/scanjob.h` (new), `controller/scancontroller.h` and `.cpp`,
`pipeline/pipelineprocessor.h` and `.cpp`, `visual/userpanel.h` and `.cpp`, `CMakeLists.txt`.

You check:

1. **Before the build**, with the program as built for step 3: the three reference scans
   of 5.2.
2. Build, and run the same three scans again.
3. In one more grid scan: pause and continue it, and change the images per position while
   it runs. Stop one and start it again.
4. Close the program. I compare the folders of the two runs and read the log.

**Result, 2026-11-16.** The reference scans were taken with the step 3 build, then step 4a
was built at 09:25 and the scans were repeated.

- The build succeeded. No `shadow:` line. The job copy was compared 23,900 times and never
  stuck, while the running flag changed 18 times, the name 9 times, mode and dimensions
  twice each and the images per position once.
- Equal in both runs, for all three scans: the names of folders and files, `scan_params`,
  every `params` file, and the note of every picture. Every picture equals the frame it was
  taken from. The two runs used different frames of the camera series, so the pictures were
  checked against their own source and not against each other.
- Every note of both runs reads delay 100 ns, in the profiles named 2000, 2500 and 3000 ns
  as well. This is finding 1 in the files: the scan does not command the TCU.
- Not comparable: the second run has no RES pictures. The capture switches did not change
  during that session, so RES was not ticked in it. They are not kept between program
  starts.
- **A flaw of step 4a, found from the file times.** In the second run, scan A started again
  by itself (finding 2), and the restarted scan wrote its first profile into the folder of
  the scan before it: two pictures were overwritten and a second screenshot was added. Step
  4a reads the name from the frame's copy of the job but still takes the indices from
  `ScanController`. When a scan is stopped and started within one frame, the frame at work
  pairs the old name with the new indices. A normal start cannot do this, because the copy
  then says that no scan is running. The step 3 build has the same double first step on a
  restart, but inside the new scan's own folder.
- No patch was saved for this state. Step 4b removes the cause (the indices move into the
  frame thread) and the trigger (a finished scan no longer restarts).

#### Step 4b. The scan commands through a signal

Rows 10 and 11. The frame thread counts the two indices itself. They start again when the
job's generation moves, from the indices the job carries: -1 for a new scan, and the place
`ScanController` last saw for a scan that was under way when the frame thread started. At
each step the frame thread emits `scan_step_requested(ScanStep)`, and
`ScanController::apply_scan_step` on the GUI thread stores the indices, moves the PTZ, sets
delay and gate width through `update_delay()` and `update_gate_width()`, which send them
(D1), and announces the job again so that it carries the new indices. The end of a scan
carries the generation and stops only that scan, once (D3). The signals
`update_delay_in_thread` and `update_gate_width_in_thread` go; nothing has emitted them
since July.

The frame path then uses no controller and no port. The temporary check still reads them,
so their includes and pointers stay for it until step 5.

Files: `pipeline/scanjob.h`, `controller/scancontroller.h` and `.cpp`,
`pipeline/pipelineprocessor.h` and `.cpp`, `visual/userpanel.cpp`.

You check, with a still image, INFO off and capture of ORI and RES ticked:

1. The three scans of 5.2. Expected against the reference run, and nothing else: one press
   of SCAN leaves one folder, and in scan A the notes of each profile carry that profile's
   delay. In scan B they may lag: a delay-only scan has no wait time, and a still image
   steps faster than the TCU answers.
2. Pause a grid scan and continue it. Stop one and start it again.
3. Press SCAN to stop a scan near its end: no new scan starts.
4. The delay slider follows a delay scan and stays at the last value when it ends.

On the hardware, when it is at hand: a PTZ grid scan, a delay scan, a gate-width scan and a
panorama scan, with the PTZ moving at each position.

**Result, 2026-11-16.** Built at 10:10. One run with six scans.

- No `shadow:` line and no warning. 23,972 comparisons, nothing stuck. The delay moved 15
  times, once for each of the 12 steps of scan A and the 3 of scan B.
- One folder per press of SCAN: six scans, six folders. The three scans that ran to their
  end stopped there, and none started again (D3).
- Scan A: every note carries the delay of its profile, 2000, 2500 and 3000 ns, where the
  reference run has 100 ns throughout (D1). Names, `scan_params`, the `params` files and
  the screenshots, one per profile, equal the reference run. Every picture equals its
  source frame.
- Scan B: the notes lag by one profile and more, as expected for a delay-only scan on a
  still image.
- Scan C and three more panoramas: one was paused and continued and finished with its
  `panorama.jpg`. Two were paused and left; the scan started after each began at its first
  position and wrote nothing into the folder of the one before.
- The second window was opened and resized: its size changed 327 times while compared and
  never stuck. This closes the point step 3 left open. The owner saw the picture in it
  stretched; that is finding 9 in section 8, which is older than this plan.
- Not exercised: capture of RES during a scan, which was not ticked in this run either, and
  the gate-width offset, which no run has moved. It shares its trigger with the delay
  offset.
- Still open: the scans on the hardware.

### Step 5. Remove the temporary check

The check goes, and with it what only the check still needs: the includes of the
controllers, of the TCU port and of the widgets, and the pointers to them in
`PipelineGlobals`. I verify the criteria of section 2 and report the result.

Files: `pipeline/pipelineprocessor.h` and `.cpp`, `visual/userpanel.cpp`, and one comment in
`pipeline/scanparams.h` that named the removed pointer.

Written on 2026-11-16: 164 lines go, 2 change. The criteria of section 2, read off the tree:

- The two pipeline files include nothing from `controller/`, `widgets/`, `visual/` or
  `port/`.
- `PipelineGlobals` has the form shown in 4.5.
- The names of controllers, ports and widgets occur in the two files in comments only. Of
  a display's widget the frame path keeps one test: whether the pointer is set.
- The fourth criterion, a test target that builds the pipeline alone, is step 6.

One thing the criteria do not cover: `pipelineprocessor.cpp` includes `image/imageio.h`,
which includes `util/util.h` and with it Qt Widgets. That is finding M-6 of the structure
design, not part of S-3.

You check: the build, and a short run with the five reference saves, one scan A with RES
ticked under CAPTURE, and the second window once. The log then has no `shadow` line of any
kind, because the check is gone.

**Result, 2026-11-16.** Built at 11:04; the build succeeded with the check and its includes
gone.

- No warning in the log.
- Reference frames: pass, from a fourth frame of the series.
- Scan A with RES ticked: 24 ORI and 24 RES pictures and 12 screenshots in one folder.
  Every picture equals the source frame, and every note, of ORI and of RES, carries its
  profile's delay. This closes the capture of RES that step 4 left open.

### Step 5b. Three changes asked for by the owner

Asked for on 2026-11-16, after the runs of steps 4b and 5. They change behaviour on purpose
and are therefore kept apart from step 5, which changed none.

1. **The second window takes the shape of the picture.** Finding 9 in section 8.
   `start_static_display` now sizes the second window for a source on the main display, as
   the camera start does.
2. **A new scan starts at once.** Finding 3. When the generation of the job moves, the
   frame thread drops what it kept of the scan before: the frame counter, the two folder
   paths, the panorama canvas and the wait timer.
3. **A scan step is not held at the delay floor.** This amends D1. A new
   `TCUController::apply_scan_delay` sends the delay with the upper limit only, and
   `ScanController::apply_scan_step` calls it in place of `update_delay()`. The delay
   slider cannot show a value below its minimum and stays there; the estimate text shows
   the real value. The slider reports only drags, so its minimum is not sent back.

Files: `visual/userpanel.cpp`, `pipeline/pipelineprocessor.cpp`,
`controller/tcucontroller.h` and `.cpp`, `controller/scancontroller.cpp`.

You check, with a still image:

1. The five reference saves.
2. The second window (DUAL DISPLAY ticked, Alt+0): the picture has its own shape, and
   keeps it when the window is resized.
3. Pause a grid scan in the middle of a position and press SCAN: the new scan begins at
   its first position at once, and the folder of the paused scan gets no more files.
4. A delay scan below the floor, for example 200, 400 and 600 ns with PTZ on: the notes
   carry those delays. The delay slider stays at its minimum meanwhile.

**Result, 2026-11-17.** Built at 15:45. One run.

- No warning in the log. Reference frames: pass, from a fifth frame of the series.
- Second window: checked by the owner, no issue reported.
- Stop, then start: a grid scan with delays of 100 and 200 ns was paused in the wait of
  its fifth profile, and SCAN was pressed 1.8 s later. The new scan wrote the `params`
  file of its first position 16 ms after its start and went through all four positions.
  The folder of the paused scan got no file after the pause.
- Below the floor: the pictures of these scans carry no note, because the TCU info was
  left off after the fifth reference save. The log shows it instead: when the second scan
  started, the port held a delay of 200 ns, the last value the paused scan had commanded.
  With the floor it would have held 1000 ns.

### Step 6. A pipeline test (decision D4)

- `TimedQueue` moves from `userpanel.cpp` into `util/timedqueue.cpp`. The pipeline uses it,
  and a test cannot link `userpanel.cpp`.
- New `tests/test_pipeline.cpp`. Frames go in, and frame signals come out with the expected
  sizes. A 2 by 2 PTZ grid with a delay route produces the expected sequence of step requests
  and saved files.
- The same test feeds frames that differ, which a still image cannot do. Four different
  frames give their mean under frame average. Two different frames give the expected range
  map under 3D, and the A/B flag exchanges their roles.

Files: `visual/userpanel.cpp`, `util/timedqueue.cpp` (new), `CMakeLists.txt`, `tests/`.

You check: the build with tests, and that the new test passes.

**Written on 2026-11-17.**

- `TimedQueue`: its 60 lines moved by script, unchanged except that `set_display_label`
  and `count` lose their `inline`. Defined inline in a source file they exist in that file
  only, and `userpanel.cpp` still calls them.
- The test target `test_pipeline` lists the pipeline with the image, yolo and util sources
  it uses, and nothing from `controller/`, `port/`, `widgets/` or `visual/`. That it links
  is the fourth criterion of section 2. It is defined for Windows only, because the yolo
  sources include `Windows.h` and need ONNX Runtime.
- Five cases:

| Case | Input | Expected |
|---|---|---|
| TC-PL-001 | one frame, widget size in the copy set to half the frame | the picture leaves at that size |
| TC-PL-002 | four flat frames of grey 10, 20, 30, 40, frame average on | the result is 25 everywhere |
| TC-PL-003 | two patterned frames, 3D on, once with the frame flag off and once on | the range map equals `gated_3d_v2` of the two frames, in the order the flag gives; the flag is back at its start value |
| TC-PL-004 | grid scan of 2 by 2 positions and two delays, on a frame that repeats | eight steps in order, the PTZ asked to move at each new position only, two pictures saved per profile, the end announced once |
| TC-PL-005 | a scan held in its first wait, then a second scan published | the second begins at its first position at once and runs to its end; no picture is saved for the first |

- The test hands the pipeline a stand-in for the display's widget pointer. The pipeline
  only tests that pointer for null, and sends no picture without it. This is the last trace
  of the widget in the frame path and belongs to S-4.
- What the test does not cover: panorama scans, the saved RES pictures, enhance and
  pseudocolor (those have the reference frames), and anything with a real device.

**Result, 2026-11-17.** Two builds: the program at 16:06, the whole project with the test
at 16:29. The builds of the earlier steps made the program alone, and so did the first one
here; the test programs are part of the default target but were not rebuilt until the
second.

- The program: the FPS label of the main display counts as before, which is the part of
  the program that the move of `TimedQueue` touches. No warning in the log. Reference
  frames: pass. The frame of step 5b was loaded again, so the two sets could be compared
  directly: all five saves are equal outside the time stamp, the 3D picture included.
- The test: it built and linked as written, without a change to the test or to its
  target. `test_pipeline` reports 8 passed, 0 failed, 0 skipped. The eight are the five
  cases, TC-PL-003 once for each value of the frame flag, and the opening and closing
  functions of the test framework. Run by the owner, then thirty more times in a row:
  every run passed, in about 0.7 s, and no temporary folder was left behind.
- That the target links is the last check of section 2. All of them hold now: the
  pipeline builds and runs with no controller, port, widget or window source.
- The two older tests whose sources this sprint changed pass as well: `test_tcu` with 15
  cases and `test_scanparams` with 20.
- The test frames have 48 rows on purpose; see finding 11 in section 8.

### Open after step 6

Everything above was checked at the desk, with still images, files and no device. Three
checks need what the desk does not have:

- The scans on the hardware: a PTZ grid scan, a delay scan, a gate-width scan and a
  panorama scan, with the PTZ moving at each position and the TCU answering (step 4).
- The program with frames that differ: the 3D picture from a gated camera, the swap of the
  two frames, and auto MCP settling (step 2). The test of step 6 covers the pairing of the
  two frames and the frame average, not the program with a camera.
- The gate-width offset. No run has moved it. It shares its trigger with the delay offset,
  which was moved and compared in step 3.

## 6. Behaviour that changes

- During a scan, the PTZ command passes through the GUI thread. It leaves one queue hop later,
  milliseconds against a settle time of seconds.
- The TCU values of one frame come from one moment. Today a frame can read them at several
  moments.
- Auto MCP takes the current MCP from the copy. The copy follows the port once the command
  has been sent, which takes the port 20 ms or more; today the frame thread reads the value
  as soon as the port has stored it. The steps are the same; one can come a frame later.
- The 3D frame flag is read once per frame instead of twice.
- A picture is scaled to the widget size held in the copy. After a resize, a frame that was
  already under way arrives in the old size, and the next one is right. Today the size is
  read when the frame is finished, so this happens less often.
- A thumbnail gets a frame one GUI event after its display, not together with it.
- With D1: the TCU is commanded again at each scan step. A step is not held at the delay
  floor (`tcu.min_delay`, 1000 ns); the upper limit applies. Decided by the owner on
  2026-11-16, in place of the first form of step 4b, which sent the step through
  `update_delay()` and so held it at the floor.
- With D3: a finished scan cannot restart itself.
- A new scan begins at its first position at once. What was left of the scan before it is
  not finished and nothing more is saved into its folder (step 5b).
- With a file or a stream on the main display, the second window takes the shape of the
  picture, where it kept 16:9 and stretched the picture (step 5b).

Nothing else is meant to change. If a check shows a difference outside this list, I stop and
report it.

## 7. Decisions, taken on 2026-11-14

**D1. The TCU is not commanded during a scan: restore the command in step 4.** See finding 1
in section 8. Step 4 rewrites those lines anyway. If delay scans have worked on the hardware
since July, a path has been overlooked and the finding has to be looked at again.

**D2. The scan sequence stays in the pipeline for this sprint.** Earlier I said the choice of
the next step would move to `ScanController`. Moving 220 lines of stepping logic with no test
is the riskiest change available here. This plan keeps the logic where it is and changes only
its inputs and outputs, which is enough for the goal in section 2. The move can follow once
step 6 gives the sequence a test.

**D3. A finished scan can restart itself: in step 4 the pipeline reports the end once per
scan.** See finding 2.

**D4. Step 6 is part of this sprint.** It is the proof that the pipeline builds alone, and S-4
needs it. It adds a source file and a test target to `CMakeLists.txt`.

**D5. Name and place.** The documents of this refactor live in `others/refactor/second move/`,
next to those of the first. The sprint number continues: this is sprint 26. The patches of
section 1 are working files and stay outside the tracked folders, as does the scan report the
findings come from.

## 8. Found while reading, not changed

1. **A delay or gate-width scan does not command the TCU.** In `advance_scan_main`, the two
   lines that tell the GUI to send the new delay and gate width
   (`emit update_delay_in_thread()`, `emit update_gate_width_in_thread()`, lines 1501-1502) are
   inside an `#if 0` block that begins at line 1496. The block was meant for the alt1 side
   capture above them. It arrived with commit `6a8f93c` of 2026-07-30; the commit before it
   emitted the signal. `set_delay_dist` (line 1472) only stores a number in `TCUController`.
   What sends it is `update_delay()`, and nothing else calls that during a scan, at the start
   of a scan included (`scancontroller.cpp:109-112`). The folders are still named after the
   stepped values. I read this in the code and have not run a scan.
2. **A finished scan can restart itself.** At the end, the pipeline emits `finish_scan_signal`
   and goes on. If another frame arrives before the GUI has handled the signal, the pipeline
   emits it a second time (lines 1463-1464, and 1563 for the panorama). The slot is
   `ScanController::on_SCAN_BUTTON_clicked`, a toggle: the first call stops the scan, the
   second starts a new one. It needs a GUI that is busy for longer than one frame, so it is
   more likely at high frame rates. At the desk it is to be expected: a still image is
   processed about 76 times a second, and the end of a grid scan comes a few frames after
   the GUI was asked for the screenshot of the last profile.
3. **Stop, then start: the old position is finished first.** The frame counter of the grid
   scan survives a stop, which is what lets Continue resume. A new start does not reset it, so
   the remaining frames of the old position are counted, and may be saved into the old folder,
   before the new scan takes its first step. The panorama scan does reset. Not changed by this
   plan unless you ask. **Asked for on 2026-11-16 and changed in step 5b.**
4. **The TCU text of the frame path is built and never used.** `run()` builds
   `DELAY ... ns  GATE ... ns` (or `DIST ... m  DOV ... m`, or the custom text) for every
   frame and hands it to `render_and_display` and `record_frame`. Both discard it
   (`(void)info_tcu`); only the time is drawn. Step 2 changes where its numbers come from
   and leaves the rest.
5. **The program starts below its own delay floor.** `tcu.min_delay` is 1000 ns in the
   configuration, which is 150 m in air and 113 m in water, and the delay slider gets that
   minimum at start. The controller starts at 15 m (100 ns), set in its constructor, and
   nothing lifts it to the floor. The first touch of the slider therefore jumps to 150 m or
   more, and the start value cannot be reached again. Noticed in the run of step 2. The
   floor came with commit `f842241`; this plan does not touch it.
6. **`scan_params` shows gate width and MCP as zero.** `ScanController` passes the two as
   `double` to a `%d` in `asprintf`. The files of the reference scans read
   `gate width: 000000 ns` and `MCP: 0000`, while the notes of the same scans read 40 ns
   and 5.
7. **A delay-only scan saves one picture fewer per profile, and no screenshot.** Without
   PTZ there is no wait time, the frame counter starts at the number of images, and it is
   lowered before it is tested. With 2 images per profile, scan B holds only `2.bmp`.
8. **A scan can be paused but not closed.** The SCAN button starts a scan and, while one
   runs, pauses it. After a pause nothing runs and nothing is commanded, and CONTINUE
   resumes. There is no action that ends a paused scan: the next press of SCAN starts a new
   one, the paused one stays resumable until then, and a paused panorama never gets its
   `panorama.jpg`. Raised by the owner in the run of step 4b. It was so before this plan.
9. **The second window stretches a picture that does not come from the camera.** The
   frame path resizes the unprocessed frame to the size of that window's display, and the
   window keeps the shape it was last told. It was told only when the camera started
   grabbing, so with a file or a stream it kept 16:9: a 1608 x 1104 picture came out 22 %
   too wide. Reported by the owner after the run of step 4b. The tree at `5481571` does
   the same; steps 3 and 4 did not change it. Changed in step 5b.
10. **The FPS label of the main display lies over the TCU box.** The label is a child of
    the central area. It is placed 6 px inside the lower left corner of the main display
    and raised each time it is placed, which puts it above the floating panel in that
    corner. Named by the owner in the run of step 6 as an old issue; this plan does not
    touch the placement. It belongs to the UI redesign.
11. **The 3D bar assumes a frame of 254 rows or more.** In `gated_3d_v2` the height of one
    grey step of the bar is the row count divided by 254, in whole numbers. Below 254 rows
    it is 0, and with an odd row count the loop that draws the steps does not advance. No
    camera in use gives such a frame. Seen while reading for the test of step 6.

## 9. Not in S-3

- `paint_mutex` held for a whole frame, the recorders released outside it, and the split of
  `DisplaySlot`. That is S-4.
- Port state read across threads. S-3 only moves the pipeline's share of those reads to the
  GUI thread, where the controllers already read them.
- `QPixmap` built on frame threads (L-5).
- `cam/cam.h` including `pipeline/routing.h`, the dependency cycle, and dead code.
- `ConsumerThread` being declared in `userpanel.h`.
- Moving the scan sequence into `ScanController` (D2).

## 10. Risks

| Risk | Guard |
|---|---|
| A value in the copy is stale because a trigger is missing | The temporary check of step 1. The writer also re-reads everything on any trigger |
| A run by hand tests less than it seems to | The summary line of the temporary check: comparisons made, and how often each value moved |
| The sizes of the alt displays and of the second window have not been compared yet | Step 3 keeps the check for them, and its run puts a source on every display |
| A display gets a picture of the wrong size after a resize | The trigger that feeds `display_width` today is used for all sizes. Checked in step 3 |
| A step does not compile, since I cannot build | The steps are small, and the compiler message is enough for me to correct one |
| Scan timing shifts | One more queue hop for the PTZ command. Checked with real scans in step 4 |
| The `LVTONG` branches of the 3D code (lines 658-661, 671-674) | They are changed the same way as the live branches, but no build defines the flag, so no compiler sees them |
