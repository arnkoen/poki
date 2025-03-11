#pragma once
#include "texture.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pk_pass_desc {
	sg_load_action load_action;
	sg_color clear_value;
	int width, height; // default: sapp_width()/sapp_height()
	int attachment_count; //cannot be > SG_MAX_COLOR_ATTACHMENTS
	int sample_count; // default: 1
	sg_pixel_format color_format; //default: sglue_swapchain().color_format
	sg_pixel_format depth_format; // if undefined, depth image will be skipped
	sg_sampler_desc sampler; // default: nearest filter, wrap repeat
} pk_pass_desc;

typedef struct pk_pass {
	sg_pass_action action;
	sg_attachments attachments;
	sg_image color_imgs[SG_MAX_COLOR_ATTACHMENTS];
	sg_sampler color_smp;
	sg_image depth_img; // only populated if desc->depth_format !undefined
	sg_sampler depth_smp; // only populated if desc->depth_format !undefined
	sg_image resolve_img; // only populated id desc->samplecount > 1
} pk_pass;

void pk_init_pass(pk_pass* pass, const pk_pass_desc* desc);
void pk_release_pass(pk_pass* pass);
void pk_begin_pass(pk_pass* pass); // just end with sg_end_pass

#ifdef __cplusplus
} //extern "C"
#endif


