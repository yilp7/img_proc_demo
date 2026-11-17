# Proposed structure and patterns

2026-11-14. A file copy of the structure proposed in the conversation of that day, kept for the
structure discussion that is to follow the logic refactor.

**Status: a proposal.** Nothing in it is decided except the starting point: work begins with
finding S-3 on branch `refactor-2` (plan in `../2. refactor/sprint-26-plan.md`). Finding numbers
such as S-1 or M-1 are listed in section 10. All figures were measured on 2026-11-14 at commit
`5481571`.

## 1. The idea

Programs that control devices, carry a live image path and have an operator UI stay maintainable
when they share one shape: **a core that runs without a window, and a UI that is only one client
of that core.** "Layered", "hexagonal" and "clean" architecture are three names for this.

Three open-source programs built this way, useful for comparison:

- OBS Studio: the core library `libobs` under a Qt front end, with sources, filters and outputs.
- Micro-Manager: `MMCore` and its device adapters under the GUI.
- QGroundControl: links, a vehicle model and one store of settings under a Qt UI.

## 2. The rows

| Row | Directories today | Holds | Must not contain |
|---|---|---|---|
| app | `src/main.cpp` | creates every long-lived object and connects them | logic |
| ui | `visual`, `widgets` | windows, panels, overlays: show state, report what the operator did | devices, threads, port or file I/O |
| control | `controller`, `automation`, `plugins` | decides what happens: devices, scan, sources | widget types |
| processing | `pipeline`, `image`, `yolo` | the frame path: sources, queues, stages, recorders | controller or widget types |
| hardware | `port`, `cam`, `thread` | one interface per device kind, one thread per device | processing types |
| core | `util` | frame and id types, queues, logging | Qt Widgets, SDK headers |

A row may include only rows below it.

## 3. How the rows talk

Four kinds of traffic cross the rows, each in one fixed way.

| Traffic | Direction | Form |
|---|---|---|
| Commands | down | a call: UI to controller to device, queued into the device's thread |
| State | up | a signal carrying a value, never a pointer to a live object |
| Frames | along the processing row | queues: source, queue, pipeline, then image, recorder and detections. The pipeline works on a per-frame copy of the settings, reports results as events, and calls nothing above it |
| Settings | out of one store | the UI edits the store; everyone else receives copies |

## 4. Where the current tree differs

The tree already has these rows. Three things separate it from the picture.

1. `UserPanel` fills three rows at once: app, ui, and the source and camera part of control
   (S-1, S-2, M-1).
2. The frame path calls upward (S-3) and shares an open struct with the UI (S-4).
3. Nothing enforces the order. The build is one executable from one flat list, and
   `util/util.h` pulls Qt Widgets, OpenCV, FFmpeg and an SDK header into the bottom row (M-6).
   One CMake library per row would turn an upward include into a compile error and give the
   tests something to link (M-7).

## 5. Patterns

| Pattern | In plain words | In this program |
|---|---|---|
| Composition root | One place creates the long-lived objects and passes each what it needs | Missing: the `UserPanel` constructor does it |
| Passive view | A widget shows values and emits what the user did, nothing more | Controllers are free of the form; the window still runs camera, sources and recording |
| Ports and adapters | The program talks to an interface, and SDK classes implement it | Present for `Cam` and `IPTZController`; the pipeline gets concrete controllers |
| Pipes and filters | The frame path is stages joined by bounded queues | Present (`BoundedQueue`, `DisplayRouter`), but the stages reach outside |
| Snapshot | A thread works on its own copy, taken at one moment | Present for `ProcessingParams` and `PipelineConfig`; missing for `ScanParams`, TCU values and port state |
| Active object | An object lives on its own thread: calls go in queued, results come out as signals | Present for the seven ports; bypassed by direct field reads |
| State machine | A sequence is one object with named states, driven by events | The scan is split between `ScanController` and the frame loop |
| Single source of truth | One store per setting | Up to four copies of a setting, 292 lines of sync code |

## 6. Rules

Most can be checked mechanically after every step. The second column says what to look for.

| # | Rule | What to look for |
|---|---|---|
| 1 | Includes go downward only, with no cycle | An `#include` of a file in a higher row; a chain of includes that returns to its start |
| 2 | No widget type below the ui row, and no controller type in the frame path | An include of `widgets/`, `visual/` or `controller/` in `pipeline/`, `image/` or `yolo/` |
| 3 | Only the composition root creates long-lived objects, and nobody reaches through one object to another (`a->get_b()->c()`) | `new` of a controller or a thread outside the root; calls chained through an accessor |
| 4 | Every object belongs to one thread. Widgets are touched on the GUI thread only | A member that code of two threads reaches without a lock; a widget call in code a worker thread runs |
| 5 | Every shared field names its lock in a comment. A lock is held for a copy or a swap, never for a frame's work | A mutex without such a comment; the length of each locked scope |
| 6 | Whoever starts a thread stops and joins it. No detached threads | `detach()` |
| 7 | Nothing blocks the GUI thread: no sleep, no waiting on a port | `sleep` and blocking waits in slots and event handlers |
| 8 | Connections are typed, not strings. A lambda longer than a few lines becomes a named slot | `SIGNAL(` and `SLOT(`; the line count of lambdas in `connect` |
| 9 | New code below the ui row comes with a test that runs without a window | Source files that no test target compiles |

## 7. What this does not need

No dependency-injection library, no event bus, no move to QML. At 36,000 lines, constructors,
signals and plain value structs are enough.

## 8. Proposed order of work

| Order | Finding | Why here |
|---|---|---|
| 1 | S-3: the pipeline depends on the layers above it | Small, one file, and it makes a pipeline test possible |
| 2 | S-4: shared display state has no written locking rule | Same files, and it needs the test from step 1 |
| 3 | M-1: a camera and source controller | The first real cut into S-1. Most of the 1,764 lines on pipeline, displays and sources leave the window, and the threads the window starts (S-5) go with them |
| 4 | The rest of S-1: the composition root leaves the window | Possible once the parts have homes |
| With the redesign | S-2: the UI is defined twice | It is the redesign under another name. One part is safe earlier: each panel builds its own content (five constructor blocks, about 1,100 lines) |
| Any time | S-5: GUI calls from a capture thread | Small. It disappears when the `cv:` path is replaced, at the latest in step 3 |
| Before step 1, undecided | Clearing: the dead files (M-3) and the two includes that close the cycle (M-6) | Nearly free. Whether dead code is deleted or parked is the owner's call |

Why S-1 is not first: it is the sum of the others. The file was cut to 3,943 lines and grew
back to 7,362. My reading is that two kinds of code had no other home: UI building (the
constructor went from 458 lines after sprint 9 to 2,377) and source and display control.

## 9. Open for the discussion

- The names and exact borders of the rows, and whether directories move to match them.
- One CMake library per row: whether, and at which point of the work.
- The settings store: what it is and in which row it lives. Today `Config` is in `util`, and a
  setting exists in up to four copies (M-5).
- One controller for cameras and video sources, or two.
- Where the scan sequence lives: `ScanController` or the frame path. The sprint 26 plan keeps
  it in the frame path for now and says why.
- How much of S-2 to do before the UI redesign.
- Dead code: delete or park.

## 10. The findings referred to

From the structure scan of 2026-11-14, in the order of how much each stands in the way of
further work.

| # | Finding |
|---|---|
| S-1 | `UserPanel` is the application. It creates every object, builds the UI, wires it, and runs the camera and the video sources. `userpanel.cpp` has 7,362 lines; the class reaches into 82 types and makes 253 of the 448 signal connections |
| S-2 | The UI is defined twice: a Designer form with 255 widgets, and a 2,377-line constructor that re-homes about 40 of them, hides about 60 and builds the rest in code |
| S-3 | The pipeline depends on the layers above it. `PipelineGlobals` carries pointers to four controllers and a window, and the scan is stepped inside the frame loop |
| S-4 | Shared display state has no written locking rule. `DisplaySlot` has 40 public fields, 4 mutexes and 7 atomics; the window touches it 265 times and the pipeline 153 times; one mutex is held for a whole frame |
| S-5 | GUI calls from a capture thread (the `cv:` path of `load_video_file`), and seven detached threads started by the window |
| M-1 | No camera or source controller: 46 methods and 1,764 lines of `UserPanel` deal with pipeline, displays and sources |
| M-2 | The FFmpeg decode loop exists twice: in `FfmpegSource::run_thread` and in the export code of `load_video_file` |
| M-3 | About 5,000 dead lines: files not built, classes never instantiated, unreferenced methods, flags nobody defines, commented-out code |
| M-4 | Wiring is concentrated and partly by string: 253 connections and 142 lambdas in one class, 68 string connects, 18 dead signals |
| M-5 | Settings live in several copies: `Config`, `PipelineConfig`, the form widgets and members of `UserPanel`, with 292 lines of copying code |
| M-6 | Header weight and one cycle. `util/util.h` pulls in four Qt modules, OpenCV, FFmpeg and an SDK header; `userpanel.h` includes 52 project headers; seven units form a dependency cycle that two includes close |
| M-7 | Test reach: the tests compile 14 of 73 source units, nothing above the port layer. No CI, and no library target to test against |
| M-8 | UI and waiting in the control layer: two dialogs in `DeviceManager`, a window held by `TCUController`, one-second sleeps in `ptz_button_pressed` |
| L-1 | 113 mutable function-local statics and 31 mutable globals |
| L-2 | 36 `goto` |
| L-3 | 2,620 unnamed numeric literals |
| L-4 | Classes in the wrong file: `TimedQueue` is implemented in `userpanel.cpp` |
| L-5 | `QPixmap` built on consumer threads in five places |
| L-6 | Public data members on eight classes |
| L-7 | The optional SDKs cannot be left out of a build |
