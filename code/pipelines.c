#include "pipelines.h"
#include "primitive.h"
#include "hmm.h"
#include "unlit.glsl.h"
#include "phong.glsl.h"

#define PIP_DEF(val, def) (val == 0 ? def : val)

sg_pipeline pk_unlit_tex_pip(const pk_pip_desc* desc) {
    static sg_pipeline_desc pd = { 0 };
    pd.shader = sg_make_shader(pk_unlit_texture_shader_desc(sg_query_backend()));
    pd.index_type = SG_INDEXTYPE_UINT32;
    pd.primitive_type = SG_PRIMITIVETYPE_TRIANGLES;
    pd.layout.buffers[0].stride = sizeof(pk_vertex_pnt);
    pd.layout.attrs[ATTR_pk_unlit_texture_position].format = SG_VERTEXFORMAT_FLOAT3;
    pd.layout.attrs[ATTR_pk_unlit_texture_normal].format = SG_VERTEXFORMAT_FLOAT3;
    pd.layout.attrs[ATTR_pk_unlit_texture_uv].format = SG_VERTEXFORMAT_FLOAT2;
    pd.depth = desc->depth_state;
    pd.sample_count = desc->sample_count;
    // MIGHT OTHERWISE DEFAULT TO RGBA8 (?)
    if (desc->pixel_format != _SG_PIXELFORMAT_DEFAULT) {
        pd.colors[0].pixel_format = desc->pixel_format;
    }
    pd.cull_mode = SG_CULLMODE_BACK;
    pd.face_winding = SG_FACEWINDING_CCW;
    pd.label = "unlit_tex_pipeline";
    return sg_make_pipeline(&pd);
}

sg_pipeline pk_color_phong_pip(const pk_pip_desc* desc) {
    static sg_pipeline_desc pd = { 0 };
    pd.shader = sg_make_shader(pk_phong_color_shader_desc(sg_query_backend()));
    pd.index_type = SG_INDEXTYPE_UINT32;
    pd.primitive_type = SG_PRIMITIVETYPE_TRIANGLES;
    pd.layout.buffers[0].stride = sizeof(pk_vertex_pnt);
    pd.layout.attrs[ATTR_pk_phong_color_position].format = SG_VERTEXFORMAT_FLOAT3;
    pd.layout.attrs[ATTR_pk_phong_color_normal].format = SG_VERTEXFORMAT_FLOAT3;
    pd.depth = desc->depth_state;
    pd.sample_count = desc->sample_count;
    if (desc->pixel_format != _SG_PIXELFORMAT_DEFAULT) {
        pd.colors[0].pixel_format = desc->pixel_format;
    }
    pd.cull_mode = SG_CULLMODE_BACK;
    pd.face_winding = SG_FACEWINDING_CCW;
    pd.label = "pk_color_phong_pipeline";
    return sg_make_pipeline(&pd);
}

sg_pipeline pk_texture_phong_pip(const pk_pip_desc* desc) {
    static sg_pipeline_desc pd = { 0 };
    pd.shader = sg_make_shader(pk_phong_texture_shader_desc(sg_query_backend()));
    pd.index_type = SG_INDEXTYPE_UINT32;
    pd.primitive_type = SG_PRIMITIVETYPE_TRIANGLES;
    pd.layout.buffers[0].stride = sizeof(pk_vertex_pnt);
    pd.layout.attrs[ATTR_pk_phong_texture_position].format = SG_VERTEXFORMAT_FLOAT3;
    pd.layout.attrs[ATTR_pk_phong_texture_normal].format = SG_VERTEXFORMAT_FLOAT3;
    pd.layout.attrs[ATTR_pk_phong_texture_uv].format = SG_VERTEXFORMAT_FLOAT2;
    pd.depth = desc->depth_state;
    pd.sample_count = desc->sample_count;
    if (desc->pixel_format != _SG_PIXELFORMAT_DEFAULT) {
        pd.colors[0].pixel_format = desc->pixel_format;
    }
    pd.cull_mode = SG_CULLMODE_BACK;
    pd.face_winding = SG_FACEWINDING_CCW;
    pd.label = "pk_texture_phong_pipeline";
    return sg_make_pipeline(&pd);
}
