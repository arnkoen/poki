@ctype vec3 HMM_Vec3
@ctype mat4 HMM_Mat4
@ctype vec4 sg_color

@include common.inc.glsl

//--UNLIT---------------------------------------------------------------------
//TEXTURED

@vs unlit_tex_vs
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 uv;

out vec3 v_normal;
out vec2 v_uv;

@include_block vs_uniforms

void main() {
    gl_Position = viewproj * model * vec4(position, 1.0);
    v_normal = normal;
    v_uv = uv;
}
@end

@fs unlit_tex_fs

layout(binding=0) uniform sampler smp;
layout(binding=0)uniform texture2D tex;

in vec3 v_normal;
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

out vec3 v_pos;
out vec3 v_normal;

@include_block vs_uniforms

void main() {
    gl_Position = viewproj * model * vec4(position, 1.0);
    v_pos = vec3(model * vec4(position, 1.0));
    v_normal = mat3(model) * normal;
}
@end

@fs phong_color_fs
in vec3 v_pos;
in vec3 v_normal;

out vec4 FragColor;

layout(binding=1) uniform fs_params {
    vec3 viewpos;
};

layout(binding=2) uniform col_material {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    float shininess;
} col_mat;

@include_block dirlight_uniforms

void main() {
    vec3 norm = normalize(v_normal);
    vec3 view_dir = normalize(viewpos - v_pos);
    vec3 light_dir = normalize(-light.direction);

    // diffuse shading
    float diff = max(dot(norm, light_dir), 0.0);

    // specular shading
    vec3 reflect_dir = reflect(-light_dir, norm);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), col_mat.shininess);

    // combine results
    vec3 ambient  = light.ambient.rgb * col_mat.ambient.rgb;
    vec3 diffuse  = light.diffuse.rgb  * diff * col_mat.diffuse.rgb;
    vec3 specular = light.specular.rgb * (spec * col_mat.specular.rgb);
    vec3 result = vec3(ambient + diffuse + specular);

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

@include_block vs_uniforms

void main() {
    gl_Position = viewproj * model * vec4(position, 1.0);
    v_pos = vec3(model * vec4(position, 1.0));
    v_normal = mat3(model) * normal;
    v_uv = uv;
}
@end

@fs phong_tex_fs
in vec3 v_pos;
in vec3 v_normal;
in vec2 v_uv;

out vec4 FragColor;

layout(binding=0) uniform sampler col_smp;
layout(binding=0) uniform texture2D col_tex;

layout(binding=1) uniform fs_params {
    vec3 viewpos;
};

layout(binding=2) uniform tex_material {
    float shininess;
} tex_mat;

@include_block dirlight_uniforms

void main() {
    vec3 norm = normalize(v_normal);
    vec3 view_dir = normalize(viewpos - v_pos);
    vec3 light_dir = normalize(-light.direction);

    // diffuse shading
    float diff = max(dot(norm, light_dir), 0.0);

    // specular shading
    vec3 reflect_dir = reflect(-light_dir, norm);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), tex_mat.shininess);

    // combine results
    vec4 tex = vec4(texture(sampler2D(col_tex, col_smp), v_uv));
    vec3 ambient  = light.ambient.rgb * tex.rgb;
    vec3 diffuse  = light.diffuse.rgb * diff * tex.rgb;
    vec3 specular = light.specular.rgb * spec * tex.a;
    vec3 result = vec3(ambient + diffuse + specular);

    FragColor = vec4(result, 1.0);
}

@end

@program phong_tex phong_tex_vs phong_tex_fs

