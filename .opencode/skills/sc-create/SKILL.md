---
name: sc-create
description: create or modify project artifacts using docs/sc-config.yaml. use when the user invokes /sc-create or @sc-create, and when the user asks to generate, create, add, update, edit, or implement repository files such as bash or shell scripts, c++ code, tools, requirements, architecture, tests, or documentation. first read only docs/sc-config.yaml to decide whether a configured process applies; if matched, load only that process guidelines, relevant action inputs, and milestone context when needed. write by default after required fields are resolved, using a feature branch and pull request instead of modifying main directly.
---

# SC Create

Create or modify process-defined project artifacts using `docs/sc-config.yaml`, shared guidelines, milestone context, and required creation inputs.

## Trigger and Scope

Use this skill when the user invokes `/sc-create` or `@sc-create`, or when the user asks to generate, create, add, update, edit, or implement a repository/project file and the task may match a process in `docs/sc-config.yaml`. This includes natural-language requests that do not mention the skill, such as creating a new bash script, shell script, C++ file, tool, test, requirements document, architecture document, or project documentation.

Use this skill only as a minimal pre-write guard for ordinary edit tasks. Do not load every guideline file. Read the config, determine whether a configured process applies, then load only the files required for that process and action.

## Command Forms

Accept these forms:

```text
/sc-create <process> [--ignore-milestone] [request]
@sc-create <process> [--ignore-milestone] [request]
```

For non-command edit requests, infer the likely process from the target path, file type, requested artifact, and action wording such as generate, create, add, update, edit, implement, or refactor. If no configured process clearly matches, proceed normally without loading process guidelines.


## Natural-Language Trigger Examples

Trigger this skill for requests like:

- "Please generate a new bash script..."
- "Create a shell script that..."
- "Add a C++ tool for..."
- "Implement a new test for..."
- "Update the architecture document..."

For these requests, first read only `docs/sc-config.yaml`. If a configured process such as `tools` has `actions.create.type: Shell script or C++ code`, then shell script and C++ creation requests match that process and its guidelines must be loaded before writing.

## Required Config Shape

Load `docs/sc-config.yaml` first. Expected structure:

```yaml
version: 1

defaults:
  mode: draft
  publish: none
  milestone: docs/release_milestone.md

processes:
  tools:
    guidelines:
      - docs/guidelines/tools_guidelines.md
    actions:
      create:
        type: Shell script or C++ code
```

A process may define additional action fields, including `inputs`, required fields, naming rules, templates, output locations, or constraints. Follow them when present.

## Minimal Loading Rule

Do not read all configured guideline files.

For every task:

1. Read `docs/sc-config.yaml` only.
2. Identify the single best matching process, if any.
3. If no process clearly matches, do not load any guideline files.
4. If one process matches, load only that process's `guidelines`.
5. Resolve only the relevant action, normally `processes.<process>.actions.create`.
6. Load only input files explicitly required by that action or directly needed to complete the request.
7. Never load guidelines or inputs from unrelated processes.

## Workflow

1. Parse the user request.
   - For `/sc-create` or `@sc-create`, extract `<process>`, `--ignore-milestone`, and the freeform request.
   - For ordinary edit/create requests, infer whether the task maps to a process in `docs/sc-config.yaml`.
2. Load `docs/sc-config.yaml`.
3. Determine the matching process.
   - Explicit command process wins.
   - Otherwise match by target path, artifact type, action type, or process name.
   - If no process clearly matches, continue without process guidelines.
4. Resolve `processes.<process>.actions.create`.
5. Load only `processes.<process>.guidelines` for the matched process.
6. Load milestone context only when all of these are true:
   - `--ignore-milestone` was not provided.
   - `defaults.milestone` or an action/process-specific milestone file is configured.
   - The task could be affected by release scope, milestone constraints, naming, or acceptance criteria.
7. Load only action-defined inputs that are required to create or edit the artifact.
8. Gather only missing required fields. Do not ask for information already present in the request, config, guidelines, milestone, or required inputs.
9. Create or modify the requested artifact.
10. Write files by default once required fields are available.
11. Do not create or modify files on the default `main` branch. Create a new branch for the work and create a pull request when finished.
12. End with a brief source-only completion report.

## Process Matching Guidance

Prefer exact, narrow matches over broad ones.

- Match by explicit process name in `/sc-create <process>` or `@sc-create <process>`.
- Match by configured input/output path when the request names a file or directory.
- Match by `actions.create.type` when the requested artifact type clearly corresponds to that action.
- Match by guideline relevance only after reading the config; do not open guideline files just to decide relevance.

If multiple processes could apply, choose the most specific process that matches the file path or artifact type. If ambiguity affects correctness, ask one concise clarification before editing.

## Inputs

If `actions.create.inputs` is configured, load only the declared inputs needed for the task. Inputs may be files, directories, templates, or primary source locations.

If `actions.create.inputs` is absent, use only the files directly named by the user or required by the requested edit.

Do not scan broad directories unless the action explicitly requires it or the requested change cannot be made safely without locating affected files.

## Guidelines

Guideline files are mandatory only after a matching process is identified.

- Load every file listed in `processes.<process>.guidelines` for the matched process.
- Treat guidelines as binding constraints for generation and editing.
- Do not load guideline files from other processes.
- If a listed guideline file is missing, report it and continue only if the task can be completed safely without it.

## Milestone Context

By default, use the milestone file from `defaults.milestone` when it is relevant to the task. Skip milestone loading when:

- The user provides `--ignore-milestone`.
- No milestone file is configured.
- The task is a small mechanical edit unaffected by release scope or milestone constraints.

When loaded, use milestone context to constrain scope, naming, acceptance criteria, compatibility decisions, and what should or should not be included.

## Git and Write Rules

Before writing files, check the current Git branch when a repository is available.

- Never create or modify files directly on the default `main` branch.
- If currently on `main`, create a new feature branch before editing.
- Use a descriptive branch name derived from the process and request.
- After completing the work, create a pull request when the environment and available tools allow it.
- If branch or PR creation is not possible in the current environment, clearly report that limitation.

## Completion Report

At the end of every task, report only the source files used to execute the task:

- Input files read from `actions.create.inputs`.
- Guideline files read from `processes.<process>.guidelines`.
- Milestone/context file read, if used.

Do not list files created or modified. Do not list unrelated files that were not read. Keep the report brief.

Use this format:

```text
Sources used:
- Inputs: <input files or "none">
- Guidelines: <guideline files or "none">
- Milestone: <milestone file or "not used">
```

## Example Behavior

For this config:

```yaml
processes:
  tools:
    guidelines:
      - docs/guidelines/tools_guidelines.md
    actions:
      create:
        type: Shell script or C++ code
```

A request to create or edit a shell script or C++ tool should:

1. Read `docs/sc-config.yaml`.
2. Match the `tools` process.
3. Read only `docs/guidelines/tools_guidelines.md`.
4. Load milestone context only if relevant and not ignored.
5. Read only the directly required input files.
6. Create or edit the tool.
7. Report the sources used, not the files created or modified.
