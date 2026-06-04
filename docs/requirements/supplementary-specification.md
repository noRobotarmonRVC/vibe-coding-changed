# Supplementary Specification

This document captures system requirements not expressed in the Use-Case Model. It follows the FURPS+ classification.

---

## SRS Change Trace - 2026-05-29

### [추가]
- Added right-side obstacle detection by Front Sensor right scan.
- Added tick-level movement constraint for backward escape movement.

### [삭제]
- Removed periodic Right Sensor polling from active functional and performance requirements.

### [변경]
- Changed periodic sensor evaluation from `Left, Right, Dust` to `Left, Dust`.
- Changed motor movement semantics so `BACKWARD` movement is constrained to one cell per Tick, matching `FORWARD`.

---

## SRS Change Trace - 2026-06-04

### [변경]
- Constrained the Front Sensor interrupt (FUNC-01) to fire only while cruising (`CLEANING`/`INTENSIFYING`). Because the Front Sensor is reused for the right scan (FUNC-02), a rotation during obstacle handling raised a false rising-edge interrupt that hijacked the `CHECKING_RIGHT` evaluation and broke the multi-tick backward escape, trapping the RVC in dead-end corridors even when an exit existed (see failure F-10).

---

## 1. Functionality

Functional requirements are fully covered by the Use-Case Model (`use-case-model.md`). This section records functional constraints not tied to a specific use case.

| ID | Requirement |
|---|---|
| FUNC-01 | The system must process Front Sensor input as an interrupt (not polled) **while cruising (`CLEANING`/`INTENSIFYING`)**, ensuring immediate response to front obstacles. The interrupt is **suppressed during obstacle-handling states** (`AVOIDING_OBSTACLE`, `CHECKING_RIGHT`, `ESCAPING`), where the Front Sensor is reused for the right scan (FUNC-02) and a rotation would otherwise raise a false interrupt. [변경] |
| FUNC-02 | Left and Dust Sensor inputs are evaluated on each Timer Tick (periodic polling). Right-side obstacle status is evaluated through a Front Sensor right scan during front-obstacle handling. |
| FUNC-03 | Motor direction commands are mutually exclusive; only one direction is active at a time. |
| FUNC-04 | Cleaner power states are mutually exclusive: Off, On, or Power Up. |
| FUNC-05 | Forward and backward translation commands must move the RVC at the same granularity: one cell per Tick. |

---

## 2. Usability

Not applicable. The RVC Control SW has no direct user interface; user interaction is limited to start/stop commands (UC-01, UC-06).

---

## 3. Reliability

| ID | Requirement |
|---|---|
| REL-01 | The system must respond to a Front Sensor interrupt within one processing cycle. |
| REL-02 | The system must not issue conflicting motor commands (e.g., Forward and Backward simultaneously). |
| REL-03 | If sensor state cannot be determined, the system must default to a safe state (Motor: Stop, Cleaner: Off). |

---

## 4. Performance

| ID | Requirement |
|---|---|
| PERF-01 | Periodic sensor evaluation (Left, Dust) must complete within a single Tick interval. |
| PERF-02 | The intensified cleaning duration (UC-05) must be configurable as a system constant, not hardcoded inline. |

---

## 5. Supportability

| ID | Requirement |
|---|---|
| SUPP-01 | The design must allow additional sensor types to be integrated without modifying existing sensor-handling logic (Open/Closed Principle). |
| SUPP-02 | Navigation logic must be decoupled from sensor reading logic to facilitate future replacement of the navigation algorithm (e.g., ML-based). |
| SUPP-03 | The system must expose a defined interface boundary for future mobile app communication without requiring changes to core control logic. |

---

## 6. Design Constraints (+)

| ID | Constraint |
|---|---|
| DC-01 | Implementation language: C++17. |
| DC-02 | No external libraries or packages may be introduced. |
| DC-03 | All modules must have corresponding Google Test unit tests. |
| DC-04 | Code must pass clang-tidy static analysis with the project-defined rule set. |
| DC-05 | HW-level control (electrical signals, register access) is out of scope; the SW operates on abstracted I/O events only. |

---

## 7. Test Constraints

| ID | Constraint |
|---|---|
| TEST-01 | `RvcControllerTest` must observe, tick by tick, the `RIGHT` right scan, front detect, `LEFT` heading restore, and `BACKWARD` progression. |
| TEST-02 | `SimulatorTest` must verify that during ESCAPING the position changes by at most one cell per tick. |
| TEST-03 | `SimulatorTest` must verify, with map-based regression tests, that the RVC escapes a dead-end corridor when an exit exists (see failure F-10). The suite must include `EscapesDeadEndCorridorThroughLeftGap`, `EscapesDeadEndCorridorThroughRightGap`, `BacksUpFullCorridorThenEscapes`, and `AvoidanceInterruptDoesNotBreakBackupChain`, confirming that a false front interrupt during avoidance does not sever the multi-tick backward chain. [추가] |
| TEST-04 | `RvcControllerTest` must verify the interrupt-acceptance policy at the unit level: `onFrontObstacleDetected()` returns `true` and stops while cruising (`CLEANING`/`INTENSIFYING`), and returns `false` (ignored, no motor command) during the avoidance sequence (`AVOIDING_OBSTACLE`/`CHECKING_RIGHT`/`ESCAPING`). See failure F-10. [추가] |
| TEST-05 | Existing start, stop, dust, and normal tick behaviors must remain intact. |
