# Checker Cleanup Loop

## Purpose
Use this skill when you need to verify or restore a clean checker run in this repository.

## Workflow
1. Run `tools/tasks/run.sh` with the relevant preset.
2. Inspect the generated checker log before assuming success.
3. Treat any warning, error, failed check, missing output, or ambiguous log line as failure/incomplete.
4. Fix the underlying issue.
5. Rerun `tools/tasks/run.sh`.
6. Repeat until the log is clean.

## Log Paths
- Do not assume the release log lives at `build/release/checker.log`.
- The actual release workflow writes to `build/release/release-work/checker.log`.
- `tools/tasks/run.sh` writes `build/<preset>/checkpp_style_check.log`.

Always inspect the log path produced by the command you ran.

## Stop Conditions
Stop only when all of the following are true:
- The checker command completes successfully.
- The inspected log contains no warnings or errors.
- The log shows a clean pass for the intended preset.

## Escalation
Escalate to the user when:
- The failure repeats after a fix and rerun.
- The log points to an environment, dependency, or repo-state issue you cannot resolve locally.
- The log path or preset is unclear and you cannot confirm the correct artifact.

When escalating, include the exact command run, the log path inspected, and the first blocking warning/error.
