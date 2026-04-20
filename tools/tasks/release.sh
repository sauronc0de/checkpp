#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
RELEASE_COMMON_SCRIPT="${SCRIPT_DIR}/release_common.sh"

if [ ! -r "$RELEASE_COMMON_SCRIPT" ]; then
  printf 'Missing release flow script: %s\n' "$RELEASE_COMMON_SCRIPT" >&2
  exit 1
fi

RELEASE_PROJECT_NAME="checkpp"
RELEASE_PROJECT_ROOT="$PROJECT_ROOT"
RELEASE_REMOTE="origin"
RELEASE_PRESET="release"
RELEASE_VERSION_SOURCE="CMakeLists.txt"
RELEASE_HELPER="${PROJECT_ROOT}/build/${RELEASE_PRESET}/checkpp-release-tool"
RELEASE_PACKAGE_BINARY_PATH="${PROJECT_ROOT}/build/${RELEASE_PRESET}/checkpp"
RELEASE_CHECKER_BINARY_PATH="${PROJECT_ROOT}/tools/programs/checkpp"
RELEASE_RULES_PATH="${PROJECT_ROOT}/config/rules.yaml"
RELEASE_IGNORE_PATHS_PATH="${PROJECT_ROOT}/config/ignore_paths.txt"
RELEASE_PACKAGE_BINARY_ASSET_NAME="checkpp"
RELEASE_RULES_ASSET_NAME="rules"
RELEASE_IGNORE_PATHS_ASSET_NAME="ignore_paths.txt"

release_project_version() {
  "$RELEASE_HELPER" project-version
}

release_previous_release_ref() {
  "$RELEASE_HELPER" previous-release-ref "$1"
}

release_assert_no_warning_lines() {
  "$RELEASE_HELPER" assert-no-warning-lines "$1"
}

release_run_checker() {
  "$RELEASE_CHECKER_BINARY_PATH" "$PROJECT_ROOT" "${PROJECT_ROOT}/build/${RELEASE_PRESET}" "$RELEASE_RULES_PATH" --ignore-paths "$RELEASE_IGNORE_PATHS_PATH"
}

release_verify_checker_output() {
  "$RELEASE_HELPER" verify-checker-output "$1"
}

release_release_notes_subject() {
  printf '%s\n' "checkpp"
}

usage() {
  cat <<EOF
Usage: $0 [patch|minor|major]

checkpp release entrypoint.
This script supplies checkpp-specific paths and hooks, then runs the shared flow in
tools/tasks/release_common.sh.

Without an argument, the script prompts for the release type. The common flow owns
branch checks, build/validation, release notes, checksums, tagging, and GitHub release creation.
EOF
}

if [ "${1:-}" = "-h" ] || [ "${1:-}" = "--help" ]; then
  usage
  exit 0
fi

if [ "$#" -gt 1 ]; then
  printf 'This command accepts at most one positional argument.\n' >&2
  usage >&2
  exit 1
fi

# shellcheck source=/dev/null
source "$RELEASE_COMMON_SCRIPT"

release_main "${1:-}"
