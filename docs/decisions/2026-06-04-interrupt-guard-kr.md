# Interrupt 게이트 결정 — 2026-06-04

front obstacle interrupt를 controller 상태로 게이트한 결정 기록. 실패 **F-10**
(막다른 길에서 출구가 있어도 탈출이 발화되지 않고 두 칸 사이를 진동)의 수정이다.
`docs/failures/2026-06-04-dead-end-escape-fix-kr.md` 참조.

---

## 맥락

Front Sensor는 같은 측정값을 두 용도로 공유한다.

1. 정상 주행 중 — **front obstacle interrupt** (rising edge: clear → blocked).
2. 회피 처리 중 — **Right Scan**: RVC가 우회전한 뒤 `FrontSensor::detect()`로
   기존 오른쪽 방향을 확인한다(`CHECKING_RIGHT`).

Right Scan을 위해 우회전하면 새 정면이 벽인 경우가 많아 front 측정값에
clear → blocked rising edge가 생긴다. Simulator는 이를 새 interrupt로 보고
`onFrontObstacleDetected()`를 호출했고, 이는 STOP을 발행하며 상태를
`AVOIDING_OBSTACLE`로 리셋했다. 그 결과 `CHECKING_RIGHT` 평가가 가로채여
"오른쪽 막힘 → ESCAPING" 전이가 일어나지 않고 multi-tick 후진 연쇄가 한 칸
만에 끊겨 진동했다. 출구가 **있는** 막다른 길과 완전히 막힌 막다른 길의 상태
천이가 동일해 controller가 둘을 구분할 수 없었다.

## 결정

front interrupt를 controller 상태로 게이트한다. interrupt는 정상 주행 중에만
의미가 있다.

```cpp
const bool cruising = _controller.state() == RvcState::CLEANING
                   || _controller.state() == RvcState::INTENSIFYING;
if (front_blocked && !_prev_front_blocked && cruising) {
    _controller.onFrontObstacleDetected();
} else {
    _controller.onTick();
}
```

회피 시퀀스(`AVOIDING_OBSTACLE` / `CHECKING_RIGHT` / `ESCAPING`) 중에는
interrupt를 억제하고 rising edge를 `onTick()`으로 평가한다. 이를 위해
Simulator가 controller 상태를 읽을 수 있도록 `RvcController::state()`를
노출했다.

- 코드: `src/simulator/Simulator.cpp`(`tick()` cruising 가드),
  `src/app/RvcController.hpp`(`state()` getter — 클래스 다이어그램에 반영).

## 거부한 대안 (범위 밖)

이 게이트는 출구가 **있는** 막다른 길을 고친다. **완전 밀폐**(출구 자체가 없는
영역)는 여전히 종료조건이 없어 RVC가 통로 끝까지 후진한 뒤 그 자리에 머문다.
"출구 없음"을 감지하는 두 방법을 검토했으나 거부했다.

- **후방센서.** 하드웨어 추가는 RightSensor 제거 방향(AD-11)과 모순이다 —
  센서를 빼는 방향이지 더하는 방향이 아니다.
- **위치 메모리.** 방문 칸을 기억하는 것은 controller가 따르는 reactive,
  memoryless 설계를 위반한다.

따라서 밀폐 영역 케이스는 임의의 stop counter로 가리지 않고 의도적으로 범위
밖으로 둔다.

## 트레이드오프

Simulator가 `RvcController::state()`에 의존하게 되어 simulator와 controller
표면 사이에 약간의 결합이 추가된다. 그 대가로 Front Sensor의 두 용도가 더 이상
충돌하지 않고, 출구가 있는 막다른 길에서 탈출이 동작하며, 상태 천이에서
"출구 있음"과 "출구 없음"을 다시 구분할 수 있다.

## 검증

- `test/simulator/SimulatorTest.cpp`에 맵 기반 회귀 테스트 4개 추가:
  - `EscapesDeadEndCorridorThroughLeftGap`
  - `EscapesDeadEndCorridorThroughRightGap`
  - `BacksUpFullCorridorThenEscapes`
  - `AvoidanceInterruptDoesNotBreakBackupChain`
- `ctest` — **30/30 통과** (기존 26개 영향 없음). clang-tidy clean.
