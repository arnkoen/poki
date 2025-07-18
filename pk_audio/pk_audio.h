#ifndef PK_AUDIO_H
#define PK_AUDIO_H

#ifndef PK_AUDIO_SINGLE_HEADER
#include "../poki.h"
#include "sokol_audio.h"
#include "tmixer.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pk_audio_desc {
    saudio_desc saudio;
    tm_callbacks mixer_callbacks;
} pk_audio_desc;

void pk_audio_setup(const pk_audio_desc* desc);
void pk_audio_shutdown(void);

/*
Options for playing a sound.
Filling in the node field will make the sound spatial.
In this case, you should also fill in range_min and range_max.
*/
typedef struct pk_sound_channel_desc {
    const tm_buffer* buffer;
    float range_min;
    float range_max;
    pk_node* node;
    bool loop;
} pk_sound_channel_desc;

typedef struct pk_sound {
    tm_channel channel;
    pk_node* node;
} pk_sound;

void pk_play_sound(pk_sound* sound, const pk_sound_channel_desc* desc);
void pk_update_sound(pk_sound* sound);
void pk_stop_sound(pk_sound* sound);

typedef struct pk_sound_listener {
    HMM_Vec3 position;
    float smoothing; // Around 2 is a good starting point.
} pk_sound_listener;

void pk_update_sound_listener(pk_sound_listener* listener, HMM_Vec3 new_pos, float dt);

//--loading------------------------------------------------------------

typedef void(*pk_sound_buffer_loaded_callback)(const tm_buffer* buffer);

typedef struct pk_sound_buffer_request {
    const char* path;
    sfetch_range_t buffer;
    pk_sound_buffer_loaded_callback loaded_cb;
    pk_fail_callback fail_cb;
} pk_sound_buffer_request;

sfetch_handle_t pk_load_sound_buffer(const pk_sound_buffer_request* req);
//since the header is included, just use tm_release_buffer(...) here.



#ifdef __cplusplus
}
#endif
#endif //PK_AUDIO_H


