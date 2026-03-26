// PBR Main Shader - Cook-Torrance BRDF
// Phoenix Engine Phase 3

$include "common.sh"

// ==================== Uniforms ====================

$constants
{
    // 材质属性
    vec4 u_albedo;              // 基础颜色
    float u_metallic;           // 金属度
    float u_roughness;          // 粗糙度
    float u_ao;                 // 环境光遮蔽
    float u_normalScale;        // 法线缩放
    vec4 u_emissive;            // 自发光
    float u_clearCoat;          // 清漆层
    float u_clearCoatRoughness; // 清漆层粗糙度
    float u_anisotropy;         // 各向异性
    float u_anisotropyRotation; // 各向异性旋转
    float u_ior;                // 折射率
    float u_alphaCutoff;        // Alpha 测试阈值
    uint u_flags;               // 材质标志
    
    // 纹理变换
    vec2 u_uvScale;
    vec2 u_uvOffset;
    float u_uvRotation;
    float u_padding;
    
    // 相机位置
    vec3 u_cameraPosition;
    float u_time;
};

// 纹理
SAMPLER2D(s_albedoMap, 0);
SAMPLER2D(s_metallicRoughnessMap, 1);
SAMPLER2D(s_normalMap, 2);
SAMPLER2D(s_aoMap, 3);
SAMPLER2D(s_emissiveMap, 4);
SAMPLER2D(s_clearCoatMap, 5);
SAMPLER2D(s_anisotropyMap, 6);

// IBL 纹理
SAMPLERCUBE(s_irradianceMap, 7);
SAMPLERCUBE(s_prefilteredMap, 8);
SAMPLER2D(s_brdfLUT, 9);

// 光照 Uniforms
struct Light {
    vec4 position;      // xyz=位置，w=类型
    vec4 direction;     // xyz=方向，w=范围
    vec4 color;         // rgb=颜色，w=强度
    vec4 shadowParams;  // x=bias, y=normalBias, z=shadowMapSize, w=cascadeIndex
};

#define MAX_LIGHTS 16
BUFFER(lightBuffer, Light, u_lights, MAX_LIGHTS);
uniform uint u_lightCount;

// ==================== 工具函数 ====================

// 伽马校正
vec3 toLinear(vec3 srgb) {
    return pow(srgb, vec3(2.2));
}

vec3 toSRGB(vec3 linear) {
    return pow(linear, vec3(1.0/2.2));
}

// 法线贴图采样
vec3 sampleNormal(vec2 uv) {
    vec3 normal = texture2D(s_normalMap, uv).rgb * 2.0 - 1.0;
    normal.xy *= u_normalScale;
    return normalize(normal);
}

// TBN 矩阵
mat3 createTBN(vec3 normal, vec3 tangent, vec3 bitangent) {
    return mat3(tangent, bitangent, normal);
}

// ==================== Cook-Torrance BRDF ====================

// GGX 法线分布函数
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return num / denom;
}

// 各向异性 GGX
float DistributionAnisotropicGGX(vec3 N, vec3 H, vec3 T, vec3 B, 
                                  float roughnessX, float roughnessY) {
    float aX = roughnessX * roughnessX;
    float aY = roughnessY * roughnessY;
    
    float XdotH = dot(T, H);
    float YdotH = dot(B, H);
    float NdotH = max(dot(N, H), 0.0);
    
    float num = 1.0;
    float denom = PI * aX * aY;
    float elliptic = XdotH * XdotH / (aX * aX) + 
                     YdotH * YdotH / (aY * aY) + 
                     NdotH * NdotH;
    
    return num / (denom * elliptic * elliptic);
}

// Schlick-GGX 几何函数
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return num / denom;
}

// Smith 几何函数
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

// Schlick 菲涅尔近似
vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// 粗糙度菲涅尔
vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness) - F0, 0.0) - F0) * 
           pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// 计算 F0
vec3 calculateF0(vec3 albedo, float metallic, float ior) {
    // 电介质的基础反射率
    float F0_dielectric = pow((ior - 1.0) / (ior + 1.0), 2.0);
    
    // 金属的 F0 就是 albedo
    return mix(vec3(F0_dielectric), albedo, metallic);
}

// ==================== IBL 函数 ====================

vec3 getIrradiance(vec3 N) {
    return textureCube(s_irradianceMap, N).rgb;
}

vec3 getPrefilteredColor(vec3 R, float roughness) {
    const float MAX_REFLECTION_LOD = 7.0;
    float lod = roughness * MAX_REFLECTION_LOD;
    return textureCubeLod(s_prefilteredMap, R, lod).rgb;
}

vec2 getBRDFLUT(vec2 uv) {
    return texture2D(s_brdfLUT, uv).rg;
}

// ==================== 顶点着色器 ====================

void main() {
    // 变换 UV
    vec2 uv = a_texcoord0 * u_uvScale + u_uvOffset;
    
    // 旋转变换
    if (u_uvRotation != 0.0) {
        float s = sin(u_uvRotation);
        float c = cos(u_uvRotation);
        vec2 center = vec2(0.5);
        uv -= center;
        uv = vec2(c * uv.x - s * uv.y, s * uv.x + c * uv.y);
        uv += center;
    }
    
    v_texcoord = uv;
    v_normal = normalize((u_model * vec4(a_normal, 0.0)).xyz);
    v_tangent = normalize((u_model * vec4(a_tangent, 0.0)).xyz);
    v_bitangent = cross(v_normal, v_tangent);
    v_position = (u_model * vec4(a_position, 1.0)).xyz;
    v_viewPosition = v_position - u_cameraPosition;
    
    gl_Position = u_viewProj * vec4(v_position, 1.0);
}

// ==================== 片元着色器 ====================

void main() {
    // 采样纹理
    vec4 albedoSample = texture2D(s_albedoMap, v_texcoord);
    vec4 mrSample = texture2D(s_metallicRoughnessMap, v_texcoord);
    vec4 aoSample = texture2D(s_aoMap, v_texcoord);
    vec4 emissiveSample = texture2D(s_emissiveMap, v_texcoord);
    
    // 材质属性
    vec3 albedo = toLinear(albedoSample.rgb) * u_albedo.rgb;
    float metallic = mrSample.r * u_metallic;
    float roughness = mrSample.g * u_roughness;
    float ao = aoSample.r * u_ao;
    vec3 emissive = toLinear(emissiveSample.rgb) * u_emissive.rgb * u_emissive.a;
    
    // 法线
    vec3 N = sampleNormal(v_texcoord);
    vec3 T = normalize(v_tangent);
    vec3 B = normalize(v_bitangent);
    mat3 TBN = createTBN(N, T, B);
    N = normalize(TBN * N);
    
    // 视线方向
    vec3 V = normalize(-v_viewPosition);
    
    // 计算 F0
    vec3 F0 = calculateF0(albedo, metallic, u_ior);
    
    // 累加光照
    vec3 Lo = vec3(0.0);
    
    for (uint i = 0; i < u_lightCount; ++i) {
        Light light = u_lights[i];
        
        // 光线方向和距离
        vec3 L;
        float distance;
        
        if (light.position.w == 0.0) {
            // 方向光
            L = normalize(-light.direction.xyz);
            distance = 1000.0;
        } else {
            // 点光源/聚光灯
            L = normalize(light.position.xyz - v_position);
            distance = length(light.position.xyz - v_position);
        }
        
        // 半程向量
        vec3 H = normalize(V + L);
        
        // 角度
        float NdotL = max(dot(N, L), 0.0);
        float NdotV = max(dot(N, V), 0.0);
        float NdotH = max(dot(N, H), 0.0);
        float VdotH = max(dot(V, H), 0.0);
        
        if (NdotL > 0.0) {
            // Cook-Torrance BRDF
            float D = DistributionGGX(N, H, roughness);
            float G = GeometrySmith(N, V, L, roughness);
            vec3 F = FresnelSchlick(VdotH, F0);
            
            // 镜面反射
            vec3 numerator = D * G * F;
            float denominator = 4.0 * NdotV * NdotL + 0.0001;
            vec3 specular = numerator / denominator;
            
            // 漫反射 (能量守恒)
            vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
            vec3 diffuse = kD * albedo / PI;
            
            // 衰减
            float attenuation = 1.0 / (1.0 + distance * distance * 0.01);
            
            // 聚光灯衰减
            if (light.position.w == 2.0) {
                float theta = dot(L, normalize(-light.direction.xyz));
                float epsilon = light.direction.w - 0.05;
                attenuation *= smoothstep(epsilon, light.direction.w, theta);
            }
            
            // 累加
            Lo += (diffuse + specular) * light.color.rgb * light.color.a * 
                  NdotL * attenuation;
        }
    }
    
    // IBL - 漫反射
    vec3 irradiance = getIrradiance(N);
    vec3 diffuseIBL = irradiance * albedo;
    
    // IBL - 镜面反射
    vec3 R = reflect(-V, N);
    vec3 prefilteredColor = getPrefilteredColor(R, roughness);
    vec2 brdf = getBRDFLUT(vec2(NdotV, roughness));
    vec3 specularIBL = prefilteredColor * (F0 * brdf.x + brdf.y);
    
    // 组合
    vec3 ambient = (diffuseIBL + specularIBL) * ao;
    vec3 color = ambient + Lo;
    
    // 自发光
    color += emissive;
    
    // 清漆层
    if (u_clearCoat > 0.0) {
        float clearCoatRough = max(u_clearCoatRoughness, 0.01);
        float Dc = DistributionGGX(N, H, clearCoatRough);
        float Gc = GeometrySmith(N, V, L, clearCoatRough);
        vec3 Fc = FresnelSchlick(VdotH, vec3(0.04));
        
        float clearCoat = u_clearCoat / (4.0 * NdotV * NdotL + 0.0001);
        color += Dc * Gc * Fc * clearCoat;
    }
    
    // Gamma 校正
    color = toSRGB(color);
    
    // Alpha 测试
    if ((u_flags & 0x1) != 0 && albedoSample.a < u_alphaCutoff) {
        discard;
    }
    
    gl_FragColor = vec4(color, albedoSample.a * u_albedo.a);
}
