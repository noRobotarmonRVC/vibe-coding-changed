# Use-Case Model

## SRS Change Trace - 2026-05-29

### [추가]
- UC-03, UC-04에서 사용하는 내부 동작 `Right Scan`을 추가한다.
- UC-04에 tick 단위 escape 진행을 추가한다.

### [삭제]
- 외부 actor로서의 `Right Sensor`를 삭제한다.
- UC-04의 단일 이벤트 escape 완료 가정을 삭제한다.

### [변경]
- 오른쪽 장애물 감지를 Right Sensor polling에서 Front Sensor right scan으로 변경한다.
- UC-04의 escape가 `BACKWARD` 후 side evaluation으로 돌아가도록 변경한다.

---

## 1. Actor

| Actor | 유형 | 설명 |
|---|---|---|
| User | Primary | 청소 세션을 시작하거나 종료한다. |
| Front Sensor | External System | 전방 장애물을 interrupt 방식으로 알린다. |
| Left Sensor | External System | 왼쪽 장애물 상태를 제공한다. |
| Right Scan | Internal Behavior | 오른쪽으로 회전한 뒤 Front Sensor로 오른쪽 장애물을 확인한다. |
| Dust Sensor | External System | 바닥 먼지 감지 결과를 제공한다. |
| Timer | External System | 주기적인 tick을 발생시킨다. |

---

## 2. Use Case 개요

```text
User          -> UC-01 Start Cleaning
User          -> UC-06 Stop Cleaning
Timer         -> UC-02 Navigate and Clean
Front Sensor  -> UC-03 Avoid Front Obstacle
Left Sensor   -> UC-04 Escape Surrounded State
Right Scan    -> UC-04 Escape Surrounded State
Dust Sensor   -> UC-05 Intensify Cleaning
```

---

## 3. Use Cases

### UC-01: 청소 세션 시작

| 항목 | 내용 |
|---|---|
| Primary Actor | User |
| 사전 조건 | RVC가 idle 상태이다. |
| 기본 흐름 | User가 start를 요청하면 cleaner를 `ON`으로 설정하고 motor를 `FORWARD`로 명령한다. |
| 사후 조건 | RVC가 cleaning 상태로 진입한다. |

### UC-02: 탐색 및 청소

| 항목 | 내용 |
|---|---|
| Primary Actor | Timer |
| 사전 조건 | 청소 세션이 활성 상태이다. |
| 기본 흐름 | 매 tick마다 dust 상태를 확인하고, 필요한 경우 cleaner power를 조절하며, 기본적으로 전진 명령을 유지한다. |
| 확장 | 전방 장애물이 감지되면 UC-03으로 확장된다. 먼지가 감지되면 UC-05로 확장된다. |

### UC-03: 전방 장애물 회피

| 항목 | 내용 |
|---|---|
| Primary Actor | Front Sensor |
| 사전 조건 | RVC가 cleaning 중이고 전방 장애물이 감지되었다. |
| 기본 흐름 | 전방 장애물 interrupt에서는 즉시 정지만 수행한다. 이후 tick에서 Left Sensor를 확인하고, left가 막혔으면 오른쪽으로 회전해 `CHECKING_RIGHT` 상태에서 Front Sensor로 기존 오른쪽 방향을 확인한다. |
| 사후 조건 | 회피 명령 후 전진 청소로 복귀한다. |

### UC-04: 포위 상태 탈출

| 항목 | 내용 |
|---|---|
| Primary Actor | Front Sensor, Left Sensor, Right Scan |
| 사전 조건 | Front, Left, Right Scan 결과가 모두 blocked이다. |
| 기본 흐름 | 장애물 감지 interrupt에서는 정지만 수행한다. 이후 tick에서 left 확인, right probe, heading 복구를 진행하고, 포위 상태이면 `BACKWARD` 한 칸 후 다시 side evaluation으로 돌아간다. |
| 사후 조건 | RVC가 cleaning 상태로 복귀한다. |

### UC-05: 청소 강도 높이기

| 항목 | 내용 |
|---|---|
| Primary Actor | Dust Sensor |
| 사전 조건 | 청소 중 먼지가 감지되었다. |
| 기본 흐름 | cleaner power를 `POWER_UP`으로 설정하고 지정 tick 이후 `ON`으로 복구한다. |

### UC-06: 청소 세션 종료

| 항목 | 내용 |
|---|---|
| Primary Actor | User |
| 기본 흐름 | User가 stop을 요청하면 motor를 `STOP`으로, cleaner를 `OFF`로 설정한다. |
