// Phoenix Engine - Base Fragment Shader
// 基础片段着色器 - 支持 Lambert 光照和纹理采样

#version 450

// Uniform buffers
layout(std140, binding = 0) uniform UniformBuffer {
    mat4 u_model;
    mat4 u_view;
    mat4 u_projection;
    vec4 u_color;
} ubo;

// Light uniform
layout(std140, binding = 1) uniform LightBuffer {
    vec3 u_lightDir;
    vec3 u_lightColor;
    float u_lightIntensity;
} lightUbo;

// Input from vertex shader
layout(location = 0) in vec3 v_worldPos;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_texcoord;
layout(location = 3) in vec4 v_color;

// Output color
layout(location = 0) out vec4 o_color;

// Texture samplers
layout(binding = 0) uniform sampler2D u_diffuseMap;
layout(binding = 1) uniform sampler2D u_normalMap;

void main() {
    // Sample diffuse texture
    vec4 diffuseColor = texture(u_diffuseMap, v_texcoord) * v_color;
    
    // Calculate normal (sample from normal map or use interpolated)
    vec3 normal = normalize(v_normal);
    
    // Simple Lambert lighting
    vec3 lightDir = normalize(-ubo.u_lightDir);
    float diff = max(dot(normal, lightDir), 0.0);
    
    // Ambient + diffuse
    vec3 ambient = vec3(0.1, 0.1, 0.1);
    vec3 diffuse = diff * lightUbo.u_lightColor * lightUbo.u_lightIntensity;
    
    vec3 finalColor = (ambient + diffuse) * diffuseColor.rgb;
    
    o_color = vec4(finalColor, diffuseColor.a);
}
