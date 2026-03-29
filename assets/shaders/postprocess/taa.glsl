// TAA (Temporal Anti-Aliasing) Fragment Shader
// 实现时间性抗锯齿，包含运动矢量重投影和邻域钳制
#version 450 core

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec2 v_texcoord;

// Uniforms
layout(std140, binding = 0) uniform TAAUniforms {
    vec2 u_texelSize;           // 纹素大小 (1/width, 1/height)
    float u_blendFactor;        // 历史混合因子
    float u_sharpness;          // 锐化强度
    float u_varianceThreshold;  // 方差阈值
    float u_motionBlurFactor;   // 运动模糊因子
    vec4 u_jitter;              // 抖动偏移
    mat4 u_prevViewProj;        // 上一帧的 View*Projection 矩阵
    mat4 u_currViewProj;        // 当前帧的 View*Projection 矩阵
    mat4 u_inverseProjection;   // 逆投影矩阵
};

// 纹理绑定
layout(binding = 1) uniform sampler2D u_currentFrame;   // 当前帧颜色
layout(binding = 2) uniform sampler2D u_historyFrame;   // 历史帧颜色
layout(binding = 3) uniform sampler2D u_motionVectors;  // 运动矢量
layout(binding = 4) uniform sampler2D u_depth;          // 深度纹理

// 常量
const float EPSILON = 1e-5;
const int NEIGHBORHOOD_SIZE = 4;  // 邻域大小 (4x4)

/**
 * @brief 从深度重建世界空间位置
 */
vec3 reconstructWorldPosition(vec2 uv, float depth) {
    vec4 clipPos = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 worldPos = u_inverseProjection * clipPos;
    return worldPos.xyz / worldPos.w;
}

/**
 * @brief 计算上一帧的 UV 坐标 (时间性重投影)
 */
vec2 reprojectToPrevious(vec3 worldPos) {
    vec4 prevClip = u_prevViewProj * vec4(worldPos, 1.0);
    prevClip /= prevClip.w;
    return prevClip.xy * 0.5 + 0.5;
}

/**
 * @brief 计算当前帧的 UV 坐标
 */
vec2 projectToCurrent(vec3 worldPos) {
    vec4 currClip = u_currViewProj * vec4(worldPos, 1.0);
    currClip /= currClip.w;
    return currClip.xy * 0.5 + 0.5;
}

/**
 * @brief 邻域钳制 (Neighborhood Clamping)
 * 防止鬼影的关键技术
 */
vec4 neighborhoodClamp(vec2 uv, vec4 centerColor, float depth) {
    vec4 minColor = centerColor;
    vec4 maxColor = centerColor;
    
    // 采样邻域像素
    for (int y = -NEIGHBORHOOD_SIZE / 2; y <= NEIGHBORHOOD_SIZE / 2; y++) {
        for (int x = -NEIGHBORHOOD_SIZE / 2; x <= NEIGHBORHOOD_SIZE / 2; x++) {
            if (x == 0 && y == 0) continue;
            
            vec2 neighborUV = uv + vec2(x, y) * u_texelSize;
            
            // 检查深度相似性 (避免跨越深度边界)
            float neighborDepth = texture(u_depth, neighborUV).r;
            float depthDiff = abs(neighborDepth - depth);
            
            if (depthDiff < 0.01) {  // 深度阈值
                vec4 neighborColor = texture(u_currentFrame, neighborUV);
                minColor = min(minColor, neighborColor);
                maxColor = max(maxColor, neighborColor);
            }
        }
    }
    
    // 钳制中心颜色到邻域范围
    return clamp(centerColor, minColor, maxColor);
}

/**
 * @brief 计算颜色方差
 */
float calculateVariance(vec4 a, vec4 b) {
    vec4 diff = a - b;
    return dot(diff, diff) / 4.0;
}

/**
 * @brief 自适应混合因子
 * 根据运动速度和颜色变化调整混合
 */
float adaptiveBlendFactor(vec2 uv, vec2 motionVector, float depth) {
    float baseBlend = u_blendFactor;
    
    // 运动速度越快，历史权重越低
    float motionSpeed = length(motionVector);
    float motionFactor = exp(-motionSpeed * 5.0);
    
    // 深度边缘降低混合
    float depthEdge = 0.0;
    for (int i = -1; i <= 1; i += 2) {
        float neighborDepth1 = texture(u_depth, uv + vec2(i * u_texelSize.x, 0)).r;
        float neighborDepth2 = texture(u_depth, uv + vec2(0, i * u_texelSize.y)).r;
        depthEdge += abs(neighborDepth1 - depth) + abs(neighborDepth2 - depth);
    }
    depthEdge = smoothstep(0.0, 0.1, depthEdge);
    
    // 综合混合因子
    return baseBlend * motionFactor * (1.0 - depthEdge * 0.5);
}

/**
 * @brief 锐化滤波器
 */
vec4 applySharpening(vec2 uv, vec4 color, float sharpness) {
    if (sharpness <= 0.0) return color;
    
    vec4 center = texture(u_currentFrame, uv);
    vec4 sum = vec4(0.0);
    
    // 简单的锐化核
    sum += texture(u_currentFrame, uv + vec2(-1, 0) * u_texelSize) * 0.125;
    sum += texture(u_currentFrame, uv + vec2(1, 0) * u_texelSize) * 0.125;
    sum += texture(u_currentFrame, uv + vec2(0, -1) * u_texelSize) * 0.125;
    sum += texture(u_currentFrame, uv + vec2(0, 1) * u_texelSize) * 0.125;
    
    vec4 sharpened = center + (center - sum) * sharpness;
    return sharpened;
}

void main() {
    vec2 uv = v_texcoord;
    
    // 采样当前帧
    vec4 currentColor = texture(u_currentFrame, uv);
    float depth = texture(u_depth, uv).r;
    
    // 采样运动矢量
    vec2 motionVector = texture(u_motionVectors, uv).xy;
    
    // 重建世界空间位置
    vec3 worldPos = reconstructWorldPosition(uv, depth);
    
    // 时间性重投影到上一帧
    vec2 prevUV = reprojectToPrevious(worldPos);
    
    // 检查上一帧 UV 是否在有效范围内
    if (prevUV.x < 0.0 || prevUV.x > 1.0 || prevUV.y < 0.0 || prevUV.y > 1.0) {
        // 超出范围，不使用历史
        fragColor = applySharpening(uv, currentColor, u_sharpness);
        return;
    }
    
    // 采样历史帧
    vec4 historyColor = texture(u_historyFrame, prevUV);
    
    // 邻域钳制 (防止鬼影)
    vec4 clampedHistory = neighborhoodClamp(prevUV, historyColor, depth);
    
    // 方差钳制 (可选)
    if (u_varianceThreshold > 0.0) {
        float variance = calculateVariance(currentColor, clampedHistory);
        if (variance > u_varianceThreshold) {
            // 方差过大，降低历史权重
            u_blendFactor *= 0.5;
        }
    }
    
    // 自适应混合
    float blendFactor = adaptiveBlendFactor(uv, motionVector, depth);
    
    // 混合当前帧和历史帧
    vec4 result = mix(clampedHistory, currentColor, blendFactor);
    
    // 应用锐化
    result = applySharpening(uv, result, u_sharpness);
    
    fragColor = result;
}
