# Architecture Decisions - 2026-05-29 / Updated 2026-06-01

## AD-11: Replace RightSensor with FrontSensor Right Probe

### [삭제]
- Removed `RightSensor` from the active dependency graph of `RvcController`.
- Removed `hal/RightSensor.cpp` from the active CMake build target.
- Removed `Simulator::injectRight()` from the simulator control API.
- Removed `SensorData::is_right_blocked` from the active domain data structure.

### [추가]
- Added `CHECKING_RIGHT` as the state where the robot has rotated right and `FrontSensor::detect()` represents the old right side.
- Added edge-triggered simulator interrupt handling so front obstacle detection starts avoidance once, then later ticks progress the state machine.

### [변경]
- Right-side detection is now modeled as `AVOIDING_OBSTACLE -> CHECKING_RIGHT`, not as a field copied into `SensorData`.
- `DefaultNavigationStrategy` returns `BACKWARD` for front+left blocked as a signal to probe right before deciding escape.
- Requirements and design documents describe right-side detection as FrontSensor probing after a right turn.

## AD-12: Escape Movement Advances One Command per Tick

### [삭제]
- Removed the atomic `STOP -> BACKWARD -> LEFT -> FORWARD` escape sequence from a single front-obstacle event.
- Removed `_escape_step` and `continueEscaping()` from the active controller design.

### [추가]
- Added explicit `ESCAPING` behavior that emits one `BACKWARD` command per tick.
- Added simulator tests for dead-end backing, interrupt edge behavior, original-heading backup, and one-cell-per-tick movement.

### [변경]
- `onFrontObstacleDetected()` issues `STOP` only and sets `AVOIDING_OBSTACLE`.
- Later ticks perform side evaluation, right probe, heading restore, and backward escape.
- After `BACKWARD`, the controller returns to `AVOIDING_OBSTACLE` to re-evaluate the environment.
