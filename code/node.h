#pragma once
#include "hmm.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct pk_node {
        const char* name;
        struct pk_node* parent;
        HMM_Vec3 position;
        HMM_Vec3 scale;
        HMM_Quat rotation;
    } pk_node;

    pk_node* pk_new_node(void);
    void pk_init_node(pk_node* node);
    HMM_Mat4 pk_node_transform(const pk_node* node);

#ifdef __cplusplus
} // extern "C"
#endif 
