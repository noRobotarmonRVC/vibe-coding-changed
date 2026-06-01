# Trace Consistency Review - 2026-05-29

## [추가]
- Added a repository-wide review item to confirm that SRS, design, code, simulator, and tests describe the same Right Scan / tick escape change.

## [삭제]
- Deleted the assumption that code-only comments are enough for this change; SRS and design traces are also required.

## [변경]
- Changed the review standard so every affected folder uses `[추가]`, `[삭제]`, and `[변경]` markers in its native form: Markdown sections for documents and comments for code/tests.

## Result
- The consistency review found stale Korean design and requirement wording from the old RightSensor model. Those documents were marked for correction under the current trace convention.
