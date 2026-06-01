# The RVC Contorl SW

## SRS Change Trace - 2026-05-29

### [추가]
- Added `Right Scan Result` as the right-side obstacle input derived from a Front Sensor scan.
- Added the requirement that backward escape movement follows the same tick granularity as forward movement.

### [삭제]
- Removed dedicated `Right Sensor Input` from the active system input set.
- Removed the assumption that surrounded escape completes all movement commands in one event.

### [변경]
- Changed surrounded detection from `Front + Left + Right Sensor` to `Front Sensor + Left Sensor + Right Scan`.
- Changed escape behavior so `Backward`, `Turn`, and `Forward` are issued across separate ticks.

## Preliminary Requirements for RVC SW Controller
- An RVC automatically cleans and mops household surface.
- It goes straight forward while cleaning
- If its sensors found an obstacle, it stops cleaning, turns aside left or right, and goes forward with cleaning.
- If there are obstacles in front, left, and the right scan result, it moves backward one cell on a tick, turns aside on a later tick, and goes forward on a later tick.
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
<- Right Scan Result - Front Sensor scan
<- Dust Sensor Input  - Dust Sensor
<- Tick               - Digital Clock
-> Direction          - Motor
-> Clean              - Cleaner
```

| Input/Output Event | Description                                                                                        | Format / Type                     |
| ------------------ | -------------------------------------------------------------------------------------------------- | --------------------------------- |
| Front Sensor Input | Detects obstacles in front of the RVC                                                              | True / False, interrupt           |
| Left Sensor Input  | Detects obstacles in the left side of the RVC periodically.                                        | True / False, Periodic            |
| Right Scan Result | Detects obstacles on the right side by rotating right and sampling the Front Sensor.                | True / False, Event-driven scan<br> |
| Dust Sensor Input  | Detects dust on the floor periodically.                                                            | True / False,  Periodic           |
| Direction          | Direction commands to the motor; forward and backward movement advance one cell per Tick            | Forward / Backward / Left / Right |
| Clean              | Turn off / Turn on / Power Up                                                                      | On / Off / Up                     |



