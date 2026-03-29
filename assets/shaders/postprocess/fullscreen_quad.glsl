// Fullscreen Quad Vertex Shader
// 用于后处理的 fullscreen quad

#version 450 core

layout(location = 0) out vec2 outUV;

// 顶点索引 (0-3 对应两个三角形)
void main() {
    // 生成 fullscreen quad 顶点
    // 顶点布局: 0=左下，1=右下，2=左上，3=右上
    
    float x = float((gl_VertexIndex & 1) << 2) - 1.0;
    float y = float((gl_VertexIndex & 2) << 1) - 1.0;
    
    gl_Position = vec4(x, y, 0.0, 1.0);
    
    // 生成 UV 坐标 (0,0) 到 (1,1)
    outUV = vec2((x + 1.0) * 0.5, (y + 1.0) * 0.5);
    
    // 翻转 Y 坐标 (如果需要)
    // outUV.y = 1.0 - outUV.y;
}
