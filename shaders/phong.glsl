@ctype vec3 HMM_Vec3
@ctype mat4 HMM_Mat4
@ctype vec4 sg_color

@module pk_phong

@vs color_vs
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;

out vec3 v_pos;
out vec3 v_normal;

layout(binding=0) uniform vs_params {
    mat4 model;
    mat4 viewproj;
};

void main() {
    gl_Position = viewproj * model * vec4(position, 1.0);
    v_pos = vec3(model * vec4(position, 1.0));
    v_normal = mat3(model) * normal;
}
@end

@fs color_fs
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

layout(binding=3) uniform dir_light {
    vec3 direction;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
} light;

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
@program color color_vs color_fs

//--textured-------------------------

@vs tex_vs
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 uv;

out vec3 v_pos;
out vec3 v_normal;
out vec2 v_uv;

layout(binding=0) uniform vs_params {
    mat4 model;
    mat4 viewproj;
};

void main() {
    gl_Position = viewproj * model * vec4(position, 1.0);
    v_pos = vec3(model * vec4(position, 1.0));
    v_normal = mat3(model) * normal;
    v_uv = uv;
}
@end

@fs tex_fs
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

layout(binding=3) uniform dir_light {
    vec3 direction;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
} light;

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

@program texture tex_vs tex_fs

