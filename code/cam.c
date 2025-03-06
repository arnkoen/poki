#include "cam.h"
#include "common.h"
#include "sokol_app.h"


#define CAMERA_DEFAULT_MIN_DIST (2.0f)
#define CAMERA_DEFAULT_MAX_DIST (50.0f)
#define CAMERA_DEFAULT_MIN_LAT (-85.0f)
#define CAMERA_DEFAULT_MAX_LAT (85.0f)
#define CAMERA_DEFAULT_DIST (10.0f)
#define CAMERA_DEFAULT_ASPECT (60.0f)
#define CAMERA_DEFAULT_NEARZ (1.0f)
#define CAMERA_DEFAULT_FARZ (1000.0f)
#define CAMERA_DEFAULT_SENSITIVITY (30.0f)

static float _cam_def(float val, float def) { return ((val == 0.0f) ? def : val); }

static HMM_Vec3 _cam_euclidean(float latitude, float longitude) {
    const float lat = latitude * HMM_DegToRad;
    const float lng = longitude * HMM_DegToRad;
    return HMM_V3(cosf(lat)* sinf(lng), sinf(lat), cosf(lat)* cosf(lng));
}

void pk_init_cam(pk_cam* cam, const pk_cam_desc* desc) {
    pk_assert(desc && cam);
    cam->mindist = _cam_def(desc->mindist, CAMERA_DEFAULT_MIN_DIST);
    cam->maxdist = _cam_def(desc->maxdist, CAMERA_DEFAULT_MAX_DIST);
    cam->minlat = _cam_def(desc->minlat, CAMERA_DEFAULT_MIN_LAT);
    cam->maxlat = _cam_def(desc->maxlat, CAMERA_DEFAULT_MAX_LAT);
    cam->distance = _cam_def(desc->distance, CAMERA_DEFAULT_DIST);
    cam->center = desc->center;
    cam->latitude = desc->latitude;
    cam->longitude = desc->longitude;
    cam->aspect = _cam_def(desc->aspect, CAMERA_DEFAULT_ASPECT);
    cam->nearz = _cam_def(desc->nearz, CAMERA_DEFAULT_NEARZ);
    cam->farz = _cam_def(desc->farz, CAMERA_DEFAULT_FARZ);
    cam->sensitivity = _cam_def(desc->sensitivity, CAMERA_DEFAULT_SENSITIVITY);
}

void pk_orbit_cam(pk_cam* cam, float dx, float dy) {
    pk_assert(cam);
    cam->longitude -= dx;
    if (cam->longitude < 0.0f) {
        cam->longitude += 360.0f;
    }
    if (cam->longitude > 360.0f) {
        cam->longitude -= 360.0f;
    }
    cam->latitude = HMM_Clamp(cam->minlat, cam->latitude + dy, cam->maxlat);
}

void pk_zoom_cam(pk_cam* cam, float d) {
    pk_assert(cam);
    cam->distance = HMM_Clamp(cam->mindist, cam->distance + d, cam->maxdist);
}

void pk_update_cam(pk_cam* cam, int fb_width, int fb_height) {
    pk_assert(cam);
    const float w = (float)fb_width;
    const float h = (float)fb_height;

    cam->eyepos =  HMM_AddV3(cam->center, HMM_MulV3F(_cam_euclidean(cam->latitude, cam->longitude), cam->distance));
    cam->view = HMM_LookAt_RH(cam->eyepos, cam->center, HMM_V3(0.0f, 1.0f, 0.0f));
    cam->proj = HMM_Perspective_RH_ZO(cam->aspect * HMM_DegToRad, w / h, 0.1f, 1000.f);
    cam->viewproj = HMM_MulM4(cam->proj, cam->view);
}

void pk_cam_input(pk_cam* cam, const sapp_event* ev) {
    pk_assert(cam);
    switch (ev->type) {
    case SAPP_EVENTTYPE_MOUSE_DOWN:
        if (ev->mouse_button == SAPP_MOUSEBUTTON_RIGHT) {
            sapp_lock_mouse(true);
        }
        break;
    case SAPP_EVENTTYPE_MOUSE_UP:
        if (ev->mouse_button == SAPP_MOUSEBUTTON_RIGHT) {
            sapp_lock_mouse(false);
        }
        break;
    case SAPP_EVENTTYPE_MOUSE_SCROLL:
        pk_zoom_cam(cam, ev->scroll_y * 0.9f);
        break;
    case SAPP_EVENTTYPE_MOUSE_MOVE:
        if (sapp_mouse_locked()) {
            pk_orbit_cam(cam, ev->mouse_dx * 0.25f, ev->mouse_dy * 0.25f);
        }
        break;
    default:
        break;
    }
}
