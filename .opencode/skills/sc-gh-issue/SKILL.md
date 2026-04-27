---
name: sc-gh-issue
description: create or update GitHub issues in the current repository using gh. use when the user invokes /sc-gh-issue, or when another skill delegates GitHub issue publication. resolve a mandatory milestone, check for duplicate issue topics, create only when no same-topic issue exists, otherwise add a concise comment to the existing issue.
metadata:
  version: "0.0.0"
---

# sc-gh-issue

Publish concise GitHub issue updates with `gh` in the current repository. Prefer commenting on an existing same-topic issue over creating a duplicate.

## Inputs

Support only:

```bash
/sc-gh-issue create-or-comment --title "<title>" --body "<markdown>" [--topic "<topic>"] [--milestone "<milestone>"]
/sc-gh-issue split-task --title "<title>" --body "<markdown>" --parent "<issue number or url>" [--topic "<topic>"] [--milestone "<milestone>"]
```

Require `--title` and `--body`. Require `--parent` for `split-task`. Do not invent flags.

## Required resolution

Before writing to GitHub, resolve:

```bash
REPO="$(gh repo view --json nameWithOwner -q .nameWithOwner)"
OWNER="$(gh repo view --json owner -q .owner.login)"
```

Resolve milestone from explicit `--milestone`; otherwise parse the first H1 in `docs/release_milestone.md`:

- `# Release milestone M0 - Signal Mastery Prototype` -> full H1 text after `# `
- `# Release milestone M0` -> `M0`

If repository, owner, title, body, parent, or milestone cannot be resolved, stop before writing and report the missing value.

## Duplicate check

Normalize the duplicate topic from `--topic`, otherwise `--title`: lowercase, trim, collapse whitespace, remove heading markers and surrounding quotes.

Search all open and closed issues before creating:

```bash
gh issue list --repo "$REPO" --state all --limit 100 --search "in:title $TOPIC" --json number,title,state,url
gh issue list --repo "$REPO" --state all --limit 100 --search "$TOPIC" --json number,title,state,url
```

Treat an issue as matching when its normalized title equals or meaningfully contains the topic, or clearly represents the same review publication, finding, or task. If multiple plausible matches exist and no best match is clear, stop and report candidates.

## Write behavior

- Matching issue found: add a concise comment with `gh issue comment "$NUMBER" --repo "$REPO" --body-file "$BODY_FILE"` and `gh issue edit "$NUMBER" --repo "$REPO" --add-assignee "$OWNER"`.
  - If the authenticated user is assigned and is not the owner, remove them:
    ```bash
      if [ "$ME" != "$OWNER" ]; then
        gh issue edit "$NUMBER" --repo "$REPO" --remove-assignee "$ME"
      fi
    ```
- No match found: create a concise issue with `gh issue create --repo "$REPO" --title "$TITLE" --body-file "$BODY_FILE" --milestone "$MILESTONE" --assignee "$OWNER"`.
- For `split-task`, include the parent issue reference in the new issue body or comment.
- Use `--body-file` for multiline markdown. Do not commit temporary files.
- Keep issue bodies and comments short; do not paste long raw logs unless necessary.

## Output

Return only:

```text
action: created issue | commented on existing issue
issue: #<number> <url>
milestone: <milestone>
assignee: <owner>
duplicate topic checked: <topic>
duplicate search: <brief summary>
```
