#pragma once
#include "texture.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	float pos[3];
	float norm[3];
	float uv[2];
} pk_vertex_pnt;

typedef struct {
	uint32_t joints;
	uint32_t weights;
} pk_vertex_skin;

typedef struct {
	sg_range vertices;
	sg_range indices;
	int num_elements;
	sg_usage usage;
} pk_primitive_desc;

typedef struct pk_primitive {
	sg_bindings bindings;
	int base_element;
	int num_elements;
	bool has_textures;
} pk_primitive;

void pk_alloc_primitive(pk_primitive* primitive, uint16_t vubf_count, uint16_t sbuf_count);
void pk_init_primitive(pk_primitive* primitive, const pk_primitive_desc* desc);
void pk_release_primitive(pk_primitive* primitive);
void pk_texture_primitive(pk_primitive* primitive, const pk_texture* tex, int slot);
void pk_bind_primitive(const pk_primitive* primitive);
void pk_draw_primitive(const pk_primitive* primitive, int num_instances);

#ifdef __cplusplus
} // extern "C"
#endif 

