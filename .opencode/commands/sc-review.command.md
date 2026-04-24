---
description: Run a configured project review and optionally publish it
agent: plan
subtask: true
---

Run the `sc-review` skill for this repository.

User command arguments: $ARGUMENTS

Interpret arguments using this contract:
- first positional argument = process name
- optional `--publish github`
- optional `--publish local`
- optional `--ignore-milestone`

Behavior:
- default is draft review, unpublished
- if `--publish github` is present, publish via `/sc-gh-issue`
- if `--publish local` is present, write a markdown report to the configured local review output directory (currently `docs/review/` from `docs/sc-config.yaml`)
- if no publish target is provided, return the draft report only
- if `--ignore-milestone` is present, skip milestone scoping even if configured

Load and follow the `sc-review` skill.
Use `docs/sc-config.yaml` as the source of review definitions.
Resolve review settings from `processes.<process>.actions.review`, plus shared process context from `processes.<process>` and defaults such as the milestone file, local review output directory, and GitHub issue handoff command.
