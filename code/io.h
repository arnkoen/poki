#pragma once
#include "sokol_gfx.h"
#include "sokol_fetch.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct pk_texture pk_texture;
    typedef struct pk_sound pk_sound;
    typedef struct pk_primitive pk_primitive;
    typedef struct pk_model pk_model;
    typedef struct pk_gltf_anim pk_gltf_anim;
    typedef struct pk_node pk_node;
    typedef struct m3d_t m3d_t;
    typedef struct cgltf_data cgltf_data;
    typedef struct tinymixer_buffer tinymixer_buffer;

    typedef void(*pk_fail_callback)(const sfetch_response_t* response);

    //--TEXTURES-----------------------------------

    typedef struct {
        sg_filter min_filter;
        sg_filter mag_filter;
        sg_wrap wrap_u;
        sg_wrap wrap_v;
    } pk_tex_opts;

    typedef struct pk_texture_request {
        const char* path;
        pk_texture* tex;
        pk_tex_opts opts; //optional
        sfetch_range_t buffer;
        pk_fail_callback fail_cb; //optional
    } pk_texture_request;

    sfetch_handle_t pk_load_texture(const pk_texture_request* req);

    //--M3D-----------------------------------

    typedef void(*pk_m3d_loaded_callback)(m3d_t* m3d);

    typedef struct pk_m3d_request {
        const char* path;
        sfetch_range_t buffer;
        pk_m3d_loaded_callback loaded_cb;
        pk_fail_callback fail_cb;
    } pk_m3d_request;

    sfetch_handle_t pk_load_m3d_data(const pk_m3d_request* req);
    void pk_release_m3d_data(m3d_t* m3d);
    bool pk_load_m3d(pk_primitive* mesh, pk_node* node, m3d_t* m3d);


    //--GLTF/GLB-----------------------------------

    typedef void(*pk_gltf_loaded_callback)(cgltf_data* gltf);

    typedef struct pk_gltf_request {
        const char* path;
        sfetch_range_t buffer;
        pk_gltf_loaded_callback loaded_cb;
        pk_fail_callback fail_cb;
    } pk_gltf_request;

    sfetch_handle_t pk_load_gltf_data(const pk_gltf_request* req);
    void pk_release_gltf_data(cgltf_data* data);
    bool pk_load_gltf(pk_model* model, cgltf_data* data);
    bool pk_load_gltf_anim(pk_gltf_anim* anim, pk_model* model, cgltf_data* data);

    //--SOUND--------------------------------------

    typedef void(*pk_sound_buffer_loaded_callback)(const tinymixer_buffer* buffer);

    typedef struct pk_sound_buffer_request {
        const char* path;
        sfetch_range_t buffer;
        pk_sound_buffer_loaded_callback loaded_cb;
        pk_fail_callback fail_cb;
    } pk_sound_buffer_request;

    sfetch_handle_t pk_load_sound_buffer(const pk_sound_buffer_request* req);

#ifdef __cplusplus
} //extern "C"
#endif

