# System Operation Interface (한국어)

## 개정 이력

| 버전 | 날짜 | 변경 내용 |
|---|---|---|
| 1.0 | 2026-05-21 | 최초 작성 |
| 1.1 | 2026-05-29 | `tick()`/`onFrontObstacleDetected()`에서 우측 센서 읽기 제거; `onFrontObstacleDetected()`는 STOP + AVOIDING_OBSTACLE 진입만; 결정 로직을 우측 회전 탐지 멀티틱으로 재작성 (AD-11, AD-12) |

---

| «interface» RVCSystem | `startCleaning()` · `tick()` · `onFrontObstacleDetected()` · `stopCleaning()` |
|---|---|

System Operation은 Use-Case 시나리오의 시스템 이벤트에서 도출된다. 각 오퍼레이션은 외부 액터가 RVC 시스템에 보내는 메시지를 나타낸다.

---

## 1. 오퍼레이션 요약

| 오퍼레이션 | 트리거 액터 | 관련 UC |
|---|---|---|
| `startCleaning()` | User | UC-01 |
| `tick()` | Timer | UC-02, UC-03, UC-04, UC-05 |
| `onFrontObstacleDetected()` | Front Sensor | UC-03, UC-04 |
| `stopCleaning()` | User | UC-06 |

---

## 2. 시스템 오퍼레이션 명세

---

### SO-01: `startCleaning()`

| 항목 | 내용 |
|---|---|
| **오퍼레이션** | `startCleaning()` |
| **관련 UC** | UC-01: 청소 세션 시작 |
| **트리거 액터** | User |
| **설명** | RVC를 IDLE 상태에서 CLEANING 상태로 전환하고, Cleaner를 활성화하며, 전진을 시작한다. |
| **사전 조건** | RVC가 IDLE 상태다. |
| **사후 조건** | RVC 상태 = CLEANING. Motor 명령 = FORWARD. Cleaner 명령 = ON. |

**참조:** UC-01 Step 1–4

---

### SO-02: `tick()`

| 항목 | 내용 |
|---|---|
| **오퍼레이션** | `tick()` |
| **관련 UC** | UC-02, UC-03, UC-04, UC-05 |
| **트리거 액터** | Timer |
| **설명** | RVC가 활성 상태일 때 모든 센서 폴링과 navigation 결정을 구동하는 주기적 신호. 매 tick마다 Left, Dust 센서를 읽고 회피/탈출 상태머신을 한 단계 진행한다. 우측은 필요 시 우회전 후 전방 센서로 확인한다. |
| **사전 조건** | RVC가 활성 상태다 (CLEANING, AVOIDING_OBSTACLE, ESCAPING, INTENSIFYING 중 하나). |
| **사후 조건** | 현재 센서 값에 따라 Motor 방향과 Cleaner 상태가 업데이트된다. 아래 표에 따라 상태가 전이될 수 있다. |

**`tick()` 에 의한 상태 전이:**

| 센서 읽기 | 전이 상태 | Motor | Cleaner |
|---|---|---|---|
| 장애물 없음, 먼지 없음 | CLEANING | FORWARD | ON |
| 전방 차단, 좌측 또는 우측(확인) 개방 | AVOIDING_OBSTACLE → CHECKING_RIGHT → CLEANING | 틱당 한 단계 (회전 후 전진) | ON |
| 전방 차단, 좌측 차단, 우측(확인) 차단 | ESCAPING | BACKWARD (틱당 한 칸) 후 재평가 | ON |
| Dust = True | INTENSIFYING | FORWARD | POWER_UP |
| 강화 지속 시간 경과 | CLEANING | FORWARD | ON |

**참조:** UC-02 주요 성공 시나리오 및 대안 흐름

---

### SO-03: `onFrontObstacleDetected()`

| 항목 | 내용 |
|---|---|
| **오퍼레이션** | `onFrontObstacleDetected()` |
| **관련 UC** | UC-03, UC-04 |
| **트리거 액터** | Front Sensor (interrupt 방식) |
| **설명** | 전방 장애물 감지 시 발생하는 interrupt 기반 알림. `tick()`과 달리 비동기적으로 감지 즉시 발생한다. STOP만 발행하고 AVOIDING_OBSTACLE에 진입하며, 실제 회피/탈출은 이후 tick들에서 진행된다 (AD-12). |
| **사전 조건** | RVC가 CLEANING 상태다. |
| **사후 조건** | Motor 명령 = STOP; 상태 = AVOIDING_OBSTACLE. |

**결정 로직 (`onTick()`마다 한 단계 진행):**

| 단계 (상태) | 조건 | 동작 | 다음 상태 |
|---|---|---|---|
| 인터럽트 | 전방 차단 | STOP | AVOIDING_OBSTACLE |
| AVOIDING_OBSTACLE | 좌측 개방 | 좌회전 | CLEANING |
| AVOIDING_OBSTACLE | 좌측 차단 | 우회전(우측 엿보기) | CHECKING_RIGHT |
| CHECKING_RIGHT | 우측 개방 | 전진 재개 | CLEANING |
| CHECKING_RIGHT | 우측 차단 | 원래 방향 복귀 | ESCAPING |
| ESCAPING | — | 한 칸 후진 | AVOIDING_OBSTACLE |

**참조:** UC-03 Step 1–3, UC-03 대안 흐름, UC-04 Step 1

---

### SO-04: `stopCleaning()`

| 항목 | 내용 |
|---|---|
| **오퍼레이션** | `stopCleaning()` |
| **관련 UC** | UC-06: 청소 세션 종료 |
| **트리거 액터** | User |
| **설명** | 활성 청소 세션을 종료하고, Motor를 정지하며, Cleaner를 끈다. |
| **사전 조건** | RVC가 활성 상태다 (CLEANING, AVOIDING_OBSTACLE, ESCAPING, INTENSIFYING 중 하나). |
| **사후 조건** | RVC 상태 = IDLE. Motor 명령 = STOP. Cleaner 명령 = OFF. |

**참조:** UC-06 Step 1–4

---

## 3. 시스템 시퀀스 다이어그램

### 시나리오 A: 정상 청소 (장애물 없음, 먼지 없음)

```
User          RVC System       Timer
 |                |               |
 |--startCleaning()-->            |
 |                |               |
 |                |<----tick()----|
 |                |  [장애물 없음, 먼지 없음]
 |                |  Motor: FORWARD, Cleaner: ON
 |                |               |
 |                |<----tick()----|
 |                |  (반복)        |
 |                |               |
 |--stopCleaning()-->             |
 |                |               |
```

### 시나리오 B: 전방 장애물 회피

```
User       Front Sensor     RVC System       Timer
 |               |               |               |
 |--startCleaning()------------>|               |
 |               |               |<----tick()----|
 |               |               |  Motor: FORWARD
 |               |               |               |
 |               |--onFrontObstacleDetected()--->|
 |               |               |  Motor: STOP (AVOIDING_OBSTACLE 진입)
 |               |               |  [onTick: Left 읽기; 우회전으로 Right 엿보기]
 |               |               |  Motor: TURN → FORWARD (이후 tick들에서)
 |               |               |               |
 |               |               |<----tick()----|
 |               |               |  (navigation 재개)
```

### 시나리오 C: 청소 강도 높이기

```
User          RVC System       Timer        Dust Sensor
 |                |               |               |
 |--startCleaning()-->            |               |
 |                |<----tick()----|               |
 |                |  [Dust Sensor = True]         |
 |                |  Cleaner: POWER_UP            |
 |                |               |               |
 |                |<----tick()----|               |
 |                |  [지속 시간 경과]              |
 |                |  Cleaner: ON (정상)           |
```
