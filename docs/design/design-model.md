# Design Model

## Design Change Trace - 2026-05-29

### [추가]
- Added Right Scan as the source of right-side obstacle information.
- Added `_escape_step` to represent tick-by-tick ESCAPING progress.

### [삭제]
- Removed dedicated RightSensor from the active `RvcController` dependency model.

### [변경]
- Changed `SensorData::is_right_blocked` to mean Right Scan blocked.
- Changed surrounded escape sequencing from one event to multiple ticks.

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
    AVOIDING_OBSTACLE,  // front blocked, at least one side open
    ESCAPING,           // surrounded (front + left + right scan blocked)
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
Implemented by: `FrontSensor`, `LeftSensor`, and `DustSensor`. Right-side obstacle checks reuse `FrontSensor` through a right-scan maneuver.

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
+ is_right_blocked : bool  // result of right-side front-sensor scan
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

### `DefaultNavigationStrategy : INavigationStrategy`
Implements the rule-based navigation logic from the requirements.

```
DefaultNavigationStrategy
─────────────────────────────
+ navigate(data : SensorData) : Direction
```

Rules encoded (priority order):
1. Front + Left + Right Scan blocked → BACKWARD (caller transitions to ESCAPING, then advances one command per tick)
2. Front blocked, side open → LEFT or RIGHT (turn to open side)
3. No obstacles → FORWARD

### `RvcController`
The central orchestrator. Owns all component references and manages the RVC state machine.

```
RvcController
─────────────────────────────────────────────────────
- _front_sensor      : ISensor*
- _left_sensor       : ISensor*
- _dust_sensor       : ISensor*
- _motor             : IMotorController*
- _cleaner           : ICleanerController*
- _nav_strategy      : INavigationStrategy*
- _state             : RvcState
- _intensify_ticks   : int       ← countdown for PowerUp duration
- _escape_step       : int       ← tick-by-tick escape progress
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
         ISensor  ISensor  ISensor  ISensor  IMotorController  ICleanerController  INavigationStrategy
             │        │        │        │
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

### UC-03: Avoid Front Obstacle

```
FrontSensor    RvcController        ISensor(L/R)        IMotorController   ICleanerController
  │                  │                    │                    │                   │
  │──onFrontObstacleDetected()──────────>│                    │                   │
  │                  │────────────────────────move(STOP)──────>│                   │
  │                  │──detect()─────────>│ (left)             │                   │
  │                  │<──false────────────│                    │                   │
  │                  │   [left is open → turn LEFT]            │                   │
  │                  │────────────────────────move(LEFT)──────>│                   │
  │                  │────────────────────────move(FORWARD)───>│                   │
  │                  │   [state = CLEANING]                    │                   │
```

---

### UC-04: Escape Surrounded State

```
FrontSensor    RvcController        ISensor(L/R)        IMotorController
  │                  │                    │                    │
  │──onFrontObstacleDetected()──────────>│                    │
  │                  │──detect()─────────>│ (left) → true      │
  │                  │──detect()─────────>│ (right) → true     │
  │                  │   [state = ESCAPING]                    │
  │                  │────────────────────────move(BACKWARD)──>│
  │                  │   [select turn direction]               │
  │                  │────────────────────────move(LEFT/RIGHT)>│
  │                  │────────────────────────move(FORWARD)───>│
  │                  │   [state = CLEANING]                    │
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

## 8. State Machine: RvcController

```
         start()
  [IDLE] ──────────────────────────────────────────> [CLEANING]
                                                         │   ▲
                      onFrontObstacleDetected()           │   │ turn + forward
            ┌─────── (side open) ──────────────────> [AVOIDING_OBSTACLE]
            │                                             │
            │        onFrontObstacleDetected()            │
            └─────── (all blocked) ───────────────> [ESCAPING]
                                                         │   ▲
                                                         └───┘ backward + turn + forward
            
  [CLEANING] ──── dust detected ──────────────────> [INTENSIFYING]
  [INTENSIFYING] ─ timer expires ─────────────────> [CLEANING]

  Any state ──── stop() ──────────────────────────> [IDLE]
```
