# 着色器编写第二课：顶点着色器

## 🎯 学习目标
- 顶点处理
- 变换矩阵
- 顶点输出

## 📝 代码示例

```glsl
#version 450
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;

void main() {
    gl_Position = projMatrix * viewMatrix * modelMatrix * vec4(position, 1.0);
}
```

## 📚 下一步
**下一课**: [第三课：片段着色器](lesson-03-fragment-shader.md)
