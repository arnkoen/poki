@ctype mat4 HMM_Mat4
@ctype vec3 HMM_Vec3
@ctype vec4 sg_color

@module pk_unlit
@vs vs
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 uv;

out vec3 v_normal;
out vec2 v_uv;

layout(binding=1) uniform vs_params {
    mat4 viewproj;
    mat4 model;
};


void main() {
    gl_Position = viewproj * model * vec4(position, 1.0);
    v_normal = normal;
    v_uv = uv;
}
@end

@fs fs

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

@program texture vs fs
