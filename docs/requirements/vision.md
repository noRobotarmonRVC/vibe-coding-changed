# Vision

## SRS Change Trace - 2026-05-29

### [추가]
- Added Right Scan as the right-side obstacle detection mechanism inside the system boundary.

### [삭제]
- Removed the dedicated Right Sensor as an active input actor for this iteration.

### [변경]
- Changed surrounded obstacle escape to mean Front Sensor, Left Sensor, and Right Scan are all blocked.

## 1. Introduction

This document defines the high-level goals, stakeholders, and product scope for the RVC (Robot Vacuum Cleaner) Control SW. It serves as the authoritative statement of intent for all subsequent phases of the Unified Process.

---

## 2. Problem Statement

| | |
|---|---|
| **The problem of** | repetitive and time-consuming household floor cleaning |
| **affects** | household residents |
| **the impact of which is** | significant time and effort spent on routine manual cleaning |
| **a successful solution would be** | an autonomous RVC that navigates, avoids obstacles, and adapts cleaning power without human intervention |

---

## 3. Stakeholders

| Stakeholder | Role | Interest |
|---|---|---|
| End User | Operates the RVC | Reliable autonomous cleaning, no intervention needed during operation |
| SW Developer | Implements RVC Control SW | Clear requirements, well-defined interfaces, testable behavior |
| HW Engineer | Provides sensors, motor, and cleaner hardware | Well-defined I/O contracts so HW and SW can be developed independently |

---

## 4. Product Overview

The **RVC Control SW** is an embedded control system that autonomously navigates household surfaces and manages cleaning operations. It receives sensor events and periodic timer ticks as inputs, then issues direction commands to the motor and power commands to the cleaner.

The SW does **not** control HW directly at the electrical level. It operates on abstracted I/O events as defined in the system interface contract.

### System Boundary

```
                        ┌─────────────────────┐
 Front Sensor ─────────>│                     │──────────> Motor (Direction)
 Left Sensor  ─────────>│   RVC Control SW    │
 Right Scan ───────────>│                     │──────────> Cleaner (Power)
 Dust Sensor  ─────────>│                     │
 Timer (Tick) ─────────>│                     │
                        └─────────────────────┘
```

---

## 5. Key Features

| ID | Feature | Description |
|---|---|---|
| F-01 | Autonomous Forward Navigation | Moves straight forward and cleans by default |
| F-02 | Single-Side Obstacle Avoidance | Stops, turns left or right, and resumes when only the front is blocked |
| F-03 | Surrounded Obstacle Escape | Moves backward, turns, and resumes when front, left, and right scan are all blocked |
| F-04 | Adaptive Cleaning Intensity | Temporarily powers up the cleaner when dust is detected |

---

## 6. Constraints and Scope

- HW-level motor and sensor control is **out of scope**; the SW interfaces only via abstracted I/O events.
- Implemented in **C++17** on Ubuntu 24.04 (WSL2).
- No external libraries may be added.
- Future extensions (additional sensors, mobile app integration, ML-based navigation) are noted but **not in scope** for this iteration.
