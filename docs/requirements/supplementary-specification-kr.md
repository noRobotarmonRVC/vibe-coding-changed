# Supplementary Specification

## 개정 이력

| 버전 | 날짜 | 변경 내용 |
|---|---|---|
| 1.0 | 2026-05-21 | 최초 작성 |
| 1.1 | 2026-05-29 | FUNC-02·PERF-01의 주기적 폴링 목록에서 우측 센서 제거 (우측은 전방 센서로 탐지 — AD-11) |

---

이 문서는 Use-Case Model에서 표현되지 않은 시스템 요구사항을 수록한다. FURPS+ 분류 체계를 따른다.

---

## 1. Functionality (기능)

기능 요구사항은 Use-Case Model(`use-case-model.md`)에서 완전히 다룬다. 이 섹션은 특정 Use Case에 귀속되지 않는 기능적 제약을 기록한다.

| ID | 요구사항 |
|---|---|
| FUNC-01 | 시스템은 Front Sensor 입력을 interrupt로 처리해야 한다 (polling 불가). 전방 장애물에 즉각 응답을 보장한다. |
| FUNC-02 | Left, Dust Sensor 입력은 매 Timer Tick마다 평가된다 (주기적 polling). 우측은 전용 센서가 없으며, 회피 중 우회전 후 전방 센서로 확인한다 (AD-11). |
| FUNC-03 | Motor direction 명령은 상호 배타적이다; 한 번에 하나의 방향만 활성화된다. |
| FUNC-04 | Cleaner 전력 상태는 상호 배타적이다: Off, On, 또는 Power Up. |

---

## 2. Usability (사용성)

해당 없음. RVC Control SW는 직접적인 사용자 인터페이스가 없으며, 사용자 상호작용은 시작/정지 명령(UC-01, UC-06)으로 제한된다.

---

## 3. Reliability (신뢰성)

| ID | 요구사항 |
|---|---|
| REL-01 | 시스템은 하나의 처리 사이클 내에 Front Sensor interrupt에 응답해야 한다. |
| REL-02 | 시스템은 충돌하는 motor 명령(예: Forward와 Backward 동시)을 발행해서는 안 된다. |
| REL-03 | 센서 상태를 결정할 수 없으면, 시스템은 safe state(Motor: Stop, Cleaner: Off)로 기본 설정해야 한다. |

---

## 4. Performance (성능)

| ID | 요구사항 |
|---|---|
| PERF-01 | 주기적 센서 평가(Left, Dust)는 단일 Tick 간격 내에 완료되어야 한다. |
| PERF-02 | 강화 청소 지속 시간(UC-05)은 인라인 하드코딩이 아닌 시스템 상수로 설정 가능해야 한다. |

---

## 5. Supportability (지원성)

| ID | 요구사항 |
|---|---|
| SUPP-01 | 설계는 기존 센서 처리 로직을 수정하지 않고도 추가 센서 타입을 통합할 수 있어야 한다 (Open/Closed Principle). |
| SUPP-02 | Navigation 로직은 향후 navigation 알고리즘 교체(예: ML 기반)를 위해 센서 읽기 로직과 분리되어야 한다. |
| SUPP-03 | 시스템은 핵심 제어 로직 변경 없이 향후 모바일 앱 통신을 위한 정의된 인터페이스 경계를 노출해야 한다. |

---

## 6. Design Constraints (설계 제약, +)

| ID | 제약 |
|---|---|
| DC-01 | 구현 언어: C++17. |
| DC-02 | 외부 라이브러리나 패키지를 도입할 수 없다. |
| DC-03 | 모든 모듈은 대응하는 Google Test 단위 테스트를 가져야 한다. |
| DC-04 | 코드는 프로젝트 정의 규칙 세트로 clang-tidy 정적 분석을 통과해야 한다. |
| DC-05 | HW 수준 제어(전기 신호, 레지스터 접근)는 범위 밖이며, SW는 추상화된 I/O 이벤트에서만 동작한다. |
