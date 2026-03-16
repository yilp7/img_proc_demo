# Sprint 0 — Progress Log

> **Branch:** refactor (TBD)
> **Started:** 2026-03-02
> **Goal:** Fix obvious defects that require no architectural change.

---

## Task 0.1: Add `virtual` to `Cam::~Cam()`

- **File:** `src/cam/cam.h:16`
- **Change:** `~Cam() {};` → `virtual ~Cam() {};`
- **Rationale:** `Cam` is an abstract base class with pure virtual methods. Subclasses (`MVCam`, `EBusCam`, `EuresysCam`) are deleted through `Cam*` pointers. Without a virtual destructor, this is undefined behavior.
- **Status:** DONE

## Task 0.2: Eliminate fragile heap allocation in PTZ `generate_ba` calls

- **Files:** `src/port/ptz.cpp:10-13, 118-127`
- **Change:** Replace `generate_ba(new uchar[7]{...}, 7)` with stack-allocated arrays + direct `QByteArray` construction.
- **Rationale:** The original roadmap classified this as a "memory leak", but `generate_ba()` actually calls `delete[] data` internally (`controlport.cpp:78`), so there is no leak. However, the pattern is fragile: it heap-allocates small 7-byte arrays, relies on hidden ownership transfer (caller `new`s, callee `delete[]`s), and is strictly slower than stack allocation. The fix uses stack arrays, which are both safer and more efficient — heap alloc/dealloc costs 100-400+ ns per call vs ~0 ns for stack.
- **Instances fixed:** 12 (2 in constructor, 10 in `ptz_control`)
- **Status:** DONE

## Task 0.3: Eliminate fragile heap allocation in TCU constructor

- **File:** `src/port/tcu.cpp:32`
- **Change:** Same pattern as 0.2 — replace `generate_ba(new uchar[7]{...}, 7)` with stack array.
- **Instances fixed:** 1
- **Status:** DONE

## Task 0.4: Replace `void*` in GrabThread with typed `UserPanel*`

- **Files:** `src/visual/userpanel.h:53,62`, `src/visual/userpanel.cpp:5-7,26,2598,5758`
- **Change:** `void *p_info` → `UserPanel *m_panel`. Remove `(void*)` and `(UserPanel*)` casts.
- **Rationale:** The `void*` forces an unsafe C-style cast in `run()`. Since GrabThread is always constructed with a `UserPanel*`, the type should be explicit.
- **Status:** DONE

## Task 0.5: Replace `void*` in JoystickThread with typed `UserPanel*`

- **Files:** `src/thread/joystick.h:15,62`, `src/thread/joystick.cpp:3,15`
- **Change:** `void *p_info` → `UserPanel *m_panel`. Add forward declaration `class UserPanel;` in header.
- **Note:** `p_info` is currently assigned but never read in joystick.cpp — it's dead storage. The typed pointer preserves the original intent (access to UserPanel from joystick thread) while making the type explicit for future use.
- **Status:** DONE

## Task 0.6: Fix `get(GATE_WIDTH_A)` unreachable code

- **File:** `src/port/tcu.cpp:57-62`
- **Change:** Remove the early `return gate_width_a;` before the `switch (tcu_type)` block.
- **Rationale:** The `get(GATE_WIDTH_A)` method was intended to remap to `gate_width_b` for TCU type 2 (same pattern as `LASER_WIDTH` and `DELAY_A`), but an early `return gate_width_a;` prevented the `switch (tcu_type)` from executing. Removed the early return to restore the intended behavior.
- **TODO (hardware test):** Verify type 2 `GATE_WIDTH_A` remap behavior during hardware testing.
- **Status:** DONE

---

## Build Verification

- **Status:** PASS
- **Build:** Successful
- **Smoke test:** Camera image grabbing and TCU control verified working
- **Date:** 2026-03-02
