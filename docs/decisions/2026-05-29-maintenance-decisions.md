# Maintenance Decisions — 2026-05-29

Decisions made for the RVC Control SW in response to a HW change (right sensor
removed) and a reverse-motion defect (F-06).

---

## AD-11: Right Sensor Removed → Probe the Right Side by Reusing the Front Sensor

**Context**
A HW configuration change removed the right sensor. The right-obstacle reading
is still required to decide the Surrounded state.

**Rationale**
Without a dedicated right sensor, the only way to know the right side is to reuse
an existing sensor. The front sensor can face the right cell after a rotation.
The alternative "add a new sensor" requires an external HW change, and "decide
from the left side only" abandons the right-detection requirement. We choose the
rotate-and-probe approach that satisfies the requirement with existing resources.

**Decision**
When front and left are both blocked, rotate right (RIGHT) and enter
`CHECKING_RIGHT`. On the next tick, read the right side (now the front) via the
front sensor. If open, resume forward; if blocked, rotate left back to the
original heading and declare Surrounded (`ESCAPING`).

**Artifacts**
- `domain/SensorData.hpp` — removed `is_right_blocked`
- `domain/DefaultNavigationStrategy.cpp` — front+left blocked → `BACKWARD` (a
  signal telling the controller to probe right, then escape)
- `app/RvcController` — removed `_right_sensor`; use `_front_sensor` for the
  `CHECKING_RIGHT` probe
- `simulator/Simulator` — removed right auto-injection; inject the front sensor
  every tick so it can be polled

**Trade-off**
Probing the right side adds one rotation (right → back), lengthening the
avoidance sequence. This is a pragmatic way to compensate for the missing
dedicated sensor through motion.

---

## AD-12: Drop Atomic Avoidance/Escape (AD-03) → Multi-Tick State Machine

**Context**
AD-03 handled `AVOIDING_OBSTACLE`/`ESCAPING` atomically inside
`onFrontObstacleDetected()` and returned to `CLEANING` immediately. This applied
back-up + turn + forward all in one tick, making the robot jump several cells in
a single tick (F-06).

**Rationale**
A physical RVC performs only one action (move or turn) per tick. Atomic handling
causes teleport-like motion in simulation, and the right-probe rotation (AD-11)
is inherently multi-tick.

**Decision**
`onFrontObstacleDetected()` only issues STOP and enters `AVOIDING_OBSTACLE` (the
interrupt's immediate halt). `onTick()` then advances one step per tick:

| State | Action per tick | Next state |
|---|---|---|
| AVOIDING_OBSTACLE | check left → turn left / turn right (probe) | CLEANING / CHECKING_RIGHT |
| CHECKING_RIGHT | read right via front sensor → forward / turn back | CLEANING / ESCAPING |
| ESCAPING | back up one cell | AVOIDING_OBSTACLE (re-evaluate) |

**Artifacts**
- `domain/RvcState.hpp` — added `CHECKING_RIGHT`
- `app/RvcController.cpp` — `onTick()` rewritten as a switch-based multi-tick FSM
- `simulator/Simulator.cpp` — fire the front interrupt only on the rising edge
  (clear→blocked); otherwise advance via `onTick()` (`_prev_front_blocked`)

**Trade-off**
A single avoidance now spans several ticks, so it takes longer to complete. This
matches the real robot's one-action-per-tick model and makes each action
observable, so it is accepted.
