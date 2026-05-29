# Supplementary Specification

## Revision History

| Version | Date | Changes |
|---|---|---|
| 1.0 | 2026-05-21 | Initial draft |
| 1.1 | 2026-05-29 | Removed the right sensor from FUNC-02 and PERF-01 periodic-polling lists (right side now probed via the front sensor — AD-11) |

---

This document captures system requirements not expressed in the Use-Case Model. It follows the FURPS+ classification.

---

## 1. Functionality

Functional requirements are fully covered by the Use-Case Model (`use-case-model.md`). This section records functional constraints not tied to a specific use case.

| ID | Requirement |
|---|---|
| FUNC-01 | The system must process Front Sensor input as an interrupt (not polled), ensuring immediate response to front obstacles. |
| FUNC-02 | Left and Dust Sensor inputs are evaluated on each Timer Tick (periodic polling). The right side has no dedicated sensor; it is probed by rotating right and reading the front sensor during avoidance (AD-11). |
| FUNC-03 | Motor direction commands are mutually exclusive; only one direction is active at a time. |
| FUNC-04 | Cleaner power states are mutually exclusive: Off, On, or Power Up. |

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
