---
name: checker-clean-loop
description: >
  Keep the checkpp checker loop running until it finishes cleanly.
  Trigger: use this when the workspace must end with a clean checker log or when checker warnings/errors need to be fixed.
license: Apache-2.0
metadata:
  author: gentleman-programming
  version: "1.0"
---

## When to Use

- Use for the checkpp checker workflow in this workspace.
- Use when you need to guarantee `build/release/checker.log` has zero warnings and zero errors.

## Critical Patterns

- Always run `tools/tasks/run.sh` from the repository root.
- Inspect `build/release/checker.log` after every run.
- Treat any warning or error line in that log as a failed run, even if the command exits successfully.
- Fix the underlying repo issue, not the log symptom.
- Re-run the checker until the log is clean.

## Required Loop

1. Run `tools/tasks/run.sh`.
2. Read `build/release/checker.log`.
3. If the log contains warnings or errors, treat the iteration as incomplete.
4. Fix the smallest underlying cause in the repository.
5. Run `tools/tasks/run.sh` again.
6. Repeat until the log contains no warnings and no errors.

## Stop Conditions

- Stop only when `build/release/checker.log` is clean.
- Stop and escalate if progress is blocked by a missing dependency, a broken toolchain, or a change that needs user input.
- Stop and report the exact blocker if the same failure repeats after a reasonable fix attempt.

## Escalation Guidance

- Report the exact checker output and the suspected blocker.
- State whether the failure is reproducible and whether it is in repo code, generated output, or the environment.
- If the blocker is external, ask for the missing dependency, permission, or decision.

## Commands

```bash
tools/tasks/run.sh
```

## Notes

- Keep this skill aligned with the canonical checker log path if the repository layout changes.
