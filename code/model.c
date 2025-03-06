#include "model.h"
#include "mesh.h"
#include "primitive.h"
#include "node.h"
#include "common.h"

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
