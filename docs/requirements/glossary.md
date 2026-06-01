# Glossary

Terms are listed alphabetically. Abbreviations are expanded on first use.

---

| Term | Definition |
|---|---|
| **Cleaner** | The hardware component responsible for vacuuming and mopping. Accepts power commands: Off, On, Power Up. |
| **Control SW** | The RVC Control Software — the software system being developed in this project. Processes sensor inputs and issues actuator commands. |
| **Direction Command** | A command issued to the Motor specifying movement: Forward, Backward, Left (turn), or Right (turn). |
| **Dust Sensor** | A periodic sensor that reports True when dust is detected on the floor, False otherwise. |
| **Front Sensor** | An interrupt-driven sensor that reports True when an obstacle is detected directly ahead of the RVC. |
| **I/O Event** | An abstracted input or output signal used by the Control SW. Decouples SW logic from HW implementation. |
| **Interrupt** | A hardware-triggered asynchronous signal. The Front Sensor uses an interrupt so the system responds immediately, without waiting for the next Tick. |
| **Left Sensor** | A periodic sensor that reports True when an obstacle is detected on the left side of the RVC, False otherwise. |
| **Motor** | The hardware component that drives RVC movement. Accepts Direction Commands. |
| **OOAD** | Object-Oriented Analysis and Design — the development methodology used in this project. |
| **Obstacle** | Any physical object detected by the Front Sensor, Left Sensor, or right-side front-sensor scan that blocks the RVC's path. |
| **Periodic** | A sensor reading mode where the sensor state is sampled once per Tick interval. |
| **Power Up** | A Cleaner command that increases cleaning intensity above the normal On level, used when dust is detected. |
| **Right Scan** | A right-side obstacle check performed by turning right, sampling the Front Sensor, and turning left to restore the original heading. |
| **RVC** | Robot Vacuum Cleaner — the autonomous household cleaning device whose control software is developed in this project. |
| **Safe State** | The fallback state when the system encounters an indeterminate condition: Motor stopped, Cleaner off. |
| **Simulator** | A software component that emulates the RVC hardware environment for integration testing purposes. |
| **Surrounded State** | The condition where Front Sensor = True, Left Sensor = True, and Right Scan = blocked, requiring the RVC to reverse before turning. |
| **Tick** | A periodic signal from the Digital Clock that drives the main control loop of the RVC Control SW. |
| **Timer** | The Digital Clock subsystem that generates Tick signals at a fixed interval. |
| **TDD** | Test-Driven Development — tests are written before the implementation code. |
| **UI** | User Interface — a separate component (out of scope for Control SW) through which the end user interacts with the RVC. |
| **UP** | Unified Process — the iterative, use-case-driven software development process followed in this project. Phases: Inception → Elaboration → Construction. |
| **V&V** | Verification and Validation — confirming the system is built correctly (verification) and that it is the right system (validation). |
