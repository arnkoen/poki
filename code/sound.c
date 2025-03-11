#include "sound.h"
#include "node.h"
#include "common.h"
#include "log.h"
#include "io.h"

void pk_init_sound(pk_sound* sound, const pk_sound_channel_desc* desc) {
    pk_assert(desc->buffer);
    sound->channel = (tinymixer_channel){ 0 };
    sound->node = desc->node;
    if (desc->should_play) {
        // check if we need positional audio
        if (desc->node != NULL) {
            if (desc->loop) {
                tinymixer_add_loop2(
                    desc->buffer, 0, 0.75f, 1.0f,
                    (const float*)&desc->node->position,
                    desc->range_min, desc->range_max,
                    &sound->channel
                );
            }
            else tinymixer_add2(
                desc->buffer, 0, 0.75f, 1.0f,
                (const float*)&desc->node->position,
                desc->range_min, desc->range_max,
                &sound->channel
            );
        }
        else {
            if (desc->loop) {
                tinymixer_add_loop(desc->buffer, 0, 0.75f, 1.0f, &sound->channel);
            }
            else tinymixer_add(desc->buffer, 0, 0.75f, 1.0f, &sound->channel);
        }
        sound->ready = true;
    }
}

void pk_update_sound(pk_sound* sound) {
    if (sound->ready && sound->node) {
        tinymixer_channel_set_position(sound->channel, sound->node->position.Elements);
    }
}

void pk_release_sound(pk_sound* sound) {
    pk_assert(sound);
    tinymixer_channel_stop(sound->channel);
    sound->node = NULL;
}

void pk_update_sound_listener(pk_sound_listener* li, HMM_Vec3 pos, float dt) {
    const float smoothing = 1.0f - expf(-dt * li->smoothing);
    li->position = HMM_LerpV3(li->position, smoothing, pos);
    tinymixer_update_listener((const float*)li->position.Elements);
}

