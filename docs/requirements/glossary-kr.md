# Glossary

## SRS Change Trace - 2026-05-29

### [추가]
- `Right Scan`과 `Tick-based Escape` 용어를 추가한다.

### [삭제]
- 활성 용어에서 전용 `Right Sensor`를 삭제한다.

### [변경]
- `is_right_blocked`의 의미를 Right Sensor 감지값이 아니라 Right Scan 결과로 변경한다.

---

## 용어

| 용어 | 정의 |
|---|---|
| RVC | Robot Vacuum Cleaner. 자율 청소 로봇을 의미한다. |
| Control SW | sensor 입력을 해석해 motor와 cleaner 명령을 결정하는 소프트웨어이다. |
| Front Sensor | 전방 장애물을 감지하는 센서이다. interrupt 기반 장애물 이벤트와 Right Scan 중 detect에 사용된다. |
| Left Sensor | 왼쪽 장애물 상태를 제공하는 센서이다. |
| Right Scan | RVC가 오른쪽으로 회전한 뒤 Front Sensor로 오른쪽 장애물을 확인하는 내부 동작이다. |
| Dust Sensor | 바닥 먼지 상태를 감지하는 센서이다. |
| Tick | timer가 주기적으로 발생시키는 제어 단위이다. |
| ESCAPING | 전방, 왼쪽, 오른쪽이 모두 막힌 상태에서 탈출하는 controller 상태이다. |
| Tick-based Escape | `BACKWARD`, `LEFT`, `FORWARD`를 서로 다른 tick에 나누어 실행하는 탈출 방식이다. |
| `is_right_blocked` | Right Scan 결과가 blocked인지 나타내는 `SensorData` 필드이다. |
