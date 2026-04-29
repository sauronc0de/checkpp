# SC-SDD Policy Contract

Policy version: 1.0

This document defines how the project-local SC skills govern Spec-Driven Development (SDD) work in this repository. SDD owns the change lifecycle and implementation artifacts; SC owns repository policy, selective guideline loading, review/publication behavior, and GitHub issue hygiene.

## 1. Responsibilities

- **SDD lifecycle**: use the SDD phases (`explore`, `propose`, `spec`, `design`, `tasks`, `apply`, `verify`, `archive`) to define, implement, verify, and archive substantive changes.
- **SC governance**: use `docs/sc-config.yaml` as the first-read machine contract for create/edit/review/issue work. SC skills resolve the relevant process, action, guidelines, references, milestone, and publication behavior before performing work.
- **SC publication**: GitHub issue creation and comments are centralized in `sc-gh-issue`; review skills and issue-work skills must hand off publication rather than invoking `gh issue create/comment` directly.
- **Local wrappers over upstream SDD**: prefer project-local SC wrapper behavior and metadata over modifying global/upstream SDD skills.

## 2. Routing Rules

- Before any create, edit, implementation, review, or GitHub issue work, read `docs/sc-config.yaml` first.
- If the request matches a configured process, path, type, or action, load only that process/action's configured guidelines, references, inputs, and milestone context.
- If no configured process or action matches, proceed with normal project conventions without loading unrelated guidelines.
- Route substantive implementation/change work through SDD concepts: proposal/spec/design/tasks/apply/verify when the work is larger than a small mechanical edit.
- Use `sc-create` as the repository governance entry point for creating or modifying project artifacts; it may hand off to SDD phases or require SDD metadata when appropriate.
- Use `sc-review` for process-defined reviews and to normalize verification findings into SC severity/status vocabulary.
- Use `sc-work` for issue-driven work; feasible implementation should dispatch into `sc-create`/SDD flow, and blockers/results must be published through `sc-gh-issue`.

## 3. GitHub Issue and Milestone Rules

- `sc-gh-issue` is the single GitHub issue publication endpoint for SC workflows.
- Every GitHub issue publication must resolve a milestone from an explicit input or from the configured milestone file.
- Before creating an issue, search open and closed issues for the same topic/title. If a same-topic issue exists, add a concise comment instead of creating a duplicate.
- Created or updated issues should be assigned to the repository owner according to the `sc-gh-issue` skill; avoid leaving transient agent identities as assignees when they differ from the owner.
- Review publication and blocker/completion comments must be concise and avoid raw long logs unless necessary for actionability.

## 4. Guideline and Reference Resolution

- `docs/sc-config.yaml` is the machine-readable source of truth.
- Do not read all configured guidelines or references by default.
- Resolve the single best matching process/action using explicit command arguments first, then target path, artifact type, action type, or process name.
- Load only the matching process/action's required guidelines, references, declared inputs, and relevant milestone context.
- Treat this policy document as the human-readable governance policy when it is referenced by `docs/sc-config.yaml`.

## 5. SC ↔ SDD Severity and Status Mapping

SC review severities:

- `info`: non-blocking context; maps to SDD informational findings.
- `warning`: non-blocking concern; maps to SDD risks or partial concerns.
- `error`: blocking issue; maps to SDD blocking verification failures.

SC review decisions:

- `pass`: no errors and no blocking SDD verification failures.
- `pass-with-concerns`: warnings/risks exist, but no blocking errors.
- `fail`: at least one error, missing required dependency, or blocking SDD verification failure.

SDD phase status mapping:

- `success` → SC `pass` unless warnings are present.
- `partial` → SC `pass-with-concerns` unless a blocking error is present.
- `blocked` → SC `fail`.

## 6. Required Artifact Metadata

SC-governed SDD artifacts and handoffs should include enough metadata to reproduce scope and publication decisions:

- `policy_version`
- `process`
- `action`
- `change_name` or issue/review topic
- `milestone`
- `guidelines_used`
- `references_used`
- `inputs_used`
- `publication_target` (`none`, `local`, or `github`)
- `github_issue_skill` when GitHub publication is involved
- `validation_sources` when implementation or verification is performed, including the matched process/action/guidelines and any explicit request or SDD artifact instructions that defined validation
- `sc_status` and/or `sdd_status` when reporting review/verification output

## 7. Validation Resolution

Validation is process/action/guideline-driven, not globally tied to one language, build system, or command set. For implementation and verification work, resolve validation in this order:

1. Matched process action validation instructions in `docs/sc-config.yaml`.
2. Matched process validation instructions in `docs/sc-config.yaml`.
3. Matched process guidelines and references.
4. Explicit validation instructions from the request or SDD artifacts.

Reports must identify the validation sources used, checks or commands run when defined, pass/fail/blocker results, and any skipped or unavailable checks with reasons. If no matched process/action/guideline defines validation, report validation as not configured instead of inventing repository-wide defaults.

For example, a C++/CMake process or guideline may require `cmake --preset develop` followed by `cmake --build --preset develop`; a shell-script process may require a syntax check and focused execution examples; a documentation process may require link or formatting checks. These are examples of process-specific validation, not global defaults.

## 8. Delegation and Handoff Rules

- When handing from SC to SDD, pass artifact references, relevant process/action names, policy version, milestone, validation sources, and the exact guideline/reference paths resolved from `docs/sc-config.yaml`.
- When handing from SDD verification back to SC review, preserve SDD status/risks and map findings through the SC severity/status mapping above.
- When handing to GitHub publication, call `sc-gh-issue` with a normalized title/topic, concise body, and resolved milestone; do not publish directly from other skills.
- Sub-agents must save important decisions, discoveries, bug fixes, or config changes to Engram when available.
