/*
This program loads and displays an m3d model with a skeletal animation
and adds a simple post processing effect.
*/
#include "../poki.h"
#define SOKOL_IMPL
#include "../deps/sokol_app.h"
#include "../deps/sokol_glue.h"
#include "../deps/sokol_log.h"
#include "effect.glsl.h"

#define BUFFER_SIZE 1024*128
static pk_cam cam;
static sg_pipeline offscreen_pip;
static uint8_t model_buffer[BUFFER_SIZE];
static pk_primitive prim;
static pk_skeleton skeleton;
static pk_bone_anim_data* anims;
static pk_bone_anim_state anim_state;
static int anim_count;
static pk_texture tex;
static pk_rendertarget rt;
static pk_primitive display_rect;
static sg_pipeline display_pip;
static pk_allocator allocator;

static void primitive_loaded(m3d_t* m3d, void* udata) {
    (void)udata;
    bool ok = pk_load_m3d(&allocator, &prim, NULL, m3d);
    pk_assert(ok);
    ok = pk_load_skeleton(&allocator, &skeleton, m3d);
    pk_assert(ok);
    anims = pk_load_bone_anims(&allocator, m3d, &anim_count);
    pk_assert(anims && anim_count > 0);
    anim_state.anim = &anims[0];
    anim_state.loop = true;
    pk_printf("loaded anims: %i\n", anim_count);
    pk_release_m3d_data(m3d);
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

    /*
    Poki assumes, that the buffers are preallocated.
    We need, to allocate one for the positions, normals and uvs
    and one for the skinning data. If you don't want, to use the
    skeletal animation, you could just allocate one buffer.
    */
    pk_alloc_primitive(&prim, 2, 0);
    pk_load_m3d_data(&(pk_m3d_request) {
        .path = "assets/cesium_man.m3d",
        .buffer = SFETCH_RANGE(model_buffer),
        .loaded_cb = primitive_loaded,
    });
    pk_checker_texture(&tex);
    pk_texture_primitive(&prim, &tex, 0);

    pk_init_rendertarget(&rt, &(pk_rendertarget_desc) {
        .width = sapp_width(),
        .height = sapp_height(),
        .color_format = SG_PIXELFORMAT_RGBA8,
        .depth_format = SG_PIXELFORMAT_DEPTH,
        .color_attachment_count = 1,
        .action = {
            .colors[0] = {
                .load_action = SG_LOADACTION_CLEAR,
            },
        },
    });

    //Create a rectangle, to display the render result.
    const float vertices[] = {
         1.0f,  1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f
    };
    const uint16_t indices[] = {
        0, 1, 3,
        1, 2, 3
    };

    pk_init_primitive(&display_rect, &(pk_primitive_desc) {
        .indices = SG_RANGE(indices),
        .vertices = SG_RANGE(vertices),
        .num_elements = 6,
    });

    //The texture for the primitive is gonna be the render result.
    display_rect.bindings.views[0] = sg_make_view(&(sg_view_desc) {
        .texture.image = rt.color_images[0],
    });
    display_rect.bindings.samplers[0] = sg_make_sampler(&(sg_sampler_desc) {
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
        .wrap_u = SG_WRAP_REPEAT,
        .wrap_v = SG_WRAP_REPEAT,
        .label = "rt_sampler",
    });

    offscreen_pip = sg_make_pipeline(&(sg_pipeline_desc) {
        .layout = pk_skinned_layout(),
        .shader = sg_make_shader(pk_skinned_phong_tex_shader_desc(sg_query_backend())),
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
        .index_type = SG_INDEXTYPE_UINT32, //Poki loads gltf and m3d indices as uint32_t.
        .cull_mode = SG_CULLMODE_FRONT, //No need, to draw both sides of all the faces.
        .depth = {
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = true,
            .pixel_format = SG_PIXELFORMAT_DEPTH,
        },
    });

    display_pip = sg_make_pipeline(&(sg_pipeline_desc) {
        .shader = sg_make_shader(effect_shader_desc(sg_query_backend())),
        .layout.buffers[0].stride = 3 * sizeof(float),
        .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
        .index_type = SG_INDEXTYPE_UINT16, //Display rect uses uint16_t indices.
    });

    pk_init_cam(&cam, &(pk_cam_desc) {
        .distance = 2.0f,
        .mindist = 1.0f,
        .maxdist = 5.0f,
        .sensitivity = 10.f,
    });
}

static void frame(void) {
    // Keep loading stuff.
    sfetch_dowork();

    // Draw the scene to the fbo.
    pk_begin_rendertarget(&rt);

    pk_update_cam(&cam, sapp_width(), sapp_height());

    pk_vs_params_t vs_params = {
        .model = HMM_Translate(HMM_V3(0.f, -0.5f, 0.f)),
        .view = cam.view,
        .proj = cam.proj,
        .viewpos = cam.center,
    };

    pk_bone_matrices_t bones = {0};
    pk_play_bone_anim(bones.bones, &skeleton, &anim_state, (float)sapp_frame_duration());

    pk_dir_light_t light = {
        .ambient = {0.1f, 0.1f, 0.1f, 1.0f},
        .diffuse = {1.0f, 1.0f, 1.0f, 1.0f},
        .specular = {1.0f, 1.0f, 1.0f, 1.0f},
        .direction = HMM_V3(-0.5f, 0.0f, -0.75f),
    };

    pk_tex_material_t mat = {
        .shininess = 16.f,
    };

    sg_apply_pipeline(offscreen_pip);

    sg_apply_uniforms(UB_pk_vs_params, &SG_RANGE(vs_params));
    sg_apply_uniforms(UB_pk_bone_matrices, &SG_RANGE(bones));
    sg_apply_uniforms(UB_pk_tex_material, &SG_RANGE(mat));
    sg_apply_uniforms(UB_pk_dir_light, &SG_RANGE(light));

    pk_draw_primitive(&prim, 1);

    pk_end_rendertarget();

    //Draw the render result to the screen.
    sg_begin_pass(&(sg_pass) {
        .swapchain = sglue_swapchain(),
        .action = {
            .colors[0] = {
                .load_action = SG_LOADACTION_DONTCARE,
            },
        },
    });

    effect_params_t params = {
        .rand = (float)(rand() % 1000),
    };

    sg_apply_pipeline(display_pip);

    sg_apply_uniforms(UB_effect_params, &SG_RANGE(params));
    pk_draw_primitive(&display_rect, 1);

    sg_end_pass();
    sg_commit();
}

static void cleanup(void) {
    pk_shutdown();
}

static void event(const sapp_event* e) {
    pk_cam_input(&cam, e);
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
        .win32_console_attach = true,
        .win32_console_create = true,
    };
}

