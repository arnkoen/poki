#include "io.h"
#include "qoi.h"
#include "texture.h"
#include "common.h"
#include "log.h"

//--IMAGE-LOADING-------------------------------------------------

static const uint32_t _fail_pixels[4 * 4] = {
    0xFFAAAAAA, 0xFF555555, 0xFFAAAAAA, 0xFF555555,
    0xFF555555, 0xFFAAAAAA, 0xFF555555, 0xFFAAAAAA,
    0xFFAAAAAA, 0xFF555555, 0xFFAAAAAA, 0xFF555555,
    0xFF555555, 0xFFAAAAAA, 0xFF555555, 0xFFAAAAAA,
};

// Code for generating the "fail image"
static sg_image_desc _fail_image_desc(void) {
    sg_image_desc desc = { 0 };
    desc.width = 4;
    desc.height = 4;
    desc.data.subimage[0][0] = SG_RANGE(_fail_pixels);
    return desc;
}


typedef struct {
    pk_texture* tex;
    pk_tex_opts opts;
    pk_fail_callback fail_cb;
} texture_request_data;


static void _tex_fetch_callback(const sfetch_response_t* response) {
    texture_request_data data = *(texture_request_data*)response->user_data;

    if (response->fetched) {
		qoi_desc qoi = { 0 };
		void* pixels = NULL;
		pixels = qoi_decode(response->buffer.ptr, (int)response->buffer.size, &qoi, 4);
		if (pixels) {
			sg_image_desc desc = { 0 };
			desc.pixel_format = SG_PIXELFORMAT_RGBA8;
			desc.width = qoi.width;
			desc.height = qoi.height;
			desc.data.subimage[0][0].ptr = pixels;
			desc.data.subimage[0][0].size = qoi.width * qoi.height * 4;
			sg_init_image(data.tex->image, &desc);
			pk_free(pixels);
		}
        else {
			sg_image_desc img = _fail_image_desc();
			sg_init_image(data.tex->image, &img);
		}

    }
    else if (response->failed) {
        switch (response->error_code) {
        case SFETCH_ERROR_FILE_NOT_FOUND: log_debug("Image file not found: %s", response->path); break;
        case SFETCH_ERROR_BUFFER_TOO_SMALL: log_debug("Image buffer too small: %s", response->path); break;
        default: break;
        }
		if (data.fail_cb != NULL) {
			data.fail_cb(response);
		}
    }
}


sfetch_handle_t pk_load_texture(const pk_texture_request* req) {
	req->tex->image = sg_alloc_image();
	req->tex->sampler = sg_make_sampler(&(sg_sampler_desc) {
		.min_filter = req->opts.min_filter,
		.mag_filter = req->opts.mag_filter,
		.wrap_u = req->opts.wrap_u,
		.wrap_v = req->opts.wrap_v,
	});


	texture_request_data data = {
		.tex = req->tex,
		.opts = req->opts,
		.fail_cb = req->fail_cb,
	};

	return sfetch_send(&(sfetch_request_t) {
		.path = req->path,
		.callback = _tex_fetch_callback,
		.buffer = req->buffer,
		.user_data = SFETCH_RANGE(data),
	});

}
