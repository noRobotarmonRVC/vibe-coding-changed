# Simulator Tick Behavior - 2026-06-04

## [추가]
- Added a simulator running-state guard so `tick()` has no effect before `start()` and after `stop()`.
- Added simulator tests for the running-state contract.

## [삭제]
- Removed idle clock advancement from the console app display loop. The displayed tick count now increases only while the simulator is running.
- Removed the boundary-steering experiment so map edges remain part of the existing `isBlocked()` obstacle path.

## [변경]
- Kept boundary handling minimal: out-of-grid cells are still treated as blocked by `Simulator::isBlocked()`.
- Kept the core `RvcController` right-scan, interrupt guard, and tick-based escape design unchanged by simulator demo behavior.
