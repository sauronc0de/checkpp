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
RELEASE_CHECKER_BINARY_PATH="${RELEASE_PACKAGE_BINARY_PATH}"
RELEASE_RULES_PATH="${PROJECT_ROOT}/config/rules.yaml"
RELEASE_IGNORE_PATHS_PATH="${PROJECT_ROOT}/config/ignore_paths.txt"
RELEASE_PACKAGE_BINARY_ASSET_NAME="checkpp"
RELEASE_RULES_ASSET_NAME="rules"
RELEASE_IGNORE_PATHS_ASSET_NAME="ignore_paths.txt"

print_version() {
  local version
  version="$(sed -nE 's/^project\(checkpp VERSION ([0-9]+\.[0-9]+\.[0-9]+).*$/\1/p' "${PROJECT_ROOT}/CMakeLists.txt")"
  if [ -z "${version}" ]; then
    printf 'Failed to detect the project version from %s\n' "${PROJECT_ROOT}/CMakeLists.txt" >&2
    exit 1
  fi
  printf '%s\n' "${version}"
}

release_package_extra_assets() {
  local release_dir="$1"
  local artifact_dir="$2"
  local tag="$3"
  local c_family_dst="${artifact_dir}/rules-c-family-${tag}.yaml"
  local c_family_cpp_dst="${artifact_dir}/rules-c-family-cpp-${tag}.yaml"
  : "${release_dir}"

  cp "${PROJECT_ROOT}/config/rules_c_family.yaml" "$c_family_dst"
  cp "${PROJECT_ROOT}/config/rules_c_family_cpp.yaml" "$c_family_cpp_dst"

  printf '%s\n' "$c_family_dst" "$c_family_cpp_dst"
}

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
SYNOPSIS
    $0 [--help|-h] [--version|-v]
DESCRIPTION
    Run the checkpp release entrypoint. This wrapper supplies project-specific
    paths and hooks, then delegates the flow to tools/scripts/release_common.sh.
===============================================================
OPTIONS
    -h, --help                    Print this help.
    -v, --version                 Print the tool version.
===============================================================
PARAMETERS
    none                          This command does not accept positional
                                  parameters or extra options.
===============================================================
EXAMPLES
    $0
===============================================================
DEPENDENCIES
    git, gh, cmake, tools/scripts/release_common.sh,
    build/release/checkpp-release-tool
===============================================================
IMPLEMENTATION
    version         $(print_version)
    project         checkpp
    location        tools/scripts/release.sh
EOF
}

short_help() {
  printf '%s\n' "Run the checkpp release flow; example: $0; dependencies: git, gh, cmake, and build/release/checkpp-release-tool."
}

if [ "${1:-}" = "-h" ] || [ "${1:-}" = "--help" ]; then
  usage
  exit 0
fi

if [ "${1:-}" = "--short-help" ]; then
  short_help
  exit 0
fi

if [ "${1:-}" = "--version" ] || [ "${1:-}" = "-v" ]; then
  print_version
  exit 0
fi

if [ "$#" -gt 0 ]; then
  printf 'This command does not accept positional arguments.\n' >&2
  usage >&2
  exit 1
fi

# shellcheck source=/dev/null
source "$RELEASE_COMMON_SCRIPT"

release_main
