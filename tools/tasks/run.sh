#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

preset="release"
build_only="NO"

if [ "$#" -ge 1 ]; then
  if [ "$1" == "--help" ] || [ "$1" == "-h" ]; then
    echo "Usage: $0 [PRESET] [--build-only]"
    echo "Available presets: release, develop"
    echo "Default preset: release"
    echo ""
    echo "Examples:"
    echo "  $0            # Build release, then run checkpp"
    echo "  $0 develop    # Build develop, then run"
    echo "  $0 release --build-only  # Build release only"
    exit 0
  fi
  preset="$1"
fi

if [ "$#" -ge 2 ] && [ "$2" == "--build-only" ]; then
  build_only="YES"
fi

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
echo "  Plugin: ${build_dir}/clang-tidy-module/CompanyClangTidyModule.so"
echo "  Log: ${log_file}"
echo ""

mkdir -p "${build_dir}"

set +e
"${build_dir}/checkpp" \
  "${PROJECT_ROOT}" \
  "${build_dir}" \
  "${PROJECT_ROOT}/config/rules.yaml" \
  "${build_dir}/clang-tidy-module/CompanyClangTidyModule.so" 2>&1 | tee "${log_file}"
tool_exit_code=${PIPESTATUS[0]}
set -e

echo ""
echo "Results saved to: ${log_file}"

exit "${tool_exit_code}"
