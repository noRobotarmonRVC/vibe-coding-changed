# Glossary

## 개정 이력

| 버전 | 날짜 | 변경 내용 |
|---|---|---|
| 1.0 | 2026-05-21 | 최초 작성 |
| 1.1 | 2026-05-29 | **Right Sensor** 용어 삭제; **Obstacle**·**Surrounded State** 정의에서 우측 센서 참조 제거 (우측은 전방 센서로 탐지 — AD-11) |

---

용어는 알파벳 순으로 나열된다. 약어는 첫 사용 시 풀어서 표기한다.

---

| 용어 | 정의 |
|---|---|
| **Cleaner** | 청소기 및 걸레질을 담당하는 HW 컴포넌트. Off, On, Power Up 전원 명령을 수신한다. |
| **Control SW** | RVC Control Software — 이 프로젝트에서 개발 중인 SW 시스템. 센서 입력을 처리하고 액추에이터 명령을 발행한다. |
| **Direction Command** | Motor에 발행되는 이동 명령: Forward, Backward, Left(회전), Right(회전). |
| **Dust Sensor** | 바닥에 먼지가 감지되면 True, 그렇지 않으면 False를 보고하는 주기적(periodic) 센서. |
| **Front Sensor** | RVC 정면에 장애물이 감지되면 True를 보고하는 interrupt 방식 센서. |
| **I/O Event** | Control SW가 사용하는 추상화된 입출력 신호. SW 로직을 HW 구현으로부터 분리한다. |
| **Interrupt** | HW에서 트리거되는 비동기 신호. Front Sensor는 interrupt를 사용하여 다음 Tick을 기다리지 않고 즉시 응답한다. |
| **Left Sensor** | RVC 좌측에 장애물이 감지되면 True, 그렇지 않으면 False를 보고하는 주기적(periodic) 센서. |
| **Motor** | RVC 이동을 구동하는 HW 컴포넌트. Direction Command를 수신한다. |
| **OOAD** | Object-Oriented Analysis and Design — 이 프로젝트에서 사용하는 개발 방법론. |
| **Obstacle** | RVC의 경로를 막는 물리적 물체. Front, Left 센서가 감지(우측은 우회전 후 전방 센서로 확인). |
| **Periodic** | 센서 읽기 모드의 일종으로, Tick 간격마다 한 번 센서 상태를 샘플링한다. |
| **Power Up** | 먼지가 감지될 때 사용하는, 정상 On 수준 이상으로 청소 강도를 높이는 Cleaner 명령. |
| **RVC** | Robot Vacuum Cleaner — 이 프로젝트에서 Control SW를 개발하는 자율 가정용 청소 기기. |
| **Safe State** | 시스템이 불확정 조건을 만났을 때의 폴백 상태: Motor 정지, Cleaner Off. |
| **Simulator** | 통합 테스트 목적으로 RVC HW 환경을 에뮬레이션하는 SW 컴포넌트. |
| **Surrounded State** | 전방·좌측이 차단되고, 우회전 후 전방 센서로 확인한 우측도 차단된 상태로, RVC가 회전 전에 후진해야 한다. |
| **Tick** | RVC Control SW의 메인 제어 루프를 구동하는 Digital Clock의 주기적 신호. |
| **Timer** | 고정 간격으로 Tick 신호를 생성하는 Digital Clock 서브시스템. |
| **TDD** | Test-Driven Development — 구현 코드 이전에 테스트를 먼저 작성한다. |
| **UI** | User Interface — 최종 사용자가 RVC와 상호작용하는 별도 컴포넌트 (Control SW 범위 밖). |
| **UP** | Unified Process — 이 프로젝트에서 따르는 반복적, Use-Case 중심 SW 개발 프로세스. 단계: Inception → Elaboration → Construction. |
| **V&V** | Verification and Validation — 시스템이 올바르게 구축되었는지(verification)와 올바른 시스템인지(validation)를 확인한다. |
