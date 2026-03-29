// Phoenix Engine - PBR Vertex Shader
// PBR 顶点着色器 - 支持物理基础渲染的属性传递

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

// Output to fragment shader (PBR G-buffer)
layout(location = 0) out vec3 v_worldPos;
layout(location = 1) out vec3 v_normal;
layout(location = 2) out vec2 v_texcoord;
layout(location = 3) out vec3 v_tangent;
layout(location = 4) out vec3 v_bitangent;

void main() {
    vec4 worldPos = ubo.u_model * vec4(a_position, 1.0);
    v_worldPos = worldPos.xyz;
    
    // Transform normal to world space
    mat3 normalMatrix = transpose(inverse(mat3(ubo.u_model)));
    v_normal = normalize(normalMatrix * a_normal);
    
    v_texcoord = a_texcoord;
    
    // Calculate tangent and bitangent for normal mapping
    vec3 tangent = normalize(normalMatrix * a_tangent.xyz);
    v_tangent = tangent;
    v_bitangent = normalize(cross(v_normal, tangent) * a_tangent.w);
    
    gl_Position = ubo.u_projection * ubo.u_view * worldPos;
}
