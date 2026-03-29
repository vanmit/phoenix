// Phoenix Engine - BRDF LUT Generation Shader
// 生成 BRDF 查找表 (几何项 + 菲涅尔项预计算)

#version 450

// Input from vertex shader
layout(location = 0) in vec2 v_texcoord;

// Output color
layout(location = 0) out vec4 o_color;

// Constants
const float PI = 3.14159265359;

// Hammersley sequence for low-discrepancy sampling
vec2 hammersley(uint i, uint N) {
    uint bits = (i << 16u) | (i >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    
    float radicalInverseV = float(bits) * 2.3283064365386963e-10; // 1 / 2^32
    return vec2(float(i) / float(N), radicalInverseV);
}

// Importance sampling GGX
vec3 importanceSampleGGX(vec2 xi, vec3 N, float roughness) {
    float a = roughness * roughness;
    
    float phi = 2.0 * PI * xi.x;
    float cosTheta = sqrt((1.0 - xi.y) / (1.0 + (a * a - 1.0) * xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    
    return vec3(
        cos(phi) * sinTheta,
        sin(phi) * sinTheta,
        cosTheta
    );
}

// Geometry function (Schlick-GGX)
float geometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

// Geometry Smith function
float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometrySchlickGGX(NdotV, roughness);
    float ggx1 = geometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

void main() {
    // UV coordinates map to (roughness, NdotV)
    float roughness = v_texcoord.x;
    float NdotV = max(v_texcoord.y, 0.0);
    
    vec3 V = vec3(sqrt(1.0 - NdotV * NdotV), 0.0, NdotV);
    vec3 N = vec3(0.0, 0.0, 1.0);
    
    vec3 F0 = vec3(0.0); // F0 = 0 for the LUT (we'll scale it later)
    
    // Integrate numerically with Monte Carlo
    vec3 integratedE = vec3(0.0);
    float integratedD = 0.0;
    uint sampleCount = 1024u;
    
    for (uint i = 0u; i < sampleCount; ++i) {
        vec2 xi = hammersley(i, sampleCount);
        vec3 H = importanceSampleGGX(xi, N, roughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);
        
        float NdotL = max(L.z, 0.0);
        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(V, H), 0.0);
        
        if (NdotL > 0.0) {
            float G = geometrySmith(N, V, L, roughness);
            vec3 F = F0 + (vec3(1.0) - F0) * pow(1.0 - VdotH, 5.0);
            
            integratedE += F * G * NdotL;
            integratedD += G * NdotL;
        }
    }
    
    integratedE /= float(sampleCount);
    integratedD /= float(sampleCount);
    
    // Store scale and bias for Fresnel term
    // F = F0 * scale + bias
    o_color = vec4(integratedE.xy, integratedD, 1.0);
}
