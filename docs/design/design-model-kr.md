# Design Model

## Design Change Trace - 2026-05-29

### [추가]
- `Right Scan`을 오른쪽 장애물 정보의 출처로 추가한다.
- ESCAPING 진행을 tick 단위로 추적하는 `_escape_step`을 추가한다.

### [삭제]
- 활성 `RvcController` 의존성 모델에서 전용 `RightSensor`를 삭제한다.

### [변경]
- `SensorData::is_right_blocked` 의미를 Right Scan blocked로 변경한다.
- 포위 상태 탈출을 단일 이벤트 처리에서 여러 tick 진행으로 변경한다.

---

## 1. 설계 원칙

- **SRP**: sensor 읽기, navigation 결정, actuator 명령, simulator 책임을 분리한다.
- **OCP**: navigation strategy는 controller 수정 없이 교체 가능해야 한다.
- **DIP**: `RvcController`는 concrete HAL이 아니라 `ISensor`, `IMotorController`, `ICleanerController`, `INavigationStrategy`에 의존한다.
- **Testability**: controller는 mock sensor와 mock actuator로 검증 가능해야 한다.

---

## 2. 주요 enum

```cpp
enum class Direction  { FORWARD, BACKWARD, LEFT, RIGHT, STOP };
enum class CleanPower { OFF, ON, POWER_UP };

enum class RvcState {
    IDLE,
    CLEANING,
    AVOIDING_OBSTACLE,
    ESCAPING,
    INTENSIFYING
};
```

---

## 3. 주요 interface

| Interface | 책임 |
|---|---|
| `ISensor` | `detect()`를 통해 sensor 상태를 반환한다. |
| `IMotorController` | `move(Direction)`으로 이동 명령을 수행한다. |
| `ICleanerController` | `setPower(CleanPower)`로 청소 장치를 제어한다. |
| `INavigationStrategy` | `SensorData`를 받아 다음 `Direction`을 결정한다. |

---

## 4. `RvcController`

`RvcController`는 control flow의 중심이다.

주요 책임:
- start/stop 명령 처리
- tick 처리
- front obstacle interrupt 처리
- Right Scan orchestration
- ESCAPING tick progression 관리
- cleaner power up duration 관리

활성 sensor 의존성:
- `_front_sensor`
- `_left_sensor`
- `_dust_sensor`

삭제된 활성 의존성:
- `_right_sensor`

---

## 5. Right Scan 처리

전방 장애물이 감지되면 controller는 다음 순서로 오른쪽 상태를 판단한다.

```text
STOP
RIGHT
FrontSensor::detect()
LEFT
NavigationStrategy::navigate(data)
```

이때 `SensorData::is_right_blocked`는 전용 RightSensor 값이 아니라 Right Scan 결과이다.

---

## 6. ESCAPING 처리

ESCAPING은 실제 상태로 유지되며 tick마다 한 단계씩 진행한다.

| tick | 명령 | 설명 |
|---|---|---|
| obstacle event | `STOP`, `RIGHT`, `LEFT` | 정지와 Right Scan, heading 복구 |
| next tick | `BACKWARD` | 한 칸 후진 |
| next tick | `LEFT` | 기본 탈출 회전 |
| next tick | `FORWARD` | 한 칸 전진 후 cleaning 복귀 |
