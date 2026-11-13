# Sprint 25 Progress: Function Tuning

## Prerequisite: Sprint 24
Status: SATISFIED

Sprint 24 closed the hardware-function-test walkthrough and committed the fixes that pass surfaced (UDPPTZ protocol gating, lens relay, setup-panel sync, custom-widget polish, YOLO non-ASCII paths). Sprint 25 picks up the remaining feature behaviours that worked but need calibration / tuning rather than structural fixes.

## Branch
All Sprint 25 work stays on `refactor`.

## Strategy

Open list — tasks added as field testing surfaces them. Each task lands as a small focused change; commit cadence stays one-per-sprint at sprint end.

## Out of scope

- New features
- Architecture changes
- Full i18n audit

Both feature and architecture exclusions were lifted during the sprint as field work surfaced them: T3 (routing refactor) by explicit approval, T8–T18 one at a time. The i18n exclusion held; T7 wrapped runtime strings only.

---

## Tasks

### T1 — Display-swap behaviour

Background: both source types (industrial cam + RTSP/HTTP/local-file streams) feed a single shared pipeline. Industrial cam is hard-wired to `q_img[0]` via `cam->set_user_pointer(&cam_union)` ([userpanel.cpp:2746](src/visual/userpanel.cpp#L2746)); streams pick a target display via the dialog at [userpanel.cpp:3687-3743](src/visual/userpanel.cpp#L3687-L3743) → `load_video_file` → `start_static_display` ([userpanel.cpp:5220](src/visual/userpanel.cpp#L5220)). Swap retargets only `_display_idx`; the source queue stays put.

- [x] **Stopping-frame on empty/non-empty swap.** `swap_grab_thread_display` ([userpanel.cpp:2391-2410](src/visual/userpanel.cpp#L2391-L2410)) called `displays[old]->clear()` synchronously, but in-flight `set_pixmap` events (Qt::QueuedConnection from [pipelineprocessor.cpp:691](src/pipeline/pipelineprocessor.cpp#L691)) posted before the swap took `image_mutex` still landed afterwards, repainting the cleared display. Fixed by switching to `QMetaObject::invokeMethod(... "clear", Qt::QueuedConnection)` so the clear lands after any stale pixmap (FIFO).

- [x] **YOLO model keyed by source thread instead of destination display.** `detect_yolo` ([pipelineprocessor.cpp:214](src/pipeline/pipelineprocessor.cpp#L214)) indexed `pcfg.{main,alt1,alt2}_display_model`, `m_yolo_detector[]`, and `m_yolo_last_model[]` by `thread_idx`. After swap, the model followed the source instead of the destination — picking a model on the main-display dropdown actually configured whichever source originally sat at thread 0. Re-keyed all three by `display_idx`. The local `yolo` cache in `PipelineProcessor::run` is now resynced from `m_yolo_detector[display_idx]` at the top of every `detect_yolo` call so a swap immediately picks up the destination display's existing detector without reinit. `draw_yolo_boxes` also moved off the hardcoded `m_yolo_detector[0]` for class names; takes `display_idx` and pulls labels from the detector that produced the results.

Out of scope for T1: cam-control widgets (gain / duty / frame_rate / trigger) still operate on the single `curr_cam`, so they continue to drive the industrial-cam source regardless of which display currently shows it. User accepted this — there's only one industrial cam, and the controls correctly belong to it.

### T2 — YOLO model status indicator

User wanted a per-display "model loaded" dot next to each model combobox in the MODELS tab. Initially specified as a two-state indicator (loaded / unloaded), then expanded to three states after we walked through the failure handling in `detect_yolo`.

**Final implementation:**
- New `PipelineProcessor::yolo_status_changed(int display_idx, int state)` signal ([pipelineprocessor.h](src/pipeline/pipelineprocessor.h)), emitted from `detect_yolo` at the tail of every reinit branch. State values: `0` = none (no model selected), `1` = loaded (detector running), `2` = failed (selected model couldn't load). Computed from `current_model_selection` and `m_yolo_detector[display_idx]` post-reinit.
- Three 11×11 `QFrame` dots created inside `ImgProcWidget::setup` ([imgprocwidget.cpp](src/widgets/sidebar/imgprocwidget.cpp)) as a third column in the MODELS-tab grid layout, alongside the reparented label + combobox. Colours: gray `#888888` / green `#4CAF50` / red `#CD5C5C` (red matches the app's existing "disconnected" convention for COM ports). Tooltip per state.
- `ImgProcWidget::set_model_status(display_idx, state)` ([imgprocwidget.h](src/widgets/sidebar/imgprocwidget.h)) called from a `Qt::QueuedConnection` slot in `UserPanel` ([userpanel.cpp](src/visual/userpanel.cpp)) that just forwards the signal payload.

**Honest implementation history** — several false starts before landing the right spot:
1. First attempt put the indicators in `user_panel.ui` under `YOLO_GRP`. They were invisible. I assumed Qt 5.12 stylesheet specificity quirks (`.QLabel { background: transparent; }` overriding inline) and iterated on widget type (QLabel → QFrame), selectors (bare `background-color` → `QFrame#ID {...}`), and colours (#3A3A3A → #888888) — none of which fixed it because the diagnosis was wrong.
2. Root cause, found by tracing the actual UI composition: the visible MODELS tab is built at runtime by `ImgProcWidget` ([userpanel.cpp:631](src/visual/userpanel.cpp#L631)), which **reparents** the comboboxes out of `YOLO_GRP` into its own QGridLayout. Widgets I'd added to `YOLO_GRP` stayed orphaned inside the hidden `MISC_DISPLAY` page — uic generated them, they were valid, they just rendered into a parent nothing displays. Should have checked the runtime widget tree before theorising about QSS.
3. Cleaned up: reverted all `.ui` edits (combobox widths and orphan widgets), created the dots directly inside `ImgProcWidget::setup`. No `.ui` changes in the final diff.

**Failure-handling semantics (preserved from before):**
- `detect_yolo` sets `m_yolo_last_model[display_idx] = current_model_selection` unconditionally, including on load failure. Consequence: a failed load is "sticky" — subsequent iterations see no change and skip the reinit block. Recovery requires the user to re-toggle the dropdown.
- Inference-time errors (`YoloDetector::run` returning non-OK) are not reflected in the dot — only build-time success/failure is. A model that loaded green could fail inference on every frame and the dot wouldn't change.
- User accepted both behaviours for now.

**Known defect — flagged, not fixed:**
- Combobox `currentIndexChanged` handlers ([userpanel.cpp:2169-2180](src/visual/userpanel.cpp#L2169-L2180)) write to `config->get_data()` but do not call `update_processing_params()`. The change reaches the pipeline's `m_pipeline_config` (and therefore `pcfg` in `detect_yolo`) only when the catch-all `focusChanged` connection ([userpanel.cpp:464](src/visual/userpanel.cpp#L464)) fires — typically when focus shifts after the dropdown closes. Today this works but is fragile (programmatic changes, keyboard nav patches, etc. could leave the pipeline using a stale model). Recommended fix: explicit `update_processing_params()` inside each combobox lambda. Deferred — not part of T2's stated scope.

### T3 — Display routing refactor

**Scope expansion notice:** this task expands the original sprint's "Architecture changes" exclusion. Discussed and approved during the sprint, after T2 walkthrough re-exposed the dual-indexing fragility from a different angle. Doc cadence for this task is overridden: written on plan confirmation (now), updated after each sub-commit once tested. The default "explicit verb" rule from [[feedback-no-progress-doc-edits]] still applies outside this task.

**Background:** the codebase carries two parallel identifiers for "which display" — `display_idx` (UI screen slot) and `thread_idx` (per-thread arrays slot). The bridge is `display_thread_idx[3]`, identity in steady state and non-identity during swap. Three things must stay in sync: array slot, `display_thread_idx[d]`, and `h_grab_thread[ti]->_display_idx`. T1 fixed the YOLO-side symptom by re-keying detector arrays from thread→display; T3 removes the dual-identifier problem at the source.

**Design — synthesized through sprint Q&A:**

- Strong enums `enum class StreamId`, `enum class DisplayId` + `IdArray<Id, T, N>` wrappers make wrong-side indexing a compile error.
- `StreamSlot` owns producer-side state: source object (`unique_ptr<Source>`, null when slot vacant), frame queue, frame-info queue, write mutex (bundled in `StreamFrameSink` for the cam adapter pointer), source metadata (w/h/color/fps/codec/source-type enum).
- `DisplaySlot` owns consumer-side state: widget, consumer thread, per-display `PipelineConfig` copy, YOLO detector + last-model, frame-average buffers, ECC state, hist accumulator, paint mutex, FPS label, source-death corner label, raw recorder + processed recorder, last-seen-route-generation.
- `DisplayRouter` is the single routing source. API: `start(DisplayId, SourceConfig)` / `blank(DisplayId)` / `swap(DisplayId, DisplayId)` / `snapshot(DisplayId) -> {optional<StreamId>, gen}` / `displayFor(StreamId) -> DisplayId`. Mutex-protected; generation counter bumped on every mutation.
- Bijection invariant: display → `optional<StreamId>` (can be blank); stream → `DisplayId` (non-optional). Stream lifecycle is bound to display routing — start requires picking a display; blanking a display stops its stream.
- `BoundedQueue` has `pop_or_timeout(duration)` + `close()`. Closed-empty wakes blocked pops; consumer also wakes on timeout to re-check route generation.
- Consumer loop: snapshot route → if generation changed, reset runtime caches (frame-avg accumulator, last-rendered pixmap, sourceDied label state, YOLO last-output cache, hist accumulator) but NOT user preferences (toggles, model selection, thresholds) → `pop_or_timeout` → tap pre-pipeline for raw recorder → run pipeline → render → tap post-pipeline for processed recorder.
- Recording: both recorders live on `DisplaySlot`. Consumer thread taps the popped frame before pipeline processing for raw, and the rendered output for processed, using metadata from the currently routed `StreamSlot`. Both auto-finalize on route change, source death, display blank. File naming inserts `_main`/`_alt1`/`_alt2` into the existing `MMdd_hhmmss_zzz_{raw,res}.mp4` convention. Main display keeps both raw and proc buttons (`SAVE_AVI` / `SAVE_FINAL`); alt1/alt2 record processed output only (single REC button each, writes proc_writer).
- Source death surfaces as a corner label on the affected display widget (FPS-label style, bottom-right). Appears on "queue closed and empty"; clears when frames flow again.
- cv:// path stays as transitional webcam handler per [[project-video-no-opencv]]; redirected to push through `StreamFrameSink` instead of touching `q_img[]`. FFmpeg-based replacement deferred.

**Sub-commits (mid-sprint cadence approved):**

**C1a — pipeline refactor (large)**
- [x] Enums + `IdArray<>` template. ([src/pipeline/routing.h](src/pipeline/routing.h))
- [x] `StreamSlot`, `DisplaySlot`, `DisplayRouter` (generation, mutex, snapshot API, bijection invariant). `DisplaySlot` carries both `paint_mutex` (pipeline output guard) and `setup_mutex` (external loader serialisation) — separated to avoid the non-recursive-mutex deadlock between `display_mutex` and `image_mutex` discovered during walkthrough.
- [x] `BoundedQueue::pop_or_timeout` + `close()` / `reopen()`.
- [x] `StreamFrameSink` bundle; cam SDK user-pointer aims at the sink. All three cam adapters migrated: `set_user_pointer(StreamFrameSink*)` on [mvcam](src/cam/mvcam.cpp), [ebuscam](src/cam/ebuscam.cpp), [euresyscam](src/cam/euresyscam.cpp); callbacks cast user_data to `StreamFrameSink*` and push directly to `sink->frames` / `sink->frame_info`.
- [x] Rename `GrabThread` → `ConsumerThread`.
- [x] `PipelineConfig` becomes per-`DisplaySlot`. `main/alt1/alt2_display_model` collapsed to a single `model_idx` per slot; `model_idx` (legacy fishnet field) renamed to `fishnet_model_idx`. Per-display YOLO model selection is GUI-only (not persisted) per [[feedback-per-display-toggles-not-persisted]].
- [x] YOLO arrays, frame-avg state, ECC state, hist accumulator → `DisplaySlot`.
- [x] Producer state (q_img, q_frame_info, w/h, pixel_depth, is_color, frame_rate_edit, grab_image, grab_thread_state) → `StreamSlot`.
- [x] `device_type` magic ints (`1`, `-1`, `-2`) → named enum on `StreamSlot::meta::SourceType` (`IndustrialCam`, `LocalFile`, `NetworkStream`, `Webcam`, `StaticImage`, `None`).
- [x] `PipelineProcessor::SharedState` dissolved → `PipelineGlobals`. Stage methods take `(StreamSlot&, DisplaySlot&, ...)` refs. Run loop snapshots the route once per iteration and bumps a generation counter so cache resets (frame-avg accumulator, last-rendered pixmap, YOLO last-output, hist accumulator) happen at route boundaries without disturbing user preferences.
- [x] `swap_grab_thread_display` rewritten as a thin `Router::swap` wrapper ([userpanel.cpp:2339](src/visual/userpanel.cpp#L2339)); three-branch body deleted. Queued `Display::clear()` for stale-pixmap protection preserved from T1.
- [x] Deletions: `display_thread_idx[]`, "fallback to 0" guards, `h_grab_thread[ti]->display_idx(false, ...)` callback machinery, `cam->set_user_pointer(&cam_union)` hardcoded q_img[0]. Industrial cam now points at `m_cam_sink` (a stable `StreamFrameSink` aimed at `StreamId::S0`).
- [x] Cam callback redirected through `StreamFrameSink` (all three SDK adapters push direct).
- [x] cv:// branch + cam callback redirected through `StreamFrameSink`. cv:// path: when `display=true` the capture thread builds a `StreamFrameSink` for the routed `StreamSlot` and pushes frames into `sink.frames` directly; the display-less export path keeps the legacy `process_frame` callback (offline conversion pipeline).
- [x] Cam stop+restart crash: structural fix in place (cam SDK `stop_grabbing()` runs before queue `close()`+`clear()` in `on_STOP_GRABBING_BUTTON_clicked`, so callbacks deregister before the queue tears down). Verified in walkthrough finding #1 below; the broken-connection variant is T16.
- [x] Proxy-shim removal: all `_PxXxx` structs and their member instances deleted from `UserPanel.h`; ~265 array-style callsites in `UserPanel.cpp` rewritten to direct `m_router.stream(static_cast<StreamId>(i))` / `m_router.display(static_cast<DisplayId>(i))` access with the strongly-typed enum at compile-known sites (`StreamId::S0`, `DisplayId::Main`, etc.). One collateral bug from a substring collision (`h[0]` → `pixel_depth` accidentally rewritten to `pixel_deptm_router…meta.height`) caught and fixed.

**Adjacent fixes that landed inside C1a (not part of original C1a scope, surfaced during walkthrough):**
- HLS within-segment burst-then-stall fixed: PTS-based throttle gate widened from `is_local_file && display && pts != AV_NOPTS_VALUE` to `display && pts != AV_NOPTS_VALUE` ([userpanel.cpp:5118](src/visual/userpanel.cpp#L5118)) so any source with valid PTS paces to content rate, not decode rate.
- HLS HTTP options added at format-context open: `http_persistent=1`, `http_multiple=1`, `seg_max_retry=3`. Saves TLS re-handshake between segment bodies.
- Resize-event divide-by-zero guarded: when source meta has zero w/h (the new no-seed default for `StreamMeta`), the layout falls back to 16:9 proportions instead of dividing by zero.
- `brightness_and_contrast` calls disabled (commented, not deleted) in `pipelineprocessor.cpp` — the GUI sliders that drove them were removed in an earlier sprint, so the calls were doing math on default contrast/brightness/gamma values and writing the unchanged result back. Per [[feedback-no-explanatory-comments]]: commented out with a single line stating why, re-enable when sliders return.

**Deferred to future work (acknowledged, parked, NOT in C1a):**
- HLS inter-segment stalls (200ms–2000ms gaps between segment bodies). FFmpeg's HLS demuxer reads segment bodies sequentially; an application-level prefetch buffer between producer and consumer would be required to bridge multi-second gaps. Out of scope here; the within-segment burst issue is fixed.

**C1b — UI + recording + status surface (medium)**
- [x] 3D row layout: `[☐] 3D | threshold edit | 16×16 icon-only RESET_3D_BTN]` on one row in `ImgProcWidget`'s GENERAL tab. Faithful to the pre-sprint-15 `user_panel.ui` geometry (`IMG_3D_CHECK` x=10/w=39, `RANGE_THRESH_EDIT` x=52/w=41, `RESET_3D_BTN` x=95/w=16/h=16, text empty). Reset button uses the existing `self_test` themed icon.
- [x] Display picker combobox at row 0 of GENERAL tab: items `MAIN / ALT1 / ALT2` matching `DisplayId` ordering. Per-display widgets (`FRAME_AVG_CHECK/OPTIONS`, `IMG_3D_CHECK`, `IMG_ENHANCE_CHECK`, `ENHANCE_OPTIONS`, `PSEUDOCOLOR_CHK`, `RANGE_THRESH_EDIT`) now write to `m_router.display(m_picked_display).pcfg.X` via `apply_picked_pcfg_from_widgets()`. Switching the picker fires `repopulate_general_widgets_from_picked()` which mirrors the new display's `pcfg` into the widgets under `QSignalBlocker` so the bouncing widget signals don't corrupt the slot being switched in.
- [x] Per-display fields moved from `ProcessingParams` (global) to `PipelineConfig` (in `DisplaySlot::pcfg`): `frame_avg_enabled`, `frame_avg_mode`, `enable_3d`, `image_3d`, `enhance_enabled`, `enhance_option`, `pseudocolor_enabled`, `pseudocolor`. Pipeline now reads these from `pcfg.X` instead of `params.X`. `update_processing_params()` apply-to-all loop preserves each display's per-display fields (incl. `lower_3d_thresh` + `model_idx`) when propagating the Config-derived fields. Per [[feedback-per-display-toggles-not-persisted]]: these are session-only / never persisted.
- [x] Raw recorder on `DisplaySlot`: consumer at [pipelineprocessor.cpp:759](src/pipeline/pipelineprocessor.cpp#L759) taps the popped frame before pipeline processing; if `display.record_raw`, writes to `display.raw_writer` using metadata from the currently routed `StreamSlot`.
- [x] Processed recorder on `DisplaySlot`: consumer at [pipelineprocessor.cpp:775](src/pipeline/pipelineprocessor.cpp#L775) writes post-pipeline output to `display.proc_writer` if `display.record_proc`.
- [x] Both auto-finalize on route change, source death, display blank.
- [x] File naming with `_main`/`_alt1`/`_alt2` inserted into existing pattern. Alts use `_res` only.
- [x] Source-death corner label per display, FPS-style. Main parented to `ui->CENTRAL`; alt1/alt2 are children of their Display widgets. Edge-triggered via `PipelineProcessor::source_state_changed(int, bool)`; steady-state truth lives on `DisplaySlot::source_dead`.
- [x] `SAVE_AVI_BUTTON` / `SAVE_FINAL_BUTTON` callsites drive per-display recorders directly via `m_router.display(...).raw_writer` / `proc_writer`. Alt button (`alt_display_control` case 2) drives the picked alt's `proc_writer`.
- [x] Export pipeline (`vid_out_proc`) kept separate from live recording — stays as a UserPanel member; metadata sourced from `m_router.stream(StreamId::S0).meta.*`.
- [ ] **Out of scope:** alt1/alt2 raw recording. The doc's "simultaneous raw + processed" promise was struck above; alts are processed-only per user decision.

**Walkthrough findings (post-C1a + C1b):**
- #1 Cam stop+restart crash repro — **verified pass**. Multiple stop→start cycles in a row no longer crash. Fix landed in C1a (`on_STOP_GRABBING_BUTTON_clicked` calls `curr_cam->stop_grabbing()` before `frames.close()` so SDK callbacks deregister before the queue tears down).
- #2–#8 (basic display, streams, swap, YOLO per-display) — pass.
- #9 / #10 Source-death overlay on stop — label not appearing. Diagnosis: on STOP_GRABBING the sequence `cam.stop_grabbing()` → `frames.close()` → `unroute()` runs in a single GUI-thread tick, so the consumer typically transitions through `snap.stream = None` (via the unroute gen-bump) before observing the closed-and-empty condition. The source-death check is gated behind `if (snap.stream == None) continue;` and thus skipped. User accepted the gap (not required to fix); a clean fix would be to emit `source_state_changed(d, true)` from the route-change branch when the previous snap stream was a real stream and the new snap is None.
- #11–#12 (per-display recording filenames, alt rec files) — pass.
- #13 Auto-finalize on swap — broken on swap-to-empty (alt1 empty, Main records https stream, click swap → nothing visible). Two compounded issues:
  - (a) Alt's consumer thread is created only on `start_static_display(display_idx, ...)`. With Alt empty, no consumer exists. Post-swap the route points at Alt but nothing pops the queue → no render.
  - (b) Auto-finalize on the consumer side released the writer but never told the GUI: `record_raw` stayed `true`, button stayed `STOP`, temp file never moved to `save_location`.
  - **Fix A2:** `swap_grab_thread_display` ensures both target consumers exist before swapping. If absent, creates + starts a `ConsumerThread`. FloatingWindow visibility is left alone — alt windows are user-controlled (Alt+1 / Alt+2) and swap doesn't auto-show.
  - **Fix B1:** `PipelineProcessor::recorder_finalized(int display_idx, int kind, QString temp_path)` signal emitted on every auto-release (route change + source death). UserPanel slot moves temp to save_location, flips `record_raw`/`record_proc` to false, resets `SAVE_AVI_BUTTON` / `SAVE_FINAL_BUTTON` / `RECORD_ALT[12]_BTN` text. Path tracked on `DisplaySlot::raw_path` / `proc_path` so consumer can pass it through.
  - **Drive-by:** alt-rec `vid_out` metadata was reading `m_router.stream(StreamId(display_idx)).meta` (S1/S2 hardcoded). Now reads from `m_router.snapshot(DisplayId(display_idx)).stream` so post-swap recording uses the actually-routed stream's metadata.
- #14–#24 (auto-finalize on source death, 3D row, picker, threshold sync, brightness disabled) — pass.

**C2 — FfmpegSource extraction (medium)**
- [x] Extracted the FFmpeg lambda body into [`src/pipeline/ffmpegsource.{h,cpp}`](src/pipeline/ffmpegsource.cpp) as `class FfmpegSource : public QObject`. API: `start(StreamId, QString url, bool format_gray)`, `stop(int timeout_ms = 2000)`.
- [x] Signals: `opened(StreamId, FfmpegSource::VideoInfo)`, `frame_decoded(StreamId, cv::Mat)`, `failed(StreamId, QString reason)`, `finished(StreamId)`. All cross-thread (Qt::QueuedConnection by default). Metatypes registered for `StreamId`, `DisplayId`, `FfmpegSource::VideoInfo`, and `cv::Mat`.
- [x] UserPanel handles `opened` on GUI thread (`UserPanel::on_ffmpeg_opened`): calls `start_static_display(w, h, is_color, display_idx, 8, -2)` which tears down any previous consumer, sets `StreamSlot::meta`, creates a fresh `ConsumerThread`, and routes the slot. Then writes the demuxer-extracted fps into `StreamSlot::meta.fps` and emits `send_double_tcu_msg` for Main only.
- [x] `failed` and `finished` slots call `m_router.stream(target).frames.close()` + `frame_info.close()` to wake the consumer.
- [x] Detached thread + 30 s interrupt heuristic replaced. The new worker is `std::thread` joined in `stop()`; `InterruptCtx` carries a pointer to the atomic stop flag and a `last_activity` timestamp. FFmpeg interrupt callback returns 1 when either the stop flag is set OR no IO for 30 s. The hard 30 s ceiling is preserved because the stop flag alone doesn't bound a connect() that wedges silently — the legacy heuristic still earns its keep there.
- [x] FFmpeg bug fixes applied earlier carried over verbatim: shared_ptr address-of deleter for `buffer` (so the late `av_malloc` is freed), receive-loop drain for B-frames, `init_filter_graph` leak guards (avfilter_inout_free no-op on NULL), HW→SW codec retry on `avcodec_open2` failure, PTS throttle reading `frame->pts` not `sw_frame->pts`.
- [x] Lifecycle handoff: `UserPanel::~UserPanel` calls `m_ffmpeg[i]->stop()` BEFORE tearing down consumers, so the worker threads stop pushing into queues that are about to disappear.
- [x] `load_video_file` `else if (display)` branch now just dispatches: `m_ffmpeg[display_idx]->start(StreamId(display_idx), filename, format_gray)`. The legacy lambda body kept for the export path (`display=false`) since export uses the `process_frame` callback → `vid_out_proc.write()` shape that doesn't fit the signal/slot model.
- [ ] **Deferred:** legacy lambda cleanup. The export branch still contains all the `if (display)` dead-code paths that used to handle the live case. Removing them would shorten the lambda by ~80 lines but adds no behavior change; left for a future cleanup commit.

**C2 walkthrough findings:**
- #1 RTSP — **verified pass**. Same `FfmpegSource` code path as HTTPS; `rtsp_transport=tcp` + tuned `analyzeduration` / `probesize` / `max_delay` block applied.
- #2–#4 (HTTP/HLS, local file, alt displays) — pass.
- #5 Reload same display with new file — **broken**: user reports old + new sources alternating in the display at >10 Hz. Diagnosis is unconclusive from code-reading alone (every traced path is sequential), but the only mechanism that produces sustained alternation at that rate is concurrent emission from two workers. Working hypothesis: stale `frame_decoded` events from worker A still queued in the GUI event loop after `stop()`'s `join()` returns, interleaving with worker B's fresh emits at the GUI's event dispatch rate.
- #6 Swap after live stream — pass.
- #7 App close while stream playing — pass; drive-by surfaced: `"Camera shutdown failed with error code: -2147483648"` (INT_MIN) logged even with no industrial cam connected. Fixed by gating `curr_cam->shut_down()` in [shut_down()](src/visual/userpanel.cpp) on `device_on` — if the device was never actually opened, skip the SDK call.
- #8 Export — skipped.
- #9 cv:// path — pass.

**#5 root cause — cross-source-type lifecycle gap:**
- Every source type (FfmpegSource, cv://, industrial cam, drag-dropped image) pushes to the same `StreamSlot::frames` queue, but each has an independent stop mechanism. Loading a new source while a different-type source was still alive left both producers feeding the queue concurrently; the consumer popped FIFO and rendered them interleaved at ~ (rate-A + rate-B) Hz.
- **Design clarification (user):** industrial cam on Main is *sticky* — to load a different source on Main while the cam is grabbing, the user must explicitly stop the cam OR swap the cam to an Alt display. The load itself does not silently tear down the cam.
- **Fix:**
  - Central `stop_all_sources_on_slot(int display_idx)` helper stops the slot's stream-style producers: `m_ffmpeg[i]->stop()` (joins) and `m_cv_run[i]->store(false)` (cv:// keep-alive flag). It does NOT touch the industrial cam.
  - Each cv:// dispatch captures a fresh `std::shared_ptr<std::atomic<bool>>` (separate from `DisplaySlot::consumer_run` — so a new load can reliably kill the cv:// loop without `start_static_display`'s `consumer_run = true` resurrecting it).
  - `load_video_file` checks `device_on` at the top when `display_idx == 0`; if the cam is initialized, refuses the load with a `QMessageBox::warning` asking the user to shut down or swap the cam first. Same `device_on` check + same wording in `load_image_file` (existing message updated to match). After that gate, calls `stop_all_sources_on_slot` to tear down any FFmpeg/cv:// producer on the target slot.
  - `on_START_GRABBING_BUTTON_clicked` calls `stop_all_sources_on_slot(0)` before binding the cam — so a video/webcam that was playing on Main is stopped first.
  - `load_image_file` (when `init=true`) calls `stop_all_sources_on_slot(0)` before pushing the static frame; the existing `device_on` warning still blocks image-over-cam.
  - `UserPanel::~UserPanel` stops every producer (`m_ffmpeg[i]->stop()`, `m_cv_run[i]->store(false)`, `curr_cam->stop_grabbing()` when grabbing) before tearing down consumers — so producer threads / SDK callbacks don't write into freed queue memory.
- Session counter on `FfmpegSource` retained as defensive (drops events whose session doesn't match the source's current session). qDebug logs in `FfmpegSource::stop()` and `run_thread` kept for observability.

**Prerequisites before C1a code starts:** (resolved)
- [x] Field-by-field migration table for `PipelineProcessor::SharedState` — walked through during the rewrite; final destinations encoded in `routing.h` (`StreamSlot` / `DisplaySlot`) and the `pipelineprocessor.h` stage signatures.
- [x] Config-file load policy resolved: per-display toggles (enhance / 3D / frame-avg / pseudocolor / per-display YOLO model selection) are GUI state, default-off on launch, never persisted. See [[feedback-per-display-toggles-not-persisted]].
- [x] Walkthrough cadence: per sub-commit, with mid-C1a check-ins as fixes (resize crash, deadlock, HLS burst) surfaced.

**Pre-start state:** working tree stashed (pre-T3 state) to keep C1a's commit focused on the refactor only; T1/T2 work and earlier FFmpeg bug fixes ride with the sprint-end commit. Stash gets popped after C1a lands.

### T4 — SETUP overlay: IP / serial-hex commit regression

Background: in `SETUP` mode, setting a new device IP via `IP_EDIT` did nothing — typing a value and pressing Enter or tabbing away never reached `curr_cam->ip_address(false, ...)`. The serial-comm "send raw hex bytes" field (`COM_DATA_EDT`) was silently broken the same way: `SetupOverlay::send_cmd(QString)` is defined but had zero callers.

**Root cause — common ancestor:** in the legacy `Preferences` dialog ([deleted in 4792c72](src/_deprecated/progsettings.cpp#L307-L313)), every line edit was committed on `Qt::Key_Enter` in the dialog's `keyPressEvent`. Three branches: `IP_EDIT → config_ip(false)`, `COM_DATA_EDT → send_cmd(text)`, and a generic fall-through `data_exchange(true)` that re-read every other field into Config in one sweep. When sprint 22 ported `Preferences` to `SetupOverlay` (a `QFrame` overlay, no longer a `QDialog`), `keyPressEvent` was dropped. The fall-through branch was replaced by per-field `editingFinished` lambdas in `SetupOverlay::bind()` — which is why `MAX_DIST` / `GAMMA` / `FISHNET_THRESH` / etc. still commit. But the two special-case branches had no `connect()` to port from, so both were silently lost.

**Fix:** two `returnPressed` connections in `SetupOverlay::bind()` ([setupoverlay.cpp:211](src/widgets/overlays/setupoverlay.cpp#L211), [setupoverlay.cpp:300](src/widgets/overlays/setupoverlay.cpp#L300)) — `IP_EDIT → config_ip(false)` and `COM_DATA_EDT → send_cmd(text)`. Chose `returnPressed` over `editingFinished` deliberately: both trigger destructive side effects (IP triggers a network reconfigure on the camera; COM_DATA_EDT actually sends bytes over the wire), so a focus-out commit on accidental click-away is wrong. Matches the legacy "Enter only" intent exactly.

Out of scope: re-auditing whether the `editingFinished` choice was correct for the other (benign-write) line edits in `bind()`. Their commits only write to Config — no immediate side effect — so the looser trigger is fine there. Flagged but not changed.

### T5 - RTSP latency in FFmpeg SW decode path

Background: RTSP streams decoded through the extracted `FfmpegSource` path showed much higher latency than other players (roughly 5-10 s) while using software decode. The RTSP input options were already tuned (`rtsp_transport=tcp`, reduced `analyzeduration` / `probesize`, `fflags=nobuffer`, `max_delay=200ms`), so the remaining delay was likely above the demuxer.

**Diagnosis:**
- `FfmpegSource::frame_decoded` was connected with `Qt::QueuedConnection` ([userpanel.cpp](src/visual/userpanel.cpp)), so every decoded `cv::Mat` became a GUI-thread event before it reached `StreamSlot::frames`.
- The intended live queue (`BoundedQueue<cv::Mat>` capacity 5) drops oldest frames, but it sat after Qt's unbounded event queue. If decode/conversion outran GUI event dispatch or consumer processing, stale frames accumulated before the bounded queue could drop them.
- RTSP frames were also going through the PTS throttle in `FfmpegSource` ([ffmpegsource.cpp](src/pipeline/ffmpegsource.cpp)). Live RTSP is already paced by network input; sleeping on PTS preserves any backlog instead of catching up.

**Fix:**
- Kept lifecycle signals (`opened`, `failed`, `finished`) queued to the GUI thread, but changed `frame_decoded` to `Qt::DirectConnection` so decoded frames are pushed straight into `StreamSlot::frames` / `BoundedQueue` from the FFmpeg worker thread.
- Added `FfmpegSource::mark_display_ready(session)`: after `on_ffmpeg_opened()` finishes `start_static_display(...)` and reopens the route queue, it acks the worker. The worker waits for that ack before emitting frames, avoiding the first-frame race where direct delivery could push into a closed or not-yet-routed queue.
- Gated the PTS sleep with `!is_rtsp_stream`, so RTSP stays network-paced. Local files and HTTP/HLS keep the existing PTS pacing behavior.
- Follow-up: removed `save_info` text overlay from raw recording. Raw recorder now writes the untouched source frame (plus only the required RGB->BGR conversion for color writer input); processed recording keeps the existing info overlay behavior.
- Follow-up stutter tuning: when RTSP playback showed correct average FPS (~29) but several tiny stalls per second, changed RTSP decoder threading from auto frame+slice threading to single-thread slice mode (`thread_count=1`, `thread_type=FF_THREAD_SLICE`). Rationale: FFmpeg frame threading can batch/reorder software-decoder output, producing burst/gap delivery even when the average pop rate is correct. Local files and HTTP/HLS keep the previous auto-threaded path.
- Follow-up swap/recording fixes:
  - Main raw/proc recorder open now reads metadata from `router.snapshot(DisplayId::Main).stream`, not hardcoded `StreamId::S0`. This fixes the swapped Alt1 RTSP -> Main case where Main recorded RTSP frames into a writer opened with industrial-cam metadata, producing a tiny header-only file.
  - Alt floating-window resize now uses `fw_display[display_idx - 1]`; the previous `fw_display[display_idx]` was off by one for Alt1 and out of bounds for Alt2.
  - `start_static_display()` only updates legacy global `device_type` / `start_grabbing` for Main display loads. Starting an Alt RTSP stream no longer rewrites Main/camera acquisition state.
  - `swap_grab_thread_display()` recreates a display consumer if the pointer exists but the thread is no longer running, covering intermittent swap-to-display-with-dead-consumer cases.
  - Camera stop now targets the display currently routing `StreamId::S0` instead of always stopping Main's consumer. This prevents a swapped camera stop from killing an unrelated Main stream.
  - Camera restart now force-cleans stale S0 state before `curr_cam->start_grabbing()`: if `start_grabbing`, `StreamSlot::producer_run`, or router state still says S0 is active, START first calls `curr_cam->stop_grabbing()`, closes/clears S0 queues, stops the display currently routing S0, unroutes S0, and resets `start_grabbing=false`. This covers the field case where an industrial cam swapped to Alt stops underneath the app, is swapped back to Main, and then crashes on restart because the SDK is still half-live.

**Verification:**
- `git diff --check` passed, aside from the repo's existing CRLF normalization warnings.
- Local compile was attempted with `cmake --build build-Desktop_Qt_5_12_12_MSVC2015_64bit-RelWithDebInfo --config RelWithDebInfo --target GLI_user_panel`, but the configured generator requires `jom`, which was not available in the shell. Runtime RTSP verification still needs to be done in the Qt/MSVC environment.

### T6 - Addon plugin floating panel

Background: the legacy addon hook lived inside the console/addon page and was not suitable for the monitor workflow. User wanted a monitor-only floating panel, opened from a dedicated top-bar addon button, with the panel resizable like the panorama panel but behaving like the existing Optics / TCU / Console floating windows.

**Fix:**
- [x] Added a resizable `AddonPanel` floating window hosted on the monitor view.
- [x] Moved addon activation to a top-bar button placed before the URL button; current icon is temporary and can be replaced later.
- [x] Default position/size now matches the main display rectangle.
- [x] ~~Hard-wired the current plugin target to `plugins/map_ais/plugin_map_ais.dll`, matching the requested `plugin_map_ais` name.~~ Superseded by T14: the panel plugin is whichever DLL under `plugins/` declares `"kind": "panel"`.
- [x] Loaded the DLL through `QPluginLoader`, verified it implements `PluginInterface`, then calls `create(panelHost)`, `attach(host)` (since T14) and `start()`.
- [x] Kept the panel hidden outside monitor mode and restored/fitted the plugin child when the panel is shown or resized.
- [x] Added short implementation guidance for external programs that want to implement the plugin interface.

### T7 - Theme / translation polish

Status: DONE (dark/light parity is a visual check, user-driven).

- [x] Theme work: `ThemeManager` ([thememanager.h](src/util/thememanager.h)) substitutes tokens from `:/themes/<name>/{dark,light}.json` into the two QSS files; `gruvbox`, `nord`, `graphite` shipped; selector in Setup > Presets writes `ui.theme` / `ui.color_mode` and restyles live. The old "theme" naming now means colour mode throughout (`app_color_mode`, `apply_color_mode()`, `stylesheet_dark/light`), and window-control + addon icons come in per-mode variants.
- [x] Custom widgets expose their colours as `Q_PROPERTY` so QSS drives them: `TimingBarWidget`, `JoystickWidget`, `ArcSliderWidget`, `UnlockBar`, `DotIndicator`, `PtzRadarWidget`.
- [x] Translation work: runtime strings wrapped (`tr()` sites in `src` 87 → 189); `lupdate.bat` / `lrelease.bat` added; `zh_cn.ts` 303 → 411 entries, none unfinished.
- [ ] Dark/light visual parity: user-driven walkthrough.

### T8 - PTZ panorama scan mode

Status: DONE.

Background: the grid scan walks a PTZ lattice with the TCU stepping at each stop. Field work wanted a single horizontal sweep at a fixed elevation that stitches the stops into a preview, with the step derived from the lens FOV at the current zoom rather than typed in.

- [x] `ScanParams` gained `ptz_scan_mode` (grid / panorama), the panorama fields (start / optional end / fixed V / step / count / direction / wait / zoom / fusion mode) and a `panorama_zoom_fov` calibration table ([scanparams.h](src/pipeline/scanparams.h)). `get_panorama_route()`, `get_panorama_fov()` (interpolated from zoom), `update_panorama_fov_from_zoom()`, sweep / stop-count helpers.
- [x] Grid scan dimensions individually switchable (`scan_delay_enabled` / `scan_gate_width_enabled` / `scan_ptz_enabled`); a disabled dimension keeps the device value active at scan start. Gate width now has its own route (`get_gate_width_route()`, `ScanController::get_scan_gate_width_route()`).
- [x] SCAN overlay: mode radios, per-card enable checkboxes, a panorama card with CW/CCW direction and a fusion combo ([scanoverlay.h](src/widgets/overlays/scanoverlay.h)).
- [x] Pipeline: `PanoramaCanvas` accumulates each stop into a degree-indexed canvas with `advance_panorama_scan_main()`; fusion modes none / gain / matching; preview emitted via `panorama_preview_updated` and saved with the scan output.
- [x] `PanoramaPanel` floating window ([panoramapanel.h](src/widgets/panels/panoramapanel.h)): zoom, pan, reset, user-resizable; generating / generated status.
- [x] Tests: seven new `test_scanparams` cases (gate-width route, panorama route default / endpoint / CCW wrap / step fallback / ending-H, zoom-FOV interpolation).

### T9 - PTZ radar widget

Status: DONE.

- [x] `PtzRadarWidget` in the right stack ([ptzradarwidget.h](src/widgets/sidebar/ptzradarwidget.h)) draws the current pan / tilt on a ring; fed from `DeviceManager::ptz_angle_changed`.
- [x] Zero-angle offset `device.ptz_zero_angle_h` in config, with `DeviceManager::to_display_ptz_angle_h()` / `to_device_ptz_angle_h()` as the single conversion pair.
- [x] North reference: `setHeading(deg, valid)` is source-agnostic (the 709 addon feeds it today; anything else may). The reference expires 5 s after the last valid sample, so a dead feed does not leave a stale north on the dial. Toggle button on the widget.

### T10 - Closed-loop PTZ target follow

Status: DONE. One design item carried (see below).

Background: the UDP turret takes velocity commands and `TargetEstimator::offsetFromCentreDeg()` already converts a box to angular error, so a PID on that error closes the loop without per-zoom retuning.

- [x] `PtzFollowController` ([ptzfollowcontroller.h](src/controller/ptzfollowcontroller.h)): per-axis PID in degrees → deg/s with deadband, integral clamp, derivative filter, coast-then-disengage on missing detections, pause without losing state, own-motion compensation from the last command (the 530 status rate bytes are deliberately not parsed).
- [x] Actuator path: `DeviceManager::ptz_move_velocity()` / `ptz_follow_offset()`; `UDPPTZ` gained `apply_manual_velocity` / `apply_follow_target` / `apply_angle_target` with thread-safe dispatch ([udpptz.h](src/port/udpptz.h)).
- [x] Engagement: auto-follow checkbox on the GENERAL tab (`ImgProcWidget::auto_follow_changed`) follows the first confirmed track; PTZ-tool click with `closed_loop` engages a specific one; any manual PTZ / STOP press disengages and notifies addons (`notify_operator_took_turret`).
- [x] `PidTuningWindow` (Ctrl+P): gains and limits apply immediately, save separately; live error / output telemetry. Settings persisted as `pid_follow`.
- Carried: manual VL/IR lens holds and follow commands share `UDPPTZ::current_operation` with no arbiter; today the follow loop pauses on operator input, but a held lens operation arriving mid-follow is not sequenced.

### T11 - Operator-designated follow gate

Status: DONE.

Background: follow should also work with no detector loaded, from a region the operator draws.

- [x] `Display`: Alt+drag in PTZ mode rubber-bands a gate (`gate_selected`), a plain click drops it (`gate_cleared`); both drawn in `paintEvent` ([display.h](src/widgets/display.h)).
- [x] `GateTracker` ([gatetracker.h](src/yolo/gatetracker.h)): Lucas-Kanade optical flow with optional forward-backward check, or normalized cross-correlation template matching with optional multi-scale search and an untouched anchor patch for recovery. Both estimate scale. Lives on `DisplaySlot`, touched only by the pipeline thread; the GUI hands requests over under `gate_mutex`.
- [x] SETUP > Gate Tracking group selects the method and options; persisted as `gate_tracking`. Gate follow engages the PID loop under the reserved `GateTrackId`.

### T12 - Target tracking, class table and estimation

Status: DONE with open items.

- [x] `TargetTracker` ([targettracker.h](src/yolo/targettracker.h)): greedy IOU association with a centroid gate, confirm-after-N, coast-then-drop, one-shot `justLost`. No motion model by design. Per display, on `DisplaySlot`.
- [x] `DetectedTarget` ([detectedtarget.h](src/pipeline/detectedtarget.h)) + `PipelineProcessor::main_targets_updated`: id, class, source-frame geometry; feeds the addon host's `publish_targets`.
- [x] `TargetClassTable` ([targetclasstable.h](src/util/targetclasstable.h)): per-model class attributes (height in m, 709 type / category) attached to names the model files define; entries survive model swaps. `TargetClassWindow` (Ctrl+T) edits them, holds the operator-entered zoom (the device reports none) and the range-on-labels toggle.
- [x] `TargetEstimator` ([targetestimator.h](src/util/targetestimator.h)): range from class height and box height, tangent-projection angular offset, hull→geographic rotation (inverse of the G1 rotation). Range printed on YOLO labels when enabled; class heights snapshotted onto `DisplaySlot` under `label_mutex` so the pipeline needs no table access.
- [ ] **Open:** the addon-host adapter's range path ([userpanel.cpp:200](src/visual/userpanel.cpp#L200)) builds `Optics` with `pow(..., 1/2)`; integer division makes the exponent 0, so the sensor diagonal is always 1. Carried from the 709 report builder, flagged, not yet fixed.
- [ ] **Open:** range accuracy at high zoom unverified against a known-distance target; `sensor_diagonal_mm` is the nominal 1/1.8" value.

### T13 - LightNAFNet learned enhancer

Status: DONE.

- [x] `NafNetEnhancer` ([nafnetenhancer.h](src/image/nafnetenhancer.h)) wraps the delivered gray model (others/jiaofu) through ONNX Runtime CUDA as a frame-average option; colour frames are enhanced per channel. Full-frame preprocessing per the delivery's MODEL_CONTRACT so output matches input size.
- [x] The "LightNAFNet" option is added to the frame-average combo only when `NafNetEnhancer::supported()` passes (model file next to the exe, CUDA provider listed), so installations without it see no change. Session created lazily per `DisplaySlot` on first selection; load failure is sticky.
- Note: `models/lightnafnet_gray_best.onnx` is not copied by the build; it has to be placed next to the executable by hand (root cause of the "no effect" report during the sprint).

### T14 - Addon host and runtime plugin loading

Status: DONE. Addon-side behaviours carried in the addon project.

Background: UDP709 was first built into this project behind a bridge, then behind a CMake option. Decision: addons are separate projects, discovered at runtime, and this project ships no protocol code.

- [x] `PluginInterface` v2 ([plugininterface.h](src/plugins/plugininterface.h)): `create(parent)` / `attach(host)` / `start()` / `stop()`, IID `com.zkss.glass.addons.plugin-interface/2`. Copies in each addon project are verbatim.
- [x] `AddonHost` ([addonhost.h](src/plugins/addonhost.h)): the service surface an addon gets: PTZ angles / velocity / stop / follow-centre, lens press / release, channel and laser state, TCU delay / gate width / laser width / gain, enhance and frame-average, target class lookup, offset and range, heading reference; publishers for targets, PTZ angle, operator-took-turret / lens, tracking state. Implemented by a private `UserPanel::AddonHostAdapter`.
- [x] `UserPanel::load_addons()` scans `<exe>/plugins/<name>/*.dll`, reads `QPluginLoader::metaData()` without instantiating, treats `"kind": "window"` plugins as autostarted services and keeps the first `"kind": "panel"` plugin for the T6 AddonPanel button.
- [x] Map/AIS plugin (`Demo_map_AIS`) migrated to v2 (`attach` / `stop`, `plugin.json`).
- [x] UDP709 moved wholesale to the sibling project `Addon_udp709` (plugin DLL, standalone tool, the two codec / service tests, the Python test tool); `protocol709/`, `udp709window`, the `Udp709Settings` config section and the `ENABLE_UDP709` option are gone from this repo. Addon settings live in `plugins/udp709/udp709.json`.
- Carried (addon project): targets are reported whenever the socket is bound and the form is not manual, with no operator toggle; reports carry a zero attitude when no valid one has arrived.

### T15 - TCU command flow control and delay floor

Status: DONE.

Background: holding a delay / gate-width key made the slider and value jump back and forth. Cause: the port echoes every completed write back through `update_tcu_params`, so the echo of an older command overwrote the newer local value, and the first throttle attempt could drop the last step of a burst.

- [x] Per-parameter `Mailbox` in `TCUController` ([tcucontroller.h](src/controller/tcucontroller.h)): one command in flight for EST_DIST / EST_DOV, later requests collapse into a single pending send released by the echo; a request is superseded, never dropped. Echoes that do not match the command in flight (superseded values, the AB-lock duplicate) do not touch the UI. Watchdog re-arms on a lost echo; a full readback resyncs. AB / AB-lock fan-out in `TCU::set_user_param` untouched.
- [x] `tcu.min_delay` (ns, default 1000) as a TCU timing floor, unaffected by light speed; `min_delay_dist()`, `delay_slider_min_changed`, clamps in `apply_delay` / `update_delay` and the DIST dialog. `default.json` updated.
- [x] `apply_laser_width` / `apply_delay` / `apply_gatewidth` slots for external control (addon host) with UI follow-through and clamping.

### T16 - Industrial camera hardening

Status: DONE for the tier-one fixes; root causes for the long-connection break carried.

Background (field report): connections longer than ~10 min sometimes break; the first connect + start after launch showed no image; stop + restart after a broken connection crashed. Evidence: MVS SDK log (socket mode after "Driver Version Match failed", jumbo packets, heartbeat timeouts, write timeouts, packet loss) and six crash dumps: August dumps were an integer divide-by-zero in the pipeline crop with display size 0; July dumps were access violations in `cv::Mat::clone` / `convertTo` on a freed frame buffer.

- [x] `Cam::is_connected()` / `reconnect()` ([cam.h](src/cam/cam.h)); `mvcam` registers an exception callback that flags `reconnect_required`, `release_handle()` enforces StopGrabbing → CloseDevice → DestroyHandle everywhere, `reconnect()` stops before closing, `stop_grabbing()` is gated only on the handle, all `MVCC_*` result structs zero-initialised, `frame_size` returns 0 on failure. START checks connection first and reconnects.
- [x] `ebuscam` streaming thread moved to `std::thread` + atomic flag with the same connection hooks.
- [x] Start path reports SDK error codes in the message boxes; aborts with a message on a zero frame size; region dialog and resize guard against zero increments; `render_and_display` returns on a zero display size (the August crash).
- [x] Logging: `qInstallMessageHandler` writes to `GLI_logs/` and chains to the console.
- [ ] Carried: automatic disconnect handling and retry loop on the exception callback; the frame-buffer race behind the July dumps (SDK callback writing into a buffer the consumer still reads); SDK DLL (4.5.1.1 beside the exe) vs installed driver (4.8.0.3) mismatch on the deployment PC; heartbeat / packet-size tuning.

### T17 - UI polish

Status: DONE.

- [x] Fonts: base pixel size raised by 1 px through the QSS (11 → 12, 10 → 11, 9/8 → 10), Monaco 12 / Consolas 13, `sans_heiti` picked from system CJK sans fallbacks (smileysans removed); numeric edits sized from font metrics.
- [x] `INFO_CHECK` defaults on. Optional custom title label over the display (`ui.custom_title`).
- [x] Console panel: `QTabWidget` replaced by the classic three-selector `MiscSelection` switcher ([consolepanel.h](src/widgets/panels/consolepanel.h)).
- [x] TopBar: addon button (T6), colour-mode button, window controls grouped with state sync.
- [x] PID aim: `point_ptz_to_target` uses the box centre.

### T18 - Config and runtime additions

Status: DONE.

- [x] New sections: `video.default_urls` (URL dialog presets), `automatic.enabled` (actions run once per launch via `run_automatic_actions`), `debug.generate_dmp`, `panorama_optics` (sensor diagonal + zoom→FOV table), `pid_follow`, `gate_tracking`; `ui.theme` / `ui.color_mode` / `ui.custom_title`; `tcu.min_delay`; `device.ptz_zero_angle_h`.
- [x] Per-display YOLO model selection dropped from config (`main/alt1/alt2_display_model`) per T3; `image_proc.model_idx` renamed `fishnet_model_idx`.
- [x] Version 0.12.0 (`version.h`, `gist.rc`).

---

## Definition of Done for Sprint 25

- [x] T1–T5 display routing, YOLO status, FfmpegSource, SETUP commit fix, RTSP latency
- [x] T6 / T14 addon panel + runtime plugin loading; UDP709 and map/AIS as external projects
- [x] T7 theme system and `tr()` sweep
- [x] T8–T13 panorama, radar, follow, gate, tracking / classes, NAFNet
- [x] T15 TCU flow control and delay floor
- [x] T16 camera tier-one fixes
- [x] T17 / T18 UI polish, config, version bump
- [x] Main program rebuilt; both plugins loaded and exercised; UDP709 addon tests pass
- [ ] `test_config` rebuilt against the September config changes (binary predates them)
- [ ] Dark/light parity walkthrough (T7)
- [ ] Feature test matrix rows for the sprint 25 features

## Carried to Sprint 26+

- T12: range formula integer division; range verification at high zoom.
- T16: camera disconnect handler / retry, frame-buffer race, SDK DLL vs driver match, heartbeat / packet tuning.
- T10: UDPPTZ held-operation arbiter between manual lens holds and follow.
- T14 (addon project): target-report toggle; attitude handling when none is valid.
- T13: post-build copy of the NAFNet model; enhance parity on the export path.
- T3 C2 deferred: dead live-path branches in the export lambda of `load_video_file`.
- T2 flagged: model combobox handlers do not call `update_processing_params()`.
- T3 walkthrough #9/#10: source-death label not shown on STOP; HLS inter-segment stalls.
- `curr_cam->start()` and the IP-address calls index the device list with `tcu.type` ([userpanel.cpp:3665](src/visual/userpanel.cpp#L3665), [userpanel.cpp:4120-4133](src/visual/userpanel.cpp#L4120-L4133)); BayerGB display path incomplete (`// TODO` at the pixel-type switch).
- Keep PDBs with each release build so future dumps resolve.
- Sprint 26: style unification across the refactored code.
