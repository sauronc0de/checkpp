#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
DEFAULT_BRANCH="main"
RELEASE_BRANCH="release"
REMOTE="origin"
PRESET="release"

usage() {
  cat <<EOF
Usage: $0 [--help]

Runs the strict release workflow.
You will be prompted to choose patch, minor, or major release type.
EOF
}

if [ "$#" -gt 0 ]; then
  case "$1" in
    -h|--help)
      usage
      exit 0
      ;;
    *)
      printf 'Unknown argument: %s\n' "$1" >&2
      usage >&2
      exit 1
      ;;
  esac
fi

die() {
  printf 'Release failed: %s\n' "$1" >&2
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

project_version() {
  python3 - <<'PY'
from pathlib import Path
import re
import sys

text = Path('CMakeLists.txt').read_text(encoding='utf-8')
match = re.search(r'project\(cpp_style_tool\s+VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)\b', text)
if not match:
    sys.exit('could not find project version in CMakeLists.txt')
print(match.group(1))
PY
}

previous_release_ref() {
  local tag="$1"
  python3 - "$tag" <<'PY'
import subprocess
import sys

tag = sys.argv[1]
try:
    result = subprocess.run(
        ['git', 'describe', '--tags', '--abbrev=0', '--match', 'v[0-9]*', f'{tag}^'],
        check=True,
        capture_output=True,
        text=True,
    )
    print(result.stdout.strip())
except subprocess.CalledProcessError:
    result = subprocess.run(
        ['git', 'rev-list', '--max-parents=0', 'HEAD'],
        check=True,
        capture_output=True,
        text=True,
    )
    print(result.stdout.strip())
PY
}

choose_release_type() {
  if [ ! -t 0 ]; then
    die "Interactive terminal required to choose patch, minor, or major"
  fi

  while true; do
    printf 'Select release type [patch/minor/major]: '
    IFS= read -r release_type || die "Failed to read release type"
    case "${release_type}" in
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

assert_clean_tree() {
  if [ -n "$(git status --porcelain --untracked-files=all)" ]; then
    die "Working tree must be clean before releasing"
  fi
}

assert_no_warning_lines() {
  local log_file="$1"
  python3 - "$log_file" <<'PY'
from pathlib import Path
import re
import sys

path = Path(sys.argv[1])
warning_lines = []
for line in path.read_text(encoding='utf-8', errors='replace').splitlines():
    if re.search(r'\bwarning\b', line, re.IGNORECASE):
        warning_lines.append(line)

if warning_lines:
    print('warning(s) found in build log:', file=sys.stderr)
    for line in warning_lines[:20]:
        print(line, file=sys.stderr)
    raise SystemExit(1)
PY
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
  python3 - "$log_file" <<'PY'
from pathlib import Path
import re
import sys

text = Path(sys.argv[1]).read_text(encoding='utf-8', errors='replace')
text = re.sub(r'\x1b\[[0-9;]*m', '', text)

errors = re.search(r'^Errors:\s+([0-9]+)', text, re.MULTILINE)
warnings = re.search(r'^Warnings:\s+([0-9]+)', text, re.MULTILINE)

if errors is None or warnings is None:
    print('could not parse checker summary', file=sys.stderr)
    raise SystemExit(1)

if int(errors.group(1)) != 0 or int(warnings.group(1)) != 0:
    print(f'checker reported errors={errors.group(1)} warnings={warnings.group(1)}', file=sys.stderr)
    raise SystemExit(1)
PY
}

bump_version() {
  local release_type="$1"
  python3 - "$release_type" <<'PY'
from pathlib import Path
import re
import sys

release_type = sys.argv[1]
path = Path('CMakeLists.txt')
text = path.read_text(encoding='utf-8')
pattern = re.compile(r'(project\(cpp_style_tool\s+VERSION\s+)([0-9]+)\.([0-9]+)\.([0-9]+)(\s+LANGUAGES\s+C\s+CXX\))')
match = pattern.search(text)
if not match:
    raise SystemExit('could not find project version in CMakeLists.txt')

major, minor, patch = map(int, match.group(2, 3, 4))
if release_type == 'patch':
    patch += 1
elif release_type == 'minor':
    minor += 1
    patch = 0
elif release_type == 'major':
    major += 1
    minor = 0
    patch = 0
else:
    raise SystemExit(f'invalid release type: {release_type}')

new_version = f'{major}.{minor}.{patch}'
path.write_text(pattern.sub(rf'\1{new_version}\5', text, count=1), encoding='utf-8')
print(new_version)
PY
}

require_cmd git
require_cmd cmake
require_cmd gh
require_cmd python3
require_cmd tar
require_cmd sha256sum

cd "$PROJECT_ROOT"

DEFAULT_REMOTE_HEAD="$(git symbolic-ref --quiet --short "refs/remotes/${REMOTE}/HEAD" 2>/dev/null || true)"
if [ -n "$DEFAULT_REMOTE_HEAD" ]; then
  DEFAULT_BRANCH="${DEFAULT_REMOTE_HEAD#${REMOTE}/}"
fi

CURRENT_BRANCH="$(current_branch)"
if [ "$CURRENT_BRANCH" != "$DEFAULT_BRANCH" ]; then
  die "Run this script from the default branch (${DEFAULT_BRANCH}), not ${CURRENT_BRANCH}"
fi

assert_clean_tree

log "Pulling latest ${DEFAULT_BRANCH} from ${REMOTE}"
git pull --ff-only "$REMOTE" "$DEFAULT_BRANCH"
assert_clean_tree

RELEASE_TYPE="$(choose_release_type)"
VERSION="$(project_version)"
TAG="v${VERSION}"
BUILD_DIR="${PROJECT_ROOT}/build/${PRESET}"
CHECKPP_BIN="${BUILD_DIR}/checkpp"
PLUGIN_PATH="${BUILD_DIR}/clang-tidy-module/CompanyClangTidyModule.so"
RULES_PATH="${PROJECT_ROOT}/config/rules.yaml"
RELEASE_DIR="${BUILD_DIR}/release-${TAG}"
ARTIFACT_DIR="${RELEASE_DIR}/payload"
ARCHIVE_PATH="${RELEASE_DIR}/checkpp-${TAG}.tar.gz"
NOTES_PATH="${RELEASE_DIR}/release-notes-${TAG}.md"
SHA_PATH="${RELEASE_DIR}/SHA256SUMS"
BUILD_LOG="${RELEASE_DIR}/build.log"
CHECKER_LOG="${RELEASE_DIR}/checker.log"

mkdir -p "$RELEASE_DIR"

log "Validating release build on ${DEFAULT_BRANCH}"
if ! run_logged "$BUILD_LOG" cmake --preset "$PRESET"; then
  die "CMake configure failed (see ${BUILD_LOG})"
fi
assert_no_warning_lines "$BUILD_LOG" || die "Warnings found during configure (see ${BUILD_LOG})"

if ! run_logged "$BUILD_LOG" cmake --build --preset "$PRESET" -j"$(nproc)"; then
  die "Build failed (see ${BUILD_LOG})"
fi
assert_no_warning_lines "$BUILD_LOG" || die "Warnings found during build (see ${BUILD_LOG})"

if [ ! -x "$CHECKPP_BIN" ]; then
  die "Built binary not found: ${CHECKPP_BIN}"
fi
if [ ! -f "$PLUGIN_PATH" ]; then
  die "Built plugin not found: ${PLUGIN_PATH}"
fi

log "Running checker validation"
if ! run_logged "$CHECKER_LOG" "$CHECKPP_BIN" "$PROJECT_ROOT" "$BUILD_DIR" "$RULES_PATH" "$PLUGIN_PATH"; then
  die "Checker execution failed (see ${CHECKER_LOG})"
fi
verify_checker_output "$CHECKER_LOG" || die "Checker reported warnings or errors (see ${CHECKER_LOG})"

if git rev-parse --verify --quiet "refs/tags/${TAG}" >/dev/null; then
  die "Tag already exists locally: ${TAG}"
fi
if git ls-remote --exit-code --tags "$REMOTE" "refs/tags/${TAG}" >/dev/null 2>&1; then
  die "Tag already exists on ${REMOTE}: ${TAG}"
fi

log "Switching to release branch ${RELEASE_BRANCH}"
if git show-ref --verify --quiet "refs/heads/${RELEASE_BRANCH}"; then
  git switch "$RELEASE_BRANCH"
else
  git switch -c "$RELEASE_BRANCH"
fi

log "Merging ${DEFAULT_BRANCH} into ${RELEASE_BRANCH}"
git merge --no-ff --no-edit "$DEFAULT_BRANCH"

log "Tagging release ${TAG}"
git tag -a "$TAG" -m "Release ${TAG}"

log "Packaging release artifacts"
mkdir -p "$ARTIFACT_DIR"
cp "$CHECKPP_BIN" "$ARTIFACT_DIR/checkpp"
cp "$PLUGIN_PATH" "$ARTIFACT_DIR/CompanyClangTidyModule.so"
cp "$PROJECT_ROOT/config/rules.yaml" "$ARTIFACT_DIR/rules.yaml"
cp "$PROJECT_ROOT/README.md" "$ARTIFACT_DIR/README.md"

previous_ref="$(previous_release_ref "$TAG")"
cat > "$NOTES_PATH" <<EOF
# ${TAG}

Release generated from ${RELEASE_BRANCH} at $(git rev-parse --short HEAD).

## Changes since ${previous_ref}
EOF
git log --no-merges --pretty=format:'- %s (%h)' "${previous_ref}..HEAD" >> "$NOTES_PATH"
printf '\n' >> "$NOTES_PATH"

tar -czf "$ARCHIVE_PATH" -C "$ARTIFACT_DIR" .
sha256sum "$ARCHIVE_PATH" "$NOTES_PATH" > "$SHA_PATH"

log "Pushing tag and release branch"
git push --atomic "$REMOTE" "$RELEASE_BRANCH" "refs/tags/${TAG}"

log "Creating GitHub release ${TAG}"
gh release create "$TAG" \
  --title "$TAG" \
  --notes-file "$NOTES_PATH" \
  "$ARCHIVE_PATH" \
  "$NOTES_PATH" \
  "$SHA_PATH"

log "Checking out ${DEFAULT_BRANCH} to bump version"
git switch "$DEFAULT_BRANCH"

NEXT_VERSION="$(bump_version "$RELEASE_TYPE")"
git add CMakeLists.txt
git commit -m "chore: bump version to v${NEXT_VERSION}"
git push "$REMOTE" "$DEFAULT_BRANCH"

log "Release completed: ${TAG} -> next version ${NEXT_VERSION} (${RELEASE_TYPE})"
