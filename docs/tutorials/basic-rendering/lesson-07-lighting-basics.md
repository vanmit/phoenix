# 第七课：光照基础

学习如何在 Phoenix Engine 中实现基本光照。

## 🎯 学习目标

- 理解光照原理
- 创建光源
- 实现漫反射光照
- 实现镜面反射光照

## 📝 代码示例

### 基础光照

```cpp
#include <phoenix/phoenix.hpp>

int main() {
    phoenix::Application app("Lighting Basics");
    auto window = app.createWindow(800, 600);
    auto renderer = phoenix::RenderDevice::create();
    renderer->initialize(window);
    
    // 1. 创建光源
    auto light = phoenix::Light{
        .type = phoenix::LightType::Directional,
        .direction = {-0.5f, -1.0f, -0.3f},
        .color = {1.0f, 1.0f, 0.9f},
        .intensity = 1.0f
    };
    
    // 2. 创建带法线的顶点
    struct Vertex {
        float position[3];
        float normal[3];
    };
    
    // 3. 光照着色器
    auto shader = renderer->createShader({
        .vertexSource = R"(
            #version 450
            layout(location = 0) in vec3 position;
            layout(location = 1) in vec3 normal;
            layout(location = 0) out vec3 fragNormal;
            layout(location = 1) out vec3 fragPosition;
            
            void main() {
                gl_Position = vec4(position, 1.0);
                fragNormal = normalize(normal);
                fragPosition = position;
            }
        )",
        .fragmentSource = R"(
            #version 450
            layout(location = 0) in vec3 fragNormal;
            layout(location = 1) in vec3 fragPosition;
            layout(location = 0) out vec4 outColor;
            
            layout(std140, binding = 0) uniform Light {
                vec3 direction;
                vec3 color;
                float intensity;
            } light;
            
            void main() {
                // 环境光
                vec3 ambient = 0.1 * light.color;
                
                // 漫反射 (Lambert)
                vec3 normal = normalize(fragNormal);
                vec3 lightDir = normalize(-light.direction);
                float diff = max(dot(normal, lightDir), 0.0);
                vec3 diffuse = diff * light.color * light.intensity;
                
                // 最终颜色
                vec3 result = ambient + diffuse;
                outColor = vec4(result, 1.0);
            }
        )"
    });
    
    // ... 创建缓冲区、管线
    
    return 0;
}
```

## 💡 光源类型

### 方向光

```cpp
phoenix::Light directionalLight{
    .type = phoenix::LightType::Directional,
    .direction = {0.0f, -1.0f, 0.0f},  // 向下
    .color = {1.0f, 1.0f, 1.0f},
    .intensity = 1.0f
};
```

### 点光源

```cpp
phoenix::Light pointLight{
    .type = phoenix::LightType::Point,
    .position = {0.0f, 5.0f, 0.0f},
    .color = {1.0f, 0.8f, 0.6f},
    .intensity = 1.0f,
    .range = 10.0f  // 光照范围
};
```

### 聚光灯

```cpp
phoenix::Light spotLight{
    .type = phoenix::LightType::Spot,
    .position = {0.0f, 5.0f, 0.0f},
    .direction = {0.0f, -1.0f, 0.0f},
    .color = {1.0f, 1.0f, 1.0f},
    .intensity = 1.0f,
    .innerConeAngle = 30.0f,  // 内锥角度
    .outerConeAngle = 45.0f   // 外锥角度
};
```

## 🎨 光照模型

### Lambert 漫反射

```glsl
// 漫反射 = max(N·L, 0) * 光颜色 * 光强度
float diff = max(dot(normal, lightDir), 0.0);
vec3 diffuse = diff * light.color * light.intensity;
```

### Phong 镜面反射

```glsl
// 镜面反射 = (R·V)^shininess
vec3 viewDir = normalize(cameraPos - fragPosition);
vec3 reflectDir = reflect(-lightDir, normal);
float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
vec3 specular = spec * light.color * light.intensity;
```

### Blinn-Phong

```glsl
// 改进的镜面反射 (更高效)
vec3 halfwayDir = normalize(lightDir + viewDir);
float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
vec3 specular = spec * light.color * light.intensity;
```

## ⚠️ 常见问题

### 问题 1: 法线方向错误

```cpp
// 确保法线指向正确方向
// 对于球体，法线应该从中心指向外
for (auto& vertex : vertices) {
    vertex.normal = normalize(vertex.position - sphereCenter);
}
```

### 问题 2: 法线未归一化

```glsl
// ❌ 错误
float diff = dot(normal, lightDir);

// ✅ 正确
float diff = dot(normalize(normal), normalize(lightDir));
```

## 🏃 练习

### 练习 1: 多光源

实现多个光源：

```glsl
layout(std140, binding = 0) uniform Lights {
    vec3 positions[4];
    vec3 colors[4];
    float intensities[4];
    int lightCount;
} lights;

vec3 totalLight = vec3(0.0);
for (int i = 0; i < lights.lightCount; i++) {
    vec3 lightDir = normalize(lights.positions[i] - fragPosition);
    float diff = max(dot(normal, lightDir), 0.0);
    totalLight += diff * lights.colors[i] * lights.intensities[i];
}
```

## 📚 下一步

**下一课**: [第八课：材质系统](lesson-08-material-system.md)
