# Claude Design Absorption - 2026-06-01

Decision record for `integrated-claude-design`, which goes one step beyond
`integrated-code` (codex base + 2 claude tests) by absorbing claude's **design**,
not just its tests.

`integrated-code` kept codex's state machine and pulled in only two tests, citing
submission stability and document traceability. But codex itself rated claude's
multi-tick granularity as superior, so discarding that design was inconsistent.
This branch shows the build stability and doc quality of codex can be preserved
while still absorbing claude's design.

---

## [추가] Added
- Absorbed claude's explicit multi-tick state machine: `CHECKING_RIGHT` added to `RvcState`.
  - Right detection (turn right → probe with front sensor → restore) becomes an observable state.
- Replaced with claude's full 26-test suite. New vs codex's 22:
  - `SimulatorTest.DeadEndBacksUpMultipleTimes`
  - `SimulatorTest.FrontInterruptFiresOnceWhileStuck`
  - `SimulatorTest.SurroundedEscapeBacksUpThenResumesForward`
  - `SimulatorTest.BacksUpAlongOriginalHeadingAfterProbe`
  - `SimulatorTest.NeverMovesMoreThanOneCellPerTick`

## [삭제] Removed
- Removed codex's `is_right_blocked` from `SensorData` — "no right sensor" is now structural.
- Dropped codex's `_escape_step` counter / `continueEscaping()` in favor of an explicit state.

## [변경] Changed
- Switched the state-machine implementation from codex (`_escape_step` counter) to
  claude (`CHECKING_RIGHT` state).
- Ported `RvcController`, `DefaultNavigationStrategy`, and `Simulator` to claude's design.

## [유지] Preserved (codex assets)
- `src/app/main.cpp` — kept codex's `#ifdef _WIN32` portable path (not claude's termios-only).
- `src/CMakeLists.txt` — kept `RightSensor.cpp` build exclusion + MSVC `/utf-8`.
- Korean document cleanup and `[추가] [삭제] [변경]` trace convention preserved.

---

## Tradeoff
One extra state (`CHECKING_RIGHT`) over codex's simpler machine, in exchange for an
observable/testable right-scan step and a structurally explicit "no right sensor."
Build stability and doc traceability are preserved by keeping codex's assets.

## Verification
- `cmake -S src -B build && cmake --build build` succeeds.
- `ctest` — **26/26 passing**.
- `.clang-tidy` config is identical to claude-code, where this code already passed.
