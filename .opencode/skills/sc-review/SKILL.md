---
name: sc-review
description: execute predefined project reviews for a named process using a user-maintained config in docs/sc-config.yaml, validate required inputs, optionally scope findings to the active release milestone defined in config defaults or process overrides, generate a structured review report, and either keep it local by default or publish it when explicitly requested. use when the user asks to run review commands such as /sc-review tools, /sc-review tools --publish github, /sc-review tools --publish local, or /sc-review tools --ignore-milestone. coordinate github issue publishing through /sc-gh-issue instead of creating or updating issues directly.
metadata:
  version: "0.0.0"
---

# sc-review

## Overview

Use this skill to run a predefined review for a named process such as `tools`.
Resolve review definitions from `docs/sc-config.yaml`.
Default to a draft review with no publication.
Only publish when the user explicitly requests `--publish github` or `--publish local`.

## Workflow

1. Parse the requested process name and optional flags.
2. Load `docs/sc-config.yaml` and resolve `processes.<process>.actions.review`, merged with shared context from `processes.<process>` and defaults. If the review action is missing, stop and return a blocking result.
3. Validate all required inputs.
4. If milestone scoping is enabled for the review and not overridden with `--ignore-milestone`, load the configured milestone file from process overrides or `defaults.milestone` and resolve the active milestone from the first H1.
5. Review only the declared in-scope content.
6. Produce a structured report.
7. If `--publish github` was requested, delegate publication to the configured GitHub issue skill command.
8. If `--publish local` was requested, write the markdown report under the configured local output directory using the naming convention below.
9. If no publish target was requested, return the report as a draft and do not create or update anything.

## Command Contract

Interpret commands with this contract:

- `/sc-review <process>`
- `/sc-review <process> --publish github`
- `/sc-review <process> --publish local`
- `/sc-review <process> --ignore-milestone`
- `/sc-review <process> --ignore-milestone --publish local`

Rules:

- The first positional argument is the process key from `docs/sc-config.yaml`.
- Default mode is draft.
- Default publish target is none.
- `--publish github` means official publication through the configured GitHub issue skill command.
- `--publish local` means create a markdown file in the configured local review output directory.
- `--ignore-milestone` disables milestone scoping even if the review config defines a milestone file.
- Do not invent unsupported flags.

## Critical Patterns

### Pattern 1: resolve review definition from docs

Always load the review definition from `docs/sc-config.yaml`.
Do not hardcode file paths when the config defines them.
Treat the config in `docs/` as user-maintained and editable.
Treat `.opencode/` as implementation detail, not user configuration.

Resolution order:

- defaults from `defaults.*`
- shared process context from `processes.<process>` such as `guidelines`
- review-specific settings from `processes.<process>.actions.review`

Treat `processes.<process>.actions.review` as the action-specific contract.
Treat shared process fields like `guidelines` as additional required review context when present.

### Pattern 2: validate required dependencies before reviewing

A review cannot proceed normally if a required dependency is missing.
Required dependency types are:

- primary input to review
- criteria or guideline file
- milestone file only when the review requires milestone scoping and `--ignore-milestone` is not set

The expected output template is optional unless the config explicitly marks it as required.

If a required dependency is missing:

- stop the normal review
- produce a blocking result
- clearly state which dependency is missing
- do not create findings against the reviewed content

### Pattern 3: milestone scope is automatic by default

If config defines a milestone file and milestone scoping is enabled:

- read the milestone file automatically
- resolve the active milestone from the first H1 heading
- restrict findings to milestone scope unless `--ignore-milestone` is passed

When `--ignore-milestone` is present:

- skip milestone scoping entirely
- review the full configured input set
- mark the report clearly as `milestone scope ignored`

### Pattern 4: parse the active milestone from the title

When a milestone file is configured, parse the first H1 using this format:

- `# Release milestone M0 - Signal Mastery Prototype`
- `# Release milestone M0`

Extraction rules:

- milestone code = the token after `Release milestone`
- milestone description = the optional text after ` - `
- use the milestone code for naming and issue titles
- use the milestone description only as report metadata

If the first H1 cannot be parsed:

- if milestone scoping is required, return a blocking result
- if milestone scoping is optional, continue without milestone scope and say so explicitly

### Pattern 5: findings must stay in scope

When milestone scoping is active:

- only report findings for content that is in scope for the active milestone
- do not convert out-of-scope items into issues
- you may mention out-of-scope observations separately only if clearly labeled as out of scope and non-blocking

### Pattern 6: severity and decision model

Use exactly these severities:

- `info`
- `warning`
- `error`

Decision rules:

- `pass`: no errors
- `pass-with-concerns`: one or more warnings and no errors
- `fail`: one or more errors, or missing required dependencies

Blocking rule:

- every `error` is blocking
- `warning` is non-blocking
- `info` never changes pass or fail

### Pattern 7: publication is separate from analysis

Draft and published reviews must use the same review logic, inputs, scope rules, and pass or fail criteria.
Only the output destination changes.

- no publish target: return the report only
- `--publish local`: write the report to `defaults.local_review_output_dir` unless the config defines another output directory later
- `--publish github`: delegate issue creation or issue comment to `defaults.github_issue_skill_command`

Do not create or update GitHub issues directly inside this skill.
Always hand off GitHub publication to the configured GitHub issue skill command.

### Pattern 8: naming convention must match across local and github

Use the same logical review name for local files and GitHub issues, derived from `processes.<process>.actions.review.outputs.naming`.

- without milestone: the configured naming pattern with an empty `{milestone_suffix}`
- with milestone: the configured naming pattern with `{milestone_suffix}` expanded to `-<milestone-lowercase>`

Normalize this into:

- local markdown file name: `<normalized-name>.md`
- GitHub issue title: `<logical review name>` with hyphens converted to spaces only if the publication flow requires it

Examples:

- process `tools`, naming `review-tools{milestone_suffix}`, no milestone → file `review-tools.md`, issue title `review tools`
- process `tools`, naming `review-tools{milestone_suffix}`, milestone `M0` → file `review-tools-m0.md`, issue title `review tools m0`

If the GitHub issue already exists, the GitHub issue publication flow must add a comment instead of creating a new issue.

## Review Report Structure

Use this exact structure unless the action config provides a stricter template file.

```markdown
# Review report

## Review status
- Execution mode: DRAFT | OFFICIAL
- Published: NO | LOCAL | GITHUB
- Result: PASS | PASS-WITH-CONCERNS | FAIL
- Blocking: YES | NO

## Review metadata
- Review name:
- Process:
- Project:
- Version:
- Branch:
- Commit hash:
- Commit datetime:
- Milestone:
- Milestone description:
- Milestone scope source:
- Review timestamp:

## Inputs used
- path/to/input
- path/to/guideline
- path/to/milestone

## Dependency check
- Primary inputs:
- Criteria:
- Milestone:
- Expected output:

## Summary
[Short summary of whether the review is ready for the milestone or not]

## Findings
### Errors
- ...

### Warnings
- ...

### Information
- ...

## Final decision
[Explicit statement saying whether the review passes for the active milestone or not]
```

## GitHub Publication Handoff

When `--publish github` is requested, prepare a normalized publication request for the configured GitHub issue publication command containing:

- issue title using the naming convention above
- report body in markdown
- instruction to create the issue if missing
- instruction to add a comment if the issue already exists

Use wording equivalent to:

```text
/sc-gh-issue create-or-comment --title "review tools m0" --body "<report markdown>"
```

The exact syntax may vary according to the configured GitHub issue skill command, but the behavior must remain the same:

- create issue if missing
- add comment if existing

## Local Publication Rules

When `--publish local` is requested:

- write the report to `defaults.local_review_output_dir`
- create the directory if needed
- use the normalized file naming convention
- keep the report in markdown

## Config Contract

Expect `docs/sc-config.yaml` to define processes with review actions using a structure equivalent to:

```yaml
version: 1

defaults:
  mode: draft
  publish: none
  local_review_output_dir: docs/review
  github_issue_skill_command: /sc-gh-issue
  milestone: docs/release_milestone.md

processes:
  tools:
    guidelines:
      - docs/tools_guidelines.md
    actions:
      review:
        inputs:
          primary:
            - tools/
        outputs:
          naming: review-tools{milestone_suffix}
          template_file: none
```

Use the config as the source of truth for:

- input files
- shared guideline files
- milestone file defaults and overrides
- naming patterns
- local output directory
- github publication command

### Config resolution contract

Resolve review settings in this order:

1. `defaults.*`
2. shared process fields from `processes.<process>`
3. action-specific fields from `processes.<process>.actions.review`

At minimum, support:

- `defaults.local_review_output_dir`
- `defaults.github_issue_skill_command`
- `defaults.milestone`
- `processes.<process>.guidelines`
- `processes.<process>.actions.review.inputs.primary`
- `processes.<process>.actions.review.outputs.naming`
- `processes.<process>.actions.review.outputs.template_file`

Interpret `template_file: none` as no template override.
If a future process adds review-specific milestone or additional review inputs, treat them as action overrides layered on top of shared process context and defaults.

## Decision Tree

```text
Missing `processes.<process>.actions.review`? -> stop and report configuration error
Missing required input or criteria? -> fail with blocking dependency result
Milestone configured and not ignored? -> parse milestone title and scope review
Milestone parse fails and scope required? -> fail with blocking dependency result
Milestone parse fails and scope optional? -> continue without milestone scope
--publish github? -> delegate to configured github issue command
--publish local? -> write markdown to configured local review output dir
No publish flag? -> return draft report only
```

## Resources

- **Templates**: See [assets/](assets/) for a review config example and command example
- **Documentation**: See [references/](references/) for implementation notes for OpenCode projects
