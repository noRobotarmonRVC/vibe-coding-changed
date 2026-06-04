# RVC Control SW

Robot Vacuum Cleaner control software developed with OOAD / Unified Process artifacts.

## Change Trace - 2026-06-04

### [추가]
- Added map-based dead-end escape regression tests in `SimulatorTest`.
- Added controller-owned front interrupt acceptance: `RvcController::onFrontObstacleDetected()` now returns `bool`.
- Added a simulator running-state guard so background ticks do not advance or mutate simulation state before `start`.
- Added F-10 decision/failure records for the dead-end escape interrupt issue.

### [삭제]
- Removed the boundary-steering experiment from the active simulator path.
- Removed the related boundary-steering regression test.

### [변경]
- Front interrupts are accepted only while cruising (`CLEANING` / `INTENSIFYING`).
- During obstacle handling (`AVOIDING_OBSTACLE`, `CHECKING_RIGHT`, `ESCAPING`), front interrupts are ignored and the simulator falls back to `onTick()`.
- Map edges remain ordinary blocked cells through `Simulator::isBlocked()`.
- The default interactive demo map remains a general obstacle map; dead-end behavior is covered by tests.

### Out of Scope
- Full map coverage planning, visited-cell memory, BFS, and zigzag sweep navigation are not implemented.
- Fully sealed regions with no exit still have no termination condition by design.

## Current Behavior

- Dedicated `RightSensor` is inactive.
- Right-side detection uses `FrontSensor` after a right turn (`Right Scan`).
- Escape movement is tick-based: each tick can apply at most one physical cell movement.
- Console ticks advance only after `start`; `stop` pauses simulator tick effects.
- The controller suppresses false front interrupts during Right Scan so dead-end escape can continue.

## Build and Test

```powershell
cmake -S src -B build
cmake --build build
cd build
ctest --output-on-failure
```

Verified current local result:

```text
34/34 tests passed
```

## Run Interactive Simulator

```powershell
cd C:\Users\0jaso\Documents\Codex\integrated-claude-design
.\build\Debug\rvc_main.exe
```

Commands inside the simulator:

```text
start          start cleaning
stop           stop cleaning
speed <ms>     set tick interval, e.g. speed 100
dust x y       place dust
obstacle x y   place obstacle
help           show commands
quit           exit
```

## Repository Layout

```text
src/
  app/          RvcController and interactive main
  domain/       state, direction, sensor data, navigation strategy
  hal/          sensor stubs and inactive RightSensor legacy type
  interfaces/   controller interfaces
  simulator/    grid simulator, simulated sensors, motor, cleaner
  ui/           console grid display
test/
  app/          RvcController tests
  domain/       navigation strategy tests
  simulator/    simulator integration tests
docs/
  requirements/ SRS, use cases, system operation interface
  design/       domain model, design model, architecture
  decisions/    architecture and change decisions
  failures/     failure analysis and resolutions
```

## Key References

- `docs/decisions/2026-05-29-right-scan-and-tick-escape.md`
- `docs/decisions/2026-06-04-interrupt-guard.md`
- `docs/decisions/2026-06-04-simulator-demo-behavior.md`
- `docs/failures/2026-06-04-dead-end-escape-fix.md`
