# SW Architecture Document

## Design Change Trace - 2026-05-29

### [추가]
- Added AD-11 for FrontSensor-based Right Scan.
- Added AD-12 for tick-by-tick ESCAPING.

### [삭제]
- Removed dedicated RightSensor from active controller and build architecture.

### [변경]
- Changed front-obstacle handling and simulator integration to use Right Scan.

## 1. Overview

The RVC Control SW follows a **layered architecture** with a strict dependency rule: upper layers depend on lower layers; lower layers never depend on upper layers. All cross-layer communication uses interfaces, keeping the system testable and extensible.

---

## 2. Architectural Layers

```
┌──────────────────────────────────────────────────┐
│              Application Layer                    │
│   RvcController  (orchestration, state machine)   │
├──────────────────────────────────────────────────┤
│               Domain Layer                        │
│   DefaultNavigationStrategy  SensorData           │
│   RvcState (enum)  Direction (enum)               │
├──────────────────────────────────────────────────┤
│             Interface Layer                       │
│   ISensor  IMotorController  ICleanerController   │
│   INavigationStrategy                             │
├──────────────────────────────────────────────────┤
│        Hardware Abstraction Layer (HAL)           │
│   FrontSensor  LeftSensor                        │
│   DustSensor  (Motor/Cleaner adapters)            │
└──────────────────────────────────────────────────┘
```

| Layer | Responsibility | Changes when... |
|---|---|---|
| Application | Orchestrates state transitions, start/stop lifecycle | Use-case logic changes |
| Domain | Encapsulates navigation rules and data structures | Navigation algorithm or sensor semantics change |
| Interface | Defines contracts between layers | I/O contracts change |
| HAL | Wraps real hardware behind abstractions | Hardware changes (sensor type, actuator protocol) |

---

## 3. Component Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                       RVC Control SW                         │
│                                                             │
│  ┌─────────────────┐        ┌──────────────────────────┐   │
│  │  RvcController  │◄──────►│  DefaultNavigation        │   │
│  │  (orchestrator) │        │  Strategy                 │   │
│  └────────┬────────┘        └──────────────────────────┘   │
│           │                                                  │
│    ┌──────┴──────────────────────────────────┐              │
│    │           Interface Layer                │              │
│    │  ISensor  IMotorController  ICleaner...  │              │
│    └──────┬──────────────────────────────────┘              │
│           │                                                  │
│    ┌──────┴───────────────────────────────────┐             │
│    │               HAL                         │             │
│    │  FrontSensor LeftSensor                  │             │
│    │  DustSensor  MotorAdapter CleanerAdapter  │             │
│    └──────────────────────────────────────────┘             │
└─────────────────────────────────────────────────────────────┘
         ▲                               ▼
   [Sensor HW]                  [Motor / Cleaner HW]
```

---

## 4. Key Architectural Decisions

### AD-01: Strategy Pattern for Navigation

**Decision:** Navigation logic is extracted into `INavigationStrategy` / `DefaultNavigationStrategy`, injected into `RvcController`.

**Rationale:** The supplementary specification (SUPP-02) requires navigation to be decoupled from control so it can be replaced by an ML-based algorithm in a future iteration. The Strategy pattern enables this without modifying `RvcController`.

**Consequence:** Adding a new navigation algorithm requires only a new class implementing `INavigationStrategy`; no existing classes change.

---

### AD-02: Interrupt vs. Polling for Sensors

**Decision:** `FrontSensor` is interrupt-driven (`onInterrupt()` sets a flag, `onFrontObstacleDetected()` is called by the controller); Left and Dust sensors are polled each Tick. Right-side obstacle checks are performed by rotating right, sampling the Front Sensor, and rotating left to restore heading.

**Rationale:** FUNC-01 requires immediate front-obstacle response. FUNC-02 defines the other sensors as periodic. Mixing both models requires the controller to handle both signal delivery paths.

**Consequence:** `FrontSensor` has an `onInterrupt()` entry point separate from `detect()`; the system ISR must call it. `RvcController` no longer depends on a dedicated `RightSensor`.

---

### AD-03: State Machine in RvcController

**Decision:** `RvcController` maintains an explicit `RvcState` enum and transitions are centralized there.

**Rationale:** Behavior varies significantly per state (e.g., Dust handling differs from Escaping). An explicit state machine prevents scattered conditional logic and makes transitions auditable.

**Consequence:** Every method in `RvcController` begins by checking or updating `_state`. New behavior is added by introducing a new state, not by adding conditionals to existing code.

---

### AD-04: Dependency Injection for All Hardware

**Decision:** `RvcController` receives all dependencies (`ISensor*`, `IMotorController*`, `ICleanerController*`, `INavigationStrategy*`) via constructor injection.

**Rationale:** Enables full unit-test isolation — Google Test substitutes mock implementations for every hardware dependency. Satisfies SUPP-01 (new sensor types pluggable without modifying `RvcController`).

**Consequence:** `RvcController` never constructs its dependencies; a composition root (main or test fixture) wires them.

---

### AD-07: onTick() Must Re-Issue FORWARD Every Cycle

**Decision:** `onTick()` appends `_motor->move(Direction::FORWARD)` at the end of every CLEANING or INTENSIFYING tick, rather than relying on the `FORWARD` issued in `start()`.

**Rationale:** Hardware H-bridge drivers latch a direction until commanded otherwise, so a single `FORWARD` at startup is sufficient for real hardware. However, a motor command issued in one tick must be re-issued in the next tick to have effect — continuous forward motion requires a new FORWARD command each cycle.

**Consequence:** Motor command log grows by one entry per tick during straight-line travel. This is acceptable as the log is only used for integration test verification and is never persisted.

---

### AD-11: Right Scan Replaces RightSensor

**Decision:** Dedicated `RightSensor` is removed from active controller dependencies. Right-side detection is performed by a right turn, `FrontSensor::detect()`, and left turn restore.

**Trace:** See `docs/decisions/2026-05-29-right-scan-and-tick-escape.md` for [추가], [삭제], and [변경] details.

---

### AD-12: ESCAPING Advances One Command per Tick

**Decision:** Surrounded escape is no longer completed inside one front-obstacle event. The controller stays in `ESCAPING` and emits `BACKWARD`, `LEFT`, and `FORWARD` across separate ticks.

**Trace:** See `docs/decisions/2026-05-29-right-scan-and-tick-escape.md` for [추가], [삭제], and [변경] details.

---

## 5. Source Directory Layout

```
src/
├── CMakeLists.txt
├── interfaces/
│   ├── ISensor.hpp
│   ├── IMotorController.hpp
│   ├── ICleanerController.hpp
│   └── INavigationStrategy.hpp
├── domain/
│   ├── SensorData.hpp
│   ├── Direction.hpp
│   ├── CleanPower.hpp
│   ├── RvcState.hpp
│   └── DefaultNavigationStrategy.hpp / .cpp
├── hal/
│   ├── FrontSensor.hpp / .cpp
│   ├── LeftSensor.hpp / .cpp
│   └── DustSensor.hpp / .cpp
└── app/
    ├── RvcController.hpp / .cpp
    └── main.cpp
```

```
test/
├── domain/
│   └── DefaultNavigationStrategyTest.cpp
└── app/
    └── RvcControllerTest.cpp
```

---

## 6. Integration and Simulation

The **Simulator** component (separate from Control SW) emulates the hardware environment for integration testing:

- Implements `IMotorController` and `ICleanerController` to record commands.
- Drives `ISensor` implementations with scripted scenarios.
- Verifies that the full control loop produces the correct sequence of Direction and CleanPower commands for each scenario.

The Simulator does not test individual classes — it tests the assembled system end-to-end, replacing hardware with software stubs.

---

## 7. Non-Functional Requirement Traceability

| Req ID | Requirement | Architectural Solution |
|---|---|---|
| FUNC-01 | Front Sensor interrupt response | AD-02: interrupt path in FrontSensor |
| FUNC-02 | Periodic sensor polling | AD-02: `detect()` called in `onTick()` |
| REL-02 | No conflicting motor commands | AD-03: state machine prevents issuing two directions |
| REL-03 | Safe state on indeterminate input | `RvcController::stop()` reachable from any state |
| PERF-01 | Tick processing completes within one interval | No blocking calls in `onTick()`; all sensor reads are synchronous returns |
| SUPP-01 | New sensor types without modifying existing code | AD-04: inject new `ISensor` implementation |
| SUPP-02 | Replaceable navigation algorithm | AD-01: `INavigationStrategy` injection |
| DC-03 | Google Test unit tests | AD-04: DI enables full mock-based isolation |
