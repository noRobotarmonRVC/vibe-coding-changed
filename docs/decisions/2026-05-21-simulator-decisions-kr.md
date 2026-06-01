# Simulator / UI Decisions - 2026-05-21

Simulator와 console UI 구현 중 내린 결정을 기록한다.

---

## AD-08: 화면과 입력 분리를 위한 Raw Input Mode

**배경**  
background tick rendering과 사용자 command 입력이 같은 terminal에서 동시에 일어나므로, 일반 line input만으로는 화면이 쉽게 깨진다.

**결정**  
Unix 계열에서는 `termios` raw mode를 사용하고, Windows에서는 Console API와 `_getch()`를 사용한다. 입력 buffer는 직접 관리한다.

**결과**  
rendering thread가 grid를 갱신해도 입력 줄을 유지할 수 있다.

---

## AD-09: `std::mutex`로 Simulator 접근 보호

**배경**  
tick thread와 main input thread가 같은 simulator 객체에 접근한다.

**결정**  
`std::mutex`를 사용해 simulator 상태 변경과 rendering 구간을 보호한다.

**결과**  
동시 접근으로 인한 출력 깨짐과 상태 경쟁을 줄인다.

---

## AD-10: Grid 기반 장애물과 먼지 표현

**결정**  
simulator는 grid 좌표에 obstacle과 dust cell을 저장한다. tick마다 현재 위치와 주변 cell을 기반으로 sensor reading을 구성한다.

**현재 상태**  
2026-05-29 이후 오른쪽 주변 cell은 전용 RightSensor가 아니라 FrontSensor Right Scan reading으로 주입된다.
