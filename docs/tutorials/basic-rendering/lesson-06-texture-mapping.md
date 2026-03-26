# 第六课：纹理映射

学习如何在 3D 模型上应用纹理。

## 🎯 学习目标

- 理解 UV 坐标
- 加载纹理
- 创建纹理采样器
- 应用纹理到模型

## 📝 代码示例

### 基础纹理映射

```cpp
#include <phoenix/phoenix.hpp>

int main() {
    phoenix::Application app("Texture Mapping");
    auto window = app.createWindow(800, 600);
    auto renderer = phoenix::RenderDevice::create();
    renderer->initialize(window);
    
    // 1. 定义带 UV 的顶点
    struct Vertex {
        float x, y, z;      // 位置
        float u, v;         // UV 坐标
    };
    
    std::vector<Vertex> vertices = {
        // 位置              // UV
        {{-0.5f, -0.5f, 0.0f}, {0.0f, 1.0f}},  // 左下
        {{ 0.5f, -0.5f, 0.0f}, {1.0f, 1.0f}},  // 右下
        {{ 0.5f,  0.5f, 0.0f}, {1.0f, 0.0f}},  // 右上
        {{-0.5f,  0.5f, 0.0f}, {0.0f, 0.0f}},  // 左上
    };
    
    std::vector<uint16_t> indices = {0, 1, 2, 2, 3, 0};
    
    // 2. 加载纹理
    auto texture = renderer->loadTexture("textures/checkerboard.png");
    if (!texture) {
        std::cerr << "Failed to load texture!" << std::endl;
        return -1;
    }
    
    // 3. 创建着色器 (带纹理采样)
    auto shader = renderer->createShader({
        .vertexSource = R"(
            #version 450
            layout(location = 0) in vec2 position;
            layout(location = 1) in vec2 uv;
            layout(location = 0) out vec2 fragUV;
            
            void main() {
                gl_Position = vec4(position, 0.0, 1.0);
                fragUV = uv;
            }
        )",
        .fragmentSource = R"(
            #version 450
            layout(location = 0) in vec2 fragUV;
            layout(location = 0) out vec4 outColor;
            
            layout(set = 0, binding = 0) uniform sampler2D texSampler;
            
            void main() {
                outColor = texture(texSampler, fragUV);
            }
        )"
    });
    
    // 4. 创建资源描述符
    auto descriptorSet = renderer->createDescriptorSet({
        .binding = 0,
        .type = phoenix::DescriptorType::Texture,
        .texture = texture,
        .sampler = renderer->createSampler({
            .magFilter = phoenix::FilterMode::Linear,
            .minFilter = phoenix::FilterMode::Linear,
            .wrapMode = phoenix::WrapMode::Repeat
        })
    });
    
    // 5. 创建管线
    auto pipeline = renderer->createPipeline({
        .shader = shader,
        .descriptorSets = {descriptorSet},
        .vertexLayout = {
            { .format = phoenix::VertexFormat::Float2, .location = 0 },
            { .format = phoenix::VertexFormat::Float2, .location = 1 },
        }
    });
    
    // 6. 主循环
    while (app.isRunning()) {
        app.pollEvents();
        renderer->clear({0.1f, 0.1f, 0.2f, 1.0f});
        renderer->drawIndexed(pipeline, indices.size(), 0, 0);
        renderer->present();
    }
    
    return 0;
}
```

## 📖 UV 坐标

### UV 坐标系统

```
(0,0) 左上角 -------- (1,0) 右上角
     |                    |
     |                    |
     |                    |
(0,1) 左下角 -------- (1,1) 右下角
```

### UV 布局示例

```cpp
// 正方形 UV 映射
std::vector<Vertex> quad = {
    {{-0.5f, -0.5f, 0.0f}, {0.0f, 1.0f}},  // 左下
    {{ 0.5f, -0.5f, 0.0f}, {1.0f, 1.0f}},  // 右下
    {{ 0.5f,  0.5f, 0.0f}, {1.0f, 0.0f}},  // 右上
    {{-0.5f,  0.5f, 0.0f}, {0.0f, 0.0f}},  // 左上
};

// 只使用纹理的一部分 (0.5-1.0)
std::vector<Vertex> partial = {
    {{-0.5f, -0.5f, 0.0f}, {0.5f, 1.0f}},
    {{ 0.5f, -0.5f, 0.0f}, {1.0f, 1.0f}},
    {{ 0.5f,  0.5f, 0.0f}, {1.0f, 0.5f}},
    {{-0.5f,  0.5f, 0.0f}, {0.5f, 0.5f}},
};
```

## 🎨 纹理过滤

### 过滤模式

```cpp
// 最近邻过滤 (像素化效果)
auto sampler = renderer->createSampler({
    .magFilter = phoenix::FilterMode::Nearest,
    .minFilter = phoenix::FilterMode::Nearest,
});

// 线性过滤 (平滑)
auto sampler = renderer->createSampler({
    .magFilter = phoenix::FilterMode::Linear,
    .minFilter = phoenix::FilterMode::Linear,
});

// 各向异性过滤 (高质量)
auto sampler = renderer->createSampler({
    .magFilter = phoenix::FilterMode::Linear,
    .minFilter = phoenix::FilterMode::Linear,
    .anisotropy = 16.0f,  // 16x 各向异性
});
```

## 🔁 纹理环绕

### 环绕模式

```cpp
// 重复 (默认)
auto sampler = renderer->createSampler({
    .wrapMode = phoenix::WrapMode::Repeat,
});

// 镜像重复
auto sampler = renderer->createSampler({
    .wrapMode = phoenix::WrapMode::MirroredRepeat,
});

// 钳位到边缘
auto sampler = renderer->createSampler({
    .wrapMode = phoenix::WrapMode::ClampToEdge,
});

// 分别设置 UV
auto sampler = renderer->createSampler({
    .wrapModeU = phoenix::WrapMode::Repeat,
    .wrapModeV = phoenix::WrapMode::ClampToEdge,
});
```

## 📊 纹理格式

### 支持格式

```cpp
// 标准格式
phoenix::TextureFormat::RGBA8      // 8-bit per channel
phoenix::TextureFormat::RGB8
phoenix::TextureFormat::RGBA16F    // 16-bit float
phoenix::TextureFormat::RGBA32F    // 32-bit float

// 压缩格式 (节省内存)
phoenix::TextureFormat::BC1        // DXT1, 4bpp
phoenix::TextureFormat::BC3        // DXT5, 8bpp
phoenix::TextureFormat::BC7        // 高质量，8bpp

// 移动端格式
phoenix::TextureFormat::ASTC_4x4   // ASTC 压缩
phoenix::TextureFormat::PVRTC      // PowerVR 压缩
```

## ⚠️ 常见错误

### 错误 1: UV 坐标超出范围

```cpp
// ❌ 可能导致意外结果 (取决于环绕模式)
Vertex v {{0.0f, 0.0f, 0.0f}, {2.0f, 2.0f}};  // UV > 1.0

// ✅ 保持在 0-1 范围
Vertex v {{0.0f, 0.0f, 0.0f}, {0.5f, 0.5f}};
```

### 错误 2: 纹理路径错误

```cpp
// ❌ 错误路径
auto tex = renderer->loadTexture("texture.png");  // 可能找不到

// ✅ 使用正确路径
auto tex = renderer->loadTexture("textures/texture.png");

// ✅ 检查加载结果
if (!tex) {
    std::cerr << "Failed to load texture" << std::endl;
}
```

### 错误 3: 忘记绑定纹理

```cpp
// ❌ 忘记创建描述符集
auto pipeline = renderer->createPipeline({
    .shader = shader,
    // 缺少 descriptorSets!
});

// ✅ 正确绑定
auto pipeline = renderer->createPipeline({
    .shader = shader,
    .descriptorSets = {textureDescriptorSet},
});
```

## 🏃 练习

### 练习 1: 纹理平铺

创建平铺纹理效果：

```cpp
// UV 坐标乘以平铺因子
float tiling = 4.0f;
std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f, 0.0f}, {0.0f, tiling}},
    {{ 0.5f, -0.5f, 0.0f}, {tiling, tiling}},
    {{ 0.5f,  0.5f, 0.0f}, {tiling, 0.0f}},
    {{-0.5f,  0.5f, 0.0f}, {0.0f, 0.0f}},
};
```

### 练习 2: 纹理动画

创建滚动纹理效果：

```cpp
float time = 0.0f;
float scrollSpeed = 0.1f;

while (app.isRunning()) {
    time += app.getDeltaTime() * scrollSpeed;
    
    // 更新 UV 偏移
    std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f, 0.0f}, {0.0f, 1.0f + time}},
        {{ 0.5f, -0.5f, 0.0f}, {1.0f, 1.0f + time}},
        {{ 0.5f,  0.5f, 0.0f}, {1.0f, 0.0f + time}},
        {{-0.5f,  0.5f, 0.0f}, {0.0f, 0.0f + time}},
    };
    
    vertexBuffer->update(vertices.data(), sizeof(vertices));
    
    renderer->drawIndexed(pipeline, indices.size(), 0, 0);
}
```

## 📚 下一步

**下一课**: [第七课：光照基础](lesson-07-lighting-basics.md)

---
*完整示例代码：examples/basic/06-texture-mapping.cpp*
