/*
 * SDL 1.2 compatibility helpers for the game core.
 *
 * The game sources still use the SDL 1.2 surface API in a few places.  SDL3
 * provides the same surface model but renamed several functions and changed
 * a handful of types.  Centralising the differences here keeps the migration
 * reviewable; new code should use the SDL3 names directly.
 */
#ifndef LG_SDL_COMPAT_H
#define LG_SDL_COMPAT_H

/* Keep the SDL 1.2/2 spellings available while the game sources migrate. */
#ifndef SDL_ENABLE_OLD_NAMES
#define SDL_ENABLE_OLD_NAMES 1
#endif

#include <SDL3/SDL.h>

#include <cstddef>

/* Implemented in lg-sdl.cpp once the game window exists. */
SDL_Surface *lg_get_video_surface();

inline Uint32 lg_map_rgb(SDL_PixelFormat format, Uint8 red, Uint8 green,
                         Uint8 blue)
{
    return ::SDL_MapRGB(SDL_GetPixelFormatDetails(format), nullptr,
                        red, green, blue);
}

inline void lg_get_rgb(Uint32 pixel, SDL_PixelFormat format,
                       Uint8 *red, Uint8 *green, Uint8 *blue)
{
    ::SDL_GetRGB(pixel, SDL_GetPixelFormatDetails(format), nullptr,
                 red, green, blue);
}

inline int lg_bytes_per_pixel(const SDL_Surface *surface)
{
    return SDL_GetPixelFormatDetails(surface->format)->bytes_per_pixel;
}

inline SDL_Surface *lg_create_surface(int /*flags*/, int width, int height)
{
    SDL_Surface *screen = lg_get_video_surface();
    const SDL_PixelFormat format = screen ? screen->format
                                          : SDL_PIXELFORMAT_RGBA8888;
    return SDL_CreateSurface(width, height, format);
}

inline bool lg_set_alpha(SDL_Surface *surface, int flag, int alpha)
{
    return SDL_SetSurfaceAlphaMod(surface,
                                  flag != 0 ? static_cast<Uint8>(alpha) : 255);
}

/* Surface flags no longer exist in SDL3; the game only used them as no-ops. */
#define SDL_SWSURFACE 0
#define SDL_HWSURFACE 0
#define SDL_FULLSCREEN 0
#define SDL_DOUBLEBUF 0
#define SDL_SRCCOLORKEY 1
#define SDL_SRCALPHA 1
#define SDL_NONFATAL 0x10000000
#define SDL_INIT_TIMER 0

/* SDL3 already provides compatibility aliases for event names, modifiers and
 * the simpler surface functions.  Only the genuinely missing pieces are
 * defined here. */
#define SDLK_SCROLLOCK SDLK_SCROLLLOCK

#define SDL_SetAlpha(surface, flag, alpha) \
    lg_set_alpha((surface), (flag), (alpha))
#define SDL_MapRGB(format, red, green, blue) \
    lg_map_rgb((format), (red), (green), (blue))
#define SDL_GetRGB(pixel, format, red, green, blue) \
    lg_get_rgb((pixel), (format), (red), (green), (blue))
#define SDL_GetVideoSurface() lg_get_video_surface()
#define SDL_CreateRGBSurface(flags, width, height, bpp, rmask, gmask, bmask, amask) \
    lg_create_surface((flags), (width), (height))
#define LG_BYTES_PER_PIXEL(surface) lg_bytes_per_pixel(surface)

#endif
