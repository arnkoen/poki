#include "poki.h"
#define SOKOL_IMPL
#include "deps/sokol_log.h"

static uint8_t file_buffer[1024 * 1024];
static pk_sound sound;

static void sound_loaded(const tm_buffer* buf) {
    pk_play_sound(&sound, &(pk_sound_channel_desc) {
        .buffer = buf,
        .loop = true,
    });
}

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    pk_setup(&(pk_desc) {
        .flags = PK_INIT_FETCH | PK_INIT_AUDIO,
        .fetch = {
            .logger.func = slog_func,
        },
        .audio = {
            .saudio = {
                .num_channels = 2,
                .logger.func = slog_func,
            },
        },
    });

    sfetch_handle_t req = pk_load_sound_buffer(&(pk_sound_buffer_request) {
        .buffer = SFETCH_RANGE(file_buffer),
        .path = "assets/loop.ogg",
        .loaded_cb = sound_loaded,
    });

    while (sfetch_handle_valid(req)) {
        sfetch_dowork();
    }

    puts("Press any key, to quit...");
    (void)getchar();

    pk_stop_sound(&sound);
    pk_shutdown();
    return 0;
}
