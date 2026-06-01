# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Change Trace - 2026-05-29

### [추가]
- Added Right Scan terminology and tick-by-tick ESCAPING behavior to the current working guidance.

### [삭제]
- Removed dedicated RightSensor from the active controller dependency description.

### [변경]
- Changed architecture guidance so right-side detection is FrontSensor scan based.

## Project Overview

This project builds an RVC (Robot Vacuum Cleaner) system using OOAD based on the Unified Process (UP). It produces three components: **RVC Control SW**, **RVC UI**, and **RVC Simulator**. Development follows UP phases sequentially: Inception → Elaboration → Construction.

- Language: C++17
- Environment: Ubuntu 24.04 WSL2
- Methodology: OOAD / UP, V&V, TDD

## Build Commands

```bash
# Configure (from project root)
cmake -S src -B build

# Build
cmake --build build

# Run all tests
cd build && ctest --output-on-failure

# Run a single test binary
./build/<test-binary-name>

# Static analysis
clang-tidy src/**/*.cpp -- -std=c++17

# Coverage (after building with coverage flags)
gcovr -r . --html --html-details -o coverage/index.html
```

## Strict Prohibitions (Highest Priority)

- **Do not add external packages or libraries** beyond what is already present.
- **Do not delete any file or directory.**

## UP Phase Outputs

Each phase must produce specific artifacts before advancing:

| Phase | Artifacts |
|---|---|
| Inception | Use-Case Model, Vision, Supplementary Specification, Non-Functional Requirements, Glossary |
| Elaboration | Domain Model, Design Model, SW Architecture Document, refined Inception outputs |
| Construction | RVC Control SW, RVC UI, RVC Simulator |

Each iteration must implement all three components (Control SW, UI, Simulator) and pass integration testing via the Simulator.

## Code Conventions

### Naming
| Construct | Style | Example |
|---|---|---|
| Class | PascalCase | `SensorController` |
| Interface | PascalCase with `I` prefix | `ISensor` |
| Abstract Class | PascalCase with `Abstract` prefix | `AbstractCleaner` |
| Method | camelCase | `detectObstacle()` |
| Variable (public) | snake_case | `front_sensor` |
| Variable (private) | snake_case with `_` prefix | `_speed` |
| Constant | UPPER_SNAKE_CASE | `MAX_SPEED` |
| Directory | kebab-case | `sensor-drivers/` |

### Rules
- Apply SOLID principles throughout.
- Boolean variables/methods use `is` or `has` prefix (e.g., `is_blocked`, `has_dust`).
- Use standard English plurals for collections: `sensors` not `sensor_list`.
- Write and execute unit tests (Google Test) for every new or modified feature.

## Architecture

The RVC Control SW uses a **4-layer architecture**: Application → Domain → Interface → HAL. Full detail is in `docs/design/sw-architecture.md`.

**Key design decisions:**
- `RvcController` is the central orchestrator with an explicit `RvcState` state machine (IDLE / CLEANING / AVOIDING_OBSTACLE / ESCAPING / INTENSIFYING).
- Navigation logic lives in `INavigationStrategy` / `DefaultNavigationStrategy` — injected into `RvcController` so it can be swapped (e.g., ML-based) without touching the controller.
- `FrontSensor` is interrupt-driven (`onInterrupt()` → `onFrontObstacleDetected()`); Left and Dust sensors are polled each Tick via `detect()`. Right-side obstacles are checked by a right turn, FrontSensor read, and left turn restore.
- All hardware dependencies are injected via constructor (`ISensor*`, `IMotorController*`, `ICleanerController*`) — enables full Google Test isolation with mocks.

**Enums:** `Direction` {FORWARD, BACKWARD, LEFT, RIGHT, STOP} · `CleanPower` {OFF, ON, POWER_UP} · `RvcState`

## Source Layout

```
src/
├── interfaces/        # ISensor, IMotorController, ICleanerController, INavigationStrategy
├── domain/            # SensorData, enums, DefaultNavigationStrategy
├── hal/               # FrontSensor, LeftSensor, DustSensor
└── app/               # RvcController, main.cpp
test/
├── domain/            # DefaultNavigationStrategyTest
└── app/               # RvcControllerTest
docs/
├── requirements/      # Vision, Use-Case Model, Supplementary Spec, Glossary
└── design/            # Domain Model, Design Model, SW Architecture
```

## CI Pipeline (GitHub Actions)

The CI pipeline runs on every push and executes: CMake build → Google Test → clang-tidy → gcovr coverage report.
