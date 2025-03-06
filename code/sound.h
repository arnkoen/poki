#pragma once
#include "hmm.h"
#include "mixer.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct pk_memory pk_memory;
    typedef struct pk_node pk_node;

    typedef struct pk_sound_channel_desc {
        const tinymixer_buffer* buffer;
        float range_min;
        float range_max;
        pk_node* node;
        bool should_play;
        bool loop;
    } pk_sound_channel_desc;

    typedef struct pk_sound {
        tinymixer_channel channel;
        pk_node* node;
        bool should_play;
        bool loop;
        bool ready;
    } pk_sound;

    void pk_init_sound(pk_sound* sound, const pk_sound_channel_desc* desc);
    void pk_update_sound(pk_sound* sound);
    void pk_release_sound(pk_sound* sound);

    typedef struct pk_sound_listener {
        HMM_Vec3 position;
        float smoothing;
    } pk_sound_listener;

    void pk_update_sound_listener(pk_sound_listener* listener, HMM_Vec3 new_pos, float dt);


#ifdef __cplusplus
} //extern "C"
#endif

