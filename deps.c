/*
It's recommended, to define the graphics backend for sokol
in your build script.
*/
#define SOKOL_IMPL
#define SOKOL_NO_DEPRECATED
#include "deps/sokol_gfx.h"
#include "deps/sokol_fetch.h"
#include "deps/sokol_audio.h"
#define SOKOL_SHDC_IMPL
#include "deps/hmm.h"
#include "shaders/shaders.glsl.h"
#ifndef PK_NO_AUDIO
#define TMIXER_IMPL
#include "deps/tmixer.h"
#endif

#define CUTE_PNG_IMPLEMENTATION
#include "deps/cute_png.h"
#define QOI_IMPLEMENTATION
#include "deps/qoi.h"
#define M3D_IMPLEMENTATION
#include "deps/m3d.h"
#define CGLTF_IMPLEMENTATION
#include "deps/cgltf.h"
