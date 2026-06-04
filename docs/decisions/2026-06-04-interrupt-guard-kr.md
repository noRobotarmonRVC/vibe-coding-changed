# Interrupt 수용 정책 결정 - 2026-06-04

통합 설계에서 사용하는 front obstacle interrupt 수용 정책을 정리한다.
관련 실패 사례는 `docs/failures/2026-06-04-dead-end-escape-fix-kr.md`의
**F-10**이다.

## [추가]

- `RvcController::onFrontObstacleDetected()`가 `bool`을 반환한다.
- interrupt를 실제로 수용할지 여부는 controller가 판단한다.
- front interrupt가 무시되면 `Simulator::tick()`은 `RvcController::onTick()`으로
  폴백한다.
- 막다른 통로 탈출과 Right Scan 중 거짓 front interrupt를 검증하는 회귀
  테스트를 추가했다.

## [삭제]

- `RvcController::state()` getter는 추가하지 않는다.
- Simulator의 경계 보정 주행 알고리즘은 유지하지 않는다.
- 방문 상태, BFS, zigzag sweep, 후방 센서, 임의 stop counter는 추가하지 않는다.

## [변경]

- front obstacle interrupt는 정상 주행 상태인 `CLEANING` 또는 `INTENSIFYING`
  중에만 수용한다.
- `AVOIDING_OBSTACLE`, `CHECKING_RIGHT`, `ESCAPING` 중에는 Front Sensor가
  Right Scan에 재사용되므로 interrupt를 무시하고 tick 진행을 계속한다.
- Simulator는 clear-to-blocked front edge를 감지하지만, 그 edge가 유효한
  interrupt인지는 controller가 결정한다.
- console tick counter와 simulator tick은 `start()` 이후에만 증가한다.

## 배경

Front Sensor는 두 가지 역할을 가진다.

1. 정상 주행 중에는 전방 장애물 interrupt를 보고한다.
2. 장애물 처리 중에는 로봇이 오른쪽으로 회전한 뒤 Front Sensor로 기존
   오른쪽 방향을 스캔한다.

기존 구조에서는 Right Scan을 위해 회전했을 때 새 정면이 벽이면
clear-to-blocked edge가 다시 발생했다. Simulator가 이를 새로운 전방 장애물
interrupt로 처리하면서 `onFrontObstacleDetected()`를 호출했고, controller
상태가 `AVOIDING_OBSTACLE`로 되돌아갔다. 그 결과 `CHECKING_RIGHT` 평가가
중단되어 `ESCAPING` 전이가 건너뛰어지고, multi-tick 후진 연쇄가 끊길 수
있었다.

## 결정

front interrupt의 유효성 판단은 controller가 담당한다.

```cpp
bool RvcController::onFrontObstacleDetected() {
    if (_state != RvcState::CLEANING && _state != RvcState::INTENSIFYING) {
        return false;
    }
    _motor->move(Direction::STOP);
    _state = RvcState::AVOIDING_OBSTACLE;
    return true;
}
```

Simulator는 rising edge에서 이 메서드를 호출하고, 처리되지 않았으면
`onTick()`으로 계속 진행한다.

```cpp
bool handled = false;
if (front_blocked && !_prev_front_blocked) {
    handled = _controller.onFrontObstacleDetected();
}
if (!handled) {
    _controller.onTick();
}
```

이 방식은 캡슐화를 지킨다. Simulator는 controller 내부 상태를 읽지 않으며
state getter도 추가하지 않는다.

## 범위

이 결정은 Right Scan 중 발생하는 거짓 interrupt를 막고, 기존 multi-tick
escape sequence가 끊기지 않도록 한다. 전체 맵 커버리지를 새로 구현하지는
않는다. 현재 controller는 coverage planner가 아니라 reactive obstacle
avoidance controller이므로 외곽 순환은 여전히 발생할 수 있다.

## 검증

- `cmake -S src -B build`
- `cmake --build build`
- `ctest --output-on-failure`
- 현재 로컬 결과: **34/34 tests passed**.
