#!/bin/sh
# Verify that SDL3 window resize events are reported to the engine.
#
# Builds a tiny probe against lgeneral/src/event.cpp. SDL reports both logical
# window size and drawable pixel size; the game needs the latter for its window
# surface. This checks that both event shapes are recognized, that the pixel
# size wins when SDL sends both, and that resize state is consumed once.
#
# Usage: tests/test-resize.sh [source-root]

set -eu

root=${1:-$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)}
tmp=$(mktemp -d "${TMPDIR:-/tmp}/lgeneral-resize.XXXXXX")
trap 'rm -rf "$tmp"' EXIT HUP INT TERM

cat > "$tmp/test-resize.cpp" <<'EOF'
#include <SDL3/SDL.h>

#include "event.h"

#include <cstdio>

static void send_window_event( SDL_EventType type, int width, int height )
{
    SDL_Event event{};
    event.type = type;
    event.window.data1 = width;
    event.window.data2 = height;
    event_filter( &event );
}

int main()
{
    int width = 0, height = 0;

    event_clear();
    if ( event_check_window_resize( &width, &height ) ) {
        std::fprintf( stderr, "resize: pending state before any event\n" );
        return 1;
    }

    send_window_event( SDL_EVENT_WINDOW_RESIZED, 1024, 768 );
    if ( !event_check_window_resize( &width, &height ) ||
         width != 1024 || height != 768 ) {
        std::fprintf( stderr, "resize: logical window size was not reported\n" );
        return 1;
    }
    if ( event_check_window_resize( &width, &height ) ) {
        std::fprintf( stderr, "resize: event was not consumed\n" );
        return 1;
    }

    /* SDL sends this after SDL_EVENT_WINDOW_RESIZED on high-DPI displays.
     * The drawable pixel size must take precedence. */
    send_window_event( SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED, 1600, 900 );
    send_window_event( SDL_EVENT_WINDOW_RESIZED, 800, 450 );
    if ( !event_check_window_resize( &width, &height ) ||
         width != 1600 || height != 900 ) {
        std::fprintf( stderr, "resize: pixel size did not win over window size\n" );
        return 1;
    }

    send_window_event( SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED, 1280, 720 );
    event_clear();
    if ( event_check_window_resize( &width, &height ) ) {
        std::fprintf( stderr, "resize: event_clear did not reset resize state\n" );
        return 1;
    }

    std::puts( "resize: OK" );
    return 0;
}
EOF

cxx=${CXX:-c++}
# shellcheck disable=SC2046
"$cxx" -std=c++17 -Wall \
    $(pkg-config --cflags sdl3) \
    -I"$root/lgeneral/src" -I"$root/lgeneral" \
    "$tmp/test-resize.cpp" "$root/lgeneral/src/event.cpp" \
    $(pkg-config --libs sdl3) -o "$tmp/test-resize"

"$tmp/test-resize"
