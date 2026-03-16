# Phase 1 — Refactoring Retrospective

> Post-refactoring architect review of Sprints 0–9.
> Based on sprint progress documents, commit messages, and full source code analysis.

---

## Table of Contents

1. [Sprint-by-Sprint Assessment](#sprint-by-sprint-assessment)
2. [Cross-Sprint Assessment](#cross-sprint-assessment)

---

## Sprint-by-Sprint Assessment

### Sprint 0 — Obvious Defect Fixes

| Criterion | Rating | Notes |
|-----------|--------|-------|
| Goal clarity | ★★★★★ | "Fix obvious defects requiring no architectural change" — perfectly scoped |
| Execution quality | ★★★★★ | 6 targeted fixes: virtual destructor, heap alloc, void* typing, unreachable code |
| Risk management | ★★★★☆ | All changes are low-risk; `get(GATE_WIDTH_A)` remap (Task 0.6) flagged as needing hardware verification — responsible deferral |
| Debt introduced | None | Clean surgical fixes |
| Dependency ordering | Correct | Foundation sprint — must come first |

**Verdict:** Textbook opening sprint. Established trust in the refactoring process by fixing real bugs with zero architectural risk. The `generate_ba` heap elimination pattern was later reused in Sprint 3 — good foresight.

---

### Sprint 1 — Dead Code Removal

| Criterion | Rating | Notes |
|-----------|--------|-------|
| Goal clarity | ★★★★★ | Remove all dead code: `#if 0`, comments, disabled features, goto labels |
| Execution quality | ★★★★★ | 10 `#if 0` blocks removed; `ENABLE_USER_DEFAULT` eliminated; goto → break; 7 static locals fixed |
| Risk management | ★★★★☆ | Static local fix in `grab_thread_process()` addressed real data races — correctly identified as safety issue, not just cleanup |
| Debt introduced | None | Pure deletion sprint |
| Dependency ordering | Correct | Dead code removal before extraction sprints prevents carrying dead weight forward |

**Verdict:** The static local variable fix (7 variables causing data races in the worker thread) elevated this from a cosmetic sprint to a safety sprint. Good judgment calling it out. The `frame_average + 3D with 6/8/10 images` bug was documented but correctly deferred — it's a pre-existing functional issue, not a refactoring concern.

---

### Sprint 2 — Leaf Module Extraction (MyWidget Breakup)

| Criterion | Rating | Notes |
|-----------|--------|-------|
| Goal clarity | ★★★★★ | Extract Display, TitleBar, StatusBar, FloatingWindow from monolithic MyWidget |
| Execution quality | ★★★★★ | 4 clean extractions; forward declarations used to break header dependencies; `THREAD → WIDGETS` dependency removed |
| Risk management | ★★★★★ | Leaf modules have no downstream dependents — zero regression risk |
| Debt introduced | None | `mywidget.h/cpp` residuals remain (base widget styling), but this is intentional — MyWidget still serves as theme base |
| Dependency ordering | Correct | Leaf-first extraction is the safest order |

**Verdict:** Model extraction sprint. Starting with leaf modules that have no dependents is the correct strategy. The forward-declaration technique to break `TitleBar → Preferences/ScanConfig` dependencies shows architectural awareness.

---

### Sprint 3 — Magic Number Extraction

| Criterion | Rating | Notes |
|-----------|--------|-------|
| Goal clarity | ★★★★☆ | "Extract magic numbers" is clear, but scope expanded to include `generate_ba` heap elimination project-wide |
| Execution quality | ★★★★★ | 3 protocol/constants headers created; 60+ magic numbers replaced; remaining `generate_ba` heap allocs eliminated |
| Risk management | ★★★★☆ | Protocol constants (`pelco_protocol.h`, `tcu_protocol.h`) require domain knowledge to verify — but values were extracted, not invented |
| Debt introduced | None | |
| Dependency ordering | Correct | Constants must exist before protocol modules are refactored (Sprint 6) |

**Verdict:** Solid sprint. The decision to create separate protocol headers (`pelco_protocol.h`, `tcu_protocol.h`) rather than a single `constants.h` dump shows good module thinking. Scope expansion to finish `generate_ba` elimination was a reasonable opportunistic cleanup.

---

### Sprint 4 — Copy-Paste Elimination

| Criterion | Rating | Notes |
|-----------|--------|-------|
| Goal clarity | ★★★★☆ | "Eliminate copy-paste" — clear intent, but "~50 lines" is modest scope |
| Execution quality | ★★★★☆ | 5 helpers extracted: `throttle_check`, `setup_slider`, `init_tcu_on_connect`, `release_buffers`, `connect_port_edit` |
| Risk management | ★★★★★ | All extractions are pure refactors — identical behavior preserved |
| Debt introduced | None | |
| Dependency ordering | Correct | Copy-paste cleanup before major extractions (Sprints 6–9) prevents duplicating the duplication |

**Verdict:** Useful but conservative sprint. The `throttle_check` helper is particularly valuable — it encapsulates a timing pattern reused across 3 TCU update functions. The `release_buffers` lambda extraction later proved important when Sprint 8 had to split it into `release_avg_buffers`/`release_all` for the 3D colorbar fix.

---

### Sprint 5 — Thread Safety Fixes

| Criterion | Rating | Notes |
|-----------|--------|-------|
| Goal clarity | ★★★★★ | Fix critical GUI mutations from worker thread; audit all cross-thread accesses |
| Execution quality | ★★★★☆ | 3 CRITICAL fixes applied (scan button, auto_mcp, model list); full audit produced categorized inventory |
| Risk management | ★★★☆☆ | **20 HIGH-risk read-only UI accesses deferred** with the rationale "practically safe on x86, fix when pipeline extracted" — this is technically correct but creates a debt ceiling |
| Debt introduced | **Moderate** | The deferred HIGH-risk items became Sprint 8's responsibility. The "x86 word-size read" argument is valid for `bool`/`int` reads but does not cover `QString` or `QSize` reads |
| Dependency ordering | ★★★★★ | Thread safety before pipeline extraction (Sprint 8) is essential — you need to understand the hazards before restructuring the code |

**Verdict:** The 3 critical fixes were correct and well-executed. The full audit (28 UI + 19 pref accesses catalogued) was invaluable for Sprint 8 planning. However, the deferral of 20 HIGH-risk items carries real risk: the assumption that Sprint 8's `ProcessingParams` would capture everything proved partially false — `config->get_data()` reads were not included in the snapshot, and some controller method calls from the worker thread remain unprotected (see Phase 3).

---

### Sprint 6 — PTZ Controller Unification

| Criterion | Rating | Notes |
|-----------|--------|-------|
| Goal clarity | ★★★★★ | Unify 3 PTZ implementations behind `IPTZController` interface |
| Execution quality | ★★★★★ | Clean interface with Direction enum, 8 virtual methods; 8 type-switches eliminated; scan loop generalized |
| Risk management | ★★★★★ | Each implementation adapted incrementally; `ptz_qobject()` bridge enables Qt signal connectivity without multiple inheritance |
| Debt introduced | None | |
| Dependency ordering | Correct | Interface extraction before controller decomposition (Sprint 9) ensures DeviceManager can manage PTZ polymorphically |

**Verdict:** Excellent sprint. The `IPTZController` design is clean — the `ptz_qobject()` method to expose the QObject for signal connection is a pragmatic solution to Qt's single-inheritance constraint. Reducing 8 type-switches to 3 polymorphic calls is a significant maintainability win. The `ControlPort::get_port_status() const` fix was a necessary API correction.

---

### Sprint 7 — Config as Single Source of Truth

| Criterion | Rating | Notes |
|-----------|--------|-------|
| Goal clarity | ★★★★★ | Eliminate duplicated state between Config, Preferences, and UserPanel |
| Execution quality | ★★★★☆ | ~55 Preferences members eliminated; `SaveSettings` and `ImageProcSettings` structs added; ~70 `pref->` reads replaced with `config->get_data().*` |
| Risk management | ★★★☆☆ | **Laser safety fix (7.0) is critical and well-handled.** However, `config->get_data()` returns a non-const reference — worker thread now reads `ConfigData` fields directly without mutex protection. This introduced a new class of data races. |
| Debt introduced | **Significant** | The non-thread-safe `Config::get_data()` pattern replaced `pref->` reads (which were also unsafe) but did not improve thread safety — it merely relocated the hazard. No mutex was added to Config. |
| Dependency ordering | ★★★★☆ | Config consolidation before pipeline extraction (Sprint 8) is correct — but the lack of a Config snapshot mechanism means Sprint 8's ProcessingParams only partially solved the problem |

**Verdict:** The architectural intent is sound — Config as single source of truth eliminates state synchronization bugs. The laser safety fix (removing `laser_on` from JSON persistence) is an important safety decision. However, this sprint introduced the project's most pervasive remaining thread safety issue: `config->get_data()` returning a mutable reference that is read from the worker thread while the GUI thread writes to it. The `ConfigData` struct contains `QString` members, which are **not** safe for concurrent read/write even on x86. This must be addressed.

---

### Sprint 8 — Pipeline Extraction

| Criterion | Rating | Notes |
|-----------|--------|-------|
| Goal clarity | ★★★★★ | Decompose 894-line `grab_thread_process()` into focused pipeline methods with thread-safe parameter snapshot |
| Execution quality | ★★★★★ | 10 methods extracted; `ProcessingParams` snapshot struct eliminates 18 direct UI reads; `FrameAverageState` and `ECCState` structs introduced; 2 bugs fixed during extraction |
| Risk management | ★★★★☆ | `ProcessingParams` snapshot is an excellent pattern. However, it only captures UI widget state — Config reads, controller method calls, and scan state mutations are not covered. The `hasFocus()` always-false risk was correctly identified. |
| Debt introduced | **Moderate** | Pipeline methods still call `config->get_data().*` (~30 access points), `m_tcu_ctrl->get_*()` (~8 calls), and `m_scan_ctrl->set_*()` (~3 writes) directly from the worker thread without synchronization |
| Dependency ordering | Correct | Pipeline extraction after thread safety audit (Sprint 5) and Config consolidation (Sprint 7) |

**Verdict:** This is the strongest sprint in the series. The `ProcessingParams` snapshot pattern is architecturally correct and eliminates the majority of Sprint 5's deferred UI reads. The bug fixes discovered during extraction (3D colorbar regression, `custom_3d_delay` source) demonstrate the value of methodical decomposition — moving code reveals hidden assumptions. The remaining gap is that the snapshot doesn't extend to Config data or controller state, leaving ~40 cross-thread accesses unprotected.

---

### Sprint 9 — UserPanel Decomposition

| Criterion | Rating | Notes |
|-----------|--------|-------|
| Goal clarity | ★★★★★ | Extract 7 controller classes from UserPanel god class |
| Execution quality | ★★★★☆ | 7 controllers extracted (1688 lines moved); UserPanel reduced 6504→4771 lines; CameraController correctly skipped |
| Risk management | ★★★☆☆ | Controller methods are now called from the worker thread via `m_tcu_ctrl->`, `m_scan_ctrl->` without synchronization — the extraction moved code but did not add thread boundaries. The decision to skip CameraController (9.6) was correct — camera state is genuinely inseparable from grab thread lifecycle. |
| Debt introduced | **Moderate** | Controllers are QObjects living in the main thread but accessed from worker threads via direct method calls. This is architecturally ambiguous — are they "owned" by the main thread or shared? The inline delegation pattern (`m_tcu_ctrl->method()` calls in pipeline code) creates coupling between the controller layer and the worker thread without explicit thread-safety contracts. |
| Dependency ordering | Correct | Final decomposition sprint — depends on all prior infrastructure |

**Verdict:** Good extraction work. The controller boundaries are reasonable, and the incremental rewiring approach (connecting signals during extraction rather than in a separate pass) reduced integration risk. However, the extraction exposed a design tension: controllers hold state that is read/written from both the GUI thread (via slots) and the worker thread (via direct method calls in pipeline code). This creates an implicit shared-state contract that needs explicit synchronization. The `advance_scan()` method is the highest-risk example — it writes `scan_ptz_idx` and `scan_tcu_idx` via ScanController setters from the worker thread while the GUI thread reads them during scan setup.

---

## Cross-Sprint Assessment

### What Was Done Well

1. **Sprint sequencing was nearly optimal.** The progression from defect fixes → dead code → leaf extraction → constants → copy-paste → thread audit → interface unification → config consolidation → pipeline extraction → controller decomposition follows a logical dependency chain. Each sprint's outputs enabled the next.

2. **Safety-critical decisions were handled responsibly.** The laser safety fix (Sprint 7.0), scan button thread-safety fix (Sprint 5.1), and the decision to keep `laser_on` out of JSON persistence show awareness that this is industrial/safety-adjacent software.

3. **The ProcessingParams snapshot pattern (Sprint 8) is the right architecture.** Capturing UI state once per frame iteration and passing it as an immutable value through the pipeline is the textbook solution for GUI↔worker thread decoupling. This pattern should be extended to cover Config and controller state.

4. **Pragmatic deferrals.** Skipping CameraController extraction (Sprint 9.6) and deferring the `frame_average + 3D with 6/8/10 images` bug were correct calls — both require deeper investigation that would have blocked the refactoring momentum.

5. **Bug discovery through decomposition.** Sprints 8 found 2 real bugs (3D colorbar regression, `custom_3d_delay` source) that were invisible in the monolithic code. This validates the refactoring approach.

### What Could Have Been Done Differently

1. **Config thread safety should have been addressed in Sprint 7.** When `config->get_data()` replaced `pref->` reads, a mutex or snapshot mechanism should have been added simultaneously. The current pattern — returning a mutable reference to `ConfigData` from a class with no mutex — is the project's most pervasive remaining thread safety issue. This was a missed opportunity.

2. **ProcessingParams should have included Config fields from the start (Sprint 8).** The snapshot struct captures 20 UI widget values but none of the ~30 Config fields read by pipeline methods. A `PipelineConfig` snapshot taken alongside `ProcessingParams` would have closed this gap.

3. **Controller thread-ownership contracts should have been defined in Sprint 9.** When extracting controllers, each should have been documented with its thread affinity: "this controller's state is accessed only from the main thread" or "this controller's getters are called from both threads and require synchronization." Without this, the extraction moved code without resolving the underlying thread safety question.

4. **Sprint 4 was under-scoped.** At ~50 lines eliminated, it delivered less impact than any other sprint. The copy-paste patterns it targeted were real, but the sprint could have also addressed the duplicated sync logic (`syncConfigToPreferences`/`syncPreferencesToConfig`/`data_exchange`) which remains a source of confusion.

### Deferred Items That Are Now Urgent

| Item | Origin | Why Urgent |
|------|--------|------------|
| `config->get_data()` thread safety | Sprint 7 | Returns mutable reference read by worker thread; `QString` fields are not safe for concurrent access; ~30 unsynchronized reads in pipeline methods |
| Scan state race (scan_ptz_idx, scan_tcu_idx) | Sprint 8/9 | Written from worker thread via `m_scan_ctrl->set_*()`, read from GUI thread during scan setup — no mutex |
| TCUController writes from worker thread | Sprint 9 | `set_frame_a_3d()` and `set_delay_dist()` called from `frame_average_and_3d()` and `advance_scan()` without synchronization |
| `update_fishnet_result` signal connection type | Sprint 8 | Uses `Qt::AutoConnection` which resolves to `DirectConnection` when emitted from worker thread — slot may execute in worker thread context |
| DeviceManager `angle_h`/`angle_v` race | Sprint 6/9 | Written by PTZ port thread, read by main thread — non-atomic `float` writes |

### Deferred Items That Can Stay Deferred

| Item | Origin | Why Can Wait |
|------|--------|-------------|
| CameraController extraction (9.6) | Sprint 9 | Cam hierarchy already provides SDK abstraction; camera state is genuinely coupled to grab thread lifecycle |
| `max_laser_changed` disconnected signal | Sprint 7 | No functional impact — signal exists but isn't connected. Can be removed in a cleanup pass |
| `get(GATE_WIDTH_A)` hardware verification (0.6) | Sprint 0 | Code path works for current TCU types; verification only needed if a new TCU type is added |
| `frame_average + 3D with 6/8/10 images` bug | Sprint 1 | Pre-existing functional limitation due to queue cap; not a regression risk |
| `ProcessingParams::hasFocus()` always false | Sprint 8 | Only affects auto-MCP behavior when MCP slider is focused during snapshot — edge case with minimal UX impact |
