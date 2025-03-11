#include "texture.h"
#include "common.h"

static const uint32_t _checker_pixels[4 * 4] = {
    0xFFAAAAAA, 0xFF555555, 0xFFAAAAAA, 0xFF555555,
    0xFF555555, 0xFFAAAAAA, 0xFF555555, 0xFFAAAAAA,
    0xFFAAAAAA, 0xFF555555, 0xFFAAAAAA, 0xFF555555,
    0xFF555555, 0xFFAAAAAA, 0xFF555555, 0xFFAAAAAA,
};

static sg_image_desc _checker_image_desc(void) {
    sg_image_desc desc = { 0 };
    desc.width = 4;
    desc.height = 4;
    desc.data.subimage[0][0] = SG_RANGE(_checker_pixels);
    return desc;
}

void pk_init_texture(pk_texture* tex, const pk_texture_desc* desc) {
    pk_assert(tex);
    sg_image_desc img = { 0 };
    img.render_target = desc->render_target;
    img.data.subimage[0][0] = desc->data;
    img.width = desc->width;
    img.height = desc->height;
    img.pixel_format = desc->format;
    tex->image = sg_make_image(&img);

    sg_sampler_desc sd = { 0 };
    sd.min_filter = desc->min_filter;
    sd.mag_filter = desc->mag_filter;
    sd.wrap_u = desc->wrap_u;
    sd.wrap_v = desc->wrap_v;
    tex->sampler = sg_make_sampler(&sd);
}

void pk_checker_texture(pk_texture* tex) {
    pk_assert(tex);
    sg_image_desc img = _checker_image_desc();
    tex->image = sg_make_image(&img);
    tex->sampler = sg_make_sampler(&(sg_sampler_desc) { 0 });
}

void pk_update_texture(pk_texture* tex, sg_range data) {
    pk_assert(tex && data.ptr);
    sg_image_data img = { 0 };
    img.subimage[0][0] = data;
    sg_update_image(tex->image, &img);
}

void pk_release_texture(pk_texture* tex) {
    pk_assert(tex);
    sg_destroy_image(tex->image);
    sg_destroy_sampler(tex->sampler);
}
