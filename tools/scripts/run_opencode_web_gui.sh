#!/usr/bin/env bash
set -e

HOSTNAME="0.0.0.0"
PORT="4444"
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
    $0 [--help|-h] [--version|-v]
DESCRIPTION
    Run the OpenCode web UI on the default host and port.
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
    opencode
===============================================================
IMPLEMENTATION
    version         $(print_version)
    project         checkpp
    location        tools/scripts/run_opencode_web_gui.sh
EOF
}

short_help() {
  printf '%s\n' "Run the OpenCode web UI; example: $0; dependency: opencode."
}

if [ "${1:-}" = "--help" ] || [ "${1:-}" = "-h" ]; then
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

exec opencode web --hostname "${HOSTNAME}" --port "${PORT}"
