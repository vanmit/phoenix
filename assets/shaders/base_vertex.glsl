// Phoenix Engine - Base Vertex Shader
// 基础顶点着色器 - 支持 MVP 变换和基础属性传递

#version 450

// Uniform buffers
layout(std140, binding = 0) uniform UniformBuffer {
    mat4 u_model;
    mat4 u_view;
    mat4 u_projection;
    vec4 u_color;
} ubo;

// Vertex attributes
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texcoord;
layout(location = 3) in vec4 a_tangent;
layout(location = 4) in vec4 a_color;

// Output to fragment shader
layout(location = 0) out vec3 v_worldPos;
layout(location = 1) out vec3 v_normal;
layout(location = 2) out vec2 v_texcoord;
layout(location = 3) out vec4 v_color;

// Output gl_Position
void main() {
    vec4 worldPos = ubo.u_model * vec4(a_position, 1.0);
    v_worldPos = worldPos.xyz;
    
    // Transform normal to world space
    mat3 normalMatrix = transpose(inverse(mat3(ubo.u_model)));
    v_normal = normalize(normalMatrix * a_normal);
    
    v_texcoord = a_texcoord;
    v_color = a_color;
    
    // MVP transform
    gl_Position = ubo.u_projection * ubo.u_view * worldPos;
}
