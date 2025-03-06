#include "sfx.h"
#include "log.h"
#include "mixer.h"
#include "sokol_audio.h"
#include "sokol_log.h"

static bool initialized = false;

static void stream_cb(float* buffer, int frame_count, int channel_count) {
    (void)channel_count;
    tinymixer_getsamples(buffer, frame_count);
}

void pk_sfx_init(const pk_sfx_desc* desc) {
    tinymixer_callbacks callbacks = { 0 };
    tinymixer_init(callbacks, desc->sample_rate);

    saudio_desc d = { 0 };
    d.sample_rate = desc->sample_rate;
    d.num_channels = desc->channel_count;
    d.buffer_frames = desc->buffer_frames;
    d.logger.func = slog_func;
    d.stream_cb = stream_cb;

    saudio_setup(&d);
}

void pk_sfx_shutdown() {
    saudio_shutdown();
}


