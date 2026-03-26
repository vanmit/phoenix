# 着色器编写第一课：着色器基础

## 🎯 学习目标
- 理解着色器
- GLSL 语法
- 着色器阶段

## 📝 代码示例

```glsl
// 顶点着色器
#version 450
void main() {
    gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
}

// 片段着色器
#version 450
layout(location = 0) out vec4 color;
void main() {
    color = vec4(1.0, 0.0, 0.0, 1.0);
}
```

## 📚 下一步
**下一课**: [第二课：顶点着色器](lesson-02-vertex-shader.md)
