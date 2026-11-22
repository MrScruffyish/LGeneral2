/***************************************************************************
                          audio.c  -  description
                             -------------------
    begin                : Sun Jul 29 2001
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

#ifdef WITH_SOUND

#include "sdl_compat.h"
#include <SDL3_mixer/SDL_mixer.h>
#include <stdio.h>
#include <stdlib.h>

#include <string>

#include "audio.h"
#include "misc.h"

namespace {

/* The game uses three logical sound channels: movement, UI and explosions. */
constexpr int AUDIO_CHANNEL_COUNT = 16;

MIX_Mixer *audio_mixer = nullptr;
MIX_Track *audio_tracks[AUDIO_CHANNEL_COUNT] = {};

bool valid_channel( int channel )
{
    return channel >= 0 && channel < AUDIO_CHANNEL_COUNT;
}

MIX_Track *channel_track( int channel )
{
    if ( !audio_mixer || !valid_channel( channel ) )
        return nullptr;
    MIX_Track *&track = audio_tracks[channel];
    if ( !track ) {
        track = MIX_CreateTrack( audio_mixer );
        if ( !track )
            fprintf( stderr, "%s\n", SDL_GetError() );
    }
    return track;
}

void fade_channel( int channel, int ms )
{
    MIX_Track *track = valid_channel( channel ) ? audio_tracks[channel] : nullptr;
    if ( !track )
        return;
    Sint64 frames = ms > 0 ? MIX_TrackMSToFrames( track, ms ) : 0;
    if ( frames < 0 )
        frames = 0;
    if ( !MIX_StopTrack( track, frames ) )
        fprintf( stderr, "%s\n", SDL_GetError() );
}

} // namespace

/*
====================================================================
If audio device was properly initiated this flag is set.
If this flag is not set; no action will be taken for audio.
====================================================================
*/
int audio_ok = 0;
/*
====================================================================
If this flag is not set no sound is played.
====================================================================
*/
int audio_enabled = 1;
/*
====================================================================
Volume of all sounds
====================================================================
*/
int audio_volume = 128;

/*
====================================================================
Initiate/close audio
====================================================================
*/
int audio_open()
{
    if ( audio_ok )
        return 1;
    if ( !MIX_Init() ) {
        fprintf( stderr, "%s\n", SDL_GetError() );
        return 0;
    }

    SDL_AudioSpec spec;
    SDL_zero( spec );
    spec.freq = 22050;
    spec.format = SDL_AUDIO_S16;
    spec.channels = 2;
    audio_mixer = MIX_CreateMixerDevice( SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec );
    if ( !audio_mixer ) {
        fprintf( stderr, "%s\n", SDL_GetError() );
        MIX_Quit();
        return 0;
    }

    audio_ok = 1;
    audio_set_volume( audio_volume );
    return 1;
}

void audio_close()
{
    if ( !audio_ok )
        return;
    /* Destroying the mixer destroys its tracks; the Wav objects destroy their
       audio before this point. */
    MIX_DestroyMixer( audio_mixer );
    audio_mixer = nullptr;
    for ( int i = 0; i < AUDIO_CHANNEL_COUNT; i++ )
        audio_tracks[i] = nullptr;
    MIX_Quit();
    audio_ok = 0;
}

/*
====================================================================
Modify audio
====================================================================
*/
void audio_enable( int enable )
{
    if ( !audio_ok )
        return;
    audio_enabled = enable;
    if ( !enable ) {
        for ( int i = 0; i < AUDIO_CHANNEL_COUNT; i++ ) {
            if ( audio_tracks[i] )
                MIX_PauseTrack( audio_tracks[i] );
        }
    } else {
        for ( int i = 0; i < AUDIO_CHANNEL_COUNT; i++ ) {
            if ( audio_tracks[i] && MIX_TrackPaused( audio_tracks[i] ) )
                MIX_ResumeTrack( audio_tracks[i] );
        }
    }
}

void audio_set_volume( int level )
{
    if ( !audio_ok )
        return;
    if ( level < 0 ) level = 0;
    if ( level > 128 ) level = 128;
    audio_volume = level;
    if ( !MIX_SetMixerGain( audio_mixer, static_cast<float>( level ) / 128.0f ) )
        fprintf( stderr, "%s\n", SDL_GetError() );
}

void audio_fade_out( int channel, int ms )
{
    if ( !audio_ok )
        return;
    fade_channel( channel, ms );
}

/*
====================================================================
Wave
====================================================================
*/
Wav* wav_load( char *fname, int channel )
{
    if ( !audio_ok )
        return nullptr;
    const std::string path = std::string( get_gamedir() ) + "/sounds/" + fname;
    MIX_Audio *audio = MIX_LoadAudio( audio_mixer, path.c_str(), true );
    if ( !audio ) {
        fprintf( stderr, "%s\n", SDL_GetError() );
        return nullptr;
    }

    Wav *wav = static_cast<Wav*>( calloc( 1, sizeof( Wav ) ) );
    if ( !wav ) {
        MIX_DestroyAudio( audio );
        return nullptr;
    }
    wav->audio = audio;
    wav->channel = valid_channel( channel ) ? channel : -1;
    return wav;
}

void wav_free( Wav *wav )
{
    if ( !audio_ok || !wav )
        return;
    if ( wav->audio ) {
        MIX_Track *track = valid_channel( wav->channel ) ? audio_tracks[wav->channel] : nullptr;
        if ( track && MIX_GetTrackAudio( track ) == wav->audio )
            MIX_SetTrackAudio( track, nullptr );
        MIX_DestroyAudio( wav->audio );
    }
    free( wav );
}

void wav_set_channel( Wav *wav, int channel )
{
    if ( !wav )
        return;
    wav->channel = valid_channel( channel ) ? channel : -1;
}

void wav_play( Wav *wav )
{
    if ( !audio_ok || !audio_enabled || !wav || !wav->audio )
        return;
    MIX_Track *track = channel_track( wav->channel );
    if ( !track )
        return;
    if ( !MIX_SetTrackAudio( track, wav->audio ) ) {
        fprintf( stderr, "%s\n", SDL_GetError() );
        return;
    }
    if ( !MIX_PlayTrack( track, 0 ) )
        fprintf( stderr, "%s\n", SDL_GetError() );
}

void wav_play_at( Wav *wav, int channel )
{
    if ( !wav )
        return;
    wav->channel = valid_channel( channel ) ? channel : -1;
    wav_play( wav );
}

void wav_fade_out( Wav *wav, int ms )
{
    if ( !wav )
        return;
    fade_channel( wav->channel, ms );
}

#endif
