// Phoenix Engine - Debug Line Shader
// 调试线着色器 - 用于渲染调试线条和辅助几何体

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
layout(location = 1) in vec4 a_color;

// Output to fragment shader
layout(location = 0) out vec4 v_color;

void main() {
    v_color = a_color;
    gl_Position = ubo.u_projection * ubo.u_view * ubo.u_model * vec4(a_position, 1.0);
}
