# RVC Control SW

## Change Trace - 2026-06-04

### [추가]
- Added map-based dead-end escape regression tests in `SimulatorTest` (left-gap, right-gap/L-shape, full-corridor backup, backward-chain integrity).
- Made `RvcController::onFrontObstacleDetected()` return a `bool` so the controller owns the interrupt acceptance policy (accept while cruising, ignore during the avoidance sequence). No `state()` getter is added — stays compliant with AD-05 / F-02.

### [변경]
- Constrained the Front Sensor interrupt to be accepted only while cruising (`CLEANING`/`INTENSIFYING`). During obstacle handling the Front Sensor is reused for the right scan, so a rotation was raising a false interrupt that hijacked `CHECKING_RIGHT` and broke the backward escape — trapping the RVC in dead-end corridors even when an exit existed (failure F-10). The controller now rejects the interrupt during the avoidance sequence by returning `false`, and the Simulator falls back to `onTick()`.

### 범위 밖
- Fully sealed regions (no exit) have no termination condition by design; see F-10 "Out of Scope".

---

## Change Trace - 2026-05-29

### [추가]
- Added Right Scan as the replacement right-side detection mechanism.
- Added tick-by-tick ESCAPING behavior for backward movement.

### [삭제]
- Removed dedicated RightSensor from the active control path.

### [변경]
- Updated requirements, design, code, simulator, and tests to follow the same Right Scan / tick escape behavior.

OOAD / Unified Process(UP) 방법론으로 개발한 로봇 청소기(RVC) 제어 소프트웨어입니다.

---

## 요구 환경

| 항목 | 버전 |
|---|---|
| OS | Ubuntu 24.04 (WSL2 포함) |
| 컴파일러 | GCC 또는 Clang (C++17 이상) |
| 빌드 시스템 | CMake 3.14 이상 |
| 테스트 프레임워크 | Google Test (CMake FetchContent로 자동 설치) |

---

## 빌드 및 실행

```bash
# 1. 빌드 디렉터리 생성 및 CMake 구성
cmake -S src -B build

# 2. 빌드
cmake --build build

# 3. 메인 프로그램 실행
./build/rvc_main

# 4. 전체 테스트 실행
cd build && ctest --output-on-failure

# 5. 특정 테스트 바이너리 직접 실행
./build/rvc_tests
```

> `build/` 디렉터리는 `.gitignore`에 포함되어 있으므로 커밋되지 않습니다.

---

## 디렉터리 구조

```
OOADProject/
├── src/
│   ├── interfaces/        # ISensor, IMotorController, ICleanerController, INavigationStrategy
│   ├── domain/            # DefaultNavigationStrategy, SensorData, Position, Heading, enum 정의
│   ├── hal/               # FrontSensor, LeftSensor, DustSensor
│   ├── app/               # RvcController, main.cpp
│   ├── simulator/         # Simulator, SimulatedMotor, SimulatedCleaner, SimulatedSensor
│   └── ui/                # ConsoleDisplay, GridDisplay
├── test/
│   ├── app/               # RvcControllerTest
│   ├── domain/            # DefaultNavigationStrategyTest
│   └── simulator/         # SimulatorTest (통합 테스트)
├── docs/
│   ├── requirements/      # Vision, Use-Case Model, System Operation Interface,
│   │                      # Supplementary Specification, Glossary
│   ├── design/            # Domain Model, Design Model, SW Architecture
│   ├── decisions/         # Architecture Decision Records (ADR)
│   └── failures/          # 실패 사례 및 해결 기록
├── .gitignore
├── CLAUDE.md
└── README.md
```

---

## 코드 아키텍처

### 4계층 구조

```
┌──────────────────────────────────────────────┐
│             Application Layer                 │
│   RvcController  (상태 머신, 오케스트레이션)   │
├──────────────────────────────────────────────┤
│              Domain Layer                     │
│   DefaultNavigationStrategy  SensorData       │
│   Position · Heading                          │
│   RvcState · Direction · CleanPower (enum)    │
├──────────────────────────────────────────────┤
│             Interface Layer                   │
│   ISensor  IMotorController  ICleanerController│
│   INavigationStrategy                         │
├──────────────────────────────────────────────┤
│      Hardware Abstraction Layer (HAL)         │
│   FrontSensor  LeftSensor                     │
│   DustSensor                                  │
└──────────────────────────────────────────────┘
```

상위 계층은 하위 계층에 의존하고, 하위 계층은 절대 상위 계층을 참조하지 않습니다. 계층 간 통신은 모두 인터페이스를 통해 이루어집니다.

### 핵심 설계 결정

| 결정 | 내용 |
|---|---|
| AD-01: Strategy 패턴 | 내비게이션 로직을 `INavigationStrategy`로 분리해 ML 알고리즘 등으로 교체 가능 |
| AD-02: 인터럽트 vs 폴링 | `FrontSensor`는 인터럽트 방식(`onInterrupt()`), 나머지 센서는 매 Tick 폴링 |
| AD-03: 상태 머신 | `RvcController`가 `RvcState` enum으로 상태를 명시적으로 관리 |
| AD-04: 의존성 주입 | 모든 하드웨어 의존성을 생성자 주입으로 전달 → Google Test 목(Mock) 격리 가능 |

### 상태 머신 (`RvcState`)

```
                         dust 감지
              ┌─────────────────────────► INTENSIFYING
              │                               │
              │                        Tick 경과 후 복귀
              │                               │
IDLE ─startCleaning()─► CLEANING ◄───────────┘
                            │  ▲
               front obstacle│  │열린 방향 확보 후 복귀
                            ▼  │
                     AVOIDING_OBSTACLE
                            │
                    좌우 모두 차단
                            ▼
                         ESCAPING ──후진 후 전진──► CLEANING
```

---

## CI 파이프라인

GitHub Actions에서 Push마다 자동 실행됩니다.

1. CMake 빌드
2. Google Test 실행
3. clang-tidy 정적 분석
4. gcovr 코드 커버리지 리포트 생성
