# Architecture Decisions — 2026-05-17

Decisions made during the Inception, Elaboration, and Construction phases of iteration 1.

---

## Supersession Note - 2026-05-29

### [추가]
- Added this note to connect the original sensor architecture decisions to the current Right Scan architecture.

### [삭제]
- Removed active reliance on a dedicated RightSensor from AD-02 and AD-04.

### [변경]
- AD-02 and AD-04 are superseded for right-side detection by FrontSensor Right Scan and tick-based ESCAPING. See `docs/decisions/2026-05-29-right-scan-and-tick-escape.md`.

---

## AD-01: Strategy Pattern for Navigation Logic

**Context**  
The `RvcController` needs to decide which direction to move given current sensor data. The supplementary specification (SUPP-02) requires that navigation be replaceable without modifying the controller (future ML-based navigation).

**Decision Factor**  
If navigation logic lives inside `RvcController`, adding a new algorithm requires modifying a class that already has other responsibilities (state machine, lifecycle, sensor orchestration). That violates OCP and SRP simultaneously.

**Decision**  
Extract navigation into `INavigationStrategy` with `DefaultNavigationStrategy` as the first implementation. Inject the strategy into `RvcController` via constructor.

**Output**  
- `src/interfaces/INavigationStrategy.hpp` — pure interface
- `src/domain/DefaultNavigationStrategy.hpp/cpp` — rule-based first implementation
- `RvcController` holds `INavigationStrategy*` and calls `navigate(SensorData)` without knowing the concrete type

**Trade-off**  
One extra indirection and one extra class per navigation variant. Accepted because the requirement to swap navigation is explicit.

---

## AD-02: Interrupt-Driven Front Sensor vs. Polling for Others

**Context**  
Four sensors exist. The preliminary requirements state that front obstacle detection triggers an immediate stop — implying it cannot wait for the next periodic tick.

**Decision Factor**  
Polling the front sensor on each tick introduces latency proportional to the tick interval. An interrupt fires as soon as the obstacle is present. The other sensors (left, right, dust) are explicitly described as periodic in the requirements table.

**Decision**  
`FrontSensor` provides `onInterrupt()` (called by the ISR) which triggers `RvcController::onFrontObstacleDetected()`. Left, Right, and Dust sensors implement only `detect()` and are polled inside `onTick()`.

**Output**  
- `FrontSensor` has two entry points: `detect()` (for testing) and `onInterrupt()` (for production ISR)
- `RvcController` has two public event handlers: `onTick()` and `onFrontObstacleDetected()`
- The design separates async (interrupt) from sync (tick) control paths

**Trade-off**  
Two code paths to maintain. Acceptable because the difference is fundamental to the HW contract.

---

## AD-03: Explicit State Machine in RvcController

**Context**  
`RvcController` behavior changes significantly depending on operating mode: idle, cleaning, intensifying cleaning, avoiding an obstacle, or escaping a surrounded state.

**Decision Factor**  
Without an explicit state machine, each method (`onTick`, `onFrontObstacleDetected`) would contain nested conditionals checking multiple flags. That makes transitions invisible and new states risky to add.

**Decision**  
Introduce `RvcState` enum (IDLE, CLEANING, AVOIDING_OBSTACLE, ESCAPING, INTENSIFYING). All transitions are made explicit in `RvcController`. AVOIDING_OBSTACLE and ESCAPING are transient — they resolve atomically within `onFrontObstacleDetected()` and return to CLEANING before the method exits.

**Output**  
- `src/domain/RvcState.hpp`
- All transitions visible in `RvcController.cpp` as direct `_state =` assignments

**Trade-off**  
The state enum is an internal implementation detail. Exposing it externally (e.g., as a getter) would couple callers to it — see AD-05 for how that was handled.

---

## AD-04: Constructor Injection for All Hardware Dependencies

**Context**  
`RvcController` needs four sensors, a motor, a cleaner, and a navigation strategy. These can be real HAL objects in production or simulated objects in tests.

**Decision Factor**  
If `RvcController` creates its own dependencies (e.g., `new FrontSensor()`), tests cannot replace them with mocks. The class becomes untestable in isolation and violates DIP.

**Decision**  
All seven dependencies are injected as raw non-owning pointers via constructor. The composition root (`main.cpp` or the `Simulator`) owns the objects and manages their lifetime.

**Output**  
- `RvcController` constructor takes 7 interface pointers
- `Simulator` owns all concrete objects and passes their addresses
- `RvcControllerTest` creates `MockSensor`, `MockMotor`, `MockCleaner` on the stack and passes addresses

**Trade-off**  
Raw pointers require the caller to guarantee lifetime. Accepted because this is embedded-style C++ where ownership is explicit and `unique_ptr` in the HAL layer would add overhead without benefit.

---

## AD-05: No state() Getter on Domain Classes — Behavior-Based Testing

**Context**  
An initial implementation added `RvcController::state()` to allow tests and the UI to read `_state`. A code review flagged this as a maintenance risk.

**Decision Factor**  
Exposing internal state via a getter couples callers to the internal enum. If the state machine is refactored (e.g., states renamed, merged, or split), every caller breaks. Tests that check `state() == CLEANING` are testing implementation, not behavior. TDD requires tests to verify *what the system does*, not *what it calls its internal mode*.

**Decision**  
Remove `state()` from `RvcController`. Tests verify behavior exclusively through motor and cleaner command logs. The UI (`ConsoleDisplay`) shows observable outputs (direction and power), not internal state labels.

**Output**  
- `state()` removed from `RvcController.hpp`
- `ConsoleDisplay::render(Direction, CleanPower)` — no `RvcState` parameter
- `RvcControllerTest` and `SimulatorTest` assert on `motor.log`, `cleaner.log`, `lastDirection()`, `lastPower()`

**Trade-off**  
Debugging is slightly harder without a visible state label at runtime. Accepted because the behavioral outputs (direction + power) are sufficient to infer state, and the tests are more robust.

---

## AD-06: Simulation-Only Naming Convention — inject*() vs set*()

**Context**  
`SimulatedSensor` had `setState(bool)` and `Simulator` had `setFront/setLeft/setRight/setDust()`. These look identical to domain setter anti-patterns.

**Decision Factor**  
A reader cannot distinguish `sim.setLeft(true)` (simulation control) from a hypothetical `rvc.setState(RvcState::IDLE)` (domain mutation) by name alone. This erodes the rule "no get/set on domain objects" because the naming convention stops signaling intent.

**Decision**  
Rename all simulation control methods to `inject*(reading)` to make the simulation-only purpose explicit in the name. `inject` means "I am feeding an artificial reading into a test double."

**Output**  
- `SimulatedSensor::inject(bool reading)`
- `Simulator::injectFront/injectLeft/injectRight/injectDust(bool reading)`
- All call sites in tests and `main.cpp` updated

**Trade-off**  
Slightly more verbose names. Accepted because intent clarity matters more than brevity in test infrastructure.
