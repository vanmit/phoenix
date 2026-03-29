// Phoenix Engine - IBL Irradiance Convolution Shader
// 生成辐照度贴图 (漫反射 IBL)

#version 450

// Uniform buffers
layout(std140, binding = 0) uniform UniformBuffer {
    mat4 u_viewProjection[6];
    uint u_face;
} ubo;

// Input from vertex shader
layout(location = 0) in vec3 v_texcoord;

// Output color
layout(location = 0) out vec4 o_color;

// Environment cubemap sampler
layout(binding = 0) uniform samplerCube u_environmentMap;

// Constants
const float PI = 3.14159265359;
const float SAMPLE_COUNT = 1024.0;

void main() {
    // The direction vector is already the normal for this texel
    vec3 normal = normalize(v_texcoord);
    
    // Calculate tangent and bitangent
    vec3 up = abs(normal.y) > 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(0.0, 1.0, 0.0);
    vec3 tangent = normalize(cross(up, normal));
    vec3 bitangent = cross(normal, tangent);
    
    // Irradiance convolution with cosine-weighted sampling
    vec3 irradiance = vec3(0.0);
    
    float sampleDelta = 0.025; // 2.5 degrees
    float nrSamples = 0.0;
    
    for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta) {
        for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta) {
            // Spherical to cartesian coordinates
            vec3 tangentSpaceVector = vec3(
                sin(theta) * cos(phi),
                sin(theta) * sin(phi),
                cos(theta)
            );
            
            // Transform to world space
            vec3 sampleVector = 
                tangentSpaceVector.x * tangent +
                tangentSpaceVector.y * bitangent +
                tangentSpaceVector.z * normal;
            
            // Cosine-weighted sampling (Lambertian)
            irradiance += texture(u_environmentMap, sampleVector).rgb * cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    
    // Average and scale by PI (Lambertian BRDF)
    irradiance = PI * irradiance / nrSamples;
    
    o_color = vec4(irradiance, 1.0);
}
