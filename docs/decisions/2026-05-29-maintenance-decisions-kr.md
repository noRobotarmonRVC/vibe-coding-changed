# 유지보수 결정 사항 — 2026-05-29

HW 변경(우측 센서 제거)과 후진 동작 결함(F-06)에 대응하여 RVC Control SW에 내린 결정.

---

## AD-11: 우측 센서 제거 → 전방 센서 재활용으로 우측 탐지

**배경**
HW 구성 변경으로 우측 센서가 제거되었다. 그러나 포위(Surrounded) 판정을 위해 우측 장애물 여부는 여전히 필요하다.

**결정 근거**
우측 전용 센서 없이 우측을 알 수 있는 유일한 방법은 기존 센서를 활용하는 것이다. 전방 센서는 회전 후 우측 칸을 바라볼 수 있다. 대안인 "새 센서 추가"는 외부 HW 변경을 요구하고, "좌측만으로 판단"은 요구사항(우측 탐지)을 포기하는 것이다. 기존 자원으로 요구를 충족하는 회전 탐지를 택한다.

**결정**
전방과 좌측이 모두 막히면 우회전(RIGHT)하여 `CHECKING_RIGHT` 상태로 진입한다. 다음 틱에 전방 센서로 우측(현재 전방)을 확인한다. 열려 있으면 그대로 전진을 재개하고, 막혀 있으면 좌회전으로 원래 방향에 복귀한 뒤 포위로 판정(`ESCAPING`)한다.

**산출물**
- `domain/SensorData.hpp` — `is_right_blocked` 제거
- `domain/DefaultNavigationStrategy.cpp` — 전방+좌측 막힘 → `BACKWARD`(컨트롤러에 "우측 탐지 후 탈출" 신호)
- `app/RvcController` — `_right_sensor` 제거, `_front_sensor`를 `CHECKING_RIGHT`의 우측 확인에 사용
- `simulator/Simulator` — 우측 자동 주입 제거, 전방 센서를 매 틱 주입(폴링 가능하도록)

**트레이드오프**
우측 확인에 회전 1회(우회전 → 복귀)가 추가되어 회피 틱이 늘어난다. 전용 센서가 없는 HW 제약을 동작으로 보완하는 현실적 선택이다.

---

## AD-12: 회피/탈출의 원자적 처리(AD-03) 폐기 → 멀티틱 상태머신

**배경**
AD-03은 `AVOIDING_OBSTACLE`/`ESCAPING`을 `onFrontObstacleDetected()` 내에서 한 틱에 원자적으로 처리하고 즉시 `CLEANING`으로 복귀했다. 이로 인해 한 틱에 후진+회전+전진이 모두 적용되어 로봇이 한 틱에 여러 칸 이동하는 결함이 발생했다(F-06).

**결정 근거**
물리적 RVC는 한 틱에 한 동작(이동 또는 회전)만 수행한다. 원자적 처리는 시뮬레이션상 순간이동을 야기하며, 우측 회전 탐지(AD-11)도 본질적으로 멀티틱이 필요하다.

**결정**
`onFrontObstacleDetected()`는 STOP 발행 + `AVOIDING_OBSTACLE` 진입만 담당한다(인터럽트의 즉각 정지). 이후 `onTick()`이 상태별로 틱당 한 단계씩 진행한다:

| 상태 | 한 틱 동작 | 다음 상태 |
|---|---|---|
| AVOIDING_OBSTACLE | 좌측 확인 → 좌회전 / 우회전(probe) | CLEANING / CHECKING_RIGHT |
| CHECKING_RIGHT | 전방 센서로 우측 확인 → 전진 / 좌회전 복귀 | CLEANING / ESCAPING |
| ESCAPING | 후진 한 칸 | AVOIDING_OBSTACLE (재평가) |

**산출물**
- `domain/RvcState.hpp` — `CHECKING_RIGHT` 추가
- `app/RvcController.cpp` — `onTick()`을 switch 기반 멀티틱 로직으로
- `simulator/Simulator.cpp` — 전방 인터럽트를 상승 엣지(clear→blocked)에서만 발화, 그 외엔 `onTick()`으로 진행(`_prev_front_blocked`)

**트레이드오프**
한 번의 회피가 여러 틱에 걸쳐 진행되어 회피 완료까지 더 오래 걸린다. 실제 로봇의 "1틱 1동작" 모델과 일치하고 각 동작이 관측 가능해지므로 수용한다.
