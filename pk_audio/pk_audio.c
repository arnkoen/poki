#ifndef PK_AUDIO_SINGLE_HEADER
#include "pk_audio.h"
#include "../poki.h"
#include <string.h>
#endif

#ifdef PK_SINGLE_HEADER
#ifndef POKI_H
#error "please include poki.h before pk_audio.h"
#endif
#endif

static void _pk_stream_cb(float* buffer, int num_frames, int num_channels, void* udata) {
    (void)num_channels; (void)udata;
    tm_getsamples(buffer, num_frames);
}

void pk_audio_setup(const pk_audio_desc* desc) {
    saudio_desc ad = { 0 };
    memcpy(&ad, &desc->saudio, sizeof(saudio_desc));
    if (!ad.stream_cb && !ad.stream_userdata_cb) {
        ad.stream_userdata_cb = _pk_stream_cb;
    }
    saudio_setup(&ad);
    tm_init(desc->mixer_callbacks, saudio_sample_rate());
}

void pk_audio_shutdown() {
    if (saudio_isvalid()) {
        tm_shutdown();
        saudio_shutdown();
    }
}

void pk_play_sound(pk_sound* sound, const pk_sound_channel_desc* desc) {
    pk_assert(desc->buffer);
    sound->channel = (tm_channel){ 0 };
    sound->node = desc->node;
    // check if we need positional audio
    if (desc->node != NULL) {
        if (desc->loop) {
            tm_add_spatial_loop(
                desc->buffer, 0, 0.75f, 1.0f,
                (const float*)&desc->node->position,
                desc->range_min, desc->range_max,
                &sound->channel
            );
        }
        else tm_add_spatial(
            desc->buffer, 0, 0.75f, 1.0f,
            (const float*)&desc->node->position,
            desc->range_min, desc->range_max,
            &sound->channel
        );
    }
    else {
        if (desc->loop) {
            tm_add_loop(desc->buffer, 0, 0.75f, 1.0f, &sound->channel);
        }
        else tm_add(desc->buffer, 0, 0.75f, 1.0f, &sound->channel);
    }
}

void pk_update_sound(pk_sound* sound) {
    if (sound->node) {
        tm_channel_set_position(sound->channel, sound->node->position.Elements);
    }
}

void pk_stop_sound(pk_sound* sound) {
    pk_assert(sound);
    tm_channel_stop(sound->channel);
}

void pk_update_sound_listener(pk_sound_listener* li, HMM_Vec3 pos, float dt) {
    const float smoothing = 1.0f - expf(-dt * li->smoothing);
    li->position = HMM_LerpV3(li->position, smoothing, pos);
    tm_update_listener((const float*)li->position.Elements);
}


typedef struct {
    pk_sound_buffer_loaded_callback loaded_cb;
    pk_fail_callback fail_cb;
} sound_request_data;

static void _sound_fetch_callback(const sfetch_response_t* response) {
    sound_request_data data = *(sound_request_data*)response->user_data;

    if (response->fetched) {
        const tm_buffer* buffer = NULL;
        tm_create_buffer_vorbis_stream(
            response->buffer.ptr,
            (int)response->buffer.size,
            NULL, NULL,
            &buffer
        );
        if (data.loaded_cb) {
            data.loaded_cb(buffer);
        }
    }
    if (response->failed) {
        switch (response->error_code) {
        case SFETCH_ERROR_FILE_NOT_FOUND: pk_printf("Sound file not found: %s\n", response->path); break;
        case SFETCH_ERROR_BUFFER_TOO_SMALL: pk_printf("Sound buffer too small: %s\n", response->path); break;
        default: break;
        }
        if (data.fail_cb) {
            data.fail_cb(response);
        }
    }
}

sfetch_handle_t pk_load_sound_buffer(const pk_sound_buffer_request* req) {
    sound_request_data data = {
        .loaded_cb = req->loaded_cb,
        .fail_cb = req->fail_cb,
    };

    return sfetch_send(&(sfetch_request_t) {
        .path = req->path,
        .buffer = req->buffer,
        .callback = _sound_fetch_callback,
        .user_data = SFETCH_RANGE(data),
    });
}

