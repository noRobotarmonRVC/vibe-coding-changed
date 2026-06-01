# Failures and Resolutions - 2026-05-17

Construction 중 발견한 실패와 원인, 해결책을 기록한다.

---

## Historical Supersession Note - 2026-05-29

### [추가]
- 오래된 simulator API 예시가 현재 활성 설계로 오해되지 않도록 이 메모를 추가한다.

### [삭제]
- 과거 `injectRight()` 예시에 대한 활성 의존을 삭제한다. 아래 내용은 2026-05-17 명명 결정의 기록으로만 유지한다.

### [변경]
- 현재 simulator 오른쪽 입력은 FrontSensor Right Scan 준비 과정에서 `injectFront()`로 표현한다. 자세한 내용은 `docs/decisions/2026-05-29-right-scan-and-tick-escape.md`를 따른다.

---

## F-01: CMake GTest 설정 실패

**문제**  
`find_package(GTest REQUIRED)`가 환경에 설치된 GTest를 찾지 못해 configure 단계에서 실패했다.

**원인**  
개발 환경에 GTest system package가 설치되어 있다고 가정했다.

**해결**  
CMake `FetchContent`로 Google Test v1.14.0을 가져오도록 변경했다.

---

## F-02: Domain Class Getter Anti-pattern

**문제**  
`RvcController::state()` getter가 내부 state enum을 외부에 노출했다.

**원인**  
테스트와 UI가 상태 확인을 쉽게 하기 위해 내부 구현에 의존했다.

**해결**  
`state()` getter를 제거하고, 테스트는 motor/cleaner output log를 검증하도록 변경했다.

---

## F-03: Simulator Setter Naming

**문제**  
simulator control API에 `set*()` 이름을 사용해 domain setter anti-pattern과 혼동되었다.

**해결**  
test double에 값을 주입하는 API는 `inject*()` 형식으로 변경했다.

**현재 상태**  
오른쪽 sensor injection은 현재 활성 API가 아니다. Right Scan 결과는 front sensor injection으로 표현한다.
