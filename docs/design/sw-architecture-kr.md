# SW Architecture Document

## Design Change Trace - 2026-05-29

### [추가]
- FrontSensor 기반 Right Scan 결정을 추가한다.
- tick 단위 ESCAPING 결정을 추가한다.

### [삭제]
- 활성 controller/build architecture에서 전용 `RightSensor`를 삭제한다.

### [변경]
- 전방 장애물 처리와 simulator 통합을 Right Scan 기준으로 변경한다.

---

## 1. 개요

RVC Control SW는 계층형 아키텍처를 따른다. 상위 계층은 하위 계층의 concrete 구현이 아니라 interface에 의존하며, domain logic은 hardware 세부사항과 분리된다.

---

## 2. 계층 구조

```text
Application Layer
  - RvcController
  - main

Domain Layer
  - DefaultNavigationStrategy
  - SensorData
  - Direction / CleanPower / RvcState

Interface Layer
  - ISensor
  - IMotorController
  - ICleanerController
  - INavigationStrategy

HAL / Simulator / UI Layer
  - FrontSensor, LeftSensor, DustSensor
  - Simulator, SimulatedSensor, SimulatedMotor, SimulatedCleaner
  - ConsoleDisplay, GridDisplay
```

---

## 3. 핵심 의존성

- `RvcController`는 sensor, motor, cleaner, navigation strategy를 constructor injection으로 받는다.
- `RightSensor`는 active build target에서 제외되며 legacy file로만 남는다.
- 오른쪽 감지는 `FrontSensor`를 이용한 Right Scan으로 수행한다.
- simulator는 오른쪽 grid 상태를 Right Scan 시점의 front sensor reading으로 주입한다.

---

## 4. 장애물 처리 흐름

```text
Front obstacle interrupt
  -> STOP
  -> read left sensor
  -> RIGHT
  -> read front sensor as Right Scan
  -> LEFT
  -> navigate(sensor data)
  -> avoid or enter ESCAPING
```

---

## 5. ESCAPING 흐름

ESCAPING은 atomic operation이 아니다. controller는 `_escape_step`을 사용해 진행 단계를 기억하고, `onTick()` 호출마다 하나의 motor command만 발행한다.

```text
step 0: BACKWARD
step 1: LEFT
step 2: FORWARD and return CLEANING
```

---

## 6. 빌드 구조

- `rvc_core`는 main을 제외한 core translation unit을 포함한다.
- `hal/RightSensor.cpp`는 active source list에서 제외한다.
- MSVC 빌드에서는 한국어 trace comment를 안정적으로 처리하기 위해 `/utf-8` compile option을 사용한다.
- `rvc_tests`는 Google Test 기반으로 domain, controller, simulator behavior를 검증한다.
