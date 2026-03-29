// Phoenix Engine - IBL Cubemap Vertex Shader
// IBL 立方体贴图生成顶点着色器

#version 450

// Uniform buffers
layout(std140, binding = 0) uniform UniformBuffer {
    mat4 u_viewProjection[6];
    uint u_face;
} ubo;

// Vertex attribute (full screen quad)
layout(location = 0) in vec3 a_position;

// Output to fragment shader
layout(location = 0) out vec3 v_texcoord;

void main() {
    // Position is the direction for cubemap sampling
    v_texcoord = a_position;
    
    // Get the view matrix for this face
    mat4 viewProjection = ubo.u_viewProjection[ubo.u_face];
    
    // Transform position
    gl_Position = viewProjection * vec4(a_position, 1.0);
}
