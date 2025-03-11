#pragma once

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct pk_sfx_desc {
        int sample_rate;
        int channel_count;
        int buffer_frames;
    } pk_sfx_desc;

    void pk_sfx_init(const pk_sfx_desc* desc);
    void pk_sfx_shutdown(void);

#ifdef __cplusplus
} //extern "C"
#endif

