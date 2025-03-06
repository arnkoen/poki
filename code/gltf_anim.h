#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct pk_node pk_node;

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
        PK_ANIM_INTERP_CUBIC, // Not used — we force cubic to linear.
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
        bool loop;
        float elapsed_time;
        bool ready;
    } pk_gltf_anim;

    void pk_play_gltf_anim(pk_gltf_anim* anim, float elapsed_time);
    void pk_release_gltf_anim(pk_gltf_anim* anim);

#ifdef __cplusplus
} //extern "C"
#endif
