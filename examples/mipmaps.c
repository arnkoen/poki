/*
This program showcases different ways to display a model with a texture and mipmaps.
*/
#include "../poki.h"
#define SOKOL_IMPL
#include "../deps/sokol_app.h"
#include "../deps/sokol_glue.h"
#include "../deps/sokol_log.h"

#define BUFFER_SIZE 1024*1024
static pk_cam cam;
static sg_pipeline pip;
static uint8_t model_buffer[BUFFER_SIZE];
static pk_primitive prim;
static uint8_t webp_buffer[BUFFER_SIZE];
static sg_image webp;
static sg_view webp_view;
static uint8_t png_buffer[BUFFER_SIZE];
static sg_image png;
static sg_view png_view;
static uint8_t dds_buffer[BUFFER_SIZE*2];
static sg_image dds;
static sg_view dds_view;
static pk_allocator allocator;

static void primitive_loaded(m3d_t* m3d, void* udata) {
    (void)udata;
    bool ok = pk_load_m3d(&allocator, &prim, NULL, m3d);
    pk_assert(ok);
    pk_release_m3d_data(m3d);
}

static void webp_loaded(sg_image_desc* desc, void* udata) {
    (void)udata;
    sg_image tmp = sg_make_image(desc);
    //if 0 is passed as mip_levels, poki will pick the number of mips.
    webp = pk_gen_mipmaps_gpu(tmp, desc->width, desc->height, 0);
    sg_destroy_image(tmp);
    pk_release_image_desc(&allocator, desc);
    webp_view = sg_make_view(&(sg_view_desc) {
        .texture.image = webp,
    });
}

static void png_loaded(sg_image_desc* desc, void* udata) {
    (void)udata;
    sg_image_desc with_mips = pk_gen_mipmaps_cpu(&allocator, desc, 0);
    png = sg_make_image(&with_mips);
    pk_release_image_desc(&allocator, &with_mips);
    png_view = sg_make_view(&(sg_view_desc) {
        .texture.image = png,
    });
}

static void dds_loaded(sg_image_desc* desc, void* udata) {
    (void)udata;
    dds = sg_make_image(desc);
    pk_release_image_desc(&allocator, desc);
    dds_view = sg_make_view(&(sg_view_desc) {
        .texture.image = dds,
    });
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

    //we only need one vertex buffer, since we wont load skinning data.
    pk_alloc_primitive(&prim, 1, 0);
    prim.bindings.samplers[0] = sg_make_sampler(&(sg_sampler_desc) {0});
    pk_load_m3d_data(&(pk_m3d_request) {
        .path = "assets/cesium_man.m3d",
        .buffer = SFETCH_RANGE(model_buffer),
        .loaded_cb = primitive_loaded,
    });

    webp_view = sg_alloc_view();
    png_view = sg_alloc_view();
    dds_view = sg_alloc_view();

    pk_load_image_data(&(pk_image_request) {
        .path = "assets/tex.webp",
        .buffer = SFETCH_RANGE(webp_buffer),
        .loaded_cb = webp_loaded,
    });

    pk_load_image_data(&(pk_image_request) {
        .path = "assets/tex.png",
        .buffer = SFETCH_RANGE(png_buffer),
        .loaded_cb = png_loaded,
    });

    pk_load_image_data(&(pk_image_request) {
        .path = "assets/tex.dds",
        .buffer = SFETCH_RANGE(dds_buffer),
        .loaded_cb = dds_loaded,
    });

    pip = sg_make_pipeline(&(sg_pipeline_desc) {
        .layout = pk_pnt_layout(),
        .shader = sg_make_shader(pk_phong_tex_shader_desc(sg_query_backend())),
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
        .index_type = SG_INDEXTYPE_UINT32, //Poki loads gltf and m3d indices as uint32_t.
        .cull_mode = SG_CULLMODE_FRONT, //No need, to draw both sides of all the faces.
        .depth = {
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = true,
        },
    });

    pk_init_cam(&cam, &(pk_cam_desc) {
        .distance = 2.0f,
        .mindist = 0.5f,
        .maxdist = 25.0f,
        .sensitivity = 10.f,
    });
}

static void frame(void) {
    //keep loading stuff
    sfetch_dowork();
    pk_update_cam(&cam, sapp_width(), sapp_height());

    sg_begin_pass(&(sg_pass) {
        .action.colors[0] = {
            .clear_value = {0.2f, 0.0f, 0.2f, 1.0f},
            .load_action = SG_LOADACTION_CLEAR
        },
        .swapchain = sglue_swapchain(),
    });

    pk_tex_material_t material = {
        .shininess = 8.f,
    };

    pk_dir_light_t light = {
        .ambient = {0.1f, 0.1f, 0.1f, 1.0f},
        .diffuse = {1.0f, 1.0f, 1.0f, 1.0f},
        .specular = {1.0f, 1.0f, 1.0f, 1.0f},
        .direction = HMM_V3(-0.5f, 0.0f, -0.75f),
    };

    sg_apply_pipeline(pip);

    sg_apply_uniforms(UB_pk_tex_material, &SG_RANGE(material));
    sg_apply_uniforms(UB_pk_dir_light, &SG_RANGE(light));


    pk_vs_params_t vs_params = {
        .model = HMM_Translate(HMM_V3(0, -0.5f, 0)),
        .proj = cam.proj,
        .view = cam.view,
        .viewpos = cam.eyepos,
    };
    sg_apply_uniforms(UB_pk_vs_params, &SG_RANGE(vs_params));
    prim.bindings.views[0] = webp_view;
    pk_draw_primitive(&prim, 1);

    vs_params.model = HMM_Translate(HMM_V3(1.5f, -0.5f, 0.f));
    sg_apply_uniforms(UB_pk_vs_params, &SG_RANGE(vs_params));
    prim.bindings.views[0] = png_view;
    pk_draw_primitive(&prim, 1);

    vs_params.model = HMM_Translate(HMM_V3(-1.5f, -0.5f, 0.f));
    sg_apply_uniforms(UB_pk_vs_params, &SG_RANGE(vs_params));
    prim.bindings.views[0] = dds_view;
    pk_draw_primitive(&prim, 1);

    sg_end_pass();
    sg_commit();
}

static void cleanup(void) {
    pk_shutdown();
}

static void event(const sapp_event* e) {
    pk_cam_input(&cam, e);
    if(e->type & SAPP_EVENTTYPE_KEY_UP && e->key_code == SAPP_KEYCODE_F) {
        sapp_toggle_fullscreen();
    }
}

//Use dedicated graphics card on windows, if available.
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
        .win32.console_attach = true,
        .win32.console_create = true,
    };
}

