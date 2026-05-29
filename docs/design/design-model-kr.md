# Design Model

## 개정 이력

| 버전 | 날짜 | 변경 내용 |
|---|---|---|
| 1.0 | 2026-05-21 | 최초 작성 |
| 1.1 | 2026-05-29 | `RvcState`에 `CHECKING_RIGHT` 추가; `SensorData`에서 `is_right_blocked` 제거; navigate 규칙·시퀀스·상태머신을 멀티틱+우측 회전 탐지로 갱신 (AD-11, AD-12) |

---

Design Model은 RVC Control SW의 소프트웨어 구조 — 클래스, 인터페이스, 책임, 주요 협력 관계를 정의한다. 모든 명명은 프로젝트 코드 컨벤션을 따른다.

---

## 1. 적용된 Design Principles

- **SRP**: 각 클래스는 변경 이유가 하나다 (센서 읽기, navigation 로직, 액추에이터 제어, 오케스트레이션이 분리됨).
- **OCP**: 새로운 센서 타입과 navigation strategy는 기존 클래스 수정 없이 새 클래스 추가로 확장된다.
- **LSP**: 모든 `ISensor` 구현체는 `RvcController`에서 상호 교체 가능하다.
- **ISP**: `ISensor`, `IMotorController`, `ICleanerController`는 별도 인터페이스이며, 어떤 클래스도 관련 없는 메서드를 구현하도록 강제되지 않는다.
- **DIP**: `RvcController`는 구체 구현이 아닌 추상화(`ISensor`, `IMotorController`, `INavigationStrategy`)에 의존한다.

---

## 2. Enumerations

```cpp
enum class Direction  { FORWARD, BACKWARD, LEFT, RIGHT, STOP };
enum class CleanPower { OFF, ON, POWER_UP };

enum class RvcState {
    IDLE,
    CLEANING,           // 정상 전진 navigation
    AVOIDING_OBSTACLE,  // 전방 차단, 다음 단계 결정
    CHECKING_RIGHT,     // 우회전하여 전방 센서로 우측 확인 중
    ESCAPING,           // 포위 상태; 틱당 한 칸씩 후진
    INTENSIFYING        // 먼지 감지, power up 활성
};
```

---

## 3. Interfaces

### `ISensor`
```
ISensor
─────────────────────────────
+ detect() : bool   {pure virtual}
```
구현체: `FrontSensor`, `LeftSensor`, `DustSensor`. (`RightSensor`는 2026-05-29 제거 — AD-11 참조; 우측은 우회전 후 전방 센서를 재활용해 탐지한다.)

### `IMotorController`
```
IMotorController
─────────────────────────────
+ move(direction : Direction) : void   {pure virtual}
```

### `ICleanerController`
```
ICleanerController
─────────────────────────────
+ setPower(power : CleanPower) : void   {pure virtual}
```

### `INavigationStrategy`
```
INavigationStrategy
─────────────────────────────
+ navigate(data : SensorData) : Direction   {pure virtual}
```
navigation 알고리즘을 제어 오케스트레이션으로부터 분리한다. `RvcController`를 수정하지 않고도 규칙 기반 로직을 ML 기반 strategy로 교체할 수 있게 한다.

---

## 4. Value Object

### `SensorData`
단일 제어 사이클의 모든 센서 readings를 집계한다.

```
SensorData
─────────────────────────────
+ is_front_blocked : bool
+ is_left_blocked  : bool
+ has_dust         : bool
```

---

## 5. Classes

### `FrontSensor : ISensor`
```
FrontSensor
─────────────────────────────
- _triggered : bool
─────────────────────────────
+ detect() : bool
+ onInterrupt() : void    ← interrupt 핸들러에서 호출
```

### `LeftSensor : ISensor`, `DustSensor : ISensor`
```
[Sensor]
─────────────────────────────
- _state : bool
─────────────────────────────
+ detect() : bool
```
> `RightSensor`는 파일로는 남아 있으나 컨트롤러에 연결되지 않는다(AD-11). 우측은 우회전 후 전방 센서를 읽어 탐지한다.

### `DefaultNavigationStrategy : INavigationStrategy`
요구사항에서 정의한 규칙 기반 navigation 로직을 구현한다.

```
DefaultNavigationStrategy
─────────────────────────────
+ navigate(data : SensorData) : Direction
```

인코딩된 규칙 (우측 센서 없음 — AD-11 참조):
1. 전방 + 좌측 차단 → BACKWARD (컨트롤러에 "우측 탐지 후 탈출" 신호)
2. 전방 차단, 좌측 개방 → LEFT
3. 장애물 없음 → FORWARD

### `RvcController`
중앙 오케스트레이터. 모든 컴포넌트 참조를 보유하고 RVC state machine을 관리한다.

```
RvcController
─────────────────────────────────────────────────────
- _front_sensor      : ISensor*   ← 우측 탐지에도 재활용
- _left_sensor       : ISensor*
- _dust_sensor       : ISensor*
- _motor             : IMotorController*
- _cleaner           : ICleanerController*
- _nav_strategy      : INavigationStrategy*
- _state             : RvcState
- _intensify_ticks   : int       ← PowerUp 지속 시간 카운트다운
─────────────────────────────────────────────────────
+ start() : void
+ stop()  : void
+ onTick() : void                ← Timer tick마다 호출
+ onFrontObstacleDetected() : void   ← interrupt에서 호출
```

---

## 6. Class Diagram (개요)

```
          ┌─────────────────────────────────────────────────┐
          │                  RvcController                   │
          │  - _state : RvcState                            │
          └──┬────────┬────────┬────────┬────────┬─────────┘
             │        │        │        │        │
         ISensor  ISensor  ISensor  IMotorController  ICleanerController  INavigationStrategy
             │        │        │
       FrontSensor  LeftSensor  DustSensor

    INavigationStrategy
          │
    DefaultNavigationStrategy
```

---

## 7. Sequence Diagrams

### UC-02: 정상 Navigation (장애물 없음, 먼지 없음)

```
Timer          RvcController        ISensor(x3+dust)    IMotorController   ICleanerController
  │                  │                    │                    │                   │
  │──onTick()───────>│                    │                    │                   │
  │                  │──detect()─────────>│ (left)             │                   │
  │                  │<──false────────────│                    │                   │
  │                  │──detect()─────────>│ (right)            │                   │
  │                  │<──false────────────│                    │                   │
  │                  │──detect()─────────>│ (dust)             │                   │
  │                  │<──false────────────│                    │                   │
  │                  │                    │                    │                   │
  │                  │──navigate(data)──> [DefaultNavigationStrategy]              │
  │                  │<──FORWARD──────────│                    │                   │
  │                  │                    │                    │                   │
  │                  │────────────────────────move(FORWARD)───>│                   │
  │                  │────────────────────────────────────────────setPower(ON)────>│
```

---

### UC-03: 전방 장애물 회피 (멀티틱, 좌측 개방)

```
FrontSensor    RvcController     ISensor(left)        IMotorController
  │                  │                 │                    │
  │──onFrontObstacleDetected()──────> │                    │
  │                  │─────────────────────move(STOP)──────>│   [state = AVOIDING_OBSTACLE]
Timer──onTick()────>│──detect()──────>│ (left) → false      │
  │                  │   [좌측 개방 → 좌회전]                │
  │                  │─────────────────────move(LEFT)──────>│   [state = CLEANING]
Timer──onTick()────>│─────────────────────move(FORWARD)───>│   [전진 재개]
```

---

### UC-04: 포위 상태 탈출 (멀티틱, 전방 센서로 우측 엿보기)

```
FrontSensor    RvcController     ISensor(left/front)  IMotorController
  │                  │                 │                    │
  │──onFrontObstacleDetected()──────> │                    │
  │                  │─────────────────────move(STOP)──────>│   [state = AVOIDING_OBSTACLE]
Timer──onTick()────>│──detect()──────>│ (left) → true       │
  │                  │   [좌측 차단 → 우측 엿보기]           │
  │                  │─────────────────────move(RIGHT)─────>│   [state = CHECKING_RIGHT]
Timer──onTick()────>│──detect()──────>│ (전방 = 원래 우측) → true
  │                  │   [우측 차단 → 원래 방향 복귀]        │
  │                  │─────────────────────move(LEFT)──────>│   [state = ESCAPING]
Timer──onTick()────>│─────────────────────move(BACKWARD)──>│   [한 칸 후진 → AVOIDING_OBSTACLE]
  │                  │   [다음 틱에 좌우 재평가]             │
```

---

### UC-05: 청소 강도 높이기

```
Timer          RvcController        DustSensor          ICleanerController
  │                  │                    │                    │
  │──onTick()───────>│──detect()─────────>│                    │
  │                  │<──true─────────────│                    │
  │                  │   [state = INTENSIFYING]                │
  │                  │────────────────────────setPower(POWER_UP)>│
  │                  │   [_intensify_ticks = INTENSIFY_DURATION]│
  │                  │                    │                    │
  │   ... tick 경과 ...                   │                    │
  │                  │   [_intensify_ticks가 0에 도달]         │
  │                  │────────────────────────setPower(ON)─────>│
  │                  │   [state = CLEANING]                    │
```

---

## 8. State Machine: RvcController (멀티틱)

인터럽트는 STOP 발행 + AVOIDING_OBSTACLE 진입만 한다. 이후 회피·탈출은
`onTick()`마다 한 단계씩 진행한다(AD-12). 틱당 이동 명령은 하나.

```
         start()
  [IDLE] ───────────────────────────────────────> [CLEANING]
                                                       │  ▲
            onFrontObstacleDetected() (STOP)           │  │ onTick: 전진
  [CLEANING] ────────────────────────> [AVOIDING_OBSTACLE] │
                                                       │     │
              onTick: 좌측 개방   → 좌회전 ───────────┼─────┤
              onTick: 좌측 차단   → 우회전             │     │
                                                       ▼     │
                                             [CHECKING_RIGHT]│
                  onTick: 우측 개방 → 전진 재개 ──────┼─────┤
                  onTick: 우측 차단 → 원래 방향 복귀   │     │
                                                       ▼     │
                                                  [ESCAPING] │
                  onTick: 한 칸 후진 → 재평가         │     │
                                                       ▼     │
                                       (다시 AVOIDING_OBSTACLE)

  [CLEANING] ──── 먼지 감지 ─────────────────────> [INTENSIFYING]
  [INTENSIFYING] ─ 타이머 만료 ──────────────────> [CLEANING]

  모든 상태 ──── stop() ─────────────────────────> [IDLE]
```
