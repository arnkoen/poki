#include "io.h"
#include "cgltf.h"
#include "array.h"
#include "primitive.h"
#include "mesh.h"
#include "model.h"
#include "gltf_anim.h"
#include "common.h"
#include "node.h"
#include "log.h"


static array_t<uint32_t> load_indices(const cgltf_primitive& prim) {
    array_t<uint32_t> indices = {};
    if (!prim.indices) {
        fprintf(stderr, "No indices found in model!\n");
        return indices;
    }

    const cgltf_accessor* indexAccessor = prim.indices;
    size_t indexCount = indexAccessor->count;

    indices.resize(indexCount);

    for (size_t i = 0; i < indexCount; ++i) {
        uint32_t index = 0;
        cgltf_accessor_read_uint(indexAccessor, i, &index, 1);
        //indices[i] = index;  // Direct assignment, no push_back
        indices.insert(i, index);
    }

    return indices;
}

static array_t<pk_vertex_pnt> interleave_attributes(const cgltf_primitive& primitive) {
    array_t<pk_vertex_pnt> interleaved = {};

    const cgltf_accessor* positionAccessor = nullptr;
    const cgltf_accessor* normalAccessor = nullptr;
    const cgltf_accessor* uvAccessor = nullptr;

    // Find the accessors for positions, normals, and uvs
    for (size_t i = 0; i < primitive.attributes_count; ++i) {
        const cgltf_attribute& attribute = primitive.attributes[i];

        switch (attribute.type) {
        case cgltf_attribute_type_position:
            positionAccessor = attribute.data;
            break;
        case cgltf_attribute_type_normal:
            normalAccessor = attribute.data;
            break;
        case cgltf_attribute_type_texcoord:
            uvAccessor = attribute.data;
            break;
        default:
            break;
        }
    }

    if (!positionAccessor) {
        fprintf(stderr, "No position attribute found in primitive.");
        return interleaved;
    }

    size_t vertexCount = positionAccessor->count;
    interleaved.resize(vertexCount);

    for (size_t i = 0; i < vertexCount; ++i) {
        pk_vertex_pnt vertex;

        // Get position
        if (positionAccessor) {
            cgltf_accessor_read_float(positionAccessor, i, vertex.pos, 3);
        }
        // Get normal
        if (normalAccessor) {
            cgltf_accessor_read_float(normalAccessor, i, vertex.norm, 3);
        }
        // Get uv
        if (uvAccessor) {
            cgltf_accessor_read_float(uvAccessor, i, vertex.uv, 2);
        }

        //interleaved.add(vertex);
        interleaved.insert(i, vertex);  // Assign the constructed vertex
    }

    return interleaved;
}

/*
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

    size_t vertexCount = weightsAccessor->count;
    interleaved.resize(vertexCount);

    for (size_t i = 0; i < vertexCount; ++i) {
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

static array_t<pk_node> load_scene_nodes(cgltf_data* data) {
    array_t<pk_node> nodes = {};
    for (size_t i = 0; i < data->nodes_count; i++) {
        const cgltf_node& cgltfNode = data->nodes[i];
        pk_node node = {};
        pk_init_node(&node);

        // Set node properties
        const char* nodeName = cgltfNode.name ? cgltfNode.name : "UNNAMED";
        node.name = strdup(nodeName);

        if (cgltfNode.has_translation) {
            node.position = HMM_V3(cgltfNode.translation[0], cgltfNode.translation[1], cgltfNode.translation[2]);
        }
        if (cgltfNode.has_scale) {
            node.scale = HMM_V3(cgltfNode.scale[0], cgltfNode.scale[1], cgltfNode.scale[2]);
        }
        if (cgltfNode.has_rotation) {
            node.rotation = HMM_Q(cgltfNode.rotation[0], cgltfNode.rotation[1], cgltfNode.rotation[2], cgltfNode.rotation[3]);
        }

        nodes.add(node);
    }
    return nodes;
}

static void organize_nodes(cgltf_data* data, array_t<pk_node>& nodes) {
    for (size_t i = 0; i < data->nodes_count; ++i) {
        const cgltf_node& cgltfNode = data->nodes[i];
        pk_node* currentNode = &nodes[i];

        // Assign parent if it exists
        if (cgltfNode.parent) {
            for (size_t j = 0; j < data->nodes_count; ++j) {
                if (&data->nodes[j] == cgltfNode.parent) {
                    currentNode->parent = &nodes[j];
                    break;
                }
            }
        }
        else {
            currentNode->parent = NULL; // Root node
        }
    }
}

static pk_primitive create_primitive(array_t<pk_vertex_pnt>& vertices, array_t<uint32_t>& indices) {
    pk_primitive prim = {};
    pk_primitive_desc desc = {};
    desc.usage = SG_USAGE_IMMUTABLE;
    desc.num_elements = indices.count;
    desc.indices = sg_range{ indices.data, indices.count * sizeof(uint32_t) };
    desc.vertices = sg_range{ vertices.data, vertices.count * sizeof(pk_vertex_pnt) };
    pk_init_primitive(&prim, &desc);
    return prim;
}

typedef struct {
    pk_gltf_loaded_callback loaded_cb;
    pk_fail_callback fail_cb;
} gltf_request_data;


static void _gltf_fetch_callback(const sfetch_response_t* response) {
    gltf_request_data data = *(gltf_request_data*)response->user_data;

    if (response->fetched) {
		cgltf_options options = {};
		cgltf_data* gltf = NULL;
        cgltf_result result = cgltf_parse(
            &options,
            response->buffer.ptr,
            response->buffer.size,
            &gltf
		);

		if (result != cgltf_result_success) {
			log_error("Failed to load glTF file: %s", response->path);
			return;
		}

		result = cgltf_load_buffers(&options, gltf, response->path);
		if (result != cgltf_result_success) {
			log_error("Failed to load glTF buffers: %s", response->path);
			cgltf_free(gltf);
			return;
		}

        if (gltf != NULL && data.loaded_cb != NULL) {
            data.loaded_cb(gltf);
        }

    }
    else if (response->failed) {
        switch (response->error_code) {
        case SFETCH_ERROR_FILE_NOT_FOUND: log_debug("Gltf file not found: %s", response->path); break;
        case SFETCH_ERROR_BUFFER_TOO_SMALL: log_debug("Gltf buffer too small: %s", response->path); break;
        default: break;
        }
        if (data.fail_cb != NULL) {
            data.fail_cb(response);
        }
    }
}

extern "C" {

sfetch_handle_t pk_load_gltf_data(const pk_gltf_request* req) {
    gltf_request_data data = {};
    data.loaded_cb = req->loaded_cb;
    data.fail_cb = req->fail_cb;

    sfetch_request_t r = {};
    r.path = req->path;
    r.callback = _gltf_fetch_callback;
    r.buffer = req->buffer;
    r.user_data = SFETCH_RANGE(data);
    return sfetch_send(r);
}


void pk_release_gltf_data(cgltf_data* data) {
    cgltf_free(data);
}


bool pk_load_gltf(pk_model* model, cgltf_data* data) {
    array_t<pk_node> nodes = load_scene_nodes(data);
    organize_nodes(data, nodes);

    model->nodes = nodes.data;
    model->node_count = nodes.count;

    log_debug("Scene node info:");
    for (uint16_t i = 0; i < model->node_count; ++i) {
        log_debug("Node: %s", model->nodes[i].name);
        if (model->nodes[i].parent) {
            log_debug("Parent: %s", model->nodes[i].parent->name);
        }
    }

    array_t<pk_mesh> meshes = {};
    // Process nodes and assign meshes/models in one loop
    for (size_t i = 0; i < data->nodes_count; ++i) {
        const cgltf_node& cgltfNode = data->nodes[i];
        pk_node* current_node = &model->nodes[i];

        // Process meshes attached to this node
        if (cgltfNode.mesh) {
            const cgltf_mesh& cgltfMesh = *cgltfNode.mesh;
            array_t<pk_primitive> primitives = {};

            bool isSkinned = (cgltfNode.skin != nullptr);

            for (size_t j = 0; j < cgltfMesh.primitives_count; ++j) {
                const cgltf_primitive& primitive = cgltfMesh.primitives[j];

                array_t<pk_vertex_pnt> vertices = interleave_attributes(primitive);
                array_t<uint32_t> indices = load_indices(primitive);

                if (vertices.count > 0 && indices.count > 0) {
                    pk_primitive prim = create_primitive(vertices, indices);
                    primitives.add(prim);
                    vertices.free();
                    indices.free();
                    /*
                    if (isSkinned) {
                        array_t<pk_vertex_skin> skinVertices = interleave_attributes_skin(primitive);
                        sg_buffer_desc skin = {};
                        skin.type = SG_BUFFERTYPE_VERTEXBUFFER;
                        skin.usage = SG_USAGE_IMMUTABLE;
                        skin.data = sg_range{ skinVertices.data, skinVertices.count * sizeof(pk_vertex_skin) };
                        prim.bindings.vertex_buffers[1] = sg_make_buffer(skin);
                        skinVertices.free();
                    }
                    */
                }
                else {
                    log_error("No vertices or indices found for mesh '%s'", cgltfMesh.name ? cgltfMesh.name : "Unnamed");
                }

            }

            pk_mesh mesh = {};
            mesh.node = current_node;
            mesh.primitives = primitives.data;
            mesh.primitive_count = primitives.count;
            meshes.add(mesh);
        }
    }

    model->meshes = meshes.data;
    model->mesh_count = meshes.count;
    return true;
}

} // extern "C"


//--ANIMATIONS--------------------------------------------------------------

static pk_gltf_anim_interp_type get_interpolation_type(cgltf_interpolation_type interpolation) {
    switch (interpolation) {
    case cgltf_interpolation_type_linear:
        return PK_ANIM_INTERP_LINEAR;
    case cgltf_interpolation_type_step:
        return PK_ANIM_INTERP_STEP;
    case cgltf_interpolation_type_cubic_spline:
        // Force cubic spline to linear.
        return PK_ANIM_INTERP_LINEAR;
    default:
        return PK_ANIM_INTERP_LINEAR;
    }
}

static void extract_animation_channel(cgltf_animation_channel* gltf_channel,
    pk_gltf_anim_channel* pk_channel,
    pk_model* model, cgltf_data* data) {
    pk_channel->target_node = pk_find_model_node(model, gltf_channel->target_node->name);
    if (!pk_channel->target_node)
        return;

    // Determine which property is animated.
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

    // Allocate keyframe array.
    pk_channel->keyframes = (pk_gltf_keyframe*)pk_malloc(sizeof(pk_gltf_keyframe) * num_keyframes);
    pk_assert(pk_channel->keyframes);

    // Determine the number of components expected.
    // Even if the sampler is cubic spline, we force linear so we expect:
    // 3 for translation/scale and 4 for rotation.
    cgltf_size components = (pk_channel->path == PK_ANIM_PATH_ROTATION) ? 4 : 3;

    for (int i = 0; i < num_keyframes; ++i) {
        pk_channel->keyframes[i].time = 0.0f;
        cgltf_accessor_read_float(gltf_channel->sampler->input, i,
            &pk_channel->keyframes[i].time, 1);
        pk_channel->keyframes[i].value = (float*)malloc(sizeof(float) * components);
        pk_assert(pk_channel->keyframes[i].value);
        cgltf_accessor_read_float(gltf_channel->sampler->output, i,
            pk_channel->keyframes[i].value, components);
    }

    pk_channel->interpolation = get_interpolation_type(gltf_channel->sampler->interpolation);
}

static void load_gltf_animations(cgltf_data* data, pk_gltf_anim* target, pk_model* model) {
    target->num_channels = 0;
    target->duration = 0;

    // Count total animation channels.
    for (int i = 0; i < data->animations_count; ++i) {
        cgltf_animation* anim = &data->animations[i];
        target->num_channels += anim->channels_count;
    }

    target->channels = (pk_gltf_anim_channel*)malloc(target->num_channels * sizeof(pk_gltf_anim_channel));
    pk_assert(target->channels);

    int channel_index = 0;
    for (int i = 0; i < data->animations_count; ++i) {
        cgltf_animation* anim = &data->animations[i];
        for (int j = 0; j < anim->channels_count; ++j) {
            cgltf_animation_channel* gltf_channel = &anim->channels[j];
            pk_gltf_anim_channel* pk_channel = &target->channels[channel_index++];
            extract_animation_channel(gltf_channel, pk_channel, model, data);
            // Update the overall duration.
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

extern "C" {

    bool pk_load_gltf_anim(pk_gltf_anim* anim, pk_model* model, cgltf_data* data) {
        if (data->animations_count > 0) {
            load_gltf_animations(data, anim, model);
            return true;
        }
        else {
            log_error("No animations found in glTF file.");
            return false;
        }
    }

} // extern "C"
