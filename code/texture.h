#pragma once
#include "sokol_gfx.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct pk_texture_desc {
        int width, height;
        bool render_target;
        sg_range data;
        sg_pixel_format format;
        sg_filter min_filter;
        sg_filter mag_filter;
        sg_wrap wrap_u;
        sg_wrap wrap_v;
    } pk_texture_desc;

    typedef struct pk_texture {
        sg_image image;
        sg_sampler sampler;
    } pk_texture;

    void pk_init_texture(pk_texture* tex, const pk_texture_desc* desc);
    void pk_checker_texture(pk_texture* tex);
    void pk_update_texture(pk_texture* tex, sg_range data);
    void pk_release_texture(pk_texture* tex);

#ifdef __cplusplus
} // extern "C"
#endif 
