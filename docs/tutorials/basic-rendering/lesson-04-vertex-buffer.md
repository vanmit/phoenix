# 第四课：顶点缓冲区

深入学习顶点缓冲区的创建和使用。

## 🎯 学习目标

- 理解缓冲区类型
- 创建和管理顶点缓冲区
- 使用索引缓冲区
- 动态更新缓冲区

## 📝 代码示例

### 顶点缓冲区基础

```cpp
#include <phoenix/phoenix.hpp>

int main() {
    phoenix::Application app("Vertex Buffer Demo");
    auto window = app.createWindow(800, 600);
    auto renderer = phoenix::RenderDevice::create();
    renderer->initialize(window);
    
    // 定义顶点格式
    struct Vertex {
        float position[3];
        float color[3];
    };
    
    // 创建顶点数据
    std::vector<Vertex> vertices = {
        {{ 0.0f,  0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
    };
    
    // 创建顶点缓冲区
    auto vertexBuffer = renderer->createVertexBuffer(
        vertices.data(),
        vertices.size() * sizeof(Vertex),
        phoenix::BufferUsage::Static
    );
    
    // 创建顶点布局
    phoenix::VertexLayout layout = {
        { .format = phoenix::VertexFormat::Float3, 
          .location = 0, 
          .offset = 0 },
        { .format = phoenix::VertexFormat::Float3, 
          .location = 1, 
          .offset = sizeof(float) * 3 },
    };
    
    // ... 创建着色器和管线
    // ... 渲染循环
    
    return 0;
}
```

## 📖 缓冲区类型

### BufferUsage 枚举

```cpp
namespace phoenix {
    enum class BufferUsage {
        Static,     // 数据很少改变
        Dynamic,    // 数据经常改变
        Stream,     // 每帧都改变
        StaticRead, // 静态且需要读取
        DynamicRead // 动态且需要读取
    };
}
```

### 使用场景

```cpp
// Static - 静态几何体 (地形、建筑)
auto staticBuffer = renderer->createVertexBuffer(
    data, size, phoenix::BufferUsage::Static
);

// Dynamic - 动态几何体 (UI、粒子)
auto dynamicBuffer = renderer->createVertexBuffer(
    data, size, phoenix::BufferUsage::Dynamic
);

// Stream - 每帧更新 (粒子系统、变形)
auto streamBuffer = renderer->createVertexBuffer(
    data, size, phoenix::BufferUsage::Stream
);
```

## 🔧 缓冲区管理

### 更新缓冲区数据

```cpp
// 方法 1: 映射缓冲区
void* mapped = vertexBuffer->map(
    phoenix::MapMode::Write
);
memcpy(mapped, newData, dataSize);
vertexBuffer->unmap();

// 方法 2: 直接更新
vertexBuffer->update(
    newData, 
    dataSize, 
    offset  // 可选偏移
);

// 方法 3: 部分更新
vertexBuffer->update(
    newData,
    dataSize,
    offset,
    size  // 更新的数据大小
);
```

### 环形缓冲区 (用于流式数据)

```cpp
class RingBuffer {
public:
    RingBuffer(std::shared_ptr<phoenix::VertexBuffer> buffer, size_t size)
        : buffer_(buffer), size_(size), offset_(0) {}
    
    void* nextFrame(size_t dataSize) {
        // 如果空间不足，回绕
        if (offset_ + dataSize > size_) {
            offset_ = 0;
        }
        
        void* ptr = buffer_->map(phoenix::MapMode::Write);
        offset_ = (offset_ + 255) & ~255;  // 对齐
        return (char*)ptr + offset_;
    }
    
    void advance(size_t size) {
        offset_ += size;
        buffer_->unmap();
    }
    
private:
    std::shared_ptr<phoenix::VertexBuffer> buffer_;
    size_t size_;
    size_t offset_;
};
```

## 📊 索引缓冲区

### 创建索引缓冲区

```cpp
// 16 位索引
std::vector<uint16_t> indices = {
    0, 1, 2,  // 三角形 1
    2, 3, 0,  // 三角形 2
};

auto indexBuffer = renderer->createIndexBuffer(
    indices.data(),
    indices.size() * sizeof(uint16_t),
    phoenix::IndexType::UInt16,
    phoenix::BufferUsage::Static
);

// 32 位索引 (大型网格)
std::vector<uint32_t> indices32 = {
    0, 1, 2, 3, 4, 5,  // ...
};

auto indexBuffer32 = renderer->createIndexBuffer(
    indices32.data(),
    indices32.size() * sizeof(uint32_t),
    phoenix::IndexType::UInt32,
    phoenix::BufferUsage::Static
);
```

### 使用索引绘制

```cpp
// 创建管线时关联索引缓冲区
auto pipeline = renderer->createPipeline({
    .shader = shader,
    .vertexBuffer = vertexBuffer,
    .indexBuffer = indexBuffer,
    .vertexLayout = layout,
});

// 索引绘制
renderer->drawIndexed(pipeline, 
    6,      // 索引数量
    0,      // 起始索引
    0       // 顶点偏移
);

// 带实例化的索引绘制
renderer->drawIndexedInstanced(pipeline,
    6,      // 索引数量
    4,      // 实例数量
    0,      // 起始索引
    0,      // 顶点偏移
    0       // 起始实例
);
```

## 🎯 顶点属性指针

### 交错顶点数据

```cpp
// 交错格式 (推荐 - 更好的缓存性能)
struct Vertex {
    float x, y, z;      // 位置
    float nx, ny, nz;   // 法线
    float u, v;         // 纹理坐标
};

phoenix::VertexLayout layout = {
    { .format = phoenix::VertexFormat::Float3, 
      .location = 0, 
      .offset = 0,
      .stride = sizeof(Vertex) },
    { .format = phoenix::VertexFormat::Float3, 
      .location = 1, 
      .offset = sizeof(float) * 3,
      .stride = sizeof(Vertex) },
    { .format = phoenix::VertexFormat::Float2, 
      .location = 2, 
      .offset = sizeof(float) * 6,
      .stride = sizeof(Vertex) },
};
```

### 分离属性 (高级)

```cpp
// 分离位置和其他属性 (用于变换反馈)
struct Position {
    float x, y, z;
};

struct Attributes {
    float nx, ny, nz;
    float u, v;
};

auto positionBuffer = renderer->createVertexBuffer(...);
auto attributeBuffer = renderer->createVertexBuffer(...);

phoenix::VertexLayout layout = {
    { .format = phoenix::VertexFormat::Float3, 
      .location = 0, 
      .buffer = 0 },  // 位置缓冲区
    { .format = phoenix::VertexFormat::Float3, 
      .location = 1, 
      .buffer = 1 },  // 属性缓冲区
    { .format = phoenix::VertexFormat::Float2, 
      .location = 2, 
      .buffer = 1 },
};
```

## ⚡ 性能优化

### 顶点缓冲区最佳实践

```cpp
// ✅ 好的做法

// 1. 使用 Static 用于静态几何
auto terrainBuffer = renderer->createVertexBuffer(
    data, size, phoenix::BufferUsage::Static
);

// 2. 使用 16 位索引 (如果可能)
std::vector<uint16_t> indices;  // 而不是 uint32_t

// 3. 交错顶点数据
struct Vertex {
    float pos[3];
    float normal[3];
    float uv[2];
};

// 4. 对齐数据
static_assert(sizeof(Vertex) % 16 == 0, "Vertex size should be 16-byte aligned");

// ❌ 避免的做法

// 1. 每帧创建新缓冲区
while (running) {
    auto buffer = renderer->createVertexBuffer(...);  // 慢!
}

// 2. 使用 Stream 用于静态数据
auto staticBuffer = renderer->createVertexBuffer(
    data, size, phoenix::BufferUsage::Stream  // 浪费!
);

// 3. 分离属性 (除非必要)
struct Position { float x, y, z; };
struct Normal { float x, y, z; };
// 导致更多缓存未命中
```

### 批量绘制

```cpp
// ❌ 低效 - 多次绘制调用
for (const auto& mesh : meshes) {
    renderer->draw(mesh.pipeline, 0, mesh.vertexCount);
}

// ✅ 高效 - 批量绘制
// 1. 合并顶点缓冲区
std::vector<Vertex> allVertices;
for (const auto& mesh : meshes) {
    allVertices.insert(allVertices.end(), 
        mesh.vertices.begin(), 
        mesh.vertices.end());
}

// 2. 一次绘制
auto batchBuffer = renderer->createVertexBuffer(...);
renderer->draw(batchPipeline, 0, allVertices.size());
```

## 🔍 调试

### 缓冲区验证

```cpp
// 检查缓冲区大小
size_t bufferSize = vertexBuffer->getSize();
std::cout << "Buffer size: " << bufferSize << " bytes" << std::endl;

// 检查使用方式
auto usage = vertexBuffer->getUsage();
std::cout << "Usage: " << static_cast<int>(usage) << std::endl;

// 读取缓冲区 (调试用)
std::vector<char> data(bufferSize);
vertexBuffer->read(data.data(), bufferSize);
```

### 可视化顶点数据

```cpp
// 调试：输出顶点信息
auto vertices = static_cast<Vertex*>(vertexBuffer->map(
    phoenix::MapMode::Read
));

for (size_t i = 0; i < vertexCount; i++) {
    std::cout << "Vertex " << i << ": "
              << "pos=(" << vertices[i].x << ", " 
                       << vertices[i].y << ", " 
                       << vertices[i].z << ") "
              << "color=(" << vertices[i].r << ", " 
                         << vertices[i].g << ", " 
                         << vertices[i].b << ")"
              << std::endl;
}

vertexBuffer->unmap();
```

## 🏃 练习

### 练习 1: 动态顶点更新

创建一个程序，每帧更新顶点位置：

```cpp
struct Vertex {
    float x, y, z;
    float r, g, b;
};

auto buffer = renderer->createVertexBuffer(
    nullptr,  // 初始数据为空
    1000 * sizeof(Vertex),
    phoenix::BufferUsage::Dynamic
);

while (app.isRunning()) {
    // 更新顶点
    std::vector<Vertex> vertices(1000);
    float time = app.getTime();
    
    for (int i = 0; i < 1000; i++) {
        float angle = (i / 1000.0f) * 3.14159f * 2.0f + time;
        vertices[i] = {
            std::cos(angle) * 0.5f,
            std::sin(angle) * 0.5f,
            0.0f,
            (std::cos(angle) + 1.0f) / 2.0f,
            (std::sin(angle) + 1.0f) / 2.0f,
            0.5f
        };
    }
    
    buffer->update(vertices.data(), vertices.size() * sizeof(Vertex));
    
    renderer->draw(pipeline, 0, vertices.size());
}
```

### 练习 2: 索引正方形

使用索引缓冲区绘制正方形：

```cpp
std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
    {{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
    {{ 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f,  0.5f, 0.0f}, {1.0f, 1.0f, 0.0f}},
};

std::vector<uint16_t> indices = {
    0, 1, 2,  // 第一个三角形
    2, 3, 0,  // 第二个三角形
};

auto vertexBuffer = renderer->createVertexBuffer(...);
auto indexBuffer = renderer->createIndexBuffer(...);

renderer->drawIndexed(pipeline, 6, 0, 0);
```

## 📚 下一步

完成本课后，你应该能够：
- ✅ 创建和管理顶点缓冲区
- ✅ 使用索引缓冲区
- ✅ 更新动态缓冲区
- ✅ 优化顶点数据布局

**下一课**: [第五课：索引缓冲区](lesson-05-index-buffer.md)

---
*完整示例代码：examples/basic/04-vertex-buffer.cpp*
