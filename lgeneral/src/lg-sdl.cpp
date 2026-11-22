/***************************************************************************
                          lg-sdl  -  description
                             -------------------
    begin                : Thu Apr 20 2000
    copyright            : (C) 2000 by Michael Speck
    email                : kulkanie@gmx.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "lg-sdl.h"

#include "misc.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "localize.h"

extern int  term_game;

Sdl sdl;
SDL_Cursor *empty_cursor = 0, *std_cursor = 0;

static SDL_Window *game_window = 0;
static SDL_Surface *window_surface = 0; /* owned by SDL */
static bool game_fullscreen = false;
static int view_zoom = 1;
static int view_offset_x = 0, view_offset_y = 0;

SDL_Surface *lg_get_video_surface()
{
    return sdl.screen;
}

/* timer */
int cur_time, last_time;

/* sdl surface */

/* return full path of bitmap */
void get_full_bmp_path( char *full_path, const char *file_name )
{
    sprintf(full_path,  "%s/gfx/%s", get_gamedir(), file_name );
}

/*
    load a surface from file putting it in soft or hardware mem
*/
SDL_Surface* load_surf(const char *fname, int f)
{
    SDL_Surface *buf;
    SDL_Surface *new_sur;
    char path[ 512 ];
    SDL_PixelFormat format = SDL_PIXELFORMAT_RGBA8888;

    get_full_bmp_path( path, fname );

    buf = SDL_LoadBMP( path );

    if ( buf == 0 ) {
        fprintf( stderr, "%s: %s\n", fname, SDL_GetError() );
        if ( f & SDL_NONFATAL )
            return 0;
        else
            exit( 1 );
    }
/*    if ( !(f & SDL_HWSURFACE) ) {

        SDL_SetColorKey( buf, SDL_SRCCOLORKEY, 0x0 );
        return buf;

    }
    new_sur = create_surf(buf->w, buf->h, f);
    SDL_BlitSurface(buf, 0, new_sur, 0);
    SDL_FreeSurface(buf);*/
    if ( sdl.screen )
        format = sdl.screen->format;
    new_sur = ::SDL_ConvertSurface( buf, format );
    SDL_FreeSurface( buf );
    SDL_SetColorKey( new_sur, SDL_SRCCOLORKEY, 0x0 );
    SDL_SetAlpha( new_sur, 0, 0 ); /* no alpha */
    return new_sur;
}

/*
    create an surface
    MUST NOT BE USED IF NO SDLSCREEN IS SET
*/
SDL_Surface* create_surf(int w, int h, int f)
{
    SDL_Surface *sur;
    SDL_PixelFormat format = SDL_PIXELFORMAT_RGBA8888;
    if ( sdl.screen )
        format = sdl.screen->format;
    if ((sur = SDL_CreateSurface(w, h, format)) == 0) {
        fprintf(stderr, "ERR: ssur_create: not enough memory to create surface...\n");
        exit(1);
    }
/*    if (f & SDL_HWSURFACE && !(sur->flags & SDL_HWSURFACE))
        fprintf(stderr, "unable to create surface (%ix%ix%i) in hardware memory...\n", w, h, spf->BitsPerPixel);*/
    SDL_SetColorKey(sur, SDL_SRCCOLORKEY, 0x0);
    SDL_SetAlpha(sur, 0, 0); /* no alpha */
    return sur;
}

void free_surf( SDL_Surface **surf )
{
    if ( *surf ) {
        SDL_FreeSurface( *surf );
        *surf = 0;
    }
}
/*
    return display format
*/
int disp_format(SDL_Surface *sur)
{
    if ( !sur || !sdl.screen )
        return 1;
    SDL_Surface *converted = ::SDL_ConvertSurface( sur, sdl.screen->format );
    if ( !converted ) {
        fprintf(stderr, "ERR: ssur_displayformat: convertion failed\n");
        return 1;
    }
    SDL_FreeSurface( converted );
    return 0;
}

/*
    lock surface
*/
void lock_surf(SDL_Surface *sur)
{
    if (SDL_MUSTLOCK(sur))
        SDL_LockSurface(sur);
}

/*
    unlock surface
*/
void unlock_surf(SDL_Surface *sur)
{
    if (SDL_MUSTLOCK(sur))
        SDL_UnlockSurface(sur);
}

/*
    blit surface with destination DEST and source SOURCE using it's actual alpha and color key settings
*/
void blit_surf(void)
{
    SDL_BlitSurface(sdl.s.s, &sdl.s.r, sdl.d.s, &sdl.d.r);
}

/*
    do an alpha blit
*/
void alpha_blit_surf(int alpha)
{
    SDL_SetAlpha(sdl.s.s, SDL_SRCALPHA, alpha);
    SDL_BlitSurface(sdl.s.s, &sdl.s.r, sdl.d.s, &sdl.d.r);
    SDL_SetAlpha(sdl.s.s, 0, 0);
}

/*
    fill surface with color c
*/
void fill_surf(int c)
{
    SDL_FillRect(sdl.d.s, &sdl.d.r, SDL_MapRGB(sdl.d.s->format, c >> 16, (c >> 8) & 0xFF, c & 0xFF));
}

/* set clipping rect */
void set_surf_clip( SDL_Surface *surf, int x, int y, int w, int h )
{
    SDL_Rect rect = { (Sint16)x, (Sint16)y, (Uint16)w, (Uint16)h };
    if ( w == h || h == 0 )
        SDL_SetClipRect( surf, 0 );
    else
        SDL_SetClipRect( surf, &rect );
}

/* set pixel */
Uint32 set_pixel( SDL_Surface *surf, int x, int y, int pixel )
{
    int pos = 0;

    if ( !surf || x < 0 || y < 0 || x >= surf->w || y >= surf->h )
	    return pixel;

    const int bytes = LG_BYTES_PER_PIXEL( surf );
    pos = y * surf->pitch + x * bytes;
    memcpy( (char*)surf->pixels + pos, &pixel, bytes );
    return pixel;
}

/* get pixel */
Uint32 get_pixel( SDL_Surface *surf, int x, int y )
{
    int pos = 0;
    Uint32 pixel = 0;

    if ( !surf || x < 0 || y < 0 || x >= surf->w || y >= surf->h )
        return pixel;

    const int bytes = LG_BYTES_PER_PIXEL( surf );
    pos = y * surf->pitch + x * bytes;
    memcpy( &pixel, (char*)surf->pixels + pos, bytes );
    return pixel;
}

/* sdl font */

/* return full font path */
void get_full_font_path( char *path, const char *file_name )
{
    strcpy( path, file_name );
/*    sprintf(path, "./gfx/fonts/%s", file_name ); */
}

/*
 * Loads glyphs into a font from file 'fname'.
 *
 * Font format description:
 *
 * Each font file is a pixmap comprised of a single row of subsequent glyphs
 * defining the appearence of the associated character code.
 *
 * The height of the font is implicitly defined by the height of the pixmap.
 *
 * Each glyph is defined by a pixmap as depicted in the following
 * example (where each letter represents the respective rgb-value):
 *
 * (1) -----> ........
 *            ........
 *            ..####..
 *            .#....#.
 *            .#....#.
 *            .#....#.
 *            .#....#.
 *            .#....#.
 *            .#.#..#.
 *            .#..#.#.
 *            ..####..
 *            ......#.
 * (2, 3) --> S.......
 *
 * (1): The top left pixel defines the transparency pixel. All pixels of
 * the same color as (1) are treated as transparent when the glyph is
 * rendered.
 * Note that (1) will only be checked for the *first* glyph in
 * the file and be applied to all other glyphs.
 *
 * (2): The bottom left pixel defines the starting column of a particular
 * glyph within the font pixmap. By scanning these pixels, the font loader
 * is able to determine the width of the glyph.
 * The color is either #FF00FF for a valid glyph, or #FF0000 for an invalid
 * glyph (in this case it will not be rendered,
 * regardless of what it contains otherwise).
 *
 * (3): For the very first glyph, (2) has a special meaning. It provides
 * some basic information about the font to be loaded.
 * The r-value specifies the starting code of the first glyph.
 * The g- and b-values are reserved and must be (0)
 *
 * The glyph-to-character-code-mapping is done by starting with the code
 * specified by (3). Then the bottom line is scanned: For every occurrence of
 * (2) the code will be incremented, and appropriate offset and width values
 * be calculated. The total count of characters will be determined by the
 * count of glyphs contained within the file.
 *
 * For display, (2) and (3) will be assumed to have color (1).
 */
void font_load_glyphs(Font *font, const char *fname)
{
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#  define R_INDEX 1
#  define G_INDEX 2
#  define B_INDEX 3
#else
#  define R_INDEX 2
#  define G_INDEX 1
#  define B_INDEX 0
#endif
#define AT_RGB(pixel, idx) (((Uint8 *)&(pixel))+(idx))
    char    path[512];
    int     i;
    Uint32  transparency_key;
    Uint8   start_code, reserved1, reserved2;
    SDL_Surface *new_glyphs;

    get_full_font_path( path, fname );

    if ((new_glyphs = load_surf(path, SDL_SWSURFACE)) == 0) {
        fprintf(stderr, tr("Cannot load new glyphs surface: %s\n"), SDL_GetError());
        exit(1);
    }
    /* use (1) as transparency key */
    transparency_key = get_pixel( new_glyphs, 0, 0 );

    /* evaluate (3) */
    SDL_GetRGB(get_pixel( new_glyphs, 0, new_glyphs->h - 1 ), new_glyphs->format, &start_code, &reserved1, &reserved2);
    // bail out early and consistently for invalid input to minimise the amount
    // of font files out of spec
    if (reserved1 || reserved2) {
        fprintf(stderr, tr("Invalid font file: %d, %d\n"), reserved1, reserved2);
        exit(1);
    }

    /* update font data */
    /* FIXME: don't blindly override, merge smartly */
    font->height = new_glyphs->h;
	
    /* cut prevalent glyph row at where to insert the new glyph row,
     * and insert it, thus overriding only the glyph range that is defined
     * by new_glyphs.
     */
    {
        /* total width of previous conserved glyphs */
        int pre_width = font->char_offset[start_code];
        /* total width of following conserved glyphs */
        int post_width = (font->pic ? font->pic->w : 0) - pre_width;
        unsigned code = start_code;
        int fixup;	/* amount of pixels following offsets have to be fixed up */
        SDL_Surface *dest;
        
        /* override widths and offsets of new glyphs */
        /* concurrently calculate width of following conserved glyphs */
        for (i = 0; i < new_glyphs->w; i++) {
            Uint32 pixel = 0;
            int valid_glyph;
            SDL_GetRGB(get_pixel(new_glyphs, i, new_glyphs->h - 1), new_glyphs->format, AT_RGB(pixel, R_INDEX), AT_RGB(pixel, G_INDEX), AT_RGB(pixel, B_INDEX));
            pixel &= 0xf8f8f8;	/* cope with RGB565 and other perversions */
            if (i != 0 && pixel != 0xf800f8 && pixel != 0xf80000) continue;

            if (code >= 256) {
                fprintf(stderr, tr("font '%s' contains too many glyphs\n"), path);
                break;
            }

            valid_glyph = i == 0 || pixel != 0xf80000;
            set_pixel(new_glyphs, i, new_glyphs->h - 1, transparency_key);
            
            font->char_offset[code] = pre_width + i;
            font->keys[code] = valid_glyph;
            code++;
            
        }
        
        fixup = pre_width + i - font->char_offset[code];
        post_width -= font->char_offset[code] - font->char_offset[start_code];
        
        /* now the gory part:
         * 1. create a new surface large enough to hold conserved and new glyphs.
         * 2. blit the conserved previous part.
         * 3. blit the conserved following part.
         * 4. blit the new glyphs.
         */
        dest = create_surf(pre_width + new_glyphs->w + post_width,
        		   font->pic ? font->pic->h : new_glyphs->h, SDL_HWSURFACE);
        if (!dest) {
            fprintf(stderr, tr("could not create font surface: %s\n"), SDL_GetError());
            exit(1);
        }

        if (pre_width > 0) {
            assert(font->pic);
            DEST(dest, 0, 0, pre_width, font->height);
            SOURCE(font->pic, 0, 0);
            blit_surf();
        }
        
        if (post_width > 0) {
            assert(font->pic);
            DEST(dest, font->char_offset[code] + fixup, 0, post_width, font->height);
            SOURCE(font->pic, font->char_offset[code], 0);
            blit_surf();
        }
        
        DEST(dest, font->char_offset[start_code], 0, new_glyphs->w, font->height);
        FULL_SOURCE(new_glyphs);
        blit_surf();
        
        /* replace old row with newly composed row */
        SDL_FreeSurface(font->pic);
        font->pic = dest;

        /* fix up offsets of successors */
        for (i = code; i < 256; i++)
            font->char_offset[i] += fixup;
        font->width += fixup;
    }
    
    SDL_SetColorKey( font->pic, SDL_SRCCOLORKEY, transparency_key );

    SDL_FreeSurface(new_glyphs);
#undef R_INDEX
#undef G_INDEX
#undef B_INDEX
#undef AT_RGB
}

/**
 * create a new font from font file 'fname'
 */
Font* load_font(const char *fname)
{
    Font *fnt = (Font*)calloc(1, sizeof(Font));
    if (fnt == 0) {
        fprintf(stderr, tr("ERR: %s: not enough memory\n"), __FUNCTION__);
        exit(1);
    }
    
    fnt->align = ALIGN_X_LEFT | ALIGN_Y_TOP;
    fnt->color = 0x00FFFFFF;
    
    font_load_glyphs(fnt, fname);
    return fnt;
}

/*
    free memory
*/
void free_font(Font **fnt)
{
    if ( *fnt ) {
        if ((*fnt)->pic) SDL_FreeSurface((*fnt)->pic);
        free(*fnt);
        *fnt = 0;
    }
}

/*
    write something with transparency
*/
int write_text(Font *fnt, SDL_Surface *dest, int x, int y, const char *str, int alpha)
{
    int len = strlen(str);
    int pix_len = text_width(fnt, str);
    int px = x, py = y;
    int i;
    SDL_Surface *spf = SDL_GetVideoSurface();
    const int * const ofs = fnt->char_offset;
	
    /* alignment */
    if (fnt->align & ALIGN_X_CENTER)
        px -= pix_len >> 1;
    else if (fnt->align & ALIGN_X_RIGHT)
        px -= pix_len;
    if (fnt->align & ALIGN_Y_CENTER)
        py -= (fnt->height >> 1 ) + 1;
    else
        if (fnt->align & ALIGN_Y_BOTTOM)
            py -= fnt->height;

    fnt->last_x = MAXIMUM(px, 0);
    fnt->last_y = MAXIMUM(py, 0);
    fnt->last_width = MINIMUM(pix_len, spf->w - fnt->last_x);
    fnt->last_height = MINIMUM(fnt->height, spf->h - fnt->last_y);

    if (alpha != 0)
        SDL_SetAlpha(fnt->pic, SDL_SRCALPHA, alpha);
    else
        SDL_SetAlpha(fnt->pic, 0, 0);
    for (i = 0; i < len; i++) {
        unsigned c = (unsigned char)str[i];
        if (!fnt->keys[c]) c = '\177';
        {
            const int w = ofs[c+1] - ofs[c];
            DEST(dest, px, py, w, fnt->height);
            SOURCE(fnt->pic, ofs[c], 0);
            blit_surf();
            px += w;
        }
    }
    
    return 0;
}

/*
====================================================================
Write string to x, y and modify y so that it draws to the 
next line.
====================================================================
*/
void write_line( SDL_Surface *surf, Font *font, const char *str, int x, int *y )
{
    write_text( font, surf, x, *y, str, 255 );
    *y += font->height;
}

/*
    lock font surface
*/
void lock_font(Font *fnt)
{
    if (SDL_MUSTLOCK(fnt->pic))
        SDL_LockSurface(fnt->pic);
}

/*
    unlock font surface
*/
void unlock_font(Font *fnt)
{
    if (SDL_MUSTLOCK(fnt->pic))
        SDL_UnlockSurface(fnt->pic);
}
	
/*
    return last update region
*/
SDL_Rect last_write_rect(Font *fnt)
{
    SDL_Rect    rect={(Sint16)fnt->last_x, (Sint16)fnt->last_y, (Uint16)fnt->last_width, (Uint16)fnt->last_height};
    return rect;
}

int  char_width(Font *fnt, char c)
{
    unsigned i = (unsigned char)c;
    return fnt->char_offset[i + 1] - fnt->char_offset[i];
}

/*
    return the text width in pixels
*/
int text_width(Font *fnt, const char *str)
{
    unsigned int i;
    int pix_len = 0;
    for (i = strlen(str); i > 0; )
        pix_len += char_width(fnt, str[--i]);
    return pix_len;
}

/* sdl */

/*
    initialize sdl
*/
void init_sdl( int f )
{
    /* check flags: if SOUND is not enabled flag SDL_INIT_AUDIO musn't be set */
#ifndef WITH_SOUND
    if ( f & SDL_INIT_AUDIO )
        f = f & ~SDL_INIT_AUDIO;
#endif

    sdl.screen = 0;
    if (!SDL_Init(f)) {
        fprintf(stderr, "ERR: sdl_init: %s", SDL_GetError());
        exit(1);
    }
    atexit(quit_sdl);
    /* create empty cursor */
    empty_cursor = create_cursor( 16, 16, 8, 8,
                                  "                "
                                  "                "
                                  "                "
                                  "                "
                                  "                "
                                  "                "
                                  "                "
                                  "                "
                                  "                "
                                  "                "
                                  "                "
                                  "                "
                                  "                "
                                  "                "
                                  "                "
                                  "                " );
    std_cursor = SDL_GetDefaultCursor();
}

/*
    free screen
*/
void quit_sdl()
{
    /* window_surface belongs to the window and is released by SDL_Quit(). */
    if ( game_window )
        SDL_StopTextInput( game_window );
    if ( sdl.screen && sdl.screen != window_surface )
        SDL_FreeSurface( sdl.screen );
    sdl.screen = 0;
    window_surface = 0;
    game_window = 0;
    if ( empty_cursor ) SDL_FreeCursor( empty_cursor );
    if (sdl.vmodes) free(sdl.vmodes);
    SDL_Quit();
    printf("SDL finalized\n");
}

void set_window_title( const char *title )
{
    if ( game_window )
        SDL_SetWindowTitle( game_window, title );
}

/** Get list of all video modes. Allocate @vmi and return number of
 * entries. */
int get_video_modes( VideoModeInfo **vmi )
{
    int i, count = 0;
    int bpp = 32;
    SDL_DisplayMode **modes = 0;
    SDL_Surface *screen = lg_get_video_surface();

    *vmi = 0;
    if ( screen )
        bpp = SDL_GetPixelFormatDetails( screen->format )->bits_per_pixel;

    modes = SDL_GetFullscreenDisplayModes( SDL_GetPrimaryDisplay(), &count );
    if ( !modes || count <= 0 ) {
        VideoModeInfo stdvmi[2] = {
            { 800, 600, 32, 0 },
            { 800, 600, 32, 1 }
        };
        fprintf(stderr,"No video modes available or specified.\n");
        *vmi = (VideoModeInfo*)calloc(2, sizeof(VideoModeInfo));
        (*vmi)[0] = stdvmi[0];
        (*vmi)[1] = stdvmi[1];
        return 2;
    }

    *vmi = (VideoModeInfo*)calloc(static_cast<std::size_t>(count) * 2,
                                  sizeof(VideoModeInfo));
    for ( i = 0; i < count; i++ ) {
        VideoModeInfo *info = &((*vmi)[i * 2]);
        /* Ignore tiny modes so the GUI keeps its minimum layout. */
        if ( modes[i]->w < 640 || modes[i]->h < 480 ) {
            info->width = 800;
            info->height = 600;
        } else {
            info->width = modes[i]->w;
            info->height = modes[i]->h;
        }
        info->depth = bpp;
        info->fullscreen = 0;
        (*vmi)[i * 2 + 1] = *info;
        (*vmi)[i * 2 + 1].fullscreen = 1;
    }
    SDL_free( modes );
    return count * 2;
}

/** Smallest logical view that still fits the fixed GUI widgets. */
void lg_min_logical_size( int zoom, int *width, int *height )
{
    if ( zoom <= LG_ZOOM_MIN ) {
        if ( width ) *width = 800;
        if ( height ) *height = 600;
    }
    else {
        if ( width ) *width = 640;
        if ( height ) *height = 480;
    }
}

/** Copy a logical rectangle to the window surface, magnifying whole pixels. */
static void lg_present_region( int x, int y, int w, int h )
{
    if ( !game_window || !window_surface || !sdl.screen )
        return;
    const bool full = w <= 0 || h <= 0;
    if ( full ) {
        x = 0; y = 0;
        w = sdl.screen->w; h = sdl.screen->h;
        SDL_FillRect( window_surface, nullptr,
                      SDL_MapRGB( window_surface->format, 0, 0, 0 ) );
    }
    if ( x < 0 ) { w += x; x = 0; }
    if ( y < 0 ) { h += y; y = 0; }
    if ( x + w > sdl.screen->w ) w = sdl.screen->w - x;
    if ( y + h > sdl.screen->h ) h = sdl.screen->h - y;
    if ( w <= 0 || h <= 0 )
        return;

    SDL_Rect src = { x, y, w, h };
    SDL_Rect dst = { view_offset_x + x * view_zoom,
                     view_offset_y + y * view_zoom,
                     w * view_zoom, h * view_zoom };
    if ( !SDL_BlitSurfaceScaled( sdl.screen, &src, window_surface, &dst,
                                 SDL_SCALEMODE_NEAREST ) )
        fprintf( stderr, "%s", SDL_GetError() );
    if ( full )
        SDL_UpdateWindowSurface( game_window );
    else
        SDL_UpdateWindowSurfaceRects( game_window, &dst, 1 );
}

/*
====================================================================
Switch to passed video mode.
====================================================================
*/
int set_video_mode( int width, int height, int fullscreen, int zoom )
{
    if ( zoom < LG_ZOOM_MIN || zoom > LG_ZOOM_MAX ) {
        fprintf( stderr, "Invalid zoom level %d\n", zoom );
        return 1;
    }
    int min_w = 0, min_h = 0;
    lg_min_logical_size( zoom, &min_w, &min_h );
    if ( width < min_w ) width = min_w;
    if ( height < min_h ) height = min_h;

    const bool want_fullscreen = fullscreen != 0;
    const bool fullscreen_changed = game_fullscreen != want_fullscreen;
    int physical_w = width * zoom, physical_h = height * zoom;
    int current_w = 0, current_h = 0;
    const bool size_changed = !game_window ||
        ( lg_get_window_size( &current_w, &current_h ) &&
          ( current_w != physical_w || current_h != physical_h ) );

    if ( !game_window ) {
        game_window = SDL_CreateWindow( "LGeneral", physical_w, physical_h,
                                        SDL_WINDOW_RESIZABLE );
        if ( !game_window ) {
            fprintf(stderr, "%s", SDL_GetError());
            return 1;
        }
        SDL_SetWindowMinimumSize( game_window, min_w * zoom, min_h * zoom );
    } else {
        /* Apply the window size after leaving fullscreen; some platforms ignore
         * SDL_SetWindowSize() while the window is fullscreen. */
        if ( game_fullscreen && !want_fullscreen )
            if ( !SDL_SetWindowFullscreen( game_window, false ) ) {
                fprintf(stderr, "%s", SDL_GetError());
                return 1;
            }
        if ( size_changed )
            if ( !SDL_SetWindowSize( game_window, physical_w, physical_h ) ) {
                fprintf(stderr, "%s", SDL_GetError());
                return 1;
            }
        if ( fullscreen_changed )
            if ( !SDL_SetWindowFullscreen( game_window, want_fullscreen ) ) {
                fprintf(stderr, "%s", SDL_GetError());
                return 1;
            }
    }
    game_fullscreen = want_fullscreen;
    /* SDL3 invalidates the window surface whenever the window changes, so
     * always fetch it again instead of relying on the old pointer. */
    window_surface = SDL_GetWindowSurface( game_window );
    if ( !window_surface ) {
        fprintf(stderr, "%s", SDL_GetError());
        return 1;
    }
    /* Some platforms ignore SDL_SetWindowSize() in fullscreen. Keep the fixed
     * GUI layout intact and lower the zoom to the largest integer that fits
     * the mode SDL actually gave us. */
    if ( want_fullscreen &&
         ( window_surface->w < physical_w || window_surface->h < physical_h ) ) {
        const int fit_x = window_surface->w / width;
        const int fit_y = window_surface->h / height;
        zoom = fit_x < fit_y ? fit_x : fit_y;
        if ( zoom < LG_ZOOM_MIN ) {
            zoom = LG_ZOOM_MIN;
            width = window_surface->w;
            height = window_surface->h;
        }
        else {
            lg_min_logical_size( zoom, &min_w, &min_h );
            if ( width < min_w ) width = min_w;
            if ( height < min_h ) height = min_h;
        }
        physical_w = width * zoom;
        physical_h = height * zoom;
    }
    view_zoom = zoom;
    view_offset_x = window_surface->w > physical_w
                    ? ( window_surface->w - physical_w ) / 2 : 0;
    view_offset_y = window_surface->h > physical_h
                    ? ( window_surface->h - physical_h ) / 2 : 0;

    /* The game draws into a logical surface; presentation scales it into the
     * window surface. Unlike the old window surface, this one has no colour
     * key, so black pixels remain opaque. */
    SDL_Surface *old_screen = sdl.screen;
    SDL_Surface *new_screen = SDL_CreateSurface( width, height,
                                                 window_surface->format );
    if ( !new_screen ) {
        fprintf(stderr, "Could not create logical surface: %s", SDL_GetError());
        return 1;
    }
    sdl.screen = new_screen;
    if ( old_screen && old_screen != window_surface )
        SDL_FreeSurface( old_screen );
    lg_present_region( 0, 0, 0, 0 );
    /* SDL3 does not deliver SDL_EVENT_TEXT_INPUT unless it is started. */
    if ( !SDL_TextInputActive( game_window ) )
        SDL_StartTextInput( game_window );
    return 0;
}

/** Return the drawable size of the game window. */
int lg_get_window_size( int *width, int *height )
{
    if ( !game_window )
        return 0;
    return SDL_GetWindowSizeInPixels( game_window, width, height ) ? 1 : 0;
}

/** Return the transform used to present the logical surface. */
int lg_get_view_transform( int *zoom, int *offset_x, int *offset_y )
{
    if ( zoom ) *zoom = view_zoom;
    if ( offset_x ) *offset_x = view_offset_x;
    if ( offset_y ) *offset_y = view_offset_y;
    return 1;
}

/** Return whether the game window is currently fullscreen. */
int lg_is_fullscreen()
{
    return game_fullscreen ? 1 : 0;
}

/*
    show hardware capabilities
*/
void hardware_cap()
{
    printf("video hardware capabilities:\n");
    printf("Using SDL3 window surface (software blitting)\n");
    printf("------\n");
}

/*
    update rectangle (0,0,0,0)->fullscreen
*/
void refresh_screen(int x, int y, int w, int h)
{
    if ( !game_window )
        return;
    lg_present_region( x, y, w, h );
    sdl.rect_count = 0;
}

/*
    draw all update regions
*/
void refresh_rects()
{
    if ( !game_window )
        return;
    for ( int i = 0; i < sdl.rect_count; i++ )
        lg_present_region( sdl.rect[i].x, sdl.rect[i].y,
                           sdl.rect[i].w, sdl.rect[i].h );
    sdl.rect_count = 0;
}

/*
    add update region/rect
*/
void add_refresh_region( int x, int y, int w, int h )
{
    if (sdl.rect_count == RECT_LIMIT) return;
    if (x < 0) {
        w += x;
        x = 0;
    }
    if (y < 0) {
        h += y;
        y = 0;
    }
    if (x + w > sdl.screen->w)
        w = sdl.screen->w - x;
    if (y + h > sdl.screen->h)
        h = sdl.screen->h - y;
    if (w <= 0 || h <= 0)
        return;
    sdl.rect[sdl.rect_count].x = x;
    sdl.rect[sdl.rect_count].y = y;
    sdl.rect[sdl.rect_count].w = w;
    sdl.rect[sdl.rect_count].h = h;
    sdl.rect_count++;
}
void add_refresh_rect( SDL_Rect *rect )
{
    if ( rect )
        add_refresh_region( rect->x, rect->y, rect->w, rect->h );
}

/*
    fade screen to black
*/
void dim_screen(int steps, int delay, int trp)
{
#ifndef NODIM
    SDL_Surface    *buffer;
    int per_step = trp / steps;
    int i;
    if (term_game) return;
    buffer = create_surf(sdl.screen->w, sdl.screen->h, SDL_SWSURFACE);
    SDL_SetColorKey(buffer, 0, 0);
    FULL_DEST(buffer);
    FULL_SOURCE(sdl.screen);
    blit_surf();
    for (i = 0; i <= trp; i += per_step) {
        FULL_DEST(sdl.screen);
        fill_surf(0x0);
        FULL_SOURCE(buffer);
        alpha_blit_surf(i);
        refresh_screen( 0, 0, 0, 0);
        SDL_Delay(delay);
    }
    if (trp == 255) {
        FULL_DEST(sdl.screen);
        fill_surf(0x0);
        refresh_screen( 0, 0, 0, 0);
    }
    SDL_FreeSurface(buffer);
#else
    refresh_screen( 0, 0, 0, 0);
#endif
}

/*
    undim screen
*/
void undim_screen(int steps, int delay, int trp)
{
#ifndef NODIM
    SDL_Surface    *buffer;
    int per_step = trp / steps;
    int i;
    if (term_game) return;
    buffer = create_surf(sdl.screen->w, sdl.screen->h, SDL_SWSURFACE);
    SDL_SetColorKey(buffer, 0, 0);
    FULL_DEST(buffer);
    FULL_SOURCE(sdl.screen);
    blit_surf();
    for (i = trp; i >= 0; i -= per_step) {
        FULL_DEST(sdl.screen);
        fill_surf(0x0);
        FULL_SOURCE(buffer);
        alpha_blit_surf(i);
        refresh_screen( 0, 0, 0, 0);
        SDL_Delay(delay);
    }
    FULL_DEST(sdl.screen);
    FULL_SOURCE(buffer);
    blit_surf();
    refresh_screen( 0, 0, 0, 0);
    SDL_FreeSurface(buffer);
#else
    refresh_screen( 0, 0, 0, 0);
#endif
}

/*
    wait for a key
*/
int wait_for_key()
{
    /* wait for key */
    SDL_Event event;
    while (1) {
        SDL_WaitEvent(&event);
        if (event.type == SDL_QUIT) {
            term_game = 1;
            return 0;
        }
        if (event.type == SDL_KEYUP)
            return event.key.key;
    }
}

/*
    wait for a key or mouse click
*/
void wait_for_click()
{
    /* wait for key or button */
    SDL_Event event;
    while (1) {
        SDL_WaitEvent(&event);
        if (event.type == SDL_QUIT) {
            term_game = 1;
            return;
        }
        if (event.type == SDL_KEYUP || event.type == SDL_MOUSEBUTTONUP)
            return;
    }
}

/*
    lock surface
*/
void lock_screen()
{
    if (SDL_MUSTLOCK(sdl.screen))
        SDL_LockSurface(sdl.screen);
}

/*
    unlock surface
*/
void unlock_screen()
{
    if (SDL_MUSTLOCK(sdl.screen))
        SDL_UnlockSurface(sdl.screen);
}

/*
    flip hardware screens (double buffer)
*/
void flip_screen()
{
    lg_present_region( 0, 0, 0, 0 );
}

/* cursor */

/* creates cursor */
SDL_Cursor* create_cursor( int width, int height, int hot_x, int hot_y, const char *source )
{
    unsigned char *mask = 0, *data = 0;
    SDL_Cursor *cursor = 0;
    int i, j, k;
    char data_byte, mask_byte;
    int pot;

    /* meaning of char from source:
        b : black, w: white, ' ':transparent */

    /* create mask&data */
    mask = (unsigned char*)malloc( width * height * sizeof ( char ) / 8 );
    data = (unsigned char*)malloc( width * height * sizeof ( char ) / 8 );

    k = 0;
    for (j = 0; j < width * height; j += 8, k++) {

        pot = 1;
        data_byte = mask_byte = 0;
        /* create byte */
        for (i = 7; i >= 0; i--) {

            switch ( source[j + i] ) {

                case 'b':
                    data_byte += pot;
                case 'w':
                    mask_byte += pot;
                    break;

            }
            pot *= 2;

        }
        /* add to mask */
        data[k] = data_byte;
        mask[k] = mask_byte;

    }

    /* create and return cursor */
    cursor = SDL_CreateCursor( data, mask, width, height, hot_x, hot_y );
    free( mask );
    free( data );
    return cursor;
}

/*
    get milliseconds since last call
*/
int get_time()
{
    int ms;
    cur_time = SDL_GetTicks();
    ms = cur_time - last_time;
    last_time = cur_time;
    if (ms == 0) {
        ms = 1;
        SDL_Delay(1);
    }
    return ms;
}

/*
    reset timer
*/
void reset_timer()
{
    last_time = SDL_GetTicks();
}
