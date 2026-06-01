# Architecture Decisions - 2026-05-17

Iteration 1의 Inception, Elaboration, Construction 단계에서 내린 주요 아키텍처 결정을 기록한다.

---

## Supersession Note - 2026-05-29

### [추가]
- 기존 sensor architecture 결정을 현재 Right Scan architecture와 연결하는 메모를 추가한다.

### [삭제]
- AD-02와 AD-04에서 전용 `RightSensor`에 대한 활성 의존을 삭제한다.

### [변경]
- 오른쪽 감지에 한해 AD-02와 AD-04는 FrontSensor Right Scan 및 tick 기반 ESCAPING 결정으로 대체된다. 자세한 내용은 `docs/decisions/2026-05-29-right-scan-and-tick-escape.md`를 따른다.

---

## AD-01: Navigation Logic에 Strategy Pattern 적용

**배경**  
`RvcController`가 sensor data를 바탕으로 이동 방향을 결정해야 한다.

**결정**  
navigation logic을 `INavigationStrategy`로 분리하고, 기본 구현으로 `DefaultNavigationStrategy`를 둔다.

**결과**  
controller는 concrete navigation algorithm을 알지 않고 `navigate(SensorData)`만 호출한다.

---

## AD-02: Front Sensor는 Interrupt, 나머지 입력은 Tick 기반 처리

**배경**  
전방 장애물은 즉시 정지가 필요하므로 tick polling만으로 처리하면 반응이 늦다.

**결정**  
Front Sensor는 interrupt path를 제공하고, 다른 입력은 tick 또는 obstacle handling 흐름에서 읽는다.

**현재 상태**  
2026-05-29 이후 오른쪽 입력은 전용 Right Sensor가 아니라 FrontSensor Right Scan으로 대체되었다.

---

## AD-03: `RvcController`에 명시적 State Machine 도입

**결정**  
`IDLE`, `CLEANING`, `AVOIDING_OBSTACLE`, `ESCAPING`, `INTENSIFYING` 상태를 사용한다.

**현재 상태**  
2026-05-29 이후 `ESCAPING`은 단일 이벤트 안에서 끝나는 임시 처리 방식이 아니라 여러 tick에 걸쳐 유지되는 실제 진행 상태이다.

---

## AD-04: Hardware Dependency는 Constructor Injection으로 주입

**결정**  
sensor, motor, cleaner, navigation strategy를 controller 생성자에서 주입한다.

**현재 상태**  
전용 RightSensor는 constructor dependency에서 제거되었다. 오른쪽 판단은 FrontSensor Right Scan 결과로 전달된다.

---

## AD-05: Internal State Getter 제거

**결정**  
외부 테스트와 UI가 controller 내부 state에 직접 의존하지 않도록 `state()` getter를 제거한다.

**결과**  
테스트는 state label 대신 motor log와 cleaner log 같은 observable output을 검증한다.
