# Failures and Resolutions — 2026-05-17

Failures encountered during construction and their root causes and resolutions.

---

## Historical Supersession Note - 2026-05-29

### [추가]
- Added this note so older simulator API examples are not mistaken for the current active design.

### [삭제]
- Removed active reliance on the historical `injectRight()` example; it remains below only as a record of the 2026-05-17 naming decision.

### [변경]
- Current simulator right-side input is represented by `injectFront()` during FrontSensor Right Scan setup. See `docs/decisions/2026-05-29-right-scan-and-tick-escape.md`.

---

## F-01: CMake GTest Configuration Failure

### What Failed
`src/CMakeLists.txt` was written with `find_package(GTest REQUIRED)`. The build failed at CMake configure time:

```
CMake Error: Could NOT find GTest
  (missing: GTEST_LIBRARY GTEST_INCLUDE_DIR GTEST_MAIN_LIBRARY)
```

### Root Cause
`libgtest-dev` was not installed on the system. `find_package(GTest REQUIRED)` treats a missing package as a fatal error with no fallback.

### Why It Was Missed
The assumption was that Google Test would be pre-installed on the Ubuntu 24.04 WSL2 environment since it is listed in both the development and CI toolchains in `AGENTS.md`. That assumption was not verified before writing the CMakeLists.

### Resolution
Replaced `find_package(GTest REQUIRED)` with CMake `FetchContent`, which downloads and builds Google Test automatically at configure time:

```cmake
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
FetchContent_MakeAvailable(googletest)
```

This makes the build self-contained regardless of system package state, which is also better for CI environments.

### Prevention
For any test framework dependency, prefer `FetchContent` over `find_package(REQUIRED)` when system installation cannot be guaranteed. Document the GTest version in `CMakeLists.txt` (currently v1.14.0) so upgrades are intentional.

---

## F-02: Getter Anti-Pattern on Domain Class — RvcController::state()

### What Failed
`RvcController` was given a `state()` getter returning the internal `_state` enum:

```cpp
RvcState state() const { return _state; }
```

Tests were written using it:
```cpp
EXPECT_EQ(controller.state(), RvcState::CLEANING);
EXPECT_EQ(controller.state(), RvcState::INTENSIFYING);
```

### Root Cause
The state was needed in three places — tests, `Simulator`, and `ConsoleDisplay` — and the path of least resistance was to expose it directly. The coupling consequence was not considered at the time.

### Why It Is a Problem
- Tests couple to the internal `RvcState` enum. Refactoring the state machine (renaming, splitting, or merging states) breaks every test that asserts on `state()`, even if behavior is unchanged.
- `ConsoleDisplay` depended on `RvcState`, making the UI layer aware of an internal implementation detail of the controller.
- Violates the principle that tests verify observable behavior (outputs), not internal implementation labels.

### Resolution
1. Removed `state()` from `RvcController.hpp` entirely.
2. Rewrote all state-checking assertions to use motor and cleaner output logs:

| Replaced assertion | With behavioral assertion |
|---|---|
| `state() == CLEANING` | `cleaner.last() == ON` or `motor.last() == FORWARD` |
| `state() == INTENSIFYING` | `cleaner.last() == POWER_UP` |
| `state() == IDLE` | `motor.log.empty()` and `cleaner.log.empty()` |

3. Changed `ConsoleDisplay::render(RvcState, Direction, CleanPower)` to `render(Direction, CleanPower)`. The UI now shows only observable actuator outputs.

### Prevention
Domain class public interfaces should expose **commands** and **queries about behavior**, never raw internal state fields. If a caller needs to know the current state, ask: "Is this for display or for driving behavior?" If for display, show the observable outputs instead. If for driving behavior, that logic belongs inside the domain class.

---

## F-03: Setter Naming on Simulator — setState() / setX()

### What Failed
Simulation control methods were named `setState(bool)` on `SimulatedSensor` and `setFront/setLeft/setRight/setDust()` on `Simulator`:

```cpp
void setState(bool state) { _state = state; }   // SimulatedSensor
void setLeft(bool state);                         // Simulator
```

### Root Cause
The names were chosen for brevity without considering that they are indistinguishable — by name alone — from domain setter anti-patterns.

### Why It Is a Problem
The project has an explicit rule against simple get/set functions that expose internal state. A `set*` method on a production class is a red flag. When simulation infrastructure uses the same naming pattern, readers cannot tell whether a `set*` call is violating the rule or is legitimately operating on test infrastructure.

The convention breaks down: "no get/set" cannot be enforced by code review when test doubles are named identically to what the rule forbids.

### Resolution
Renamed all simulation control entry points to `inject*(reading)`:

```cpp
void inject(bool reading);          // SimulatedSensor
void injectLeft(bool reading);      // Simulator
void injectRight(bool reading);
void injectDust(bool reading);
void injectFront(bool reading);
```

The word `inject` clearly signals: "I am artificially feeding a reading into a test double, bypassing real hardware." This is a simulation-specific concept distinct from domain mutation.

### Prevention
Establish a naming convention for test doubles upfront:
- Simulation control (feeding artificial data) → `inject*()`
- Simulation observation (reading recorded outputs) → `last()`, `log()`
- Never use `get*()/set*()` in test infrastructure — the same names that are banned in domain code.
