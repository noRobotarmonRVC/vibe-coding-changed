# Interrupt Guard — 2026-06-04

Decision record for who owns the front-obstacle interrupt acceptance policy,
the fix for failure **F-10** (dead-end escape never fires; the RVC oscillates
between two cells). See `docs/failures/2026-06-04-dead-end-escape-fix.md`.
Relates to **AD-05** / **F-02** (no `state()` getter on domain classes).

---

## Context

The Front Sensor serves two roles that share the same reading:

1. While cruising — a **front-obstacle interrupt** (rising edge: clear → blocked).
2. During obstacle handling — the **Right Scan**: the RVC rotates right and reuses
   `FrontSensor::detect()` to probe its old right side (`CHECKING_RIGHT`).

When the RVC rotates right to scan, its new facing is often a wall, producing a
clear → blocked rising edge on the front reading. The Simulator treated this as a
fresh interrupt and called `onFrontObstacleDetected()`, which issued STOP and
reset the state to `AVOIDING_OBSTACLE`. This hijacked the `CHECKING_RIGHT`
evaluation, so the "right is blocked → ESCAPING" transition never fired and the
multi-tick backward chain was cut after one step — the RVC oscillated. A
dead-end *with* an exit was state-trace-identical to a sealed dead-end, so the
controller could not tell them apart.

## Decision

Let the **controller own** the interrupt acceptance policy.
`onFrontObstacleDetected()` returns a `bool`: while cruising (`CLEANING` /
`INTENSIFYING`) it issues STOP, transitions to `AVOIDING_OBSTACLE`, and returns
`true`; during the avoidance sequence it does nothing and returns `false`. The
Simulator calls the method on the rising edge and falls back to `onTick()` only
when it was not handled:

```cpp
// RvcController — owns the interrupt acceptance policy
bool RvcController::onFrontObstacleDetected() {
    if (_state != RvcState::CLEANING && _state != RvcState::INTENSIFYING) {
        return false;          // ignore while in the avoidance sequence
    }
    _motor->move(Direction::STOP);
    _state = RvcState::AVOIDING_OBSTACLE;
    return true;
}
// Simulator::tick() — fall back to onTick when not handled
bool handled = false;
if (front_blocked && !_prev_front_blocked) {
    handled = _controller.onFrontObstacleDetected();
}
if (!handled) { _controller.onTick(); }
```

During the avoidance sequence (`AVOIDING_OBSTACLE` / `CHECKING_RIGHT` /
`ESCAPING`) the controller returns `false`, so the rising edge is evaluated
through the `onTick()` fallback instead. No `state()` getter is added — the
Simulator never reads the controller's internal state.

- Code: `src/simulator/Simulator.cpp` (`tick()` handled/fallback),
  `src/app/RvcController.cpp` (`onFrontObstacleDetected()` returns `bool`).

## Alternatives Rejected (Out of Scope)

The guard fixes dead-ends **with** an exit. A **fully sealed** region (no exit at
all) still has no termination condition — the RVC backs to the corridor end and
idles. Two ways to detect "no exit" were considered and rejected:

- **Rear sensor.** Adding hardware contradicts the RightSensor removal direction
  (AD-11) — we are taking sensors away, not adding them.
- **Position memory.** Remembering visited cells violates the reactive,
  memoryless design the controller is built on.

The sealed-region case is therefore declared out of scope rather than masked with
an arbitrary stop counter.

## How We Got Here (AD-05 / F-02 Compliance)

The first attempt at this fix put the guard in the Simulator: it read
`_controller.state()`, called `onFrontObstacleDetected()` only when the state was
`CLEANING` / `INTENSIFYING`, and otherwise fell back to `onTick()`. That required
re-adding a `RvcController::state()` getter — which **violated AD-05** ("No
state() Getter on Domain Classes", see `docs/failures/2026-05-17-failures-and-resolutions.md` F-02).
On noticing the violation, we corrected it: the acceptance policy now lives inside
the controller, expressed as the `bool` return of `onFrontObstacleDetected()`, and
the `state()` getter was removed again. The result is AD-05 / F-02 compliant.

## Tradeoff

The interrupt acceptance policy is now decided inside the controller, so the
Simulator no longer depends on the controller's internal state — the coupling the
first attempt introduced is gone, which is itself a win for AD-05. In exchange,
the Front Sensor's two roles no longer collide, dead-end escape with an exit
works, and "exit exists" is once again distinguishable from "no exit" in the
state trace.

## Verification

- New map-based regression tests in `test/simulator/SimulatorTest.cpp` (4 added):
  - `EscapesDeadEndCorridorThroughLeftGap`
  - `EscapesDeadEndCorridorThroughRightGap`
  - `BacksUpFullCorridorThenEscapes`
  - `AvoidanceInterruptDoesNotBreakBackupChain`
- `ctest` — **30/30 passing** (prior 26 unaffected). clang-tidy clean.
