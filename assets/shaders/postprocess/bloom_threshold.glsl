// Bloom Threshold Shader
// 提取高亮区域用于 Bloom 效果

#version 450 core

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(std140, binding = 0) uniform BloomThresholdUBO {
    float u_threshold;
    float u_thresholdSoft;
    float u_intensity;
    float u_knee;
    vec4 u_tint;
} bloom;

layout(binding = 0) uniform sampler2D u_inputTexture;

void main() {
    vec3 color = texture(u_inputTexture, inUV).rgb;
    
    // 计算亮度 (使用 luminance)
    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    
    // 计算膝盖值 (平滑过渡)
    float knee = bloom.u_knee;
    float threshold = bloom.u_threshold - knee;
    
    // 提取超过阈值的部分
    vec3 extracted = max(color - threshold, vec3(0.0));
    
    // 软过渡
    float alpha = smoothstep(0.0, knee * 2.0, luminance - threshold);
    extracted *= alpha;
    
    // 应用强度
    extracted *= bloom.u_intensity;
    
    // 应用色调
    extracted *= bloom.u_tint.rgb;
    
    outColor = vec4(extracted, 1.0);
}
