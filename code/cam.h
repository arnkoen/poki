#pragma once
#include "hmm.h"

//modified from https://github.com/floooh/sokol-samples

#ifdef __cplusplus
extern "C" {
#endif

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

    typedef struct sapp_event sapp_event;

    void pk_init_cam(pk_cam* cam, const pk_cam_desc* desc);
    void pk_orbit_cam(pk_cam* cam, float dx, float dy);
    void pk_zoom_cam(pk_cam* cam, float d);
    void pk_update_cam(pk_cam* cam, int fb_width, int fb_height);
    void pk_cam_input(pk_cam* cam, const sapp_event* ev);

#ifdef __cplusplus
} // extern "C"
#endif
