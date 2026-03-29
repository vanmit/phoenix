// SSAO Blur Shader
// 模糊 SSAO 结果以去除噪声

#version 450 core

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(std140, binding = 0) uniform SSAOBlurUBO {
    vec2 u_texelSize;
    int u_direction; // 0 = horizontal, 1 = vertical
    int u_iterations;
    float u_sharpness;
    float u_padding1;
    float u_padding2;
    float u_padding3;
} blur;

layout(binding = 0) uniform sampler2D u_aoTexture;
layout(binding = 1) uniform sampler2D u_depthTexture;

// 双边模糊 (边缘感知)
float bilateralFilter(vec2 uv, vec2 texelSize) {
    float centerDepth = texture(u_depthTexture, uv).r;
    float centerAO = texture(u_aoTexture, uv).r;
    
    float result = 0.0;
    float totalWeight = 0.0;
    
    // 5x5 核
    for (int x = -2; x <= 2; x++) {
        for (int y = -2; y <= 2; y++) {
            vec2 offset = vec2(x, y) * texelSize;
            vec2 sampleUV = uv + offset;
            
            float sampleDepth = texture(u_depthTexture, sampleUV).r;
            float sampleAO = texture(u_aoTexture, sampleUV).r;
            
            // 空间权重 (高斯)
            float dist = length(vec2(x, y));
            float spatialWeight = exp(-dist * dist / 8.0);
            
            // 深度权重 (边缘感知)
            float depthDiff = abs(sampleDepth - centerDepth);
            float depthWeight = exp(-depthDiff * 100.0);
            
            // 范围权重 (AO 值相似)
            float rangeDiff = abs(sampleAO - centerAO);
            float rangeWeight = exp(-rangeDiff * 10.0);
            
            float weight = spatialWeight * depthWeight * rangeWeight;
            
            result += sampleAO * weight;
            totalWeight += weight;
        }
    }
    
    return totalWeight > 0.0 ? result / totalWeight : centerAO;
}

// 简单的分离式高斯模糊
float gaussianBlur(vec2 uv, vec2 texelSize, int direction) {
    float result = 0.0;
    
    // 9-tap 高斯核
    const float weights[9] = float[](
        0.0205, 0.0855, 0.2300, 0.2433, 0.2433, 0.2300, 0.0855, 0.0205, 0.0
    );
    const int offsets[9] = int[](-4, -3, -2, -1, 0, 1, 2, 3, 4);
    
    for (int i = 0; i < 8; i++) {
        vec2 offset;
        if (direction == 0) {
            offset = vec2(float(offsets[i]) * texelSize.x, 0.0);
        } else {
            offset = vec2(0.0, float(offsets[i]) * texelSize.y);
        }
        
        result += texture(u_aoTexture, uv + offset).r * weights[i];
    }
    
    return result;
}

void main() {
    float ao;
    
    // 使用双边模糊保持边缘
    ao = bilateralFilter(inUV, blur.u_texelSize);
    
    // 可选：额外的分离式高斯模糊
    /*
    if (blur.u_iterations > 0) {
        // 水平模糊
        ao = gaussianBlur(inUV, blur.u_texelSize, 0);
        // 垂直模糊会在另一个 pass 中完成
    }
    */
    
    outColor = vec4(vec3(ao), 1.0);
}
