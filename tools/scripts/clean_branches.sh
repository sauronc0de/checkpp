#!/usr/bin/env bash

set -euo pipefail

REMOTE="origin"
PROTECTED_BRANCHES=("main" "master" "develop" "staging")

echo "🔄 Fetching latest remote info..."
git fetch "$REMOTE" --prune

DEFAULT_BRANCH="$(git symbolic-ref --quiet --short "refs/remotes/$REMOTE/HEAD" | sed "s#^$REMOTE/##")"

if [[ -z "$DEFAULT_BRANCH" ]]; then
  echo "❌ Could not detect the default branch from $REMOTE/HEAD"
  exit 1
fi

echo "✅ Default branch detected: $DEFAULT_BRANCH"

is_protected_branch() {
  local branch="$1"

  [[ "$branch" == "$DEFAULT_BRANCH" ]] && return 0

  for protected in "${PROTECTED_BRANCHES[@]}"; do
    [[ "$branch" == "$protected" ]] && return 0
  done

  return 1
}

echo
echo "🧹 Checking merged local branches..."
git branch --merged "$REMOTE/$DEFAULT_BRANCH" | while IFS= read -r branch; do
  branch="$(echo "$branch" | sed 's/^[* ]*//;s/[[:space:]]*$//')"
  [[ -z "$branch" ]] && continue

  if is_protected_branch "$branch"; then
    echo "⏭️  Skipping protected local branch: $branch"
    continue
  fi

  if [[ "$(git branch --show-current)" == "$branch" ]]; then
    echo "⏭️  Skipping current local branch: $branch"
    continue
  fi

  echo "✅ Deleting local branch: $branch"
  git branch -d "$branch"
done

echo
echo "🧹 Checking merged remote branches..."
git branch -r --merged "$REMOTE/$DEFAULT_BRANCH" | while IFS= read -r remote_branch; do
  remote_branch="$(echo "$remote_branch" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')"
  [[ -z "$remote_branch" ]] && continue
  [[ "$remote_branch" == "$REMOTE/HEAD"*"->"* ]] && continue
  [[ "$remote_branch" != "$REMOTE/"* ]] && continue

  branch="${remote_branch#${REMOTE}/}"

  if is_protected_branch "$branch"; then
    echo "⏭️  Skipping protected remote branch: $branch"
    continue
  fi

  echo "✅ Deleting remote branch: $branch"
  git push "$REMOTE" --delete "$branch"
done

echo
echo "🎉 Cleanup complete."