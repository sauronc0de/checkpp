---
name: checker-release-log-clean-loop
description: >
  Run the checkpp checker release loop until the checker log is clean.
  Trigger: use this when the release checker must finish with zero warnings and zero errors.
license: Apache-2.0
metadata:
  author: gentleman-programming
  version: "1.0"
---

## Purpose

- Use this skill for the release checker workflow in checkpp.
- Success requires zero warnings and zero errors in the checker log.
- Any warning or error means the process is incomplete and must be fixed and re-run.

## Log Paths

- Primary generated log: `build/release/release-work/checker.log`.
- Some docs and older references mention `build/release/checker.log`.
- If both paths appear, trust the actual generated log under `build/release/release-work/checker.log` and treat the shorter path as a reference alias only.

## Required Loop

1. Run `tools/tasks/run.sh` from the repository root.
2. Inspect the relevant checker log immediately after the run.
3. Treat every warning line and every error line as a failed iteration, even if the command exits successfully.
4. Fix the smallest underlying code, config, or build issue.
5. Run `tools/tasks/run.sh` again.
6. Repeat until the checker log has zero warnings and zero errors.

## Stop Conditions

- Stop only when the checker log is clean.
- Stop and escalate if blocked by a missing dependency, toolchain failure, permission issue, or a change that needs user input.
- Stop and report a true blocker if the same failure repeats after a reasonable fix attempt.

## Notes

- Do not treat a green exit code as success unless the checker log is clean.
- Do not patch the log file directly; fix the underlying issue and rerun.
- Keep this skill aligned with the actual generated checker path if the workflow changes.

## Command

```bash
tools/tasks/run.sh
```
