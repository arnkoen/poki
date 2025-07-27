#include "../poki.h"
#include "../pk_audio/pk_audio.h"
#define SOKOL_IMPL
#include "../deps/sokol_log.h"
#include "../deps/sokol_app.h"

static uint8_t file_buffer[1024 * 1024];
static pk_sound sound;

static void sound_loaded(const tm_buffer* buf, void* udata) {
    (void)udata;
    pk_play_sound(&sound, &(pk_sound_channel_desc) {
        .buffer = buf,
        .loop = true,
    });
}

static void init(void) {
    pk_setup(&(pk_desc) {
        .fetch = {
            .logger.func = slog_func,
        },
    });

    pk_audio_setup(&(pk_audio_desc) {
        .saudio = {
            .num_channels = 2,
            .logger.func = slog_func,
        },
    });

    pk_load_sound_buffer(&(pk_sound_buffer_request) {
        .buffer = SFETCH_RANGE(file_buffer),
        .path = "assets/loop.ogg",
        .loaded_cb = sound_loaded,
    });
}

static void frame(void) {
    sfetch_dowork();
}

static void cleanup(void) {
    pk_stop_sound(&sound);
    pk_audio_shutdown();
    pk_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    return (sapp_desc) {
        .init_cb = init,
        .frame_cb = frame,
        .cleanup_cb = cleanup,
        .width = 800,
        .height = 600,
    };
}
