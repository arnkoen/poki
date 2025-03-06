#version 410

in vec3 v_pos;
in vec3 v_normal;

uniform vec4 viewpos;
uniform vec4 base_color;
uniform vec4 halo_color;

out vec4 frag_color;

void main() {
    vec3 viewdir = normalize(viewpos.xyz - v_pos);
    float surf = max(dot(normalize(v_normal), viewdir), 0.0);
    surf = max((1.0 - surf) + 0.1, 0.0);
    surf = pow(surf, 5.0);
    vec3 col = mix(base_color.rgb, halo_color.rgb, surf);
    frag_color = vec4(col, 1.0);
}
