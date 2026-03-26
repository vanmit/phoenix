# 第五课：索引缓冲区

学习如何使用索引缓冲区优化渲染。

## 🎯 学习目标

- 理解索引缓冲区的作用
- 创建索引缓冲区
- 使用索引绘制
- 优化顶点数据

## 📝 代码示例

### 使用索引绘制正方形

```cpp
#include <phoenix/phoenix.hpp>

int main() {
    phoenix::Application app("Index Buffer Demo");
    auto window = app.createWindow(800, 600);
    auto renderer = phoenix::RenderDevice::create();
    renderer->initialize(window);
    
    // 1. 定义顶点 (4 个顶点)
    struct Vertex {
        float x, y, z;
        float r, g, b;
    };
    
    std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},  // 0: 左下 (红)
        {{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},  // 1: 右下 (绿)
        {{ 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},  // 2: 右上 (蓝)
        {{-0.5f,  0.5f, 0.0f}, {1.0f, 1.0f, 0.0f}},  // 3: 左上 (黄)
    };
    
    // 2. 定义索引 (6 个索引，组成 2 个三角形)
    std::vector<uint16_t> indices = {
        0, 1, 2,  // 三角形 1: 左下 -> 右下 -> 右上
        2, 3, 0,  // 三角形 2: 右上 -> 左上 -> 左下
    };
    
    // 3. 创建顶点缓冲区
    auto vertexBuffer = renderer->createVertexBuffer(
        vertices.data(),
        vertices.size() * sizeof(Vertex),
        phoenix::BufferUsage::Static
    );
    
    // 4. 创建索引缓冲区
    auto indexBuffer = renderer->createIndexBuffer(
        indices.data(),
        indices.size() * sizeof(uint16_t),
        phoenix::IndexType::UInt16,
        phoenix::BufferUsage::Static
    );
    
    // 5. 创建管线
    auto pipeline = renderer->createPipeline({
        .shader = shader,
        .vertexBuffer = vertexBuffer,
        .indexBuffer = indexBuffer,
        .vertexLayout = {
            { .format = phoenix::VertexFormat::Float3, .location = 0 },
            { .format = phoenix::VertexFormat::Float3, .location = 1 },
        }
    });
    
    // 6. 主循环
    while (app.isRunning()) {
        app.pollEvents();
        renderer->clear({0.1f, 0.1f, 0.2f, 1.0f});
        
        // 索引绘制
        renderer->drawIndexed(pipeline, 
            indices.size(),  // 索引数量
            0,               // 起始索引
            0                // 顶点偏移
        );
        
        renderer->present();
    }
    
    return 0;
}
```

## 📖 索引缓冲区优势

### 内存效率

```cpp
// ❌ 无索引 - 需要 6 个顶点
std::vector<Vertex> noIndices = {
    // 三角形 1
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
    {{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
    {{ 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
    // 三角形 2 (重复 2 个顶点)
    {{ 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},  // 重复
    {{-0.5f,  0.5f, 0.0f}, {1.0f, 1.0f, 0.0f}},
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},  // 重复
};
// 内存：6 * sizeof(Vertex)

// ✅ 有索引 - 只需 4 个顶点 + 6 个索引
std::vector<Vertex> withIndices = {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
    {{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
    {{ 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f,  0.5f, 0.0f}, {1.0f, 1.0f, 0.0f}},
};
std::vector<uint16_t> indices = {0, 1, 2, 2, 3, 0};
// 内存：4 * sizeof(Vertex) + 6 * sizeof(uint16_t)
// 节省约 33% 内存
```

### 顶点缓存效率

```cpp
// 使用索引时，GPU 可以缓存变换后的顶点
// 重复使用的顶点不需要重复变换

// 球体示例:
// 无索引：~2000 顶点
// 有索引：~1000 顶点 + ~3000 索引
// 节省 50% 顶点和变换计算
```

## 🔧 索引类型

### 16 位索引

```cpp
// 适用于最多 65536 个顶点的网格
std::vector<uint16_t> indices16 = {0, 1, 2, 3, 4, 5};

auto indexBuffer = renderer->createIndexBuffer(
    indices16.data(),
    indices16.size() * sizeof(uint16_t),
    phoenix::IndexType::UInt16,
    phoenix::BufferUsage::Static
);
```

### 32 位索引

```cpp
// 适用于大型网格 (>65536 顶点)
std::vector<uint32_t> indices32 = {0, 1, 2, 100000, 100001, 100002};

auto indexBuffer = renderer->createIndexBuffer(
    indices32.data(),
    indices32.size() * sizeof(uint32_t),
    phoenix::IndexType::UInt32,
    phoenix::BufferUsage::Static
);
```

### 选择指南

```cpp
// 顶点数 < 65536: 使用 UInt16 (更小，更快)
// 顶点数 >= 65536: 使用 UInt32 (支持更多顶点)

if (vertexCount < 65536) {
    indexType = phoenix::IndexType::UInt16;
} else {
    indexType = phoenix::IndexType::UInt32;
}
```

## 🎯 绘制调用

### 基础索引绘制

```cpp
// 绘制所有索引
renderer->drawIndexed(pipeline, 
    indexCount,     // 索引数量
    0,              // 起始索引
    0               // 顶点偏移
);

// 绘制部分索引
renderer->drawIndexed(pipeline,
    6,              // 绘制 6 个索引 (2 个三角形)
    12,             // 从第 12 个索引开始
    0               // 顶点偏移
);
```

### 带顶点偏移

```cpp
// 多个网格共享索引缓冲区
// 网格 1: 顶点 0-99, 索引 0-179
// 网格 2: 顶点 100-199, 索引 180-359

// 绘制网格 2
renderer->drawIndexed(pipeline,
    180,            // 索引数量
    180,            // 起始索引
    100             // 顶点偏移
);
```

### 实例化索引绘制

```cpp
// 绘制多个实例
renderer->drawIndexedInstanced(pipeline,
    indexCount,     // 每个实例的索引数
    instanceCount,  // 实例数量
    startIndex,     // 起始索引
    vertexOffset,   // 顶点偏移
    startInstance   // 起始实例
);
```

## 🔍 索引优化

### 顶点缓存优化

```cpp
// 使用 Foursquare 或 AMD TOOT 优化
MeshOptimizer optimizer;

// 优化顶点缓存 (提高 GPU 缓存命中率)
optimizer.optimizeVertexCache(
    vertices,
    indices,
    vertexCount
);

// 优化顶点获取 (改善内存访问模式)
optimizer.optimizeVertexFetch(
    vertices,
    indices
);
```

### 过度绘制优化

```cpp
// 按深度排序透明物体
// 从后向前绘制，减少过度绘制

std::sort(transparentObjects.begin(), 
          transparentObjects.end(),
          [&](const auto& a, const auto& b) {
              return camera.distanceTo(a.position) > 
                     camera.distanceTo(b.position);
          });
```

## ⚠️ 常见错误

### 错误 1: 索引类型不匹配

```cpp
// ❌ 错误 - 使用 UInt16 但索引值 > 65535
std::vector<uint16_t> indices = {0, 1, 70000};  // 溢出!

// ✅ 正确 - 使用 UInt32
std::vector<uint32_t> indices = {0, 1, 70000};
auto buffer = renderer->createIndexBuffer(
    indices.data(),
    indices.size() * sizeof(uint32_t),
    phoenix::IndexType::UInt32,
    ...
);
```

### 错误 2: 索引数量错误

```cpp
// ❌ 错误 - 索引数量不匹配
renderer->drawIndexed(pipeline, 
    100,    // 实际只有 6 个索引
    0, 0
);

// ✅ 正确
renderer->drawIndexed(pipeline,
    indices.size(),
    0, 0
);
```

### 错误 3: 忘记设置索引缓冲区

```cpp
// ❌ 错误
auto pipeline = renderer->createPipeline({
    .shader = shader,
    .vertexBuffer = vertexBuffer,
    // 缺少 .indexBuffer!
});

renderer->drawIndexed(pipeline, ...);  // 不会工作

// ✅ 正确
auto pipeline = renderer->createPipeline({
    .shader = shader,
    .vertexBuffer = vertexBuffer,
    .indexBuffer = indexBuffer,
});
```

## 🏃 练习

### 练习 1: 创建立方体

使用索引缓冲区创建立方体：

```cpp
// 8 个顶点
std::vector<Vertex> vertices = {
    // 前面
    {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}},
    {{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},
    {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}},
    // 后面
    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}},
    {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}},
    {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}},
    {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 0.0f}},
};

// 36 个索引 (12 个三角形)
std::vector<uint16_t> indices = {
    // 前面
    0, 1, 2, 2, 3, 0,
    // 后面
    4, 6, 5, 6, 4, 7,
    // 上面
    3, 2, 6, 6, 7, 3,
    // 下面
    4, 5, 1, 1, 0, 4,
    // 右面
    1, 5, 6, 6, 2, 1,
    // 左面
    4, 0, 3, 3, 7, 4,
};
```

### 练习 2: 动态索引更新

创建可更新的索引缓冲区：

```cpp
auto indexBuffer = renderer->createIndexBuffer(
    nullptr,  // 初始数据为空
    1000 * sizeof(uint16_t),
    phoenix::IndexType::UInt16,
    phoenix::BufferUsage::Dynamic
);

while (app.isRunning()) {
    // 更新索引
    std::vector<uint16_t> indices = generateIndices();
    indexBuffer->update(indices.data(), 
                        indices.size() * sizeof(uint16_t));
    
    renderer->drawIndexed(pipeline, indices.size(), 0, 0);
}
```

## 📚 下一步

完成本课后，你应该能够：
- ✅ 创建和使用索引缓冲区
- ✅ 选择合适的索引类型
- ✅ 优化顶点数据
- ✅ 实现高效的网格渲染

**下一课**: [第六课：纹理映射](lesson-06-texture-mapping.md)

---
*完整示例代码：examples/basic/05-index-buffer.cpp*
