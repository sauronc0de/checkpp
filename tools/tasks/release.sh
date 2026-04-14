#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
PRESET="release"
REMOTE="origin"
DEFAULT_BRANCH=""
RELEASE_TOOL="${PROJECT_ROOT}/build/${PRESET}/checkpp-release-tool"

usage() {
  cat <<EOF
Usage: $0

Runs the strict release workflow.
Uses the version already declared in CMakeLists.txt.
Fails if that version tag already exists locally or on the remote.
EOF
}

if [ "${1:-}" = "-h" ] || [ "${1:-}" = "--help" ]; then
  usage
  exit 0
fi

if [ "$#" -gt 0 ]; then
  printf 'This command does not take positional arguments.\n' >&2
  usage >&2
  exit 1
fi

die() {
  printf '\033[31mRelease failed: %s\033[0m\n' "$1" >&2
  exit 1
}

log() {
  printf '%s\n' "$1"
}

require_cmd() {
  command -v "$1" >/dev/null 2>&1 || die "Required command not found: $1"
}

current_branch() {
  git branch --show-current
}

default_branch() {
  local remote_head
  remote_head="$(git symbolic-ref --quiet --short "refs/remotes/${REMOTE}/HEAD" 2>/dev/null || true)"
  if [ -n "$remote_head" ]; then
    printf '%s\n' "${remote_head#${REMOTE}/}"
  else
    printf '%s\n' "main"
  fi
}

project_version() {
  "$RELEASE_TOOL" project-version
}

previous_release_ref() {
  local tag="$1"
  "$RELEASE_TOOL" previous-release-ref "$tag"
}

append_release_notes_section() {
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

build_release_notes_fallback() {
  local notes_path="$1"
  local previous_ref="$2"
  local notes_head="$3"
  local commit_count="$4"
  local short_head
  local subject
  local -a features=()
  local -a fixes=()
  local -a docs=()
  local -a chores=()
  local -a others=()

  short_head="$(git rev-parse --short "$notes_head")"

  cat > "$notes_path" <<EOF
# ${TAG}

Release generated from ${DEFAULT_BRANCH} at ${short_head}.

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

  append_release_notes_section "$notes_path" "Features" "${features[@]}"
  append_release_notes_section "$notes_path" "Fixes" "${fixes[@]}"
  append_release_notes_section "$notes_path" "Documentation" "${docs[@]}"
  append_release_notes_section "$notes_path" "Maintenance" "${chores[@]}"
  append_release_notes_section "$notes_path" "Other Changes" "${others[@]}"
}

extract_md_block() {
  awk '
    /^```md[[:space:]]*$/ { in_block=1; next }
    in_block && /^```[[:space:]]*$/ { exit }
    in_block { print }
  '
}

generate_release_notes_with_ai() {
  local notes_path="$1"
  local previous_ref="$2"
  local notes_head="$3"
  local commit_count="$4"
  local commit_log
  local code_fence='```'
  local prompt
  local ai_output
  local markdown_block

  if ! command -v opencode >/dev/null 2>&1; then
    return 1
  fi

  if [ "$commit_count" -eq 0 ]; then
    build_release_notes_fallback "$notes_path" "$previous_ref" "$notes_head" "$commit_count"
    return 0
  fi

  commit_log="$(git log --no-merges --pretty=format:'- %h %s' "${previous_ref}..${notes_head}")"
  prompt=$(cat <<EOF
Create release notes for checkpp.

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

  markdown_block="$(printf '%s\n' "$ai_output" | extract_md_block)"
  if [ -z "$markdown_block" ]; then
    return 1
  fi

  printf '%s\n' "$markdown_block" > "$notes_path"
}


generate_release_notes() {
  local notes_path="$1"
  local previous_ref="$2"
  local notes_head="$3"
  local commit_count="$4"

  if generate_release_notes_with_ai "$notes_path" "$previous_ref" "$notes_head" "$commit_count"; then
    return 0
  fi

  log "Falling back to deterministic release notes generation"
  build_release_notes_fallback "$notes_path" "$previous_ref" "$notes_head" "$commit_count"
}

prompt_release_type() {
  local release_type="${1:-}"

  if [ -n "$release_type" ]; then
    printf '%s\n' "$release_type"
    return 0
  fi

  if [ ! -r /dev/tty ] || [ ! -w /dev/tty ]; then
    printf '%s\n' "patch"
    return 0
  fi

  while true; do
    printf 'Select release type [patch/minor/major] (default: patch): ' > /dev/tty
    IFS= read -r release_type < /dev/tty || die "Failed to read release type"
    case "$release_type" in
      "")
        printf '%s\n' "patch"
        return 0
        ;;
      patch|minor|major)
        printf '%s\n' "$release_type"
        return 0
        ;;
      *)
        printf 'Please enter patch, minor, or major.\n' >&2
        ;;
    esac
  done
}

bump_version() {
  local release_type="$1"
  "$RELEASE_TOOL" bump-version "$release_type"
}

assert_clean_tree() {
  if [ -n "$(git status --porcelain --untracked-files=all)" ]; then
    die "Working tree must be clean before releasing"
  fi
}

assert_no_warning_lines() {
  local log_file="$1"
  "$RELEASE_TOOL" assert-no-warning-lines "$log_file"
}

run_logged() {
  local log_file="$1"
  shift
  : > "$log_file"
  set +e
  "$@" 2>&1 | tee "$log_file"
  local status=${PIPESTATUS[0]}
  set -e
  return "$status"
}

verify_checker_output() {
  local log_file="$1"
  local summary
  local errors
  local warnings

  summary="$({
    tr -d '\000' < "$log_file" |
      sed -E 's/\x1B\[[0-9;]*[A-Za-z]//g' |
      awk '/Errors:[[:space:]]*[0-9]+/ || /Warnings:[[:space:]]*[0-9]+/'
  } | tail -n 2)"

  errors="$(printf '%s\n' "$summary" | awk '/Errors:[[:space:]]*[0-9]+/ { print $2 }' | tail -n 1)"
  warnings="$(printf '%s\n' "$summary" | awk '/Warnings:[[:space:]]*[0-9]+/ { print $2 }' | tail -n 1)"

  if [ -z "$errors" ] || [ -z "$warnings" ]; then
    printf 'Could not parse checker summary from %s\n' "$log_file" >&2
    return 1
  fi

  if [ "$errors" -ne 0 ] || [ "$warnings" -ne 0 ]; then
    printf 'Checker summary reports Errors=%s Warnings=%s\n' "$errors" "$warnings" >&2
    return 1
  fi
}

require_cmd git
require_cmd cmake
require_cmd gh
require_cmd sha256sum

cd "$PROJECT_ROOT"

if ! gh auth status >/dev/null 2>&1; then
  die "No GitHub authentication. Run 'gh auth login' first."
fi

CURRENT_VERSION="$(project_version)"
TAG="v${CURRENT_VERSION}"

if git ls-remote --exit-code --tags "$REMOTE" "refs/tags/${TAG}" >/dev/null 2>&1; then
  die "Version ${TAG} already pushed to ${REMOTE}"
fi

DEFAULT_BRANCH="$(default_branch)"
CURRENT_BRANCH="$(current_branch)"

if [ "$CURRENT_BRANCH" != "$DEFAULT_BRANCH" ]; then
  die "Release can only run from ${DEFAULT_BRANCH}; current branch is ${CURRENT_BRANCH}"
fi

assert_clean_tree

log "Pulling latest ${DEFAULT_BRANCH} from ${REMOTE}"
git pull --ff-only "$REMOTE" "$DEFAULT_BRANCH"

assert_clean_tree

BUILD_DIR="${PROJECT_ROOT}/build/${PRESET}"
CHECKPP_BIN="${BUILD_DIR}/checkpp"
RULES_PATH="${PROJECT_ROOT}/config/rules.yaml"
RELEASE_STAGE_DIR="${BUILD_DIR}/release-work"
BUILD_LOG="${RELEASE_STAGE_DIR}/build.log"
CHECKER_LOG="${RELEASE_STAGE_DIR}/checker.log"

mkdir -p "$RELEASE_STAGE_DIR"

log "Configuring release build on ${DEFAULT_BRANCH}"
if ! run_logged "$BUILD_LOG" cmake --preset "$PRESET"; then
  die "CMake configure failed (see ${BUILD_LOG})"
fi
assert_no_warning_lines "$BUILD_LOG" || die "Warnings found during configure (see ${BUILD_LOG})"

if ! run_logged "$BUILD_LOG" cmake --build --preset "$PRESET" -j"$(nproc)"; then
  die "Build failed (see ${BUILD_LOG})"
fi
assert_no_warning_lines "$BUILD_LOG" || die "Warnings found during build (see ${BUILD_LOG})"

if [ ! -x "$RELEASE_TOOL" ]; then
  die "Release helper not found: ${RELEASE_TOOL}"
fi

CURRENT_VERSION="$(project_version)"
TAG="v${CURRENT_VERSION}"
RELEASE_DIR="${BUILD_DIR}/release-${TAG}"
ARTIFACT_DIR="${RELEASE_DIR}/assets"
NOTES_PATH="${RELEASE_DIR}/release-notes-${TAG}.md"
SHA_PATH="${RELEASE_DIR}/SHA256SUMS"

mkdir -p "$RELEASE_DIR"

log "Preparing release ${TAG} from version declared in CMakeLists.txt"

if [ ! -x "$CHECKPP_BIN" ]; then
  die "Built binary not found: ${CHECKPP_BIN}"
fi

log "Running checker validation"
if ! run_logged "$CHECKER_LOG" "$CHECKPP_BIN" "$PROJECT_ROOT" "$BUILD_DIR" "$RULES_PATH" --ignore-paths "$PROJECT_ROOT/config/ignore_paths.txt"; then
  die "Checker execution failed (see ${CHECKER_LOG})"
fi
verify_checker_output "$CHECKER_LOG" || die "Checker reported warnings or errors (see ${CHECKER_LOG})"

if git rev-parse --verify --quiet "refs/tags/${TAG}" >/dev/null; then
  die "Tag already exists locally: ${TAG}"
fi

previous_ref="$(previous_release_ref "$TAG")"
notes_head="$(git rev-parse HEAD 2>/dev/null || true)"
commit_count="$(git rev-list --count "${previous_ref}..${notes_head}")"

log "Packaging release artifacts"
mkdir -p "$ARTIFACT_DIR"
cp "$CHECKPP_BIN" "$ARTIFACT_DIR/checkpp-${TAG}"
cp "$PROJECT_ROOT/config/rules.yaml" "$ARTIFACT_DIR/rules-${TAG}.yaml"

release_assets=(
  "$ARTIFACT_DIR/checkpp-${TAG}"
  "$ARTIFACT_DIR/rules-${TAG}.yaml"
)

if [ -f "$PROJECT_ROOT/config/ignore_paths.txt" ]; then
  cp "$PROJECT_ROOT/config/ignore_paths.txt" "$ARTIFACT_DIR/ignore_paths.txt"
  release_assets+=("$ARTIFACT_DIR/ignore_paths.txt")
fi

generate_release_notes "$NOTES_PATH" "$previous_ref" "$notes_head" "$commit_count"

sha256sum \
  "${release_assets[@]}" \
  > "$SHA_PATH"

log "Tagging release ${TAG}"
git tag -a "$TAG" -m "Release ${TAG}"
git push "$REMOTE" "refs/tags/${TAG}"

log "Creating GitHub release ${TAG}"
gh release create "$TAG" \
  --title "$TAG" \
  --notes-file "$NOTES_PATH" \
  "${release_assets[@]}" \
  "$SHA_PATH"

log "Release completed: ${TAG}"
