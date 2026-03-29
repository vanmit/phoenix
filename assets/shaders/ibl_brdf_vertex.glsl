// Phoenix Engine - IBL BRDF LUT Vertex Shader
// BRDF LUT 生成顶点着色器 (全屏四边形)

#version 450

// Vertex attribute (full screen quad)
layout(location = 0) in vec3 a_position;

// Output to fragment shader
layout(location = 0) out vec2 v_texcoord;

void main() {
    // Generate texture coordinates from position
    // Position is in [-1, 1], texcoord should be [0, 1]
    v_texcoord = a_position.xy * 0.5 + 0.5;
    
    // Full screen quad
    gl_Position = vec4(a_position, 1.0);
}
