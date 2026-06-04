# Interrupt Guard Decision - 2026-06-04

Decision record for the front-obstacle interrupt policy used by the integrated
design. This document is linked to failure **F-10** in
`docs/failures/2026-06-04-dead-end-escape-fix.md`.

## [Added]

- `RvcController::onFrontObstacleDetected()` now returns `bool`.
- The controller owns the interrupt acceptance policy.
- `Simulator::tick()` falls back to `RvcController::onTick()` when the front
  interrupt is ignored.
- Regression coverage was added for dead-end escape and false front interrupts
  during Right Scan.

## [Removed]

- No `RvcController::state()` getter is added.
- No boundary-steering algorithm is kept in the Simulator.
- No visited-cell memory, BFS, zigzag sweep, rear sensor, or arbitrary stop
  counter is introduced.

## [Changed]

- A front-obstacle interrupt is accepted only while cruising:
  `CLEANING` or `INTENSIFYING`.
- During `AVOIDING_OBSTACLE`, `CHECKING_RIGHT`, and `ESCAPING`, the same front
  sensor is being reused for Right Scan, so the interrupt is ignored and normal
  tick progression continues.
- The simulator still detects a clear-to-blocked front edge, but the controller
  decides whether that edge is a valid interrupt.
- The console tick counter and simulator ticks advance only after `start()`.

## Context

The Front Sensor has two roles:

1. While cruising, it reports a front-obstacle interrupt.
2. During obstacle handling, the robot rotates right and reuses the Front Sensor
   to scan the old right side.

Before this decision, a right-scan rotation could face a wall and create a new
clear-to-blocked front edge. The Simulator treated that edge as a fresh obstacle
interrupt, called `onFrontObstacleDetected()`, and reset the controller back to
`AVOIDING_OBSTACLE`. That interrupted `CHECKING_RIGHT`, so the transition to
`ESCAPING` was skipped and the multi-tick backward chain could break.

## Decision

The controller decides whether a front interrupt is valid.

```cpp
bool RvcController::onFrontObstacleDetected() {
    if (_state != RvcState::CLEANING && _state != RvcState::INTENSIFYING) {
        return false;
    }
    _motor->move(Direction::STOP);
    _state = RvcState::AVOIDING_OBSTACLE;
    return true;
}
```

The Simulator calls this method on the rising edge and continues with `onTick()`
when the interrupt was ignored.

```cpp
bool handled = false;
if (front_blocked && !_prev_front_blocked) {
    handled = _controller.onFrontObstacleDetected();
}
if (!handled) {
    _controller.onTick();
}
```

This preserves encapsulation: the Simulator never reads the controller's private
state and no state getter is added.

## Scope

This fixes false interrupts during Right Scan and allows the existing
multi-tick escape sequence to continue. It does not attempt full map coverage.
Perimeter-following can still happen because the current controller remains a
reactive obstacle-avoidance controller, not a coverage planner.

## Verification

- `cmake -S src -B build`
- `cmake --build build`
- `ctest --output-on-failure`
- Current local result: **34/34 tests passed**.
