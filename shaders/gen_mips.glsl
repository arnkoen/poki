@cs generate_mip
layout(binding=0) uniform texture2D src_mip;
layout(binding=0) uniform sampler smp;
layout(binding=1, rgba8) uniform writeonly image2D dst_mip;

layout(local_size_x=8, local_size_y=8, local_size_z=1) in;

void main() {
    ivec2 dst_coord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 dst_size = imageSize(dst_mip);

    if (dst_coord.x >= dst_size.x || dst_coord.y >= dst_size.y) {
        return;
    }

    //normalized coordinates for sampling the source mip
    //sample from the center of 4 source pixels
    vec2 src_size = vec2(textureSize(sampler2D(src_mip, smp), 0));
    vec2 texel_size = 1.0 / src_size;

    //map destination pixel to source texture coordinates
    vec2 uv = (vec2(dst_coord) + 0.5) / vec2(dst_size);

    //the sampler will automatically interpolate
    vec4 color = texture(sampler2D(src_mip, smp), uv);

    imageStore(dst_mip, dst_coord, color);
}

@end

@program generate_mip generate_mip
