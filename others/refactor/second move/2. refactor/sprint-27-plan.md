# Sprint 27 plan: a locking rule for the display state

2026-11-17. Branch `refactor-2`, at `ff40eaa` (sprint 26).

**Status: approved on 2026-11-17, with the decisions of section 7 taken as proposed. Steps
1, 2, 3 and 3b are built and checked; 3b was added on 2026-11-18 at the owner's word and
corrects finding 9. Step 4 took the frame lock away, and a white strip showed while an
edge of the window was dragged; step 4b, a limit on the drag, did not remove it. Both were
taken back on 2026-11-18 (D8): the program's code is that of step 3b again, plus a
comment. Built on 2026-11-19, and the strip is gone. The frame lock therefore stays, with
one user left in the window. One exit of step 1 ended with a fatal line that is not
explained; it has not come back in the sixteen exits read since. Step 5, the last, is
built and checked by the owner, and the tests of the final state pass. The sprint was
closed on 2026-11-19 and is committed in one commit, together with this file.** Line
numbers are those of the tree at `ff40eaa`.

The subject is finding S-4: the display state that the window and the frame threads share
has no written locking rule. The findings are listed in section 10 of
`../1. analysis/structure-design.md`. This file says what is shared today, what the rule will
be, in which order the code is brought under it, and how each step is checked.

Of the five items the scan listed under S-4, sprint 26 closed two (scan parameters and
controller state read from the frame thread). One is left out on purpose, the port state read
across threads (section 9). This sprint is about the other two, the lock held for a whole
frame and the recorders, and about writing the rule down.

## 1. Rules of this sprint

The same as in sprint 26, with one change in the first.

- A step changes which lock guards a value, or which thread owns it. No step changes what a
  stage computes.
- One concern per step. Each step builds and runs before the next one starts.
- I cannot build or run. After each step you build, run the checks listed for it, and tell me
  the result. The build has to be the whole project, tests included (Build Project), because
  every step changes code the pipeline test compiles, and most add a case. The build of the
  program alone leaves the tests old.
- I change only the files a step names. Anything else I find is reported, not fixed.
- No commit before the end: one commit for the sprint, with the message shown to you first.
  After each step you have checked, I save the whole diff as a patch file, a local working
  file outside the tracked folders.

## 2. Goal

When S-4 is done:

- Every field of `DisplaySlot` and `StreamSlot` is of one of five kinds, and the header says
  which: fixed after start-up, the window's own, the frame thread's own, shared under a named
  lock, or atomic.
- The code follows that, except for the places listed in section 9.
- No lock of a slot is held for a frame's work. A lock is held for a copy, a swap, or one call
  into a recorder, and no thread takes a second one while it holds one. `setup_mutex`, the
  busy flag of the window's loaders, is the one exception and is not part of this sprint.
- The window does not wait for a frame to finish, whatever the operator does.
- What the operator sees and what is saved stay the same, except for the points in section 6.

**Outcome, 2026-11-18.** One point was not reached. `paint_mutex` is still held for a
frame, and the window still waits for it in one place, where it lays out the main display
(`resizeEvent`). Taking it away brought a white strip onto the screen whose cause is not
known; see steps 4 and 4b, which were taken back. The other points hold.

How it is checked: the header carries the rule; the census of section 3 is repeated at the
end and shows no access outside its rule other than those of section 9; `paint_mutex` no
longer exists; the pipeline test has four more cases, three for the recorders and one that
tries every lock of a slot while a slow frame is being worked on.

## 3. What is shared today

The figures come from a census of the code at `ff40eaa`: every access to a field of
`DisplaySlot`, `StreamSlot` and `StreamMeta`, with the thread that makes it, whether it
writes, and the locks in scope. It is the screen the structure scan used, run again. It
follows direct calls only, not signals or stored function pointers, so it can miss an access.
Every line named below was then read in the code.

### 3.1 The 41 fields of `DisplaySlot`

| Kind | Fields | Count |
|---|---|---|
| Fixed after start-up, read by both sides | `id`, `widget`, `fps_calc` | 3 |
| The window's own, never touched by a frame thread | `fps_label`, `source_dead_label`, `consumer`, `setup_mutex` | 4 |
| The frame thread's own, never touched by the window | `modified_result`, `avg_state`, `ecc_state`, `hist`, `yolo`, `yolo_last_model`, `nafnet`, `tracker`, `tracker_frame_size`, `gate_tracker`, `last_route_gen` | 11 |
| Shared under the params mutex | `pcfg` | 1 |
| Shared under the ROI mutex | `user_mask` | 1 |
| Shared under `label_mutex`, with the mutex | `range_label_enabled`, `class_heights`, `label_optics` | 4 |
| Shared under `gate_mutex`, with the mutex | `gate_request`, `gate_request_pending`, `gate_settings` | 4 |
| Shared under `paint_mutex`, with the mutex | `raw_writer`, `proc_writer`, `raw_path`, `proc_path` | 5 |
| Atomic | `record_raw`, `record_proc`, `save_raw`, `save_proc`, `frame_a_3d`, `consumer_run`, `consumer_state`, `source_dead` | 8 |

So the struct is less shared than it looks. Only 15 fields, three mutexes among them, are
shared under a lock, and for `label_mutex` and `gate_mutex` every access on both sides is
already under its mutex. One exception to the third row: `UserPanel::clean` releases
`modified_result` (item 5 below).

### 3.2 Where the code breaks that picture

1. **One lock for a whole frame.** A frame thread holds `paint_mutex` from pre-processing to
   recording (`pipelineprocessor.cpp` 300-338), detection and enhancement included. The
   window takes the same mutex in 11 places:

   | Lines in `userpanel.cpp` | What the window does there |
   |---|---|
   | 3814, 3829, 6734, 6747, 7010, 7024 | opens or closes a recorder |
   | 5086, 7305, 6621 | writes a setting: 3D, pseudocolor, rotation |
   | 6669 | writes the stream's metadata before a display starts |
   | 5859 | moves the main display and its labels in `resizeEvent` |

   Each of them can wait for the frame in progress. Since sprint 26 the frame thread no
   longer reads the widget, so the last row guards nothing a frame thread uses. In the row
   before it the display's own frame thread has already been stopped. The three settings
   need another lock (item 3). **What the frame-long lock still protects is the recorders,
   and nothing else.**

   The reverse also holds: a recorder file is opened while the lock is held (3851, 6762,
   7035), so that display's frame thread stands still until the encoder is ready.

2. **The recorders are touched outside their lock.** At a route change and when a source
   has died, the frame thread tests and releases both writers and reads their paths without
   the lock (213-220, 268-275). The window's handler of `recorder_finalized` clears the path
   without it as well (2821-2822).

3. **Nine writes of per-frame settings use the wrong lock or none.** The frame thread copies
   `pcfg` and the parameters under the params mutex (285-290), before it takes
   `paint_mutex`. Of 74 writes to `pcfg`, 66 are under the params mutex, and of 21 writes to
   the parameters, 20. The rest:

   | Field | Lines | Lock there |
   |---|---|---|
   | `pcfg.image_3d`, `pcfg.enable_3d` | 5087-5088 | `paint_mutex` |
   | `pcfg.image_3d`, `pcfg.enable_3d` | 3133-3134 | none |
   | `pcfg.pseudocolor`, `pcfg.pseudocolor_enabled` | 7306-7307 | `paint_mutex` |
   | `pcfg.lower_3d_thresh` | 859, 3980 | none |
   | `m_processing_params.image_rotate` | 6622 | `paint_mutex` |

   `paint_mutex` excludes nothing here, because the copy is taken outside it. A frame can
   therefore get one half of a pair. For pseudocolor being switched on, that frame is drawn
   as colour over a one-channel picture and reads past its end by twice the display width.
   The gap is two assignments wide, so this is unlikely, and I know of no report of it.

4. **The ROI mask is replaced under a different lock.** The window draws into `user_mask`
   under the ROI mutex (793, 800) and the frame thread reads it under that mutex (701), but
   at a route change the frame thread replaces the matrix while holding only the stream's
   metadata lock (250).

5. **`clean()` trusts the wrong flag.** It releases `latest_raw` and `modified_result`
   (3924-3925) when `consumer_run` is false. That flag falls when the thread is asked to
   stop. `consumer_state` is the one that says the thread has ended.

6. **Shutdown deletes in the wrong order.** The destructor deletes each `fps_calc`
   (2942-2948) before it stops the frame threads (2950-2958). A frame thread that still gets
   frames can call into the deleted object. A still image keeps supplying frames, because
   its frame is put back into the queue each time.

7. **The stream's metadata has a lock that only one side uses.** Within one frame the frame
   thread reads `StreamMeta` under `meta_mutex` in seven separate places (410-412, 535, 776,
   827, 1456, 1676, 1714) and reads `source_type` five more times without it (370, 385, 395,
   1680, 1684), so a frame can see two states of the stream. The window writes the metadata
   in 25 places and a loader thread in one, none of them under `meta_mutex`.

8. **Three widget pointers live in the slot.** `widget`, `fps_label` and
   `source_dead_label`. The frame path uses one of them, for a null test (989).

### 3.3 What already holds

- The per-frame copies of the parameters, `pcfg` and the scan job.
- `label_mutex` and `gate_mutex`: every access on both sides is under the mutex.
- The eight atomics. `BoundedQueue` and `DisplayRouter` lock inside themselves.
- No function takes one lock of a slot while holding another, with two exceptions. The frame
  thread takes the smaller locks while it holds `paint_mutex`. And a loader in the window
  holds `setup_mutex` for the whole time it sets a slot up; that one is a busy flag, not a
  guard, and is left alone here (section 9). Once `paint_mutex` is gone, every guarding lock
  is taken alone, so there is no lock order to design.

## 4. Design

### 4.1 The rule, written into `routing.h`

The fields of `DisplaySlot` are regrouped by kind, each group under a comment that names its
owner or its lock. No field is renamed, so no access changes.

```cpp
struct DisplaySlot {
    // Fixed once the window is built. Any thread reads them.
    DisplayId   id;
    TimedQueue *fps_calc;          // owned by UserPanel, deleted after the last consumer stops

    // The window's own. No frame thread touches them.
    ConsumerThread *consumer;
    QMutex          setup_mutex;   // held by a loader while it sets up this slot

    // The frame thread's own. The window touches them only when this display has no
    // consumer thread, which consumer_state tells.
    cv::Mat modified_result;
    FrameAverageRuntime avg_state;
    ECCRuntime          ecc_state;
    // ... hist, yolo, yolo_last_model, nafnet, tracker, tracker_frame_size,
    //     gate_tracker, last_route_gen

    // Shared. Each group names its lock. A lock is held for a copy, a swap or one call
    // into a recorder, never across a stage, and never together with another one.
    PipelineConfig pcfg;           // params mutex; the frame thread copies it once per frame
    cv::Mat        user_mask;      // ROI mutex
    QMutex label_mutex;            // the three fields below
    QMutex gate_mutex;             // the three fields below
    QMutex recorder_mutex;         // the two writers and their paths

    // Atomic.
    // ... record_raw, record_proc, save_raw, save_proc, frame_a_3d, consumer_run,
    //     consumer_state, source_dead
};
```

`StreamSlot` gets the same kind of comment: `meta` and `latest_raw` under `meta_mutex`, the
two queues locking inside themselves, `producer_run` atomic.

### 4.2 One lock for the recorders

A new `recorder_mutex` in the slot guards `raw_writer`, `proc_writer`, `raw_path` and
`proc_path`.

- The window's six recorder places take it in place of `paint_mutex`. The handler of
  `recorder_finalized` takes it to clear the path.
- The two release blocks of `run()` become one function. It releases what is open and
  copies the path while it holds the lock, and emits the signal after it has let go:

  ```cpp
  void PipelineProcessor::finalize_recorders(DisplaySlot &display)
  {
      QString raw_path, proc_path;
      bool raw_closed = false, proc_closed = false;
      {
          QMutexLocker lk(&display.recorder_mutex);
          if (display.raw_writer.isOpened())  { display.raw_writer.release();  raw_path  = display.raw_path;  raw_closed  = true; }
          if (display.proc_writer.isOpened()) { display.proc_writer.release(); proc_path = display.proc_path; proc_closed = true; }
      }
      if (raw_closed)  emit recorder_finalized(static_cast<int>(display.id), 0, raw_path);
      if (proc_closed) emit recorder_finalized(static_cast<int>(display.id), 1, proc_path);
  }
  ```

  The path stays in the slot, and the window clears it, as today. If the frame thread
  cleared it, a Stop pressed between the release and the signal would find no path and the
  file would stay in the temporary folder.
- `record_frame` builds the picture first and takes the lock only around the test and the
  write of that one frame. A display that is not recording never takes it: the two record
  flags are tested first, as today.

### 4.3 A frame takes the stream's metadata once

`acquire_frame` copies `StreamMeta` under `meta_mutex` once, as soon as it has a frame or
has given up waiting for one. The stages get that copy as `const StreamMeta &meta`, and
their own reads of `stream.meta` go (the twelve places of item 7). The three values the loop
keeps from the last route change, `w`, `h` and `pixel_depth`, stay as they are.

On the window's side only one block changes: `start_static_display` writes the metadata
under the stream's `meta_mutex` in place of the display's `paint_mutex` (6669-6682). The
other writes stay for now (section 9).

### 4.4 The frame lock goes

After 4.2 and 4.3 nothing in the window takes `paint_mutex` any more:

| Place | Today | After | Step |
|---|---|---|---|
| 5086 3D, 7305 pseudocolor, 6621 rotation | `paint_mutex` | params mutex | 1 |
| 3814, 3829, 6734, 6747, 7010, 7024 recorders | `paint_mutex` | `recorder_mutex` | 2 |
| 6669 metadata before a display starts | `paint_mutex` | `meta_mutex` | 3 |
| 5859 `resizeEvent` | `paint_mutex` | no lock | 4 |
| `run()` 300-338, the frame | `paint_mutex` | no lock | 4 |

The lock is removed last, in a step of its own, and the census is repeated just before, so
that anything still leaning on it shows up first.

Step 4 was taken back on 2026-11-18, so the last two rows of the table did not happen:
`resizeEvent` and `run()` still take `paint_mutex`. The census had found nothing that
leans on the lock, and nothing in the data does; what leaned on it was the look of the
window while it is resized.

### 4.5 The widget pointers leave the slot

`widget`, `fps_label` and `source_dead_label` move into a small struct that `UserPanel`
keeps per display. That changes 41 accesses in `userpanel.cpp`, all of the same form. The
null test in the frame path goes: every display gets its widget in the constructor
(557-559) and keeps it, so the test is always true in the program. The forward declarations of
`Display` and `QLabel` leave `routing.h`, and the pipeline test no longer needs its stand-in
pointer.

### 4.6 Tests

| Case | Input | Expected | Step |
|---|---|---|---|
| TC-PL-006 | a recorder open on a display with frames flowing, then a route change | the writer is closed, `recorder_finalized` arrives once with the path, the file holds frames | 2 |
| TC-PL-007 | a second thread opens and closes a recorder under `recorder_mutex` while frames flow and the route flips | no deadlock within the time limit, every file that was opened is closed | 2 |
| TC-PL-008 | every frame made to take 80 ms, and the test taking in turn each lock the window shares with the frame thread: the params mutex, the ROI mutex, `label_mutex`, `gate_mutex`, `recorder_mutex`, `meta_mutex` | no lock keeps the test waiting for 40 ms, half a frame; the frame thread goes on working | 4 |
| TC-PL-009 | a recorder open, the frame thread held inside a frame, then the source ended as the window ends it: `source_dead` set first, the queue closed after | the writer is closed, `recorder_finalized` arrives once with the path, the file holds the frames | 3b |

TC-PL-007 cannot prove the absence of a race. It is there to catch a deadlock or a crash
that a change to these paths would bring. The five cases of sprint 26 stay and have to pass
after every step.

TC-PL-009 was added with step 3b. It holds the frame thread by taking the recorder's mutex
before it pushes a frame: the thread then stops where it comes to write that frame. That is
the one order of events in which the code before step 3b left the recording open, so the
case is meant to fail on that code. I have not seen it fail there, because I cannot build.

TC-PL-008 was first planned with a frame that is slow because it is large, a limit of 5 ms
and a second thread. As written with step 4, the test makes the frame slow itself: its slot
for the picture runs on the frame thread, inside the frame, and waits there for 80 ms. The
result then does not depend on how fast the machine is, and the limit, half of that wait,
leaves room for the scheduler. The test thread takes the locks itself, as the window would.
The case does not look at `paint_mutex`, which is back since step 4 was taken back. It is
there so that none of the other six locks comes to be held for a frame's work.

## 5. Steps

For every step: you build the whole project and do the run described. I then read the log,
check the saved files against their source frame, and run `test_pipeline` and the older
tests myself.

### Step 1. The rule, and the fields that are one line away from it

- `routing.h`: the regrouping and the comments of 4.1. `recorder_mutex` is not added yet.
- The nine writes of item 3 go under the params mutex, each in a block of its own.
- The frame thread replaces `user_mask` under the ROI mutex (item 4).
- `clean()` also tests `consumer_state` (item 5).
- The destructor stops the frame threads before it deletes `fps_calc` (item 6).

Files: `pipeline/routing.h`, `pipeline/pipelineprocessor.cpp`, `visual/userpanel.cpp`.

You check, with a still image: the five reference saves, which switch pseudocolor and 3D;
the image turned once and turned back; a rectangle drawn on the picture and another image
loaded after it; CLS pressed with a source running and without one; the program closed
while the image is still shown.

**Written on 2026-11-17.** Three files, 96 lines added and 72 removed, most of them the
moved fields of `DisplaySlot`.

- `routing.h`: the struct holds the same 41 fields with the same types and start values,
  compared by name before and after. `paint_mutex` stays for now and its group says what
  it guards today. `StreamSlot` gets a comment and no other change.
- The nine writes are each in a block of their own under the params mutex. The window
  holds that mutex in eight other places. While holding it they only assign, or read
  values off widgets and controllers, so none of them can reach these handlers and meet
  its own lock.
- The frame thread now lets go of the metadata lock before it takes the ROI mutex for the
  mask, so the two are not held together.
- The census, repeated on the changed tree: all 74 writes to `pcfg` and all 21 to the
  parameters are under the params mutex; all four accesses to `user_mask` are under the
  ROI mutex; the window takes `paint_mutex` in 8 places, down from 11.
- Not compiled. The three files pass the structure check I can run here.

**Result, 2026-11-17, with one point open.** Built at 17:40, the whole project. One run.

- The five saves equal those of step 3 of sprint 26, which came from the same frame, in
  every pixel outside the time stamp.
- The log shows the rotation (the display region turns to 424 x 618 and back) and the
  second image. No warning.
- `test_pipeline` passes, 30 runs of 30. So do the six older tests that need no window.
- **Open: the run ended with a fatal line,** `QThread: Destroyed while thread is still
  running`, 165 ms after the destructor of the window began. Windows recorded the abort.
  It is the first such line in the 28 logs of the build folder, and it came on the first
  run after this step changed the order of shutdown, so it is not waved off as chance.
  What reading the code gives:
  - The frame threads and the FPS counters, which this step reordered, are each waited
    for without a time limit before they are deleted. Neither can be destroyed while it
    runs, in the old order or the new.
  - The seven port threads are the only ones waited for with a limit, 100 ms each, and
    their thread objects are destroyed right after (finding 7 in section 8). One port
    thread that misses its limit gives exactly this line, and the 165 ms fit one such
    wait plus the rest of the shutdown.
  - Nothing in this step touches a port or its thread.

  That makes the port shutdown the likely source and this step an unlikely cause, but it
  does not say why a port thread was slow in this run and never in 27 before. Until a few
  more exits have been looked at, step 2 is not started.
- **The exits looked at, 2026-11-18.** The owner started and closed the same build four
  times, three with a still image and one with a video playing. All four logs end as
  usual, without the line. So it does not come with every exit, and step 1 does not bring
  it by itself. It stays unexplained: one case in five exits of this build, none in the
  27 before. Every later log of this sprint is read for it. If it shows again, the old
  order of the shutdown is put back first, to see whether it then stays away. The logs
  counted here were deleted by the owner later that day to free space; the numbers above
  are what was read before.

### Step 2. One lock for the recorders

4.2, with TC-PL-006 and TC-PL-007. The frame thread still holds `paint_mutex` for the frame
in this step, so nothing else moves.

Files: `pipeline/routing.h`, `pipeline/pipelineprocessor.h` and `.cpp`,
`visual/userpanel.cpp`, `tests/test_pipeline.cpp`.

You check, with a video file: ORI and RES recorded on the main display, started and
stopped; a recording on an alt display; a recording left running while the main display is
swapped with an alt display; a recording left running until the video ends. Each file
should arrive in the save folder and play.

**Written on 2026-11-18.** Five files.

- `routing.h`: `recorder_mutex` heads the group of the two writers and their paths.
  `paint_mutex` keeps a line of its own that says what is left of it.
- `pipelineprocessor`: the two release blocks of `run()` are one call to the new
  `finalize_recorders`, and `record_frame` takes the mutex around the test and the write
  of each recorder.
- `userpanel.cpp`: the six recorder places and the handler of `recorder_finalized` take
  `recorder_mutex`. The one place that locked and unlocked by hand uses a locker now.
- The frame thread still holds `paint_mutex` while it records, so it takes the new mutex
  inside it. The window takes the new mutex alone at all seven places, so the two orders
  cannot cross.
- The census, repeated: every access to the four recorder fields is under
  `recorder_mutex`, 20 in the window and 10 in the frame thread. The window is left with
  two places that take `paint_mutex`.
- Tests: TC-PL-006 and TC-PL-007 as in 4.6. Both use Motion JPEG in an AVI file, which
  OpenCV writes and reads without an installed codec. TC-PL-006 expects the four frames
  pushed while recording to be in the file.
- Not compiled. I have never run the two new cases; if one of them fails, it may be the
  case and not the code.

**Result so far, 2026-11-18.** Built at 12:13: the program only. The test program is still
the one of 17 November, so the two new cases have not been compiled or run.

- The run ended without the fatal line.
- A recording on ALT 1 was started and stopped: the writer opened, in 0.3 s, and the file
  arrived in the save folder.
- On the main display the buttons under CAPTURE were used, not those under RECORD. The
  log has no line of a main recorder being opened and the save folder no main recording,
  but 142 and then 186 pictures saved in a row. So the recorders of the main display, a
  swap during a recording and the end of a video during a recording are still unchecked.
  My instruction said "record" and did not name the row; that was unclear.
- The owner found that checks 3 and 4 "did not end by themselves". With CAPTURE that is
  how the program behaves, before this step as well: saving pictures in a row is not a
  recording, and neither a swap nor the end of a video stops it.
- For the end of a video I had promised more than the program does, with RECORD as well:
  whether a recording then ends by itself depends on timing, see finding 9 in section 8.
  The check is corrected below.

Added on 2026-11-18 with the owner's agreement, outside the files of this step:
`controller/devicemanager.cpp` writes a warning that names a port thread which is still
running after its 100 ms (finding 7). The waits themselves are unchanged. It is there to
show, the next time the fatal line of step 1 appears, whether a port thread is behind it;
my reading says so, but that is an inference and not an observation.

**Built again at 12:25, the whole project.** The two new cases compiled as written.

- TC-PL-007 passes, 20 runs of 20. The five cases of sprint 26 pass, and so do the six
  older tests that need no window.
- TC-PL-006 failed in all 20 runs, at its last check: the file could not be read back. The
  fault was in the case. It opened its recorder right after starting the frame thread, and
  the frame thread closes the recorders when it meets its route for the first time, so the
  recorder was closed with no frame in it, and a Motion JPEG file without a frame is left
  unfinished by OpenCV. The checks before the last one passed for the wrong reason: the
  one announcement they count came from that first closing. The case now sends a frame
  through before it opens the recorder, and checks that nothing is announced before the
  route change. It has to be built once more.

The checks again, with the row named. Build the whole project, then with a video file on
the main display:

1. RECORD, ORI: start, a few seconds, stop. The same with RECORD, RES. Two files arrive.
2. RECORD, RES, then swap the main display with an alt display: the recording ends by
   itself, the button reads RES again and the file arrives.
3. RECORD, ORI, and let the video run to its end: the recording may end by itself or stay
   on, depending on timing (finding 9). If the button still reads STOP, press it. Either
   way one file arrives.

**Result, 2026-11-18.** The whole project was built once more at 12:32, which compiled the
corrected case; the program is the one of 12:25. One run at 12:34 with three video files,
all on the main display.

- RECORD, ORI and RECORD, RES, each stopped after six seconds: both files arrived.
- RECORD, ORI, then the main display swapped with an alt display: the recording ended by
  itself, as the owner saw. The log shows that file closed and moved 3.5 s after it was
  opened; it does not say what closed it.
- RECORD, ORI with the video running to its end, twice: the recording stayed on both
  times. The first was closed with STOP eight seconds after the end, the second by closing
  the program, which stops the recorders of the main display on its way out. Both files
  arrived. This is finding 9. The test that decides it stood in the code before this
  step and is unchanged, so the step did not bring it.
- Five recordings in the save folder, 0.3 to 3.9 MB, and none left in the temporary
  folder. I looked at names and sizes only; whether they play is not checked.
- The log has no warning and ends without the fatal line. No port thread is reported as
  still running; three control ports were connected in this run.
- `test_pipeline`: 10 passed, 30 runs of 30. Nine older test programs that need no window
  pass, 120 cases. The builds of this sprint did not rebuild any of them, so none compiles
  a file the sprint changes, and their passing says nothing about it.

### Step 3. A frame takes the stream's metadata once

4.3. Pipeline files, and the one block of `start_static_display`.

Files: `pipeline/pipelineprocessor.h` and `.cpp`, `visual/userpanel.cpp`, and two comments
in `pipeline/routing.h`. `tests/test_pipeline.cpp` does not change: the signatures of the
stages do not reach the test, and no case needs the copy.

You check: the five reference saves; a colour picture or video on the main display; scan A
of sprint 26, which uses the stream's frame rate.

**Written on 2026-11-18.** Four files, 70 lines added and 63 removed.

- `acquire_frame` copies `StreamMeta` under `meta_mutex`, in the branch that got a frame
  and in the one that gave up waiting. `run()` holds the copy for the frame and hands it
  to the stages as `const StreamMeta &meta`.
- The twelve reads of item 7 now read the copy: `preprocess_frame`,
  `frame_average_and_3d`, `enhance_frame`, `render_and_display`, `advance_scan_main`,
  `record_frame` and `save_to_file`, and the three reads of `source_type` in
  `acquire_frame` itself. Seven of them took `meta_mutex` each time; those lock takes
  are gone.
- `enhance_frame` used the stream for that read only, so it no longer gets it.
- `start_static_display` writes width, height, colour, depth and source type under the
  stream's `meta_mutex`. The two title labels and `m_device_type` are set after the lock
  is let go. The call to `update_pixel_depth` is replaced there by its two statements,
  the write inside the lock and the label outside.
- `routing.h`, comments only, which the list of files above did not name at first: the
  line about `paint_mutex` no longer says that the window takes it to set up a stream,
  and the head of `StreamSlot` says that a frame thread copies the metadata once per
  frame.
- The census, repeated: the frame thread reads the shared metadata in six places, all
  under `meta_mutex`, the four fields at a route change and the two copies. Before the
  step there were 19, five of them without the lock. The window is left with one place
  that takes `paint_mutex`, `resizeEvent`.
- No new test. The cases there are run every stage that reads the copy except
  `save_to_file`, all with a grey 8-bit stream. The five reference saves run
  `save_to_file`, and a colour stream is in your checks.
- The block of `start_static_display` matters when another display's frame thread is
  still in a frame of the stream being set up, which a swap makes possible. That moment
  cannot be hit by hand, so no check is listed for it; the block is right by the lock it
  takes, and every load of a file runs it.
- Not compiled. The four files pass the structure check I can run here.

**Result, 2026-11-18, with the scan still to do.** Built at 16:05, the whole project; it
compiled as written. One run at 16:07.

- The five saves pass every check against their source frame, and agree with the set of
  step 0 in everything that does not depend on the frame.
- A colour video, 1280 x 720, played on the main display from start to end. No warning.
- **The scan did not run.** SCAN was pressed 35 s after that video had ended, with no
  other source loaded. A display whose source has ended gets no frames, and a scan
  advances on frames, so it never took its first step: the scan folder holds
  `scan_params` and nothing else (finding 11). The settings were also not those of scan
  A, 2 delay values of step 1000 ns where scan A has 3 of step 500 ns. The one line of
  this step that a scan uses, the frame rate read from the copy, is run by the two scan
  cases of the pipeline test, which pass. The scan at the desk was repeated with the next
  build, with the still image loaded first; its result is under step 3b.
- The log ends without the fatal line and without a port thread reported. Three control
  ports were connected.
- `test_pipeline`: 10 passed, 31 runs of 31.

### Step 3b. A source that ends closes its recorders

Added on 2026-11-18. The owner saw finding 9 in the run of step 2 and asked for it to be
corrected in this sprint. It fits the subject: the fault is a flag with two writers.

The frame thread gets a variable of its own, `death_seen`, a local of `run()`. It is set
when the thread closes the recorders at a closed and empty queue, and cleared on every
pass that finds the queue alive. `source_dead` goes on as the state of the label, written
by both sides as before; the frame thread no longer reads it to decide about the
recorders.

Files: `pipeline/pipelineprocessor.cpp`, `tests/test_pipeline.cpp`.

You check, after a build of the whole project:

1. A video on the main display. RECORD, ORI, and let the video run to its end: the
   recording ends by itself, the button reads ORI again, and the file is in the save
   folder. The same with RECORD, RES.
2. RECORD, ORI, and STOP a few seconds later, while the video still plays: as before.
3. Scan A, left over from step 3. Load the still image first, so that the main display
   has a source that delivers frames. Then, as in sprint 26: grid with delay and PTZ,
   delay from 2000 ns to 3000 ns in 3 values (step 500 ns), PTZ 2 by 2, INFO off, capture
   of ORI and RES on, 2 images per position, wait time 1 s.

Not in the checks: a recording on an alt display at the end of its video. It runs the
same lines of the frame thread; what differs is the window's handler, which is unchanged.

**Written on 2026-11-18.** Two files, 88 lines added and 4 removed, 11 and 4 of them in
the pipeline.

- A recording that the operator starts on a source that has already ended stays as it
  is: it is opened, gets no frame, and waits for STOP.
- The label's side of the flag is left alone. A pass that gets past the queue check still
  clears `source_dead`, whether or not it got a frame, so at the end of a video the label
  can go off and on once within a few hundredths of a second. That is as before.
- TC-PL-009, see 4.6. The test rig got one helper, which waits until the frame thread
  stands still.
- Not compiled. Both files pass the structure check I can run here.

**Result, 2026-11-18.** Built at 16:23, the whole project; it compiled as written. One run
at 16:27.

- RECORD, ORI with the video running to its end: the file was closed and moved 21 ms
  after the video ended, with no hand on it.
- RECORD, ORI stopped with STOP after six seconds, while the video still played: the file
  arrived, as before.
- RECORD, RES to the end of a video was not run. TC-PL-009 records with that writer, and
  one call closes both writers.
- Both recordings are in the save folder, 15.5 and 8.0 MB, and none is left in the
  temporary folder. Names and sizes only.
- The log ends without the fatal line and without a port thread reported. Three control
  ports were connected.
- `test_pipeline`: 11 passed, 31 runs of 31. TC-PL-009 passed in the first run it ever
  had.

**The scan left over from step 3** ran in this run, on the still image. It was a grid of
4 positions with 2 delay values, 2000 and 3000 ns, where scan A has three; ORI pictures
only, and no notes in them.

- 29 files: 16 pictures, every one equal to the source frame, one screenshot per profile,
  and the `params` of the four positions equal to those of the reference scan.
- Each profile took 26 passes of the frame thread. That is the count the step works out
  from the copy: the stream's frame rate, 24, times the wait time of 1 s, plus the 2
  pictures. So the frame rate reaches the scan through the copy as it did before.
- The frame rate was 24 because a video of that rate had played on the stream before the
  image was loaded (finding 12). With a still image the frame thread makes about 130
  passes a second, so a wait time counted in frames is over in a fraction of it. That is
  as before, and matters at the desk only.

### Step 4. The frame lock goes

4.4, with TC-PL-008. `resizeEvent` and `run()` stop taking `paint_mutex`, and the field is
removed.

Files: `pipeline/routing.h`, `pipeline/pipelineprocessor.cpp`, `visual/userpanel.cpp`,
`tests/test_pipeline.cpp`.

You check: before building this step, drag the window's edge for a few seconds while a slow
enhancement runs on a still image, to know how it feels today. After the build, the same:
the window should follow the mouse without waiting for frames. Then the five reference
saves, a recording, and scan A, with sources on all three displays.

**Written on 2026-11-18.** Four files, as listed.

- The census, repeated on the tree of step 3b before anything was changed: under
  `paint_mutex` the window reads `fps_label`, `source_dead_label` and the atomic
  `source_dead`, all in `resizeEvent`, and nothing else. So nothing leans on the lock.
- `run()` no longer takes a lock for the frame. Its stages take theirs one at a time, each
  for a copy or for one call into a recorder.
- `resizeEvent` lays out the main display without a lock. The frame thread does not read
  the widget: it gets the display's size and region from its per-frame copy, which
  `update_processing_params()` publishes at the end of `resizeEvent`. A frame that
  started just before works with the old size once; that was so with the lock as well,
  because the copy is taken before the lock.
- `routing.h`: the field and its comment are gone, and the head of `DisplaySlot` states
  the rule without an exception. The struct has 41 fields again.
- The census after it: no function of the pipeline takes a mutex while another is in
  scope. The mutexes of a slot are taken in 3, 2 and 10 places (`gate_mutex`,
  `label_mutex`, `recorder_mutex`), `meta_mutex` in 6, and `setup_mutex` by the loaders
  only.
- One mutex is still held for long: `yolo_init_mutex`, while a detection model is loaded.
  It is not a lock of a slot. Only frame threads take it, so that two displays do not
  load a model at the same time, and the window never waits for it.
- Two lines of `UserPanel::screenshot`, commented out long ago, still name
  `paint_mutex`. They are left as they are; the name occurs nowhere else in the sources.
- TC-PL-008, see 4.6. The test rig got two accessors and a switch that makes every frame
  slow.
- Not compiled. The four files pass the structure check I can run here.

The checks, in the order to do them. The first is done with the program as it is built
now, before this step is built.

1. Before the build: the still image on the main display, enhancement switched on with
   DCP chosen, which should bring the frame rate down a long way; if it does not, AINDANE.
   Drag the edge of the window for a few seconds and see how it follows.
2. Build the whole project. The same again: the window should now follow the mouse.
   Then switch the enhancement off.
3. The five reference saves.
4. A video on the main display: RECORD, ORI for a few seconds, then STOP.
5. The still image on the main display again, a video on ALT 1 and one on ALT 2, so that
   all three displays show something. Then the scan as in the run of step 3b, with its
   settings unchanged: 4 positions, 2 delay values.

**Result so far, 2026-11-18, with one difference outside section 6.** The drag of check 1
was done with the build of step 3b in a run at 16:54. The whole project was then built at
16:55; it compiled as written. One run of the new build at 16:58.

- The drag, from the size lines in the two logs. The old build with AINDANE took a size
  step every 116 ms, one per frame, about 9 a second; with the plain picture one every
  30 ms. The new build takes one every 10 to 12 ms, about 90 a second, whatever a frame
  takes. The window no longer waits for frames.
- **Not foreseen: a white strip at the edge that is being dragged.** The owner saw none
  with the old build and sees one with the new. The theme is dark and no widget of the
  window is white, so the strip is window area that has not been painted yet. Qt registers
  its window classes on Windows with a white background, and Windows shows that
  background in the area a window has just gained until Qt has painted it. This is known
  for Qt on Windows and shows most with a dark theme (QTBUG-89688). Every size step has
  such a moment. At 9 steps a second the strip is hardly ever on the screen, at 90 it is
  there most of the time. The frame lock had slowed the drag down as a side effect, and
  that hid it. The pictures, the saves and the recordings are not touched by this.
  **Open with the owner: what to do about it,** see below.
- The five saves: four of the five checks pass. The second save was taken with AINDANE
  still chosen from the drag, not Histogram: its grey values are not one output per input
  value, as Histogram gives. There is no reference for AINDANE.
- RECORD, RES on the still image, stopped after five seconds, and RECORD, ORI on a video,
  which was closed 19 ms after the video ended. Both files arrived, and none is left in
  the temporary folder.
- Check 5 could not be done as written, and that is my fault: a file cannot be loaded
  onto an alt display, and I had not read that side before I wrote the check. The owner
  loaded the videos on the main display and swapped them to the alt displays; loading the
  next file then stopped the video that had been swapped away (finding 13). Three live
  displays need cameras or network streams, so that check goes to the hardware. No scan
  was run.
- Both runs ended without the fatal line and without a port thread reported.
- `test_pipeline`: 12 passed, 31 runs of 31. TC-PL-008 passed in the first run it ever
  had.

The white strip, what can be done:

- **A. One size step per 30 ms while an edge is dragged.** A limit in the window's own
  drag handler, `mouseMoveEvent`. 30 ms is what the old build did with a plain picture, so
  the drag would look as it did in everyday use, and it would still not wait for frames:
  with AINDANE 33 steps a second where the old build made 9.
- **B. A dark background for the unpainted area.** The window class gets a background in
  the colour of the theme, so that the strip is dark on dark and does not show at any
  rate. It needs a call into the Windows API and has to follow the theme.
- **C. Leave it.** The window follows the mouse, and the strip stays.

Proposed: A, because it brings back what was there. B can come with the UI redesign.
The owner chose A on 2026-11-18 (D7).

### Step 4b. One size step per 30 ms while an edge is dragged

Added on 2026-11-18 for the white strip that the run of step 4 showed. It changes the
window's own drag handler and nothing in the frame path.

- `mouseMoveEvent` takes a size step at once if the last one is 30 ms or more ago. A move
  that comes sooner is held back, and a timer takes one step to wherever the mouse is when
  the 30 ms are over. So the steps are at least 30 ms apart, and no move is lost.
- The first move after a press is never held back. A step still held back when the button
  is let go is taken at once, so the edge ends where the mouse was let go.
- The arithmetic of the seven drag places is unchanged. It moved, as it was, into a
  function of its own, `drag_resize`, because the timer has to run it too.
- Moving the window by its title bar is not limited: it changes no size.

Files: `visual/userpanel.h` and `.cpp`.

You check, after a build:

1. The still image on the main display, enhancement on with AINDANE. Drag the right edge,
   the bottom edge and a corner, as before. The white strip should be gone, or as rare as
   it was with the old build, and the window should follow the mouse more smoothly than
   the old build did with AINDANE.
2. The same with the enhancement off.
3. Drag an edge fast and let go while it still moves: the edge should end under the
   cursor. Then the left edge and the top edge, which move the window's corner as well.
4. Left over from step 4: the five reference saves, with Histogram chosen for the second,
   and the scan as in the run of step 3b.

**Written on 2026-11-18.** Two files, 86 lines added and 48 removed, most of them the
moved arithmetic.

- 30 ms is the spacing the old build had with a plain picture, measured from its log.
  Whether the strip is gone at that spacing is for the eye to say; I cannot see the screen.
- Not compiled. Both files pass the structure check I can run here.

**Result, 2026-11-18: the strip is still there.** Built at 18:02; it compiled as written.
One run at 18:03.

- The limit works as written: in the two drags of the run the size steps are 32 ms apart
  at the median, 31 a second.
- The owner still sees the white strip. So the pace of the drag is not what kept the strip
  away in the old build, and my explanation in the result of step 4 was wrong in that
  point. What kept it away was the lock itself. Why a lock between the frame thread and
  the layout of the main display decides whether unpainted window area shows, I do not
  know.
- The five saves pass every check. The scan agrees with that of the run of step 3b in
  every file. Its profiles took 98 passes each: the frame rate of 24 times the wait time
  of 4 s, which is the value after a start of the program, plus the 2 pictures.
- The run ended without the fatal line.

### Steps 4 and 4b are taken back

2026-11-18. After that run the owner asked whether the old behaviour had been put back or
new code written. It was new code, and it did not do what the old code did. The old code
is back:

- `pipelineprocessor.cpp`, `routing.h`, `userpanel.cpp` and `userpanel.h` are again as
  they were in the build of step 3b, in which the owner saw no strip. They were compared
  with that state byte for byte. The one difference is a comment at `paint_mutex` that
  says why it is there.
- So `paint_mutex` is held by the frame thread for a whole frame again, and `resizeEvent`
  waits for it. The drag limit of step 4b is gone.
- TC-PL-008 stays. It never looked at `paint_mutex`; it keeps the six other locks from
  being held for a frame.
- What steps 1 to 3b moved off the frame lock stays moved: recorders, settings and
  metadata. Of the eleven places where the window took that lock, `resizeEvent` is the
  one that is left.

What is not known is why the strip shows without the lock. Three states were seen on the
screen. Without the lock, at 90 size steps a second: a strip. Without the lock, at 31 a
second: a strip. With the lock, at 9 a second: none; the same build also made drags at 33
a second in that run, and no strip was reported for them. Two things have to be found out
before the lock is tried again: where the strip lies, at the outer edge of the window or
inside it, and whether a dark background for the window class hides it (B above).

You check, after a build of the whole project: the drag with AINDANE, as in check 1 of
step 4. There should be no strip, as before step 4.

**Result, 2026-11-19.** Built at 15:02, the program only. One run.

- The strip is gone, the owner reports.
- The log shows the pace of the old build again: with AINDANE one size step every 118 ms.
- The owner saw something else in this run. While the window is made smaller from its
  bottom right corner, the widgets at its right and bottom are cut off, and they come
  back when the mouse rests. This is the other face of the lock, and it was so before the
  sprint (finding 15): `resizeEvent` and the three mouse handlers are the code of
  `ff40eaa` character for character, and the frame thread holds the lock as it did then.
- The tests were not rebuilt by that build. They are run with the build of step 5.

### Step 5. The widget pointers leave the slot

4.5.

Files: `pipeline/routing.h`, `pipeline/pipelineprocessor.cpp`, `visual/userpanel.h` and
`.cpp`, `tests/test_pipeline.cpp`.

You check: pictures on all three displays and their thumbnails; the FPS label of each
display in its place; the label of a dead source, shown by letting a video end; the five
reference saves.

**Written on 2026-11-19.** Five files, 50 lines added and 52 removed.

- `UserPanel` keeps the three pointers of each display in a small struct of its own,
  `DisplayWidgets`, one per display. The 41 accesses in `userpanel.cpp` changed form and
  nothing else: 35 by a pattern, of the kind `m_router.get_display(x).widget` to
  `m_display_widgets[x].widget`, and 6 by hand in three functions that had the slot in a
  local name. Every changed line was read afterwards.
- `DisplaySlot` has 39 fields. The forward declarations of `Display` and `QLabel` left
  `routing.h`; no file of the pipeline names a widget class any more.
- The frame path sends the picture of a display without asking whether it has a widget.
  Every display gets its widget when the window is built, so the answer was always yes.
- The pipeline test lost its stand-in pointer.
- Not compiled. The five files pass the structure check I can run here.

The checks, as they can be done at the desk, where a file can only be loaded on the main
display:

1. Build the whole project.
2. The still image on the main display: the picture, and the FPS label in the lower left
   corner of the picture. Resize the window once: the label stays in its corner.
3. A video on the main display, then swap it to ALT 1, and back, and to ALT 2: each
   display shows the video with its own FPS label.
4. Let the video run to its end: the label of a dead source appears in the lower right
   corner of the display that shows it.
5. The small pictures of the displays in the setup panel and in the scan panel still
   follow.
6. The five reference saves.

**Result, 2026-11-19, with the tests still to be built.** Built at 15:42, the program
only. One run, and a short second one.

- The owner reports the picture and the FPS label of the main display, the swaps, the
  small pictures in the two panels and the label of an ended stream on the main display
  as fine. The five saves pass every check.
- The owner missed the FPS label and the label of an ended stream on the alt displays.
  They are not shown where the owner looked, and they were not before this step. The side
  panel shows small copies of the alt pictures and nothing else
  (`SideDisplayWidget::setup`). The two labels of an alt display are children of the
  display widget in that display's own window, which Alt+1 and Alt+2 open (the
  constructor of `UserPanel`, `keyPressEvent`). My checks 3 and 4 promised the labels
  without saying where; the fault is in the checks (finding 16).
- That the step changed nothing in the window is shown this way: with the new form of the
  41 accesses turned back by a pattern, `userpanel.cpp` equals the file before the step
  except in the three functions that were edited by hand, and there the same objects are
  read and written as before.
- Each display counts its own frames and writes the rate to its own label. So the label
  of the main display goes on working after a swap, for whatever the main display then
  shows.
- Both runs ended without the fatal line.
- The test program was not rebuilt by this build, nor by the one of the same day before
  it: it is still that of step 4. The tests of the final state are still to be run.
- Later the same day the owner opened the window of an alt display: both labels are
  there. That run also ended without the fatal line.
- The whole project was built at 16:14. On that build, which is the final state,
  `test_pipeline` passes with its nine cases, 31 runs of 31, and the nine older test
  programs pass.

The census at the end, on the tree of step 5. `DisplaySlot` has 39 fields. All 74 writes
to `pcfg` and all 21 to the parameters are under the params mutex. No access to the two
writers and their paths, and none to `user_mask`, is outside its lock. The frame thread
reads the shared metadata in six places, all under `meta_mutex`. The frame thread takes
its other locks while it holds `paint_mutex`, as before the sprint, because step 4 was
taken back; apart from that no function takes a mutex of a slot while another is in
scope.

## 6. Behaviour that changes

- The window no longer waits for the frame in progress when a recording is started or
  stopped, when 3D, pseudocolor or rotation is switched, or when a file is loaded.
  Starting or stopping a recording can still wait for one frame being written to that
  recorder.
- A display no longer stops showing frames while a recorder file is being opened, unless
  that display is already recording.
- A frame uses one state of the stream's metadata. Today its stages can see two.
- 3D, pseudocolor, rotation and the lower 3D threshold reach a frame as whole values, never
  as one half of a pair.
- `clean()` leaves the last frame alone until the frame thread has ended.
- At exit the frame threads stop before the FPS counters are deleted.
- A recording ends when its source ends: a video that reaches its end, a stream that
  drops, a source that is stopped. The file is closed and moved, and the button goes back.
  Until step 3b this depended on timing (finding 9).
- Not changed after all: resizing the window. It waits for the frame in progress, as
  before the sprint. Steps 4 and 4b, which changed that, were taken back because of a
  white strip at the dragged edge.

Nothing else is meant to change. If a check shows a difference outside this list, I stop and
report it.

## 7. Decisions, taken on 2026-11-17

All five were taken as proposed.

**D1. The sprint number.** I take this to be sprint 27.

**D2. Step 3 is part of the sprint.** The metadata copy is not needed for removing the frame
lock. I propose it because it is small, stays inside the pipeline, and without it the
rule for `StreamMeta` cannot be stated in one line. The sprint also works without it.

**D3. The window's other writes of the metadata stay as they are.** 25 places on the GUI
thread, in the code that starts, stops and configures sources. M-1 moves that code into a
controller, and bringing it under `meta_mutex` belongs there. Until then a frame's copy can
mix an old and a new value in the instant of such a write, but it stays one copy for the
whole frame. The alternative is a sixth step that wraps those places now.

**D4. Step 5 is part of the sprint.** It is the least needed step and can be dropped
without touching the others.

**D5. The whole project is built at each step.** See section 1.

**D6, taken on 2026-11-18. Finding 9 is corrected in this sprint,** as step 3b. The owner
asked for it after seeing a recording stay on at the end of a video.

**D7, taken on 2026-11-18. An edge drag is limited to one size step per 30 ms,** as step
4b. Without the frame lock the drag took about 90 steps a second and showed a white strip
at the moving edge. The other ways, a dark background for the unpainted area or leaving
it, were not taken. The limit did not remove the strip, and D8 replaced this decision the
same day.

**D8, 2026-11-18. The frame lock stays where the window lays out the main display.**
Steps 4 and 4b are taken back, so that the window looks while it is resized as it did
before the sprint. I took this one myself, after the owner had asked whether the old
behaviour was back, and told the owner so; the owner can reverse it. Taking the lock away
is left for a time when the cause of the strip can be looked for on the screen.

## 8. Found while reading, not changed

The first three I read in the code and have not run. They are not locking faults and are
not touched by this plan.

1. **Pseudocolor ticked for an alt display.** The display picker offers ALT1 and ALT2, and
   the pseudocolor box writes its two flags to the picked display (7306-7307). The frame
   path colours the main display only (`pipelineprocessor.cpp` 809), but takes the picture
   format from the second flag for every display (986). An alt display with a grey source
   and the box ticked is then drawn as colour over a one-channel picture: a wrong picture,
   read past its end on every frame. With the histogram shown (835), or while that display
   is recording (1704), a colour conversion is given a one-channel picture and throws, and
   the frame thread has no handler. If this is right it can end the program; I would not
   try it with work unsaved.
2. **A recording on an alt display can be left open.** Stopping an alt display (6984-6998)
   does not close its recorder: the two lines that did are commented out (6985-6986). The
   REC button of that display then does nothing, because `alt_display_control` returns at
   once while the display has no frame thread (6982). The file stays open in the temporary
   folder until that display runs again; if the program is closed first, it is never moved
   to the save folder.
3. **After a route change the ROI list and the mask disagree.** The mask is replaced by an
   empty one (250), the list of rectangles is kept. The 3D stage then masks the range map
   with the empty mask whenever the list is not empty (701), so the range view gets zeros
   until the rectangles are cleared and drawn again.
4. **`TimedQueue::count()` reads the queue without its lock.** The window calls it in three
   places to choose a recorder's frame rate (3843, 6754, 7027).
5. **`UserPanel::filter_scan` is dead.** Nothing calls it. It is one of the two places where
   the window reads `latest_raw` without the lock.
6. **`StreamSlot::active` is never read or written.**
7. **A port thread gets 100 ms to stop.** `~DeviceManager` asks each of the seven port
   threads to quit and waits 100 ms for it (`devicemanager.cpp` 94-125). The thread objects
   are children of the window and are destroyed right after. A port thread that needs
   longer is destroyed while it still runs, and Qt ends the program for that. Related:
   `ControlPort::communicate` measures its reply timeout with one `static` timer that all
   ports share, so ports that talk at the same moment lengthen each other's wait. Found
   while looking into the fatal line of the step 1 run.
8. **With INFO off, the time stamp meant for a RES recording does not reach the file.**
   `record_frame` is to draw the time into the recording when `save_info` is set and INFO
   is not shown. It copies the picture for the recorder first and draws into the original
   afterwards (`pipelineprocessor.cpp` 1692-1695), so the frame that is written carries no
   stamp. Read, not run.
9. **Whether a recording is closed when its video ends depends on timing.** The frame
   thread closes the recorders the first time it sees a closed and empty queue, but only
   while `source_dead` is not yet set (`pipelineprocessor.cpp` 265-278). The window sets
   that flag itself just before it closes the queue (`userpanel.cpp` 6596-6598, and the
   same at 6573 and 6609). If the frame thread is waiting for a frame at that moment, it
   wakes, clears the flag as it does on every pass that gets past the queue check
   (294-297), and on its next pass finds the queue closed and the flag clear: the recording
   is closed and announced. If it is in the middle of a pass instead, it finds the flag set
   and skips the block: the recording stays open, without new frames, until STOP is
   pressed or the program is closed. Seen in the run of step 2: the recording stayed open
   two times out of two. The cause is that the flag has two writers, and that the frame
   thread uses it as its own memory of what it has already closed. The window's write
   came in sprint 25 (`8a99982`); the frame thread's test was the same at `ff40eaa` and
   through step 3. Corrected in step 3b: the frame thread keeps that memory in a variable
   of its own.
10. **The title shows the bit depth of the display that was started last.**
    `start_static_display` sets the resolution in the title for the main display only, but
    the bit depth for every display (`userpanel.cpp` 6672-6674 and 6515-6519). A file
    loaded onto an alt display therefore changes the bit depth shown for the main one.
    Step 3 keeps this as it is. Read, not run.
11. **A scan started without a live source waits and does not say so.** A scan advances
    on frames. With a video that has ended on the main display, the frame thread skips
    every stage, the scan among them. `ScanController` does not test for a live source
    when SCAN is pressed: it makes the scan folder, writes `scan_params` and waits. Seen
    in the run of step 3: a folder with that one file, and no line in the log.
12. **The camera's frame rate setting is the main stream's `fps`, and a video overwrites
    it.** The field is filled from the frame rate box of the camera and from the config
    file (`userpanel.cpp` 4677). A video that opens on the main display writes its own
    rate into it (6549), and at exit the field is saved as the camera's frequency (4610).
    After a video the box and the config file therefore hold the video's rate, and a still
    image loaded next is scanned at that rate. Seen after the run of step 3b: the config
    file says 24, the rate of the video played.
13. **A file loaded on the main display always goes to stream 0, also while an alt
    display shows that stream.** The loaders stop the source of slot 0
    (`userpanel.cpp` 6065, 6601) and set up display 0 with stream 0, which takes the
    stream away from the display that shows it (6666, 6703). A swap changes which display
    shows a stream, not which stream a file is loaded into (3245). So a video that was
    loaded on the main display and swapped to an alt display stops as soon as the next
    file is loaded, and only one file can play at a time. Seen by the owner in the run of
    step 4, and in the log of the run of step 2. It belongs to M-1, which gives the
    sources a controller.
14. **A recording started within five seconds of a change of source gets a wrong frame
    rate.** The writer is opened with the arrival rate measured over the last five
    seconds (`userpanel.cpp` 3843-3844). Right after a video replaces a still image that
    is still the rate of the image: 194 was taken for a video of 24 frames a second in
    the run of step 4. Right after a first load it is too low: 5.8 for 13 in the run of
    step 2. The file then plays too fast or too slow. Read from the logs; the files were
    not played.
15. **While the window is resized, its inside follows one frame late.** A drag changes
    the size of the window at once, but `resizeEvent` then waits for `paint_mutex`, that
    is for the frame in progress, before it has placed all the widgets. Until then the
    window shows the old arrangement in the new size: made smaller, the widgets at the
    right and at the bottom are cut off. With a plain picture the wait is short and hard
    to see; with a slow enhancement it is a tenth of a second for every step. Seen by the
    owner on 2026-11-19 with AINDANE. Without the lock the arrangement follows at once,
    but the white strip of step 4 appears. Both go together, and both are left for the
    work on the window.
16. **An alt display shows its frame rate and its ended stream only in its own window.**
    The side panel has the small pictures of ALT 1 and ALT 2 and no label for either.
    The FPS label and the label of an ended stream belong to the window that Alt+1 or
    Alt+2 opens. An operator who watches the side panel does not see that an alt stream
    has ended. Noticed by the owner in the run of step 5.

## 9. Not in S-4

- Port state read across threads: 18 fields of `TCU` and 3 to 10 each of four other ports.
  Other files and other threads; a sprint of its own.
- The window's 25 unlocked writes of the stream's metadata, and its two unlocked reads of
  `latest_raw` (922-923, and the dead `filter_scan`). With M-1, see D3.
- `setup_mutex` used as a busy flag, and the threads the window starts (S-5).
- The frame lock itself, since 2026-11-18. `paint_mutex` is still held for a frame and
  has one user left in the window, `resizeEvent`. It can go once it is known why a white
  strip shows at the dragged edge without it (steps 4 and 4b).
- Making the fields private. The struct stays open; the rule is in its comments and in the
  census. An accessor layer is worth it once M-1 has moved the window's 187 accesses.
- `ConsumerThread` being declared in `userpanel.h`, and who starts and stops it.
- `QPixmap` built on frame threads (L-5).

## 10. Risks

| Risk | Guard |
|---|---|
| Something still leans on the frame lock that the census did not see | The lock goes last, in a step of its own, after each known user has its own lock. The census is repeated before that step. Its run uses all three displays, a recording and a scan |
| A deadlock through the new lock | `recorder_mutex` is taken alone on both sides. TC-PL-007 runs it against frames and route changes |
| A fault that depends on timing does not show at the desk | The tests of 4.6. The sprint is committed apart from sprint 26, so each can be built alone on the hardware |
| Step 1 moves every field of the struct | Only the order changes, every use is by name. The compiler sees all of it |
| A step does not compile, since I cannot build | The steps are small, and the compiler message is enough for me to correct one |
| TC-PL-008 depends on how fast the machine is | The test makes the frame slow itself, by waiting inside it, and its limit is half of that wait |
| The `LVTONG` branches | Changed the same way as the live ones; no build defines the flag |
