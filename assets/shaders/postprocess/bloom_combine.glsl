// Bloom Combine Shader
// 将 Bloom 效果合成到原始图像

#version 450 core

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(std140, binding = 0) uniform BloomCombineUBO {
    float u_intensity;
    float u_scatter;
    float u_padding1;
    float u_padding2;
} bloom;

layout(binding = 0) uniform sampler2D u_inputTexture;
layout(binding = 1) uniform sampler2D u_bloomTexture;

void main() {
    vec3 originalColor = texture(u_inputTexture, inUV).rgb;
    vec3 bloomColor = texture(u_bloomTexture, inUV).rgb;
    
    // 应用散射
    bloomColor *= bloom.u_scatter;
    
    // 合成 (加法混合)
    vec3 result = originalColor + bloomColor * bloom.u_intensity;
    
    // 简单的色调映射防止过曝
    result = result / (1.0 + result);
    
    outColor = vec4(result, 1.0);
}
