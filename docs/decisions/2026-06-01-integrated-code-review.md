# Integrated Code Review - 2026-06-01

## [추가]
- Added this integration note for the `integrated-code` branch.
- Added simulator regression tests inspired by the `claude-code` branch:
  - one-cell maximum movement per tick during surrounded escape
  - backward movement along the original heading after Right Scan restore

## [삭제]
- Did not restore the dedicated `RightSensor` build dependency from `claude-code`.
- Did not adopt the Windows-incompatible `termios`-only main executable path.

## [변경]
- Kept `codex-code` as the base because it already passed full Windows build, CTest, Korean document cleanup, and `[추가] [삭제] [변경]` trace requirements.
- Integrated the safer parts of `claude-code` by strengthening tests rather than replacing the controller state machine wholesale.
