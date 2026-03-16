# Sprint 6 Progress: PTZ Controller Unification

## Prerequisite: Characterization tests TC-PTZ-*
Status: SATISFIED

Phase 4 test_ptz (14 cases) covers Pelco-D protocol encoding, angle set/get, speed clamping, checksum. USBCAN and UDPPTZ are not testable without hardware (CAN device driver, UDP socket) — no ControlPort base class to stub.

## Analysis

### Current state
Three PTZ controller classes with independent implementations:
- `PTZ` (serial Pelco-D): inherits `ControlPort : public QObject`, angles as `double`, signal `ptz_param_updated(qint32, double)`
- `USBCAN` (CAN bus): inherits `QObject`, angles as `float`, signal `angle_updated(float, float)`
- `UDPPTZ` (UDP network): inherits `QObject`, angles as `float`, signal `angle_updated(float, float)`

UserPanel dispatches via `pref->ptz_type` switch in 8 locations:
1. `init_control_port()` — PTZ_COM_EDIT connection lambda (connect/disconnect)
2. `update_ptz_status()` — connection status display
3. `keyPressEvent()` — ANGLE_H_EDIT return key (set horizontal angle)
4. `keyPressEvent()` — ANGLE_V_EDIT return key (set vertical angle)
5. `ptz_button_pressed()` — 8-way directional control (~90 lines)
6. `ptz_button_released()` — stop movement
7. `set_ptz_angle()` — programmatic angle setting
8. `on_STOP_BTN_clicked()` — stop button

Three angle-update slots do the same thing:
- `update_ptz_params(qint32, double)` — from PTZ, updates ANGLE_H/V_EDIT
- `update_usbcan_angle(float, float)` — from USBCAN, updates ANGLE_H/V_EDIT
- `update_udpptz_angle(float, float)` — from UDPPTZ, updates ANGLE_H/V_EDIT

Scan loop (line 1719-1720) hardcodes `p_usbcan->emit control(USBCAN::POSITION/PITCH, ...)`.

### Design: IPTZController interface

Qt does not support multiple QObject inheritance. IPTZController must be a pure C++ abstract class (no Q_OBJECT). The three concrete classes already have QObject ancestry and define their own signals.

The interface defines pure virtual methods. For signal connections, a `ptz_qobject()` method returns the `QObject*` so UserPanel can connect to the common `angle_updated(float, float)` signal by name.

Direction enum: 9 values (STOP + 8 directions) matching the existing button grid layout (0=UP_LEFT through 8=DOWN_RIGHT).

```cpp
class IPTZController {
public:
    virtual ~IPTZController() = default;

    enum Direction {
        DIR_UP_LEFT = 0, DIR_UP = 1, DIR_UP_RIGHT = 2,
        DIR_LEFT = 3, DIR_SELF_CHECK = 4, DIR_RIGHT = 5,
        DIR_DOWN_LEFT = 6, DIR_DOWN = 7, DIR_DOWN_RIGHT = 8,
    };

    virtual void ptz_move(int direction, int speed) = 0;
    virtual void ptz_stop() = 0;
    virtual void ptz_set_angle(float h, float v) = 0;
    virtual void ptz_set_angle_h(float h) = 0;
    virtual void ptz_set_angle_v(float v) = 0;
    virtual float ptz_get_angle_h() const = 0;
    virtual float ptz_get_angle_v() const = 0;
    virtual bool ptz_is_connected() const = 0;

    // For signal connection: concrete class returns this as QObject*
    // All implementors must define: signal angle_updated(float h, float v)
    virtual QObject* ptz_qobject() = 0;
};
```

### Changes per concrete class

**PTZ**: Add `IPTZController` as second base. Implement all 8 methods:
- `ptz_move()`: maps Direction enum to existing `ptz_control(direction+1)` call, passes speed
- `ptz_stop()`: calls `ptz_control(PTZ::STOP)`
- `ptz_set_angle()`: calls `ptz_control(SET_H, h)` then `ptz_control(SET_V, v)`
- `ptz_set_angle_h()`: calls `ptz_control(SET_H, h)` only
- `ptz_set_angle_v()`: calls `ptz_control(SET_V, v)` only
- `ptz_get_angle_h/v()`: return `angle_h`/`angle_v` cast to float
- `ptz_is_connected()`: checks `get_port_status() & (SERIAL_CONNECTED | TCP_CONNECTED)`
- `ptz_qobject()`: returns `this`
- Add `angle_updated(float, float)` signal, emit alongside existing `ptz_param_updated`

**USBCAN**: Add `IPTZController` as second base. Implement:
- `ptz_move()`: maps Direction to LEFT/RIGHT/UP/DOWN control calls (existing logic from ptz_button_pressed case 1)
- `ptz_stop()`: calls `device_control(USBCAN::STOP, 0)`
- `ptz_set_angle()`: calls `ptz_set_angle_h(h)` then `ptz_set_angle_v(v)`
- `ptz_set_angle_h()`: calls `device_control(POSITION, h)` + `transmit_data(POSITION)`
- `ptz_set_angle_v()`: calls `device_control(PITCH, v)` + `transmit_data(PITCH)`
- `ptz_get_angle_h/v()`: return `position`/`pitch`
- `ptz_is_connected()`: return `connected`
- `ptz_qobject()`: returns `this`
- Already has `angle_updated(float, float)` signal

**UDPPTZ**: Add `IPTZController` as second base. Implement:
- `ptz_move()`: maps Direction to `set_velocities()` + `transmit_data(MANUAL_SEARCH)` (existing logic from ptz_button_pressed case 2)
- `ptz_stop()`: calls `set_velocities(0,0)` + `transmit_data(MANUAL_SEARCH)`
- `ptz_set_angle()`: calls `device_control(ANGLE_H, h)`, `device_control(ANGLE_V, v)`
- `ptz_set_angle_h()`: calls `device_control(ANGLE_H, h)` only
- `ptz_set_angle_v()`: calls `device_control(ANGLE_V, v)` only
- `ptz_get_angle_h/v()`: return `horizontal_angle`/`vertical_angle`
- `ptz_is_connected()`: return existing `is_connected()`
- `ptz_qobject()`: returns `this`
- Already has `angle_updated(float, float)` signal

### Changes in UserPanel

- Add `IPTZController* active_ptz` member (nullptr initially, set on connect)
- Replace 3 angle-update slots with one `update_ptz_angle(float h, float v)`
- Connect `angle_updated` from all 3 classes to `update_ptz_angle`
- Replace 8 ptz_type switches with calls through `active_ptz->ptz_move/stop/set_angle`
- Replace scan loop `p_usbcan->emit control(...)` with `active_ptz->ptz_set_angle()`
- Keep `pref->ptz_type` for PTZ_COM_EDIT connection logic (different connect/disconnect mechanisms) and for self-check (button 4, type-specific initialization sequences)

## Task 6.1: Define `IPTZController` abstract interface
Status: DONE

New file: `src/port/iptzcontroller.h`
Header-only abstract class with Direction enum (0-8 matching button grid), 8 pure virtual methods + `ptz_qobject()`.
Includes `ptz_set_angle_h(float)` and `ptz_set_angle_v(float)` for single-axis control.
Added to `CMakeLists.txt`.

## Task 6.2: Adapt `PTZ` to implement `IPTZController`
Status: DONE

Added `IPTZController` as second base class. Implemented 8 interface methods:
- `ptz_move()`: sets speed, then delegates to `ptz_control(direction + 1)` (Pelco-D enum mapping)
- `ptz_stop()`: delegates to `ptz_control(STOP)`
- `ptz_set_angle()`: delegates to `ptz_control(SET_H)` then `ptz_control(SET_V)`
- `ptz_set_angle_h()`: delegates to `ptz_control(SET_H)` only
- `ptz_set_angle_v()`: delegates to `ptz_control(SET_V)` only
- `ptz_get_angle_h/v()`: return `angle_h`/`angle_v` cast to float
- `ptz_is_connected()`: checks ControlPort status flags

Added `angle_updated(float, float)` signal. Emitted from `try_communicate()` and `ptz_control()` alongside existing `ptz_param_updated`.

Files changed: `src/port/ptz.h`, `src/port/ptz.cpp`

## Task 6.3: Adapt `USBCAN` to implement `IPTZController`
Status: DONE

Added `IPTZController` as second base class. Implemented 5 non-trivial methods:
- `ptz_move()`: maps Direction enum to LEFT/RIGHT/UP/DOWN `device_control()` calls
- `ptz_stop()`: delegates to `device_control(STOP, 0)`
- `ptz_set_angle()`: delegates to `ptz_set_angle_h()` then `ptz_set_angle_v()`
- `ptz_set_angle_h()`: `device_control(POSITION, h)` + `transmit_data(POSITION)`
- `ptz_set_angle_v()`: `device_control(PITCH, v)` + `transmit_data(PITCH)`

Inline implementations for `ptz_get_angle_h/v()`, `ptz_is_connected()`, `ptz_qobject()`.

Files changed: `src/port/usbcan.h`, `src/port/usbcan.cpp`

## Task 6.4: Adapt `UDPPTZ` to implement `IPTZController`
Status: DONE

Added `IPTZController` as second base class. Implemented 5 non-trivial methods:
- `ptz_move()`: converts direction + speed to velocity pair, calls `set_velocities()` + `transmit_data(MANUAL_SEARCH)`
- `ptz_stop()`: calls `set_velocities(0,0)` + `transmit_data(MANUAL_SEARCH)`
- `ptz_set_angle()`: delegates to `device_control(ANGLE_H)` then `device_control(ANGLE_V)`
- `ptz_set_angle_h()`: delegates to `device_control(ANGLE_H)` only
- `ptz_set_angle_v()`: delegates to `device_control(ANGLE_V)` only

Inline implementations for `ptz_get_angle_h/v()`, `ptz_is_connected()`, `ptz_qobject()`.

Files changed: `src/port/udpptz.h`, `src/port/udpptz.cpp`

## Task 6.5: Replace 3 angle-update slots with one
Status: DONE

Removed `update_usbcan_angle(float, float)` and `update_udpptz_angle(float, float)`.
Added `update_ptz_angle(float h, float v)` — normalizes negative angles to 0-360 range.
Connected `angle_updated(float, float)` from all 3 classes to `update_ptz_angle`.
Kept `update_ptz_params` for PTZ::NO_PARAM case only (speed sync on initial connection).
Set `active_ptz` pointer on connection status change for each PTZ type.

Files changed: `src/visual/userpanel.h`, `src/visual/userpanel.cpp`

## Task 6.6: Replace PTZ type switch in dispatch functions
Status: DONE

Replaced 5 ptz_type switches with `active_ptz->` calls:
- `ptz_button_pressed()`: ~90 lines → `active_ptz->ptz_move(id, speed)` (kept type-specific self-check for id==4)
- `ptz_button_released()`: 8 lines → `active_ptz->ptz_stop()`
- `set_ptz_angle()`: 12 lines → `active_ptz->ptz_set_angle(angle_h, angle_v)`
- `on_STOP_BTN_clicked()`: 5 lines → `active_ptz->ptz_stop()`
- `keyPressEvent` ANGLE_H_EDIT: → `active_ptz->ptz_set_angle_h(angle_h)` (single-axis only)
- `keyPressEvent` ANGLE_V_EDIT: → `active_ptz->ptz_set_angle_v(angle_v)` (single-axis only)

Single-axis methods prevent the behavior change where editing one angle would re-send the other axis to the device, potentially causing unwanted movement.

Kept ptz_type switch in:
- PTZ_COM_EDIT lambda (different connect/disconnect protocols per type)
- `ptz_button_pressed` id==4 (type-specific self-check sequences)
- `update_ptz_status()` (type-specific display styling: serial vs network)

Files changed: `src/visual/userpanel.h`, `src/visual/userpanel.cpp`

## Task 6.7: Update scan loop to use `IPTZController`
Status: DONE

Replaced `p_usbcan->emit control(USBCAN::POSITION/PITCH, ...)` with `active_ptz->ptz_set_angle(h, v)`.
Scan now works with any connected PTZ type, not just USBCAN.

Files changed: `src/visual/userpanel.cpp`

## Summary of files to change

| File | Action |
|------|--------|
| `src/port/iptzcontroller.h` | New — abstract interface |
| `src/port/ptz.h` | Add IPTZController inheritance, angle_updated signal, method declarations |
| `src/port/ptz.cpp` | Implement 6 interface methods, emit angle_updated |
| `src/port/usbcan.h` | Add IPTZController inheritance, method declarations |
| `src/port/usbcan.cpp` | Implement 6 interface methods |
| `src/port/udpptz.h` | Add IPTZController inheritance, method declarations |
| `src/port/udpptz.cpp` | Implement 6 interface methods |
| `src/visual/userpanel.h` | Add active_ptz member, replace 3 slots with 1 |
| `src/visual/userpanel.cpp` | Replace 8 ptz_type switches, add angle connection, update scan loop |
| `src/port/controlport.h` | Made `get_port_status()` const |
| `src/port/controlport.cpp` | Made `get_port_status()` const |
| `CMakeLists.txt` | Add iptzcontroller.h to sources |

## Build fix: `get_port_status()` const
Status: DONE

`PTZ::ptz_is_connected()` is `const` (from IPTZController), but called non-const `get_port_status()`.
Fixed by adding `const` to `ControlPort::get_port_status()` declaration and definition — it only reads `connected_to_tcp` and `connected_to_serial`, no mutation.

Files changed: `src/port/controlport.h`, `src/port/controlport.cpp`
