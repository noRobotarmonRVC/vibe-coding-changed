# RVC Control SW

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
│   ├── hal/               # FrontSensor, LeftSensor, DustSensor (RightSensor 미사용 — AD-11)
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
│   FrontSensor  LeftSensor  DustSensor         │
│   (RightSensor 제거 — AD-11)                  │
└──────────────────────────────────────────────┘
```

상위 계층은 하위 계층에 의존하고, 하위 계층은 절대 상위 계층을 참조하지 않습니다. 계층 간 통신은 모두 인터페이스를 통해 이루어집니다.

### 핵심 설계 결정

| 결정 | 내용 |
|---|---|
| AD-01: Strategy 패턴 | 내비게이션 로직을 `INavigationStrategy`로 분리해 ML 알고리즘 등으로 교체 가능 |
| AD-02: 인터럽트 vs 폴링 | `FrontSensor`는 인터럽트 방식(`onInterrupt()`), 좌측·먼지 센서는 매 Tick 폴링 |
| AD-03: 상태 머신 | `RvcController`가 `RvcState` enum으로 상태를 명시적으로 관리 |
| AD-04: 의존성 주입 | 모든 하드웨어 의존성을 생성자 주입으로 전달 → Google Test 목(Mock) 격리 가능 |
| AD-11: 우측 센서 제거 | 우측 센서 제거, 전방 센서를 회전·재활용해 우측 탐지 (2026-05-29) |
| AD-12: 멀티틱 회피/탈출 | 회피·탈출을 한 틱에 원자 처리하지 않고 `onTick()`마다 한 단계씩 (후진 1틱 1칸) (2026-05-29) |

### 상태 머신 (`RvcState`) — 멀티틱

인터럽트는 STOP + `AVOIDING_OBSTACLE` 진입만 하고, 회피·탈출은 `onTick()`마다
한 단계씩 진행합니다(AD-12). 우측 센서가 없어 좌측이 막히면 우회전 후 전방 센서로
우측을 확인합니다(AD-11).

```
                         dust 감지
              ┌─────────────────────────► INTENSIFYING
              │                               │
              │                        Tick 경과 후 복귀
              │                               │
IDLE ─startCleaning()─► CLEANING ◄───────────┘
                            │  ▲
          front obstacle    │  │ 전진 재개
          (STOP)            ▼  │
                     AVOIDING_OBSTACLE ──(좌측 개방)─ 좌회전 ─┐
                            │                                 │
                    (좌측 차단) 우회전                         │
                            ▼                                 │
                     CHECKING_RIGHT ──(우측 개방)─ 전진 재개 ─┤
                            │                                 │
                    (우측 차단) 원래 방향 복귀                  │
                            ▼                                 │
                       ESCAPING ──후진 1칸→ 재평가(AVOIDING) ─┘
```

---

## CI 파이프라인

GitHub Actions에서 Push마다 자동 실행됩니다.

1. CMake 빌드
2. Google Test 실행
3. clang-tidy 정적 분석
4. gcovr 코드 커버리지 리포트 생성
