# Construction Decisions - 2026-05-21

Construction 단계에서 구현과 테스트를 진행하며 내린 결정을 기록한다.

---

## AD-06: Google Test는 FetchContent로 가져온다

**배경**  
개발 환경마다 Google Test가 system package로 설치되어 있다고 보장할 수 없다.

**결정**  
`find_package(GTest REQUIRED)` 대신 CMake `FetchContent`로 Google Test v1.14.0을 가져온다.

**결과**  
로컬과 CI에서 동일한 방식으로 test dependency를 준비할 수 있다.

---

## AD-07: Simulator API는 `inject*()` 명명 규칙을 사용한다

**배경**  
test double의 `set*()` 이름은 production domain의 setter anti-pattern과 혼동될 수 있다.

**결정**  
simulated sensor에 값을 넣는 API는 `injectFront`, `injectLeft`, `injectDust`처럼 `inject*()` 형식으로 명명한다.

**현재 상태**  
2026-05-29 이후 전용 right sensor injection은 제거되었다. 오른쪽 scan 결과는 Right Scan 상황에서 front sensor injection으로 표현한다.

---

## AD-08: Tests는 Observable Behavior를 검증한다

**결정**  
테스트는 controller 내부 state가 아니라 motor command log, cleaner power log, simulator position을 검증한다.

**결과**  
state machine 내부 구현이 바뀌어도 외부 동작이 유지되면 테스트가 불필요하게 깨지지 않는다.
