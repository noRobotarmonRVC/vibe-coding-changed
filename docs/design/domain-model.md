# Domain Model

## Revision History

| Version | Date | Changes |
|---|---|---|
| 1.0 | 2026-05-21 | Initial draft |
| 1.1 | 2026-05-29 | Removed RightSensor (conceptual class, attribute, associations, generalization); updated Obstacle and Surrounded State; the right side is now sensed by rotating and reusing the FrontSensor (AD-11) |

---

The Domain Model identifies the key conceptual classes in the RVC problem domain, their attributes, and their relationships. These are not software classes — they represent real-world concepts.

---

## 1. Conceptual Classes

| Class | Description |
|---|---|
| **RVC** | The robot vacuum cleaner. The central entity that navigates and cleans. |
| **CleaningSession** | A single run from start to stop. Tracks duration and state. |
| **Sensor** | Abstract concept for any input device on the RVC that detects environment state. |
| **FrontSensor** | Interrupt-driven sensor detecting obstacles directly ahead. Also reused to check the right side after rotating right (AD-11). |
| **LeftSensor** | Periodic sensor detecting obstacles on the left side. |
| **DustSensor** | Periodic sensor detecting dust on the floor surface. |
| **Obstacle** | A physical object that blocks the RVC's path. Detected by the Front and Left sensors (the right side is probed via the front sensor). |
| **Dust** | Particulate matter on the floor surface. Detected by the DustSensor. |
| **Motor** | Drives the physical movement of the RVC. Receives direction commands. |
| **Cleaner** | The vacuum/mop mechanism. Receives power-level commands. |
| **Timer** | Digital clock that generates periodic Tick signals to drive the control loop. |
| **DirectionCommand** | A command to the Motor: Forward, Backward, Left (turn), Right (turn), Stop. |
| **CleaningCommand** | A command to the Cleaner: Off, On, Power Up. |

---

## 2. Attributes

| Class | Attributes |
|---|---|
| CleaningSession | state {Idle, Active, Stopped} |
| FrontSensor | is_triggered : bool |
| LeftSensor | is_blocked : bool |
| DustSensor | has_dust : bool |
| DirectionCommand | value {Forward, Backward, Left, Right, Stop} |
| CleaningCommand | value {Off, On, PowerUp} |
| Timer | tick_interval : duration |

---

## 3. Associations

```
RVC ──────────────── conducts ────────────────> CleaningSession
RVC ──────────────── has ──────────────────1──> FrontSensor
RVC ──────────────── has ──────────────────1──> LeftSensor
RVC ──────────────── has ──────────────────1──> DustSensor
RVC ──────────────── driven by ────────────1──> Timer
RVC ──────────────── commands ─────────────1──> Motor          (via DirectionCommand)
RVC ──────────────── commands ─────────────1──> Cleaner        (via CleaningCommand)

FrontSensor ──────── detects ──────────────*──> Obstacle       (interrupt; also right side after rotating)
LeftSensor ───────── detects ──────────────*──> Obstacle       (periodic)
DustSensor ───────── detects ──────────────*──> Dust           (periodic)

FrontSensor ─────────────────┐
LeftSensor ──────────────────┤──── generalize ──────────────── Sensor
DustSensor ──────────────────┘
```

---

## 4. Key Domain Rules

- A CleaningSession is Active only while both Motor and Cleaner are operating.
- The RVC issues exactly one DirectionCommand and one CleaningCommand at any point in time.
- FrontSensor triggers asynchronously (interrupt); all other sensors are read synchronously per Tick.
- An Obstacle blocking the front and left, with the right side (probed via the front sensor) also blocked, defines the **Surrounded State**, which requires backward movement before turning.
- Dust detection during active cleaning raises CleaningCommand to PowerUp temporarily; it does not stop navigation.
