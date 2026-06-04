# 막다른 통로 탈출 실패와 해결 - 2026-06-04

Right Scan과 multi-tick escape를 통합하면서 확인한 실패 사례를 기록한다.

## [추가]

- 실패 사례 **F-10**을 추가했다. 내용은 Right Scan 중 거짓 front interrupt가
  발생해 후진 탈출이 끊기는 문제다.
- 왼쪽 출구, 오른쪽 출구, 긴 통로 후진, 후진 연쇄 유지 시나리오를 맵 기반
  회귀 테스트로 추가했다.
- controller의 interrupt 수용 정책을 단위 테스트로 검증한다.

## [삭제]

- Simulator가 controller 내부 상태를 직접 읽는 방식은 사용하지 않는다.
- 경계 보정 주행, 방문 상태, BFS, zigzag planner, 후방 센서, stop counter는
  이 수정 범위에 포함하지 않는다.

## [변경]

- `onFrontObstacleDetected()`는 interrupt를 수용했을 때만 `true`를 반환한다.
- 회피 상태 중 front edge는 interrupt로 처리하지 않고 `onTick()`으로 평가한다.
- 이 수정은 막다른 통로 탈출 연쇄가 끊기는 문제를 해결하는 것이며, 전체
  영역 커버리지 보장은 목표가 아니다.

## F-10: Front Interrupt가 Right Scan을 가로채는 문제

### 문제

막다른 통로에서 RVC가 출구가 있어도 빠져나가지 못하고 제한된 구간을 왕복할
수 있었다. 출구가 있는 맵과 완전히 막힌 맵의 상태 흐름이 동일하게 나타나
controller가 기대한 escape 전이에 도달하지 못했다.

### 원인

Front Sensor는 Right Scan에도 재사용된다. 로봇이 기존 오른쪽 방향을 확인하기
위해 오른쪽으로 회전하면, 새 정면이 벽을 향할 수 있다. 이때 front reading에
clear-to-blocked edge가 생기고, Simulator가 이를 새로운 전방 장애물
interrupt로 해석했다.

그 결과 `onFrontObstacleDetected()`가 호출되어 `STOP`을 내고 상태를
`AVOIDING_OBSTACLE`로 되돌렸다. 이 동작이 `CHECKING_RIGHT` 평가를 가로채면서
"right blocked -> ESCAPING" 전이가 건너뛰어졌고, multi-tick 후진 연쇄가 끊길
수 있었다.

### 해결

interrupt 수용 여부는 controller가 결정한다.

- `CLEANING` / `INTENSIFYING`: interrupt를 수용하고 `STOP` 후
  `AVOIDING_OBSTACLE`로 전이하며 `true`를 반환한다.
- `AVOIDING_OBSTACLE` / `CHECKING_RIGHT` / `ESCAPING`: interrupt를 무시하고
  `false`를 반환한다.

Simulator는 interrupt가 처리되지 않았을 때 `onTick()`으로 폴백한다. 이로써
controller state getter를 노출하지 않고도 Right Scan과 multi-tick escape
sequence가 계속 진행된다.

```cpp
bool handled = false;
if (front_blocked && !_prev_front_blocked) {
    handled = _controller.onFrontObstacleDetected();
}
if (!handled) {
    _controller.onTick();
}
```

### 검증

- `RvcControllerTest.FrontInterruptAcceptedWhileCruising`
- `RvcControllerTest.FrontInterruptIgnoredDuringAvoidance`
- `RvcControllerTest.FrontInterruptIgnoredWhileEscaping`
- `SimulatorTest.EscapesDeadEndCorridorThroughLeftGap`
- `SimulatorTest.EscapesDeadEndCorridorThroughRightGap`
- `SimulatorTest.BacksUpFullCorridorThenEscapes`
- `SimulatorTest.AvoidanceInterruptDoesNotBreakBackupChain`

현재 로컬 결과: `ctest --output-on-failure` 기준 **34/34 tests passed**.

### 남은 범위

이 수정은 전체 맵 커버리지를 보장하지 않는다. 로봇이 외곽을 계속 도는 현상은
기존 reactive navigation model의 한계다. 이를 제대로 해결하려면 별도의
coverage planning 요구사항이 필요하다.
