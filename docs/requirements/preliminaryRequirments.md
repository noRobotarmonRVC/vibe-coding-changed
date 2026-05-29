# The RVC Contorl SW

## Revision History

| Version | Date | Changes |
|---|---|---|
| 1.0 | 2026-05-21 | Initial draft |
| 1.1 | 2026-05-29 | Removed Right Sensor Input from the DFD Level 0 and the I/O event table (right side now probed via the front sensor — AD-11) |

---

## Preliminary Requirements for RVC SW Controller
- An RVC automatically cleans and mops household surface.
- It goes straight forward while cleaning
- If its sensors found an obstacle, it stops cleaning, turns aside left or right, and goes forward with cleaning.
- If there are obstacles in both front, left and right, it move backward and turn aside left or right, and goes forward.
- If it detects dust, power up the cleaning for a while.
- We do not consider the detail design and implementation on HW controls.
- We only focus on the automatic cleaning function.

## Future pr Extended Requirements to Consider
- The RVC will add or change sensors.
- It will be able to circulate one spot for a while.
- It will have to communicate with a mobile app.
- It can do machine learning and inferring for more efficient cleaning.

# DFD Level 0 from SASD

- '<-' means input direction.
- '->' means output direction.
```
RVC Control SW
<- Front Sensor Input - Front Sensor
<- Left Sensor Input  - Left Sensor
<- Dust Sensor Input  - Dust Sensor
<- Tick               - Digital Clock
-> Direction          - Motor
-> Clean              - Cleaner
```

| Input/Output Event | Description                                                                                        | Format / Type                     |
| ------------------ | -------------------------------------------------------------------------------------------------- | --------------------------------- |
| Front Sensor Input | Detects obstacles in front of the RVC                                                              | True / False, interrupt           |
| Left Sensor Input  | Detects obstacles in the left side of the RVC periodically.                                        | True / False, Periodic            |
| Dust Sensor Input  | Detects dust on the floor periodically.                                                            | True / False,  Periodic           |
| Direction          | Direction commands to the motor (go forward / turn left with an angle  / turn right with an angle) | Forward / Backward / Left / Right |
| Clean              | Turn off / Turn on / Power Up                                                                      | On / Off / Up                     |



