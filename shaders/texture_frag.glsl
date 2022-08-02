#version 450 core

layout(location = 0) in vec3 in_color;
layout(location = 1) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform sampler2D in_sampledTexture;

void main() {
	vec4 texSample = texture(in_sampledTexture, in_uv);
	//out_color = texSample * vec4(in_color, 1.0);
	out_color = texSample;
}