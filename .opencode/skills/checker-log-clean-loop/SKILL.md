---
name: checker-log-clean-loop
description: >
  Run the checkpp checker loop until `build/release/checker.log` is clean.
  Trigger: use this when the workspace must end with zero warnings and zero errors in the checker log.
license: Apache-2.0
metadata:
  author: gentleman-programming
  version: "1.0"
---

## Purpose

- Keep the checkpp checker workflow running until the log is clean.
- Use this skill for any task that must finish with no warnings and no errors in `build/release/checker.log`.

## Required Loop

1. Run `tools/tasks/run.sh` from the repository root.
2. Inspect `build/release/checker.log` immediately after the run.
3. Treat every warning line and every error line in that log as a failed iteration, even if the command exits successfully.
4. Fix the smallest underlying code, config, or build issue in the repository.
5. Run `tools/tasks/run.sh` again.
6. Repeat until the log contains zero warnings and zero errors.

## Stop Conditions

- Stop only when `build/release/checker.log` is clean.
- Stop and escalate if the checker is blocked by a missing dependency, toolchain failure, permission issue, or a change that requires user input.
- Stop and report a true blocker if the same failure repeats after a reasonable fix attempt.

## Escalation Guidance

- Report the exact checker output and the suspected root cause.
- Say whether the blocker is in repository code, generated output, or the local environment.
- If the blocker is external, ask for the missing dependency, permission, or decision.

## Notes

- Do not treat a green exit code as success unless the log is clean.
- Do not patch the log file directly; fix the underlying issue and rerun.
- Keep this skill aligned with the canonical checker log path if the repository layout changes.

## Command

```bash
tools/tasks/run.sh
```
