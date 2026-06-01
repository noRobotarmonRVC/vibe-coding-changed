# Failures and Resolutions - 2026-05-21

Construction 후반에 발견한 구현상 문제와 해결책을 기록한다.

---

## F-04: 테스트가 내부 구현에 과도하게 의존함

**문제**  
일부 테스트가 controller 내부 state나 구현 순서를 지나치게 강하게 가정했다.

**원인**  
observable behavior보다 내부 state를 직접 검증하는 방식이 빠르게 작성되었다.

**해결**  
테스트 기준을 motor command, cleaner command, simulator position 같은 외부 관찰 결과로 변경했다.

---

## F-05: Build 환경 차이에 따른 실패 가능성

**문제**  
Linux/WSL 환경을 기준으로 작성된 코드가 Windows MSVC 빌드에서 실패할 수 있었다.

**해결**  
platform-specific 코드는 compile-time 분기로 처리하고, core logic은 platform neutral하게 유지한다.

---

## F-06: 변경 추적 누락 위험

**문제**  
요구사항, 설계, 코드, 테스트가 함께 바뀌는 변경에서 일부 문서가 뒤늦게 갱신될 위험이 있었다.

**해결**  
변경 단위마다 `[추가]`, `[삭제]`, `[변경]`을 명시해 추적성을 높인다.
