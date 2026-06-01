# Supplementary Specification

## SRS Change Trace - 2026-05-29

### [추가]
- Front Sensor 기반 Right Scan의 정확성 요구사항을 추가한다.
- ESCAPING 중 tick당 하나의 이동 명령만 적용해야 한다는 simulator 요구사항을 추가한다.

### [삭제]
- 전용 Right Sensor 하드웨어 의존성을 활성 설계에서 삭제한다.

### [변경]
- 포위 상태 판단 기준을 Front, Left, Right Sensor에서 Front, Left, Right Scan으로 변경한다.

---

## 1. 품질 속성

| 속성 | 요구사항 |
|---|---|
| Testability | controller는 mock sensor와 mock actuator로 독립 테스트 가능해야 한다. |
| Modifiability | navigation strategy는 controller 수정 없이 교체 가능해야 한다. |
| Portability | core logic과 tests는 Windows와 Linux 환경에서 빌드 가능해야 한다. |
| Traceability | 요구사항 변경은 문서, 코드, 테스트에 추적 표기를 남겨야 한다. |

---

## 2. 성능과 동작 제약

- Front Sensor interrupt는 전방 장애물에 즉시 반응해야 한다.
- tick 기반 동작은 한 tick에 하나의 실제 이동만 반영해야 한다.
- ESCAPING은 여러 tick에 나누어 진행되어야 하며, 단일 이벤트에서 여러 칸 이동하면 안 된다.

---

## 3. 설계 제약

- controller는 concrete HAL class가 아니라 interface에 의존한다.
- RightSensor 파일은 legacy로 남길 수 있으나 active build target에서는 제외한다.
- 오른쪽 감지는 Right Scan 결과로 표현하고, `SensorData::is_right_blocked`는 이 결과를 담는다.

---

## 4. 테스트 제약

- `RvcControllerTest`는 Right Scan 시 `RIGHT`, front detect, `LEFT` 복구 순서를 검증해야 한다.
- `SimulatorTest`는 ESCAPING 중 tick마다 한 칸 이하로만 위치가 변하는지 검증해야 한다.
- 기존 start, stop, dust, normal tick 동작은 유지되어야 한다.
