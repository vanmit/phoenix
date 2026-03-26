# 着色器编写第四课：计算着色器

## 🎯 学习目标
- 计算着色器概念
- 工作组
- GPU 计算

## 📝 代码示例

```glsl
#version 450
layout(local_size_x = 64) in;

layout(std430, binding = 0) buffer Data {
    float data[];
};

void main() {
    uint index = gl_GlobalInvocationID.x;
    data[index] *= 2.0;
}
```

## 📚 下一步
**下一课**: [第五课：着色器优化](lesson-05-shader-optimization.md)
