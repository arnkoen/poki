#pragma once
#include "common.h"

#if defined(_MSC_VER)
#define PK_UNIFORMS(x) __declspec(align(PK_SHADER_ALIGN)) typedef struct x
#else
#define PK_UNIFORMS(x) __attribute__((aligned(PK_SHADER_ALIGN))) typedef struct x
#endif

