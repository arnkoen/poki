
@block vs_uniforms
layout(binding=0) uniform vs_params {
    mat4 view;
    mat4 proj;
    mat4 model;
    vec3 viewpos;
};
@end

@block dirlight_uniforms
layout(binding=3) uniform dir_light {
    vec3 direction;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
} light;
@end

@block phong
vec3 phong_light(
    vec3 v_pos,
    vec3 v_normal,
    vec3 viewpos,
    vec3 material_ambient,
    vec3 material_diffuse,
    vec3 material_specular,
    float shininess,
    vec4 tex_color
) {
    vec3 norm = normalize(v_normal);
    vec3 view_dir = normalize(viewpos - v_pos);
    vec3 light_dir = normalize(-light.direction);

    float diff = max(dot(norm, light_dir), 0.0);

    vec3 reflect_dir = reflect(-light_dir, norm);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), shininess);

    vec3 ambient  = light.ambient.rgb * material_ambient;
    vec3 diffuse  = light.diffuse.rgb * diff * material_diffuse;
    vec3 specular = light.specular.rgb * spec * material_specular;

    return ambient + diffuse + specular;
}
@end
