// SSAO Shader (Screen-Space Ambient Occlusion)
// 屏幕空间环境光遮蔽

#version 450 core

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(std140, binding = 0) uniform SSAOUBO {
    float u_radius;
    float u_bias;
    float u_intensity;
    float u_scale;
    float u_sharpness;
    int u_sampleCount;
    int u_useNoise;
    int u_useNormals;
    float u_normalThreshold;
    float u_padding1;
    float u_padding2;
    float u_padding3;
    
    // 采样核 (16 个样本)
    vec4 u_samples[16];
} ssao;

layout(binding = 0) uniform sampler2D u_normalTexture;
layout(binding = 1) uniform sampler2D u_depthTexture;
layout(binding = 2) uniform sampler2D u_noiseTexture;

// 投影参数 (需要从外部传入)
uniform vec3 u_cameraPosition;
uniform mat4 u_viewProjection;
uniform mat4 u_invViewProjection;
uniform vec2 u_screenSize;
uniform float u_near;
uniform float u_far;
uniform float u_fov;

// 从深度缓冲重建世界空间位置
vec3 reconstructWorldPosition(vec2 uv, float depth) {
    // 转换到 NDC 空间
    vec4 ndcPos = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    
    // 转换到视图空间
    vec4 viewPos = u_invViewProjection * ndcPos;
    viewPos /= viewPos.w;
    
    return viewPos.xyz;
}

// 从深度值重建线性深度
float reconstructLinearDepth(float depth) {
    // 对于 OpenGL 深度范围 [0, 1]
    float z = depth * 2.0 - 1.0;
    return (2.0 * u_near * u_far) / (u_far + u_near - z * (u_far - u_near));
}

// 计算 SSAO
float calculateAO(vec3 position, vec3 normal, vec2 uv) {
    float occlusion = 0.0;
    float radius = ssao.u_radius;
    
    // 获取随机旋转 (从噪声纹理)
    vec2 noiseScale = vec2(u_screenSize.x / 4.0, u_screenSize.y / 4.0);
    vec3 randomVec = texture(u_noiseTexture, uv * noiseScale).xyz;
    randomVec = normalize(randomVec);
    
    // 构建切线空间
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    
    int sampleCount = min(ssao.u_sampleCount, 16);
    
    for (int i = 0; i < sampleCount; i++) {
        // 获取采样方向
        vec3 sampleDir = TBN * normalize(ssao.u_samples[i].xyz);
        
        // 采样位置
        vec3 samplePos = position + sampleDir * radius;
        
        // 投影到屏幕空间
        vec4 samplePosProj = u_viewProjection * vec4(samplePos, 1.0);
        samplePosProj /= samplePosProj.w;
        vec2 sampleUV = samplePosProj.xy * 0.5 + 0.5;
        
        // 检查 UV 是否在有效范围内
        if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || 
            sampleUV.y < 0.0 || sampleUV.y > 1.0) {
            continue;
        }
        
        // 获取采样深度
        float sampleDepth = texture(u_depthTexture, sampleUV).r;
        float sampleLinearDepth = reconstructLinearDepth(sampleDepth);
        
        // 计算深度差
        float depthDiff = sampleLinearDepth - (-position.z);
        
        // 范围检查
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(depthDiff));
        
        // 累积遮蔽
        if (depthDiff > ssao.u_bias) {
            occlusion += rangeCheck;
        }
    }
    
    occlusion = 1.0 - (occlusion / float(sampleCount));
    return occlusion;
}

// 边缘感知模糊 (可选)
float edgeAwareBlur(vec2 uv, vec2 texelSize) {
    float centerDepth = texture(u_depthTexture, uv).r;
    float centerAO = texture(u_depthTexture, uv).r; // 这里应该是 AO 纹理
    
    float blur = 0.0;
    float totalWeight = 0.0;
    
    // 简单的 3x3 模糊
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            vec2 offset = vec2(x, y) * texelSize;
            float sampleDepth = texture(u_depthTexture, uv + offset).r;
            
            // 深度敏感的权重
            float depthDiff = abs(sampleDepth - centerDepth);
            float weight = exp(-depthDiff * 10.0);
            
            blur += weight;
            totalWeight += weight;
        }
    }
    
    return totalWeight > 0.0 ? blur / totalWeight : centerAO;
}

void main() {
    // 获取法线
    vec3 normal = texture(u_normalTexture, inUV).rgb;
    normal = normalize(normal * 2.0 - 1.0); // 从 [0,1] 转换到 [-1,1]
    
    // 获取深度
    float depth = texture(u_depthTexture, inUV).r;
    
    // 重建世界空间位置
    vec3 position = reconstructWorldPosition(inUV, depth);
    
    // 计算 SSAO
    float ao = calculateAO(position, normal, inUV);
    
    // 应用强度
    ao = mix(1.0, ao, ssao.u_intensity);
    
    // 应用缩放
    ao = pow(ao, ssao.u_scale);
    
    outColor = vec4(vec3(ao), 1.0);
}
