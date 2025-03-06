#include "mesh.h"
#include "common.h"
#include "primitive.h"

void pk_release_mesh(pk_mesh* mesh) {
    pk_assert(mesh);
    for (uint16_t i = 0; i < mesh->primitive_count; ++i) {
        pk_release_primitive(&mesh->primitives[i]);
    }
}
