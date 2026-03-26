#version 450

/**
 * @file basic.vert
 * @brief Phoenix Engine 基础顶点着色器
 * 
 * 功能:
 * - 顶点变换 (Model-View-Projection)
 * - 法线变换
 * - 纹理坐标传递
 * - 顶点颜色传递
 */

// ==================== 顶点属性 ====================

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texcoord;
layout(location = 3) in vec4 a_color;
layout(location = 4) in vec3 a_tangent;
layout(location = 5) in vec3 a_bitangent;

// ==================== 输出到片段着色器 ====================

layout(location = 0) out vec3 v_normal;
layout(location = 1) out vec2 v_texcoord;
layout(location = 2) out vec4 v_color;
layout(location = 3) out vec3 v_position;
layout(location = 4) out mat3 v_tangentToWorld;

// ==================== Uniform 缓冲 ====================

layout(std140, binding = 0) uniform PerFrameUniforms {
    mat4 u_view;
    mat4 u_projection;
    mat4 u_viewProjection;
    vec4 u_cameraPosition;
    vec4 u_time;
};

layout(std140, binding = 1) uniform PerObjectUniforms {
    mat4 u_model;
    mat4 u_normalMatrix;
    vec4 u_color;
    float u_metallic;
    float u_roughness;
    float u_padding0;
    float u_padding1;
};

// ==================== 实例化支持 ====================

// 如果启用了实例化，使用此 buffer
layout(std140, binding = 2) uniform InstanceUniforms {
    mat4 u_instanceMatrix[];
};

// ==================== 主函数 ====================

void main() {
    // 计算世界空间位置
    vec4 worldPosition = u_model * vec4(a_position, 1.0);
    v_position = worldPosition.xyz;
    
    // 变换法线到世界空间
    v_normal = mat3(u_normalMatrix) * a_normal;
    
    // 计算 TBN 矩阵 (用于法线贴图)
    vec3 T = normalize(mat3(u_model) * a_tangent);
    vec3 B = normalize(mat3(u_model) * a_bitangent);
    vec3 N = normalize(v_normal);
    v_tangentToWorld = mat3(T, B, N);
    
    // 传递纹理坐标
    v_texcoord = a_texcoord;
    
    // 传递顶点颜色 (与 uniform 颜色混合)
    v_color = a_color * u_color;
    
    // 最终变换到裁剪空间
    gl_Position = u_projection * u_view * worldPosition;
}
