#include "node.h"
#include "common.h"

pk_node* pk_new_node(void) {
    pk_node* ret = (pk_node*)pk_malloc(sizeof(pk_node));
    pk_assert(ret);
    pk_init_node(ret);
    return ret;
}

void pk_init_node(pk_node* node) {
    pk_assert(node);
    node->name = NULL;
    node->parent = NULL;
    node->position = HMM_V3(0, 0, 0);
    node->scale = HMM_V3(1, 1, 1);
    node->rotation = HMM_Q(0, 0, 0, 1);
}

HMM_Mat4 pk_node_transform(const pk_node* node) {
    pk_assert(node);
    
    HMM_Mat4 pos = HMM_Translate(node->position);
    HMM_Mat4 rot = HMM_QToM4(node->rotation);
    HMM_Mat4 scl = HMM_Scale(node->scale);
    
    HMM_Mat4 ret = HMM_MulM4(pos, HMM_MulM4(rot, scl));

    if (node->parent != NULL) {
        HMM_Mat4 parent_transform = pk_node_transform(node->parent);
        ret = HMM_MulM4(parent_transform, ret);
    }
    return ret;
}
