#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct pk_mesh pk_mesh;
    typedef struct pk_node pk_node;
    typedef struct pk_texture pk_texture;
    typedef struct pk_vs_params_t pk_vs_params_t;

    typedef struct pk_model {
        pk_mesh* meshes;
        uint16_t mesh_count;
        pk_node* nodes;
        uint16_t node_count;
    } pk_model;

    pk_node* pk_find_model_node(const pk_model*, const char* name);
    void pk_set_model_texture(pk_model* model, const pk_texture* tex, int slot);
    void pk_draw_model(pk_model* model, pk_vs_params_t* vs_params);
    void pk_release_model(pk_model* model);

#ifdef __cplusplus
} // extern "C"
#endif 
