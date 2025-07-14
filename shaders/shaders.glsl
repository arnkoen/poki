@module pk
@ctype vec3 HMM_Vec3
@ctype mat4 HMM_Mat4
@ctype vec4 sg_color

@include common.inc.glsl

//--SKINNED-------------------------------------------------------------------
@vs skinned_vs

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 nrm;
layout(location = 2) in vec2 uv;
layout(location = 3) in uvec4 bone_indices;
layout(location = 4) in vec4 weights;

@include_block vs_uniforms //binding=0

#define MAX_BONES 32
layout(binding=1) uniform bone_matrices {
    mat4 bones[MAX_BONES];
};

out vec3 v_pos;
out vec3 v_normal;
out vec2 v_uv;
out vec3 v_viewpos;

void main() {
    uvec4 idx = bone_indices;
    mat4 skin_mat = weights.x * bones[idx.x] +
                    weights.y * bones[idx.y] +
                    weights.z * bones[idx.z] +
                    weights.w * bones[idx.w];

    vec4 skinned_pos = skin_mat * vec4(pos, 1.0);
    vec3 skinned_nrm = mat3(skin_mat) * nrm;

    gl_Position = proj * view * model * skinned_pos;
    v_pos = skinned_pos.xyz;
    v_normal = normalize(skinned_nrm);
    v_uv = uv;
    v_viewpos = viewpos;
}
@end

//--UNLIT---------------------------------------------------------------------

//COLOUR

@vs unlit_col_vs
layout(location=0) in vec3 position;

@include_block vs_uniforms

void main() {
    gl_Position = proj * view * model * vec4(position, 1.0);
}
@end

@fs unlit_col_fs

layout(binding=1) uniform color {
    vec4 col;
};

out vec4 frag_color;

void main() {
    frag_color = col;
}
@end

@program unlit_color unlit_col_vs unlit_col_fs


//TEXTURED

@vs unlit_tex_vs
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 uv;

out vec2 v_uv;

@include_block vs_uniforms

void main() {
    gl_Position = proj * view * model * vec4(position, 1.0);
    v_uv = uv;
}
@end

@fs unlit_tex_fs

layout(binding=0) uniform sampler smp;
layout(binding=0)uniform texture2D tex;

in vec2 v_uv;

out vec4 frag_color;

void main() {
    vec4 col = texture(sampler2D(tex, smp), v_uv);
    frag_color = col;
}
@end

@program unlit_tex unlit_tex_vs unlit_tex_fs


//--PHONG--------------------------------------------------------

//COLOR

@vs phong_color_vs
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 uv;

out vec3 v_pos;
out vec3 v_normal;
out vec3 v_viewpos;

@include_block vs_uniforms

void main() {
    gl_Position = proj * view * model * vec4(position, 1.0);
    v_pos = vec3(model * vec4(position, 1.0));
    v_normal = mat3(model) * normal;
    v_viewpos = viewpos;
}
@end

@fs phong_color_fs
in vec3 v_pos;
in vec3 v_normal;
in vec3 v_viewpos;

out vec4 FragColor;

layout(binding=2) uniform col_material {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    float shininess;
} col_mat;

@include_block dirlight_uniforms
@include_block phong

void main() {
    vec3 result = phong_light(
        v_pos,
        v_normal,
        v_viewpos,
        col_mat.ambient.rgb,
        col_mat.diffuse.rgb,
        col_mat.specular.rgb,
        col_mat.shininess,
        vec4(1.0)
    );
    FragColor = vec4(result, 1.0);
}

@end
@program phong_color phong_color_vs phong_color_fs

//TEXTURED

@vs phong_tex_vs
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 uv;

out vec3 v_pos;
out vec3 v_normal;
out vec2 v_uv;
out vec3 v_viewpos;

@include_block vs_uniforms

void main() {
    gl_Position = proj * view * model * vec4(position, 1.0);
    v_pos = vec3(model * vec4(position, 1.0));
    v_normal = mat3(model) * normal;
    v_uv = uv;
    v_viewpos = viewpos;
}
@end

@fs phong_tex_fs
in vec3 v_pos;
in vec3 v_normal;
in vec2 v_uv;
in vec3 v_viewpos;

out vec4 FragColor;

layout(binding=0) uniform sampler col_smp;
layout(binding=0) uniform texture2D col_tex;

layout(binding=2) uniform tex_material {
    float shininess;
} tex_mat;

@include_block dirlight_uniforms
@include_block phong

void main() {
    vec4 tex = texture(sampler2D(col_tex, col_smp), v_uv);
    vec3 result = phong_light(
        v_pos,
        v_normal,
        v_viewpos,
        tex.rgb,
        tex.rgb,
        vec3(tex.a),
        tex_mat.shininess,
        tex
    );
    FragColor = vec4(result, 1.0);
}

@end

@program phong_tex phong_tex_vs phong_tex_fs
@program skinned_phong_tex skinned_vs phong_tex_fs
