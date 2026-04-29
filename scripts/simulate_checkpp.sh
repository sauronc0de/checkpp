#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="${WORKSPACE_DIR}"
SCRIPT_NAME="$(basename "$0")"

source_dir=""
source_dir_provided="NO"
build_dir=""
build_dir_provided="NO"
checkpp_bin="${PROJECT_ROOT}/build/release/checkpp"
rules_file="${PROJECT_ROOT}/config/rules_c_family.yaml"
ignore_paths=""
ignore_paths_provided="NO"
positional_source_dir=""
verbose="NO"

print_version() {
  local version
  version="$(sed -nE 's/^project\(checkpp VERSION ([0-9]+\.[0-9]+\.[0-9]+).*$/\1/p' "${PROJECT_ROOT}/CMakeLists.txt")"
  if [ -z "${version}" ]; then
    printf 'Failed to detect the project version from %s\n' "${PROJECT_ROOT}/CMakeLists.txt" >&2
    return 1
  fi
  printf '%s\n' "${version}"
}

usage() {
  cat <<EOF
SYNOPSIS
    ${SCRIPT_NAME} SOURCE_DIR [--help|-h] [--version|-v] [--verbose] [--source_dir DIR] [--build_dir DIR]
DESCRIPTION
    Rebuild and run the C example simulation, then run checkpp against that
    example using the C-family rules.
===============================================================
OPTIONS
    SOURCE_DIR                   Required positional source directory unless
                                 --source_dir is passed.
    --source_dir DIR             C example source directory
                                  (required if SOURCE_DIR is omitted).
    --build_dir DIR              CMake build directory
                                 (default: derived from SOURCE_DIR).
    --checkpp PATH               checkpp executable
                                 (default: ${checkpp_bin}).
    --rules FILE                 checkpp rules file
                                 (default: ${rules_file}).
    --ignore_paths FILE          checkpp ignore-paths file (optional).
    --verbose                    Print progress logs to stderr.
    -h, --help                   Print this help.
    -v, --version                Print the tool version.
===============================================================
EXAMPLES
    ${SCRIPT_NAME} /tmp/example_c
    ${SCRIPT_NAME} --verbose
    ${SCRIPT_NAME} --build_dir /tmp/checkpp-example-c-build
    ${SCRIPT_NAME} --source_dir /tmp/example_c
===============================================================
IMPLEMENTATION
    version         $(print_version)
    project         checkpp
    location        scripts/project_specific/simulate_checkpp.sh
EOF
}

short_help() {
  printf '%s\n' "Rebuild/run the C example and check it with C-family rules; usage: ${SCRIPT_NAME} SOURCE_DIR [--verbose]; deps: cmake, checkpp."
}

log() {
  if [ "${verbose}" = "YES" ]; then
    printf '%s\n' "$1" >&2
  fi
}

require_value() {
  if [ "$#" -lt 2 ] || [ -z "${2:-}" ]; then
    printf 'Missing value for %s.\n' "$1" >&2
    usage >&2
    exit 1
  fi
}

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
    --source_dir)
      require_value "$1" "${2:-}"
      source_dir="$2"
      source_dir_provided="YES"
      shift
      ;;
    --build_dir)
      require_value "$1" "${2:-}"
      build_dir="$2"
      build_dir_provided="YES"
      shift
      ;;
    --checkpp)
      require_value "$1" "${2:-}"
      checkpp_bin="$2"
      shift
      ;;
    --rules)
      require_value "$1" "${2:-}"
      rules_file="$2"
      shift
      ;;
    --ignore_paths)
      require_value "$1" "${2:-}"
      ignore_paths="$2"
      ignore_paths_provided="YES"
      shift
      ;;
    --verbose)
      verbose="YES"
      ;;
    --*)
      printf 'Unknown option: %s\n' "$1" >&2
      usage >&2
      exit 1
      ;;
    *)
      if [ -n "${positional_source_dir}" ]; then
        printf 'Unexpected positional argument: %s\n' "$1" >&2
        usage >&2
        exit 1
      fi
      positional_source_dir="$1"
      ;;
  esac
  shift
done

if [ -n "${positional_source_dir}" ] && [ "${source_dir_provided}" = "NO" ]; then
  source_dir="${positional_source_dir}"
fi

if [ -z "${source_dir}" ]; then
  printf 'Missing source directory. Provide SOURCE_DIR as the first positional argument or use --source_dir DIR.\n' >&2
  usage >&2
  exit 1
fi

if [ "${build_dir_provided}" = "NO" ]; then
  build_dir="${source_dir}/build"
fi

if [ ! -d "${source_dir}" ]; then
  printf 'Source directory does not exist: %s\n' "${source_dir}" >&2
  exit 1
fi

if [ ! -x "${checkpp_bin}" ]; then
  printf 'checkpp executable is missing or not executable: %s\n' "${checkpp_bin}" >&2
  exit 1
fi

if [ ! -f "${rules_file}" ]; then
  printf 'Rules file does not exist: %s\n' "${rules_file}" >&2
  exit 1
fi

if [ "${ignore_paths_provided}" = "YES" ] && [ ! -f "${ignore_paths}" ]; then
  printf 'Ignore-paths file does not exist: %s\n' "${ignore_paths}" >&2
  exit 1
fi

log "Preparing build directory: ${build_dir}"
rm -rf "${build_dir}"
mkdir -p "${build_dir}"

log "Configuring C example"
cmake -S "${source_dir}" -B "${build_dir}" >&2

log "Building C example"
cmake --build "${build_dir}" >&2

app_path=""
for candidate in "${build_dir}"/*; do
  if [ -f "${candidate}" ] && [ -x "${candidate}" ]; then
    if [ -n "${app_path}" ]; then
      printf 'Multiple built executables found in %s; pass --build_dir for an isolated build directory.\n' "${build_dir}" >&2
      exit 1
    fi
    app_path="${candidate}"
  fi
done

if [ ! -x "${app_path}" ]; then
  printf 'Built application is missing or not executable in: %s\n' "${build_dir}" >&2
  exit 1
fi

log "Running C example"
app_status=0
"${app_path}" || app_status=$?

log "Running checkpp"
checkpp_status=0
checkpp_cmd=("${checkpp_bin}" "${source_dir}" "${build_dir}" "${rules_file}")
if [ "${ignore_paths_provided}" = "YES" ]; then
  checkpp_cmd+=(--ignore-paths "${ignore_paths}")
fi
"${checkpp_cmd[@]}" || checkpp_status=$?

if [ "${checkpp_status}" -ne 0 ]; then
  exit "${checkpp_status}"
fi

exit "${app_status}"
