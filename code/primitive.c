#include "primitive.h"
#include "common.h"

#define GFX_DEF(val, def) ((val == 0) ? def : val)

void pk_alloc_primitive(pk_primitive* primitive, uint16_t vbuf_count, uint16_t sbuf_count) {
    primitive->bindings.index_buffer = sg_alloc_buffer();

    for (uint16_t i = 0; i < vbuf_count; ++i) {
        if (i > SG_MAX_VERTEXBUFFER_BINDSLOTS) break;
        primitive->bindings.vertex_buffers[i] = sg_alloc_buffer();
    }

    for (uint16_t i = 0; i < sbuf_count; ++i) {
        if (i > SG_MAX_STORAGEBUFFER_BINDSLOTS) break;
        primitive->bindings.storage_buffers[i] = sg_alloc_buffer();
    }
}

void pk_init_primitive(pk_primitive* primitive, const pk_primitive_desc* desc) {
    pk_assert(primitive && desc);
    pk_release_primitive(primitive);
    sg_buffer_desc bufdesc = { 0 };
    bufdesc.type = SG_BUFFERTYPE_VERTEXBUFFER,
    bufdesc.data = desc->vertices;
    bufdesc.usage = GFX_DEF(desc->usage, SG_USAGE_IMMUTABLE);
    primitive->bindings.vertex_buffers[0] = sg_make_buffer(&bufdesc);

    if (desc->indices.size != 0) {
        bufdesc.data = desc->indices;
        bufdesc.type = SG_BUFFERTYPE_INDEXBUFFER;
        primitive->bindings.index_buffer = sg_make_buffer(&bufdesc);
    }

    primitive->base_element = 0;
    primitive->num_elements = desc->num_elements;
}

void pk_release_primitive(pk_primitive* primitive) {
    pk_assert(primitive);
    for (int i = 0; i < SG_MAX_VERTEXBUFFER_BINDSLOTS; ++i) {
        sg_destroy_buffer(primitive->bindings.vertex_buffers[i]);
    }
    for (int i = 0; i < SG_MAX_STORAGEBUFFER_BINDSLOTS; ++i) {
        sg_destroy_buffer(primitive->bindings.storage_buffers[i]);
    }
    sg_destroy_buffer(primitive->bindings.index_buffer);
    primitive->has_textures = false;
}

void pk_texture_primitive(pk_primitive* primitive, const pk_texture* tex, int slot) {
    pk_assert(primitive && tex);
    primitive->bindings.images[slot] = tex->image;
    primitive->bindings.samplers[slot] = tex->sampler;
    primitive->has_textures = true;
}

void pk_bind_primitive(const pk_primitive* primitive) {
    pk_assert(primitive);
    sg_apply_bindings(&primitive->bindings);
}

void pk_draw_primitive(const pk_primitive* primitive, int num_instances) {
    pk_assert(primitive);
    sg_draw(primitive->base_element, primitive->num_elements, num_instances);
}
