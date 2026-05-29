# Failures and Resolutions — 2026-05-29

A defect addressed during the right-sensor removal and multi-tick conversion.

---

## F-06: Reverse Motion Moves Several Cells in One Tick

### What failed
During escape, the robot jumped several cells in a single tick. Forward motion
was consistently one cell per tick, but reverse/escape moved several cells
asymmetrically.

### Root cause
`onFrontObstacleDetected()` queued `STOP → BACKWARD → TURN → FORWARD` into the
motor log all in one tick (AD-03's atomic handling). `Simulator::applyPendingMotorCommands()`
applies queued commands in order, so back-up (1 cell) + forward (1 cell) were
applied back-to-back within a single tick, changing the position twice per tick.

### Why it was missed
AD-03 intentionally handled the transient states (AVOIDING/ESCAPING) atomically,
and we did not recognize the conflict with the simulator's one-action-per-tick
model. The unit tests only checked command order (STOP→BACKWARD→LEFT→FORWARD),
not the actual number of cells moved per tick.

### Resolution
Split avoidance/escape into multiple ticks (AD-12). Reverse motion issues a
single `BACKWARD` per tick in the `ESCAPING` state, then returns to
`AVOIDING_OBSTACLE` to re-evaluate. Added `SimulatorTest.NeverMovesMoreThanOneCellPerTick`
as a regression guard — it asserts the Manhattan displacement per tick is at most 1.

### Prevention
If the simulator is a one-action-per-tick model, the controller must issue only
one motion command per tick. Do not queue multiple motion commands from a single
handler. Pin down not just command order but also per-tick displacement
(observable position change) with a test.
