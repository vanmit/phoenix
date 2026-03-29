// Phoenix Engine - PBR Fragment Shader with IBL
// PBR 片段着色器 - 支持 IBL 光照的 Cook-Torrance BRDF 实现

#version 450

// Uniform buffers
layout(std140, binding = 0) uniform UniformBuffer {
    mat4 u_model;
    mat4 u_view;
    mat4 u_projection;
    vec4 u_color;
} ubo;

// Light uniforms
layout(std140, binding = 1) uniform LightBuffer {
    vec3 u_lightDir;
    float u_padding1;
    vec3 u_lightColor;
    float u_lightIntensity;
    vec3 u_viewPos;
    float u_exposure;
} lightUbo;

// IBL uniforms
layout(std140, binding = 2) uniform IBLBuffer {
    vec3 u_cameraPos;
    float u_padding2;
} iblUbo;

// Input from vertex shader
layout(location = 0) in vec3 v_worldPos;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_texcoord;
layout(location = 3) in vec3 v_tangent;
layout(location = 4) in vec3 v_bitangent;

// Output color
layout(location = 0) out vec4 o_color;

// PBR Texture samplers
layout(binding = 0) uniform sampler2D u_albedoMap;
layout(binding = 1) uniform sampler2D u_normalMap;
layout(binding = 2) uniform sampler2D u_metallicRoughnessMap;
layout(binding = 3) uniform sampler2D u_aoMap;
layout(binding = 4) uniform sampler2D u_emissiveMap;

// IBL Texture samplers
layout(binding = 5) uniform samplerCube u_irradianceMap;
layout(binding = 6) uniform samplerCube u_prefilteredMap;
layout(binding = 7) uniform sampler2D u_brdfLUT;

// Constants
const float PI = 3.14159265359;
const float MAX_REFLECTANCE = 0.04;

// ============================================================================
// BRDF Functions
// ============================================================================

// Fresnel-Schlick approximation
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Fresnel-Schlick with roughness
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness) - F0, 0.0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Smith visibility function (GGX)
float smithVisibilityGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

// GGX normal distribution function
float distributionGGX(float NdotH, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH2 = NdotH * NdotH;
    float denom = NdotH2 * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

// ============================================================================
// IBL Functions
// ============================================================================

// Get BRDF LUT values
vec2 getBRDFLUT(float NdotV, float roughness) {
    return texture(u_brdfLUT, vec2(NdotV, roughness)).rg;
}

// Sample prefiltered environment map
vec3 samplePrefilteredEnvMap(vec3 R, float roughness) {
    const float MAX_REFLECTION_LOD = 7.0; // 根据 MIP 级别数调整
    float lod = roughness * MAX_REFLECTION_LOD;
    return textureLod(u_prefilteredMap, R, lod).rgb;
}

// ============================================================================
// Main PBR Lighting
// ============================================================================

void main() {
    // Sample PBR textures
    vec3 albedo = pow(texture(u_albedoMap, v_texcoord).rgb, vec3(2.2));
    float metallic = texture(u_metallicRoughnessMap, v_texcoord).r;
    float roughness = texture(u_metallicRoughnessMap, v_texcoord).g;
    float ao = texture(u_aoMap, v_texcoord).r;
    vec3 emissive = pow(texture(u_emissiveMap, v_texcoord).rgb, vec3(2.2));
    
    // Sample normal map and reconstruct normal in world space
    vec3 normalSample = texture(u_normalMap, v_texcoord).rgb * 2.0 - 1.0;
    mat3 TBN = mat3(v_tangent, v_bitangent, v_normal);
    vec3 N = normalize(TBN * normalSample);
    
    // View direction
    vec3 V = normalize(iblUbo.u_cameraPos - v_worldPos);
    vec3 R = reflect(-V, N);
    
    // Calculate F0 (base reflectance)
    vec3 F0 = vec3(MAX_REFLECTANCE);
    F0 = mix(F0, albedo, metallic);
    
    // ========================================================================
    // IBL Diffuse Lighting (Irradiance)
    // ========================================================================
    vec3 F_diffuse = vec3(0.0);
    vec3 irradiance = texture(u_irradianceMap, N).rgb;
    
    // Diffuse BRDF (Lambertian)
    vec3 kD = (1.0 - F0) * (1.0 - metallic);
    vec3 diffuse = kD * irradiance * albedo / PI;
    
    // ========================================================================
    // IBL Specular Lighting (Prefiltered Environment Map)
    // ========================================================================
    vec2 brdf = getBRDFLUT(max(dot(N, V), 0.0), roughness);
    vec3 F_specular = F0 * brdf.x + brdf.y;
    
    vec3 prefilteredColor = samplePrefilteredEnvMap(R, roughness);
    vec3 specular = prefilteredColor * F_specular;
    
    // ========================================================================
    // Direct Light Contribution (if any)
    // ========================================================================
    vec3 lightDir = normalize(-lightUbo.u_lightDir);
    vec3 halfDir = normalize(V + lightDir);
    
    float NdotL = max(dot(N, lightDir), 0.0);
    float NdotV_dir = max(dot(N, V), 0.0);
    float NdotH = max(dot(N, halfDir), 0.0);
    float VdotH = max(dot(V, halfDir), 0.0);
    
    vec3 directDiffuse = vec3(0.0);
    vec3 directSpecular = vec3(0.0);
    
    if (NdotL > 0.0) {
        // NDF
        float NDF = distributionGGX(NdotH, roughness);
        
        // Geometry
        float NDF_V = smithVisibilityGGX(NdotV_dir, roughness);
        float NDF_L = smithVisibilityGGX(NdotL, roughness);
        float G = NDF_V * NDF_L;
        
        // Fresnel
        vec3 F = fresnelSchlick(VdotH, F0);
        
        // Cook-Torrance BRDF
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * NdotV_dir * NdotL + 0.001;
        vec3 brdf_direct = numerator / denominator;
        
        // Direct lighting
        directDiffuse = kD * albedo / PI * lightUbo.u_lightColor * lightUbo.u_lightIntensity * NdotL;
        directSpecular = brdf_direct * lightUbo.u_lightColor * lightUbo.u_lightIntensity * NdotL;
    }
    
    // ========================================================================
    // Final Color
    // ========================================================================
    // Combine IBL and direct lighting
    vec3 color = (diffuse + specular) * ao + directDiffuse + directSpecular + emissive;
    
    // Apply exposure
    color = vec3(1.0) - exp(-color * lightUbo.u_exposure);
    
    // Tone mapping (ACES approximation)
    color = color * (1.0 + color * 0.004) / (color * (0.004 + 0.994) + 1.0);
    
    // Gamma correction (sRGB)
    color = pow(color, vec3(1.0 / 2.2));
    
    o_color = vec4(color, 1.0);
}
