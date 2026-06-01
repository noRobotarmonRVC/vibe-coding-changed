# Simulator Failures and Resolutions - 2026-05-21

Simulator와 console UI 구현 중 발견한 문제를 기록한다.

---

## F-07: Background Rendering이 입력 줄을 깨뜨림

**문제**  
tick thread가 grid를 다시 그리는 동안 사용자가 입력 중인 command line이 깨질 수 있었다.

**원인**  
rendering과 input이 같은 console output을 공유했지만, cursor 위치와 입력 buffer가 별도로 관리되지 않았다.

**해결**  
raw input mode를 사용하고, 입력 buffer를 직접 관리하며, rendering 후 고정된 input row를 다시 그리도록 변경했다.

---

## F-08: Thread 간 Simulator 접근 경쟁

**문제**  
tick thread와 input thread가 동시에 simulator 상태를 읽고 쓸 수 있었다.

**해결**  
`std::mutex`로 simulator 접근을 보호했다.

---

## F-09: Platform-specific Terminal API

**문제**  
Unix의 `termios`는 Windows MSVC 환경에서 제공되지 않는다.

**해결**  
Unix는 `termios`를 사용하고, Windows는 Console API와 `_getch()`를 사용하도록 분기한다.

**결과**  
Windows와 Linux 모두에서 전체 CMake 빌드가 가능해졌다.
