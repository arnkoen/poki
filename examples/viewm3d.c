/*
This program loads and displays an m3d model with a skeletal animation.
It also provides a really basic example of how to override poki's allocator.
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
static pk_primitive prim;
static pk_skeleton skeleton;
static pk_bone_anim_data* anims;
static pk_bone_anim_state anim_state;
static int anim_count;
static pk_texture tex;
static pk_allocator allocator;

#define MEM_SIZE 1024 * 1024
static struct {
    uint8_t buf[MEM_SIZE];
    size_t top;
} memory;

static void* static_alloc(size_t size, void* udata) {
    (void)udata;
    // Round up heap_top to next multiple of 16 for alignment
    // Otherwise we'll have weird issues with some HandmadeMath functions.
    size_t align = 16;
    size_t aligned_top = (memory.top + (align - 1)) & ~(align - 1);
    size_t old_top = aligned_top;
    memory.top = aligned_top + size;
    assert(memory.top <= MEM_SIZE);
    return &memory.buf[old_top];
}

static void* static_realloc(void* ptr, size_t size, void* udata) {
    (void)udata;
    return static_alloc(size, udata);
}

static void static_free(void* ptr, void* udata) {
    (void)ptr;
    (void)udata;
};

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
    allocator = (pk_allocator) {
        .alloc = static_alloc,
        .realloc = static_realloc,
        .free = static_free,
    };
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

    pip = sg_make_pipeline(&(sg_pipeline_desc) {
        .layout = pk_skinned_layout(),
        .shader = sg_make_shader(pk_skinned_phong_tex_shader_desc(sg_query_backend())),
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
        .mindist = 1.0f,
        .maxdist = 5.0f,
        .sensitivity = 10.f,
    });
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

    pk_update_cam(&cam, sapp_width(), sapp_height());

    pk_bone_matrices_t mat = { 0 };
    pk_play_bone_anim(mat.bones, &skeleton, &anim_state, (float)sapp_frame_duration());

    pk_vs_params_t vs_params = {
        .model = HMM_Translate(HMM_V3(0, -0.5f, 0)),
        .proj = cam.proj,
        .view = cam.view,
        .viewpos = cam.eyepos,
    };

    pk_tex_material_t material = {
        .shininess = 32.f,
    };

    pk_dir_light_t light = {
        .ambient = {0.1f, 0.1f, 0.1f, 1.0f},
        .diffuse = {1.0f, 1.0f, 1.0f, 1.0f},
        .specular = {1.0f, 1.0f, 1.0f, 1.0f},
        .direction = HMM_V3(-0.5f, 0.0f, -0.75f),
    };

    sg_apply_pipeline(pip);

    sg_apply_uniforms(UB_pk_vs_params, &SG_RANGE(vs_params));
    sg_apply_uniforms(UB_pk_bone_matrices, &SG_RANGE(mat));
    sg_apply_uniforms(UB_pk_tex_material, &SG_RANGE(material));
    sg_apply_uniforms(UB_pk_dir_light, &SG_RANGE(light));

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
        .sample_count = 4,
        .win32.console_attach = true,
        .win32.console_create = true,
    };
}

