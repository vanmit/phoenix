# 着色器编写第三课：片段着色器

## 🎯 学习目标
- 片段处理
- 纹理采样
- 光照计算

## 📝 代码示例

```glsl
#version 450
layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

uniform sampler2D texSampler;

void main() {
    outColor = texture(texSampler, fragColor.xy);
}
```

## 📚 下一步
**下一课**: [第四课：计算着色器](lesson-04-compute-shader.md)
