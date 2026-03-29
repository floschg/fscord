#include <client/sound.h>
#include <client/generated/asset_sound_user_connected.c>
#include <client/generated/asset_sound_user_disconnected.c>


void
play_sound_update(PlaySound *ps, OSSoundBuffer *dest)
{
    if (!dest) {
        return;
    }

    i32 samples_readable = ps->sound->sample_count - ps->play_cursor;
    i32 samples_writable = dest->sample_count;
    i32 samples_to_play;
    if (samples_readable > samples_writable) {
        samples_to_play = samples_writable;
    } else {
        samples_to_play = samples_readable;
    }

    i16 *from = &ps->sound->samples[ps->play_cursor];
    i16 *to = dest->samples;
    for (i32 i = 0; i < samples_to_play; i++) {
        to[i] = from[i];
    }
    
    ps->play_cursor += samples_to_play;
}


void
play_sound_init(PlaySound *ps, Sound *sound)
{
    ps->play_cursor = 0;
    ps->sound = sound;
}

Sound *
sound_load(SoundId id)
{
    if (id == 0) {
        Sound *sound = (Sound*)(g_asset_sound_user_connected);
        sound->samples = (i16*)(sound + 1);
        return sound;
    } else {
        Sound *sound = (Sound*)(g_asset_sound_user_disconnected);
        sound->samples = (i16*)(sound + 1);
        return sound;
    }
}

