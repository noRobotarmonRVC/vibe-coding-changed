# Dead-End Escape Failure and Resolution - 2026-06-04

Failure record for the integrated Right Scan and multi-tick escape behavior.

## [Added]

- Failure **F-10** documents a false front interrupt during Right Scan.
- Map-based regression tests cover left-exit, right-exit, full-corridor backup,
  and uninterrupted backup-chain scenarios.
- Unit tests cover the controller interrupt acceptance policy.

## [Removed]

- No Simulator-side access to controller state is used.
- No boundary steering, visited-cell memory, BFS, zigzag planner, rear sensor, or
  stop counter is introduced as part of this fix.

## [Changed]

- `onFrontObstacleDetected()` returns `true` only when the interrupt is accepted.
- During avoidance states, front edges are ignored as interrupts and evaluated
  by `onTick()`.
- The fix targets dead-end escape interruption, not full-area coverage.

## F-10: Front Interrupt Hijacks Right Scan

### What Failed

In dead-end corridors, the RVC could oscillate instead of backing out through an
available exit. A map with an exit and a fully sealed map produced the same
state trace, so the controller could not reach the expected escape transition.

### Root Cause

The Front Sensor is reused for Right Scan. When the robot rotated right to check
the old right side, the new front direction could face a wall. That created a
new clear-to-blocked front edge. The Simulator interpreted it as a fresh
front-obstacle interrupt and called `onFrontObstacleDetected()`.

That call issued `STOP` and reset the state to `AVOIDING_OBSTACLE`, which
pre-empted `CHECKING_RIGHT`. Because `CHECKING_RIGHT` did not finish, the
"right blocked -> ESCAPING" transition could be skipped and the backward chain
could break.

### Resolution

The controller owns interrupt acceptance:

- `CLEANING` / `INTENSIFYING`: accept interrupt, issue `STOP`, move to
  `AVOIDING_OBSTACLE`, return `true`.
- `AVOIDING_OBSTACLE` / `CHECKING_RIGHT` / `ESCAPING`: ignore interrupt and
  return `false`.

The Simulator falls back to `onTick()` when the interrupt was not handled. This
lets the Right Scan and multi-tick escape sequence continue without exposing a
controller state getter.

```cpp
bool handled = false;
if (front_blocked && !_prev_front_blocked) {
    handled = _controller.onFrontObstacleDetected();
}
if (!handled) {
    _controller.onTick();
}
```

### Verification

- `RvcControllerTest.FrontInterruptAcceptedWhileCruising`
- `RvcControllerTest.FrontInterruptIgnoredDuringAvoidance`
- `RvcControllerTest.FrontInterruptIgnoredWhileEscaping`
- `SimulatorTest.EscapesDeadEndCorridorThroughLeftGap`
- `SimulatorTest.EscapesDeadEndCorridorThroughRightGap`
- `SimulatorTest.BacksUpFullCorridorThenEscapes`
- `SimulatorTest.AvoidanceInterruptDoesNotBreakBackupChain`

Current local result: **34/34 tests passed** with `ctest --output-on-failure`.

### Remaining Scope

This fix does not make the robot guarantee full map coverage. If the robot keeps
following the outer perimeter, that is a limitation of the existing reactive
navigation model. Solving that properly would require a separate coverage
planning requirement.
