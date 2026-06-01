# Claude 설계 흡수 결정 — 2026-06-01

`integrated-code`(codex base + claude 테스트 2종)에서 한 단계 더 나아가, claude의 **설계**까지
흡수한 `integrated-claude-design` 브랜치에 대한 결정 기록.

`integrated-code`는 제출 안정성·문서 추적성을 이유로 codex 상태머신을 그대로 두고 테스트 2종만
가져왔다. 그러나 codex 스스로 "claude의 멀티틱 세분화가 더 낫다"고 평가했음에도 그 설계를
버린 것은 모순이었다. 빌드 안정성·문서 품질은 codex의 자산을 유지한 채로도 claude 설계를
흡수할 수 있음을 확인하여 본 브랜치를 만든다.

---

## [추가]
- claude의 명시적 멀티틱 상태머신을 흡수: `RvcState`에 `CHECKING_RIGHT` 추가.
  - 우측 탐지(우회전 → 전방 센서 확인 → 복귀)가 별도 상태로 관측 가능해진다.
- claude의 전체 테스트 스위트(26종)로 교체 — codex 22종 대비 다음이 추가됨:
  - `SimulatorTest.DeadEndBacksUpMultipleTimes` (막다른 길 반복 후진)
  - `SimulatorTest.FrontInterruptFiresOnceWhileStuck` (정지 중 인터럽트 1회만 발화)
  - `SimulatorTest.SurroundedEscapeBacksUpThenResumesForward` (포위 탈출 후 전진 재개)
  - `SimulatorTest.BacksUpAlongOriginalHeadingAfterProbe` (probe 후 원래 heading 기준 후진)
  - `SimulatorTest.NeverMovesMoreThanOneCellPerTick` (틱당 이동 1칸 이하)

## [삭제]
- codex의 `is_right_blocked`(`SensorData`)를 제거 — "우측 센서 없음"을 자료구조 수준에서 드러낸다.
- codex의 `_escape_step` 카운터 / `continueEscaping()` 방식을 폐기하고 명시 상태로 대체.

## [변경]
- 상태머신 구현을 codex(`_escape_step` 카운터)에서 claude(`CHECKING_RIGHT` 상태) 방식으로 교체.
- `RvcController`·`DefaultNavigationStrategy`·`Simulator`를 claude 설계본으로 포팅.

## [유지] — codex 자산 보존
- `src/app/main.cpp` — codex의 `#ifdef _WIN32` 포팅 버전 유지 (claude의 termios 전용 경로 미채택).
- `src/CMakeLists.txt` — `RightSensor.cpp` 빌드 제외 + MSVC `/utf-8` 유지.
- 한국어 문서 정리 및 `[추가] [삭제] [변경]` 추적 컨벤션 유지.

---

## 트레이드오프
codex의 더 단순한 상태머신 대신 상태가 하나 늘어난다(`CHECKING_RIGHT`). 그러나 우측 탐지가
독립 상태로 관측·테스트 가능해지고, 자료구조에서 우측 센서 부재가 명확해진다. 빌드 안정성과
문서 추적성은 codex 자산을 그대로 보존하므로 손실이 없다.

## 검증
- `cmake -S src -B build && cmake --build build` 성공.
- `ctest` — **26/26 통과**.
- `.clang-tidy` 설정은 claude-code와 동일하며 해당 코드는 이미 통과 이력 있음.
