---
name: sc-gh-issue
description: create or add a concise comment on a GitHub issue in the current repository using gh. use when the user invokes /sc-gh-issue directly or when another skill, such as sc-review, delegates GitHub publication. always resolve a mandatory milestone, check for duplicate issue topics before creating or splitting work, and comment on the existing issue instead of creating a duplicate.
metadata:
  version: "0.0.0"
---

# sc-gh-issue

## Overview

Use this skill to publish concise GitHub issue updates for the current repository.
The skill creates a new issue only when no issue with the same topic already exists.
If a matching issue exists, add a comment to that issue instead of creating a duplicate.

This skill is the GitHub publication target for `sc-review`.

## Workflow

1. Parse the requested command, title, body, optional topic, optional milestone, and optional mode.
2. Resolve the current repository with `gh repo view`.
3. Resolve the mandatory milestone from the explicit request or from `docs/release_milestone.md`.
4. Resolve the repository owner and use that login as the assignee for newly created issues.
5. Normalize the duplicate-detection topic.
6. Search all GitHub issues in the current repository for the same topic before creating an issue or splitting work into a new task.
7. If a matching issue exists, add a concise comment to it.
8. If no matching issue exists, create a concise new issue assigned to the repository owner and attached to the resolved milestone.
9. Return the issue URL, action taken, milestone, assignee, and duplicate-check result.

## Command Contract

Support these command forms:

```bash
/sc-gh-issue create-or-comment --title "<issue title>" --body "<markdown body>"
/sc-gh-issue create-or-comment --title "<issue title>" --body "<markdown body>" --topic "<duplicate topic>"
/sc-gh-issue create-or-comment --title "<issue title>" --body "<markdown body>" --milestone "<milestone title>"
/sc-gh-issue split-task --title "<task title>" --body "<markdown body>" --parent "<issue number or URL>"
```

Rules:

- `create-or-comment` is the default behavior for publication handoffs.
- `split-task` may create a new task issue only after the same duplicate checks pass.
- `--title` is required.
- `--body` is required and must be concise.
- `--topic` is optional; when omitted, use `--title` as the duplicate-detection topic.
- `--milestone` is optional only because it can be resolved from `docs/release_milestone.md`; the final resolved milestone is always mandatory.
- `--parent` is required for `split-task` and must be referenced in the new issue body or existing-issue comment.
- Do not invent unsupported flags.

## Required Metadata

Resolve these values before writing to GitHub:

- repository: `gh repo view --json nameWithOwner -q .nameWithOwner`
- repository owner: `gh repo view --json owner -q .owner.login`
- milestone title: explicit `--milestone`, otherwise the current milestone from `docs/release_milestone.md`
- assignee for new issues: repository owner login
- duplicate topic: normalized `--topic` or `--title`
- issue title: concise title from `--title`
- issue or comment body: concise markdown from `--body`

If any required metadata cannot be resolved, stop and report the blocking missing value.
Do not ask the user to create the issue manually.

## Milestone Resolution Rules

Milestone is mandatory for every new issue.

Resolution priority:

1. Explicit `--milestone "<milestone title>"` from the request.
2. The active milestone in `docs/release_milestone.md`.

When reading `docs/release_milestone.md`, parse the first H1 using either form:

- `# Release milestone M0 - Signal Mastery Prototype`
- `# Release milestone M0`

Extraction rules:

- milestone code = the token after `Release milestone`
- milestone description = the optional text after ` - `
- GitHub milestone title = the full H1 text after `# ` when a description exists, otherwise the milestone code

If the milestone file does not exist, has no parseable first H1, or the resolved milestone is empty, stop before writing to GitHub.

## Duplicate Detection Rules

Duplicate detection is required before creating a new issue or splitting work into a new task.

Normalize topics as follows:

- lowercase
- trim leading and trailing whitespace
- collapse repeated whitespace to one space
- remove markdown heading markers and surrounding quotes
- prefer `--topic` over `--title` when provided

Search all issues, open and closed, in the current repository before creating anything:

```bash
gh issue list --state all --limit 100 --search "repo:<owner>/<repo> in:title <topic>" --json number,title,state,url
```

If needed, run a second broader search over issue bodies and comments:

```bash
gh issue list --state all --limit 100 --search "repo:<owner>/<repo> <topic>" --json number,title,state,url
```

Treat an issue as the same topic when:

- the normalized title equals the normalized topic
- the normalized title contains the normalized topic as a meaningful phrase
- the issue clearly represents the same review publication, finding, task, or split-task target

When a same-topic issue exists:

- do not create a new issue
- add a comment to the best matching issue
- include enough context to explain the update, but keep the comment concise

When multiple matches exist, choose the best title match. If there is no clear best match, stop and report the ambiguous candidates instead of creating a duplicate.

## GitHub Command Forms

Use `gh` for all GitHub reads and writes.
Do not use direct web requests for GitHub data when `gh` can perform the operation.

Repository and owner:

```bash
REPO="$(gh repo view --json nameWithOwner -q .nameWithOwner)"
OWNER="$(gh repo view --json owner -q .owner.login)"
```

Duplicate search:

```bash
gh issue list --repo "$REPO" --state all --limit 100 --search "in:title $TOPIC" --json number,title,state,url
```

Create issue:

```bash
gh issue create --repo "$REPO" --title "$TITLE" --body-file "$BODY_FILE" --milestone "$MILESTONE" --assignee "$OWNER"
```

Add comment to existing issue:

```bash
gh issue comment "$ISSUE_NUMBER" --repo "$REPO" --body-file "$BODY_FILE"
```

Read an existing issue when needed:

```bash
gh issue view "$ISSUE_NUMBER" --repo "$REPO" --json number,title,state,url,body,comments
```

Use `--body-file` for multi-line markdown bodies.
Temporary body files are implementation details and should not be committed.

## Content Rules

Issue and comment content should be concise.

New issue bodies should include:

- one short summary paragraph
- relevant source or review context
- required follow-up, if any
- parent issue reference for `split-task`, if applicable

Comments on existing issues should include:

- one short update sentence
- the new concise report, finding, or task context
- source command or publication context when useful

Do not paste long raw logs unless they are necessary to understand the issue.
Do not ask the user to create, comment, assign, or milestone the issue manually.

## sc-review Compatibility

When `sc-review` delegates GitHub publication, expect a request equivalent to:

```text
/sc-gh-issue create-or-comment --title "review tools m0" --body "<report markdown>"
```

For review publications:

- use the delegated title as both issue title and duplicate topic unless `--topic` is provided
- preserve the review result and final decision in the body
- keep the GitHub issue or comment shorter than the local review when possible
- if the review issue already exists, comment with the latest result instead of creating a new issue

## Decision Tree

```text
Missing title or body? -> stop and ask only for the missing required field
No current GitHub repository from gh? -> stop and report the gh repository error
Explicit milestone provided? -> use it
No explicit milestone? -> parse docs/release_milestone.md
Milestone unresolved? -> stop before writing to GitHub
Owner unresolved? -> stop before writing to GitHub
Duplicate topic provided? -> normalize topic
No duplicate topic? -> normalize title as topic
Same-topic issue found? -> add concise comment to existing issue
Multiple ambiguous same-topic issues found? -> stop and report candidates
No same-topic issue found? -> create concise issue with milestone and owner assignee
```

## Output Draft Rules

Return a concise publication result containing:

- action: `created issue` or `commented on existing issue`
- issue number and URL
- milestone used
- assignee used for new issues
- duplicate topic checked
- duplicate search summary

## Resources

- Review handoff contract: `.opencode/skills/sc-review/SKILL.md`
- Shared config default command: `docs/sc-config.yaml` key `defaults.github_issue_skill_command`
- Default milestone source: `docs/release_milestone.md`
