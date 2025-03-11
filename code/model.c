#include "model.h"
#include "mesh.h"
#include "primitive.h"
#include "node.h"
#include "common.h"
#include "shaders.glsl.h"

pk_node* pk_find_model_node(const pk_model* model, const char* name) {
    pk_assert(model);
    for (int i = 0; i < model->node_count; ++i) {
        pk_node* node = &model->nodes[i];
        if (strcmp(node->name, name) == 0) {
            return node; // Found matching node by name
        }
    }
    return NULL;
}

void pk_set_model_texture(pk_model* model, const pk_texture* tex, int slot) {
    for (uint16_t i = 0; i < model->mesh_count; ++i) {
        pk_mesh* mesh = &model->meshes[i];
        for (uint16_t j = 0; j < mesh->primitive_count; j++) {
            pk_texture_primitive(&mesh->primitives[j], tex, slot);
        }
    }
}

void pk_draw_model(pk_model* model, pk_vs_params_t* vs_params) {
    for (uint16_t i = 0; i < model->mesh_count; ++i) {
        pk_mesh* mesh = &model->meshes[i];
        vs_params->model = pk_node_transform(mesh->node);
        sg_apply_uniforms(UB_pk_vs_params, &(sg_range){vs_params, sizeof(pk_vs_params_t)});
        for (uint16_t j = 0; j < mesh->primitive_count; ++j) {
            pk_bind_primitive(&mesh->primitives[j]);
            pk_draw_primitive(&mesh->primitives[j], 1);
        }
    }
}

void pk_release_model(pk_model* model) {
    for (uint16_t i = 0; i < model->mesh_count; ++i) {
        pk_mesh* mesh = &model->meshes[i];
        for (uint16_t j = 0; j < mesh->primitive_count; j++) {
            pk_release_primitive(&mesh->primitives[j]);
        }
    }
    pk_free(model->meshes);
    pk_free(model->nodes);
}
