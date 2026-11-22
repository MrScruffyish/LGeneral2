#!/bin/sh
# Build the normal and TRACE=1 variants from a pristine copy of the sources.
#
# The copy excludes .git/, .build/ and local notes, so stale build artifacts
# cannot hide a failure. Both builds must succeed without compiler warnings and
# must produce the expected executables.
#
# Usage:
#   PG_DATA=/path/to/pg-data.tar.gz tests/build-smoke.sh
#   TMPDIR=/tmp/opencode make smoke
#
# TMPDIR selects where the temporary checkout is created (default /tmp).

set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
tmp=$(mktemp -d "${TMPDIR:-/tmp}/lgeneral-smoke.XXXXXX")
trap 'rm -rf "$tmp"' EXIT HUP INT TERM

pg_data=${PG_DATA:-$HOME/PG_DATA/pg-data.tar.gz}
if [ ! -e "$pg_data" ]; then
    echo "smoke: PG_DATA not found: $pg_data" >&2
    exit 1
fi

echo "==> smoke: copying sources to $tmp"
(
    cd "$root"
    tar --exclude=./.git --exclude=./.build --exclude=./TODO.md -cf - .
) | tar -xf - -C "$tmp"

# CMake generates all build files inside the build copy, so no source-tree
# cleanup is needed here.

for mode in normal trace; do
    if [ "$mode" = trace ]; then
        trace=1
        build="$tmp/.build/lgeneral-trace"
    else
        trace=0
        build="$tmp/.build/lgeneral"
    fi
    log="$tmp/$mode.log"

    echo "==> smoke: $mode build"
    if ! PG_DATA="$pg_data" TRACE="$trace" make -C "$tmp" >"$log" 2>&1; then
        echo "smoke: $mode build failed; last 40 lines of $log:" >&2
        tail -40 "$log" >&2
        exit 1
    fi

    warnings=$(grep -c 'warning:' "$log" || true)
    if [ "$warnings" -ne 0 ]; then
        echo "smoke: $mode build produced $warnings warning(s):" >&2
        grep 'warning:' "$log" >&2
        exit 1
    fi

    for exe in "$build/src/lgeneral" "$build/lgc-pg/lgc-pg" \
               "$tmp/.build/lgeditor$([ "$trace" = 1 ] && echo -trace)/src/lgeditor"; do
        if [ ! -x "$exe" ]; then
            echo "smoke: missing executable: $exe" >&2
            exit 1
        fi
    done

    if [ -n "${SDL3_MIXER_PREFIX:-}" ] && \
       ! ldd "$build/src/lgeneral" | grep -q 'SDL3_mixer'; then
        echo "smoke: SDL3_MIXER_PREFIX was set but the game did not link SDL3_mixer" >&2
        exit 1
    fi

    if [ ! -f "$build/src/lgeneral.conf" ]; then
        echo "smoke: missing generated config: $build/src/lgeneral.conf" >&2
        exit 1
    fi

    if [ "$mode" = normal ]; then
        echo "==> smoke: translation check"
        "$tmp/tests/test-translations.sh" "$build"
        echo "==> smoke: event mapping check"
        "$tmp/tests/test-events.sh" "$tmp"
        echo "==> smoke: resize event check"
        "$tmp/tests/test-resize.sh" "$tmp"
        echo "==> smoke: date parser check"
        "$tmp/tests/test-date.sh" "$tmp"
        echo "==> smoke: alpha blit check"
        "$tmp/tests/test-alpha.sh" "$tmp"
    fi

    echo "==> smoke: runtime launch ($mode)"
    "$tmp/tests/run-smoke.sh" "$build/src/lgeneral"

    if [ "$mode" = trace ]; then
        echo "==> smoke: deployment map cleanup check"
        "$tmp/tests/test-deploy.sh" "$build/src/lgeneral"
    fi
done

if ! strings "$tmp/.build/lgeneral-trace/src/lgeneral" | grep -q '\[trace\]'; then
    echo "smoke: trace build contains no trace messages" >&2
    exit 1
fi

echo "smoke: OK"
