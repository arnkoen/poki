#pragma once
#include "sokol_gfx.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	sg_depth_state depth_state;
	sg_pixel_format pixel_format; //default: RGBA8
	int sample_count; //default: 1
} pk_pip_desc;

sg_pipeline pk_unlit_tex_pip(const pk_pip_desc* desc);
sg_pipeline pk_color_phong_pip(const pk_pip_desc* desc);
sg_pipeline pk_texture_phong_pip(const pk_pip_desc* desc);


#ifdef __cplusplus
} //extern "C"
#endif
