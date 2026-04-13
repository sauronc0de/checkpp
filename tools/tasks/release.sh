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
if git ls-remote --exit-code --tags "$REMOTE" "refs/tags/${TAG}" >/dev/null 2>&1; then
  die "Tag already exists on ${REMOTE}: ${TAG}"
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

cat > "$NOTES_PATH" <<EOF
# ${TAG}

Release generated from ${DEFAULT_BRANCH} at $(git rev-parse --short HEAD).

## Changes since ${previous_ref}

${commit_count} commit(s)
EOF
if [ "$commit_count" -gt 0 ]; then
  git log --no-merges --pretty=format:'- %s' "${previous_ref}..${notes_head}" >> "$NOTES_PATH"
else
  printf '%s\n' '- No changes' >> "$NOTES_PATH"
fi
printf '\n' >> "$NOTES_PATH"

sha256sum \
  "${release_assets[@]}" \
  "$NOTES_PATH" > "$SHA_PATH"

log "Tagging release ${TAG}"
git tag -a "$TAG" -m "Release ${TAG}"
git push "$REMOTE" "refs/tags/${TAG}"

log "Creating GitHub release ${TAG}"
gh release create "$TAG" \
  --title "$TAG" \
  --notes-file "$NOTES_PATH" \
  "${release_assets[@]}" \
  "$NOTES_PATH" \
  "$SHA_PATH"

log "Release completed: ${TAG}"
