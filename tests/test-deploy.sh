#!/bin/sh
# Verify that deployment-pool units are removed from the map when the deploy
# window opens.
#
# Poland's initial deployment list reuses the units already on the map. If the
# map cells keep pointing at those units, every old position stays blocked and
# only one new placement fits. This probe opens the Poland deploy window in a
# TRACE=1 build and checks that no pool unit is still referenced by a map tile.
#
# Usage: tests/test-deploy.sh /path/to/trace-build/src/lgeneral

set -eu

binary=${1:-}
if [ -z "$binary" ]; then
    echo "test-deploy: usage: $0 /path/to/lgeneral" >&2
    exit 2
fi
if [ ! -x "$binary" ]; then
    echo "test-deploy: not executable: $binary" >&2
    exit 2
fi

case "$binary" in
    /*) ;;
    *) binary=$(pwd)/"$binary" ;;
esac

game_dir=$(CDPATH= cd -- "$(dirname -- "$binary")" && pwd)
binary="$game_dir/$(basename -- "$binary")"
tmp=$(mktemp -d "${TMPDIR:-/tmp}/lgeneral-deploy.XXXXXX")
log="$tmp/game.log"
trace_log="$tmp/trace.log"
pid=
watchdog=

cleanup() {
    if [ -n "$watchdog" ] && kill -0 "$watchdog" 2>/dev/null; then
        kill "$watchdog" 2>/dev/null || true
        wait "$watchdog" 2>/dev/null || true
    fi
    if [ -n "$pid" ] && kill -0 "$pid" 2>/dev/null; then
        kill -KILL "$pid" 2>/dev/null || true
        wait "$pid" 2>/dev/null || true
    fi
    rm -rf "$tmp"
}
trap cleanup EXIT HUP INT TERM

if command -v stdbuf >/dev/null 2>&1; then
    set -- stdbuf -oL -eL "$binary" --notitle --deploy-turn pg/Poland
else
    set -- "$binary" --notitle --deploy-turn pg/Poland
fi

cd "$game_dir"
LC_ALL=C SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
    LG_TRACE_FILE="$trace_log" "$@" >"$log" 2>&1 &
pid=$!

ready=0
attempt=0
while [ "$attempt" -lt 150 ]; do
    if grep -q 'deploy window stale map refs=' "$trace_log" 2>/dev/null; then
        ready=1
        break
    fi
    if ! kill -0 "$pid" 2>/dev/null; then
        break
    fi
    sleep 0.1
    attempt=$((attempt + 1))
done

if kill -0 "$pid" 2>/dev/null; then
    kill -TERM "$pid"
fi

( sleep 10; kill -KILL "$pid" 2>/dev/null ) &
watchdog=$!
set +e
wait "$pid"
rc=$?
set -e
kill "$watchdog" 2>/dev/null || true
wait "$watchdog" 2>/dev/null || true
watchdog=

if [ "$ready" -ne 1 ]; then
    echo "test-deploy: deploy window not reached; last 40 lines of trace:" >&2
    tail -40 "$trace_log" >&2 || true
    tail -40 "$log" >&2 || true
    exit 1
fi
if [ "$rc" -ne 0 ]; then
    echo "test-deploy: game exited with status $rc; last 40 lines of log:" >&2
    tail -40 "$log" >&2 || true
    exit 1
fi

stale=$(grep 'deploy window stale map refs=' "$trace_log" | tail -1)
case "$stale" in
    *'stale map refs=0'*) ;;
    *)
        echo "test-deploy: deploy pool still on map: $stale" >&2
        exit 1
        ;;
esac

if ! grep -q 'deploy window avail=' "$trace_log"; then
    echo "test-deploy: missing deploy window availability trace" >&2
    exit 1
fi
clear_count=$(grep -c 'deploy window clear ' "$trace_log" || true)
if [ "$clear_count" -lt 2 ]; then
    echo "test-deploy: expected multiple units to be cleared for deployment" >&2
    exit 1
fi

echo "test-deploy: OK ($binary)"
