# System Operation Interface

## SRS Change Trace - 2026-05-29

### [추가]
- 오른쪽 장애물 판단을 위한 `Right Scan Result` 입력 의미를 추가한다.
- ESCAPING 상태에서 tick별 이동 명령을 관찰할 수 있어야 한다는 인터페이스 기대를 추가한다.

### [삭제]
- 활성 시스템 입력에서 전용 `Right Sensor` 입력을 삭제한다.

### [변경]
- 장애물 회피 흐름의 오른쪽 상태 출처를 Right Sensor에서 Front Sensor Right Scan으로 변경한다.

---

## 1. 목적

이 문서는 RVC Control SW가 외부 actor, sensor, actuator, simulator와 주고받는 시스템 수준 인터페이스를 정의한다.

---

## 2. 입력 인터페이스

| 인터페이스 | 방향 | 설명 |
|---|---|---|
| `startCleaning()` | User -> System | 청소 세션을 시작한다. |
| `stopCleaning()` | User -> System | 청소 세션을 종료한다. |
| `onFrontObstacleDetected()` | Front Sensor -> System | 전방 장애물 interrupt를 전달한다. |
| `onTick()` | Timer -> System | 주기 제어 tick을 전달한다. |
| `LeftSensor::detect()` | Sensor -> System | 왼쪽 장애물 상태를 반환한다. |
| `FrontSensor::detect()` during Right Scan | Sensor -> System | 오른쪽 방향으로 회전한 상태에서 오른쪽 장애물 상태를 반환한다. |
| `DustSensor::detect()` | Sensor -> System | 먼지 감지 상태를 반환한다. |

---

## 3. 출력 인터페이스

| 인터페이스 | 방향 | 설명 |
|---|---|---|
| `MotorController::move(Direction)` | System -> Motor | 이동, 회전, 정지 명령을 전달한다. |
| `CleanerController::setPower(CleanPower)` | System -> Cleaner | 청소 장치 전원 또는 강도 명령을 전달한다. |

---

## 4. 주요 동작 계약

### 전방 장애물 처리

1. System은 먼저 `STOP`을 명령한다.
2. System은 `RIGHT` 회전으로 오른쪽 scan 자세를 만든다.
3. System은 Front Sensor를 읽어 Right Scan 결과를 얻는다.
4. System은 `LEFT` 회전으로 원래 heading을 복구한다.
5. Left Sensor와 Right Scan 결과를 navigation strategy에 전달한다.

### ESCAPING 처리

- 장애물 감지 tick에서는 후진하지 않는다.
- 다음 tick마다 하나의 motor command만 발생한다.
- 기본 순서는 `BACKWARD`, `LEFT`, `FORWARD`이다.

---

## 5. Simulator 관찰 계약

- simulator는 한 tick에 실제 위치 이동을 한 칸 이하로 반영한다.
- Right Scan 테스트는 별도 right sensor injection이 아니라 front sensor injection으로 표현한다.
- motor log와 cleaner log는 controller의 observable behavior 검증에 사용한다.
