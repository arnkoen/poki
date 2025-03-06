#include "io.h"
#include "mixer.h"
#include "sound.h"
#include "node.h"
#include "log.h"

typedef struct {
    pk_sound_buffer_loaded_callback loaded_cb;
    pk_fail_callback fail_cb;
} sound_request_data;


static void _sound_fetch_callback(const sfetch_response_t* response) {
    sound_request_data data = *(sound_request_data*)response->user_data;

    if (response->fetched) {
        const tinymixer_buffer* buffer = NULL;
        tinymixer_create_buffer_vorbis_stream(
            response->buffer.ptr,
            response->buffer.size,
            NULL, NULL,
            &buffer
        );
        if (data.loaded_cb) {
            data.loaded_cb(buffer);
        }
    }
    if (response->failed) {
        switch (response->error_code) {
        case SFETCH_ERROR_FILE_NOT_FOUND: log_debug("Sound file not found"); break;
        case SFETCH_ERROR_BUFFER_TOO_SMALL: log_debug("Sound buffer too small"); break;
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
