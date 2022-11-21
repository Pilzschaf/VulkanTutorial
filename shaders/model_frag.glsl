#version 450 core

layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec2 in_texcoord;
layout(location = 2) in vec3 in_position;

layout(set = 0, binding = 1) uniform sampler2D in_albedoSampledTexture;

layout(location = 0) out vec4 out_color;

void main() {
	vec3 view = normalize(-in_position);

	vec4 texSample = texture(in_albedoSampledTexture, in_texcoord);
	vec3 normal = normalize(in_normal);

	vec3 light = normalize(vec3(1, 1, -1));
	vec3 reflection = reflect(-light, normal);
	float shininess = 16.0f;

	vec3 diffuse = max(dot(normal, light), 0.0) * texSample.rgb;
	vec3 specular = pow(max(dot(view, reflection), 0.0), shininess) * vec3(1.0);
	vec3 ambient = 0.2f * texSample.rgb;

	vec3 combined = 0.5 * diffuse + ambient + 0.4 * specular;

	//out_color = vec4(normal * 0.5 + 0.5, 1.0);
	out_color = vec4(combined, texSample.a);
}