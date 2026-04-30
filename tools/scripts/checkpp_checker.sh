#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_DIR="${WORKSPACE_DIR:-$(cd "${SCRIPT_DIR}/../.." && pwd)}"
PRESETS_FILE="${WORKSPACE_DIR}/CMakePresets.json"
CHECKPP_TEST_PRESET="${CHECKPP_CHECKER_TEST_PRESET:-checkpp-tests}"
CHECKPP_RELEASE_BINARY="${WORKSPACE_DIR}/build/release/checkpp"
CHECKPP_RULES_FILE="${WORKSPACE_DIR}/config/rules.yaml"
CHECKPP_IGNORE_PATHS_FILE="${WORKSPACE_DIR}/config/ignore_paths.txt"
CHECKPP_LOG_FILE="${WORKSPACE_DIR}/build/checkpp.log"

ensure_log_file() {
  local log_dir
  log_dir="$(dirname "${CHECKPP_LOG_FILE}")"

  if ! mkdir -p "${log_dir}" 2>/dev/null; then
    printf 'Checker log setup failed:\n' >&2
    printf -- '- %s\n' "Cannot create log directory: ${log_dir}" >&2
    return 1
  fi

  if [ ! -d "${log_dir}" ] || [ ! -w "${log_dir}" ] || [ ! -x "${log_dir}" ]; then
    printf 'Checker log setup failed:\n' >&2
    printf -- '- %s\n' "Log directory is not usable: ${log_dir}" >&2
    return 1
  fi

  if ! : >"${CHECKPP_LOG_FILE}"; then
    printf 'Checker log setup failed:\n' >&2
    printf -- '- %s\n' "Cannot write log file: ${CHECKPP_LOG_FILE}" >&2
    return 1
  fi
}

extract_error_detail() {
  local log_file="$1"
  local line

  while IFS= read -r line; do
    if [[ "${line}" =~ ([^[:space:]]+:[0-9]+(:[0-9]+)?:[[:space:]].*) ]]; then
      printf '%s\n' "${BASH_REMATCH[1]}"
      return 0
    fi
  done <"${log_file}"

  while IFS= read -r line; do
    if [[ "${line}" == *"error:"* ]]; then
      printf '%s\n' "${line}"
      return 0
    fi
  done <"${log_file}"

  printf '%s\n' "See log for details: ${log_file}"
}

discover_build_presets() {
  if [ -n "${CHECKPP_CHECKER_PRESETS:-}" ]; then
    printf '%s\n' "${CHECKPP_CHECKER_PRESETS}"
    return 0
  fi

  local list_output
  if ! list_output="$(cmake --list-presets 2>/dev/null)"; then
    return 1
  fi

  local names=()
  local line
  while IFS= read -r line; do
    if [[ "${line}" =~ ^[[:space:]]*\"([^\"]+)\"[[:space:]]*- ]]; then
      names+=("${BASH_REMATCH[1]}")
    fi
  done <<<"${list_output}"

  printf '%s\n' "${names[*]}"
}

run_build_for_preset() {
  local preset="$1"

  if cmake --preset "${preset}" >>"${CHECKPP_LOG_FILE}" 2>&1 && cmake --build --preset "${preset}" >>"${CHECKPP_LOG_FILE}" 2>&1; then
    printf 'Build preset %s ok\n' "${preset}"
    return 0
  fi

  printf 'Build preset %s failed:\n' "${preset}" >&2
  printf -- '- %s\n' "$(extract_error_detail "${CHECKPP_LOG_FILE}")" >&2
  printf -- '- %s\n' "Log: ${CHECKPP_LOG_FILE}" >&2
  return 1
}

run_ctest_preset() {
  local test_preset="$1"

  if ctest --preset "${test_preset}" >>"${CHECKPP_LOG_FILE}" 2>&1; then
    printf 'CTest preset %s ok\n' "${test_preset}"
    return 0
  fi

  printf 'CTest preset %s failed:\n' "${test_preset}" >&2
  printf -- '- %s\n' "$(extract_error_detail "${CHECKPP_LOG_FILE}")" >&2
  printf -- '- %s\n' "Log: ${CHECKPP_LOG_FILE}" >&2
  return 1
}

resolve_checkpp_command() {
  if [ -x "${CHECKPP_RELEASE_BINARY}" ]; then
    printf '%s\n' "${CHECKPP_RELEASE_BINARY}"
    return 0
  fi

  printf '%s\n' "checkpp"
}

run_code_check() {
  local checkpp_cmd
  checkpp_cmd="$(resolve_checkpp_command)"
  local errors_count=''
  local warnings_count=''
  local line

  "${checkpp_cmd}" "${WORKSPACE_DIR}" "${WORKSPACE_DIR}/build/release" "${CHECKPP_RULES_FILE}" --ignore-paths "${CHECKPP_IGNORE_PATHS_FILE}" >>"${CHECKPP_LOG_FILE}" 2>&1 || true

  while IFS= read -r line; do
    if [[ "${line}" =~ ^[[:space:]]*Errors:[[:space:]]*([0-9]+)[[:space:]]*$ ]]; then
      errors_count="${BASH_REMATCH[1]}"
    elif [[ "${line}" =~ ^[[:space:]]*Warnings:[[:space:]]*([0-9]+)[[:space:]]*$ ]]; then
      warnings_count="${BASH_REMATCH[1]}"
    fi
  done <"${CHECKPP_LOG_FILE}"

  if [ -z "${errors_count}" ] || [ -z "${warnings_count}" ]; then
    printf 'Code check failed:\n' >&2
    printf -- '- %s\n' 'Unable to parse check summary (Errors/Warnings).' >&2
    printf -- '- %s\n' "Log: ${CHECKPP_LOG_FILE}" >&2
    return 1
  fi

  if [ "${errors_count}" -ne 0 ] || [ "${warnings_count}" -ne 0 ]; then
    printf 'Code check failed:\n' >&2
    printf -- '- %s\n' "Errors: ${errors_count}, Warnings: ${warnings_count}" >&2
    printf -- '- %s\n' "Log: ${CHECKPP_LOG_FILE}" >&2
    return 1
  fi

  printf 'Code check ok\n'
  return 0

}

main() {
  ensure_log_file

  local presets_line
  if ! presets_line="$(discover_build_presets)"; then
    printf 'Build presets failed:\n' >&2
    printf -- '- %s\n' 'Unable to list CMake presets with `cmake --list-presets`.' >&2
    exit 1
  fi

  if [ -z "${presets_line// /}" ]; then
    printf 'Build presets failed:\n' >&2
    printf -- '- %s\n' 'No presets found from `cmake --list-presets`.' >&2
    exit 1
  fi

  local presets=()
  # shellcheck disable=SC2206
  presets=(${presets_line})

  local preset
  for preset in "${presets[@]}"; do
    run_build_for_preset "${preset}"
  done

  run_ctest_preset "${CHECKPP_TEST_PRESET}"
  run_code_check
}

main "$@"
