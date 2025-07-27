/*
This program is just a test for compiling poki from the single header sources.
It loads a sound, a model and an animation and attaches the sound to the model
node, to give spatial audio playback.
*/
#define POKI_IMPL
#define SOKOL_GLCORE
#include "../single/poki.h"
#include "../single/pk_audio.h"
#define SOKOL_LOG_IMPL
#include "../deps/sokol_log.h"

uint8_t ogg_buffer[1028 * 1028];
pk_sound sound;
pk_sound_listener listener;
uint8_t m3d_buffer[1028 * 1028];
pk_node node;
pk_cam cam;
pk_primitive model;
pk_texture tex;
pk_skeleton skel;
pk_bone_anim_data* anims;
pk_bone_anim_state anim_state;
sg_pipeline pip;

static void sound_loaded(const tm_buffer* buf, void* udata) {
    (void)udata;
    pk_play_sound(&sound, &(pk_sound_channel_desc){
        .buffer = buf,
        .loop = true,
        .node = &node,
        .range_min = 0.2f,
        .range_max = 20.f,
    });
}

static void model_loaded(m3d_t* m3d, void* udata) {
    (void)udata;
    bool ok = pk_load_m3d(&model, &node, m3d);
    pk_assert(ok);
    ok = pk_load_skeleton(&skel, m3d);
    pk_assert(ok);
    int count = 0;
    anims = pk_load_bone_anims(m3d, &count);
    anim_state.anim = &anims[0];
    anim_state.loop = true;
    pk_assert(count > 0);
    pk_release_m3d_data(m3d);
}

static void init(void) {
    pk_setup(&(pk_desc) {
        .gfx = {
            .environment = sglue_environment(),
            .logger.func = slog_func,
        },
        .fetch = {
            .logger.func = slog_func,
        }
    });
    pk_audio_setup(&(pk_audio_desc) {
        .saudio = {
            .num_channels = 2,
            .logger.func = slog_func,
        }
    });

    pk_load_sound_buffer(&(pk_sound_buffer_request) {
        .path = "assets/loop.ogg",
        .buffer = SFETCH_RANGE(ogg_buffer),
        .loaded_cb = sound_loaded,
    });

    pk_alloc_primitive(&model, 2, 0);
    pk_load_m3d_data(&(pk_m3d_request) {
        .path = "assets/cesium_man.m3d",
        .buffer = SFETCH_RANGE(m3d_buffer),
        .loaded_cb = model_loaded,
    });

    pk_checker_texture(&tex);
    pk_texture_primitive(&model, &tex, 0);

    pip = sg_make_pipeline(&(sg_pipeline_desc) {
        .shader = sg_make_shader(pk_skinned_phong_tex_shader_desc(sg_query_backend())),
        .layout = pk_skinned_layout(),
        .index_type = SG_INDEXTYPE_UINT32,
        .depth = {
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = true,
        },
    });

    pk_init_cam(&cam, &(pk_cam_desc){
        .mindist = 1.0f,
        .maxdist = 19.f,
        .sensitivity = 10.f,
    });

    pk_init_node(&node);
    node.position.Y -= 0.5f;

    listener = (pk_sound_listener){
        .position = cam.eyepos,
        .smoothing = 2.0f,
    };
}

static void frame(void) {
    sfetch_dowork();

    sg_begin_pass(&(sg_pass) {
        .swapchain = sglue_swapchain(),
        .action.colors[0] = {
            .load_action = SG_LOADACTION_CLEAR,
            .clear_value = (sg_color){0.2f, 0.0f, 0.2f, 1.0f},
        },
    });

    sg_apply_pipeline(pip);

    pk_update_cam(&cam, sapp_width(), sapp_height());
    pk_update_sound_listener(&listener, cam.eyepos, (float)sapp_frame_duration());

    pk_vs_params_t vs_params = {
        .model = pk_node_transform(&node),
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

    pk_bone_matrices_t bones = {0};
    pk_play_bone_anim(bones.bones, &skel, &anim_state, (float)sapp_frame_duration());

    sg_apply_uniforms(UB_pk_vs_params, &SG_RANGE(vs_params));
    sg_apply_uniforms(UB_pk_tex_material, &SG_RANGE(material));
    sg_apply_uniforms(UB_pk_dir_light, &SG_RANGE(light));
    sg_apply_uniforms(UB_pk_bone_matrices, &SG_RANGE(bones));

    pk_draw_primitive(&model, 1);

    sg_end_pass();
    sg_commit();
}

static void cleanup(void) {
    pk_audio_shutdown();
    pk_shutdown();
}

static void event(const sapp_event* e) {
    pk_cam_input(&cam, e);
}

sapp_desc sokol_main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    return (sapp_desc) {
        .init_cb = init,
        .frame_cb = frame,
        .cleanup_cb = cleanup,
        .event_cb = event,
        .width = 800,
        .height = 600,
        .swap_interval = 1,
        .win32_console_attach = true,
    };
}
