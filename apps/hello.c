#include "poki.h"

#include "sokol_app.h"
#include "sokol_debugtext.h"
#include "sokol_log.h"
#include "sokol_glue.h"
#include "sokol_color.h"

#include "shaders.glsl.h"

static pk_cam cam = { 0 };
static pk_node node = { 0 };
static pk_primitive mesh = { 0 };
static pk_model model = { 0 };
static pk_gltf_anim anim = { 0 };
static pk_texture tex = { 0 };
static pk_texture checker_tex = { 0 };
static pk_sound sound = { 0 };
static pk_sound_listener listener = { .smoothing = 2.5f };
static sg_pipeline pip = { 0 };

#define FILE_SIZE (1024*1024)
static uint8_t img_buf[FILE_SIZE*8];
static uint8_t snd_buf[FILE_SIZE*8];
static uint8_t msh_buf[FILE_SIZE];
static uint8_t mod_buf[FILE_SIZE];


static void sound_loaded(const tinymixer_buffer* buffer) {
    pk_init_sound(&sound, &(pk_sound_channel_desc) {
        .buffer = buffer,
		.node = &node,
		.range_min = 1.0f,
		.range_max = 60.f,
		.should_play = true,
		.loop = true,
    });
    tinymixer_release_buffer(buffer);
}

static void mesh_loaded(m3d_t* m3d) {
    bool ok = pk_load_m3d(&mesh, &node, m3d);
    pk_assert(ok);
    pk_release_m3d_data(m3d);
}

static void model_loaded(cgltf_data* gltf) {
    bool ok = pk_load_gltf(&model, gltf);
    pk_assert(ok);
    ok = pk_load_gltf_anim(&anim, &model, gltf);
    pk_assert(ok);
    pk_set_model_texture(&model, &checker_tex, 0);
    pk_release_gltf_data(gltf);
}

static void init(void) {
    log_set_level(LOG_DEBUG);

    sg_setup(&(sg_desc) {
        .environment = sglue_environment(),
        .logger.func = slog_func,
    });

    sdtx_setup(&(sdtx_desc_t) {
        .fonts[0] = sdtx_font_oric(),
        .logger.func = slog_func,
    });

    pk_sfx_init(&(pk_sfx_desc) {
        .buffer_frames = 512,
        .channel_count = 2,
        .sample_rate = 44100,
    });

    sfetch_setup(&(sfetch_desc_t) { 
        .logger.func = slog_func, 
        .max_requests = 8, 
        .num_channels = 1, 
    });

    pk_load_texture(&(pk_texture_request) {
        .tex = &tex,
        .path = "assets/sphere_col_spec.qoi",
		.buffer = SFETCH_RANGE(img_buf),
		.opts = {
			.min_filter = SG_FILTER_LINEAR,
			.mag_filter = SG_FILTER_LINEAR,
			.wrap_u = SG_WRAP_REPEAT,
			.wrap_v = SG_WRAP_REPEAT,
        },
    });

    pk_alloc_primitive(&mesh, 1, 0);
    pk_texture_primitive(&mesh, &tex, 0);

    pk_load_m3d_data(&(pk_m3d_request) {
		.path = "assets/sphere.m3d",
        .buffer = SFETCH_RANGE(msh_buf),
		.loaded_cb = mesh_loaded,
    });

    pk_load_gltf_data(&(pk_gltf_request) {
        .path = "assets/gltf.glb",
		.buffer = SFETCH_RANGE(mod_buf),
		.loaded_cb = model_loaded,
    });

    pk_load_sound_buffer(&(pk_sound_buffer_request) {
        .path = "assets/winds00.ogg",
        .loaded_cb = sound_loaded,
        .buffer = SFETCH_RANGE(snd_buf),
    });

    pk_checker_texture(&checker_tex);

    pk_init_cam(&cam, &(pk_cam_desc){ 0 });
    pk_init_node(&node);

    pip = pk_texture_phong_pip(&(pk_pip_desc) {
        .depth_state = {
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = true,
        },
    });
}

static void frame(void) {
    sfetch_dowork();

    pk_update_cam(&cam, sapp_width(), sapp_height());
    pk_play_gltf_anim(&anim, (float)sapp_frame_duration());

	pk_update_sound(&sound);
	pk_update_sound_listener(&listener, cam.eyepos, (float)sapp_frame_duration());

    sg_begin_pass(&(sg_pass) {
        .action.colors[0].clear_value = { 0, 0, 0, 1 },
        .action.colors[0].load_action = SG_LOADACTION_CLEAR,
        .swapchain = sglue_swapchain(),
    });

	pk_vs_params_t vs_params = {
		.viewproj = cam.viewproj,
	};
	pk_fs_params_t fs_params = { .viewpos = cam.eyepos };
	pk_tex_material_t mat = { 16.f };

	pk_dir_light_t light = {
		.ambient = sg_make_color_4b(50, 50, 50, 255),
		.diffuse = sg_make_color_4b(255, 200, 200, 255),
		.specular = sg_make_color_4b(150, 150, 150, 255),
		.direction = HMM_V3(0, -1, -1),
	};

	sg_apply_pipeline(pip);

	sg_apply_uniforms(UB_pk_fs_params, &SG_RANGE(fs_params));
	sg_apply_uniforms(UB_pk_tex_material, &SG_RANGE(mat));
	sg_apply_uniforms(UB_pk_dir_light, &SG_RANGE(light));

    vs_params.model = pk_node_transform(&node);
	sg_apply_uniforms(UB_pk_vs_params, &SG_RANGE(vs_params));
	pk_bind_primitive(&mesh);
	pk_draw_primitive(&mesh, 1);

    pk_draw_model(&model, &vs_params);

    sdtx_canvas(sapp_widthf() * 0.5f, sapp_heightf() * 0.5f);
    sdtx_origin(3.0f, 3.0f);
    sdtx_color3b(200, 200, 200);
    sdtx_printf("Node X: %f", node.position.X);
    sdtx_draw();

    sg_end_pass();
    sg_commit();
}

static void cleanup(void) {
    pk_sfx_shutdown();
    sdtx_shutdown();
    sg_shutdown();
}

static void event(const sapp_event* e) {
    pk_cam_input(&cam, e);
    if (e->type == SAPP_EVENTTYPE_KEY_DOWN && !e->key_repeat) {
        switch (e->key_code) {
        case SAPP_KEYCODE_ESCAPE: sapp_request_quit(); break;
        case SAPP_KEYCODE_F: sapp_toggle_fullscreen(); break;
        case SAPP_KEYCODE_A: node.position.X -= 1.0f; break;
        case SAPP_KEYCODE_D: node.position.X += 1.0f; break;
        default: break;
        }
    }
}


sapp_desc sokol_main(int argc, char* argv[]) {
    return (sapp_desc) {
        .init_cb = init,
        .frame_cb = frame,
        .cleanup_cb = cleanup,
        .event_cb = event,
        .width = 1280,
        .height = 720,
        .window_title = "hello",
        .icon.sokol_default = true,
        .win32_console_attach = true,
        .sample_count = 2,
        .swap_interval = 1,
		.html5_canvas_resize = true,
        .html5_update_document_title = true,
    };
}
