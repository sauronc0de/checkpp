# Gentle AI — Agent Skills Index

When working on this project, load the relevant skill(s) BEFORE writing any code.

## Mandatory Project Governance

Before any create, edit, implementation, review, or GitHub issue work:

1. Read `docs/sc-config.yaml` first.
2. Use that config to resolve only the process, action, guideline, reference, milestone, or policy paths relevant to the task.
3. If the task matches a configured process, type, action, or path, read only that matching process's required guidelines/references before proceeding.
4. If no relevant process, guideline, reference, milestone, or policy is configured for the task, proceed without loading extra docs.
5. Do not load unrelated guidelines, references, policies, milestones, or process docs.
6. Treat `docs/sc-sdd-policy-contract.md` as the human-readable governance policy when it exists and/or is referenced by `docs/sc-config.yaml`; do not require it unconditionally when config does not reference it.
7. When delegating to sub-agents, include the resolved relevant config, policy, guideline, reference, and milestone paths in the prompt.

## How to Use

1. Check the trigger column to find skills that match your current task
2. Load the skill by reading the SKILL.md file at the listed path
3. Follow ALL patterns and rules from the loaded skill
4. Multiple skills can apply simultaneously

## Skills

| Skill | Trigger | Path |
|-------|---------|------|
| sc-create | User invokes `/sc-create` or `@sc-create` to create artifacts from `docs/sc-config.yaml`. | `file:///workspaces/checkpp/.opencode/skills/sc-create/SKILL.md` |
| sc-gh-issue | User invokes `/sc-gh-issue` or another skill delegates GitHub issue publication; creates an issue or comments on an existing same-topic issue using `gh`. | `file:///workspaces/checkpp/.opencode/skills/sc-gh-issue/SKILL.md` |
| sc-review | User requests `/sc-review <process>` commands, optionally with `--publish github`, `--publish local`, or `--ignore-milestone`. | `file:///workspaces/checkpp/.opencode/skills/sc-review/SKILL.md` |
| sdd-apply | Implement tasks from a change/spec during the apply phase. | `file:///home/dockuser/.config/opencode/skills/sdd-apply/SKILL.md` |
| sdd-archive | Archive a completed change after implementation and verification. | `file:///home/dockuser/.config/opencode/skills/sdd-archive/SKILL.md` |
| sdd-design | Create or update technical design documents for a change. | `file:///home/dockuser/.config/opencode/skills/sdd-design/SKILL.md` |
| sdd-explore | Explore ideas, investigate the codebase, or clarify requirements before committing to a change. | `file:///home/dockuser/.config/opencode/skills/sdd-explore/SKILL.md` |
| sdd-init | Initialize Spec-Driven Development context for the project. | `file:///home/dockuser/.config/opencode/skills/sdd-init/SKILL.md` |
| sdd-propose | Create or update a change proposal. | `file:///home/dockuser/.config/opencode/skills/sdd-propose/SKILL.md` |
| sdd-spec | Write or update specifications for a change. | `file:///home/dockuser/.config/opencode/skills/sdd-spec/SKILL.md` |
| sdd-tasks | Create or update the implementation task breakdown. | `file:///home/dockuser/.config/opencode/skills/sdd-tasks/SKILL.md` |
| sdd-verify | Validate implementation against specs, design, and tasks. | `file:///home/dockuser/.config/opencode/skills/sdd-verify/SKILL.md` |
