/*
 * SDL3 helpers for the headless converter.
 *
 * The SDL 1.2 converter created its intermediate icon surfaces from the
 * 16-bit video surface (RGB565 on little-endian systems) and used
 * SDL_GetVideoSurface() to map palette colours.  SDL3 has no global video
 * surface, so the format is named explicitly here.  Keeping it unchanged is
 * required for byte-for-byte identical converter output.
 */
#ifndef LGC_PG_SDL3_COMPAT_H
#define LGC_PG_SDL3_COMPAT_H

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

inline int lg_bytes_per_pixel(const SDL_Surface *surface)
{
    return SDL_GetPixelFormatDetails(surface->format)->bytes_per_pixel;
}

inline SDL_Surface *lg_create_rgb565_surface(int width, int height)
{
    return SDL_CreateSurface(width, height, SDL_PIXELFORMAT_RGB565);
}

inline SDL_Surface *lg_create_surface_like(int width, int height,
                                           const SDL_Surface *source)
{
    return SDL_CreateSurface(width, height,
                             source ? source->format : SDL_PIXELFORMAT_RGB565);
}

inline Uint32 lg_map_rgb565(Uint8 red, Uint8 green, Uint8 blue)
{
    static const SDL_PixelFormatDetails *details =
        SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGB565);
    return SDL_MapRGB(details, NULL, red, green, blue);
}

inline bool lg_fill_surface(SDL_Surface *surface, Uint32 color)
{
    return SDL_FillSurfaceRect(surface, NULL, color);
}

/*
 * SDL 1.2 expanded RGB565 components to 8 bits with integer division by the
 * maximum component value, while SDL3 reproduces the high bits.  Save the
 * converter's RGB565 surfaces
 * with the old expansion so generated images remain byte-for-byte identical.
 */
inline bool lg_save_rgb565_bmp(SDL_Surface *surface, const char *path)
{
    if (!surface || surface->format != SDL_PIXELFORMAT_RGB565)
        return false;

    const int width = surface->w;
    const int height = surface->h;
    const std::size_t row_size = static_cast<std::size_t>((width * 3 + 3) & ~3);
    const std::size_t image_size = row_size * static_cast<std::size_t>(height);
    const std::uint32_t file_size = static_cast<std::uint32_t>(54 + image_size);

    unsigned char header[54] = {0};
    auto put16 = [&header](std::size_t offset, std::uint16_t value) {
        header[offset] = static_cast<unsigned char>(value & 0xff);
        header[offset + 1] = static_cast<unsigned char>(value >> 8);
    };
    auto put32 = [&header](std::size_t offset, std::uint32_t value) {
        for (int i = 0; i < 4; ++i)
            header[offset + i] = static_cast<unsigned char>((value >> (8 * i)) & 0xff);
    };

    header[0] = 'B';
    header[1] = 'M';
    put32(2, file_size);
    put32(10, 54);
    put32(14, 40);
    put32(18, static_cast<std::uint32_t>(width));
    put32(22, static_cast<std::uint32_t>(height));
    put16(26, 1);
    put16(28, 24);
    put32(34, static_cast<std::uint32_t>(image_size));

    FILE *file = std::fopen(path, "wb");
    if (!file)
        return false;
    bool ok = std::fwrite(header, 1, sizeof header, file) == sizeof header;

    std::vector<unsigned char> row(row_size, 0);
    if (SDL_LockSurface(surface)) {
        for (int y = height - 1; ok && y >= 0; --y) {
            const unsigned char *src = static_cast<const unsigned char *>(surface->pixels)
                                       + static_cast<std::size_t>(y) * surface->pitch;
            std::fill(row.begin(), row.end(), 0);
            for (int x = 0; x < width; ++x) {
                std::uint16_t pixel = 0;
                std::memcpy(&pixel, src + static_cast<std::size_t>(x) * 2, sizeof pixel);
                const unsigned red = (pixel >> 11) & 0x1f;
                const unsigned green = (pixel >> 5) & 0x3f;
                const unsigned blue = pixel & 0x1f;
                row[static_cast<std::size_t>(x) * 3 + 0] =
                    static_cast<unsigned char>(blue * 255 / 31);
                row[static_cast<std::size_t>(x) * 3 + 1] =
                    static_cast<unsigned char>(green * 255 / 63);
                row[static_cast<std::size_t>(x) * 3 + 2] =
                    static_cast<unsigned char>(red * 255 / 31);
            }
            ok = std::fwrite(row.data(), 1, row.size(), file) == row.size();
        }
        SDL_UnlockSurface(surface);
    } else {
        ok = false;
    }

    if (std::fclose(file) != 0)
        ok = false;
    return ok;
}

#endif
