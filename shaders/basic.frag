#version 450

/**
 * @file basic.frag
 * @brief Phoenix Engine 基础片段着色器
 * 
 * 功能:
 * - 基础 Lambert 光照
 * - 纹理采样
 * - 顶点颜色
 * - PBR 基础支持
 */

// ==================== 从顶点着色器输入 ====================

layout(location = 0) in vec3 v_normal;
layout(location = 1) in vec2 v_texcoord;
layout(location = 2) in vec4 v_color;
layout(location = 3) in vec3 v_position;
layout(location = 4) in mat3 v_tangentToWorld;

// ==================== 输出 ====================

layout(location = 0) out vec4 out_color;

// ==================== Uniform 缓冲 ====================

layout(std140, binding = 0) uniform PerFrameUniforms {
    mat4 u_view;
    mat4 u_projection;
    mat4 u_viewProjection;
    vec4 u_cameraPosition;
    vec4 u_time;
};

layout(std140, binding = 1) uniform PerObjectUniforms {
    mat4 u_model;
    mat4 u_normalMatrix;
    vec4 u_color;
    float u_metallic;
    float u_roughness;
    float u_padding0;
    float u_padding1;
};

// ==================== 光照 Uniforms ====================

layout(std140, binding = 2) uniform LightingUniforms {
    vec3 u_ambientLight;
    float u_ambientIntensity;
    
    vec3 u_directionalLightDir;
    float u_padding0;
    
    vec3 u_directionalLightColor;
    float u_directionalLightIntensity;
    
    vec3 u_directionalLightPos; // 用于阴影计算
    int u_shadowEnabled;
};

// ==================== 纹理采样器 ====================

layout(binding = 0) uniform sampler2D u_diffuseTexture;
layout(binding = 1) uniform sampler2D u_normalTexture;
layout(binding = 2) uniform sampler2D u_metallicRoughnessTexture;
layout(binding = 3) uniform sampler2D u_aoTexture;
layout(binding = 4) uniform sampler2D u_emissiveTexture;

// ==================== 材质参数 ====================

layout(std140, binding = 3) uniform MaterialUniforms {
    vec4 u_baseColorFactor;
    vec4 u_emissiveFactor;
    float u_metallicFactor;
    float u_roughnessFactor;
    float u_aoFactor;
    float u_alphaCutoff;
    
    int u_hasDiffuseTexture;
    int u_hasNormalTexture;
    int u_hasMetallicRoughnessTexture;
    int u_hasAoTexture;
    int u_hasEmissiveTexture;
    int u_useNormalMap;
    int u_padding0;
    int u_padding1;
};

// ==================== 工具函数 ====================

/**
 * @brief 将法线从 [0,1] 转换到 [-1,1]
 */
vec3 decodeNormal(vec3 encoded) {
    return encoded * 2.0 - 1.0;
}

/**
 * @brief Schlick Fresnel 近似
 */
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

/**
 * @brief GGX 法线分布函数
 */
float distributionGGX(float NdotH, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH2 = NdotH * NdotH;
    
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.14159265 * denom * denom;
    
    return num / denom;
}

/**
 * @brief 几何遮蔽函数 (Schlick-GGX)
 */
float geometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return num / denom;
}

/**
 * @brief 几何函数 (Smith)
 */
float geometrySmith(float NdotV, float NdotL, float roughness) {
    float ggx2 = geometrySchlickGGX(NdotV, roughness);
    float ggx1 = geometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

// ==================== 主函数 ====================

void main() {
    // ==================== 基础颜色 ====================
    
    vec3 baseColor = v_color.rgb * u_baseColorFactor.rgb;
    
    // 采样漫反射纹理
    if (u_hasDiffuseTexture == 1) {
        vec4 diffuseSample = texture(u_diffuseTexture, v_texcoord);
        baseColor *= diffuseSample.rgb;
        
        // Alpha 测试
        if (diffuseSample.a < u_alphaCutoff) {
            discard;
        }
    }
    
    // ==================== 法线 ====================
    
    vec3 normal = normalize(v_normal);
    
    // 采样法线贴图
    if (u_useNormalMap == 1 && u_hasNormalTexture == 1) {
        vec3 normalMapSample = texture(u_normalTexture, v_texcoord).rgb;
        normal = normalize(v_tangentToWorld * decodeNormal(normalMapSampleSample));
    }
    
    // ==================== PBR 参数 ====================
    
    float metallic = u_metallicFactor;
    float roughness = u_roughnessFactor;
    
    // 采样金属度/粗糙度纹理
    if (u_hasMetallicRoughnessTexture == 1) {
        vec4 mrSample = texture(u_metallicRoughnessTexture, v_texcoord);
        roughness *= mrSample.g;
        metallic *= mrSample.b;
    }
    
    // ==================== AO ====================
    
    float ao = u_aoFactor;
    if (u_hasAoTexture == 1) {
        ao *= texture(u_aoTexture, v_texcoord).r;
    }
    
    // ==================== 自发光 ====================
    
    vec3 emissive = u_emissiveFactor.rgb;
    if (u_hasEmissiveTexture == 1) {
        emissive *= texture(u_emissiveTexture, v_texcoord).rgb;
    }
    
    // ==================== 光照计算 ====================
    
    vec3 viewDir = normalize(u_cameraPosition.xyz - v_position);
    vec3 lightDir = normalize(-u_directionalLightDir);
    
    // 半程向量
    vec3 halfDir = normalize(viewDir + lightDir);
    
    // NDF
    float NDF = distributionGGX(max(dot(normal, halfDir), 0.0), roughness);
    
    // 几何遮蔽
    float NdotV = max(dot(normal, viewDir), 0.0);
    float NdotL = max(dot(normal, lightDir), 0.0);
    float geometry = geometrySmith(NdotV, NdotL, roughness);
    
    // Fresnel
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, baseColor, metallic);
    vec3 F = fresnelSchlick(max(dot(halfDir, viewDir), 0.0), F0);
    
    // 镜面反射
    vec3 numerator = NDF * geometry * F;
    float denominator = 4.0 * NdotV * NdotL + 0.0001;
    vec3 specular = numerator / denominator;
    
    // 漫反射
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    // 最终颜色
    vec3 Lo = (kD * baseColor / 3.14159265 + specular);
    Lo *= NdotL * u_directionalLightColor * u_directionalLightIntensity;
    
    // 环境光
    vec3 ambient = (u_ambientLight * baseColor) * u_ambientIntensity;
    ambient *= ao;
    
    // 组合
    vec3 color = Lo + ambient + emissive;
    
    // Gamma 校正
    color = pow(color, vec3(1.0 / 2.2));
    
    // 输出
    out_color = vec4(color, v_color.a * u_baseColorFactor.a);
}
