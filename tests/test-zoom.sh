#!/bin/sh
# Verify pointer-coordinate conversion for integer display zoom.
#
# Builds a tiny probe against lgeneral/src/event.cpp. The game keeps drawing in
# logical screen coordinates; when the window surface is magnified, pointer
# positions must be divided by the zoom factor and corrected for letterboxing.
#
# Usage: tests/test-zoom.sh [source-root]

set -eu

root=${1:-$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)}
tmp=$(mktemp -d "${TMPDIR:-/tmp}/lgeneral-zoom.XXXXXX")
trap 'rm -rf "$tmp"' EXIT HUP INT TERM

cat > "$tmp/test-zoom.cpp" <<'EOF'
#include <SDL3/SDL.h>

#include "event.h"

#include <cstdio>

static void send_motion( float x, float y )
{
    SDL_Event event{};
    event.type = SDL_EVENT_MOUSE_MOTION;
    event.motion.x = x;
    event.motion.y = y;
    event_filter( &event );
}

static int cursor_is( int expected_x, int expected_y )
{
    int x = -1, y = -1;
    event_get_cursor_pos( &x, &y );
    return x == expected_x && y == expected_y;
}

int main()
{
    event_clear();
    event_set_view_transform( 2, 40, 30, 400, 300 );

    send_motion( 440, 330 );
    if ( !cursor_is( 200, 150 ) ) {
        std::fprintf( stderr, "zoom: scaled pointer was not converted\n" );
        return 1;
    }

    send_motion( 39, 29 );
    if ( !cursor_is( 0, 0 ) ) {
        std::fprintf( stderr, "zoom: pointer before the viewport was not clamped\n" );
        return 1;
    }

    send_motion( 9999, 9999 );
    if ( !cursor_is( 399, 299 ) ) {
        std::fprintf( stderr, "zoom: pointer past the viewport was not clamped\n" );
        return 1;
    }

    SDL_Event button{};
    button.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    button.button.button = SDL_BUTTON_LEFT;
    button.button.x = 240;
    button.button.y = 230;
    event_filter( &button );
    int pressed = -1, x = -1, y = -1;
    if ( !event_get_buttondown( &pressed, &x, &y ) ||
         pressed != BUTTON_LEFT || x != 100 || y != 100 ) {
        std::fprintf( stderr, "zoom: button coordinates were not converted\n" );
        return 1;
    }

    event_set_view_transform( 1, 0, 0, 800, 600 );
    send_motion( 123, 456 );
    if ( !cursor_is( 123, 456 ) ) {
        std::fprintf( stderr, "zoom: identity transform changed coordinates\n" );
        return 1;
    }

    std::puts( "zoom: OK" );
    return 0;
}
EOF

cxx=${CXX:-c++}
# shellcheck disable=SC2046
"$cxx" -std=c++17 -Wall \
    $(pkg-config --cflags sdl3) \
    -I"$root/lgeneral/src" -I"$root/lgeneral" \
    "$tmp/test-zoom.cpp" "$root/lgeneral/src/event.cpp" \
    $(pkg-config --libs sdl3) -o "$tmp/test-zoom"

"$tmp/test-zoom"
