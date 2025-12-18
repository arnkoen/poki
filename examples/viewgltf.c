/*
This program loads and displays a gltf file with a keyframe animation,
which uses location, rotation, scale and parent-child relationships.
Also the image, which should be loaded as the model texture, does not
exist and poki should generate a checkerboard texture instead.
*/

#include "../poki.h"
#define SOKOL_IMPL
#include "../deps/sokol_app.h"
#include "../deps/sokol_glue.h"
#include "../deps/sokol_log.h"

#define BUFFER_SIZE 1024*128
static pk_cam cam;
static sg_pipeline pip;
static uint8_t model_buffer[BUFFER_SIZE];
static bool model_ready = false;
static pk_model model;
static pk_gltf_anim anim;
static uint8_t image_buffer[BUFFER_SIZE*2];
static pk_texture tex;
static pk_allocator allocator;

static void model_loaded(cgltf_data* gltf, void* udata) {
    (void)udata;
    bool ok = pk_load_gltf(&allocator, &model, gltf);
    pk_assert(ok);
    ok = pk_load_gltf_anim(&allocator, &anim, &model, gltf);
    pk_set_model_texture(&model, &tex, 0);
    pk_release_gltf_data(gltf);
    if(ok) model_ready = true;
}

static void image_loaded(sg_image_desc* desc, void* udata) {
    (void)udata;
    sg_init_image(tex.image, desc);
    //note: only free the image desc if the error code is SFETCH_ERROR_NONE
    //because on error, poki provides a checker texture, which is statically allocated.
}

static void init(void) {
    allocator = pk_default_allocator();
    pk_setup(&(pk_desc) {
        .gfx = {
            .environment = sglue_environment(),
            .logger.func = slog_func,
        },
        .fetch = {
            .logger.func = slog_func,
        },
    });

    //If the image could not be loaded,
    //a checker texture should be generated instead.
    pk_load_image_data(&(pk_image_request) {
        .path = "assets/lkjhlkjhlkjh.png", //This image doesn't exist.
        .buffer = SFETCH_RANGE(image_buffer),
        .loaded_cb = image_loaded,
    });

    //Allocating the sg_image makes sure, draw calls using this image
    //are dropped silently instead of crashing.
    tex = PK_TEXTURE(sg_alloc_image(), sg_make_sampler(&(sg_sampler_desc) { 0 }));

    //Load a gltf file asynchronously via sokol_fetch.
    pk_load_gltf_data(&(pk_gltf_request) {
        .path = "assets/gltf.glb",
        .buffer = SFETCH_RANGE(model_buffer),
        .loaded_cb = model_loaded,
    });

    pip = sg_make_pipeline(&(sg_pipeline_desc) {
        .layout = pk_pnt_layout(), //Poki loads vertex data as positions, normals and texcoords.
        .shader = sg_make_shader(pk_unlit_tex_shader_desc(sg_query_backend())),
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
        .index_type = SG_INDEXTYPE_UINT32, //Poki loads gltf and m3d indices as uint32_t.
        .cull_mode = SG_CULLMODE_FRONT, //No need, to draw both sides of all the faces.
        .depth = {
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = true,
        },
    });

    //If you initialize a descriptor to 0, in most cases poki will pick some defaults.
    pk_init_cam(&cam, &(pk_cam_desc) { 0 });
}

static void frame(void) {
    //keep loading stuff
    sfetch_dowork();

    sg_begin_pass(&(sg_pass) {
        .action.colors[0] = {
            .clear_value = {0.2f, 0.0f, 0.2f, 1.0f},
            .load_action = SG_LOADACTION_CLEAR
        },
        .swapchain = sglue_swapchain(),
    });

    if (model_ready) {
        pk_update_cam(&cam, sapp_width(), sapp_height());

        pk_play_gltf_anim(&anim, (float)sapp_frame_duration());

        sg_apply_pipeline(pip);

        pk_vs_params_t vs_params = {
            //.model will be filled in during pk_draw_model.
            .view = cam.view,
            .proj = cam.proj,
            .viewpos = cam.center,
        };

        pk_draw_model(&model, &vs_params);
    }

    sg_end_pass();
    sg_commit();
}

static void cleanup(void) {
    pk_shutdown();
    //Just let the os clean up the other stuff...
}

static void event(const sapp_event* e) {
    pk_cam_input(&cam, e);
    if(e->type & SAPP_EVENTTYPE_KEY_UP && e->key_code == SAPP_KEYCODE_F) {
        sapp_toggle_fullscreen();
    }
}

//Use dedicated graphics card on windows, if available
PK_REQUEST_DEDICATED_DEVICE

sapp_desc sokol_main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    return (sapp_desc) {
        .init_cb = init,
        .frame_cb = frame,
        .cleanup_cb = cleanup,
        .event_cb = event,
        .width = 800,
        .height = 600,
        .window_title = "poki",
        .icon.sokol_default = true,
        .swap_interval = 1,
        .sample_count = 4,
        .win32.console_attach = true,
        .win32.console_create = true,
    };
}
