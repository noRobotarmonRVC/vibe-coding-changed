# SW Architecture Document

## Design Change Trace - 2026-06-04

### [추가]
- Added `RvcController::state() const` so the simulator can read the controller state when deciding whether a front rising edge is a real interrupt. (F-10 참조)

### [변경]
- Edge-triggered front interrupt is now also state-gated: the simulator calls `onFrontObstacleDetected()` only when front changes from clear to blocked **and** the controller is in `CLEANING` / `INTENSIFYING`. During `AVOIDING_OBSTACLE` / `CHECKING_RIGHT` / `ESCAPING` the rising edge is routed to `onTick()` instead, so the Right Scan right-turn cannot raise a false interrupt. (F-10 참조)

---

## Design Change Trace - 2026-06-01

### [추가]
- Added `CHECKING_RIGHT` to the application state machine.
- Added edge-triggered simulator front interrupt handling.

### [삭제]
- Removed active `RightSensor` build and controller dependency.
- Removed `_escape_step` based escape orchestration from the active architecture.

### [변경]
- Changed right-side detection to a multi-tick controller flow using `FrontSensor` after a right turn.
- Changed surrounded escape to back up one cell, then re-enter side evaluation.

---

## 1. Overview

RVC Control SW follows a layered architecture. Application logic depends on interfaces and domain types, not concrete hardware classes.

---

## 2. Layers

```text
Application Layer
  - RvcController
  - main

Domain Layer
  - DefaultNavigationStrategy
  - SensorData
  - Direction / CleanPower / RvcState

Interface Layer
  - ISensor
  - IMotorController
  - ICleanerController
  - INavigationStrategy

HAL / Simulator / UI Layer
  - FrontSensor, LeftSensor, DustSensor
  - Simulator, SimulatedSensor, SimulatedMotor, SimulatedCleaner
  - ConsoleDisplay, GridDisplay
```

---

## 3. Key Dependencies

- `RvcController` receives all dependencies through constructor injection.
- `RightSensor.cpp` is excluded from the active CMake source list.
- The right side is checked by rotating right and reading `FrontSensor` in `CHECKING_RIGHT`.
- The simulator feeds front readings according to the robot's current heading, so after a right turn the front sensor represents the old right side.

---

## 4. Obstacle Flow

```text
Front obstacle rising edge
  -> RvcController::onFrontObstacleDetected()
  -> STOP
  -> state = AVOIDING_OBSTACLE

Timer tick in AVOIDING_OBSTACLE
  -> read LeftSensor
  -> LEFT if left is open
  -> RIGHT and state = CHECKING_RIGHT if left is blocked

Timer tick in CHECKING_RIGHT
  -> read FrontSensor as right-side probe
  -> CLEANING if open
  -> LEFT and state = ESCAPING if blocked

Timer tick in ESCAPING
  -> BACKWARD
  -> state = AVOIDING_OBSTACLE
```

---

## 5. Simulator Integration

- Front obstacle interrupt is edge-triggered **and** state-gated: the simulator calls `onFrontObstacleDetected()` only when front changes from clear to blocked **and** the controller is in `CLEANING` / `INTENSIFYING` (read via `RvcController::state()`). Otherwise the rising edge is routed to `onTick()`. This prevents the right-turn performed for Right Scan during `AVOIDING_OBSTACLE` / `CHECKING_RIGHT` / `ESCAPING` from producing a false interrupt that hijacks the right-side evaluation. (F-10 참조)
- While front remains blocked, later behavior progresses through `onTick()`.
- `applyPendingMotorCommands()` applies each newly emitted command in order, and tests verify no tick moves more than one cell.

---

## 6. Build Structure

- `rvc_core` contains all non-main production code.
- `hal/RightSensor.cpp` remains in the repository as inactive legacy code but is not compiled.
- MSVC uses `/utf-8` so Korean trace comments compile cleanly.
- `rvc_tests` covers domain, controller, and simulator behavior.
