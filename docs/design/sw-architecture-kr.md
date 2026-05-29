# SW Architecture Document

## 개정 이력

| 버전 | 날짜 | 변경 내용 |
|---|---|---|
| 1.0 | 2026-05-21 | 최초 작성 |
| 1.1 | 2026-05-29 | HAL 다이어그램에서 RightSensor 제외; AD-02 갱신(폴링할 우측 센서 없음); AD-11(우측 탐지)·AD-12(멀티틱 회피) 추가 |

---

## 1. 개요

RVC Control SW는 엄격한 의존성 규칙을 가진 **계층형 아키텍처**를 따른다: 상위 계층은 하위 계층에 의존하며, 하위 계층은 절대 상위 계층에 의존하지 않는다. 모든 계층 간 통신은 인터페이스를 사용하여 시스템의 테스트 가능성과 확장성을 유지한다.

---

## 2. 아키텍처 계층

```
┌──────────────────────────────────────────────────┐
│              Application Layer                    │
│   RvcController  (오케스트레이션, state machine)  │
├──────────────────────────────────────────────────┤
│               Domain Layer                        │
│   DefaultNavigationStrategy  SensorData           │
│   RvcState (enum)  Direction (enum)               │
├──────────────────────────────────────────────────┤
│             Interface Layer                       │
│   ISensor  IMotorController  ICleanerController   │
│   INavigationStrategy                             │
├──────────────────────────────────────────────────┤
│        Hardware Abstraction Layer (HAL)           │
│   FrontSensor  LeftSensor  DustSensor             │
│   (RightSensor 제거 — AD-11 참조)                 │
│   (Motor/Cleaner adapters)                        │
└──────────────────────────────────────────────────┘
```

| 계층 | 책임 | 변경 시점 |
|---|---|---|
| Application | State 전이 오케스트레이션, 시작/종료 생명주기 | Use-case 로직 변경 시 |
| Domain | Navigation 규칙 및 데이터 구조 캡슐화 | Navigation 알고리즘 또는 센서 의미론 변경 시 |
| Interface | 계층 간 계약 정의 | I/O 계약 변경 시 |
| HAL | 실제 하드웨어를 추상화로 래핑 | 하드웨어 변경 시 (센서 타입, 액추에이터 프로토콜) |

---

## 3. Component Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                       RVC Control SW                         │
│                                                             │
│  ┌─────────────────┐        ┌──────────────────────────┐   │
│  │  RvcController  │◄──────►│  DefaultNavigation        │   │
│  │  (orchestrator) │        │  Strategy                 │   │
│  └────────┬────────┘        └──────────────────────────┘   │
│           │                                                  │
│    ┌──────┴──────────────────────────────────┐              │
│    │           Interface Layer                │              │
│    │  ISensor  IMotorController  ICleaner...  │              │
│    └──────┬──────────────────────────────────┘              │
│           │                                                  │
│    ┌──────┴───────────────────────────────────┐             │
│    │               HAL                         │             │
│    │  FrontSensor LeftSensor DustSensor        │             │
│    │  MotorAdapter CleanerAdapter              │             │
│    └──────────────────────────────────────────┘             │
└─────────────────────────────────────────────────────────────┘
         ▲                               ▼
   [Sensor HW]                  [Motor / Cleaner HW]
```

---

## 4. 주요 Architecture Decisions

### AD-01: Navigation에 Strategy Pattern 적용

**결정:** Navigation 로직을 `INavigationStrategy` / `DefaultNavigationStrategy`로 추출하고, `RvcController`에 주입한다.

**근거:** Supplementary Specification(SUPP-02)은 navigation이 제어와 분리되어 향후 이터레이션에서 ML 기반 알고리즘으로 교체될 수 있도록 요구한다. Strategy Pattern은 `RvcController`를 수정하지 않고 이를 가능하게 한다.

**결과:** 새로운 navigation 알고리즘 추가 시 `INavigationStrategy`를 구현하는 새 클래스만 필요하며, 기존 클래스는 변경되지 않는다.

---

### AD-02: 센서의 Interrupt vs. Polling

**결정:** `FrontSensor`는 interrupt 방식(`onInterrupt()`가 플래그를 설정하고 controller가 `onFrontObstacleDetected()`를 호출); Left, Dust 센서는 매 Tick마다 polling.

> 2026-05-29 갱신(AD-11): 우측 센서가 제거되었다. 우측은 우회전 후 전방 센서를 재활용해 탐지하므로 더 이상 polling할 우측 센서가 없다.

**근거:** FUNC-01은 전방 장애물에 즉각 응답을 요구한다. FUNC-02는 나머지 센서를 주기적으로 정의한다. 두 모델을 혼용하려면 controller가 두 가지 신호 전달 경로를 모두 처리해야 한다.

**결과:** `FrontSensor`는 `detect()`와 별개로 `onInterrupt()` 진입점을 가지며, 시스템 ISR이 이를 호출해야 한다. 나머지 모든 센서는 `detect()`만 사용한다.

---

### AD-03: RvcController의 State Machine

**결정:** `RvcController`는 명시적 `RvcState` enum을 유지하고 전이는 그 안에서 중앙 집중 처리된다.

**근거:** State마다 동작이 크게 다르다(예: Dust 처리와 Escaping이 다름). 명시적 state machine은 분산된 조건부 로직을 방지하고 전이를 감사 가능하게 한다.

**결과:** `RvcController`의 모든 메서드는 `_state` 확인 또는 업데이트로 시작한다. 새로운 동작은 기존 코드에 조건문을 추가하는 것이 아니라 새로운 state를 도입하여 추가된다.

---

### AD-04: 모든 HW에 Dependency Injection 적용

**결정:** `RvcController`는 모든 의존성(`ISensor*`, `IMotorController*`, `ICleanerController*`, `INavigationStrategy*`)을 constructor injection으로 받는다.

**근거:** 완전한 단위 테스트 격리 가능 — Google Test가 모든 HW 의존성을 mock 구현으로 대체한다. SUPP-01(새로운 센서 타입을 `RvcController` 수정 없이 플러그 가능) 충족.

**결과:** `RvcController`는 의존성을 생성하지 않으며, 조합 루트(main 또는 test fixture)가 연결한다.

---

### AD-07: onTick()은 매 사이클마다 FORWARD를 재발행해야 한다

**결정:** `onTick()`은 CLEANING 또는 INTENSIFYING 상태일 때 매 틱 마지막에 `_motor->move(Direction::FORWARD)`를 발행한다.

**근거:** 실제 H-브리지 하드웨어는 명령이 다시 올 때까지 방향을 래치한다. 그러나 `IMotorController` 인터페이스는 이런 보장을 하지 않는다. 각 `move()` 호출은 독립적인 명령이며, Control SW는 구현 세부사항에 의존하지 않고 인터페이스 계약만을 기준으로 동작해야 한다.

**결과:** 직선 주행 중에도 틱마다 모터 명령이 하나씩 추가된다. `IMotorController` 구현 세부사항에 대한 가정 없이 인터페이스 계약을 자급자족하게 만들기 위해 허용한다.

---

### AD-11: 우측 센서 제거 → 전방 센서 재활용으로 우측 탐지 (2026-05-29)

**결정:** HW에서 우측 센서가 제거되었다. 전방+좌측이 모두 막히면 컨트롤러가 우회전하여 전방 센서로 우측을 읽는다.

**결과:** `SensorData.is_right_blocked` 제거, `RvcController`가 우측 센서를 더 이상 보유하지 않는다. 상세는 `docs/decisions/2026-05-29-maintenance-decisions-kr.md` 참조.

---

### AD-12: 회피/탈출의 멀티틱 처리 (2026-05-29)

**결정:** 회피·탈출을 인터럽트에서 원자적으로 처리하지 않고(AD-03의 원자적 처리를 대체) `onTick()`마다 한 단계씩 진행한다. 인터럽트는 STOP만 발행한다.

**결과:** 틱당 이동 명령은 정확히 하나(F-06: 후진이 한 틱에 여러 칸 이동하던 결함 해결). 새 상태 `CHECKING_RIGHT` 추가. 상세는 `docs/decisions/2026-05-29-maintenance-decisions-kr.md` 참조.

---

## 5. 소스 디렉터리 구조

```
src/
├── CMakeLists.txt
├── interfaces/
│   ├── ISensor.hpp
│   ├── IMotorController.hpp
│   ├── ICleanerController.hpp
│   └── INavigationStrategy.hpp
├── domain/
│   ├── SensorData.hpp
│   ├── Direction.hpp          ← Direction enum
│   ├── CleanPower.hpp         ← CleanPower enum
│   ├── RvcState.hpp           ← RvcState enum
│   └── DefaultNavigationStrategy.hpp / .cpp
├── hal/
│   ├── FrontSensor.hpp / .cpp
│   ├── LeftSensor.hpp / .cpp
│   ├── RightSensor.hpp / .cpp   (미사용, 보존 — AD-11)
│   └── DustSensor.hpp / .cpp
└── app/
    ├── RvcController.hpp / .cpp
    └── main.cpp
```

```
test/
├── CMakeLists.txt
├── domain/
│   └── DefaultNavigationStrategyTest.cpp
└── app/
    └── RvcControllerTest.cpp
```

---

## 6. 통합 및 시뮬레이션

**Simulator** 컴포넌트(Control SW와 별개)는 통합 테스트를 위해 HW 환경을 에뮬레이션한다:

- `IMotorController`와 `ICleanerController`를 구현하여 명령을 기록한다.
- 스크립트된 시나리오로 `ISensor` 구현체를 구동한다.
- 각 시나리오에 대해 전체 제어 루프가 올바른 Direction 및 CleanPower 명령 시퀀스를 생성하는지 검증한다.

Simulator는 개별 클래스를 테스트하지 않으며, HW를 소프트웨어 stub으로 대체하여 조립된 시스템을 end-to-end로 테스트한다.

---

## 7. Non-Functional Requirement 추적성

| Req ID | 요구사항 | 아키텍처 솔루션 |
|---|---|---|
| FUNC-01 | Front Sensor interrupt 응답 | AD-02: FrontSensor의 interrupt 경로 |
| FUNC-02 | 주기적 센서 polling | AD-02: `onTick()`에서 `detect()` 호출 |
| REL-02 | 충돌하는 motor 명령 없음 | AD-03: state machine이 두 방향 동시 발행 방지 |
| REL-03 | 불확정 입력 시 safe state | `RvcController::stop()`이 모든 state에서 도달 가능 |
| PERF-01 | Tick 처리가 하나의 간격 내 완료 | `onTick()`에 블로킹 호출 없음; 모든 센서 읽기는 동기 반환 |
| SUPP-01 | 기존 코드 수정 없이 새 센서 타입 추가 | AD-04: 새로운 `ISensor` 구현체 주입 |
| SUPP-02 | 교체 가능한 navigation 알고리즘 | AD-01: `INavigationStrategy` injection |
| DC-03 | Google Test 단위 테스트 | AD-04: DI가 완전한 mock 기반 격리 가능하게 함 |
