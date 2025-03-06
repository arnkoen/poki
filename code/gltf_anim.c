#include "gltf_anim.h"
#include "model.h"
#include "node.h"
#include "common.h"
#include "log.h"

static float interpolate(float a, float b, float t) {
    return a + t * (b - a);
}

static float quat_dot(HMM_Quat q1, HMM_Quat q2) {
    return (q1.X * q2.X) + (q1.Y * q2.Y) + (q1.Z * q2.Z) + (q1.W * q2.W);
}

static HMM_Quat lerp_quat(HMM_Quat q1, float t, HMM_Quat q2) {
    float x = HMM_Lerp(q1.X, t, q2.X);
    float y = HMM_Lerp(q1.Y, t, q2.Y);
    float z = HMM_Lerp(q1.Z, t, q2.Z);
    float w = HMM_Lerp(q1.W, t, q2.W);
    return HMM_NormQ(HMM_Q(x, y, z, w));
}


static void find_keyframes(float time, pk_gltf_anim_channel* channel, int* key1, int* key2, float* t) {
    int num_keyframes = channel->num_keyframes;
    for (int i = 0; i < num_keyframes - 1; ++i) {
        if (time >= channel->keyframes[i].time && time <= channel->keyframes[i + 1].time) {
            *key1 = i;
            *key2 = i + 1;
            float time1 = channel->keyframes[i].time;
            float time2 = channel->keyframes[i + 1].time;
            *t = (time - time1) / (time2 - time1);
            return;
        }
    }
    // Clamp to the last keyframe if time is beyond the range.
    *key1 = num_keyframes - 1;
    *key2 = num_keyframes - 1;
    *t = 0.0f;
}

static void interpolate_animation(pk_gltf_anim_channel* channel, float current_time) {
    int key1 = 0, key2 = 0;
    float t = 0.f;
    find_keyframes(current_time, channel, &key1, &key2, &t);

    // For STEP interpolation, use the first keyframe's value.
    if (channel->interpolation == PK_ANIM_INTERP_STEP) {
        t = 0.0f;
    }

    pk_gltf_keyframe* kf1 = &channel->keyframes[key1];
    pk_gltf_keyframe* kf2 = &channel->keyframes[key2];

    if (channel->path == PK_ANIM_PATH_TRANSLATION) {
        float result[3] = { 0 };
        for (int i = 0; i < 3; ++i) {
            result[i] = interpolate(kf1->value[i], kf2->value[i], t);
        }
        channel->target_node->position = HMM_V3(result[0], result[1], result[2]);
    }
    else if (channel->path == PK_ANIM_PATH_ROTATION) {
        // Always use the first 4 components for rotation.
        HMM_Quat rot1 = HMM_Q(kf1->value[0], kf1->value[1], kf1->value[2], kf1->value[3]);
        HMM_Quat rot2 = HMM_Q(kf2->value[0], kf2->value[1], kf2->value[2], kf2->value[3]);

        // Ensure shortest path by flipping if necessary
        if (quat_dot(rot1, rot2) < 0.0f) {
            rot2.X = -rot2.X;
            rot2.Y = -rot2.Y;
            rot2.Z = -rot2.Z;
            rot2.W = -rot2.W;
        }

        if (quat_dot(rot1, rot2) > 0.9995f) {
            // Use linear interpolation for nearly identical quaternions
            channel->target_node->rotation = lerp_quat(rot1, t, rot2);
        }
        else {
            channel->target_node->rotation = HMM_NormQ(HMM_SLerp(rot1, t, rot2));
        }
    }
    else if (channel->path == PK_ANIM_PATH_SCALE) {
        float result[3] = { 0 };
        for (int i = 0; i < 3; ++i) {
            result[i] = interpolate(kf1->value[i], kf2->value[i], t);
        }
        channel->target_node->scale = HMM_V3(result[0], result[1], result[2]);
    }
}

//--PUBLIC--------------------------------------------------------------------------

void pk_play_gltf_anim(pk_gltf_anim* animation, float dt) {
    if (!animation->ready) return;
    animation->elapsed_time += dt;
    if (animation->loop) {
        animation->elapsed_time = fmodf(animation->elapsed_time, animation->duration);
    }
    else if (animation->elapsed_time > animation->duration) {
        animation->elapsed_time = animation->duration;
    }

    for (int i = 0; i < animation->num_channels; ++i) {
        pk_gltf_anim_channel* channel = &animation->channels[i];
        if (channel->target_node) {
            interpolate_animation(channel, animation->elapsed_time);
        }
    }
}

void pk_release_gltf_anim(pk_gltf_anim* anim) {
    for (int i = 0; i < anim->num_channels; ++i) {
        pk_gltf_anim_channel* channel = &anim->channels[i];
        pk_assert(channel);
        for (int j = 0; j < channel->num_keyframes; ++j) {
            if (channel->keyframes[j].value != NULL) {
                pk_free(channel->keyframes[j].value);
            }
        }
        if (channel->keyframes != NULL) {
            pk_free(channel->keyframes);
        }
    }
    pk_free(anim->channels);
}
