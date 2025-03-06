#include "io.h"
#include "m3d.h"
#include "primitive.h"
#include "node.h"
#include "common.h"
#include "log.h"

static uint32_t pack_u32(uint32_t x, uint32_t y, uint32_t z, uint32_t w) {
    return (uint32_t)(((uint32_t)w << 24) | ((uint32_t)z << 16) | ((uint32_t)y << 8) | x);
}

static uint32_t pack_f4_byte4n(float x, float y, float z, float w) {
    int8_t x8 = (int8_t)(x * 127.0f);
    int8_t y8 = (int8_t)(y * 127.0f);
    int8_t z8 = (int8_t)(z * 127.0f);
    int8_t w8 = (int8_t)(w * 127.0f);
    return pack_u32((uint8_t)x8, (uint8_t)y8, (uint8_t)z8, (uint8_t)w8);
}

static uint32_t pack_f4_ubyte4n(float x, float y, float z, float w) {
    uint8_t x8 = (uint8_t)(x * 255.0f);
    uint8_t y8 = (uint8_t)(y * 255.0f);
    uint8_t z8 = (uint8_t)(z * 255.0f);
    uint8_t w8 = (uint8_t)(w * 255.0f);
    return pack_u32(x8, y8, z8, w8);
}

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
        case SFETCH_ERROR_FILE_NOT_FOUND: log_debug("M3d file not found: %s", response->path); break;
        case SFETCH_ERROR_BUFFER_TOO_SMALL: log_debug("M3d buffer too small: %s", response->path); break;
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

bool pk_load_m3d(pk_primitive* prim, pk_node* node, m3d_t* m3d) {
    pk_assert(m3d && prim);
    uint32_t total_vertices = m3d->numface * 3;

    pk_vertex_pnt* pnt = pk_malloc(total_vertices * sizeof(pk_vertex_pnt));

    // If skin data exists, build a corresponding skin vertex array.
    bool has_skin = (m3d->numbone > 0 && m3d->numskin > 0);
    pk_vertex_skin* skin = NULL;
    if (has_skin) {
        skin = pk_malloc(total_vertices * sizeof(pk_vertex_skin));
    }

    // 4. Build an index array: we create vertices in order so indices are sequential.
    uint32_t* indices = pk_malloc(sizeof(uint32_t) * total_vertices);

    // 5. For each face, extract vertex attributes.
    for (uint32_t i = 0; i < m3d->numface; i++) {
        m3d_face_t* face = &m3d->face[i];
        for (int n = 0; n < 3; n++) {
            uint32_t vertIdx = face->vertex[n];
            uint32_t outIndex = i * 3 + n;

            pk_vertex_pnt vert = { 0 };
            m3d_vertex_t* m3dVert = &m3d->vertex[vertIdx];

            // Position: scale applied (m3d->scale)
            vert.pos[0] = m3dVert->x;
            vert.pos[1] = m3dVert->y;
            vert.pos[2] = m3dVert->z;

            // Normal: if provided via face normals, use them; otherwise default to (0,0,0)
            if (face->normal[0] != M3D_UNDEF) {
                // Using the normal index from the face. (Assuming face->normal[n] is valid.)
                m3d_vertex_t* normVert = &m3d->vertex[face->normal[n]];
                vert.norm[0] = normVert->x;
                vert.norm[1] = normVert->y;
                vert.norm[2] = normVert->z;
            }
            else {
                vert.norm[0] = vert.norm[1] = vert.norm[2] = 0.0f;
            }

            // UV coordinates: if available, using the texture mapping array (tmap)
            if (face->texcoord[0] != M3D_UNDEF) {
                // Raylib flips the V coordinate.
                uint32_t tcIdx = face->texcoord[n];
                if (tcIdx < m3d->numtmap) {
                    vert.uv[0] = m3d->tmap[tcIdx].u;
                    vert.uv[1] = 1.0f - m3d->tmap[tcIdx].v;
                }
                else {
                    vert.uv[0] = vert.uv[1] = 0.0f;
                }
            }
            else {
                vert.uv[0] = vert.uv[1] = 0.0f;
            }

            pnt[outIndex] = vert;
            indices[outIndex] = outIndex;

            // Process skin data (if available)
            /*
            if (has_skin) {
                pk_vertex_skin skv = { 0 };
                int skinid = m3dVert->skinid;
                if (skinid != M3D_UNDEF && (uint32_t)skinid < m3d->numskin) {
                    m3d_skin_t* m3dSkin = &m3d->skin[skinid];
                    M3D_INDEX* idx = m3dSkin->boneid;
                    skv.joints = pack_u32(idx[0], idx[1], idx[2], idx[3]);
                    M3D_FLOAT* wgt = m3dSkin->weight;
                    skv.weights = pack_f4_ubyte4n(wgt[0], wgt[1], wgt[2], wgt[3]);
                }
                else {
                    // No valid skin; assign a default "no bone" influence.
                    skv.joints = m3d->numbone; // Convention: last bone reserved as "no bone"
                    skv.weights = 1;
                }
                skin[outIndex] = skv;
            } 
            */
        }
    }

    // Create sokol-gfx buffers.
    sg_buffer_desc pnt_desc = { 0 };
    pnt_desc.type = SG_BUFFERTYPE_VERTEXBUFFER;
    pnt_desc.usage = SG_USAGE_IMMUTABLE;
    pnt_desc.data = (sg_range){ pnt, total_vertices * sizeof(pk_vertex_pnt) };

    /*
    // Skin vertex buffer (if available).
    sg_buffer_desc skin_desc = { 0 };
   //sg_buffer_desc ssbo_desc = { 0 };
    if (has_skin) {
        skin_desc.type = SG_BUFFERTYPE_VERTEXBUFFER;
        skin_desc.usage = SG_USAGE_IMMUTABLE;
        skin_desc.data = (sg_range){ skin, total_vertices * sizeof(pk_vertex_skin) };

        //ssbo_desc.type = SG_BUFFERTYPE_STORAGEBUFFER;
        //ssbo_desc.usage = SG_USAGE_STREAM;
        //ssbo_desc.size = m3d->numbone * sizeof(skinned_bone_t);
    }
    */

    // Index buffer.
    sg_buffer_desc idx_desc = { 0 };
    idx_desc.type = SG_BUFFERTYPE_INDEXBUFFER;
    idx_desc.usage = SG_USAGE_IMMUTABLE;
    idx_desc.data = (sg_range){ indices, total_vertices * sizeof(uint32_t) };

    // Create a pk_primitive and assign buffer bindings.

    //prim->bindings.index_buffer = sg_make_buffer(&idx_desc);
    sg_init_buffer(prim->bindings.index_buffer, &idx_desc);
    //prim->bindings.vertex_buffers[0] = sg_make_buffer(&pnt_desc);
    sg_init_buffer(prim->bindings.vertex_buffers[0], &pnt_desc);

    /*
    if (has_skin) {
        prim->bindings.vertex_buffers[1] = sg_make_buffer(&skin_desc);
        //prim->bindings.storage_buffers[0] = sg_make_buffer(&ssbo_desc);
    }

    if (node) {
        node->scale.X = m3d->scale;
        node->scale.Y = m3d->scale;
        node->scale.Z = m3d->scale;
    }
    */

    /*
    M3D_INDEX mat_count = m3d->nummaterial;
    if (mat_count == 0) {
        log_debug("No materials found on model %s", m3d->name);
    }
    */

    prim->base_element = 0;
    prim->num_elements = total_vertices;

    pk_free(pnt);
    pk_free(skin);
    pk_free(indices);
    log_debug("Loaded pk_primitive %s", m3d->name);
    return true;
}

