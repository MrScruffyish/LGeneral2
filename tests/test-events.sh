#!/bin/sh
# Verify SDL3 mouse-wheel and button mapping in the game's event filter.
#
# Builds a tiny probe against lgeneral/src/event.cpp and SDL3. This checks the
# SDL3-specific path that headless startup alone does not exercise: wheel
# events must become the game's synthetic WHEEL_UP/WHEEL_DOWN buttons, and
# SDL3 X1/X2 buttons must not be mistaken for wheel input.
#
# Usage: tests/test-events.sh [source-root]

set -eu

root=${1:-$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)}
tmp=$(mktemp -d "${TMPDIR:-/tmp}/lgeneral-events.XXXXXX")
trap 'rm -rf "$tmp"' EXIT HUP INT TERM

cat > "$tmp/test-events.cpp" <<'EOF'
#include <SDL3/SDL.h>

#include "event.h"

#include <cstdio>

static int button = -1;
static int x = 0;
static int y = 0;

static int check_wheel( float delta, int expected )
{
    SDL_Event event{};
    event.type = SDL_EVENT_MOUSE_WHEEL;
    event.wheel.direction = SDL_MOUSEWHEEL_NORMAL;
    event.wheel.y = delta;
    event_filter( &event );
    return event_get_buttondown( &button, &x, &y ) && button == expected;
}

int main()
{
    if ( !check_wheel( 1.0f, WHEEL_UP ) ) {
        std::fprintf( stderr, "events: wheel up was not mapped\n" );
        return 1;
    }
    if ( !check_wheel( -1.0f, WHEEL_DOWN ) ) {
        std::fprintf( stderr, "events: wheel down was not mapped\n" );
        return 1;
    }

    SDL_Event event{};
    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    event.button.button = SDL_BUTTON_X1;
    event_filter( &event );
    if ( event_get_buttondown( &button, &x, &y ) || event_check_button( WHEEL_UP ) ) {
        std::fprintf( stderr, "events: X1 was mistaken for wheel input\n" );
        return 1;
    }

    event.button.button = SDL_BUTTON_LEFT;
    event_filter( &event );
    if ( !event_get_buttondown( &button, &x, &y ) || button != BUTTON_LEFT ||
         !event_check_button( BUTTON_LEFT ) ) {
        std::fprintf( stderr, "events: left button was not mapped\n" );
        return 1;
    }

    std::puts( "events: OK" );
    return 0;
}
EOF

cxx=${CXX:-c++}
# shellcheck disable=SC2046
"$cxx" -std=c++17 -Wall \
    $(pkg-config --cflags sdl3) \
    -I"$root/lgeneral/src" -I"$root/lgeneral" \
    "$tmp/test-events.cpp" "$root/lgeneral/src/event.cpp" \
    $(pkg-config --libs sdl3) -o "$tmp/test-events"

"$tmp/test-events"
