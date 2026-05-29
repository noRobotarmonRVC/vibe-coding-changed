# Use-Case Model

## Revision History

| Version | Date | Changes |
|---|---|---|
| 1.0 | 2026-05-21 | Initial draft |
| 1.1 | 2026-05-29 | Removed Right Sensor actor; UC-02/03/04 no longer read a right sensor — the right side is probed by rotating right and reading the front sensor (AD-11) |

---

## 1. Actors

| Actor | Type | Description |
|---|---|---|
| User | Primary | Starts and stops a cleaning session |
| Front Sensor | External System | Detects obstacles directly ahead; interrupt-driven. Also reused to check the right side after rotating right. |
| Left Sensor | External System | Detects obstacles on the left side; periodic |
| Dust Sensor | External System | Detects dust on the floor; periodic |
| Timer | External System | Provides periodic Tick signals to drive periodic behavior |

---

## 2. Use-Case Diagram

```mermaid
graph LR
    User(["👤 User"])
    Timer(["⏱ Timer"])
    FrontSensor(["📡 Front Sensor"])
    LeftSensor(["📡 Left Sensor"])
    DustSensor(["📡 Dust Sensor"])

    UC01("UC-01\nStart Cleaning Session")
    UC02("UC-02\nNavigate and Clean")
    UC03("UC-03\nAvoid Front Obstacle")
    UC04("UC-04\nEscape Surrounded State")
    UC05("UC-05\nIntensify Cleaning")
    UC06("UC-06\nStop Cleaning Session")

    User --> UC01
    User --> UC06

    Timer --> UC02
    UC02 -- "<<extend>>" --> UC03
    UC02 -- "<<extend>>" --> UC04
    UC02 -- "<<extend>>" --> UC05

    FrontSensor --> UC03
    FrontSensor --> UC04
    LeftSensor --> UC04
    DustSensor --> UC05
```

---

## 3. Use Cases

---

### UC-01: Start Cleaning Session

| Field | Content |
|---|---|
| **ID** | UC-01 |
| **Name** | Start Cleaning Session |
| **Primary Actor** | User |
| **Brief Description** | The user initiates a cleaning session; the RVC begins moving forward and cleaning. |
| **Preconditions** | RVC is powered on and idle. |
| **Postconditions** | RVC is in the active cleaning state, moving forward. |

**Main Success Scenario:**

1. User commands the RVC to start.
2. System activates the Cleaner (On).
3. System commands the Motor to move Forward.
4. System enters the active cleaning loop (UC-02).

---

### UC-02: Navigate and Clean

| Field | Content |
|---|---|
| **ID** | UC-02 |
| **Name** | Navigate and Clean |
| **Primary Actor** | Timer |
| **Brief Description** | On each Tick, the system evaluates sensor states and determines the next navigation and cleaning action. |
| **Preconditions** | Cleaning session is active (UC-01 completed). |
| **Postconditions** | Motor direction and cleaner state are updated for the current Tick. |

**Main Success Scenario (no obstacles, no dust):**

1. Timer fires a Tick.
2. System reads Left Sensor and Dust Sensor states.
3. All sensors report False (no obstacles, no dust).
4. System maintains Motor command: Forward.
5. System maintains Cleaner command: On.

**Alternative Flows:**

| Condition | Extension |
|---|---|
| Front Sensor fires interrupt (obstacle ahead) | → UC-03: Avoid Front Obstacle |
| Front blocked AND Left blocked AND right side (probed by rotating) blocked | → UC-04: Escape Surrounded State |
| Dust Sensor = True | → UC-05: Intensify Cleaning |

---

### UC-03: Avoid Front Obstacle

| Field | Content |
|---|---|
| **ID** | UC-03 |
| **Name** | Avoid Front Obstacle |
| **Primary Actor** | Front Sensor |
| **Brief Description** | When a front obstacle is detected (and the RVC is not fully surrounded), the RVC stops, turns to an open side, and resumes forward navigation. |
| **Preconditions** | Cleaning session is active. Front Sensor fires True. The left side, or the right side (probed by rotating), is open. |
| **Postconditions** | RVC is moving forward on a new heading, cleaner remains On. |

**Main Success Scenario:**

1. Front Sensor triggers interrupt with True.
2. System commands Motor: Stop.
3. System reads the Left Sensor. If the left side is open, it turns left.
4. If the left side is blocked, the system rotates right and reads the Front Sensor to check the right side.
5. If the right side is open, the RVC (now facing it) resumes forward.
6. Cleaner remains On.

**Alternative Flow — Both sides also blocked:**

- Step 4: Left blocked AND right side (probed) blocked → transfer to UC-04.

---

### UC-04: Escape Surrounded State

| Field | Content |
|---|---|
| **ID** | UC-04 |
| **Name** | Escape Surrounded State |
| **Primary Actor** | Front Sensor, Left Sensor |
| **Brief Description** | When obstacles block the front and left sides and the right side (probed by rotating the front sensor) is also blocked, the RVC reverses one cell, turns, and resumes. |
| **Preconditions** | Cleaning session is active. Front = True AND Left = True AND the right side (probed) = True. |
| **Postconditions** | RVC is moving forward on a new heading, cleaner remains On. |

**Main Success Scenario:**

1. System detects Front blocked, Left blocked, and the right side (probed by rotating) blocked.
2. System commands Motor: Backward (one cell).
3. System selects a turn direction.
4. System commands Motor: Turn to selected direction.
5. System commands Motor: Forward.
6. Cleaner remains On.

---

### UC-05: Intensify Cleaning

| Field | Content |
|---|---|
| **ID** | UC-05 |
| **Name** | Intensify Cleaning |
| **Primary Actor** | Dust Sensor |
| **Brief Description** | When the dust sensor detects dust, the cleaner power is increased temporarily. |
| **Preconditions** | Cleaning session is active. Dust Sensor = True. |
| **Postconditions** | Cleaner reverts to normal power (On) after the intensification period ends. |

**Main Success Scenario:**

1. Dust Sensor reports True.
2. System commands Cleaner: Power Up.
3. System continues navigation per UC-02.
4. After the intensification duration elapses, system commands Cleaner: On (normal).

---

### UC-06: Stop Cleaning Session

| Field | Content |
|---|---|
| **ID** | UC-06 |
| **Name** | Stop Cleaning Session |
| **Primary Actor** | User |
| **Brief Description** | The user terminates the cleaning session; the RVC halts movement and turns off the cleaner. |
| **Preconditions** | Cleaning session is active. |
| **Postconditions** | RVC is idle; Motor is stopped; Cleaner is Off. |

**Main Success Scenario:**

1. User commands the RVC to stop.
2. System commands Motor: Stop.
3. System commands Cleaner: Off.
4. System exits the active cleaning loop.

