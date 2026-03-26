# 第八课：材质系统

学习如何创建和管理材质。

## 🎯 学习目标

- 理解材质概念
- 创建材质对象
- 材质属性
- 多材质渲染

## 📝 代码示例

```cpp
#include <phoenix/phoenix.hpp>

int main() {
    phoenix::Application app("Material System");
    auto window = app.createWindow(800, 600);
    auto renderer = phoenix::RenderDevice::create();
    renderer->initialize(window);
    
    // 1. 创建材质
    phoenix::Material material;
    material.albedoColor = {1.0f, 0.5f, 0.0f};  // 橙色
    material.metallic = 0.8f;
    material.roughness = 0.5f;
    material.albedoTexture = renderer->loadTexture("textures/albedo.png");
    material.normalTexture = renderer->loadTexture("textures/normal.png");
    
    // 2. 创建材质管线
    auto pipeline = renderer->createPipeline({
        .shader = pbrShader,
        .material = material,
    });
    
    // 3. 渲染
    renderer->draw(pipeline, 0, vertexCount);
    
    return 0;
}
```

## 🎨 材质属性

### PBR 材质

```cpp
phoenix::Material pbrMaterial;
pbrMaterial.albedoColor = {1.0f, 1.0f, 1.0f};
pbrMaterial.metallic = 0.0f;      // 0=非金属，1=金属
pbrMaterial.roughness = 0.5f;     // 0=光滑，1=粗糙
pbrMaterial.ao = 1.0f;            // 环境光遮蔽
```

### 标准材质

```cpp
phoenix::Material standardMaterial;
standardMaterial.diffuseColor = {1.0f, 0.0f, 0.0f};
standardMaterial.specularColor = {0.5f, 0.5f, 0.5f};
standardMaterial.shininess = 32.0f;
```

## 📚 下一步

**下一课**: [第九课：相机控制](lesson-09-camera-control.md)
