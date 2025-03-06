#version 410

uniform mat4 viewproj;
uniform mat4 model;

layout(location=0) in vec4 position;
layout(location=1) in vec4 normal;

out vec3 v_pos;
out vec3 v_normal;

void main() {
    vec4 world_pos = model * vec4(position.xyz, 1.0);
    gl_Position = viewproj * world_pos;
    v_pos = world_pos.xyz;
    v_normal = normalize(mat3(model) * normal.xyz);
}
