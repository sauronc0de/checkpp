#!/usr/bin/env bash
set -euo pipefail

release_common_script_dir() {
  cd "$(dirname "${BASH_SOURCE[0]}")" && pwd
}

release_common_project_root() {
  cd "$(release_common_script_dir)/../.." && pwd
}

release_common_die() {
  printf '\033[31mRelease failed: %s\033[0m\n' "$1" >&2
  exit 1
}

release_common_usage() {
  cat <<EOF
SYNOPSIS
    source tools/scripts/release_common.sh

DESCRIPTION
    Shared release helpers used by tools/scripts/release.sh.
    The checkpp wrapper owns project-specific paths, helper binaries, and the checker binary.

OPTIONS
    -h, --help          Print this help.
    --short-help        Print a one-line summary.
    -v, --version       Print the tool version.

EXAMPLES
    source tools/scripts/release_common.sh

IMPLEMENTATION
    version         $(release_common_print_version)
    location        tools/scripts/release_common.sh
EOF
}

release_common_short_help() {
  printf '%s\n' 'Shared release helpers for release.sh; example: source tools/scripts/release_common.sh.'
}

release_common_print_version() {
  local version_file
  local version

  version_file="$(release_common_project_root)/CMakeLists.txt"
  version="$(sed -nE 's/^project\(checkpp VERSION ([0-9]+\.[0-9]+\.[0-9]+).*$/\1/p' "$version_file")"
  if [ -z "$version" ]; then
    printf 'Failed to detect the project version from %s\n' "$version_file" >&2
    return 1
  fi

  printf '%s\n' "$version"
}

release_common_log() {
  printf '%s\n' "$1"
}

release_common_require_cmd() {
  command -v "$1" >/dev/null 2>&1 || release_common_die "Required command not found: $1"
}

release_common_current_branch() {
  git branch --show-current
}

release_common_default_branch() {
  local remote_head
  remote_head="$(git symbolic-ref --quiet --short "refs/remotes/${RELEASE_REMOTE}/HEAD" 2>/dev/null || true)"
  if [ -n "$remote_head" ]; then
    printf '%s\n' "${remote_head#${RELEASE_REMOTE}/}"
  else
    printf '%s\n' "main"
  fi
}

release_common_release_tag() {
  printf 'v%s\n' "$(release_project_version)"
}

release_common_previous_release_ref() {
  local tag="$1"
  release_previous_release_ref "$tag"
}

release_common_append_section() {
  local notes_path="$1"
  local title="$2"
  shift 2

  if [ "$#" -eq 0 ]; then
    return 0
  fi

  printf '## %s\n\n' "$title" >> "$notes_path"
  local item
  for item in "$@"; do
    printf -- '- %s\n' "$item" >> "$notes_path"
  done
  printf '\n' >> "$notes_path"
}

release_common_notes_subject_from_config() {
  if declare -F release_release_notes_subject >/dev/null 2>&1; then
    release_release_notes_subject
    return 0
  fi

  printf '%s\n' "${RELEASE_PROJECT_NAME}"
}

release_common_notes_intro_from_config() {
  if declare -F release_release_notes_intro >/dev/null 2>&1; then
    release_release_notes_intro
    return 0
  fi

  printf 'Release generated from %s at %s.\n' "${RELEASE_DEFAULT_BRANCH}" "$(git rev-parse --short HEAD)"
}

release_common_build_release_notes_fallback() {
  local notes_path="$1"
  local previous_ref="$2"
  local notes_head="$3"
  local commit_count="$4"
  local tag="$5"
  local subject
  local -a features=()
  local -a fixes=()
  local -a docs=()
  local -a chores=()
  local -a others=()

  cat > "$notes_path" <<EOF
# ${tag}

$(release_common_notes_intro_from_config)

## Overview

- ${commit_count} commit(s) since ${previous_ref}

EOF

  if [ "$commit_count" -eq 0 ]; then
    printf '## Changes\n\n- No changes\n' >> "$notes_path"
    return 0
  fi

  while IFS= read -r subject; do
    [ -n "$subject" ] || continue
    case "$subject" in
      feat:*|add:*) features+=("${subject#*: }") ;;
      fix:*|bugfix:*) fixes+=("${subject#*: }") ;;
      docs:*) docs+=("${subject#*: }") ;;
      chore:*|refactor:*|build:*|ci:*|test:*) chores+=("${subject#*: }") ;;
      *) others+=("$subject") ;;
    esac
  done < <(git log --no-merges --pretty=format:'%s' "${previous_ref}..${notes_head}")

  release_common_append_section "$notes_path" "Features" "${features[@]}"
  release_common_append_section "$notes_path" "Fixes" "${fixes[@]}"
  release_common_append_section "$notes_path" "Documentation" "${docs[@]}"
  release_common_append_section "$notes_path" "Maintenance" "${chores[@]}"
  release_common_append_section "$notes_path" "Other Changes" "${others[@]}"
}

release_common_extract_md_block() {
  awk '
    /^```md[[:space:]]*$/ { in_block=1; next }
    in_block && /^```[[:space:]]*$/ { exit }
    in_block { print }
  '
}

release_common_generate_release_notes_with_ai() {
  local notes_path="$1"
  local previous_ref="$2"
  local notes_head="$3"
  local commit_count="$4"
  local tag="$5"
  local commit_log
  local code_fence='```'
  local prompt
  local ai_output
  local markdown_block

  if ! command -v opencode >/dev/null 2>&1; then
    return 1
  fi

  if [ "$commit_count" -eq 0 ]; then
    release_common_build_release_notes_fallback "$notes_path" "$previous_ref" "$notes_head" "$commit_count" "$tag"
    return 0
  fi

  commit_log="$(git log --no-merges --pretty=format:'- %h %s' "${previous_ref}..${notes_head}")"
  prompt=$(cat <<EOF
Create release notes for ${RELEASE_PROJECT_NAME}.

Compare commits from ${previous_ref} to ${notes_head}.
Return ONLY one fenced markdown block using this exact fence style:

${code_fence}md
...
${code_fence}

Rules:
- group related commits together
- rewrite raw commit messages into clear user-facing release notes
- avoid mentioning internal noise unless it matters for users
- keep the output concise but useful
- include Overview, Highlights, and Notable Fixes sections when applicable
- if there are no meaningful changes for a section, omit that section

Commits:
${commit_log}
EOF
)

  if ! ai_output="$(opencode run "$prompt")"; then
    return 1
  fi

  markdown_block="$(printf '%s\n' "$ai_output" | release_common_extract_md_block)"
  if [ -z "$markdown_block" ]; then
    return 1
  fi

  printf '%s\n' "$markdown_block" > "$notes_path"
}

release_common_generate_release_notes() {
  local notes_path="$1"
  local previous_ref="$2"
  local notes_head="$3"
  local commit_count="$4"
  local tag="$5"

  if release_common_generate_release_notes_with_ai "$notes_path" "$previous_ref" "$notes_head" "$commit_count" "$tag"; then
    return 0
  fi

  release_common_log "Falling back to deterministic release notes generation"
  release_common_build_release_notes_fallback "$notes_path" "$previous_ref" "$notes_head" "$commit_count" "$tag"
}

release_common_run_logged() {
  local log_file="$1"
  shift
  : > "$log_file"
  set +e
  "$@" 2>&1 | tee "$log_file"
  local status=${PIPESTATUS[0]}
  set -e
  return "$status"
}

release_common_assert_clean_tree() {
  if [ -n "$(git status --porcelain --untracked-files=all)" ]; then
    release_common_die "Working tree must be clean before releasing"
  fi
}

release_common_assert_no_warning_lines() {
  local log_file="$1"
  release_assert_no_warning_lines "$log_file"
}

release_common_run_checker() {
  local log_file="$1"
  if [ ! -x "$RELEASE_CHECKER_BINARY_PATH" ]; then
    release_common_die "Checker binary not found: ${RELEASE_CHECKER_BINARY_PATH}"
  fi

  release_run_checker "$log_file"
}

release_common_verify_checker_output() {
  local log_file="$1"
  release_verify_checker_output "$log_file"
}

release_common_copy_asset() {
  local src="$1"
  local dst="$2"

  cp "$src" "$dst"
}

release_common_package_assets() {
  local release_dir="$1"
  local artifact_dir="$2"
  local tag="$3"
  local -a release_assets=()
  local extra_assets_output

  mkdir -p "$artifact_dir"

  release_common_copy_asset "$RELEASE_PACKAGE_BINARY_PATH" "$artifact_dir/${RELEASE_PACKAGE_BINARY_ASSET_NAME}"
  release_common_copy_asset "$RELEASE_RULES_PATH" "$artifact_dir/${RELEASE_RULES_ASSET_NAME}-${tag}.yaml"

  release_assets+=("$artifact_dir/${RELEASE_PACKAGE_BINARY_ASSET_NAME}")
  release_assets+=("$artifact_dir/${RELEASE_RULES_ASSET_NAME}-${tag}.yaml")

  if [ -n "${RELEASE_IGNORE_PATHS_PATH:-}" ] && [ -f "$RELEASE_IGNORE_PATHS_PATH" ]; then
    release_common_copy_asset "$RELEASE_IGNORE_PATHS_PATH" "$artifact_dir/${RELEASE_IGNORE_PATHS_ASSET_NAME}"
    release_assets+=("$artifact_dir/${RELEASE_IGNORE_PATHS_ASSET_NAME}")
  fi

  if declare -F release_package_extra_assets >/dev/null 2>&1; then
    extra_assets_output="$(release_package_extra_assets "$release_dir" "$artifact_dir" "$tag")"
    if [ -n "$extra_assets_output" ]; then
      while IFS= read -r line; do
        [ -n "$line" ] || continue
        release_assets+=("$line")
      done <<EOF
$extra_assets_output
EOF
    fi
  fi

  printf '%s\n' "${release_assets[@]}"
}

release_main() {
  local current_branch
  local tag
  local build_dir
  local release_dir
  local artifact_dir
  local notes_path
  local sha_path
  local build_log
  local checker_log
  local previous_ref
  local notes_head
  local commit_count
  local -a release_assets=()

  release_common_require_cmd git
  release_common_require_cmd cmake
  release_common_require_cmd gh
  release_common_require_cmd sha256sum

  if ! gh api user >/dev/null 2>&1; then
    release_common_die "GitHub auth not working. Please run 'gh auth login' to authenticate or add GH_TOKEN to the environment."
  fi

  cd "$(release_common_project_root)"

  tag="$(release_common_release_tag)"
  current_branch="$(release_common_current_branch)"
  RELEASE_DEFAULT_BRANCH="$(release_common_default_branch)"

  if git ls-remote --exit-code --tags "$RELEASE_REMOTE" "refs/tags/${tag}" >/dev/null 2>&1; then
    release_common_die "Version ${tag} already pushed to ${RELEASE_REMOTE}"
  fi

  if [ "$current_branch" != "$RELEASE_DEFAULT_BRANCH" ]; then
    release_common_die "Release can only run from ${RELEASE_DEFAULT_BRANCH}; current branch is ${current_branch}"
  fi

  release_common_assert_clean_tree

  release_common_log "Pulling latest ${RELEASE_DEFAULT_BRANCH} from ${RELEASE_REMOTE}"
  git pull --ff-only "$RELEASE_REMOTE" "$RELEASE_DEFAULT_BRANCH"

  release_common_assert_clean_tree

  build_dir="${RELEASE_BUILD_DIR:-${RELEASE_PROJECT_ROOT}/build/${RELEASE_PRESET}}"
  release_dir="${RELEASE_RELEASE_DIR:-${build_dir}/release-${tag}}"
  artifact_dir="${RELEASE_ARTIFACT_DIR:-${release_dir}/assets}"
  notes_path="${RELEASE_NOTES_PATH:-${release_dir}/release-notes-${tag}.md}"
  sha_path="${RELEASE_SHA_PATH:-${release_dir}/SHA256SUMS}"
  build_log="${RELEASE_BUILD_LOG:-${release_dir}/build.log}"
  checker_log="${RELEASE_CHECKER_LOG:-${release_dir}/checker.log}"

  mkdir -p "$release_dir"

  release_common_log "Configuring release build on ${RELEASE_DEFAULT_BRANCH}"
  if ! release_common_run_logged "$build_log" cmake --preset "$RELEASE_PRESET"; then
    release_common_die "CMake configure failed (see ${build_log})"
  fi
  release_common_assert_no_warning_lines "$build_log" || release_common_die "Warnings found during configure (see ${build_log})"

  if ! release_common_run_logged "$build_log" cmake --build --preset "$RELEASE_PRESET" -j"$(nproc)"; then
    release_common_die "Build failed (see ${build_log})"
  fi
  release_common_assert_no_warning_lines "$build_log" || release_common_die "Warnings found during build (see ${build_log})"

  if [ ! -x "$RELEASE_HELPER" ]; then
    release_common_die "Release helper not found: ${RELEASE_HELPER}"
  fi

  release_common_log "Preparing release ${tag} from version declared in ${RELEASE_VERSION_SOURCE}"

  if [ ! -x "$RELEASE_PACKAGE_BINARY_PATH" ]; then
    release_common_die "Built binary not found: ${RELEASE_PACKAGE_BINARY_PATH}"
  fi

  release_common_log "Running checker validation"
  if ! release_common_run_logged "$checker_log" release_common_run_checker "$checker_log"; then
    release_common_die "Checker execution failed (see ${checker_log})"
  fi
  release_common_verify_checker_output "$checker_log" || release_common_die "Checker reported warnings or errors (see ${checker_log})"

  if git rev-parse --verify --quiet "refs/tags/${tag}" >/dev/null; then
    release_common_die "Tag already exists locally: ${tag}"
  fi

  previous_ref="$(release_common_previous_release_ref "$tag")"
  notes_head="$(git rev-parse HEAD 2>/dev/null || true)"
  commit_count="$(git rev-list --count "${previous_ref}..${notes_head}")"

  release_common_log "Packaging release artifacts"
  mapfile -t release_assets < <(release_common_package_assets "$release_dir" "$artifact_dir" "$tag")

  release_common_generate_release_notes "$notes_path" "$previous_ref" "$notes_head" "$commit_count" "$tag"

  sha256sum \
    "${release_assets[@]}" \
    > "$sha_path"

  release_common_log "Tagging release ${tag}"
  git tag -a "$tag" -m "Release ${tag}"
  git push "$RELEASE_REMOTE" "refs/tags/${tag}"

  release_common_log "Creating GitHub release ${tag}"
  gh release create "$tag" \
    --title "$tag" \
    --notes-file "$notes_path" \
    "${release_assets[@]}" \
    "$sha_path"

  release_common_log "Release completed: ${tag}"
}

if [ "${BASH_SOURCE[0]}" = "$0" ]; then
  case "${1:-}" in
    -h|--help)
      release_common_usage
      exit 0
      ;;
    --short-help)
      release_common_short_help
      exit 0
      ;;
    -v|--version)
      release_common_print_version
      exit 0
      ;;
    "")
      release_common_usage >&2
      exit 1
      ;;
    *)
      release_common_usage >&2
      exit 1
      ;;
  esac
fi
