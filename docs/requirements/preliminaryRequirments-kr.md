# RVC Control SW

## 개정 이력

| 버전 | 날짜 | 변경 내용 |
|---|---|---|
| 1.0 | 2026-05-21 | 최초 작성 |
| 1.1 | 2026-05-29 | DFD Level 0과 입출력 이벤트 표에서 Right Sensor Input 삭제 (우측은 전방 센서로 탐지 — AD-11) |

---

## RVC SW Controller 예비 요구사항
- RVC는 가정 내 표면을 자동으로 청소하고 걸레질한다.
- 청소하는 동안 직진한다.
- 센서가 장애물을 감지하면 청소를 멈추고 좌측 또는 우측으로 방향을 틀어 청소하면서 전진한다.
- 전방, 좌측, 우측 모두에 장애물이 있으면 후진한 뒤 좌측 또는 우측으로 방향을 틀어 전진한다.
- 먼지를 감지하면 잠시 동안 청소 전력을 높인다 (Power Up).
- HW 제어의 세부 설계 및 구현은 고려하지 않는다.
- 자동 청소 기능에만 집중한다.

## 미래/확장 고려 요구사항
- RVC에 센서를 추가하거나 변경할 수 있다.
- 한 지점을 잠시 동안 순환 청소할 수 있어야 한다.
- 모바일 앱과 통신할 수 있어야 한다.
- 더 효율적인 청소를 위해 머신 러닝과 추론을 수행할 수 있다.

# SASD의 DFD Level 0

- `<-`는 입력 방향을 의미한다.
- `->`는 출력 방향을 의미한다.
```
RVC Control SW
<- Front Sensor Input - Front Sensor
<- Left Sensor Input  - Left Sensor
<- Dust Sensor Input  - Dust Sensor
<- Tick               - Digital Clock
-> Direction          - Motor
-> Clean              - Cleaner
```

| Input/Output Event | 설명                                                                                               | 형식 / 타입                       |
| ------------------ | -------------------------------------------------------------------------------------------------- | --------------------------------- |
| Front Sensor Input | RVC 전방의 장애물 감지                                                                             | True / False, interrupt           |
| Left Sensor Input  | RVC 좌측의 장애물을 주기적으로 감지                                                                | True / False, Periodic            |
| Dust Sensor Input  | 바닥의 먼지를 주기적으로 감지                                                                      | True / False, Periodic            |
| Direction          | Motor에 대한 방향 명령 (전진 / 각도를 가진 좌회전 / 각도를 가진 우회전)                           | Forward / Backward / Left / Right |
| Clean              | 끄기 / 켜기 / Power Up                                                                             | On / Off / Up                     |
