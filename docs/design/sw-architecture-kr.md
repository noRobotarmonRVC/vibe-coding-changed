# SW Architecture Document

## Design Change Trace - 2026-06-01

### [추가]
- application state machine에 `CHECKING_RIGHT`를 추가한다.
- simulator의 front interrupt를 edge-trigger 방식으로 처리한다.

### [삭제]
- 활성 `RightSensor` build 및 controller 의존성을 삭제한다.
- `_escape_step` 기반 escape orchestration을 활성 architecture에서 삭제한다.

### [변경]
- 오른쪽 감지를 오른쪽 회전 후 `FrontSensor`를 읽는 multi-tick controller 흐름으로 변경한다.
- 포위 상태 escape를 한 칸 후진 후 다시 side evaluation으로 돌아가는 방식으로 변경한다.

---

## 1. 개요

RVC Control SW는 계층형 architecture를 따른다. application logic은 concrete hardware class가 아니라 interface와 domain type에 의존한다.

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

- `RvcController`는 모든 의존성을 constructor injection으로 받는다.
- `RightSensor.cpp`는 active CMake source list에서 제외한다.
- 오른쪽은 오른쪽 회전 후 `CHECKING_RIGHT` 상태에서 `FrontSensor`로 확인한다.
- simulator는 robot의 현재 heading 기준으로 front reading을 주입하므로, 오른쪽 회전 후 front sensor는 기존 오른쪽 방향을 의미한다.

---

## 4. 장애물 처리 흐름

```text
Front obstacle rising edge
  -> RvcController::onFrontObstacleDetected()
  -> STOP
  -> state = AVOIDING_OBSTACLE

Timer tick in AVOIDING_OBSTACLE
  -> LeftSensor 확인
  -> left open이면 LEFT
  -> left blocked이면 RIGHT 후 state = CHECKING_RIGHT

Timer tick in CHECKING_RIGHT
  -> FrontSensor를 right-side probe로 읽음
  -> open이면 CLEANING
  -> blocked이면 LEFT 후 state = ESCAPING

Timer tick in ESCAPING
  -> BACKWARD
  -> state = AVOIDING_OBSTACLE
```

---

## 5. Simulator 통합

- front obstacle interrupt는 edge-trigger 방식이다. simulator는 front가 clear에서 blocked로 바뀔 때만 `onFrontObstacleDetected()`를 호출한다.
- front가 계속 blocked인 동안에는 이후 동작을 `onTick()`으로 진행한다.
- `applyPendingMotorCommands()`는 새로 발행된 명령만 순서대로 반영하며, 테스트는 한 tick에 한 칸 초과 이동하지 않음을 검증한다.

---

## 6. 빌드 구조

- `rvc_core`는 main을 제외한 production code를 포함한다.
- `hal/RightSensor.cpp`는 repository에 inactive legacy code로 남지만 compile하지 않는다.
- MSVC에서는 한국어 trace comment를 안정적으로 처리하기 위해 `/utf-8`을 사용한다.
- `rvc_tests`는 domain, controller, simulator behavior를 검증한다.
