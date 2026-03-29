// Phoenix Engine - IBL Prefilter Convolution Shader
// 生成预过滤环境贴图 (镜面反射 IBL，多粗糙度级别)

#version 450

// Uniform buffers
layout(std140, binding = 0) uniform UniformBuffer {
    mat4 u_viewProjection[6];
    uint u_face;
    float u_roughness;
    float u_resolution;
    float u_samplesPerAxis;
} ubo;

// Input from vertex shader
layout(location = 0) in vec3 v_texcoord;

// Output color
layout(location = 0) out vec4 o_color;

// Environment cubemap sampler (with mipmaps)
layout(binding = 0) uniform samplerCube u_environmentMap;

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
    
    // Cartesian coordinates
    vec3 H = vec3(
        cos(phi) * sinTheta,
        sin(phi) * sinTheta,
        cosTheta
    );
    
    // Tangent space to world space
    vec3 up = abs(N.z) > 0.999 ? vec3(1.0, 0.0, 0.0) : vec3(0.0, 0.0, 1.0);
    vec3 tangent = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
    
    vec3 sampleVec = tangent.x * H + tangent.y * H.y + tangent.z * H.z;
    return normalize(sampleVec);
}

void main() {
    vec3 N = normalize(v_texcoord);
    
    // Make the simplyfying assumption that V equals R equals the normal
    vec3 R = N;
    vec3 V = R;
    
    vec3 prefilteredColor = vec3(0.0);
    float totalWeight = 0.0;
    
    uint sampleCount = uint(ubo.u_samplesPerAxis * ubo.u_samplesPerAxis);
    
    for (uint i = 0u; i < sampleCount; ++i) {
        vec2 xi = hammersley(i, sampleCount);
        vec3 H = importanceSampleGGX(xi, N, ubo.u_roughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);
        
        float NdotL = max(dot(N, L), 0.0);
        if (NdotL > 0.0) {
            // Sample from the environment map
            prefilteredColor += texture(u_environmentMap, L).rgb * NdotL;
            totalWeight += NdotL;
        }
    }
    
    prefilteredColor = prefilteredColor / totalWeight;
    
    o_color = vec4(prefilteredColor, 1.0);
}
