#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -lt 2 ]; then
  echo "Usage: $0 [TARGET] [PRESET]"
  echo "Examples:"
  echo "  $0 all release"
  echo "  $0 all develop"
  echo "  $0 checkpp develop"
  exit 1
fi

target="$1"
preset="$2"

JOBS="${CMAKE_BUILD_PARALLEL_LEVEL:-$(nproc)}"

build_dir="build/${preset}"
mkdir -p "${build_dir}"
log_file="${build_dir}/build.log"

{
echo "Build project"
echo "Date: $(date)"
echo "CMake preset: ${preset}"
echo "Target: ${target}"
echo "Parallel jobs: ${JOBS}"

echo "Configuring (cmake --preset ${preset}) ..."
cmake --preset "${preset}" || exit 1

echo "Building target '${target}' ..."
build_cmd=(cmake --build --preset "${preset}" -j"${JOBS}")
if [ "${target}" != "all" ]; then
  build_cmd+=(--target "${target}")
fi

if command -v /usr/bin/time >/dev/null 2>&1; then
  /usr/bin/time -f "elapsed: %E | user: %U | sys: %S | maxrss: %M KB" "${build_cmd[@]}" || exit 1
else
  SECONDS=0
  "${build_cmd[@]}" || exit 1
  echo "elapsed: ${SECONDS}s"
fi

echo "Build completed successfully"

} > >(tee "${log_file}") 2>&1

first_error_line="$(grep -inm1 'error' "${log_file}" | cut -d: -f1 || true)"
if [ -n "${first_error_line}" ]; then
  echo "First error at ${log_file}:${first_error_line}"
fi
