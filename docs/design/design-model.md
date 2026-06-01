# Design Model

## Design Change Trace - 2026-06-01

### [추가]
- Added `CHECKING_RIGHT` as the explicit state for FrontSensor-based right probing.
- Added multi-tick obstacle handling where the interrupt only stops the robot and later ticks perform side checks, right probe, and escape.

### [삭제]
- Removed the active `RightSensor` dependency from `RvcController`.
- Removed `SensorData::is_right_blocked`, `_escape_step`, and `continueEscaping()` from the active design.

### [변경]
- Changed Right Scan from a value stored in `SensorData` to an observable controller state flow.
- Changed ESCAPING from a fixed `_escape_step` sequence to explicit state-machine re-evaluation after each backward move.

---

## 1. Design Principles

- **SRP**: sensor reading, navigation decision, actuator command, and simulation responsibilities are separated.
- **OCP**: navigation strategy can be replaced without changing controller clients.
- **DIP**: `RvcController` depends on interfaces, not concrete HAL classes.
- **Testability**: right probing and escape are observable through motor logs and simulator position changes.

---

## 2. Enumerations

```cpp
enum class Direction  { FORWARD, BACKWARD, LEFT, RIGHT, STOP };
enum class CleanPower { OFF, ON, POWER_UP };

enum class RvcState {
    IDLE,
    CLEANING,
    AVOIDING_OBSTACLE,
    CHECKING_RIGHT,
    ESCAPING,
    INTENSIFYING
};
```

---

## 3. Interfaces

| Interface | Responsibility |
|---|---|
| `ISensor` | Returns a boolean sensor reading through `detect()`. |
| `IMotorController` | Executes movement commands through `move(Direction)`. |
| `ICleanerController` | Controls cleaner power through `setPower(CleanPower)`. |
| `INavigationStrategy` | Decides the next navigation signal from `SensorData`. |

`FrontSensor`, `LeftSensor`, and `DustSensor` are active sensor dependencies. The dedicated `RightSensor` is not active.

---

## 4. SensorData

`SensorData` contains only the sensor facts needed by `DefaultNavigationStrategy`.

```cpp
struct SensorData {
    bool is_front_blocked = false;
    bool is_left_blocked  = false;
    bool has_dust         = false;
};
```

Right-side information is intentionally not stored in `SensorData`; it is discovered by entering `CHECKING_RIGHT` after the robot has rotated right.

---

## 5. RvcController Responsibilities

`RvcController` orchestrates:
- start/stop lifecycle
- timer tick handling
- front obstacle interrupt handling
- `AVOIDING_OBSTACLE -> CHECKING_RIGHT -> ESCAPING` state progression
- cleaner power-up duration

Active dependencies:
- `_front_sensor`
- `_left_sensor`
- `_dust_sensor`
- `_motor`
- `_cleaner`
- `_nav_strategy`

---

## 6. Right Scan Flow

Right Scan is now explicit in the state machine.

```text
front obstacle interrupt
  -> STOP
  -> state = AVOIDING_OBSTACLE

next tick
  -> read LeftSensor
  -> if left is open: LEFT, then CLEANING
  -> if left is blocked: RIGHT, then CHECKING_RIGHT

next tick in CHECKING_RIGHT
  -> FrontSensor now faces the old right side
  -> if open: CLEANING, then next cleaning tick moves FORWARD
  -> if blocked: LEFT to restore original heading, then ESCAPING
```

---

## 7. ESCAPING Flow

`ESCAPING` emits one backward command per tick, then returns to `AVOIDING_OBSTACLE` so the controller can re-evaluate the environment.

```text
ESCAPING tick
  -> BACKWARD
  -> state = AVOIDING_OBSTACLE
```

This allows dead-end cases to back up over multiple ticks without moving more than one cell per tick.
