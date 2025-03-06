#pragma once
#include <stdlib.h>
#include <assert.h>

#ifndef pk_malloc
#define pk_malloc malloc
#endif
#ifndef pk_free
#define pk_free free
#endif
#ifndef pk_realloc
#define pk_realloc realloc
#endif
#ifndef pk_assert
#define pk_assert assert
#endif
#ifndef pk_memcpy
#include <string.h>
#define pk_memcpy memcpy
#endif

#ifndef PK_SHADER_ALIGN
#define PK_SHADER_ALIGN 16
#endif

#if defined(WIN32) && !defined(SOKOL_GLES3)
#define PK_REQUEST_DEDICATED_DEVICE \
_declspec(dllexport) unsigned long long NvOptimusEnablement = 1; \
_declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1; 
#else
#define PK_REQUEST_DEDICATED_DEVICE
#endif
