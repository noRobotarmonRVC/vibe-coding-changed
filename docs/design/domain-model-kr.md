# Domain Model

## 개정 이력

| 버전 | 날짜 | 변경 내용 |
|---|---|---|
| 1.0 | 2026-05-21 | 최초 작성 |
| 1.1 | 2026-05-29 | RightSensor 제거(개념 클래스·속성·연관·일반화); Obstacle·Surrounded State 갱신; 우측은 FrontSensor를 회전·재활용해 감지 (AD-11) |

---

Domain Model은 RVC 문제 도메인의 핵심 개념 클래스, 속성, 관계를 식별한다. 이것들은 소프트웨어 클래스가 아니라 실세계 개념을 표현한다.

---

## 1. 개념 클래스

| 클래스 | 설명 |
|---|---|
| **RVC** | Robot Vacuum Cleaner. 탐색하고 청소하는 중심 엔터티. |
| **CleaningSession** | 시작부터 종료까지의 단일 실행. 지속 시간과 상태를 추적. |
| **Sensor** | RVC에서 환경 상태를 감지하는 입력 장치의 추상 개념. |
| **FrontSensor** | 정면의 장애물을 감지하는 interrupt 방식 센서. 우회전 후 우측 확인에도 재활용 (AD-11). |
| **LeftSensor** | 좌측 장애물을 주기적으로 감지하는 센서. |
| **DustSensor** | 바닥 표면의 먼지를 주기적으로 감지하는 센서. |
| **Obstacle** | RVC의 경로를 막는 물리적 물체. Front, Left 센서가 감지(우측은 전방 센서로 확인). |
| **Dust** | 바닥 표면의 미립자. DustSensor가 감지. |
| **Motor** | RVC의 물리적 이동을 구동. Direction 명령을 수신. |
| **Cleaner** | 청소기/걸레 메커니즘. 전력 레벨 명령을 수신. |
| **Timer** | 제어 루프를 구동하는 주기적 Tick 신호를 생성하는 디지털 클록. |
| **DirectionCommand** | Motor에 대한 명령: Forward, Backward, Left(회전), Right(회전), Stop. |
| **CleaningCommand** | Cleaner에 대한 명령: Off, On, Power Up. |

---

## 2. 속성

| 클래스 | 속성 |
|---|---|
| CleaningSession | state {Idle, Active, Stopped} |
| FrontSensor | is_triggered : bool |
| LeftSensor | is_blocked : bool |
| DustSensor | has_dust : bool |
| DirectionCommand | value {Forward, Backward, Left, Right, Stop} |
| CleaningCommand | value {Off, On, PowerUp} |
| Timer | tick_interval : duration |

---

## 3. 연관 관계

```
RVC ──────────────── conducts ────────────────> CleaningSession
RVC ──────────────── has ──────────────────1──> FrontSensor
RVC ──────────────── has ──────────────────1──> LeftSensor
RVC ──────────────── has ──────────────────1──> DustSensor
RVC ──────────────── driven by ────────────1──> Timer
RVC ──────────────── commands ─────────────1──> Motor          (via DirectionCommand)
RVC ──────────────── commands ─────────────1──> Cleaner        (via CleaningCommand)

FrontSensor ──────── detects ──────────────*──> Obstacle       (interrupt; 회전 후 우측도)
LeftSensor ───────── detects ──────────────*──> Obstacle       (periodic)
DustSensor ───────── detects ──────────────*──> Dust           (periodic)

FrontSensor ─────────────────┐
LeftSensor ──────────────────┤──── generalize ──────────────── Sensor
DustSensor ──────────────────┘
```

---

## 4. 핵심 도메인 규칙

- CleaningSession은 Motor와 Cleaner가 모두 동작 중일 때만 Active 상태다.
- RVC는 언제든 정확히 하나의 DirectionCommand와 하나의 CleaningCommand를 발행한다.
- FrontSensor는 비동기적으로 (interrupt) 트리거되며, 나머지 모든 센서는 Tick마다 동기적으로 읽힌다.
- 전방·좌측이 막히고 (전방 센서로 확인한) 우측도 막힌 상태를 **Surrounded State**라 하며, 회전 전에 후진이 필요하다.
- 활성 청소 중 먼지 감지 시 CleaningCommand를 일시적으로 PowerUp으로 올린다. navigation은 중단하지 않는다.
