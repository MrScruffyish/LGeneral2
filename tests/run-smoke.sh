#!/bin/sh
# Launch a built game with dummy SDL drivers and verify a clean shutdown.
#
# Usage:
#   tests/run-smoke.sh /path/to/lgeneral [zoom]
#
# The game resolves data relative to its working directory, so the script runs
# it from the directory containing the executable. SIGTERM is used as the
# shutdown request because SDL turns it into SDL_EVENT_QUIT, which the engine
# handles through its normal quit path.

set -eu

binary=${1:-}
zoom=${2:-}
if [ -z "$binary" ]; then
    echo "run-smoke: usage: $0 /path/to/lgeneral" >&2
    exit 2
fi
if [ ! -x "$binary" ]; then
    echo "run-smoke: not executable: $binary" >&2
    exit 2
fi

case "$binary" in
    /*) ;;
    *) binary=$(pwd)/"$binary" ;;
esac

game_dir=$(CDPATH= cd -- "$(dirname -- "$binary")" && pwd)
binary="$game_dir/$(basename -- "$binary")"
tmp=$(mktemp -d "${TMPDIR:-/tmp}/lgeneral-run-smoke.XXXXXX")
log="$tmp/game.log"
trace_log="$tmp/trace.log"
config="$game_dir/lgeneral.conf"
config_backup=
pid=
watchdog=

cleanup() {
    if [ -n "$config_backup" ] && [ -f "$config_backup" ]; then
        cp "$config_backup" "$config"
    fi
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

if [ -n "$zoom" ]; then
    cp "$config" "$tmp/lgeneral.conf.orig"
    config_backup="$tmp/lgeneral.conf.orig"
    if grep -q '^zoom=' "$config"; then
        sed "s/^zoom=.*/zoom=$zoom/" "$config" > "$config.new"
    else
        cat "$config" > "$config.new"
        printf 'zoom=%s\n' "$zoom" >> "$config.new"
    fi
    mv "$config.new" "$config"
fi

if command -v stdbuf >/dev/null 2>&1; then
    set -- stdbuf -oL -eL "$binary"
else
    set -- "$binary"
fi

cd "$game_dir"
LC_ALL=C SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
    LG_TRACE_FILE="$trace_log" "$@" >"$log" 2>&1 &
pid=$!

# Wait for the startup banner, but do not wait forever for a process that
# crashed before reaching it.
ready=0
attempt=0
while [ "$attempt" -lt 150 ]; do
    if grep -q 'Looking up data in:' "$log" 2>/dev/null; then
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

# Reap the game under a watchdog so a broken quit path fails instead of
# hanging the whole smoke run.
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
    echo "run-smoke: game did not reach startup; last 40 lines of $log:" >&2
    tail -40 "$log" >&2 || true
    exit 1
fi
if [ "$rc" -ne 0 ]; then
    echo "run-smoke: game exited with status $rc; last 40 lines of $log:" >&2
    tail -40 "$log" >&2 || true
    exit 1
fi
for marker in 'LGeneral 1.4.4' 'Looking up data in: .' 'SDL finalized'; do
    if ! grep -q "$marker" "$log"; then
        echo "run-smoke: missing startup/shutdown marker: $marker" >&2
        tail -40 "$log" >&2 || true
        exit 1
    fi
done

echo "run-smoke: OK ($binary)"
