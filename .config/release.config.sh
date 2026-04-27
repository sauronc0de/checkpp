#!/usr/bin/env bash

RELEASE_PROJECT_NAME="checkpp"
RELEASE_PROJECT_ROOT="$PROJECT_ROOT"
RELEASE_REMOTE="origin"
RELEASE_PRESET="release"
RELEASE_VERSION_SOURCE="CMakeLists.txt"

RELEASE_PACKAGE_BINARY_PATH="${PROJECT_ROOT}/build/${RELEASE_PRESET}/checkpp"
RELEASE_CHECKER_BINARY_PATH="$RELEASE_PACKAGE_BINARY_PATH"

RELEASE_RULES_PATH="${PROJECT_ROOT}/config/rules.yaml"
RELEASE_IGNORE_PATHS_PATH="${PROJECT_ROOT}/config/ignore_paths.txt"

RELEASE_PACKAGE_BINARY_ASSET_NAME="checkpp"
RELEASE_RULES_ASSET_NAME="rules"
RELEASE_IGNORE_PATHS_ASSET_NAME="ignore_paths.txt"

release_config_run_checker() {
  "$RELEASE_CHECKER_BINARY_PATH" \
    "$PROJECT_ROOT" \
    "${PROJECT_ROOT}/build/${RELEASE_PRESET}" \
    "$RELEASE_RULES_PATH" \
    --ignore-paths "$RELEASE_IGNORE_PATHS_PATH"
}

release_config_package_extra_assets() {
  local release_dir="$1"
  local artifact_dir="$2"
  local tag="$3"
  local c_family_dst="${artifact_dir}/rules-c-family.yaml"
  local c_family_cpp_dst="${artifact_dir}/rules-c-family-cpp.yaml"
  : "$release_dir"
  : "$tag"

  cp "${PROJECT_ROOT}/config/rules_c_family.yaml" "$c_family_dst"
  cp "${PROJECT_ROOT}/config/rules_c_family_cpp.yaml" "$c_family_cpp_dst"

  printf '%s\n' "$c_family_dst" "$c_family_cpp_dst"
}

release_config_notes_intro() {
  printf 'Release generated from %s at %s.\n' "$RELEASE_DEFAULT_BRANCH" "$(git rev-parse --short HEAD)"
}