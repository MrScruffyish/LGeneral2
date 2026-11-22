/***************************************************************************
                           event.c  -  description
                              -------------------
    begin                : Wed Mar 20 2002
    copyright            : (C) 2001 by Michael Speck
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

#include "lgeneral.h"
#include "event.h"

#include <cstring>

int buttonup = 0, buttondown = 0;
int motion = 0;
int motion_x = 50, motion_y = 50;
int sdl_quit = 0;

/* SDL3 exposes key state by scancode rather than by key symbol. */
static bool key_state[SDL_SCANCODE_COUNT];
static int buttonstate[10];

/*
=====================================================================
Locals
=====================================================================
*/

/*
====================================================================
Event filter that blocks all events. Used to clear the SDL
event queue.
====================================================================
*/
int all_filter( const SDL_Event *event )
{
    (void)event;
    return 0;
}

static bool SDLCALL sdl3_event_filter( void *userdata, SDL_Event *event )
{
    (void)userdata;
    return event_filter( event ) != 0;
}

/*
====================================================================
Returns whether any mouse button is pressed
====================================================================
*/
static int event_is_button_pressed()
{
    float x = 0.0f, y = 0.0f;
    return SDL_GetMouseState( &x, &y ) != 0;
}
static int event_is_key_pressed()
{
    const bool *keys = SDL_GetKeyboardState( nullptr );
    if ( !keys )
        return 0;
    for ( int i = 0; i < SDL_SCANCODE_COUNT; i++ ) {
        if ( i == SDL_SCANCODE_NUMLOCKCLEAR || i == SDL_SCANCODE_CAPSLOCK ||
             i == SDL_SCANCODE_SCROLLLOCK )
            continue;
        if ( keys[i] )
            return 1;
    }
    return 0;
}

/*
====================================================================
Event filter. As long as this is active no KEY or MOUSE events
will be available by SDL_PollEvent().
====================================================================
*/
/* SDL3 uses distinct button numbers for X1/X2; the game enum reserves 4/5 for
   wheel events inherited from SDL 1.2. Only forward the three real buttons. */
static int game_button( Uint8 button )
{
    switch ( button ) {
        case SDL_BUTTON_LEFT:   return BUTTON_LEFT;
        case SDL_BUTTON_MIDDLE: return BUTTON_MIDDLE;
        case SDL_BUTTON_RIGHT:  return BUTTON_RIGHT;
        default:                return BUTTON_NONE;
    }
}

int event_filter( const SDL_Event *event )
{
    switch ( event->type ) {
        case SDL_MOUSEMOTION:
            motion_x = static_cast<int>( event->motion.x );
            motion_y = static_cast<int>( event->motion.y );
            /* SDL3 reports the button state as a bit mask; map it to the
               button indices used by the game (the SDL 1.2 code indexed the
               state array with the mask, which mismatched the right button). */
            if ( event->motion.state & SDL_BUTTON_LMASK )
                buttonstate[BUTTON_LEFT] = 1;
            if ( event->motion.state & SDL_BUTTON_MMASK )
                buttonstate[BUTTON_MIDDLE] = 1;
            if ( event->motion.state & SDL_BUTTON_RMASK )
                buttonstate[BUTTON_RIGHT] = 1;
            motion = 1;
            return 0;
        case SDL_MOUSEBUTTONUP: {
            const int button = game_button( event->button.button );
            if ( button != BUTTON_NONE )
                buttonstate[button] = 0;
            buttonup = button;
            return 0;
        }
        case SDL_MOUSEBUTTONDOWN: {
            const int button = game_button( event->button.button );
            if ( button != BUTTON_NONE )
                buttonstate[button] = 1;
            buttondown = button;
            return 0;
        }
        case SDL_EVENT_MOUSE_WHEEL: {
            float y = event->wheel.y;
            if ( event->wheel.direction == SDL_MOUSEWHEEL_FLIPPED )
                y = -y;
            if ( y > 0 )
                buttonup = buttondown = WHEEL_UP;
            else if ( y < 0 )
                buttonup = buttondown = WHEEL_DOWN;
            return 0;
        }
        case SDL_KEYUP:
            key_state[event->key.scancode] = false;
            return 0;
        case SDL_KEYDOWN:
            key_state[event->key.scancode] = true;
            return 1;
        case SDL_QUIT:
            sdl_quit = 1;
            return 0;
        default:
            return 1;
    }
}

/*
====================================================================
Publics
====================================================================
*/

/*
====================================================================
Enable/Disable event filtering of mouse and keyboard.
====================================================================
*/
void event_enable_filter( void )
{
    SDL_SetEventFilter( sdl3_event_filter, nullptr );
    event_clear();
}
void event_disable_filter( void )
{
    SDL_SetEventFilter( nullptr, nullptr );
}
void event_clear( void )
{
    std::memset( key_state, 0, sizeof( key_state ) );
    std::memset( buttonstate, 0, sizeof( buttonstate ) );
    buttonup = buttondown = motion = 0;
}

/*
====================================================================
Check if an motion event occured since last call.
====================================================================
*/
int event_get_motion( int *x, int *y )
{
    if ( motion ) {
        *x = motion_x;
        *y = motion_y;
        motion = 0;
        return 1;
    }
    return 0;
}
/*
====================================================================
Check if a button was pressed and return its value.
====================================================================
*/
int event_get_buttondown( int *button, int *x, int *y )
{
    if ( buttondown ) {
        *button = buttondown;
        *x = motion_x;
        *y = motion_y;
        buttondown = 0;
        return 1;
    }
    return 0;
}
int event_get_buttonup( int *button, int *x, int *y )
{
    if ( buttonup ) {
        *button = buttonup;
        *x = motion_x;
        *y = motion_y;
        buttonup = 0;
        return 1;
    }
    return 0;
}

/*
====================================================================
Get current cursor position.
====================================================================
*/
void event_get_cursor_pos( int *x, int *y )
{
    *x = motion_x; *y = motion_y;
}

/*
====================================================================
Check if 'button' button is currently set.
====================================================================
*/
int event_check_button( int button )
{
    if ( button < 0 || button >= static_cast<int>( sizeof( buttonstate ) / sizeof( buttonstate[0] ) ) )
        return 0;
    return buttonstate[button];
}

/*
====================================================================
Check if 'key' is currently set.
====================================================================
*/
int event_check_key( int key )
{
    const SDL_Scancode scancode =
        SDL_GetScancodeFromKey( static_cast<SDL_Keycode>( key ), nullptr );
    if ( scancode < 0 || scancode >= SDL_SCANCODE_COUNT )
        return 0;
    return key_state[scancode] ? 1 : 0;
}

/*
====================================================================
Check if SDL_QUIT was received.
====================================================================
*/
int event_check_quit()
{
    return sdl_quit;
}

/*
====================================================================
Clear the SDL event key (keydown events)
====================================================================
*/
void event_clear_sdl_queue()
{
    SDL_Event event;
    SDL_SetEventFilter( []( void *, SDL_Event * ) { return false; }, nullptr );
    while ( SDL_PollEvent( &event ) );
    SDL_SetEventFilter( sdl3_event_filter, nullptr );
}

/*
====================================================================
Wait until neither a key nor a button is pressed.
====================================================================
*/
void event_wait_until_no_input()
{
    SDL_PumpEvents();
    while (event_is_button_pressed()||event_is_key_pressed())
    {
        SDL_Delay(20);
        SDL_PumpEvents();
    }
    event_clear();
}
