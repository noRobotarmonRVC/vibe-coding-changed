# Simulator Demo Behavior - 2026-06-04

## [추가]
- Added a simulator running-state guard so `tick()` has no effect before `start()` and after `stop()`.
- Added boundary steering for the console demo. When the robot faces outside the grid, the simulator steps one open side cell inward and reverses the sweep direction.
- Added simulator tests for the running-state contract and boundary steering behavior.

## [삭제]
- Removed idle clock advancement from the console app display loop. The displayed tick count now increases only while the simulator is running.
- Removed the demo behavior where an out-of-bounds front cell always entered ordinary obstacle avoidance and could make the robot appear to orbit the outer wall.

## [변경]
- Boundary handling remains a simulator/demo concern. The core `RvcController` right-scan and tick-based escape design is unchanged.
- Physical movement is still limited to at most one grid cell per simulator tick; boundary steering may rotate before and after that one-cell inward step.
