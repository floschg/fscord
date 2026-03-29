#ifndef SOUND_H
#define SOUND_H


#include <basic/arena.h>
#include <os/os.h>
#include <basic/basic.h>

typedef enum {
    SOUND_USER_CONNECTED,
    SOUND_USER_DISCONNECTED
} SoundId;

typedef struct {
    i32 samples_per_second;
    i32 sample_count;
    i16 *samples;
} Sound;

typedef struct {
    i32 play_cursor;
    Sound *sound;
} PlaySound;

void play_sound_init(PlaySound *ps, Sound *sound);
void play_sound_update(PlaySound *ps, OSSoundBuffer *dest);
Sound *sound_load(SoundId id);


#endif // SOUND_H
