#version 450 core

layout(location = 0) in vec3 vertex_color;

layout(location = 0) out vec4 color_out;

void main() {
	color_out = vec4(vertex_color, 1.0);
}