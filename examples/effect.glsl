@vs effect_vs

layout(location = 0) in vec3 position;
out vec3 pos;

void main()  {
    gl_Position = vec4(position, 1.0);
    pos = position;
}

@end

@fs effect_fs

in vec3 pos;
out vec4 frag_color;

layout(binding=0) uniform effect_params {
    float rand;
};

layout(binding=0) uniform sampler smp;
layout(binding=0) uniform texture2D tex;

void main() {
    vec2 uv = pos.xy * 0.5 + 0.5;
#if SOKOL_HLSL
    uv.y = 1.0 - uv.y;
#endif
    frag_color = texture(sampler2D(tex, smp), uv + vec2(floor(sin(uv.y/250.0*rand+rand*rand))*250.0*rand,0));
}

@end

@program effect effect_vs effect_fs
