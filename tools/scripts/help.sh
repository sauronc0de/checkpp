#!/usr/bin/env bash
set -u
set -o pipefail

SCRIPT_NAME="$(basename "$0")"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
VERSION="0.4.0"

ROOTS=()
RECURSIVE=0
VERBOSE=0

usage() {
    cat <<EOF
 SYNOPSIS
    ${SCRIPT_NAME} [-hrv] [--root DIR]

 DESCRIPTION
    Lists --short-help from executable scripts and binaries.

    Default scan paths:
      ${SCRIPT_DIR}
      ${SCRIPT_DIR}/..

 OPTIONS
    --root DIR          Scan this directory instead of defaults
    -r, --recursive     Scan recursively
    --verbose           Print diagnostics to stderr
    -h, --help          Print this help
    -v, --version       Print version

 EXAMPLES
    ${SCRIPT_NAME}
    ${SCRIPT_NAME} -r
    ${SCRIPT_NAME} --root ./tools/scripts

 IMPLEMENTATION
    version         ${VERSION}
    script          ${SCRIPT_NAME}
    location        tools/scripts/help.sh
EOF
}

log() {
    [ "$VERBOSE" -eq 1 ] && printf '[INFO] %s\n' "$*" >&2
}

err() {
    printf '[ERROR] %s\n' "$*" >&2
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --root)
            shift
            [ "$#" -gt 0 ] || { err "--root requires a value"; exit 2; }
            ROOTS+=("$1")
            ;;
        --root=*)
            ROOTS+=("${1#*=}")
            ;;
        -r|--recursive)
            RECURSIVE=1
            ;;
        --verbose)
            VERBOSE=1
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        -v|--version)
            printf '%s %s\n' "$SCRIPT_NAME" "$VERSION"
            exit 0
            ;;
        --short-help)
            printf 'List executable tools; example: %s -r --root ./tools/scripts.\n' "$SCRIPT_NAME"
            exit 0
            ;;
        *)
            err "Unknown argument: $1"
            exit 2
            ;;
    esac
    shift
done

if [ "${#ROOTS[@]}" -eq 0 ]; then
    ROOTS=("$SCRIPT_DIR" "$SCRIPT_DIR/..")
fi

self_path="$(readlink -f "$0" 2>/dev/null || printf '%s\n' "$0")"

run_tool() {
    tool="$1"

    tool_path="$(readlink -f "$tool" 2>/dev/null || printf '%s\n' "$tool")"

    [ "$tool_path" = "$self_path" ] && return 0
    [ -x "$tool" ] || return 0
    [ -f "$tool" ] || return 0

    name="$(basename "$tool")"

    # 1. Try --short-help
    output="$("$tool" --short-help 2>/dev/null)"
    if [ $? -eq 0 ] && [ -n "$output" ]; then
        output="$(printf '%s\n' "$output" | head -n 1 | sed 's/[[:space:]]\+/ /g')"
        printf '%-25s | %s\n' "$name" "$output"
        return 0
    fi

    # 2. Fallback to --help (first line)
    output="$("$tool" --help 2>/dev/null | head -n 1)"
    if [ -n "$output" ]; then
        output="$(printf '%s\n' "$output" | sed 's/[[:space:]]\+/ /g')"
        printf '%-25s | %s\n' "$name" "$output"
        return 0
    fi

    # 3. Final fallback → just list it
    printf '%-25s | (no help available)\n' "$name"
}

tmp_file="$(mktemp)"
trap 'rm -f "$tmp_file"' EXIT

for root in "${ROOTS[@]}"; do
    if [ ! -d "$root" ]; then
        log "Skipping missing directory: $root"
        continue
    fi

    log "Scanning: $root"

    if [ "$RECURSIVE" -eq 1 ]; then
        find "$root" -type f -executable >> "$tmp_file"
    else
        find "$root" -maxdepth 1 -type f -executable >> "$tmp_file"
    fi
done

if [ ! -s "$tmp_file" ]; then
    err "No executable files found."
    exit 1
fi

printf '%-25s | %s\n' "TOOL" "DESCRIPTION"
printf '%-25s-+-%s\n' "-------------------------" "---------------------------------------------"

sort -u "$tmp_file" | while IFS= read -r tool; do
    run_tool "$tool"
done
