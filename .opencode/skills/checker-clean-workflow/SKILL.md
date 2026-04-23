---
name: checker-clean-workflow
description: >
  Run the checkpp project checker until it finishes cleanly.
  Trigger: use this when iterating on checker warnings/errors or when the workspace must end with a clean checker log.
license: Apache-2.0
metadata:
  author: gentleman-programming
  version: "1.0"
---

## When to Use

- Use for the checkpp checker loop in this workspace.
- Use when you need to verify that the project checker finishes with zero warnings and zero errors.

## Critical Patterns

- Always start from `run.sh` in this repository.
- After every run, inspect `build/release/checker.log`.
- Treat any warning or error line in that log as a failed run, even if the process exits successfully.
- Fix the underlying code/config issue, then run the checker again.
- Keep iterating until the log contains no warnings and no errors.

## Exact Loop

1. Run `run.sh` from the workspace root.
2. Read `build/release/checker.log`.
3. If the log has warnings or errors, fix the smallest root cause.
4. Re-run `run.sh`.
5. Repeat until the log is clean.

## Stop Conditions

- Stop when `build/release/checker.log` has zero warning entries and zero error entries.
- Stop and escalate if the checker is blocked by a missing dependency, a broken build, or a change that requires user guidance.
- Stop and report the exact blocker if the same failure repeats after a reasonable fix attempt.

## Commands

```bash
run.sh
```

## Notes

- If the repository ever changes the checker log location, keep this skill aligned with the canonical path used by the checker workflow.
