// Motion Blur Fragment Shader
// 实现基于运动矢量的每像素运动模糊
#version 450 core

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec2 v_texcoord;

// Uniforms
layout(std140, binding = 0) uniform MotionBlurUniforms {
    vec2 u_texelSize;           // 纹素大小
    float u_shutterSpeed;       // 快门速度 (1/1000 秒)
    float u_intensity;          // 运动模糊强度
    int u_sampleCount;          // 采样数
    float u_maxBlurDistance;    // 最大模糊距离
    int u_useCameraMotion;      // 是否使用相机运动
    vec3 u_cameraVelocity;      // 相机速度
    float u_nearPlane;          // 近裁剪面
    float u_farPlane;           // 远裁剪面
};

// 纹理绑定
layout(binding = 1) uniform sampler2D u_color;          // 颜色纹理
layout(binding = 2) uniform sampler2D u_motionVectors;  // 运动矢量
layout(binding = 3) uniform sampler2D u_depth;          // 深度纹理

// 常量
const float PI = 3.14159265359;
const int MAX_SAMPLES = 32;

/**
 * @brief 从深度重建线性深度
 */
float reconstructLinearDepth(float depth) {
    return (2.0 * u_nearPlane * u_farPlane) / (u_farPlane + u_nearPlane - depth * (u_farPlane - u_nearPlane));
}

/**
 * @brief 计算运动模糊的采样偏移
 */
vec2 calculateMotionOffset(vec2 motionVector, float sampleIndex, float totalSamples) {
    // 使用自适应采样模式
    float t = sampleIndex / totalSamples;
    
    // 使用泊松圆盘采样或线性采样
    return motionVector * (t - 0.5) * u_intensity;
}

/**
 * @brief 深度感知的运动模糊
 * 根据深度调整模糊强度
 */
float depthBasedIntensity(float depth) {
    // 远处物体运动模糊减弱
    float linearDepth = reconstructLinearDepth(depth);
    float depthFactor = 1.0 - smoothstep(u_nearPlane, u_farPlane, linearDepth);
    return depthFactor;
}

/**
 * @brief 相机运动模糊
 */
vec2 calculateCameraMotion(vec2 uv) {
    if (u_useCameraMotion == 0) return vec2(0.0);
    
    // 简化的相机运动模糊
    // 实际项目中应该从相机变换矩阵计算
    vec2 offset = u_cameraVelocity.xy * u_shutterSpeed * 0.001;
    return offset;
}

/**
 * @brief 高质量运动模糊采样
 */
vec4 sampleMotionBlur(vec2 uv, vec2 motionVector) {
    vec4 result = vec4(0.0);
    float totalWeight = 0.0;
    
    // 使用高斯权重
    for (int i = 0; i < u_sampleCount && i < MAX_SAMPLES; i++) {
        float t = float(i) / float(u_sampleCount - 1);
        
        // 高斯分布权重
        float gaussianT = t - 0.5;
        float weight = exp(-2.0 * gaussianT * gaussianT);
        
        // 计算采样位置
        vec2 sampleUV = uv + motionVector * gaussianT * u_intensity;
        
        // 采样颜色
        vec4 sampleColor = texture(u_color, sampleUV);
        
        result += sampleColor * weight;
        totalWeight += weight;
    }
    
    return result / totalWeight;
}

/**
 * @brief 优化的运动模糊 (使用 mipmap 近似)
 */
vec4 fastMotionBlur(vec2 uv, vec2 motionVector) {
    float blurLength = length(motionVector) * u_intensity;
    
    if (blurLength < 0.5) {
        // 模糊太小，直接返回
        return texture(u_color, uv);
    }
    
    // 使用各向异性模糊近似
    // 沿运动方向采样
    vec4 result = vec4(0.0);
    int samples = min(u_sampleCount, 16);
    
    for (int i = 0; i < samples; i++) {
        float t = float(i) / float(samples - 1) - 0.5;
        vec2 sampleUV = uv + motionVector * t * u_intensity;
        result += texture(u_color, sampleUV);
    }
    
    return result / float(samples);
}

void main() {
    vec2 uv = v_texcoord;
    
    // 采样运动矢量
    vec2 motionVector = texture(u_motionVectors, uv).xy;
    
    // 添加相机运动
    if (u_useCameraMotion == 1) {
        motionVector += calculateCameraMotion(uv);
    }
    
    // 采样深度
    float depth = texture(u_depth, uv).r;
    
    // 深度感知的强度调整
    float depthFactor = depthBasedIntensity(depth);
    
    // 应用深度因子
    motionVector *= depthFactor;
    
    // 限制最大模糊距离
    if (length(motionVector) > u_maxBlurDistance) {
        motionVector = normalize(motionVector) * u_maxBlurDistance;
    }
    
    // 如果运动太小，跳过模糊
    if (length(motionVector) < 0.001) {
        fragColor = texture(u_color, uv);
        return;
    }
    
    // 计算运动模糊
    vec4 result = sampleMotionBlur(uv, motionVector);
    
    fragColor = result;
}
