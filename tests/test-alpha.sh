#!/bin/sh
# Verify that the SDL 1.2 alpha compatibility shim actually blends in SDL3.
#
# SDL3 only applies an alpha modifier when the source blend mode is
# SDL_BLENDMODE_BLEND. The fog/danger overlays rely on this; without it they are
# painted fully opaque. This probe blits a translucent red surface onto black
# and checks that the destination is dimmed red, not solid red.
#
# Usage: tests/test-alpha.sh [source-root]

set -eu

root=${1:-$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)}
tmp=$(mktemp -d "${TMPDIR:-/tmp}/lgeneral-alpha.XXXXXX")
trap 'rm -rf "$tmp"' EXIT HUP INT TERM

cat > "$tmp/test-alpha.cpp" <<'EOF'
#include "sdl_compat.h"

#include <cstdio>

static int failures = 0;

static SDL_Surface *make_surface(Uint32 color)
{
    SDL_Surface *surface = SDL_CreateSurface(1, 1, SDL_PIXELFORMAT_XRGB8888);
    if (!surface) {
        std::fprintf(stderr, "alpha: SDL_CreateSurface failed: %s\n", SDL_GetError());
        return nullptr;
    }
    const Uint32 *src = &color;
    SDL_memcpy(surface->pixels, src, sizeof color);
    /* Game surfaces use a color key; SDL3 also needs it for alpha modifiers on
       formats without an alpha channel. */
    if (!SDL_SetSurfaceColorKey(surface, true, 0x00000000)) {
        std::fprintf(stderr, "alpha: color key failed: %s\n", SDL_GetError());
        SDL_DestroySurface(surface);
        return nullptr;
    }
    return surface;
}

static bool pixel(SDL_Surface *surface, Uint8 *r, Uint8 *g, Uint8 *b)
{
    Uint32 value = 0;
    SDL_memcpy(&value, surface->pixels, sizeof value);
    lg_get_rgb(value, surface->format, r, g, b);
    return true;
}

static void blit_and_check(const char *label, int flag, int alpha,
                           int low, int high)
{
    SDL_Surface *src = make_surface(0x00ff0000);
    SDL_Surface *dst = make_surface(0x00000000);
    Uint8 r = 0, g = 0, b = 0;
    if (!src || !dst) {
        failures++;
        goto done;
    }
    if (!lg_set_alpha(src, flag, alpha) ||
        !SDL_BlitSurface(src, nullptr, dst, nullptr)) {
        std::fprintf(stderr, "alpha: %s blit failed: %s\n", label, SDL_GetError());
        failures++;
        goto done;
    }

    pixel(dst, &r, &g, &b);
    if (r < low || r > high || g > 8 || b > 8) {
        std::fprintf(stderr, "alpha: %s expected red %d..%d, got %d,%d,%d\n",
                     label, low, high, r, g, b);
        failures++;
    }

done:
    SDL_DestroySurface(src);
    SDL_DestroySurface(dst);
}

int main()
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::fprintf(stderr, "alpha: SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    blit_and_check("50% blend", SDL_SRCALPHA, 128, 110, 145);
    blit_and_check("25% blend", SDL_SRCALPHA, 64, 45, 80);
    blit_and_check("alpha disabled", 0, 0, 245, 255);

    SDL_Quit();
    if (failures)
        return 1;
    std::puts("alpha: OK");
    return 0;
}
EOF

cxx=${CXX:-c++}
# shellcheck disable=SC2046
"$cxx" -std=c++17 -Wall \
    $(pkg-config --cflags sdl3) \
    -I"$root/lgeneral/src" -I"$root/lgeneral/util" \
    "$tmp/test-alpha.cpp" \
    $(pkg-config --libs sdl3) -o "$tmp/test-alpha"

"$tmp/test-alpha"
