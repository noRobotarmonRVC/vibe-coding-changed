# Vision

## SRS Change Trace - 2026-05-29

### [추가]
- 시스템 경계 안에 `Right Scan`을 오른쪽 장애물 감지 방식으로 추가한다.

### [삭제]
- 이번 iteration의 활성 입력 actor에서 전용 `Right Sensor`를 삭제한다.

### [변경]
- 포위 장애물 탈출 조건을 `Front Sensor`, `Left Sensor`, `Right Scan`이 모두 blocked인 경우로 변경한다.

---

## 1. 소개

이 문서는 RVC(Robot Vacuum Cleaner) Control SW의 목표, 이해관계자, 제품 범위, 주요 기능과 제약을 정의한다. 이후 요구사항, 설계, 구현, 테스트 문서의 기준 문서로 사용한다.

---

## 2. 문제 정의

| 항목 | 내용 |
|---|---|
| 문제 | 가정 내 바닥 청소는 반복적이고 시간이 많이 든다. |
| 영향을 받는 대상 | 가정 사용자 |
| 영향 | 사용자는 수동 청소에 시간과 노력을 반복적으로 사용해야 한다. |
| 성공적인 해결책 | 사람의 개입 없이 주행, 장애물 회피, 먼지 감지, 청소 강도 조절을 수행하는 자율 RVC |

---

## 3. 이해관계자

| 이해관계자 | 역할 | 관심사 |
|---|---|---|
| End User | RVC 사용 | 안정적인 자율 청소, 낮은 개입 필요성 |
| SW Developer | Control SW 구현 | 명확한 요구사항, 테스트 가능한 구조, 변경 가능한 설계 |
| HW Engineer | 센서, 모터, 청소 장치 제공 | HW와 SW 사이의 명확한 I/O 계약 |

---

## 4. 제품 개요

RVC Control SW는 센서 이벤트와 timer tick을 입력으로 받아 motor direction과 cleaner power 명령을 출력하는 embedded control software이다.

시스템은 실제 하드웨어를 직접 제어하지 않고, 추상화된 interface를 통해서만 동작한다. 오른쪽 장애물은 더 이상 전용 Right Sensor로 읽지 않고, RVC가 오른쪽으로 회전한 뒤 Front Sensor로 확인하는 `Right Scan` 방식으로 판단한다.

### 시스템 경계

```text
Front Sensor  ----\
Left Sensor   -----\
Right Scan    ------> RVC Control SW ----> Motor(Direction)
Dust Sensor   -----/                  \--> Cleaner(Power)
Timer Tick    ----/
```

---

## 5. 주요 기능

| ID | 기능 | 설명 |
|---|---|---|
| F-01 | 자율 전진 주행 | 기본 상태에서 전진하며 청소한다. |
| F-02 | 전방 장애물 회피 | 전방 장애물 감지 시 정지하고 좌우 상태를 판단해 회피한다. |
| F-03 | 포위 상태 탈출 | 전방, 왼쪽, 오른쪽이 모두 막힌 경우 후진, 회전, 전진으로 탈출한다. |
| F-04 | 청소 강도 조절 | 먼지 감지 시 일정 시간 cleaner power를 높인다. |

---

## 6. 범위와 제약

- 구현 언어는 C++17이다.
- 하드웨어 장치는 interface 뒤에 숨기며, controller는 구체 HAL에 직접 의존하지 않는다.
- 외부 라이브러리는 프로젝트 규칙에 따라 제한적으로 사용한다.
- 이번 범위는 RVC control logic, simulator, unit/integration test를 포함한다.
- 모바일 앱, 실제 모터 드라이버, ML navigation은 이번 범위에 포함하지 않는다.
