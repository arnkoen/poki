#include "poki.h"
#ifndef PK_NO_SAPP
#include "deps/sokol_app.h"
#endif //PK_NO_SAPP
#include "deps/qoi.h"
#include "deps/cute_png.h"
#include "deps/m3d.h"
#include "deps/cgltf.h"

#include <string.h>

#define PK_DEF(val, def) ((val == 0) ? def : val)

static void _pk_stream_cb(float* buffer, int num_frames, int num_channels, void* udata) {
    (void)num_channels; (void)udata;
    tm_getsamples(buffer, num_frames);
}

void pk_setup(const pk_desc* desc) {
    if (desc->flags & PK_INIT_GFX) {
        sg_setup(&desc->gfx);
    }
#ifndef PK_NO_AUDIO
    if (desc->flags & PK_INIT_AUDIO) {
        saudio_desc ad = { 0 };
        memcpy(&ad, &desc->audio.saudio, sizeof(saudio_desc));
        if (!ad.stream_cb && !ad.stream_userdata_cb) {
            ad.stream_userdata_cb = _pk_stream_cb;
        }
        saudio_setup(&ad);
        tm_init(desc->audio.mixer_callbacks, saudio_sample_rate());
    }
#endif
    if (desc->flags & PK_INIT_FETCH) {
        sfetch_setup(&desc->fetch);
    }
}

void pk_shutdown(void) {
    if (sfetch_valid()) {
        sfetch_shutdown();
    }
    if (saudio_isvalid()) {
        saudio_shutdown();
    }
    if (sg_isvalid()) {
        sg_shutdown();
    }
}


//---------------------------------------------------------------------------------
//--CAMERA-------------------------------------------------------------------------
//---------------------------------------------------------------------------------


#define CAM_DEF_MIN_DIST (2.0f)
#define CAM_DEF_MAX_DIST (50.0f)
#define CAM_DEF_MIN_LAT (-85.0f)
#define CAM_DEF_MAX_LAT (85.0f)
#define CAM_DEF_DIST (10.0f)
#define CAM_DEF_ASPECT (60.0f)
#define CAM_DEF_NEARZ (1.0f)
#define CAM_DEF_FARZ (1000.0f)
#define CAM_DEF_SENSITIVITY (30.0f)

static HMM_Vec3 _cam_euclidean(float latitude, float longitude) {
    const float lat = latitude * HMM_DegToRad;
    const float lng = longitude * HMM_DegToRad;
    return HMM_V3(cosf(lat) * sinf(lng), sinf(lat), cosf(lat) * cosf(lng));
}

void pk_init_cam(pk_cam* cam, const pk_cam_desc* desc) {
    pk_assert(desc && cam);
    cam->mindist = PK_DEF(desc->mindist, CAM_DEF_MIN_DIST);
    cam->maxdist = PK_DEF(desc->maxdist, CAM_DEF_MAX_DIST);
    cam->minlat = PK_DEF(desc->minlat, CAM_DEF_MIN_LAT);
    cam->maxlat = PK_DEF(desc->maxlat, CAM_DEF_MAX_LAT);
    cam->distance = PK_DEF(desc->distance, CAM_DEF_DIST);
    cam->center = desc->center;
    cam->latitude = desc->latitude;
    cam->longitude = desc->longitude;
    cam->aspect = PK_DEF(desc->aspect, CAM_DEF_ASPECT);
    cam->nearz = PK_DEF(desc->nearz, CAM_DEF_NEARZ);
    cam->farz = PK_DEF(desc->farz, CAM_DEF_FARZ);
    cam->sensitivity = PK_DEF(desc->sensitivity, CAM_DEF_SENSITIVITY);
}

void pk_orbit_cam(pk_cam* cam, float dx, float dy) {
    pk_assert(cam);
    cam->longitude -= dx;
    if (cam->longitude < 0.0f) {
        cam->longitude += 360.0f;
    }
    if (cam->longitude > 360.0f) {
        cam->longitude -= 360.0f;
    }
    cam->latitude = HMM_Clamp(cam->minlat, cam->latitude + dy, cam->maxlat);
}

void pk_zoom_cam(pk_cam* cam, float d) {
    pk_assert(cam);
    cam->distance = HMM_Clamp(cam->mindist, cam->distance + d, cam->maxdist);
}

void pk_update_cam(pk_cam* cam, int fb_width, int fb_height) {
    pk_assert(cam);
    const float w = (float)fb_width;
    const float h = (float)fb_height;

    cam->eyepos =  HMM_AddV3(cam->center, HMM_MulV3F(_cam_euclidean(cam->latitude, cam->longitude), cam->distance));
    cam->view = HMM_LookAt_RH(cam->eyepos, cam->center, HMM_V3(0.0f, 1.0f, 0.0f));
    cam->proj = HMM_Perspective_RH_ZO(cam->aspect * HMM_DegToRad, w / h, 0.1f, 1000.f);
    cam->viewproj = HMM_MulM4(cam->proj, cam->view);
}

#ifndef PK_NO_SAPP
void pk_cam_input(pk_cam* cam, const sapp_event* ev) {
    pk_assert(cam);
    switch (ev->type) {
    case SAPP_EVENTTYPE_MOUSE_DOWN:
        if (ev->mouse_button == SAPP_MOUSEBUTTON_RIGHT) {
            sapp_lock_mouse(true);
        }
        break;
    case SAPP_EVENTTYPE_MOUSE_UP:
        if (ev->mouse_button == SAPP_MOUSEBUTTON_RIGHT) {
            sapp_lock_mouse(false);
        }
        break;
    case SAPP_EVENTTYPE_MOUSE_SCROLL:
        pk_zoom_cam(cam, ev->scroll_y * (cam->sensitivity * 0.005f));
        break;
    case SAPP_EVENTTYPE_MOUSE_MOVE:
        if (sapp_mouse_locked()) {
            pk_orbit_cam(cam, ev->mouse_dx * 0.25f, ev->mouse_dy * 0.25f);
        }
        break;
    default:
        break;
    }
}
#endif //PK_NO_SAPP


//---------------------------------------------------------------------------------
//--TEXTURES-----------------------------------------------------------------------
//---------------------------------------------------------------------------------


#define PK_DEF_IMG_SIZE 256

static const uint32_t _checker_pixels[4 * 4] = {
    0xFFAAAAAA, 0xFF555555, 0xFFAAAAAA, 0xFF555555,
    0xFF555555, 0xFFAAAAAA, 0xFF555555, 0xFFAAAAAA,
    0xFFAAAAAA, 0xFF555555, 0xFFAAAAAA, 0xFF555555,
    0xFF555555, 0xFFAAAAAA, 0xFF555555, 0xFFAAAAAA,
};

static sg_image_desc _checker_image_desc(void) {
    sg_image_desc desc = { 0 };
    desc.width = 4;
    desc.height = 4;
    desc.data.subimage[0][0] = SG_RANGE(_checker_pixels);
    return desc;
}

void pk_init_texture(pk_texture* tex, const pk_texture_desc* desc) {
    pk_assert(tex);
    sg_image_desc img = { 0 };
    img.usage= desc->usage;
    img.data.subimage[0][0] = desc->data;
    img.width = PK_DEF(desc->width, PK_DEF_IMG_SIZE);
    img.height = PK_DEF(desc->height, PK_DEF_IMG_SIZE);
    img.pixel_format = desc->format;
    tex->image = sg_make_image(&img);

    sg_sampler_desc sd = { 0 };
    sd.min_filter = desc->min_filter;
    sd.mag_filter = desc->mag_filter;
    sd.wrap_u = desc->wrap_u;
    sd.wrap_v = desc->wrap_v;
    tex->sampler = sg_make_sampler(&sd);
}

void pk_checker_texture(pk_texture* tex) {
    pk_assert(tex);
    sg_image_desc img = _checker_image_desc();
    tex->image = sg_make_image(&img);
    tex->sampler = sg_make_sampler(&(sg_sampler_desc) { 0 });
}

void pk_update_texture(pk_texture* tex, sg_range data) {
    pk_assert(tex && data.ptr);
    sg_image_data img = { 0 };
    img.subimage[0][0] = data;
    sg_update_image(tex->image, &img);
}

void pk_release_texture(pk_texture* tex) {
    pk_assert(tex);
    sg_destroy_image(tex->image);
    sg_destroy_sampler(tex->sampler);
}


//---------------------------------------------------------------------------------
//--PRIMITIVE----------------------------------------------------------------------
//---------------------------------------------------------------------------------


sg_vertex_layout_state pk_pnt_layout() {
    return (sg_vertex_layout_state) {
        .buffers[0].stride = sizeof(pk_vertex_pnt),
        .attrs = {
            [0].format = SG_VERTEXFORMAT_FLOAT3,
            [1].format = SG_VERTEXFORMAT_FLOAT3,
            [2].format = SG_VERTEXFORMAT_FLOAT2,
        },
    };
}

sg_vertex_layout_state pk_skinned_layout() {
    return (sg_vertex_layout_state) {
        .buffers = {
            [0].stride = sizeof(pk_vertex_pnt),
            [1].stride = sizeof(pk_vertex_skin),
        },
        .attrs = {
            [0] = {.buffer_index = 0, .format = SG_VERTEXFORMAT_FLOAT3},
            [1] = {.buffer_index = 0, .format = SG_VERTEXFORMAT_FLOAT3},
            [2] = {.buffer_index = 0, .format = SG_VERTEXFORMAT_FLOAT2},
            [3] = {.buffer_index = 1, .format = SG_VERTEXFORMAT_USHORT4},
            [4] = {.buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT4},
        }
    };
}

void pk_alloc_primitive(pk_primitive* primitive, uint16_t vbuf_count, uint16_t sbuf_count) {
    pk_assert(primitive &&
        vbuf_count < SG_MAX_VERTEXBUFFER_BINDSLOTS &&
        sbuf_count < SG_MAX_STORAGEBUFFER_BINDSLOTS
    );

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
    bufdesc.usage.vertex_buffer = true;
    bufdesc.usage.immutable = !desc->is_mutable;
    bufdesc.data = desc->vertices;
    primitive->bindings.vertex_buffers[0] = sg_make_buffer(&bufdesc);

    if (desc->indices.size != 0) {
        bufdesc.data = desc->indices;
        bufdesc.usage.immutable = !desc->is_mutable;
        bufdesc.usage.index_buffer = true;
        primitive->bindings.index_buffer = sg_make_buffer(&bufdesc);
    }

    primitive->base_element = 0;
    primitive->num_elements = desc->num_elements;
}

/*
TODO: The index buffer which is generated here is very silly (0, 1, 2, ...)
Do something clever about that.
*/
bool pk_load_m3d(pk_primitive* prim, pk_node* node, m3d_t* m3d) {
    pk_assert(m3d && prim);
    uint32_t total_vertices = m3d->numface * 3;

    pk_vertex_pnt* pnt = pk_malloc(total_vertices * sizeof(pk_vertex_pnt));
    pk_assert(pnt);

    bool has_skin = (m3d->numbone > 0 && m3d->numskin > 0);
    pk_vertex_skin* skin = NULL;
    if (has_skin) {
        skin = pk_malloc(total_vertices * sizeof(pk_vertex_skin));
        pk_assert(skin);
    }

    uint32_t* indices = pk_malloc(sizeof(uint32_t) * total_vertices);
    pk_assert(indices);

    unsigned int k = 0;
    for (unsigned int i = 0; i < m3d->numface; i++) {
        for (unsigned int j = 0; j < 3; j++, k++) {
            uint32_t outIndex = i * 3 + j;
            indices[outIndex] = outIndex;

            memcpy(&pnt[k].pos.X, &m3d->vertex[m3d->face[i].vertex[j]].x, 3 * sizeof(float));
            memcpy(&pnt[k].nrm.X, &m3d->vertex[m3d->face[i].normal[j]].x, 3 * sizeof(float));

            if (m3d->tmap && m3d->face[i].texcoord[j] < m3d->numtmap) {
                pnt[k].uv.U = m3d->tmap[m3d->face[i].texcoord[j]].u;
                pnt[k].uv.V = 1.0f - m3d->tmap[m3d->face[i].texcoord[j]].v;
            }
            else {
                pnt[k].uv = HMM_V2(0.f, 0.f);
            }

            if(has_skin) {
                unsigned int s = m3d->vertex[m3d->face[i].vertex[j]].skinid;
                if (s != M3D_UNDEF) {
                    for (int b = 0; b < 4; b++) {
                        //printf("boneindex: %i\n", app.model->skin[s].boneid[b]);
                        skin[k].indices[b] = (uint16_t)m3d->skin[s].boneid[b];
                        skin[k].weights[b] = m3d->skin[s].weight[b];
                    }
                }
                else {
                    // If no skinning, default to one full-weight bone (or identity)
                    skin[k].indices[0] = 0;
                    skin[k].weights[0] = 1.0f;
                    for (int b = 1; b < 4; b++) {
                        skin[k].indices[b] = 0;
                        skin[k].weights[b] = 0.0f;
                    }
                }
            }
        }
    }

    sg_buffer_desc bd = { 0 };
    bd.usage.vertex_buffer = true;
    bd.usage.immutable = true;
    bd.data = (sg_range){ pnt, total_vertices * sizeof(pk_vertex_pnt) };
    sg_init_buffer(prim->bindings.vertex_buffers[0], &bd);

    if (has_skin) {
        bd.usage.vertex_buffer = true;
        bd.usage.immutable = true;
        bd.data = (sg_range){ skin, total_vertices * sizeof(pk_vertex_skin) };
        sg_init_buffer(prim->bindings.vertex_buffers[1], &bd);
    }

    bd.usage.index_buffer = true;
    bd.data = (sg_range){ indices, total_vertices * sizeof(uint32_t) };
    sg_init_buffer(prim->bindings.index_buffer, &bd);

    /*
    We could bake the scale into the vertex data, but let's instead scale the node,
    if there is one.
    */
    if (node) {
        node->scale.X = m3d->scale;
        node->scale.Y = m3d->scale;
        node->scale.Z = m3d->scale;
    }

    prim->base_element = 0;
    prim->num_elements = total_vertices;

    pk_free(pnt);
    pk_free(indices);
    if(skin != NULL) { pk_free(skin); }
    pk_printf("Loaded pk_primitive %s\n", m3d->name);
    return true;
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
}

void pk_texture_primitive(pk_primitive* primitive, const pk_texture* tex, int slot) {
    pk_assert(primitive && tex && slot < SG_MAX_IMAGE_SAMPLER_PAIRS);
    primitive->bindings.images[slot] = tex->image;
    primitive->bindings.samplers[slot] = tex->sampler;
}

void pk_draw_primitive(const pk_primitive* primitive, int num_instances) {
    pk_assert(primitive);
    sg_apply_bindings(&primitive->bindings);
    sg_draw(primitive->base_element, primitive->num_elements, num_instances);
}


//---------------------------------------------------------------------------------
//--NODE---------------------------------------------------------------------------
//---------------------------------------------------------------------------------


void pk_init_node(pk_node* node) {
    pk_assert(node);
    strncpy(node->name, "UNNAMED", PK_MAX_NAME_LEN - 1);
    node->name[PK_MAX_NAME_LEN - 1] = '\0';
    node->parent = NULL;
    node->position = HMM_V3(0, 0, 0);
    node->scale = HMM_V3(1, 1, 1);
    node->rotation = HMM_Q(0, 0, 0, 1);
}

void pk_release_node(pk_node* node) {
    pk_assert(node);
    pk_free(node->name);
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


//---------------------------------------------------------------------------------
//--MESH---------------------------------------------------------------------------
//---------------------------------------------------------------------------------


void pk_release_mesh(pk_mesh* mesh) {
    pk_assert(mesh);
    for (uint16_t i = 0; i < mesh->primitive_count; ++i) {
        pk_release_primitive(&mesh->primitives[i]);
    }
}

void pk_draw_mesh(pk_mesh* mesh, pk_vs_params_t* vs_params) {
    pk_assert(mesh && vs_params);
    vs_params->model = pk_node_transform(mesh->node);
    sg_apply_uniforms(UB_pk_vs_params, &(sg_range){vs_params, sizeof(pk_vs_params_t)});
    for (uint16_t i = 0; i < mesh->primitive_count; ++i) {
        pk_draw_primitive(&mesh->primitives[i], 1);
    }
}


//---------------------------------------------------------------------------------
//--MODEL/GLTF---------------------------------------------------------------------
//---------------------------------------------------------------------------------


static uint32_t* load_indices(const cgltf_primitive* prim, size_t* index_count) {
    if (!prim->indices) {
        pk_printf("No indices found in model!\n");
        return NULL;
    }

    const cgltf_accessor* index_accessor = prim->indices;
    size_t gltf_index_count = index_accessor->count;
    *index_count = gltf_index_count;
    uint32_t* indices = pk_malloc(gltf_index_count * sizeof(uint32_t));
    pk_assert(indices);

    for (size_t i = 0; i < gltf_index_count; ++i) {
        uint32_t index = 0;
        cgltf_accessor_read_uint(index_accessor, i, &index, 1);
        indices[i] = index;
    }

    return indices;
}

static pk_vertex_pnt* interleave_attributes(const cgltf_primitive* primitive, size_t* vertex_count) {
    cgltf_accessor* position_accessor = NULL;
    cgltf_accessor* normal_accessor = NULL;
    cgltf_accessor* uv_accessor = NULL;

    for (size_t i = 0; i < primitive->attributes_count; ++i) {
        const cgltf_attribute* attribute = &primitive->attributes[i];

        switch (attribute->type) {
        case cgltf_attribute_type_position:
            position_accessor = attribute->data;
            break;
        case cgltf_attribute_type_normal:
            normal_accessor = attribute->data;
            break;
        case cgltf_attribute_type_texcoord:
            uv_accessor = attribute->data;
            break;
        default:
            break;
        }
    }

    //This should fail, if there is not at least a position attribute...
    if (!position_accessor) {
        pk_printf("No position attribute found in gltf primitive!\n");
        return NULL;
    }

    size_t gltf_vertex_count = position_accessor->count;
    *vertex_count = gltf_vertex_count;

    pk_vertex_pnt* interleaved = pk_malloc(gltf_vertex_count * sizeof(pk_vertex_pnt));

    for (size_t i = 0; i < gltf_vertex_count; ++i) {
        pk_vertex_pnt vertex = {0};

        if (position_accessor) {
            cgltf_accessor_read_float(position_accessor, i, vertex.pos.Elements, 3);
        }
        if (normal_accessor) {
            cgltf_accessor_read_float(normal_accessor, i, vertex.nrm.Elements, 3);
        }
        if (uv_accessor) {
            cgltf_accessor_read_float(uv_accessor, i, vertex.uv.Elements, 2);
        }

        interleaved[i] = vertex;
    }

    return interleaved;
}

// keep as reminder for skinning
/*
static array_t<pk_vertex_skin> interleave_attributes_skin(const cgltf_primitive& primitive) {
    array_t<pk_vertex_skin> interleaved = {};

    const cgltf_accessor* jointsAccessor = nullptr;
    const cgltf_accessor* weightsAccessor = nullptr;

    // Find the accessors for joints and weights
    for (size_t i = 0; i < primitive.attributes_count; ++i) {
        const cgltf_attribute& attribute = primitive.attributes[i];
        switch (attribute.type) {
        case cgltf_attribute_type_joints:
            jointsAccessor = attribute.data;
            break;
        case cgltf_attribute_type_weights:
            weightsAccessor = attribute.data;
            break;
        default:
            break;
        }
    }

    if (weightsAccessor == nullptr || jointsAccessor == nullptr) {
        fprintf(stderr, "Missing skinning attributes (joints or weights) in primitive.\n");
        return interleaved;
    }

    size_t vertex_count = weightsAccessor->count;
    interleaved.resize(vertex_count);

    for (size_t i = 0; i < vertex_count; ++i) {
        pk_vertex_skin vertex = {};

        // Read joint indices (uint8_t)
        uint32_t joints[4] = { 0 };
        cgltf_accessor_read_uint(jointsAccessor, i, (uint32_t*)joints, 4);
        vertex.joints = pack_u32(joints[0], joints[1], joints[2], joints[3]);

        // Read weights (float), normalize to uint8_t range
        float weights[4] = { 0.0f };
        cgltf_accessor_read_float(weightsAccessor, i, weights, 4);
        vertex.weights = pack_f4_ubyte4n(weights[0], weights[1], weights[2], weights[3]);

        interleaved.insert(i, vertex);  // Assign the constructed vertex
    }

    return interleaved;
}
*/

static pk_node* load_scene_nodes(cgltf_data* data, size_t* node_count) {
    pk_node* nodes = (pk_node*)pk_malloc(sizeof(pk_node) * data->nodes_count);
    pk_assert(nodes);
    *node_count = data->nodes_count;

    for (size_t i = 0; i < data->nodes_count; i++) {
        const cgltf_node* gl_node = &data->nodes[i];

        // Set node properties
        const char* nodeName = gl_node->name ? gl_node->name : "UNNAMED";
        strncpy(nodes[i].name, nodeName, PK_MAX_NAME_LEN - 1);
        nodes[i].name[sizeof(nodes[i].name) - 1] = '\0';

        if (gl_node->has_translation) {
            nodes[i].position = HMM_V3(gl_node->translation[0], gl_node->translation[1], gl_node->translation[2]);
        }
        else {
            nodes[i].position = HMM_V3(0.f, 0.f, 0.f);
        }

        if (gl_node->has_scale) {
            nodes[i].scale = HMM_V3(gl_node->scale[0], gl_node->scale[1], gl_node->scale[2]);
        }
        else {
            nodes[i].scale = HMM_V3(1.0f, 1.0f, 1.0f);
        }

        if (gl_node->has_rotation) {
            nodes[i].rotation = HMM_Q(
                gl_node->rotation[0],
                gl_node->rotation[1],
                gl_node->rotation[2],
                gl_node->rotation[3]
            );
        }
        else {
            nodes[i].rotation = HMM_Q(0.f, 0.f, 0.f, 1.0f);
        }
    }
    return nodes;
}

static void organize_nodes(cgltf_data* data, pk_node* nodes) {
    for (size_t i = 0; i < data->nodes_count; ++i) {
        const cgltf_node* gl_node = &data->nodes[i];
        pk_node* current_node = &nodes[i];

        // Assign parent if it exists
        if (gl_node->parent) {
            for (size_t j = 0; j < data->nodes_count; ++j) {
                if (&data->nodes[j] == gl_node->parent) {
                    current_node->parent = &nodes[j];
                    break;
                }
            }
        }
        else {
            current_node->parent = NULL; //root node
        }
    }
}

static pk_primitive create_primitive(
    pk_vertex_pnt* vertices, size_t vertex_count,
    uint32_t* indices, size_t index_count) {

    pk_primitive prim = {0};
    pk_primitive_desc desc = {0};
    desc.is_mutable = false;
    desc.num_elements = (int)index_count;
    desc.vertices = (sg_range){ vertices, vertex_count * sizeof(pk_vertex_pnt) };
    desc.indices = (sg_range){ indices, index_count * sizeof(uint32_t) };
    pk_init_primitive(&prim, &desc);
    return prim;
}

//--PUBLIC----------------------

bool pk_load_gltf(pk_model* model, cgltf_data* data) {
    pk_assert(model && data);
    size_t node_count;
    pk_node* nodes = load_scene_nodes(data, &node_count);
    organize_nodes(data, nodes);

    model->nodes = nodes;
    model->node_count = (uint16_t)node_count;

    pk_printf("Scene node info:");
    for (uint16_t i = 0; i < model->node_count; ++i) {
        pk_printf("Node: %s\n", model->nodes[i].name);
        if (model->nodes[i].parent) {
            pk_printf("Parent: %s\n", model->nodes[i].parent->name);
        }
    }

    pk_mesh* meshes = pk_malloc(data->meshes_count * sizeof(pk_mesh));
    pk_assert(meshes);
    size_t mesh_idx = 0;

    //process nodes and assign meshes/models in one loop
    for (size_t i = 0; i < data->nodes_count; ++i) {
        const cgltf_node* gl_node = &data->nodes[i];
        pk_node* current_node = &model->nodes[i];

        //process meshes attached to this node
        if (gl_node->mesh) {

            const cgltf_mesh* gl_mesh = gl_node->mesh;
            pk_primitive* primitives = pk_malloc(gl_node->mesh->primitives_count * sizeof(pk_primitive));
            pk_assert(primitives);
            //bool is_skinned = (gl_node->skin != NULL);

            for (size_t j = 0; j < gl_mesh->primitives_count; ++j) {
                const cgltf_primitive* primitive = &gl_mesh->primitives[j];

                size_t vertex_count;
                pk_vertex_pnt* vertices = interleave_attributes(primitive, &vertex_count);
                size_t index_count;
                uint32_t* indices = load_indices(primitive, &index_count);

                if (vertex_count > 0 && index_count > 0) {
                    primitives[j] = create_primitive(vertices, vertex_count, indices, index_count);
                    pk_free(vertices);
                    pk_free(indices);
                    /*
                    if (is_skinned) {
                        array_t<pk_vertex_skin> skin_verts = interleave_attributes_skin(primitive);
                        sg_buffer_desc skin = {};
                        skin.type = SG_BUFFERTYPE_VERTEXBUFFER;
                        skin.usage = SG_USAGE_IMMUTABLE;
                        skin.data = sg_range{ skin_verts.data, skin_verts.count * sizeof(pk_vertex_skin) };
                        prim.bindings.vertex_buffers[1] = sg_make_buffer(skin);
                    }
                    */
                }
                else {
                    pk_printf("No vertices or indices found for mesh '%s'\n", gl_mesh->name ? gl_mesh->name : "Unnamed");
                }

            }

            meshes[mesh_idx].node = current_node;
            meshes[mesh_idx].primitives = primitives;
            meshes[mesh_idx].primitive_count = (uint16_t)gl_mesh->primitives_count;
            mesh_idx++;
        }
    }

    model->meshes = meshes;
    model->mesh_count = (uint16_t)data->meshes_count;
    return true;
}

pk_node* pk_find_model_node(const pk_model* model, const char* name) {
    pk_assert(model);
    for (int i = 0; i < model->node_count; ++i) {
        pk_node* node = &model->nodes[i];
        if (strcmp(node->name, name) == 0) {
            return node;
        }
    }
    return NULL;
}

void pk_set_model_texture(pk_model* model, const pk_texture* tex, int slot) {
    pk_assert(model && tex);
    for (uint16_t i = 0; i < model->mesh_count; ++i) {
        pk_mesh* mesh = &model->meshes[i];
        for (uint16_t j = 0; j < mesh->primitive_count; j++) {
            pk_texture_primitive(&mesh->primitives[j], tex, slot);
        }
    }
}

void pk_draw_model(pk_model* model, pk_vs_params_t* vs_params) {
    pk_assert(model && vs_params);
    for (uint16_t i = 0; i < model->mesh_count; ++i) {
        pk_mesh* mesh = &model->meshes[i];
        vs_params->model = pk_node_transform(mesh->node);
        pk_draw_mesh(mesh, vs_params);
    }
}

void pk_release_model(pk_model* model) {
    pk_assert(model);
    for (uint16_t i = 0; i < model->mesh_count; ++i) {
        pk_mesh* mesh = &model->meshes[i];
        pk_release_mesh(mesh);
    }
    pk_free(model->meshes);
    pk_free(model->nodes);
}


//---------------------------------------------------------------------------------
//--GLTF_ANIM----------------------------------------------------------------------
//---------------------------------------------------------------------------------


static pk_gltf_anim_interp_type get_interpolation_type(cgltf_interpolation_type interpolation) {
    switch (interpolation) {
    case cgltf_interpolation_type_linear:
        return PK_ANIM_INTERP_LINEAR;
    case cgltf_interpolation_type_step:
        return PK_ANIM_INTERP_STEP;
    case cgltf_interpolation_type_cubic_spline:
        //force cubic spline to linear
        return PK_ANIM_INTERP_LINEAR;
    default:
        return PK_ANIM_INTERP_LINEAR;
    }
}

static void extract_animation_channel(
    cgltf_animation_channel* gltf_channel,
    pk_gltf_anim_channel* pk_channel,
    pk_model* model, cgltf_data* data) {

    pk_channel->target_node = pk_find_model_node(model, gltf_channel->target_node->name);
    if (!pk_channel->target_node)
        return;

    //Determine which property is animated.
    switch (gltf_channel->target_path) {
    case cgltf_animation_path_type_translation:
        pk_channel->path = PK_ANIM_PATH_TRANSLATION;
        break;
    case cgltf_animation_path_type_rotation:
        pk_channel->path = PK_ANIM_PATH_ROTATION;
        break;
    case cgltf_animation_path_type_scale:
        pk_channel->path = PK_ANIM_PATH_SCALE;
        break;
    default:
        return;
    }

    int num_keyframes = (int)gltf_channel->sampler->input->count;
    pk_channel->num_keyframes = num_keyframes;

    pk_channel->keyframes = (pk_gltf_keyframe*)pk_malloc(sizeof(pk_gltf_keyframe) * num_keyframes);
    pk_assert(pk_channel->keyframes);

    //Determine the number of components expected.
    //Even if the sampler is cubic spline, we force linear so we expect
    //3 for translation/scale and 4 for rotation.
    cgltf_size components = (pk_channel->path == PK_ANIM_PATH_ROTATION) ? 4 : 3;

    for (int i = 0; i < num_keyframes; ++i) {
        pk_channel->keyframes[i].time = 0.0f;

        cgltf_accessor_read_float(gltf_channel->sampler->input, i,
            &pk_channel->keyframes[i].time, 1);

        pk_channel->keyframes[i].value = (float*)pk_malloc(sizeof(float) * components);
        pk_assert(pk_channel->keyframes[i].value);
        cgltf_accessor_read_float(gltf_channel->sampler->output, i,
            pk_channel->keyframes[i].value, components);

    }

    pk_channel->interpolation = get_interpolation_type(gltf_channel->sampler->interpolation);
}

static void load_gltf_animations(cgltf_data* data, pk_gltf_anim* target, pk_model* model) {
    target->num_channels = 0;
    target->duration = 0;

    //Count total animation channels.
    for (int i = 0; i < data->animations_count; ++i) {
        cgltf_animation* anim = &data->animations[i];
        target->num_channels += (int)anim->channels_count;
    }

    target->channels = (pk_gltf_anim_channel*)pk_malloc(target->num_channels * sizeof(pk_gltf_anim_channel));
    pk_assert(target->channels);

    int channel_index = 0;
    for (int i = 0; i < data->animations_count; ++i) {
        cgltf_animation* anim = &data->animations[i];
        for (int j = 0; j < anim->channels_count; ++j) {
            cgltf_animation_channel* gltf_channel = &anim->channels[j];
            pk_gltf_anim_channel* pk_channel = &target->channels[channel_index++];
            extract_animation_channel(gltf_channel, pk_channel, model, data);
            //Update the overall duration.
            for (int k = 0; k < pk_channel->num_keyframes; ++k) {
                if (pk_channel->keyframes[k].time > target->duration) {
                    target->duration = pk_channel->keyframes[k].time;
                }
            }
        }
    }

    target->elapsed_time = 0.0f;
    target->loop = true;
    target->ready = true;
}

bool pk_load_gltf_anim(pk_gltf_anim* anim, pk_model* model, cgltf_data* data) {
    if (data->animations_count > 0) {
        load_gltf_animations(data, anim, model);
        return true;
    }
    else {
        pk_printf("No animations found in gltf file.\n");
        return false;
    }
}

/*
TODO: These are just here, because the initial version used an older version
of handmademath... Remove on occasion.
*/

static float interpolate(float a, float b, float t) {
    return a + t * (b - a);
}

static float quat_dot(HMM_Quat q1, HMM_Quat q2) {
    return (q1.X * q2.X) + (q1.Y * q2.Y) + (q1.Z * q2.Z) + (q1.W * q2.W);
}

static HMM_Quat lerp_quat(HMM_Quat q1, float t, HMM_Quat q2) {
    float x = HMM_Lerp(q1.X, t, q2.X);
    float y = HMM_Lerp(q1.Y, t, q2.Y);
    float z = HMM_Lerp(q1.Z, t, q2.Z);
    float w = HMM_Lerp(q1.W, t, q2.W);
    return HMM_NormQ(HMM_Q(x, y, z, w));
}

static void find_keyframes(float time, pk_gltf_anim_channel* channel, int* key1, int* key2, float* t) {
    int num_keyframes = channel->num_keyframes;
    for (int i = 0; i < num_keyframes - 1; ++i) {
        if (time >= channel->keyframes[i].time && time <= channel->keyframes[i + 1].time) {
            *key1 = i;
            *key2 = i + 1;
            float time1 = channel->keyframes[i].time;
            float time2 = channel->keyframes[i + 1].time;
            *t = (time - time1) / (time2 - time1);
            return;
        }
    }
    //Clamp to the last keyframe if time is beyond the range.
    *key1 = num_keyframes - 1;
    *key2 = num_keyframes - 1;
    *t = 0.0f;
}

static void interpolate_animation(pk_gltf_anim_channel* channel, float current_time) {
    pk_assert(channel);
    int key1 = 0, key2 = 0;
    float t = 0.f;
    find_keyframes(current_time, channel, &key1, &key2, &t);

    //For STEP interpolation, use the first keyframe's value.
    if (channel->interpolation == PK_ANIM_INTERP_STEP) {
        t = 0.0f;
    }

    pk_gltf_keyframe* kf1 = &channel->keyframes[key1];
    pk_gltf_keyframe* kf2 = &channel->keyframes[key2];

    if (channel->path == PK_ANIM_PATH_TRANSLATION) {
        float result[3] = { 0 };
        for (int i = 0; i < 3; ++i) {
            result[i] = interpolate(kf1->value[i], kf2->value[i], t);
        }
        channel->target_node->position = HMM_V3(result[0], result[1], result[2]);
    }
    else if (channel->path == PK_ANIM_PATH_ROTATION) {
        HMM_Quat rot1 = HMM_Q(kf1->value[0], kf1->value[1], kf1->value[2], kf1->value[3]);
        HMM_Quat rot2 = HMM_Q(kf2->value[0], kf2->value[1], kf2->value[2], kf2->value[3]);

        //ensure shortest path by flipping if necessary
        if (quat_dot(rot1, rot2) < 0.0f) {
            rot2.X = -rot2.X;
            rot2.Y = -rot2.Y;
            rot2.Z = -rot2.Z;
            rot2.W = -rot2.W;
        }

        if (quat_dot(rot1, rot2) > 0.9995f) {
            //Use linear interpolation for nearly identical quaternions.
            channel->target_node->rotation = lerp_quat(rot1, t, rot2);
        }
        else {
            channel->target_node->rotation = HMM_NormQ(HMM_SLerp(rot1, t, rot2));
        }
    }
    else if (channel->path == PK_ANIM_PATH_SCALE) {
        float result[3] = { 0 };
        for (int i = 0; i < 3; ++i) {
            result[i] = interpolate(kf1->value[i], kf2->value[i], t);
        }
        channel->target_node->scale = HMM_V3(result[0], result[1], result[2]);
    }
}

//--PUBLIC--------------------------------------------------

void pk_play_gltf_anim(pk_gltf_anim* animation, float dt) {
    pk_assert(animation);
    if (!animation->ready) return;
    animation->elapsed_time += dt;
    if (animation->loop) {
        animation->elapsed_time = fmodf(animation->elapsed_time, animation->duration);
    }
    else if (animation->elapsed_time > animation->duration) {
        animation->elapsed_time = animation->duration;
    }

    for (int i = 0; i < animation->num_channels; ++i) {
        pk_gltf_anim_channel* channel = &animation->channels[i];
        if (channel->target_node) {
            interpolate_animation(channel, animation->elapsed_time);
        }
    }
}

void pk_release_gltf_anim(pk_gltf_anim* anim) {
    pk_assert(anim);
    for (int i = 0; i < anim->num_channels; ++i) {
        pk_gltf_anim_channel* channel = &anim->channels[i];
        pk_assert(channel);
        for (int j = 0; j < channel->num_keyframes; ++j) {
            if (channel->keyframes[j].value != NULL) {
                pk_free(channel->keyframes[j].value);
            }
        }
        if (channel->keyframes != NULL) {
            pk_free(channel->keyframes);
        }
    }
    pk_free(anim->channels);
}


//---------------------------------------------------------------------------------
//--BONE_ANIM----------------------------------------------------------------------
//---------------------------------------------------------------------------------

/*
This is based around the implementation in raylib.
TODO: Make more efficient by not storing every keyframe
and interpolating the poses instead.
*/

static HMM_Mat4 HMM_TRS(HMM_Vec3 pos, HMM_Quat rotation, HMM_Vec3 scale) {
    HMM_Mat4 T = HMM_Translate(pos);
    HMM_Mat4 R = HMM_QToM4(rotation);
    HMM_Mat4 S = HMM_Scale(scale);
    return HMM_MulM4(HMM_MulM4(T, R), S);
}

static HMM_Vec3 HMM_RotateVec3(HMM_Vec3 v, HMM_Quat q) {
    //extract vector part of the quaternion
    HMM_Vec3 qv = HMM_V3(q.X, q.Y, q.Z);
    //compute cross product of qv and v
    HMM_Vec3 uv = HMM_Cross(qv, v);
    //compute cross product of qv and uv
    HMM_Vec3 uuv = HMM_Cross(qv, uv);
    //scale the first cross product by 2*w
    uv = HMM_MulV3F(uv, 2.0f * q.W);
    //scale the second cross product by 2
    uuv = HMM_MulV3F(uuv, 2.0f);
    //add components to the original vector
    return HMM_AddV3(v, HMM_AddV3(uv, uuv));
}

#define M3D_ANIMDELAY 17

pk_bone_anim* pk_load_bone_anims(m3d_t* m3d, int* count) {
    pk_assert(m3d);
    int i = 0, j = 0;
    *count = 0;

    pk_bone_anim* anims = NULL;
    anims = pk_malloc(m3d->numaction*sizeof(pk_bone_anim));
    pk_assert(anims);
    memset(anims, 0, m3d->numaction * sizeof(pk_bone_anim));
    *count = m3d->numaction;

    for (unsigned int a = 0; a < m3d->numaction; a++) {
        anims[a].frame_count = m3d->action[a].durationmsec/M3D_ANIMDELAY;
        anims[a].bone_count = m3d->numbone + 1;
        anims[a].bones = pk_malloc((m3d->numbone + 1)*sizeof(pk_bone));
        anims[a].poses = pk_malloc(anims[a].frame_count*sizeof(pk_transform*));

        for (i = 0; i < (int)m3d->numbone; i++) {
            anims[a].bones[i].parent = m3d->bone[i].parent;
            strncpy(anims[a].bones[i].name, m3d->bone[i].name, PK_MAX_NAME_LEN - 1);
            anims[a].bones[i].name[PK_MAX_NAME_LEN - 1] = '\0';
        }

        //A special, never transformed "no bone" bone, used for boneless vertices.
        anims[a].bones[i].parent = -1;
        strncpy(anims[a].bones[i].name, "NO BONE", PK_MAX_NAME_LEN - 1);
        anims[a].bones[i].name[PK_MAX_NAME_LEN - 1] = '\0';

        /*
        M3D stores frames at arbitrary intervals with sparse skeletons. We need full skeletons at
        //regular intervals, so let the M3D SDK do the heavy lifting and calculate interpolated bones
        //TODO: maybe just store at arbitary intervals and interpolate at runtime during pk_play_bone_anim(...),
        which should be faster.
        */
        for (i = 0; i < anims[a].frame_count; i++) {
            anims[a].poses[i] = pk_malloc((m3d->numbone + 1)*sizeof(pk_transform));

            m3db_t *pose = m3d_pose(m3d, a, i*M3D_ANIMDELAY);

            if (pose != NULL) {
                for (j = 0; j < (int)m3d->numbone; j++) {
                    anims[a].poses[i][j].pos.X = m3d->vertex[pose[j].pos].x*m3d->scale;
                    anims[a].poses[i][j].pos.Y = m3d->vertex[pose[j].pos].y*m3d->scale;
                    anims[a].poses[i][j].pos.Z = m3d->vertex[pose[j].pos].z*m3d->scale;
                    anims[a].poses[i][j].rot.X = m3d->vertex[pose[j].ori].x;
                    anims[a].poses[i][j].rot.Y = m3d->vertex[pose[j].ori].y;
                    anims[a].poses[i][j].rot.Z = m3d->vertex[pose[j].ori].z;
                    anims[a].poses[i][j].rot.W = m3d->vertex[pose[j].ori].w;
                    anims[a].poses[i][j].rot = HMM_NormQ(anims[a].poses[i][j].rot);
                    anims[a].poses[i][j].scale.X = anims[a].poses[i][j].scale.Y = anims[a].poses[i][j].scale.Z = 1.0f;

                    //Child bones are stored in parent bone relative space, convert that into model space!
                    if (anims[a].bones[j].parent >= 0) {
                        anims[a].poses[i][j].rot = HMM_MulQ(anims[a].poses[i][anims[a].bones[j].parent].rot, anims[a].poses[i][j].rot);
                        anims[a].poses[i][j].pos = HMM_RotateVec3(anims[a].poses[i][j].pos, anims[a].poses[i][anims[a].bones[j].parent].rot);
                        anims[a].poses[i][j].pos = HMM_AddV3(anims[a].poses[i][j].pos, anims[a].poses[i][anims[a].bones[j].parent].pos);
                        anims[a].poses[i][j].scale = HMM_MulV3(anims[a].poses[i][j].scale, anims[a].poses[i][anims[a].bones[j].parent].scale);
                    }
                }

                //Default transform for the "no bone" bone
                anims[a].poses[i][j].pos.X = 0.0f;
                anims[a].poses[i][j].pos.Y = 0.0f;
                anims[a].poses[i][j].pos.Z = 0.0f;
                anims[a].poses[i][j].rot.X = 0.0f;
                anims[a].poses[i][j].rot.Y = 0.0f;
                anims[a].poses[i][j].rot.Z = 0.0f;
                anims[a].poses[i][j].rot.W = 1.0f;
                anims[a].poses[i][j].scale = HMM_V3(1.f, 1.f, 1.f);
                pk_free(pose);
            }
        }
    }
    return anims;
}

bool pk_load_skeleton(pk_skeleton* skel, m3d_t* m3d) {
    pk_assert(skel && m3d);

    if (m3d->numbone) {
        skel->bone_count = m3d->numbone + 1;

        skel->bones = pk_malloc(skel->bone_count * sizeof(pk_bone));
        pk_assert(skel->bones);
        memset(skel->bones, 0, sizeof(pk_bone) * skel->bone_count);

        skel->bind_poses = pk_malloc(skel->bone_count * sizeof(pk_transform));
        pk_assert(skel->bind_poses);
        memset(skel->bind_poses, 0, sizeof(pk_transform) * skel->bone_count);

        int i = 0;
        for (i = 0; i < (int)m3d->numbone; i++) {
            skel->bones[i].parent = m3d->bone[i].parent;
            strncpy(skel->bones[i].name, m3d->bone[i].name, PK_MAX_NAME_LEN - 1);
            skel->bones[i].name[PK_MAX_NAME_LEN - 1] = '\0';

            skel->bind_poses[i].pos.X = m3d->vertex[m3d->bone[i].pos].x*m3d->scale;
            skel->bind_poses[i].pos.Y = m3d->vertex[m3d->bone[i].pos].y*m3d->scale;
            skel->bind_poses[i].pos.Z = m3d->vertex[m3d->bone[i].pos].z*m3d->scale;
            skel->bind_poses[i].rot.X = m3d->vertex[m3d->bone[i].ori].x;
            skel->bind_poses[i].rot.Y = m3d->vertex[m3d->bone[i].ori].y;
            skel->bind_poses[i].rot.Z = m3d->vertex[m3d->bone[i].ori].z;
            skel->bind_poses[i].rot.W = m3d->vertex[m3d->bone[i].ori].w;

            skel->bind_poses[i].rot = HMM_NormQ(skel->bind_poses[i].rot);
            skel->bind_poses[i].scale.X = skel->bind_poses[i].scale.Y = skel->bind_poses[i].scale.Z = 1.0f;

            //Child bones are stored in parent bone relative space, convert that into model space..
            if (skel->bones[i].parent >= 0) {
                skel->bind_poses[i].rot = HMM_MulQ(skel->bind_poses[skel->bones[i].parent].rot, skel->bind_poses[i].rot);
                skel->bind_poses[i].pos = HMM_RotateVec3(skel->bind_poses[i].pos, skel->bind_poses[skel->bones[i].parent].rot);
                skel->bind_poses[i].pos = HMM_AddV3(skel->bind_poses[i].pos, skel->bind_poses[skel->bones[i].parent].pos);
                skel->bind_poses[i].scale = HMM_MulV3(skel->bind_poses[i].scale, skel->bind_poses[skel->bones[i].parent].scale);
            }
        }

        //Add a "no bone" bone.
        skel->bones[i].parent = -1;
        strncpy(skel->bones[i].name, "NO BONE", PK_MAX_NAME_LEN - 1);
        skel->bones[i].name[PK_MAX_NAME_LEN - 1] = '\0';
        skel->bind_poses[i].pos = HMM_V3(0.f, 0.f, 0.f);
        skel->bind_poses[i].rot = HMM_Q(0.f, 0.f, 0.f, 1.0f);
        skel->bind_poses[i].scale = HMM_V3(1.f, 1.f, 1.f);

        return true;
    }
    return false;
}

void pk_release_skeleton(pk_skeleton* skel) {
    pk_assert(skel);
    if(skel->bind_poses) {
        pk_free(skel->bind_poses);
    }
    if(skel->bones) {
        pk_free(skel->bones);
    }
}

void pk_play_bone_anim(HMM_Mat4* trs, pk_skeleton* skeleton, pk_bone_anim* anim, float dt){
    if ((anim->frame_count > 0) && (anim->bones != NULL) && (anim->poses != NULL)) {

        anim->time += dt * 1000.0f;  //to milliseconds

        while (anim->time >= M3D_ANIMDELAY) {
            anim->time -= M3D_ANIMDELAY;  //Subtract frame delay to keep timing "accurate".
            anim->frame = (anim->frame + 1) % anim->frame_count;
        }

        for (int id = 0; id < anim->bone_count; id++) {
            HMM_Vec3 in_pos = skeleton->bind_poses[id].pos;
            HMM_Quat in_rot = skeleton->bind_poses[id].rot;
            HMM_Vec3 in_scale = skeleton->bind_poses[id].scale;

            HMM_Vec3 out_pos = anim->poses[anim->frame][id].pos;
            HMM_Quat out_rot = anim->poses[anim->frame][id].rot;
            HMM_Vec3 out_scale = anim->poses[anim->frame][id].scale;

            HMM_Quat inv_rot = HMM_InvQ(in_rot);
            HMM_Vec3 inv_pos = HMM_RotateVec3(HMM_V3(-in_pos.X, -in_pos.Y, -in_pos.Z), inv_rot);
            HMM_Vec3 inv_scale = HMM_DivV3(HMM_V3(1,1,1), in_scale);

            HMM_Vec3 bone_pos = HMM_AddV3(HMM_RotateVec3(HMM_MulV3(out_scale, inv_pos), out_rot), out_pos);
            HMM_Quat bone_rot = HMM_MulQ(out_rot, inv_rot);
            HMM_Vec3 bone_scale = HMM_MulV3(out_scale, inv_scale);

            HMM_Mat4 bone_mat = HMM_MulM4(HMM_MulM4(
                HMM_Translate(bone_pos),
                HMM_QToM4(bone_rot)),
                HMM_Scale(bone_scale)
            );
            trs[id] = bone_mat;
        }
    }
}

void pk_release_bone_anim(pk_bone_anim* anim) {
    if (!anim) return;

    if (anim->poses) {
        for (int i = 0; i < anim->frame_count; ++i) {
            pk_free(anim->poses[i]);
        }
        pk_free(anim->poses);
    }

    if (anim->bones) {
        pk_free(anim->bones);
    }
}


//--------------------------------------------------------------------------
//--SOUND-------------------------------------------------------------------
//--------------------------------------------------------------------------

#ifndef PK_NO_AUDIO

void pk_play_sound(pk_sound* sound, const pk_sound_channel_desc* desc) {
    pk_assert(desc->buffer);
    sound->channel = (tm_channel){ 0 };
    sound->node = desc->node;
    // check if we need positional audio
    if (desc->node != NULL) {
        if (desc->loop) {
            tm_add_spatial_loop(
                desc->buffer, 0, 0.75f, 1.0f,
                (const float*)&desc->node->position,
                desc->range_min, desc->range_max,
                &sound->channel
            );
        }
        else tm_add_spatial(
            desc->buffer, 0, 0.75f, 1.0f,
            (const float*)&desc->node->position,
            desc->range_min, desc->range_max,
            &sound->channel
        );
    }
    else {
        if (desc->loop) {
            tm_add_loop(desc->buffer, 0, 0.75f, 1.0f, &sound->channel);
        }
        else tm_add(desc->buffer, 0, 0.75f, 1.0f, &sound->channel);
    }
}

void pk_update_sound(pk_sound* sound) {
    if (sound->node) {
        tm_channel_set_position(sound->channel, sound->node->position.Elements);
    }
}

void pk_stop_sound(pk_sound* sound) {
    pk_assert(sound);
    tm_channel_stop(sound->channel);
}

void pk_update_sound_listener(pk_sound_listener* li, HMM_Vec3 pos, float dt) {
    const float smoothing = 1.0f - expf(-dt * li->smoothing);
    li->position = HMM_LerpV3(li->position, smoothing, pos);
    tm_update_listener((const float*)li->position.Elements);
}

#endif //PK_NO_AUDIO

//-------------------==----=========-----------------------------------------------------
//-------------------||---||-------||----------------------------------------------------
//-------------------||---||-------||----------------------------------------------------
//-------------------||---||-------||----------------------------------------------------
//-------------------||---||-------||----------------------------------------------------
//-------------------==----=========-----------------------------------------------------


//--------------------------------------------------------------------------
//--IMAGE-LOADING-----------------------------------------------------------
//--------------------------------------------------------------------------

typedef struct {
    pk_image_loaded_callback loaded_cb;
    pk_fail_callback fail_cb;
} image_request_data;

// from https://github.com/phoboslab/qoi/blob/master/qoiconv.c
#define ENDS_WITH(str, end) (strcmp(str + strlen(str) - (sizeof(end)-1), end) == 0)

static void _img_fetch_callback(const sfetch_response_t* response) {
    image_request_data data = *(image_request_data*)response->user_data;

    pk_image_data img_data = { 0 };

    if (response->fetched) {
        if (ENDS_WITH(response->path, ".png")) {
            cp_image_t result = cp_load_png_mem(response->buffer.ptr, (int)response->buffer.size);
            img_data.pixels = (void*)result.pix;
            img_data.width = result.w;
            img_data.height = result.h;
            data.loaded_cb(&img_data);
        }
        else if (ENDS_WITH(response->path, ".qoi")) {
            qoi_desc qoi = { 0 };
            img_data.pixels = qoi_decode(response->buffer.ptr, (int)response->buffer.size, &qoi, 4);
            img_data.width = qoi.width;
            img_data.height = qoi.height;
            data.loaded_cb(&img_data);
        }
        else {
            pk_printf("Image format not supported: %s\n", response->path);
            if (data.fail_cb != NULL) {
                data.fail_cb(response);
            }
            else {
                img_data.pixels = (void*)_checker_pixels;
                img_data.width = img_data.height = 4;
                data.loaded_cb(&img_data);
            }
        }
    }
    else if (response->failed) {
        switch (response->error_code) {
        case SFETCH_ERROR_FILE_NOT_FOUND: pk_printf("Image file not found: %s\n", response->path); break;
        case SFETCH_ERROR_BUFFER_TOO_SMALL: pk_printf("Image buffer too small: %s\n", response->path); break;
        default: break;
        }

		if (data.fail_cb != NULL) {
			data.fail_cb(response);
		} else {
            //Note: We pk_malloc() the pixels here, because it is very likely, that the user
            //will attempt, to pk_free() them after use...not very elegant, I know.
            img_data.pixels = pk_malloc(16 * sizeof(uint32_t));
            pk_assert(img_data.pixels);
            memcpy(img_data.pixels, _checker_pixels, 16 * sizeof(uint32_t));
            img_data.width = img_data.height = 4;
            data.loaded_cb(&img_data);
        }
    }
}

sfetch_handle_t pk_load_image_data(const pk_image_request* req) {
	image_request_data data = {
        .loaded_cb = req->loaded_cb,
		.fail_cb = req->fail_cb,
	};

	return sfetch_send(&(sfetch_request_t) {
		.path = req->path,
		.callback = _img_fetch_callback,
		.buffer = req->buffer,
		.user_data = SFETCH_RANGE(data),
	});
}


//-----------------------------------------------------------------------
//--M3D-LOADING----------------------------------------------------------
//-----------------------------------------------------------------------


typedef struct {
    pk_m3d_loaded_callback loaded_cb;
    pk_fail_callback fail_cb;
} m3d_request_data;

void pk_release_m3d_data(m3d_t* data) {
    m3d_free(data);
}

static void _m3d_fetch_callback(const sfetch_response_t* response) {
    m3d_request_data data = *(m3d_request_data*)response->user_data;

    if (response->fetched) {
        m3d_t* m3d = m3d_load((unsigned char*)response->buffer.ptr, NULL, NULL, NULL);
        if (m3d != NULL && data.loaded_cb != NULL) {
			data.loaded_cb(m3d);
        }
        if (!m3d) {
            data.fail_cb(response);
        }
    }
    else if (response->failed) {
        switch (response->error_code) {
        case SFETCH_ERROR_FILE_NOT_FOUND: pk_printf("M3d file not found: %s", response->path); break;
        case SFETCH_ERROR_BUFFER_TOO_SMALL: pk_printf("M3d buffer too small: %s", response->path); break;
        default: break;
        }
        if (data.fail_cb != NULL) {
            data.fail_cb(response);
        }
    }
}

sfetch_handle_t pk_load_m3d_data(const pk_m3d_request* req) {
    m3d_request_data data = {
        .loaded_cb = req->loaded_cb,
        .fail_cb = req->fail_cb,
    };

    return sfetch_send(&(sfetch_request_t) {
        .path = req->path,
		.callback = _m3d_fetch_callback,
		.buffer = req->buffer,
		.user_data = SFETCH_RANGE(data),
    });
}


//-------------------------------------------------------------------------
//--GLTF_LOADING-----------------------------------------------------------
//-------------------------------------------------------------------------


typedef struct {
    pk_gltf_loaded_callback loaded_cb;
    pk_fail_callback fail_cb;
} gltf_request_data;

static void _gltf_fetch_callback(const sfetch_response_t* response) {
    gltf_request_data data = *(gltf_request_data*)response->user_data;

    if (response->fetched) {
		cgltf_options options = {0};
		cgltf_data* gltf = NULL;
        cgltf_result result = cgltf_parse(
            &options,
            response->buffer.ptr,
            response->buffer.size,
            &gltf
		);

		if (result != cgltf_result_success) {
			pk_printf("Failed to load glTF file: %s", response->path);
			return;
		}

		result = cgltf_load_buffers(&options, gltf, response->path);
		if (result != cgltf_result_success) {
			pk_printf("Failed to load glTF buffers: %s", response->path);
			cgltf_free(gltf);
			return;
		}

        if (gltf != NULL && data.loaded_cb != NULL) {
            data.loaded_cb(gltf);
        }

    }
    else if (response->failed) {
        switch (response->error_code) {
        case SFETCH_ERROR_FILE_NOT_FOUND: pk_printf("Gltf file not found: %s\n", response->path); break;
        case SFETCH_ERROR_BUFFER_TOO_SMALL: pk_printf("Gltf buffer too small: %s\n", response->path); break;
        default: break;
        }
        if (data.fail_cb != NULL) {
            data.fail_cb(response);
        }
    }
}

sfetch_handle_t pk_load_gltf_data(const pk_gltf_request* req) {
    gltf_request_data data = {0};
    data.loaded_cb = req->loaded_cb;
    data.fail_cb = req->fail_cb;

    sfetch_request_t r = {0};
    r.path = req->path;
    r.callback = _gltf_fetch_callback;
    r.buffer = req->buffer;
    r.user_data = SFETCH_RANGE(data);
    return sfetch_send(&r);
}

void pk_release_gltf_data(cgltf_data* data) {
    cgltf_free(data);
}


//-----------------------------------------------------------------------
//--SOUNDS---------------------------------------------------------------
//-----------------------------------------------------------------------

#ifndef PK_NO_AUDIO

typedef struct {
    pk_sound_buffer_loaded_callback loaded_cb;
    pk_fail_callback fail_cb;
} sound_request_data;

static void _sound_fetch_callback(const sfetch_response_t* response) {
    sound_request_data data = *(sound_request_data*)response->user_data;

    if (response->fetched) {
        const tm_buffer* buffer = NULL;
        tm_create_buffer_vorbis_stream(
            response->buffer.ptr,
            (int)response->buffer.size,
            NULL, NULL,
            &buffer
        );
        if (data.loaded_cb) {
            data.loaded_cb(buffer);
        }
    }
    if (response->failed) {
        switch (response->error_code) {
        case SFETCH_ERROR_FILE_NOT_FOUND: pk_printf("Sound file not found: %s\n", response->path); break;
        case SFETCH_ERROR_BUFFER_TOO_SMALL: pk_printf("Sound buffer too small: %s\n", response->path); break;
        default: break;
        }
        if (data.fail_cb) {
            data.fail_cb(response);
        }
    }
}

sfetch_handle_t pk_load_sound_buffer(const pk_sound_buffer_request* req) {
    sound_request_data data = {
        .loaded_cb = req->loaded_cb,
        .fail_cb = req->fail_cb,
    };

    return sfetch_send(&(sfetch_request_t) {
        .path = req->path,
        .buffer = req->buffer,
        .callback = _sound_fetch_callback,
        .user_data = SFETCH_RANGE(data),
    });
}

#endif //PK_NO_AUDIO
