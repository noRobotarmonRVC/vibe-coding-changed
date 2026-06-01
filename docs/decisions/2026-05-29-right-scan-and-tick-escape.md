# Architecture Decisions - 2026-05-29 (Right Scan and Tick Escape)

## AD-11: Replace RightSensor with FrontSensor Right Scan

### [삭제]
- Removed `RightSensor` from the active dependency graph of `RvcController`.
- Removed `hal/RightSensor.cpp` from the active CMake build target.
- Removed `Simulator::injectRight()` from the simulator control API.

### [추가]
- Added the **Right Scan** behavior: stop, turn right, read `FrontSensor::detect()`, then turn left to restore the original heading.
- Added documentation terminology for `Right Scan` as the source of `SensorData::is_right_blocked`.

### [변경]
- `RvcController` now receives `front_sensor`, `left_sensor`, and `dust_sensor` only.
- `Simulator` now injects right-side grid blockage into the front sensor before front-obstacle handling.
- Requirements and design documents now describe right-side detection as front-sensor scanning instead of a dedicated periodic right sensor.

## AD-12: Escape Movement Advances One Command per Tick

### [삭제]
- Removed the atomic `STOP -> BACKWARD -> LEFT -> FORWARD` escape sequence from a single front-obstacle event.

### [추가]
- Added `_escape_step` in `RvcController` to track escape progress across ticks.
- Added simulator escape tick tracking so integration tests advance escape movement one tick at a time.

### [변경]
- `ESCAPING` remains active after surrounded detection.
- Each following tick emits exactly one movement command in order: `BACKWARD`, `LEFT`, `FORWARD`.
- Tests now verify that backward movement changes position by one cell per tick, matching forward movement semantics.
