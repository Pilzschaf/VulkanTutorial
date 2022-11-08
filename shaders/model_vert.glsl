#version 450 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

layout(set = 0, binding = 0) uniform transform {
	mat4 modelViewProj;
} u_transforms;

layout(location = 0) out vec3 out_normal;
layout(location = 1) out vec2 out_texcoord;

void main() {
	mat4 modelViewProj = u_transforms.modelViewProj;
	gl_Position = modelViewProj * vec4(in_position, 1.0);
	out_normal = in_normal;
	out_texcoord = in_texcoord;
}