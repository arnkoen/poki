#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct pk_primitive pk_primitive;
    typedef struct pk_node pk_node;

    typedef struct pk_mesh {
        pk_primitive* primitives;
        uint16_t primitive_count;
        pk_node* node;
    } pk_mesh;

    void pk_release_mesh(pk_mesh* mesh);

#ifdef __cplusplus
} // extern "C"
#endif 
