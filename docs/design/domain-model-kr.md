# Domain Model

## Design Change Trace - 2026-05-29

### [추가]
- `Right Scan`을 오른쪽 장애물 감지 도메인 개념으로 추가한다.

### [삭제]
- 현재 도메인 모델의 활성 참여자에서 전용 `RightSensor`를 삭제한다.

### [변경]
- `Surrounded State` 조건을 `Front Sensor`, `Left Sensor`, `Right Scan` 기준으로 변경한다.

---

## 1. 핵심 개념

| 개념 | 설명 |
|---|---|
| RVC | 자율 주행하며 바닥을 청소하는 로봇 청소기이다. |
| Cleaning Session | 사용자의 start 명령부터 stop 명령까지 이어지는 청소 실행 단위이다. |
| Sensor | 주변 환경 또는 바닥 상태를 감지하는 입력 장치의 추상 개념이다. |
| Front Sensor | 전방 장애물을 감지한다. Right Scan 중에는 오른쪽 방향의 장애물 확인에도 사용된다. |
| Left Sensor | 왼쪽 장애물 상태를 감지한다. |
| Right Scan | 오른쪽으로 회전한 뒤 Front Sensor를 사용해 오른쪽 장애물 상태를 판단하는 내부 동작이다. |
| Dust Sensor | 먼지 상태를 감지한다. |
| Motor | RVC의 이동과 회전을 수행한다. |
| Cleaner | 청소 장치의 전원과 강도를 제어한다. |
| Timer Tick | 주기적인 control cycle을 발생시키는 시간 이벤트이다. |

---

## 2. 상태 개념

| 상태 | 의미 |
|---|---|
| Idle | 청소하지 않는 대기 상태이다. |
| Cleaning | 기본 주행과 청소를 수행하는 상태이다. |
| Intensifying | 먼지 감지 후 일시적으로 청소 강도를 높인 상태이다. |
| Avoiding Obstacle | 전방 장애물을 회피하기 위해 방향을 결정하는 일시 상태이다. |
| Escaping | 전방, 왼쪽, 오른쪽이 모두 막힌 상태에서 탈출을 진행하는 상태이다. |

---

## 3. 주요 관계

- RVC는 Cleaning Session 동안 Timer Tick과 sensor event를 처리한다.
- Control SW는 SensorData를 구성하고 Navigation Strategy에 전달한다.
- Navigation Strategy는 SensorData를 바탕으로 Direction을 결정한다.
- Motor와 Cleaner는 Control SW가 발행한 명령을 수행한다.

---

## 4. 포위 상태 정의

포위 상태는 다음 조건이 모두 참일 때 발생한다.

```text
Front Sensor = blocked
Left Sensor = blocked
Right Scan = blocked
```

이 상태에서는 한 이벤트 안에서 모든 탈출 동작을 끝내지 않는다. `ESCAPING` tick은 `BACKWARD` 한 번만 실행하고, 이후 `AVOIDING_OBSTACLE`로 돌아가 좌측 확인과 오른쪽 probe를 다시 수행한다.
