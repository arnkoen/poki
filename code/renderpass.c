#include "renderpass.h"
#include "common.h"
#include "log.h"

#include "sokol_app.h"
#include "sokol_glue.h"

#define PASS_DEF(val, def) (val == 0 ? def : val)

void pk_init_pass(pk_pass* pass, const pk_pass_desc* desc) {
    pk_assert(desc->width > 0 && desc->height > 0);

    pass->action.colors[0].load_action = desc->load_action;
    pass->action.colors[0].clear_value = desc->clear_value;

    sg_attachments_desc atts = { 0 };

    for (int i = 0; i < desc->attachment_count; ++i) {
        if (i > SG_MAX_COLOR_ATTACHMENTS) {
            log_error("Attachment count cannot be > SG_MAX_COLOR_ATTACHMENTS");
            break;
        }

        pass->color_imgs[i] = sg_make_image(&(sg_image_desc) {
            .render_target = true,
            .sample_count = PASS_DEF(desc->sample_count, 1),
            .width = PASS_DEF(desc->width, sapp_width()),
            .height = PASS_DEF(desc->height, sapp_height()),
            .pixel_format = PASS_DEF(desc->color_format, sglue_swapchain().color_format),
        });
        atts.colors[i].image = pass->color_imgs[i];
    }

    pass->color_smp = sg_make_sampler(&desc->sampler);

    if (desc->depth_format != _SG_PIXELFORMAT_DEFAULT) {
        pass->depth_img = sg_make_image(&(sg_image_desc) {
            .render_target = true,
            .sample_count = PASS_DEF(desc->sample_count, 1),
            .width = PASS_DEF(desc->width, sapp_width()),
            .height = PASS_DEF(desc->height, sapp_height()),
            .pixel_format = desc->depth_format,
        });
        pass->depth_smp = sg_make_sampler(&(sg_sampler_desc) {
            .wrap_u = desc->sampler.wrap_u,
            .wrap_v = desc->sampler.wrap_v,
        });
        atts.depth_stencil.image = pass->depth_img;
    }

    if (desc->sample_count > 1) {
        pass->resolve_img = sg_make_image(&(sg_image_desc) {
            .render_target = true,
            .sample_count = 1,
            .width = PASS_DEF(desc->width, sapp_width()),
            .height = PASS_DEF(desc->height, sapp_height()),
            .pixel_format = PASS_DEF(desc->color_format, sglue_swapchain().color_format),
        });
        atts.resolves[0].image = pass->resolve_img;
    }

    pass->attachments = sg_make_attachments(&atts);
}

void pk_release_pass(pk_pass* pass) {
    //just go over everythingp; its okay, to call sg_destroy... on an invalid ressource.
    sg_destroy_attachments(pass->attachments);
    for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; ++i) {
        sg_destroy_image(pass->color_imgs[i]);
    }
    sg_destroy_sampler(pass->color_smp);
    sg_destroy_image(pass->depth_img);
    sg_destroy_sampler(pass->depth_smp);
    sg_destroy_image(pass->resolve_img);
}

void pk_begin_pass(pk_pass* pass) {
    sg_begin_pass(&(sg_pass) {
        .action = pass->action,
        .attachments = pass->attachments,
    });
}

