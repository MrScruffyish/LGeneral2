/*
 * Small helpers that keep the editor's SDL2-era drawing code readable while
 * using the SDL3 API.  All rendering still goes through the SDL3 renderer.
 */
#ifndef LDEDITOR_SDL3_COMPAT_H
#define LDEDITOR_SDL3_COMPAT_H

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <cstddef>
#include <cstring>
#include <vector>

inline bool lg_text_size(TTF_Font *font, const char *text, int *w, int *h)
{
    return TTF_GetStringSize(font, text, std::strlen(text), w, h);
}

inline SDL_Surface *lg_render_text(TTF_Font *font, const char *text, SDL_Color color)
{
    return TTF_RenderText_Blended(font, text, std::strlen(text), color);
}

inline SDL_Surface *lg_render_text_wrapped(TTF_Font *font, const char *text,
                                           SDL_Color color, int wrap_width)
{
    return TTF_RenderText_Blended_Wrapped(font, text, std::strlen(text),
                                          color, wrap_width);
}

inline int lg_bytes_per_pixel(const SDL_Surface *surface)
{
    return SDL_GetPixelFormatDetails(surface->format)->bytes_per_pixel;
}

inline Uint32 lg_map_rgb(const SDL_Surface *surface, Uint8 r, Uint8 g, Uint8 b)
{
    return SDL_MapRGB(SDL_GetPixelFormatDetails(surface->format),
                      SDL_GetSurfacePalette(const_cast<SDL_Surface *>(surface)),
                      r, g, b);
}

inline bool lg_render_copy(SDL_Renderer *renderer, SDL_Texture *texture,
                           const SDL_Rect *src, const SDL_Rect *dst)
{
    SDL_FRect fsrc, fdst;
    if (src) {
        fsrc.x = static_cast<float>(src->x);
        fsrc.y = static_cast<float>(src->y);
        fsrc.w = static_cast<float>(src->w);
        fsrc.h = static_cast<float>(src->h);
    }
    if (dst) {
        fdst.x = static_cast<float>(dst->x);
        fdst.y = static_cast<float>(dst->y);
        fdst.w = static_cast<float>(dst->w);
        fdst.h = static_cast<float>(dst->h);
    }
    return SDL_RenderTexture(renderer, texture, src ? &fsrc : NULL,
                             dst ? &fdst : NULL);
}

inline bool lg_render_fill_rect(SDL_Renderer *renderer, const SDL_Rect *rect)
{
    SDL_FRect frect = { static_cast<float>(rect->x), static_cast<float>(rect->y),
                        static_cast<float>(rect->w), static_cast<float>(rect->h) };
    return SDL_RenderFillRect(renderer, &frect);
}

inline bool lg_render_lines(SDL_Renderer *renderer, const SDL_Point *points, int count)
{
    if (count <= 0)
        return true;
    std::vector<SDL_FPoint> fpoints(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        fpoints[i].x = static_cast<float>(points[i].x);
        fpoints[i].y = static_cast<float>(points[i].y);
    }
    return SDL_RenderLines(renderer, fpoints.data(), count);
}

#endif
