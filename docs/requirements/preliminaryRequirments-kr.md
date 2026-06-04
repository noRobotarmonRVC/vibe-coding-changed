# Preliminary Requirements

## SRS Change Trace - 2026-06-04

### [추가]
- front interrupt는 정상 주행 중에만 처리한다는 기능 요구사항(FR-08)을 추가한다. 회피 시퀀스 중에는 Front Sensor가 right scan에 재사용되므로 interrupt를 억제하여 거짓 발화가 multi-tick 후진 탈출을 끊지 않게 한다(failure F-10 참조).

## SRS Change Trace - 2026-05-29

### [추가]
- `Right Sensor Input` 대신 Front Sensor scan 기반 `Right Scan Result`를 추가한다.
- ESCAPING을 여러 tick에 나누어 실행하는 요구사항을 추가한다.

### [삭제]
- 전용 `Right Sensor Input`을 활성 시스템 입력에서 삭제한다.
- 한 번의 장애물 이벤트 안에서 후진, 회전, 전진을 모두 완료한다는 가정을 삭제한다.

### [변경]
- 오른쪽 장애물 판단을 전용 sensor polling에서 Front Sensor를 이용한 회전 scan으로 변경한다.
- 후진도 전진과 동일하게 한 tick에 한 칸만 이동하도록 변경한다.

---

## 1. 목적

이 문서는 RVC Control SW의 초기 요구사항을 기능 요구사항과 비기능 요구사항으로 정리한다.

---

## 2. 시스템 입력

| 입력 | 방식 | 설명 |
|---|---|---|
| Front Sensor | Interrupt | 전방 장애물을 즉시 알린다. |
| Left Sensor | Periodic polling | 왼쪽 장애물 상태를 tick 기반으로 읽는다. |
| Right Scan Result | Event-driven scan | 전방 장애물 처리 중 오른쪽으로 회전한 뒤 Front Sensor로 오른쪽 장애물을 확인한다. |
| Dust Sensor | Periodic polling | 현재 위치의 먼지 상태를 확인한다. |
| Timer Tick | Periodic event | controller의 주기 동작을 발생시킨다. |
| User Command | Command | 청소 시작과 중지를 요청한다. |

---

## 3. 시스템 출력

| 출력 | 설명 |
|---|---|
| Motor Direction | `FORWARD`, `BACKWARD`, `LEFT`, `RIGHT`, `STOP` 중 하나를 명령한다. |
| Cleaner Power | `OFF`, `ON`, `POWER_UP` 중 하나를 명령한다. |

---

## 4. 기능 요구사항

| ID | 요구사항 |
|---|---|
| FR-01 | 사용자가 청소 시작을 요청하면 cleaner를 켜고 전진한다. |
| FR-02 | 사용자가 청소 중지를 요청하면 motor를 정지하고 cleaner를 끈다. |
| FR-03 | 전방 장애물이 감지되면 즉시 정지한다. |
| FR-04 | 전방 장애물 처리 중 왼쪽 상태와 Right Scan 결과를 이용해 회피 방향을 결정한다. |
| FR-05 | 왼쪽과 오른쪽이 모두 막힌 경우 ESCAPING 상태로 진입한다. |
| FR-06 | ESCAPING은 tick마다 하나의 이동 명령만 실행한다. |
| FR-07 | 먼지가 감지되면 일정 tick 동안 cleaner power를 높인 뒤 일반 청소로 복귀한다. |
| FR-08 | front interrupt는 정상 주행(`CLEANING`/`INTENSIFYING`) 중에만 처리하고, 회피 시퀀스(`AVOIDING_OBSTACLE`, `CHECKING_RIGHT`, `ESCAPING`) 중에는 억제한다(F-10 참조). [추가] |

---

## 5. 비기능 요구사항

| ID | 요구사항 |
|---|---|
| NFR-01 | navigation 정책은 controller 수정 없이 교체 가능해야 한다. |
| NFR-02 | core control logic은 unit test로 검증 가능해야 한다. |
| NFR-03 | simulator는 한 tick에 실제 위치 이동을 한 칸 이하로 반영해야 한다. |
| NFR-04 | 변경된 요구사항은 문서, 코드, 테스트에서 `[추가]`, `[삭제]`, `[변경]`으로 추적 가능해야 한다. |
