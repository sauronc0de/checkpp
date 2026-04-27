---
name: sc-work
description: work through current github issues for the current repository and milestone. use when the user invokes /sc-work or @sc-work to inspect current milestone issues assigned to the local git user, ignore issues not assigned to that identity, comment blockers through /sc-gh-issue, implement feasible work, use /sc-create when a predefined process applies, and run /sc-review to validate completed work with minimal chat interruption.
metadata:
  version: "0.0.0"
---

# SC Issue Work

Autonomously process current milestone GitHub issues assigned to the local Git identity.

## Command

Support:

```bash
/sc-work [--milestone "<milestone>"] [--limit <n>]
```

Do not invent extra flags.

## Required Resolution

Before changing anything, resolve:

```bash
REPO="$(gh repo view --json nameWithOwner -q .nameWithOwner)"
OWNER="$(gh repo view --json owner -q .owner.login)"
GIT_USER_NAME="$(git config --get user.name || true)"
GIT_USER_EMAIL="$(git config --get user.email || true)"
GH_LOGIN="$(gh api user --jq .login 2>/dev/null || true)"
```

Resolve milestone from `--milestone`; otherwise parse first H1 in `docs/release_milestone.md`:

- `# Release milestone M0 - Signal Mastery Prototype` -> full H1 text after `# `
- `# Release milestone M0` -> `M0`

Stop before writes only if `gh`, `REPO`, milestone, and all self-identity values cannot be resolved, or a dangerous action is required. Require at least one of `GIT_USER_NAME`, `GIT_USER_EMAIL`, or `GH_LOGIN`.

## Issue Selection

List open issues for the milestone:

```bash
gh issue list --repo "$REPO" --state open --milestone "$MILESTONE" --limit "${LIMIT:-100}" --json number,title,url,assignees,milestone
```

For each issue, read full context:

```bash
gh issue view "$NUMBER" --repo "$REPO" --json number,title,url,body,comments,labels,assignees,milestone
```

Process only issues where at least one assignee matches the local identity. Match case-insensitively against available assignee `login`, `name`, or `email` using any resolved value:

- `git config --get user.name`
- `git config --get user.email`
- `gh api user --jq .login`

If no assignee matches any resolved value, ignore the issue without commenting.

Process every matching issue independently.

## Feasibility Decision

A job is feasible when the issue body/comments provide enough information to safely implement or update the repo.

Treat as blocked when required information is missing, acceptance criteria are unclear, dependencies are unavailable, credentials/tools are unavailable, capacity is explicitly unavailable, changes would be dangerous, or the issue is not actionable.

For blocked jobs, add one concise blocker comment by delegating to `/sc-gh-issue create-or-comment` using the issue title/topic. Include what is missing and the next concrete unblock step. Do not implement.

## Work Execution

For feasible jobs:

1. Inspect repo context and nearby conventions.
2. Check whether `docs/sc-config.yaml` defines a matching process and `actions.create`.
3. If a process applies, use `/sc-create <process> <issue request>`.
4. If no process applies, implement directly using project conventions.
5. Run relevant tests, builds, linters, or focused validation.
6. Run `/sc-review <process>` when a matching process exists; otherwise perform a focused self-review.
7. If complete, update the issue with a concise completion comment through `/sc-gh-issue create-or-comment`.
8. If partly complete or newly blocked, comment status and blocker through `/sc-gh-issue create-or-comment`.

Do not ask the user during execution unless resolution failed, a destructive action is needed, or the next step is unsafe.

## GitHub Publication Rules

Use `/sc-gh-issue` for all issue comments or created follow-up tasks.

When commenting or creating, ensure the resulting issue is assigned only to `OWNER` and not to the local identity when different from `OWNER`:

```bash
gh issue edit "$NUMBER" --repo "$REPO" --add-assignee "$OWNER"
for ASSIGNEE in "$GIT_USER_NAME" "$GIT_USER_EMAIL" "$GH_LOGIN"; do
  if [ -n "$ASSIGNEE" ] && [ "$ASSIGNEE" != "$OWNER" ]; then
    gh issue edit "$NUMBER" --repo "$REPO" --remove-assignee "$ASSIGNEE" 2>/dev/null || true
  fi
done
```

For `split-task`, use `/sc-gh-issue split-task` with the parent issue number or URL.

## Safety Rules

Do not merge, push, delete branches, rewrite history, release, deploy, rotate secrets, or run destructive commands unless explicitly requested by the user.

Do not close issues unless the issue explicitly asks for closure and validation passed.

Prefer patches and local commits only when the user or repo convention clearly requires them.

## Output

Keep chat output concise. After processing all selected issues, return:

```text
repo: <repo>
milestone: <milestone>
identity: <git name> <git email> <gh login>
processed: <n>
completed: <numbers>
blocked/commented: <numbers>
skipped: <numbers and reason>
validation: <brief summary>
```
