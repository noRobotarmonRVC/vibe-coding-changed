# AGENTS.md

## Change Trace - 2026-05-29

### [추가]
- Added repository-wide trace convention for the Right Scan / tick escape change.

### [삭제]
- Removed dedicated RightSensor from the active change intent.

### [변경]
- Changed future agent expectations: requirements, design, code, and tests must all mark [추가], [삭제], and [변경] for this change family.

## Project Outline
- Goal : Use OOAD based on UP and Make RVC Control SW, RVC UI and RVC Simulator
- Outputs : SRS, SDD, Code, UT, ST, SA, Simulator
- Language : C++17 
- Environment : Ubuntu 24.04 WSL2
- Development Method : OOAD based on UP(Unified Process), V&V and TDD

## CI / CD
### Development
- IDE : Visual Stuido Code
- Build Management : CMake
- Unit Testing : Google Test
- Version Control : Github
- Requirements Management : Notion, Jira
### CI
- Continuous Intergration : Github Action
- Build Management : CMake
- Unit Testing : Google Test
- Static Code Analysis : clang-tidy
- Coverage : gcovr
### etc.
- Intergrated Testing : Using Simulator

## Directory 

```
OOADProject/
│
├── AGENTS.md
├── .clang-format
├── .clang-tidy
├── .gitignore
├── docs/
│   ├── requirements/
│   ├── design/
├── skils/
│   ├── conventions/
├── src/
│   ├── CMakeList.txt
├── test/

```

## Strict Prohibition (Highest Priority)
- Adding external packages or libraries
- Deleting any file or directory
- Changing any 

## Test Rule
