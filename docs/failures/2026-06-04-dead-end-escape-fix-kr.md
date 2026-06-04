# 막다른 통로 탈출 실패와 해결 — 2026-06-04

통합된 multi-tick 탈출을 막다른 맵에서 검증하던 중 발견한 제어 로직 실패를
기록한다. 2026-05-21 Simulator/UI failures와는 별개다.

---

## F-10: Front Interrupt가 Right Scan을 가로채 후진 탈출을 끊음

### 문제
RVC가 막다른 통로에 들어가면 **출구가 있어도 빠져나가지 못하고** 두 칸 사이를
영원히 진동했다. 맵 `.. < / X X .` (아래 한 칸만 트인 통로)에서 RVC가 무한히
갇혔고, 그 상태 천이가 진짜 밀폐된 맵 `.. < / X X X`와 **완전히 동일**했다 —
controller가 "출구 있음"과 "출구 없음"을 구분조차 못 했다.

### 원인
Front Sensor가 같은 측정값을 두 용도로 공유한다:

1. 정상 주행 중 — **전방 장애물 interrupt** (rising edge: 열림 → 막힘)
2. 회피 중 — **Right Scan**: 우회전 후 `FrontSensor::detect()`로 원래 오른쪽을
   probe (`CHECKING_RIGHT`)

Right Scan을 위해 우회전하면 새 정면이 벽인 경우가 많아 front 측정값에 rising
edge가 생긴다. Simulator는 이를 새 전방 장애물 interrupt로 해석해
`onFrontObstacleDetected()`를 호출했고, 이게 STOP을 내고 상태를
`AVOIDING_OBSTACLE`로 리셋했다. 그 결과 **`CHECKING_RIGHT` 평가가 가로채여**
오른쪽을 분류하기도 전에 무력화되고, "오른쪽 막힘 → ESCAPING" 전이가 발생하지
않아 multi-tick 후진 연쇄가 한 칸 만에 끊겼다. RVC는 다시 회피로 돌아와
진동했다.

### 발견 경위
L자 맵(출구가 **오른쪽**)은 정상 탈출했는데, 그 경우 probe 대상 칸이 *열려*
있어 rising edge가 없고 거짓 interrupt도 안 생겨 `CHECKING_RIGHT`가 정상
동작했기 때문이다. 테스트가 마침 interrupt 충돌이 안 일어나는 경우만 덮고
있어서, 좌측 출구·출구 없음 통로에서의 버그가 가려져 있었다.

### 해결
interrupt를 controller 상태로 게이트한다 — interrupt는 정상 주행 중에만 유효:

```cpp
const bool cruising = _controller.state() == RvcState::CLEANING
                   || _controller.state() == RvcState::INTENSIFYING;
if (front_blocked && !_prev_front_blocked && cruising) {
    _controller.onFrontObstacleDetected();
} else {
    _controller.onTick();
}
```

`AVOIDING_OBSTACLE` / `CHECKING_RIGHT` / `ESCAPING` 중의 회전은 더 이상 거짓
interrupt를 발생시키지 않는다. Right Scan이 정상 완료되고 후진 연쇄가 통로 끝까지
이어진다. `RvcController::state()` 노출이 필요했다. SRS에 FUNC-01로 반영
(2026-06-04 change trace).

### 결과
- `SimulatorTest`에 맵 기반 회귀 테스트 추가:
  - `EscapesDeadEndCorridorThroughLeftGap` — 아래 출구: 후진해 빠져나와 출구로 회전
  - `EscapesDeadEndCorridorThroughRightGap` — L자, 위 출구: Right Scan으로 탈출
  - `BacksUpFullCorridorThenEscapes` — 5칸 통로: 끝까지 후진 후 탈출
  - `AvoidanceInterruptDoesNotBreakBackupChain` — 후진 연쇄가 끊기지 않음
- `ctest` — **30/30 통과** (기존 26개 영향 없음). clang-tidy clean.

### 범위 밖
**완전히 밀폐된** 영역(출구 자체가 없음)은 여전히 종료 조건이 없다 — RVC가 통로
끝까지 후진한 뒤 제자리에서 idle한다. "출구 없음"을 감지하려면 후방 센서(거부:
하드웨어 추가는 RightSensor 제거 방향과 모순) 또는 위치 메모리(거부: reactive·
memoryless 설계 위반)가 필요하다. 따라서 임의의 정지 카운터로 덮지 않고 범위
밖으로 명시한다.
