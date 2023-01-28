#version 460

layout(set = 0, binding = 0, rgba8) uniform writeonly image2D destinationImage;
layout(set = 0, binding = 1, rgba8) uniform readonly image2D sourceImage;

#define RADIUS 4
#define GROUP_SIZE 8
#define TILE_DIM (2 * RADIUS + GROUP_SIZE)

// Storage shared for this local invocation
shared vec3 tile[TILE_DIM * TILE_DIM];

vec3 tap(ivec2 pos) {
    return tile[pos.x + TILE_DIM * pos.y];
}

layout(local_size_x = GROUP_SIZE, local_size_y = GROUP_SIZE, local_size_z = 1) in;
void main() {
    ivec2 iuv = ivec2(gl_GlobalInvocationID.xy);

    // Populate local memory
    if(gl_LocalInvocationIndex < TILE_DIM * TILE_DIM / 4) {
        const ivec2 anchor = ivec2(gl_WorkGroupID.xy * GROUP_SIZE - RADIUS);

        const ivec2 coord1 = anchor + ivec2(gl_LocalInvocationIndex % TILE_DIM, gl_LocalInvocationIndex / TILE_DIM);
        const ivec2 coord2 = anchor + ivec2((gl_LocalInvocationIndex + TILE_DIM * TILE_DIM / 4) % TILE_DIM, (gl_LocalInvocationIndex + TILE_DIM * TILE_DIM / 4) / TILE_DIM);
        const ivec2 coord3 = anchor + ivec2((gl_LocalInvocationIndex + TILE_DIM * TILE_DIM / 2) % TILE_DIM, (gl_LocalInvocationIndex + TILE_DIM * TILE_DIM / 2) / TILE_DIM);
        const ivec2 coord4 = anchor + ivec2((gl_LocalInvocationIndex + TILE_DIM * TILE_DIM * 3 / 4) % TILE_DIM, (gl_LocalInvocationIndex + TILE_DIM * TILE_DIM * 3 / 4) / TILE_DIM);

        const vec3 color0 = imageLoad(sourceImage, coord1).xyz;
        const vec3 color1 = imageLoad(sourceImage, coord2).xyz;
        const vec3 color2 = imageLoad(sourceImage, coord3).xyz;
        const vec3 color3 = imageLoad(sourceImage, coord4).xyz;

        tile[gl_LocalInvocationIndex] = color0;
        tile[gl_LocalInvocationIndex + TILE_DIM * TILE_DIM / 4] = color1;
        tile[gl_LocalInvocationIndex + TILE_DIM * TILE_DIM / 2] = color2;
        tile[gl_LocalInvocationIndex + TILE_DIM * TILE_DIM * 3 / 4] = color3;
    }
    // Make fetches available to all threads
    groupMemoryBarrier();
    barrier();

    ivec2 tapBase = ivec2(gl_LocalInvocationID.xy);
    vec4 rgba = imageLoad(sourceImage, iuv);
    //vec4 rgba = vec4(tap(tapBase + ivec2(RADIUS)), 1.0);
    
    rgba.rgb = vec3(0.0);
    for(int i = 0; i < RADIUS*2; ++i) {
        for(int j = 0; j < RADIUS*2; ++j) {
            rgba.rgb += imageLoad(sourceImage, (iuv - ivec2(RADIUS) + ivec2(i, j))).rgb;
            //rgba.rgb += tap(tapBase + ivec2(i, j));
        }
    }
    rgba.rgb /= vec3(RADIUS*RADIUS*4);

    imageStore(destinationImage, iuv, rgba);
}