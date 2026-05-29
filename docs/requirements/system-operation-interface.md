# System Operation Interface

## Revision History

| Version | Date | Changes |
|---|---|---|
| 1.0 | 2026-05-21 | Initial draft |
| 1.1 | 2026-05-29 | Removed right-sensor reads from `tick()`/`onFrontObstacleDetected()`; `onFrontObstacleDetected()` now only issues STOP and enters AVOIDING_OBSTACLE; decision logic reworked for multi-tick avoidance with right-probe (AD-11, AD-12) |

---

| «interface» RVCSystem | `startCleaning()` · `tick()` · `onFrontObstacleDetected()` · `stopCleaning()` |
|---|---|

System Operations are derived from the system events identified in Use-Case scenarios. Each operation represents a message sent to the RVC system by an external actor.

---

## 1. Summary

| Operation | Trigger Actor | Related UC |
|---|---|---|
| `startCleaning()` | User | UC-01 |
| `tick()` | Timer | UC-02, UC-03, UC-04, UC-05 |
| `onFrontObstacleDetected()` | Front Sensor | UC-03, UC-04 |
| `stopCleaning()` | User | UC-06 |

---

## 2. System Operations

---

### SO-01: `startCleaning()`

| Field | Content |
|---|---|
| **Operation** | `startCleaning()` |
| **Related UC** | UC-01: Start Cleaning Session |
| **Trigger Actor** | User |
| **Description** | Transitions the RVC from IDLE to CLEANING state, activates the cleaner, and begins forward motion. |
| **Preconditions** | RVC is in IDLE state. |
| **Postconditions** | RVC state = CLEANING. Motor command = FORWARD. Cleaner command = ON. |

**Cross-references:** UC-01 Step 1–4

---

### SO-02: `tick()`

| Field | Content |
|---|---|
| **Operation** | `tick()` |
| **Related UC** | UC-02, UC-03, UC-04, UC-05 |
| **Trigger Actor** | Timer |
| **Description** | Periodic heartbeat that drives all sensor polling and navigation decisions while the RVC is active. On each tick, the system reads the Left and Dust sensors and advances the avoidance/escape state machine one step. The right side, when needed, is probed by rotating right and reading the front sensor. |
| **Preconditions** | RVC is in an active state (CLEANING, AVOIDING_OBSTACLE, ESCAPING, or INTENSIFYING). |
| **Postconditions** | Motor direction and cleaner state are updated based on current sensor readings. State may transition per the table below. |

**State transitions triggered by `tick()`:**

| Sensor Reading | Resulting State | Motor | Cleaner |
|---|---|---|---|
| No obstacles, no dust | CLEANING | FORWARD | ON |
| Front blocked, left or right (probed) open | AVOIDING_OBSTACLE → CHECKING_RIGHT → CLEANING | one step per tick (turn, then forward) | ON |
| Front blocked, left blocked, right (probed) blocked | ESCAPING | BACKWARD (one cell per tick), then re-evaluate | ON |
| Dust = True | INTENSIFYING | FORWARD | POWER_UP |
| Intensification duration elapsed | CLEANING | FORWARD | ON |

**Cross-references:** UC-02 Main Scenario and Alternative Flows

---

### SO-03: `onFrontObstacleDetected()`

| Field | Content |
|---|---|
| **Operation** | `onFrontObstacleDetected()` |
| **Related UC** | UC-03, UC-04 |
| **Trigger Actor** | Front Sensor (interrupt-driven) |
| **Description** | Interrupt-based notification that an obstacle has been detected directly ahead. Unlike `tick()`, this is asynchronous — it fires immediately upon detection. It only issues STOP and enters AVOIDING_OBSTACLE; the actual avoidance/escape proceeds over later ticks (AD-12). |
| **Preconditions** | RVC is in CLEANING state. |
| **Postconditions** | Motor command = STOP; state = AVOIDING_OBSTACLE. |

**Decision logic (advances one step per `onTick()`):**

| Step (state) | Condition | Action | Next State |
|---|---|---|---|
| interrupt | front blocked | STOP | AVOIDING_OBSTACLE |
| AVOIDING_OBSTACLE | left open | turn left | CLEANING |
| AVOIDING_OBSTACLE | left blocked | turn right (probe) | CHECKING_RIGHT |
| CHECKING_RIGHT | right open | resume forward | CLEANING |
| CHECKING_RIGHT | right blocked | turn back | ESCAPING |
| ESCAPING | — | back up one cell | AVOIDING_OBSTACLE |

**Cross-references:** UC-03 Step 1–3, UC-03 Alternative Flow, UC-04 Step 1

---

### SO-04: `stopCleaning()`

| Field | Content |
|---|---|
| **Operation** | `stopCleaning()` |
| **Related UC** | UC-06: Stop Cleaning Session |
| **Trigger Actor** | User |
| **Description** | Terminates the active cleaning session, halts the motor, and turns off the cleaner. |
| **Preconditions** | RVC is in any active state (CLEANING, AVOIDING_OBSTACLE, ESCAPING, INTENSIFYING). |
| **Postconditions** | RVC state = IDLE. Motor command = STOP. Cleaner command = OFF. |

**Cross-references:** UC-06 Step 1–4

---

## 3. System Sequence Diagram

### Scenario A: Normal Cleaning (no obstacles, no dust)

```
User          RVC System       Timer
 |                |               |
 |--startCleaning()-->            |
 |                |               |
 |                |<----tick()----|
 |                |  [no obstacles, no dust]
 |                |  Motor: FORWARD, Cleaner: ON
 |                |               |
 |                |<----tick()----|
 |                |  (repeats)    |
 |                |               |
 |--stopCleaning()-->             |
 |                |               |
```

### Scenario B: Front Obstacle Avoidance

```
User       Front Sensor     RVC System       Timer
 |               |               |               |
 |--startCleaning()------------>|               |
 |               |               |<----tick()----|
 |               |               |  Motor: FORWARD
 |               |               |               |
 |               |--onFrontObstacleDetected()--->|
 |               |               |  Motor: STOP (enter AVOIDING_OBSTACLE)
 |               |               |  [onTick: reads Left; probes Right by rotating]
 |               |               |  Motor: TURN → FORWARD (over later ticks)
 |               |               |               |
 |               |               |<----tick()----|
 |               |               |  (resumes navigation)
```

### Scenario C: Dust Intensification

```
User          RVC System       Timer        Dust Sensor
 |                |               |               |
 |--startCleaning()-->            |               |
 |                |<----tick()----|               |
 |                |  [Dust = True from Dust Sensor]
 |                |  Cleaner: POWER_UP            |
 |                |               |               |
 |                |<----tick()----|               |
 |                |  [duration elapsed]           |
 |                |  Cleaner: ON (normal)         |
```
