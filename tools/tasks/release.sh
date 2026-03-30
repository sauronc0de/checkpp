#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
PRESET="release"
REMOTE="origin"
DEFAULT_BRANCH=""

usage() {
  cat <<EOF
Usage: $0

Runs the strict release workflow.
You will be prompted for patch, minor, or major.
If no prompt is available, patch is used by default.
EOF
}

if [ "${1:-}" = "-h" ] || [ "${1:-}" = "--help" ]; then
  usage
  exit 0
fi

if [ "$#" -gt 1 ]; then
  printf 'Too many arguments.\n' >&2
  usage >&2
  exit 1
fi

if [ "$#" -eq 1 ]; then
  case "$1" in
    patch|minor|major) ;;
    *)
      printf 'Unknown argument: %s\n' "$1" >&2
      usage >&2
      exit 1
      ;;
  esac
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
  python3 - <<'PY'
from pathlib import Path
import re
import sys

text = Path('CMakeLists.txt').read_text(encoding='utf-8')
  match = re.search(r'project\(checkpp\s+VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)\b', text)
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
  python3 - "$release_type" <<'PY'
from pathlib import Path
import re
import sys

release_type = sys.argv[1]
path = Path('CMakeLists.txt')
text = path.read_text(encoding='utf-8')
pattern = re.compile(r'(project\(checkpp\s+VERSION\s+)([0-9]+)\.([0-9]+)\.([0-9]+)(\s+LANGUAGES\s+C\s+CXX\))')
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
path.write_text(pattern.sub(lambda m: f'{m.group(1)}{new_version}{m.group(5)}', text, count=1), encoding='utf-8')
print(new_version)
PY
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

require_cmd git
require_cmd cmake
require_cmd gh
require_cmd python3
require_cmd tar
require_cmd sha256sum

cd "$PROJECT_ROOT"

DEFAULT_BRANCH="$(default_branch)"
CURRENT_BRANCH="$(current_branch)"

if [ "$CURRENT_BRANCH" != "$DEFAULT_BRANCH" ]; then
  die "Release can only run from ${DEFAULT_BRANCH}; current branch is ${CURRENT_BRANCH}"
fi

assert_clean_tree

RELEASE_TYPE="$(prompt_release_type "${1:-}")"

log "Pulling latest ${DEFAULT_BRANCH} from ${REMOTE}"
git pull --ff-only "$REMOTE" "$DEFAULT_BRANCH"

assert_clean_tree

NEXT_VERSION="$(bump_version "$RELEASE_TYPE")"
TAG="v${NEXT_VERSION}"

BUILD_DIR="${PROJECT_ROOT}/build/${PRESET}"
CHECKPP_BIN="${BUILD_DIR}/checkpp"
PLUGIN_PATH="${BUILD_DIR}/clang-tidy-module/CompanyClangTidyModule.so"
RULES_PATH="${PROJECT_ROOT}/config/rules.yaml"
RELEASE_DIR="${BUILD_DIR}/release-${TAG}"
ARTIFACT_DIR="${RELEASE_DIR}/assets"
NOTES_PATH="${RELEASE_DIR}/release-notes-${TAG}.md"
SHA_PATH="${RELEASE_DIR}/SHA256SUMS"
BUILD_LOG="${RELEASE_DIR}/build.log"
CHECKER_LOG="${RELEASE_DIR}/checker.log"

mkdir -p "$RELEASE_DIR"

log "Validating version bump ${NEXT_VERSION} on ${DEFAULT_BRANCH}"
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
if ! run_logged "$CHECKER_LOG" "$CHECKPP_BIN" "$PROJECT_ROOT" "$BUILD_DIR" "$RULES_PATH" --ignore-paths "$PROJECT_ROOT/config/ignore_paths.txt"; then
  die "Checker execution failed (see ${CHECKER_LOG})"
fi
verify_checker_output "$CHECKER_LOG" || die "Checker reported warnings or errors (see ${CHECKER_LOG})"

log "Committing version bump ${NEXT_VERSION}"
git add CMakeLists.txt
git commit -m "chore: bump version to v${NEXT_VERSION}"
git push "$REMOTE" "$DEFAULT_BRANCH"

if git rev-parse --verify --quiet "refs/tags/${TAG}" >/dev/null; then
  die "Tag already exists locally: ${TAG}"
fi
if git ls-remote --exit-code --tags "$REMOTE" "refs/tags/${TAG}" >/dev/null 2>&1; then
  die "Tag already exists on ${REMOTE}: ${TAG}"
fi

log "Packaging release artifacts"
mkdir -p "$ARTIFACT_DIR"
cp "$CHECKPP_BIN" "$ARTIFACT_DIR/checkpp-${TAG}"
cp "$PLUGIN_PATH" "$ARTIFACT_DIR/CompanyClangTidyModule-${TAG}.so"
cp "$PROJECT_ROOT/config/rules.yaml" "$ARTIFACT_DIR/rules-${TAG}.yaml"

previous_ref="$(previous_release_ref "$TAG")"
notes_head="$(git rev-parse HEAD^ 2>/dev/null || true)"
if [ -z "$notes_head" ]; then
  notes_head="$previous_ref"
fi
commit_count="$(git rev-list --count "${previous_ref}..${notes_head}")"
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
  "$ARTIFACT_DIR/checkpp-${TAG}" \
  "$ARTIFACT_DIR/CompanyClangTidyModule-${TAG}.so" \
  "$ARTIFACT_DIR/rules-${TAG}.yaml" \
  "$NOTES_PATH" > "$SHA_PATH"

log "Tagging release ${TAG}"
git tag -a "$TAG" -m "Release ${TAG}"
git push "$REMOTE" "refs/tags/${TAG}"

log "Creating GitHub release ${TAG}"
gh release create "$TAG" \
  --title "$TAG" \
  --notes-file "$NOTES_PATH" \
  "$ARTIFACT_DIR/checkpp-${TAG}" \
  "$ARTIFACT_DIR/CompanyClangTidyModule-${TAG}.so" \
  "$ARTIFACT_DIR/rules-${TAG}.yaml" \
  "$NOTES_PATH" \
  "$SHA_PATH"

log "Release completed: ${TAG}"
