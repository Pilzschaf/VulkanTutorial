#version 460

layout(set = 0, binding = 0, rgba8) uniform writeonly image2D destinationImage;

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main() {
    ivec2 destinationIUV = ivec2(gl_GlobalInvocationID.xy);
    vec4 rgba = vec4(1.0, 0.0, 1.0, 1.0);
    imageStore(destinationImage, destinationIUV, rgba);
}