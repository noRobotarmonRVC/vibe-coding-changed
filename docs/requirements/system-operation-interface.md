# System Operation Interface

| «interface» RVCSystem | `startCleaning()` · `tick()` · `onFrontObstacleDetected()` · `stopCleaning()` |
|---|---|

## SRS Change Trace - 2026-05-29

### [추가]
- Added Right Scan as the system operation detail for right-side obstacle detection.
- Added tick-by-tick ESCAPING behavior to `tick()`.

### [삭제]
- Removed Right Sensor polling from the system operation interface.
- Removed single-event escape completion from `onFrontObstacleDetected()`.

### [변경]
- Changed `onFrontObstacleDetected()` to stop, read Left Sensor, perform Right Scan, then decide AVOIDING_OBSTACLE or ESCAPING.
- Changed ESCAPING motor sequence so `BACKWARD`, `TURN`, and `FORWARD` are emitted by later `tick()` calls.

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
| **Description** | Periodic heartbeat that drives sensor polling and navigation decisions while the RVC is active. On each tick, the system reads Left and Dust sensors; right-side obstacle information is produced by a front-sensor right scan during front-obstacle handling. |
| **Preconditions** | RVC is in an active state (CLEANING, AVOIDING_OBSTACLE, ESCAPING, or INTENSIFYING). |
| **Postconditions** | Motor direction and cleaner state are updated based on current sensor readings. State may transition per the table below. |

**State transitions triggered by `tick()`:**

| Sensor Reading | Resulting State | Motor | Cleaner |
|---|---|---|---|
| No obstacles, no dust | CLEANING | FORWARD | ON |
| Front = True, Left or Right Scan open | AVOIDING_OBSTACLE | STOP → RIGHT scan → TURN → FORWARD | ON |
| Front = True, Left = True, Right Scan = blocked | ESCAPING | BACKWARD → TURN → FORWARD across separate ticks | ON |
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
| **Description** | Interrupt-based notification that an obstacle has been detected directly ahead. Unlike `tick()`, this is asynchronous — the Front Sensor fires this event immediately upon detection, not on a polling cycle. |
| **Preconditions** | RVC is in CLEANING or AVOIDING_OBSTACLE state. |
| **Postconditions** | Motor command = STOP. System reads the Left Sensor and performs a right-side scan with the Front Sensor to determine next state (AVOIDING_OBSTACLE or ESCAPING). |

**Decision logic:**

| Left Sensor | Right Scan | Resulting State |
|---|---|---|
| False (open) | any | AVOIDING_OBSTACLE — turn left |
| any | False (open) | AVOIDING_OBSTACLE — turn right |
| True | True | ESCAPING |

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
 |               |               |  Motor: STOP
 |               |               |  [reads Left + performs Right Scan]
 |               |               |  Motor: TURN → FORWARD
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
