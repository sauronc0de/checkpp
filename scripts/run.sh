#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

print_version() {
  local version
  version="$(sed -nE 's/^project\(checkpp VERSION ([0-9]+\.[0-9]+\.[0-9]+).*$/\1/p' "${PROJECT_ROOT}/CMakeLists.txt")"
  if [ -z "${version}" ]; then
    printf 'Failed to detect the project version from %s\n' "${PROJECT_ROOT}/CMakeLists.txt" >&2
    exit 1
  fi
  printf '%s\n' "${version}"
}

usage() {
  cat <<EOF
SYNOPSIS
    $0 [--help|-h] [--version|-v] [--build-only] [PRESET]
DESCRIPTION
    Build checkpp with the selected preset, then optionally run the built
    checker against this repository.
===============================================================
OPTIONS
    --build-only                  Configure and build only; skip running the
                                  checker after the build completes.
    -h, --help                    Print this help.
    -v, --version                 Print the tool version.
===============================================================
PARAMETERS
    PRESET                        Optional build preset. Defaults to release.
                                  Example: develop
===============================================================
EXAMPLES
    $0
    $0 develop
    $0 release --build-only
===============================================================
DEPENDENCIES
    cmake, nproc, tee, a configured preset build tree, and the generated
    checkpp binary for run mode
===============================================================
IMPLEMENTATION
    version         $(print_version)
    project         checkpp
    location        scripts/project_specific/run.sh
EOF
}

short_help() {
  printf '%s\n' "Build and optionally run checkpp; example: $0 develop; dependencies: cmake and a configured preset."
}

preset="release"
preset_set="NO"
build_only="NO"

while [ "$#" -gt 0 ]; do
  case "$1" in
    --help|-h)
      usage
      exit 0
      ;;
    --short-help)
      short_help
      exit 0
      ;;
    --version|-v)
      print_version
      exit 0
      ;;
    --build-only)
      build_only="YES"
      ;;
    --*)
      printf 'Unknown option: %s\n' "$1" >&2
      usage >&2
      exit 1
      ;;
    *)
      if [ "${preset_set}" = "YES" ]; then
        printf 'Unexpected positional argument: %s\n' "$1" >&2
        usage >&2
        exit 1
      fi
      preset="$1"
      preset_set="YES"
      ;;
  esac
  shift
done

JOBS="${CMAKE_BUILD_PARALLEL_LEVEL:-$(nproc)}"
build_dir="${PROJECT_ROOT}/build/${preset}"
log_file="${build_dir}/checkpp_style_check.log"

echo "Building project with preset: ${preset}"
cmake --preset "${preset}" || exit 1
cmake --build --preset "${preset}" -j"${JOBS}" || exit 1

if [ "${build_only}" == "YES" ]; then
  echo "Build completed (build-only mode)"
  exit 0
fi

if [ ! -f "${build_dir}/checkpp" ]; then
  echo "Error: checkpp not found at ${build_dir}/checkpp" >&2
  exit 1
fi

echo ""
echo "Running checkpp on itself..."
echo "  Project: ${PROJECT_ROOT}"
echo "  Compile DB: ${build_dir}"
echo "  Rules: ${PROJECT_ROOT}/config/rules.yaml"
if [ "${preset}" = "release" ]; then
  echo "  Ignore paths: ${PROJECT_ROOT}/config/ignore_paths.txt"
else
  echo "  Plugin: ${build_dir}/clang-tidy-module/CompanyClangTidyModule.so"
fi
echo "  Log: ${log_file}"
echo ""

mkdir -p "${build_dir}"

set +e
run_cmd=("${build_dir}/checkpp" "${PROJECT_ROOT}" "${build_dir}" "${PROJECT_ROOT}/config/rules.yaml")
if [ "${preset}" = "release" ]; then
  run_cmd+=(--ignore-paths "${PROJECT_ROOT}/config/ignore_paths.txt")
else
  run_cmd+=(--plugin "${build_dir}/clang-tidy-module/CompanyClangTidyModule.so" --ignore-paths "${PROJECT_ROOT}/config/ignore_paths.txt")
fi
"${run_cmd[@]}" 2>&1 | tee "${log_file}"
tool_exit_code=${PIPESTATUS[0]}
set -e

echo ""
echo "Results saved to: ${log_file}"

exit "${tool_exit_code}"
