#version 450 core

// Two pass gaussian blur
// https://www.rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/

layout(binding = 0) uniform sampler2D colorTex;

layout(location = 0) in vec2 texCoord;
layout (location = 0) out vec4 outColor;

layout(constant_id = 0) const bool VERTICAL = false;

layout(push_constant) uniform pushConstants {
	float pixelSize; // pixel size eg. 1 / screenWidth and 1 / screenHeight
} u_pushConstants;

float offset[3] = float[](0.0, 1.3846153846, 3.2307692308);
float weight[3] = float[](0.2270270270, 0.3162162162, 0.0702702703);

void main(void) {
    outColor = texture(colorTex, texCoord) * weight[0];
    for (int i=1; i<3; i++) {
        
        if(VERTICAL) {
            outColor += texture(colorTex, (vec2(texCoord) + vec2(0.0, offset[i] * u_pushConstants.pixelSize))) * weight[i];
            outColor += texture(colorTex, (vec2(texCoord) - vec2(0.0, offset[i] * u_pushConstants.pixelSize))) * weight[i];
        } else {
            // Horizontal
            outColor += texture(colorTex, (vec2(texCoord) + vec2(offset[i] * u_pushConstants.pixelSize, 0.0))) * weight[i];
            outColor += texture(colorTex, (vec2(texCoord) - vec2(offset[i] * u_pushConstants.pixelSize, 0.0))) * weight[i];
        }
    }
}