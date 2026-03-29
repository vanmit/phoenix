// Phoenix Engine - Skybox Vertex Shader
// 天空盒顶点着色器 - 渲染环境立方体贴图

#version 450

// Uniform buffers
layout(std140, binding = 0) uniform UniformBuffer {
    mat4 u_model;
    mat4 u_view;
    mat4 u_projection;
    vec4 u_color;
} ubo;

// Vertex attribute
layout(location = 0) in vec3 a_position;

// Output to fragment shader
layout(location = 0) out vec3 v_texcoord;

void main() {
    // Use position as texture coordinate for cubemap
    v_texcoord = a_position;
    
    // Remove translation from view matrix for skybox
    mat4 viewNoTranslation = ubo.u_view;
    viewNoTranslation[3] = vec4(0.0, 0.0, 0.0, 1.0);
    
    // Transform position
    gl_Position = ubo.u_projection * viewNoTranslation * vec4(a_position, 1.0);
    
    // Ensure skybox is always at far depth
    gl_Position = gl_Position.xyww;
}
