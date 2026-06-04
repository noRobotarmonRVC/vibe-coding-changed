# Interrupt Guard — 2026-06-04

Decision record for gating the front-obstacle interrupt on the controller state,
the fix for failure **F-10** (dead-end escape never fires; the RVC oscillates
between two cells). See `docs/failures/2026-06-04-dead-end-escape-fix.md`.

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

Gate the front interrupt on the controller state. It is only meaningful while
the RVC is cruising:

```cpp
const bool cruising = _controller.state() == RvcState::CLEANING
                   || _controller.state() == RvcState::INTENSIFYING;
if (front_blocked && !_prev_front_blocked && cruising) {
    _controller.onFrontObstacleDetected();
} else {
    _controller.onTick();
}
```

During the avoidance sequence (`AVOIDING_OBSTACLE` / `CHECKING_RIGHT` /
`ESCAPING`) the interrupt is suppressed and the rising edge is evaluated through
`onTick()` instead. This required exposing `RvcController::state()` so the
Simulator can read the controller state.

- Code: `src/simulator/Simulator.cpp` (`tick()` cruising guard),
  `src/app/RvcController.hpp` (`state()` getter — reflected in the class diagram).

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

## Tradeoff

The Simulator now depends on `RvcController::state()`, a small added coupling
between the simulator and the controller surface. In exchange, the Front Sensor's
two roles no longer collide, dead-end escape with an exit works, and "exit exists"
is once again distinguishable from "no exit" in the state trace.

## Verification

- New map-based regression tests in `test/simulator/SimulatorTest.cpp` (4 added):
  - `EscapesDeadEndCorridorThroughLeftGap`
  - `EscapesDeadEndCorridorThroughRightGap`
  - `BacksUpFullCorridorThenEscapes`
  - `AvoidanceInterruptDoesNotBreakBackupChain`
- `ctest` — **30/30 passing** (prior 26 unaffected). clang-tidy clean.
