# Sprint 7 Progress: Config as Single Source of Truth

## Prerequisite: Characterization tests TC-CF-*
Status: SATISFIED

Phase 4 test_config (9 cases) covers JSON round-trip, defaults, partial load, reset. Sprint 5 thread safety fixes completed.

## Analysis

### Current architecture

Three components hold settings state with bidirectional sync:

```
Config (JSON persistence)  ←→  Preferences (UI dialog + member variables)  ←→  UserPanel (reads pref->)
         ↕                          ↕ data_exchange()                             ↕
    default.json               UI widgets                              grab_thread_process
```

- `Config::ConfigData` holds: COM ports, network, UI theme/language, camera, TCU, device, YOLO
- `Preferences` holds ~65 public member variables, of which ~20 overlap with Config
- `syncPreferencesToConfig()` (userpanel.cpp:3445): copies ~20 pref members → Config (before save)
- `syncConfigToPreferences()` (userpanel.cpp:3506): copies Config → ~20 pref members + UI (after load)
- UserPanel reads `pref->` in ~234 places, `config->` in ~44 places

### The duplication problem

**Fields duplicated between Config and Preferences (~20):**
- TCU: device_idx↔type, auto_rep_freq, auto_mcp, ab_lock, hz_unit, base_unit, max_dist, delay_offset, max_dov, gate_width_offset, max_laser_width, laser_width_offset, ps_step[4], laser_on
- Device: symmetry↔flip, ebus_cam↔ebus, share_port↔share_tcu_port, ptz_type

**Fields in Preferences only, NOT persisted (~45):**
- Save options: save_info, custom_topleft_info, save_in_grayscale, consecutive_capture, integrate_info, img_format
- Image processing: accu_base, gamma, low_in, high_in, low_out, high_out, dehaze_pct, sky_tolerance, fast_gf, colormap
- 3D gated: lower_3d_thresh, upper_3d_thresh, truncate_3d, custom_3d_param, custom_3d_delay, custom_3d_gate_width
- ECC temporal denoising: ecc_window_mode, ecc_warp_mode, ecc_fusion_method, ecc_backward, ecc_forward, ecc_levels, ecc_max_iter, ecc_eps, ecc_half_res_reg, ecc_half_res_fuse
- Model/fishnet: model_idx, fishnet_recog, fishnet_thresh
- Runtime: rotation, pixel_type, cameralink, port_idx, use_tcp, dist_ns, laser_enable

### Field categorization

**Persist in Config (user preferences that should survive restart):**
- Save options: save_info, custom_topleft_info, save_in_grayscale, consecutive_capture, integrate_info, img_format
- Image processing: accu_base, gamma, low_in, high_in, low_out, high_out, dehaze_pct, sky_tolerance, fast_gf, colormap — loaded as defaults, not auto-saved on change
- 3D gated: lower_3d_thresh, upper_3d_thresh, truncate_3d, custom_3d_param, custom_3d_delay, custom_3d_gate_width
- ECC temporal denoising: ecc_window_mode, ecc_warp_mode, ecc_fusion_method, ecc_backward, ecc_forward, ecc_levels, ecc_max_iter, ecc_eps, ecc_half_res_reg, ecc_half_res_fuse
- Model/fishnet: model_idx, fishnet_recog, fishnet_thresh
- Device: rotation, pixel_type (user preferences, not runtime)

**Stay in Preferences (runtime/session state):**
- dist_ns: physical constant computed from LIGHT_SPEED_M_NS (0.15 m/ns in air, changes when underwater mode enabled)
- port_idx, use_tcp: serial port UI selection state
- laser_enable: dead code (declared but never used anywhere) — remove
- cameralink: compile-time feature guarded by CAMERALINK macro, must NOT be loaded from config when macro is 0 (missing library would crash)

**Laser safety (CRITICAL):**
- `laser_on` in TCUSettings is currently persisted and loaded from JSON
- Loading a saved config with `laser_on != 0` triggers: setChecked → buttonToggled → toggle_laser → laser_toggled signal → (via titlebar.cpp:95) → setup_laser → TCU laser enable command
- This means saved configs CAN auto-enable lasers on program startup — safety hazard
- Fix: force `laser_on = 0` after every config load, remove laser_on from JSON persistence

### Approach: full migration with incremental delivery

Add settings to Config in groups. Each group: expand ConfigData → update save/load → redirect pref→Config → update UserPanel reads.

Group 1: Already in Config (TCU, device) — eliminate duplication, fix laser safety, expand DeviceSettings
Group 2: Save options — add to Config, persist
Group 3: Image processing + ECC + 3D + model/fishnet — add as ImageProcSettings, persist as defaults

Remaining Preferences members after refactoring: dist_ns, port_idx, use_tcp, split, cameralink (compile-time), laser_grp (QButtonGroup*), pressed/prev_pos (UI helpers).

### JSON file size impact

Current `default.json` with `dump(4)` has ~50 fields across 11 sections, roughly 2-3 KB. Adding SaveSettings (6 fields) and ImageProcSettings (29 fields including ECC) adds ~1-1.5 KB. Total ~4 KB — negligible for IO. For comparison, one 640x512x16-bit gated image frame is 640 KB. `load_from_file()` is called once at startup; `auto_save()` fires only on 4 manual user actions (theme toggle, language switch, COM port connect, PTZ port connect). No per-frame config IO.

## Task breakdown

### Task 7.0: Laser safety fix
Status: DONE

Force `laser_on = 0` after every config load to prevent auto-enabling lasers on startup.

Two changes:
1. In `tcu_settings_from_json()`: do NOT read `laser_on` from JSON (skip the field, leave at default 0)
2. In `tcu_settings_to_json()`: do NOT write `laser_on` to JSON (remove the line)

This ensures that even if an old config file has `laser_on` set, it will be ignored on load. The laser state is only ever set through explicit user interaction (checkbox → buttonToggled → toggle_laser → laser_toggled signal chain).

Files: `src/util/config.cpp`

### Task 7.1: Add SaveSettings, ImageProcSettings, expand DeviceSettings in Config
Status: DONE

Add two new structs and expand DeviceSettings. Defaults must match Preferences constructor (preferences.cpp:4-67).

```cpp
struct SaveSettings {
    bool save_info = true;
    bool custom_topleft_info = false;
    bool save_in_grayscale = false;
    bool consecutive_capture = true;
    bool integrate_info = true;
    int img_format = 0;
};

struct ImageProcSettings {
    // basic
    float accu_base = 1.0f;
    float gamma = 1.2f;
    float low_in = 0.0f;
    float high_in = 0.05f;
    float low_out = 0.0f;
    float high_out = 1.0f;
    // dehaze
    float dehaze_pct = 0.95f;
    float sky_tolerance = 40.0f;
    int fast_gf = 1;
    // colormap
    int colormap = 2; // cv::COLORMAP_JET
    // 3D gated
    double lower_3d_thresh = 0.0;
    double upper_3d_thresh = 0.981;
    bool truncate_3d = false;
    bool custom_3d_param = false;
    float custom_3d_delay = 0.0f;
    float custom_3d_gate_width = 0.0f;
    // model/fishnet
    int model_idx = 0;
    bool fishnet_recog = false;
    float fishnet_thresh = 0.99f;
    // ECC temporal denoising
    int ecc_window_mode = 0;
    int ecc_warp_mode = 2;
    int ecc_fusion_method = 2;
    int ecc_backward = 20;
    int ecc_forward = 0;
    int ecc_levels = 1;
    int ecc_max_iter = 8;
    double ecc_eps = 0.001;
    bool ecc_half_res_reg = true;
    bool ecc_half_res_fuse = false;
};
```

Expand DeviceSettings with rotation and pixel_type:
```cpp
struct DeviceSettings {
    bool flip = false;
    bool underwater = false;
    bool ebus = false;
    bool share_tcu_port = false;
    int ptz_type = 0;
    int rotation = 0;    // new
    int pixel_type = 0;  // new
};
```

Add JSON serializers for SaveSettings, ImageProcSettings. Expand DeviceSettings serializer. Add `save` and `image_proc` to ConfigData struct.

Files: `src/util/config.h`, `src/util/config.cpp`

### Task 7.2: Redirect Preferences duplicate members to Config
Status: DONE

For the ~20 fields duplicated between Config and Preferences:
- Remove duplicate member variables from Preferences
- Replace `pref->max_dist` reads with `config->get_data().tcu.max_dist` in UserPanel
- Replace `pref->ptz_type` reads with `config->get_data().device.ptz_type` in UserPanel
- Same for all TCU and device fields

For the newly-Config-backed fields (save, image proc, ECC):
- Remove member variables from Preferences
- `data_exchange(true)`: write UI → Config instead of UI → members
- `data_exchange(false)`: read Config → UI instead of members → UI

Files: `src/visual/preferences.h`, `src/visual/preferences.cpp`, `src/visual/userpanel.cpp`

### Task 7.3: Replace `pref->` reads in userpanel.cpp with `config->` reads
Status: DONE

Audit all ~234 `pref->` accesses. Replace data reads with `config->get_data().*`:
- `pref->max_dist` → `config->get_data().tcu.max_dist`
- `pref->symmetry` → `config->get_data().device.flip`
- `pref->auto_mcp` → `config->get_data().tcu.auto_mcp`
- `pref->save_info` → `config->get_data().save.save_info`
- `pref->colormap` → `config->get_data().image_proc.colormap`
- etc.

Keep `pref->` for:
- `pref->ui->` widget access (Preferences still owns its UI)
- `pref->data_exchange()` calls
- `pref->init()`, `pref->display_baudrate()`, etc. (Preferences methods)
- Remaining Preferences members: dist_ns, port_idx, use_tcp, split, cameralink, laser_grp

Files: `src/visual/userpanel.cpp`

### Task 7.4: Simplify sync functions
Status: DONE

The sync functions cannot be fully removed because UserPanel owns state not in Config (COM port edit widgets, UI theme/language, camera settings, UDP PTZ). Simplified by:

1. Expanded `pref->data_exchange(false)` to populate ALL Config-backed widgets: device (FLIP_OPTION_LIST, ROTATE_OPTION_LIST, PTZ_TYPE_LIST, EBUS_CHK, SHARE_CHK, UNDERWATER_CHK), save (SAVE_INFO_CHK, CUSTOM_INFO_CHK, GRAYSCALE_CHK, CONSECUTIVE_CAPTURE_CHK, INTEGRATE_INFO_CHK, IMG_FORMAT_LST), TCU (TCU_LIST, HZ_LIST), plus existing image_proc/ECC/3D/laser widgets
2. Expanded `pref->data_exchange(true)` to read ALL Config-backed widgets back to Config (same widget set)
3. Removed 14 manual `pref->ui->` widget assignments from `syncConfigToPreferences()` — now handled by `pref->data_exchange(false)`
4. Added `pref->data_exchange(true)` call to `syncPreferencesToConfig()` to capture pending text field changes

After simplification:
- `syncPreferencesToConfig()`: syncs UserPanel-owned state (COM, network, UI, camera) + calls `pref->data_exchange(true)` for text fields
- `syncConfigToPreferences()`: applies Config to UserPanel-owned state (COM, network, UI, camera) + calls `data_exchange(false)` + `pref->data_exchange(false)` for all widgets

Files: `src/visual/preferences.cpp`, `src/visual/userpanel.cpp`

### Task 7.5: Verify signal flow
Status: DONE

Audited all 28 Preferences signals. All are connected via old-style SIGNAL/SLOT macros in titlebar.cpp:74-97 to UserPanel slots (via `signal_receiver`). No formal observer pattern needed.

Signal flow after refactoring:
- Lambda handlers in Preferences constructor: write to `m_config->get_data().*`, then emit signal
- Lambda guards (`if (m_config)`) prevent Config writes during construction (before `set_config()`)
- Signal emissions during construction are harmless (no slots connected yet — connections happen later in titlebar.cpp)
- On config load: `pref->data_exchange(false)` sets combo boxes → `currentIndexChanged` fires → lambdas write same value to Config + emit signals → UserPanel slots apply settings

Pre-existing issue (not caused by refactoring):
- `max_laser_changed(float)` signal is declared (preferences.h:77) and emitted (preferences.cpp:184) but has no connection in titlebar.cpp. The max_laser_width value is used directly via `config->get_data().tcu.max_laser_width` in userpanel.cpp:3786.

Files audited: `src/visual/preferences.h`, `src/visual/preferences.cpp`, `src/widgets/titlebar.cpp`, `src/visual/userpanel.cpp`

## Summary of files changed

| File | Action |
|------|--------|
| `src/util/config.h` | Added SaveSettings, ImageProcSettings structs; expanded DeviceSettings (flip bool→int, added rotation, pixel_type) |
| `src/util/config.cpp` | Added JSON serializers for save, image_proc; expanded device serializer with backward-compat flip; removed laser_on from TCU persistence |
| `src/visual/preferences.h` | Removed ~55 Config-backed member variables; removed dead laser_enable; added Config* m_config, set_config() |
| `src/visual/preferences.cpp` | Rewrote data_exchange() to use Config for all settings (device, save, TCU, image_proc, ECC, 3D, laser); updated lambdas to write Config with null guards |
| `src/visual/userpanel.cpp` | Replaced ~70+ pref-> data reads with config->; simplified sync functions; added pref->set_config(config) |

## Risks

- **Laser safety (CRITICAL)**: `laser_on` must not be loaded from saved configs. Task 7.0 addresses this by removing laser_on from JSON persistence entirely.
- **Thread safety**: grab_thread_process reads pref-> from worker thread. Replacing with config-> has the same thread safety profile (both are plain struct reads). Sprint 5 documented this as acceptable for now.
- **Default values**: New ConfigData struct defaults must match Preferences constructor initializer list exactly. Mismatches verified: save_info=true (not false), consecutive_capture=true (not false), integrate_info=true (not false), gamma=1.2 (not 1.0), high_in=0.05 (not 1.0), sky_tolerance=40 (not 0.1), fast_gf=1 (not 0), colormap=2/COLORMAP_JET (not 0), upper_3d_thresh=0.981 (not 255.0), fishnet_thresh=0.99 (not 0.5), ecc_warp_mode=2 (not 0), ecc_fusion_method=2 (not 0), ecc_backward=20 (not 3), ecc_forward=0 (not 3), ecc_levels=1 (not 3), ecc_max_iter=8 (not 50), ecc_half_res_reg=true (not false).
- **Signal flow**: Preferences signals (e.g., max_dist_changed) must still fire when user changes values in the dialog. Preferences writes new value to Config, then emits the signal.
- **Existing configs**: Old JSON files won't have the new sections. Config loader handles missing sections gracefully (uses defaults). This is the existing pattern — `from_json` methods check field existence.
- **cameralink**: Must NOT be added to Config. Compile-time feature (CAMERALINK macro). Loading it when library is absent would crash.
