# Dead-End Escape Failure and Resolution — 2026-06-04

A control-logic failure found while validating the integrated multi-tick escape
on dead-end maps. Separate from the 2026-05-21 Simulator/UI failures.

---

## F-10: Front Interrupt Hijacks the Right Scan, Breaking Backward Escape

### What Failed
When the RVC entered a dead-end corridor, it never escaped — even when an exit
existed. Instead of backing out along the corridor, it oscillated between two
cells forever. Map `.. < / X X .` (a one-cell corridor with a single gap below)
trapped the RVC indefinitely, and its state trace was **identical** to the
genuinely-sealed map `.. < / X X X`: the controller could not tell "exit exists"
from "no exit."

### Root Cause
The Front Sensor serves two roles that share the same reading:

1. While cruising — a **front-obstacle interrupt** (rising edge: clear → blocked).
2. During obstacle handling — the **right scan**: the RVC rotates right and reuses
   `FrontSensor::detect()` to probe its old right side (`CHECKING_RIGHT`).

When the RVC rotated right to perform the scan, its new facing was often a wall,
producing a rising edge on the front reading. The Simulator interpreted this as a
fresh front-obstacle interrupt and called `onFrontObstacleDetected()`, which
issued STOP and reset the state to `AVOIDING_OBSTACLE`. This **hijacked the
`CHECKING_RIGHT` evaluation** before it could classify the right side, so the
"right is blocked → ESCAPING" transition never fired and the multi-tick backward
chain was cut after a single step. The RVC then re-entered avoidance and drifted
back, oscillating.

### Why It Was Missed
The L-shaped map (exit on the *right*) escaped correctly, because there the
probed cell was *open* — no rising edge, no false interrupt, so `CHECKING_RIGHT`
ran normally. Tests happened to cover the case where the interrupt collision does
not occur, masking the bug for left-exit and no-exit corridors.

### Resolution
Let the controller own the interrupt acceptance policy. `onFrontObstacleDetected()`
returns a `bool`: while cruising (`CLEANING` / `INTENSIFYING`) it STOPs,
transitions to `AVOIDING_OBSTACLE`, and returns `true`; during the avoidance
sequence it does nothing and returns `false`. The Simulator falls back to
`onTick()` only when the interrupt was not handled:

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

Rotations during `AVOIDING_OBSTACLE` / `CHECKING_RIGHT` / `ESCAPING` now return
`false`, so the rising edge is evaluated through the `onTick()` fallback; the
right scan completes and the backward chain runs to the end of the corridor. No
`state()` getter is exposed — the acceptance decision lives in the controller, so
this stays compliant with **AD-05** / **F-02** (no `state()` getter on domain
classes). An earlier attempt did re-add a `state()` getter so the Simulator could
gate the call; that violated AD-05 and was corrected by moving the policy into the
controller's `bool` return. Captured in SRS as FUNC-01 (2026-06-04 change trace).

### Verification
- New map-based regression tests in `SimulatorTest`:
  - `EscapesDeadEndCorridorThroughLeftGap` — exit below: backs out, turns into the gap.
  - `EscapesDeadEndCorridorThroughRightGap` — L-shape, exit above: escapes via right scan.
  - `BacksUpFullCorridorThenEscapes` — 5-cell corridor: backs the full length, then escapes.
  - `AvoidanceInterruptDoesNotBreakBackupChain` — backward chain is not severed.
- `ctest` — **30/30 passing** (prior 26 unaffected). clang-tidy clean.

### Out of Scope
A **fully sealed** region (no exit at all) still has no termination condition —
the RVC backs to the corridor end and idles in place. Detecting "no exit" requires
either a rear sensor (rejected: adding hardware contradicts the RightSensor
removal) or position memory (rejected: violates the reactive, memoryless design).
This case is therefore declared out of scope rather than masked with an arbitrary
stop counter.
