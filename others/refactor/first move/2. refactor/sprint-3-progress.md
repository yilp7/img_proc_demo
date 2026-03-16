# Sprint 3 Progress: Magic Numbers & Constants Extraction

## Task 3.1: Create `util/constants.h` for pipeline constants
Status: DONE

Created `src/util/constants.h` with the following constants extracted from userpanel.cpp and userpanel.h:
- `MAX_QUEUE_SIZE = 5` — image queue cap
- `AUTO_MCP_DIVISOR = 200` — histogram threshold for auto-MCP
- `THROTTLE_MS = 40` — parameter update throttle interval
- `FONT_SCALE_DIVISOR = 1024.0` — font size scaling factor
- `THREAD_POOL_SIZE = 40` — worker thread count
- `MAX_MCP_8BIT = 255` — max MCP for 8-bit pixel depth
- `MAX_MCP_12BIT = 4095` — max MCP for 12-bit pixel depth
- `QUEUE_EMPTY_SLEEP_MS = 10` — sleep when queue empty
- `LIGHT_SPEED_M_NS = 3e8 * 1e-9 / 2` — one-way light speed in m/ns (moved from tcu_protocol.h)

## Task 3.2: Create `port/pelco_protocol.h` for PTZ/lens protocol constants
Status: DONE

Created `src/port/pelco_protocol.h` as a merged protocol header for both PTZ and lens.

Shared constants (used by both PTZ and lens):
- `SYNC_BYTE = 0xFF` — protocol sync byte
- `MIN_SPEED = 0x01` / `MAX_SPEED = 0x3F` — speed range
- `READ_TIMEOUT_MS = 40` — read timeout
- `ANGLE_SCALE = 100` — angle value multiplier (degrees x 100)
- `FULL_ROTATION = 36000` — 360 degrees x 100
- `VERTICAL_LIMIT = 4000` — +/-40 degrees x 100 vertical limit
- `VERTICAL_DEG = 40.0` — vertical limit in degrees

PTZ-specific constants (PTZ_ prefix):
- `PTZ_QUERY_H = 0x51` / `PTZ_REPLY_H = 0x59` — horizontal angle query/reply
- `PTZ_QUERY_V = 0x53` / `PTZ_REPLY_V = 0x5B` — vertical angle query/reply
- `PTZ_SET_H = 0x4B` / `PTZ_SET_V = 0x4D` — set angle commands
- `PTZ_SELF_CHECK_DATA = 0x77` — self-check data byte
- Direction command bytes: `PTZ_DIR_STOP`, `PTZ_DIR_RIGHT`, `PTZ_DIR_LEFT`, `PTZ_DIR_UP`, `PTZ_DIR_DOWN`, `PTZ_DIR_UP_LEFT`, `PTZ_DIR_UP_RIGHT`, `PTZ_DIR_DOWN_LEFT`, `PTZ_DIR_DOWN_RIGHT`, `PTZ_DIR_SELF_CHECK`

Lens-specific constants (LENS_ prefix):
- `LENS_QUERY_ZOOM/FOCUS/RADIUS` — query command bytes
- `LENS_REPLY_ZOOM/FOCUS/RADIUS` — reply identifiers
- `LENS_CMD_STOP/ZOOM_IN/ZOOM_OUT/FOCUS_FAR` — direction commands (byte 3)
- `LENS_CMD_FOCUS_NEAR/RADIUS_UP/RADIUS_DOWN` — direction commands (byte 2)
- `LENS_SET_ZOOM/FOCUS/RADIUS` — set position commands
- `LENS_RELAY_ON/OFF` — relay commands

Note: Originally created as separate `pelco_d.h` and `lens_protocol.h`, then merged into single `pelco_protocol.h`.
Note: Named `pelco_protocol.h` because the protocol is a mix of Pelco-D and Pelco-P.
Note: Angle constants kept without prefix — they are device-related, not protocol-specific.
Note: Command length constants removed — the raw 7 is self-evident from context.

## Task 3.3: Create `port/tcu_protocol.h` for TCU protocol constants
Status: DONE

Created `src/port/tcu_protocol.h` with the following constants:
- `TCU_HEAD = 0x88` — frame header byte
- `TCU_TAIL = 0x99` — frame tail byte
- `TCU_QUERY_CMD = 0x15` / `TCU_QUERY_REPLY = 0x15` — query command/reply
- `TCU_CLOCK_8 = 8` — type 0 clock cycle
- `TCU_CLOCK_4 = 4` — type 1/2 clock cycle
- `TCU_MAX_PRF_KHZ = 30.0` — max pulse repetition frequency (kHz)

Note: `TCU_CMD_LEN` removed — command length 7 is self-evident from context.
Note: `LASER_OFF`/`LASER_ON_CMD`/`LASER_ALL_OFF` removed — raw 0x04/0x08/0x00 are clearer.
Note: Clock constants named by value (`TCU_CLOCK_8`/`TCU_CLOCK_4`) instead of type number.

## Task 3.4: Replace magic numbers in `grab_thread_process()`
Status: DONE

Replaced in userpanel.cpp:
- Added `#include "util/constants.h"`
- `_h / 1024.0` → `_h / FONT_SCALE_DIVISOR`
- `q_img[thread_idx].size() > 5` → `> MAX_QUEUE_SIZE`
- `QThread::msleep(10)` → `msleep(QUEUE_EMPTY_SLEEP_MS)`
- `img_mem[thread_idx].total() / 200` → `/ AUTO_MCP_DIVISOR`
- `mcp_max(255)` → `mcp_max(MAX_MCP_8BIT)`
- `tp(40)` → `tp(THREAD_POOL_SIZE)`
- `mcp_max = 255` → `MAX_MCP_8BIT`, `mcp_max = 4095` → `MAX_MCP_12BIT`
- `t.elapsed() < 40` → `< THROTTLE_MS` (3 instances)

## Task 3.5: Replace magic numbers in PTZ, lens, and TCU
Status: DONE

Replaced in ptz.cpp:
- `#include "pelco_protocol.h"`
- `0xFF` → `SYNC_BYTE`, direction bytes → `PTZ_DIR_*`
- Query/reply/set bytes → `PTZ_QUERY_H/V`, `PTZ_REPLY_H/V`, `PTZ_SET_H/V`
- `0x77` → `PTZ_SELF_CHECK_DATA`
- Angle values → `ANGLE_SCALE`, `FULL_ROTATION`, `VERTICAL_LIMIT`, `VERTICAL_DEG`
- Speed limits → `MIN_SPEED`, `MAX_SPEED`
- Timeout → `READ_TIMEOUT_MS`

Replaced in lens.cpp:
- `#include "pelco_protocol.h"`
- `0xFF` → `SYNC_BYTE` (sync bytes only)
- Query/reply bytes → `LENS_QUERY_*`, `LENS_REPLY_*`
- Direction command bytes → `LENS_CMD_*`
- Set command bytes → `LENS_SET_*`
- Relay bytes → `LENS_RELAY_ON/OFF`
- Speed limits → `MIN_SPEED`, `MAX_SPEED`
- Timeout → `READ_TIMEOUT_MS`

Replaced in tcu.cpp:
- `#include "tcu_protocol.h"` and `#include "util/constants.h"`
- `3e8 * 1e-9 / 2` → `LIGHT_SPEED_M_NS`
- `0x88` → `TCU_HEAD`, `0x99` → `TCU_TAIL`, `0x15` → `TCU_QUERY_CMD`/`TCU_QUERY_REPLY`
- Clock cycle `8` → `TCU_CLOCK_8`, `4` → `TCU_CLOCK_4`
- `30` (PRF limit) → `TCU_MAX_PRF_KHZ`

## Task 3.6: Eliminate `generate_ba` heap allocation in lens.cpp
Status: DONE

Replaced all `generate_ba(new uchar[7]{...}, 7)` calls with stack-allocated arrays + direct `QByteArray` construction, matching the pattern already applied in ptz.cpp (Sprint 0, task 0.2).
- 3 instances in constructor
- 12 instances in `lens_control`
- 7 instances in `lens_control_addr`

Note: Sprint 0 task 0.2 fixed ptz.cpp and task 0.3 fixed tcu.cpp, but all other files were missed.

## Task 3.7: Eliminate remaining `generate_ba` heap allocation project-wide
Status: DONE

Replaced all remaining `generate_ba(new uchar[N]{...}, N)` calls across the project for consistency with the stack-array pattern.

controlportthread.cpp:
- 4 instances in UDP PTZ serial init (5-byte arrays)
- 1 instance in TCUThread::try_communicate (static QByteArray)
- 10 instances in LensThread::lens_control (static QByteArray array)
- 3 instances in LensThread::try_communicate (static QByteArray array)
- 1 instance in LaserThread::try_communicate (static QByteArray)
- 1 instance in PTZThread::ptz_control STOP case
- 2 instances in PTZThread::try_communicate (static QByteArray array)

huanyu.h:
- 14 instances in commands map initializer — replaced with `QByteArray("\x..", N)` string literal form (member initializer context, no stack variables possible)

rangefinder.cpp:
- 4 instances in rf_control (SERIAL, FREQ_SET, FREQ_GET, BAUDRATE)

userpanel.cpp:
- 1 instance in on_DIST_BTN_clicked (6-byte display command)
- 2 instances in start_laser / on_LASER_BTN_clicked (laser on)
- 1 instance in on_LASER_BTN_clicked (laser off)

Note: Commented-out `generate_ba(new ...)` calls left unchanged (dead code).

## Removed files
- `src/port/pelco_d.h` — superseded by `pelco_protocol.h`
- `src/port/lens_protocol.h` — merged into `pelco_protocol.h`
- `src/port/ptz_protocol.h` — renamed to `pelco_protocol.h`

## Task 3.8: Remove dead `#include "thread/controlportthread.h"` from userpanel.h
Status: DONE

Removed `#include "thread/controlportthread.h"` from `src/visual/userpanel.h` — controlportthread.h/cpp is dead code (commented out in CMakeLists.txt, all instantiations commented out in userpanel.cpp, fully replaced by ControlPort-based classes under src/port/).

## Build Verification
Status: PASS
