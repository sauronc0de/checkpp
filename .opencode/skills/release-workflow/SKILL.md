# Release Workflow

Use this skill for the repository release flow.

## Purpose

- Keep releases strict and reproducible.
- Use `CMakeLists.txt` as the only version source of truth.
- Publish a GitHub release from the validated release branch.
- Bump the next development version on the default branch after release.

## Input

- The only user choice is the release type: `patch`, `minor`, or `major`.
- The script prompts for it interactively.

## Fixed Assumptions

- Default branch is detected from `origin/HEAD`.
- Release branch is `release`.
- Remote is `origin`.
- Build preset is `release`.
- Release tag format is `vX.Y.Z` from `project(... VERSION ...)`.

## Required Checks

- Start on the default branch with a clean working tree.
- Pull the latest default branch before building.
- Build must succeed.
- Build logs must contain no warnings.
- Checker must report `Errors: 0` and `Warnings: 0`.
- Tag must not already exist locally or on the remote.

## Release Sequence

1. Confirm the current branch is the default branch.
2. Pull the latest default branch from the remote.
3. Prompt for `patch`, `minor`, or `major`.
4. Build the Release preset and run the checker.
5. Switch to the release branch and merge the validated default branch.
6. Tag the merged commit using the current CMake project version.
7. Package the binary and release files.
8. Push the release branch and tag atomically.
9. Create the GitHub release and upload the assets.
10. Return to the default branch.
11. Bump the CMake project version according to the chosen release type.
12. Commit and push the version bump.

## Outputs

- Git tag: `vX.Y.Z`
- GitHub release assets:
  - `checkpp-vX.Y.Z.tar.gz`
  - `release-notes-vX.Y.Z.md`
  - `SHA256SUMS`
- A version bump commit on the default branch.

## Entry Point

- Script: `tools/tasks/release.sh`
