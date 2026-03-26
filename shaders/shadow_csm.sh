// CSM Shadow Shader - Cascaded Shadow Maps
// Phoenix Engine Phase 3

$include "common.sh"

// ==================== Uniforms ====================

$constants
{
    // 级联矩阵
    mat4 u_cascadeMatrices[4];
    
    // 级联参数
    vec4 u_cascadeSplits;     // xyzw = 4 个级联的分割距离
    vec4 u_shadowParams;      // x=bias, y=normalBias, z=softness, w=blendWidth
    
    // 阴影贴图尺寸
    float u_shadowMapSize;
    float u_padding;
};

SAMPLER2DARRAY(s_shadowMap, 0);

// ==================== 顶点着色器 ====================

void main() {
    vec4 worldPos = u_model * vec4(a_position, 1.0);
    
    // 根据级联索引选择矩阵
    uint cascadeIndex = u_cascadeIndex;
    mat4 lightMatrix = u_cascadeMatrices[cascadeIndex];
    
    // 变换到光空间
    vec4 lightSpacePos = lightMatrix * worldPos;
    
    // 输出到阴影贴图
    gl_Position = lightSpacePos;
    
    // 传递深度用于 VSM
    v_depth = lightSpacePos.z / lightSpacePos.w;
}

// ==================== 片元着色器 ====================

void main() {
    // 标准深度
    float depth = gl_FragCoord.z;
    
#ifdef USE_VSM
    // VSM: 存储深度矩
    float depth2 = depth * depth;
    gl_FragColor = vec4(depth, depth2, 0.0, 1.0);
#else
    // 标准深度测试
    gl_FragColor = vec4(depth, depth, depth, 1.0);
#endif
}

// ==================== 阴影采样函数 ====================

// PCF 滤波
float sampleShadowPCF(vec3 uvw, float bias) {
    float texelSize = 1.0 / u_shadowMapSize;
    
    float shadow = 0.0;
    const int kernelSize = 3;
    
    for (int y = -kernelSize/2; y <= kernelSize/2; ++y) {
        for (int x = -kernelSize/2; x <= kernelSize/2; ++x) {
            vec3 offset = vec3(
                uvw.x + float(x) * texelSize,
                uvw.y + float(y) * texelSize,
                uvw.z
            );
            
            float closestDepth = texture2D(s_shadowMap, offset).r;
            float currentDepth = uvw.z - bias;
            
            shadow += (currentDepth > closestDepth) ? 1.0 : 0.0;
        }
    }
    
    return shadow / float(kernelSize * kernelSize);
}

// VSM 可见性
float sampleShadowVSM(vec3 uvw, float minVariance, float lightBleedReduction) {
    vec4 moments = texture2D(s_shadowMap, uvw.xy).xyzw;
    
    float depth = uvw.z;
    float moment1 = moments.r;
    float moment2 = moments.g;
    
    // 计算方差
    float variance = max(moment2 - moment1 * moment1, minVariance);
    
    // Chebyshev 不等式
    float d = depth - moment1;
    float pMax = 0.0;
    
    if (d > 0.0) {
        pMax = variance / (variance + d * d);
    } else {
        pMax = 1.0;
    }
    
    // 减少光泄漏
    return max(pMax, lightBleedReduction);
}

// 级联混合
float calculateCascadeBlend(float depth, float cascadeNear, float cascadeFar, float blendWidth) {
    float blendStart = cascadeFar - blendWidth;
    
    if (depth < blendStart) {
        return 0.0;
    }
    
    if (depth > cascadeFar) {
        return 1.0;
    }
    
    return (depth - blendStart) / blendWidth;
}
