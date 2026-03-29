// Bloom Blur Shader
// 高斯模糊用于 Bloom 效果

#version 450 core

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(std140, binding = 0) uniform BloomBlurUBO {
    vec2 u_texelSize;  // (1.0/width, 1.0/height)
    float u_direction; // 0 = horizontal, 1 = vertical
    float u_radius;
} bloom;

layout(binding = 0) uniform sampler2D u_inputTexture;

// 高斯核系数 (9-tap)
const float gaussianWeights[9] = float[](
    0.0205, 0.0855, 0.2300, 0.2433, 0.2433, 0.2300, 0.0855, 0.0205, 0.0
);
const int gaussianOffsets[9] = int[](
    -4, -3, -2, -1, 0, 1, 2, 3, 4
);

void main() {
    vec3 result = vec3(0.0);
    
    vec2 texelSize = bloom.u_texelSize;
    if (bloom.u_direction > 0.5) {
        texelSize = texelSize.yx; // 交换为垂直方向
    }
    
    // 9-tap 高斯模糊
    for (int i = 0; i < 8; i++) {
        vec2 offset = texelSize * float(gaussianOffsets[i]) * bloom.u_radius;
        float weight = gaussianWeights[i];
        result += texture(u_inputTexture, inUV + offset).rgb * weight;
    }
    
    outColor = vec4(result, 1.0);
}
