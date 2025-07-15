/*
Poki - minimal creative coding framework

Copyright (c) 2025, Arne Koenig
Redistribution and use in source and binary forms, with or without modification, are permitted.
THIS SOFTWARE IS PROVIDED 'AS-IS', WITHOUT ANY EXPRESS OR IMPLIED WARRANTY. IN NO EVENT WILL THE AUTHORS BE HELD LIABLE FOR ANY DAMAGES ARISING FROM THE USE OF THIS SOFTWARE.
*/

#pragma once

#include "deps/sokol_gfx.h"
#include "deps/sokol_fetch.h"
#include "deps/sokol_audio.h"
#include "deps/hmm.h"
#include "shaders/shaders.glsl.h"
#ifndef PK_NO_AUDIO
#include "deps/tmixer.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef pk_malloc
#define pk_malloc(x) malloc(x)
#endif
#ifndef pk_free
#define pk_free(x) free(x)
#endif
#ifndef pk_assert
#include <assert.h>
#define pk_assert(x) assert(x)
#endif
#ifndef pk_printf
#include <stdio.h>
#define pk_printf printf
#endif

#if defined(_WIN32) && !defined(SOKOL_GLES3)
#define PK_REQUEST_DEDICATED_DEVICE \
_declspec(dllexport) unsigned long NvOptimusEnablement = 1; \
_declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
#else
#define PK_REQUEST_DEDICATED_DEVICE
#endif

//TODO: Add useful render target helpers.

//--FORWARD--------------

typedef struct m3d_t m3d_t;
typedef struct cgltf_data cgltf_data;
typedef struct sapp_event sapp_event;


//--INIT&SHUTDOWN----------------------------------------

/*
TODO: Maybe allocate the memory needed for models, animations & Co upfront, to avoid fragmentation.
The buffer sizes should be configurable using pk_desc.
TODO: Add proper allocator interface and hook it to the dependencies.
*/

enum {
    PK_INIT_GFX = 1 << 0,
    PK_INIT_FETCH = 1 << 1,
    PK_INIT_AUDIO = 1 << 2,
};

typedef struct pk_audio_desc {
    saudio_desc saudio;
    tm_callbacks mixer_callbacks;
} pk_audio_desc;

typedef struct pk_desc {
    sg_desc gfx;
    sfetch_desc_t fetch;
    pk_audio_desc audio;
    int flags;
} pk_desc;

void pk_setup(const pk_desc* desc);
void pk_shutdown(void);


//--CAMERA------------------------------------------------------------
//modified from https://github.com/floooh/sokol-samples

typedef struct pk_cam_desc {
    float mindist;
    float maxdist;
    float minlat;
    float maxlat;
    float distance;
    float latitude;
    float longitude;
    float aspect;
    float nearz;
    float farz;
    float sensitivity;
    HMM_Vec3 center;
} pk_cam_desc;

typedef struct pk_cam {
    float mindist;
    float maxdist;
    float minlat;
    float maxlat;
    float distance;
    float latitude;
    float longitude;
    float aspect;
    float nearz;
    float farz;
    float sensitivity;
    HMM_Vec3 center;
    HMM_Vec3 eyepos;
    HMM_Mat4 view;
    HMM_Mat4 proj;
    HMM_Mat4 viewproj;
} pk_cam;

void pk_init_cam(pk_cam* cam, const pk_cam_desc* desc);
void pk_orbit_cam(pk_cam* cam, float dx, float dy);
void pk_zoom_cam(pk_cam* cam, float d);
void pk_update_cam(pk_cam* cam, int fb_width, int fb_height);
#ifndef PK_NO_SAPP
void pk_cam_input(pk_cam* cam, const sapp_event* ev);
#endif //PK_NO_SAPP


//--TEXTURES--------------------------------------------------------

typedef struct pk_texture_desc {
    int width, height;
    sg_image_usage usage;
    sg_range data;
    sg_pixel_format format;
    sg_filter min_filter;
    sg_filter mag_filter;
    sg_wrap wrap_u;
    sg_wrap wrap_v;
} pk_texture_desc;

typedef struct pk_texture {
    sg_image image;
    sg_sampler sampler;
} pk_texture;

void pk_init_texture(pk_texture* tex, const pk_texture_desc* desc);
void pk_checker_texture(pk_texture* tex);
void pk_update_texture(pk_texture* tex, sg_range data);
void pk_release_texture(pk_texture* tex);


//--NODE---------------------------------------------------------------------

#define PK_MAX_NAME_LEN 32

typedef struct pk_node {
    char name[PK_MAX_NAME_LEN];
    struct pk_node* parent;
    HMM_Vec3 position;
    HMM_Vec3 scale;
    HMM_Quat rotation;
} pk_node;

void pk_init_node(pk_node* node);
void pk_release_node(pk_node* node);
HMM_Mat4 pk_node_transform(const pk_node* node);


//--PRIMITIVE--------------------------------------------------------------

typedef struct pk_vertex_pnt {
    HMM_Vec3 pos;
    HMM_Vec3 nrm;
    HMM_Vec2 uv;
} pk_vertex_pnt;

sg_vertex_layout_state pk_pnt_layout(void);

typedef struct pk_vertex_skin {
    uint16_t indices[4];
    float weights[4];
} pk_vertex_skin;

sg_vertex_layout_state pk_skinned_layout(void);

typedef struct {
	sg_range vertices;
	sg_range indices;
	int num_elements;
    bool is_mutable;
} pk_primitive_desc;

typedef struct pk_primitive {
	sg_bindings bindings;
	int base_element;
	int num_elements;
} pk_primitive;

void pk_alloc_primitive(pk_primitive* primitive, uint16_t vubf_count, uint16_t sbuf_count);
void pk_init_primitive(pk_primitive* primitive, const pk_primitive_desc* desc);
bool pk_load_m3d(pk_primitive* mesh, pk_node* node, m3d_t* m3d);
void pk_release_primitive(pk_primitive* primitive);
void pk_texture_primitive(pk_primitive* primitive, const pk_texture* tex, int slot);
void pk_draw_primitive(const pk_primitive* primitive, int num_instances);


//--MESH-------------------------------------------------------------------

typedef struct pk_mesh {
    pk_primitive* primitives;
    uint16_t primitive_count;
    pk_node* node;
} pk_mesh;

void pk_draw_mesh(pk_mesh* mesh, pk_vs_params_t* vs_params);
void pk_release_mesh(pk_mesh* mesh);


//--MODEL-----------------------------------------------------------------

typedef struct pk_model {
    pk_mesh* meshes;
    uint16_t mesh_count;
    pk_node* nodes;
    uint16_t node_count;
} pk_model;

bool pk_load_gltf(pk_model* model, cgltf_data* data);
void pk_release_model(pk_model* model);
pk_node* pk_find_model_node(const pk_model*, const char* name);
void pk_set_model_texture(pk_model* model, const pk_texture* tex, int slot);
void pk_draw_model(pk_model* model, pk_vs_params_t* vs_params);


//--ANIMATION-------------------------------------------------------------

/*
TODO: Gltf and m3d animation handling works quite different right now.
This should change. Internally both use very different methods, to
evaluate the animations, in particular the bone animation thing needs an
overhaul, because it's not very memory efficient.
*/

//--GLTF---------------------------

typedef enum {
    PK_ANIM_PATH_UNDEFINED,
    PK_ANIM_PATH_TRANSLATION,
    PK_ANIM_PATH_ROTATION,
    PK_ANIM_PATH_SCALE,
} pk_gltf_anim_path_type;

typedef enum {
    PK_ANIM_INTERP_UNDEFINED,
    PK_ANIM_INTERP_LINEAR,
    PK_ANIM_INTERP_STEP,
    PK_ANIM_INTERP_CUBIC, //not used - atm we force cubic to linear
} pk_gltf_anim_interp_type;

typedef struct pk_gltf_keyframe {
    float time;
    float* value;
} pk_gltf_keyframe;

typedef struct pk_gltf_anim_channel {
    pk_node* target_node;
    pk_gltf_anim_path_type path;
    pk_gltf_keyframe* keyframes;
    int num_keyframes;
    pk_gltf_anim_interp_type interpolation;
} pk_gltf_anim_channel;

typedef struct pk_gltf_anim {
    pk_gltf_anim_channel* channels;
    int num_channels;
    float duration;
    float elapsed_time;
    bool loop;
    bool ready;
} pk_gltf_anim;

bool pk_load_gltf_anim(pk_gltf_anim* anim, pk_model* model, cgltf_data* data);
void pk_release_gltf_anim(pk_gltf_anim* anim);
void pk_play_gltf_anim(pk_gltf_anim* anim, float delta_time);

//--M3D------------------------------

typedef struct pk_bone {
    char name[PK_MAX_NAME_LEN];
    int parent;
} pk_bone;

typedef struct pk_transform {
    HMM_Vec3 pos;
    HMM_Quat rot;
    HMM_Vec3 scale;
} pk_transform;

typedef struct pk_skeleton {
    pk_bone* bones;
    int bone_count;
    pk_transform* bind_poses;
} pk_skeleton;

bool pk_load_skeleton(pk_skeleton* skeleton, m3d_t* m3d);
void pk_release_skeleton(pk_skeleton* skeleton);

typedef struct pk_bone_anim {
    int bone_count;
    int frame_count;
    pk_bone* bones;
    pk_transform** poses;
    float time;
    int frame;
} pk_bone_anim;

pk_bone_anim* pk_load_bone_anims(m3d_t* m3d, int* count);
void pk_play_bone_anim(HMM_Mat4* trs, pk_skeleton* skeleton, pk_bone_anim* anim, float dt);
void pk_release_bone_anim(pk_bone_anim* anim); //IMPLEMENT


//--SOUND------------------------------------------------------------------------

#ifndef PK_NO_AUDIO

typedef struct pk_sound_channel_desc {
    const tm_buffer* buffer;
    float range_min;
    float range_max;
    pk_node* node;
    bool loop;
} pk_sound_channel_desc;

typedef struct pk_sound {
    tm_channel channel;
    pk_node* node;
} pk_sound;

void pk_play_sound(pk_sound* sound, const pk_sound_channel_desc* desc);
void pk_update_sound(pk_sound* sound);
void pk_stop_sound(pk_sound* sound);

typedef struct pk_sound_listener {
    HMM_Vec3 position;
    float smoothing;
} pk_sound_listener;

void pk_update_sound_listener(pk_sound_listener* listener, HMM_Vec3 new_pos, float dt);

#endif

//--IO---------------------------------------------------------------------------

//TODO: add void* udata field to the request structs, to enable avoiding globals
//Maybe add optional automatic texture loading for models.

typedef void(*pk_fail_callback)(const sfetch_response_t* response);

//--IMAGE-LOADING----------

typedef struct pk_image_data {
    void* pixels;
    int width, height;
} pk_image_data;

typedef void(*pk_image_loaded_callback)(pk_image_data* image);

typedef struct pk_image_request {
    const char* path;
    sfetch_range_t buffer;
    pk_image_loaded_callback loaded_cb;
    pk_fail_callback fail_cb;
} pk_image_request;

//Supports .qoi and .png, will generate a 4x4 checker texture on fail, if no fail callback is provided.
sfetch_handle_t pk_load_image_data(const pk_image_request* req);

//--M3D-LOADING----------------

typedef void(*pk_m3d_loaded_callback)(m3d_t* m3d);

typedef struct pk_m3d_request {
    const char* path;
    sfetch_range_t buffer;
    pk_m3d_loaded_callback loaded_cb;
    pk_fail_callback fail_cb;
} pk_m3d_request;

sfetch_handle_t pk_load_m3d_data(const pk_m3d_request* req);
void pk_release_m3d_data(m3d_t* m3d);


//--GLTF-LOADING---------------

typedef void(*pk_gltf_loaded_callback)(cgltf_data* gltf);

typedef struct pk_gltf_request {
    const char* path;
    sfetch_range_t buffer;
    pk_gltf_loaded_callback loaded_cb;
    pk_fail_callback fail_cb;
} pk_gltf_request;

sfetch_handle_t pk_load_gltf_data(const pk_gltf_request* req);
void pk_release_gltf_data(cgltf_data* data);

//--SOUND-LOADING------------

#ifndef PK_NO_AUDIO

typedef void(*pk_sound_buffer_loaded_callback)(const tm_buffer* buffer);

typedef struct pk_sound_buffer_request {
    const char* path;
    sfetch_range_t buffer;
    pk_sound_buffer_loaded_callback loaded_cb;
    pk_fail_callback fail_cb;
} pk_sound_buffer_request;

sfetch_handle_t pk_load_sound_buffer(const pk_sound_buffer_request* req);
//since the header is included, just use tm_release_buffer(...) here.

#endif

#ifdef __cplusplus
} //extern "C"
#endif
