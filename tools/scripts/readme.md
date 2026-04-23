# Release scripts

This directory contains the release entrypoint for `checkpp` and the shared shell flow it reuses.

## Files

- `release_common.sh` owns the reusable release flow.
- `release.sh` supplies `checkpp`-specific paths, binaries, and helper functions, then calls the shared flow.

## What the common flow does

`release_common.sh` handles the full release sequence:

1. verifies required tools and GitHub auth
2. checks that the branch is the default release branch and the worktree is clean
3. pulls the latest default branch
4. configures and builds the release preset
5. runs the release checker and rejects warnings or errors
6. packages release assets and SHA sums
7. generates release notes
8. creates and pushes the Git tag
9. publishes the GitHub release

The shared script also provides optional hooks for custom release notes text and extra packaged assets.

## What `release.sh` supplies

`release.sh` is the `checkpp` wrapper. It defines:

- project metadata such as `RELEASE_PROJECT_NAME`, `RELEASE_PROJECT_ROOT`, `RELEASE_REMOTE`, and `RELEASE_PRESET`
- the release helper binary at `build/release/checkpp-release-tool`
- the packaged binary at `build/release/checkpp`
- the checker executable path at `tools/programs/checkpp`
- release assets such as `config/rules.yaml`, the bundled profile variants under `config/`, and `config/ignore_paths.txt`
- project-specific helper functions for version lookup, previous release lookup, log validation, checker execution, and release notes subject

## Shared vs overridden

Shared code in `release_common.sh` owns the workflow and default behavior.

Project code in `release.sh` overrides only the pieces that vary by repository:

- how to read and bump the version
- which checker binary to run
- which assets to package
- how to label the release notes subject

Optional overrides can also supply a custom release notes intro or extra packaged assets.

## Checker integration

The release checker step calls `tools/programs/checkpp` with:

- the project root
- the release build directory
- `config/rules.yaml`
- bundled profile variants such as `config/rules_c_family.yaml`
- `--ignore-paths config/ignore_paths.txt`

The shared flow records the checker output in the release log, then validates that the summary reports `Errors: 0` and `Warnings: 0`.

## Reusing the flow in another project

A future project needs to provide the same wrapper contract:

- a project-specific `release.sh`
- `release_project_version`
- `release_previous_release_ref`
- `release_assert_no_warning_lines`
- `release_run_checker`
- `release_verify_checker_output`
- any required project paths and asset names
- a helper binary compatible with the shared release flow

If the project wants custom release notes or extra artifacts, it can also define `release_release_notes_intro` and `release_package_extra_assets`.
