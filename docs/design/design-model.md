# Design Model

## Revision History

| Version | Date | Changes |
|---|---|---|
| 1.0 | 2026-05-21 | Initial draft |
| 1.1 | 2026-05-29 | Added `CHECKING_RIGHT` to `RvcState`; removed `is_right_blocked` from `SensorData`; reworked navigate rules, sequence diagrams, and the state machine for multi-tick avoidance with right-probe (AD-11, AD-12) |

---

The Design Model defines the software structure of the RVC Control SW — classes, interfaces, responsibilities, and key collaborations. All naming follows the project code conventions.

---

## 1. Design Principles Applied

- **SRP**: Each class has one reason to change (sensor reading, navigation logic, actuator control, and orchestration are separated).
- **OCP**: New sensor types and navigation strategies are added via new classes, not by modifying existing ones.
- **LSP**: All `ISensor` implementations are interchangeable in `RvcController`.
- **ISP**: `ISensor`, `IMotorController`, and `ICleanerController` are separate interfaces; no class is forced to implement unrelated methods.
- **DIP**: `RvcController` depends on abstractions (`ISensor`, `IMotorController`, `INavigationStrategy`), not concrete implementations.

---

## 2. Enumerations

```cpp
enum class Direction  { FORWARD, BACKWARD, LEFT, RIGHT, STOP };
enum class CleanPower { OFF, ON, POWER_UP };

enum class RvcState {
    IDLE,
    CLEANING,           // normal forward navigation
    AVOIDING_OBSTACLE,  // front blocked, deciding the next step
    CHECKING_RIGHT,     // rotated right to probe the right side via front sensor
    ESCAPING,           // surrounded; backing up one cell per tick
    INTENSIFYING        // dust detected, power up active
};
```

---

## 3. Interfaces

### `ISensor`
```
ISensor
─────────────────────────────
+ detect() : bool   {pure virtual}
```
Implemented by: `FrontSensor`, `LeftSensor`, `DustSensor`. (`RightSensor` removed 2026-05-29 — see AD-11; the right side is now probed by rotating and reusing the front sensor.)

### `IMotorController`
```
IMotorController
─────────────────────────────
+ move(direction : Direction) : void   {pure virtual}
```

### `ICleanerController`
```
ICleanerController
─────────────────────────────
+ setPower(power : CleanPower) : void   {pure virtual}
```

### `INavigationStrategy`
```
INavigationStrategy
─────────────────────────────
+ navigate(data : SensorData) : Direction   {pure virtual}
```
Decouples navigation algorithm from control orchestration. Enables substitution of rule-based logic with an ML-based strategy without touching `RvcController`.

---

## 4. Value Object

### `SensorData`
Aggregates all sensor readings for a single control cycle.

```
SensorData
─────────────────────────────
+ is_front_blocked : bool
+ is_left_blocked  : bool
+ has_dust         : bool
```

---

## 5. Classes

### `FrontSensor : ISensor`
```
FrontSensor
─────────────────────────────
- _triggered : bool
─────────────────────────────
+ detect() : bool
+ onInterrupt() : void    ← called by interrupt handler
```

### `LeftSensor : ISensor`, `DustSensor : ISensor`
```
[Sensor]
─────────────────────────────
- _state : bool
─────────────────────────────
+ detect() : bool
```
> `RightSensor` still exists as a file but is no longer wired into the controller (AD-11). The right side is detected by rotating right and reading the front sensor.

### `DefaultNavigationStrategy : INavigationStrategy`
Implements the rule-based navigation logic from the requirements.

```
DefaultNavigationStrategy
─────────────────────────────
+ navigate(data : SensorData) : Direction
```

Rules encoded (no right sensor — see AD-11):
1. Front + Left blocked → BACKWARD (signals the controller to probe the right side, then escape)
2. Front blocked, left open → LEFT
3. No obstacles → FORWARD

### `RvcController`
The central orchestrator. Owns all component references and manages the RVC state machine.

```
RvcController
─────────────────────────────────────────────────────
- _front_sensor      : ISensor*   ← also reused to probe the right side
- _left_sensor       : ISensor*
- _dust_sensor       : ISensor*
- _motor             : IMotorController*
- _cleaner           : ICleanerController*
- _nav_strategy      : INavigationStrategy*
- _state             : RvcState
- _intensify_ticks   : int       ← countdown for PowerUp duration
─────────────────────────────────────────────────────
+ start() : void
+ stop()  : void
+ onTick() : void                ← called each Timer tick
+ onFrontObstacleDetected() : void   ← called from interrupt
```

---

## 6. Class Diagram (Overview)

```
          ┌─────────────────────────────────────────────────┐
          │                  RvcController                   │
          │  - _state : RvcState                            │
          └──┬────────┬────────┬────────┬────────┬─────────┘
             │        │        │        │        │
         ISensor  ISensor  ISensor  IMotorController  ICleanerController  INavigationStrategy
             │        │        │
       FrontSensor  LeftSensor  DustSensor

    INavigationStrategy
          │
    DefaultNavigationStrategy
```

---

## 7. Sequence Diagrams

### UC-02: Normal Navigation (no obstacle, no dust)

```
Timer          RvcController        ISensor(x3+dust)    IMotorController   ICleanerController
  │                  │                    │                    │                   │
  │──onTick()───────>│                    │                    │                   │
  │                  │──detect()─────────>│ (left)             │                   │
  │                  │<──false────────────│                    │                   │
  │                  │──detect()─────────>│ (right)            │                   │
  │                  │<──false────────────│                    │                   │
  │                  │──detect()─────────>│ (dust)             │                   │
  │                  │<──false────────────│                    │                   │
  │                  │                    │                    │                   │
  │                  │──navigate(data)──> [DefaultNavigationStrategy]              │
  │                  │<──FORWARD──────────│                    │                   │
  │                  │                    │                    │                   │
  │                  │────────────────────────move(FORWARD)───>│                   │
  │                  │────────────────────────────────────────────setPower(ON)────>│
```

---

### UC-03: Avoid Front Obstacle (multi-tick, left open)

```
FrontSensor    RvcController     ISensor(left)        IMotorController
  │                  │                 │                    │
  │──onFrontObstacleDetected()──────> │                    │
  │                  │─────────────────────move(STOP)──────>│   [state = AVOIDING_OBSTACLE]
Timer──onTick()────>│──detect()──────>│ (left) → false      │
  │                  │   [left open → turn left]            │
  │                  │─────────────────────move(LEFT)──────>│   [state = CLEANING]
Timer──onTick()────>│─────────────────────move(FORWARD)───>│   [resume]
```

---

### UC-04: Escape Surrounded State (multi-tick, right probed via front sensor)

```
FrontSensor    RvcController     ISensor(left/front)  IMotorController
  │                  │                 │                    │
  │──onFrontObstacleDetected()──────> │                    │
  │                  │─────────────────────move(STOP)──────>│   [state = AVOIDING_OBSTACLE]
Timer──onTick()────>│──detect()──────>│ (left) → true       │
  │                  │   [left blocked → probe right]       │
  │                  │─────────────────────move(RIGHT)─────>│   [state = CHECKING_RIGHT]
Timer──onTick()────>│──detect()──────>│ (front = old right) → true
  │                  │   [right blocked → face back]        │
  │                  │─────────────────────move(LEFT)──────>│   [state = ESCAPING]
Timer──onTick()────>│─────────────────────move(BACKWARD)──>│   [back 1 cell → AVOIDING_OBSTACLE]
  │                  │   [re-evaluate sides next tick]      │
```

---

### UC-05: Intensify Cleaning

```
Timer          RvcController        DustSensor          ICleanerController
  │                  │                    │                    │
  │──onTick()───────>│──detect()─────────>│                    │
  │                  │<──true─────────────│                    │
  │                  │   [state = INTENSIFYING]                │
  │                  │────────────────────────setPower(POWER_UP)>│
  │                  │   [_intensify_ticks = INTENSIFY_DURATION]│
  │                  │                    │                    │
  │   ... ticks pass ...                  │                    │
  │                  │   [_intensify_ticks reaches 0]          │
  │                  │────────────────────────setPower(ON)─────>│
  │                  │   [state = CLEANING]                    │
```

---

## 8. State Machine: RvcController (multi-tick)

The interrupt only issues STOP and enters AVOIDING_OBSTACLE; avoidance and
escape then advance one step per `onTick()` (AD-12). One motion command per tick.

```
         start()
  [IDLE] ───────────────────────────────────────> [CLEANING]
                                                       │  ▲
            onFrontObstacleDetected() (STOP)           │  │ onTick: forward
  [CLEANING] ────────────────────────> [AVOIDING_OBSTACLE] │
                                                       │     │
              onTick: left open  → turn left ──────────┼─────┤
              onTick: left blocked → turn right        │     │
                                                       ▼     │
                                             [CHECKING_RIGHT]│
                  onTick: right open  → resume ────────┼─────┤
                  onTick: right blocked → turn back     │     │
                                                       ▼     │
                                                  [ESCAPING] │
                  onTick: back up 1 cell → re-evaluate │     │
                                                       ▼     │
                                       (back to AVOIDING_OBSTACLE)

  [CLEANING] ──── dust detected ─────────────────> [INTENSIFYING]
  [INTENSIFYING] ─ timer expires ────────────────> [CLEANING]

  Any state ──── stop() ─────────────────────────> [IDLE]
```
