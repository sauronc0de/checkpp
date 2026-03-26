# Release Workflow

Use this skill for the repository release flow.

## Purpose

- Keep releases strict and reproducible.
- Use `CMakeLists.txt` as the version source of truth.
- Release only from the default branch.
- If the current branch is not the default branch, stop immediately.
- Bump the version before validation, then commit, push, tag, and publish.

## Input

- The only user choice is the release type: `patch`, `minor`, or `major`.
- The script prompts for it interactively.

## Fixed Assumptions

- Default branch is detected from `origin/HEAD`.
- Remote is `origin`.
- Build preset is `release`.
- Release tag format is `vX.Y.Z` from `project(... VERSION ...)`.

## Required Checks

- Start on the default branch with a clean working tree.
- Pull the latest default branch before validation.
- Build must succeed.
- Build logs must contain no warnings.
- Checker must report `Errors: 0` and `Warnings: 0`.
- Tag must not already exist locally or on the remote.

## Release Sequence

1. Confirm the current branch is the default branch.
2. Prompt for `patch`, `minor`, or `major`.
3. Bump `CMakeLists.txt` to the next version.
4. Pull the latest default branch from the remote.
5. Build the Release preset and run the checker.
6. Commit and push the version bump on the default branch.
7. Create an annotated tag on that commit.
8. Package the executable and dependency files.
9. Create the GitHub release and upload the assets.

## Outputs

- Git tag: `vX.Y.Z`
- GitHub release assets:
  - `checkpp-vX.Y.Z`
  - `CompanyClangTidyModule-vX.Y.Z.so`
  - `rules-vX.Y.Z.yaml`
  - `release-notes-vX.Y.Z.md`
  - `SHA256SUMS`
- A version bump commit on the default branch.

## Entry Point

- Script: `tools/tasks/release.sh`

## Release Notes

- Keep them compact.
- Include every commit subject since the previous release tag, excluding the version-bump commit.
- Do not include long prose or extra commentary.
