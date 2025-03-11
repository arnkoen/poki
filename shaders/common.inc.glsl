@module pk

@block vs_uniforms
layout(binding=0) uniform vs_params {
    mat4 viewproj;
    mat4 model;
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
