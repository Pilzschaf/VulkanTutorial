#version 450 core

layout(set = 0, input_attachment_index = 0, binding = 0) uniform subpassInput in_color;

layout(location = 0) out vec4 out_color;

void main() {
    vec4 color = subpassLoad(in_color);
    out_color = vec4(vec3(length(color.rgb)/sqrt(3.0)), color.a);
}