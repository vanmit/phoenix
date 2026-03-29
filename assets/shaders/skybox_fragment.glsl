// Phoenix Engine - Skybox Fragment Shader
// 天空盒片段着色器 - 采样立方体贴图

#version 450

// Input from vertex shader
layout(location = 0) in vec3 v_texcoord;

// Output color
layout(location = 0) out vec4 o_color;

// Cubemap sampler
layout(binding = 0) uniform samplerCube u_skybox;

void main() {
    // Sample the cubemap
    vec4 color = texture(u_skybox, v_texcoord);
    
    o_color = color;
}
