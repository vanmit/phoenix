# 第三课：绘制三角形

学习如何使用 Phoenix Engine 绘制第一个三角形。

## 🎯 学习目标

- 理解顶点数据
- 创建顶点着色器
- 创建片段着色器
- 绘制基本图形

## 📝 代码示例

### 绘制彩色三角形

```cpp
#include <phoenix/phoenix.hpp>

int main() {
    phoenix::Application app("Triangle");
    auto window = app.createWindow(800, 600);
    auto renderer = phoenix::RenderDevice::create();
    renderer->initialize(window);
    
    // 1. 定义顶点 (位置 + 颜色)
    struct Vertex {
        float x, y, z;  // 位置
        float r, g, b;  // 颜色
    };
    
    Vertex vertices[] = {
        // 顶点 0 - 顶部 (红色)
        { 0.0f,  0.5f, 0.0f,  1.0f, 0.0f, 0.0f },
        // 顶点 1 - 右下 (绿色)
        { 0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f },
        // 顶点 2 - 左下 (蓝色)
        {-0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f },
    };
    
    // 2. 创建顶点缓冲区
    auto vertexBuffer = renderer->createVertexBuffer(
        vertices, 
        sizeof(vertices),
        phoenix::BufferUsage::Static
    );
    
    // 3. 创建着色器
    auto shader = renderer->createShader({
        .vertexSource = R"(
            #version 450
            layout(location = 0) in vec3 position;
            layout(location = 1) in vec3 color;
            layout(location = 0) out vec3 fragColor;
            
            void main() {
                gl_Position = vec4(position, 1.0);
                fragColor = color;
            }
        )",
        .fragmentSource = R"(
            #version 450
            layout(location = 0) in vec3 fragColor;
            layout(location = 0) out vec4 outColor;
            
            void main() {
                outColor = vec4(fragColor, 1.0);
            }
        )"
    });
    
    // 4. 创建图形管线
    auto pipeline = renderer->createPipeline({
        .shader = shader,
        .vertexBuffer = vertexBuffer,
        .topology = phoenix::PrimitiveTopology::TriangleList,
        .vertexLayout = {
            { .format = phoenix::VertexFormat::Float3, .location = 0 }, // position
            { .format = phoenix::VertexFormat::Float3, .location = 1 }, // color
        }
    });
    
    // 5. 主循环
    while (app.isRunning()) {
        app.pollEvents();
        
        renderer->clear({0.1f, 0.1f, 0.2f, 1.0f});
        
        // 绘制三角形
        renderer->draw(pipeline, 0, 3);
        
        renderer->present();
    }
    
    return 0;
}
```

## 📖 代码解析

### 1. 顶点数据结构

```cpp
struct Vertex {
    float x, y, z;  // 3D 位置
    float r, g, b;  // 颜色
};
```

每个顶点包含：
- **位置**: (x, y, z) 坐标，范围 -1 到 1
- **颜色**: (r, g, b) 颜色值，范围 0 到 1

### 2. 顶点缓冲区

```cpp
auto vertexBuffer = renderer->createVertexBuffer(
    vertices,           // 顶点数据
    sizeof(vertices),   // 数据大小
    phoenix::BufferUsage::Static  // 使用方式
);
```

`BufferUsage` 选项：
- `Static`: 数据不常改变 (最佳性能)
- `Dynamic`: 数据经常改变
- `Stream`: 每帧都改变

### 3. 顶点着色器

```glsl
#version 450
// 输入属性
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

// 输出到片段着色器
layout(location = 0) out vec3 fragColor;

void main() {
    // 设置顶点位置
    gl_Position = vec4(position, 1.0);
    // 传递颜色
    fragColor = color;
}
```

### 4. 片段着色器

```glsl
#version 450
// 从顶点着色器接收
layout(location = 0) in vec3 fragColor;

// 输出颜色
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}
```

### 5. 绘制调用

```cpp
renderer->draw(pipeline, 0, 3);
//              ^      ^  ^
//              |      |  └─ 顶点数量
//              |      └─ 起始顶点
//              └─ 管线对象
```

## 🎨 顶点格式

### 常用顶点属性

```cpp
// 位置
{ .format = phoenix::VertexFormat::Float3, .location = 0 }

// 法线
{ .format = phoenix::VertexFormat::Float3, .location = 1 }

// 纹理坐标
{ .format = phoenix::VertexFormat::Float2, .location = 2 }

// 颜色
{ .format = phoenix::VertexFormat::Float4, .location = 3 }

// 切线
{ .format = phoenix::VertexFormat::Float3, .location = 4 }
```

### 完整顶点结构

```cpp
struct Vertex {
    float x, y, z;      // 位置 (location 0)
    float nx, ny, nz;   // 法线 (location 1)
    float u, v;         // 纹理坐标 (location 2)
    float r, g, b, a;   // 颜色 (location 3)
};

// 顶点布局
phoenix::VertexLayout layout = {
    { .format = phoenix::VertexFormat::Float3, .location = 0 },
    { .format = phoenix::VertexFormat::Float3, .location = 1 },
    { .format = phoenix::VertexFormat::Float2, .location = 2 },
    { .format = phoenix::VertexFormat::Float4, .location = 3 },
};
```

## 🔺 图元拓扑

### 支持的拓扑类型

```cpp
// 三角形列表 (每个三角形独立)
.topology = phoenix::PrimitiveTopology::TriangleList

// 三角形带 (共享顶点)
.topology = phoenix::PrimitiveTopology::TriangleStrip

// 线列表
.topology = phoenix::PrimitiveTopology::LineList

// 线带
.topology = phoenix::PrimitiveTopology::LineStrip

// 点列表
.topology = phoenix::PrimitiveTopology::PointList
```

### 三角形带示例

```cpp
// 使用三角形带绘制两个三角形 (只需 4 个顶点)
Vertex vertices[] = {
    { -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f },  // 0
    { -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 0.0f },  // 1
    {  0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f },  // 2
    {  0.5f,  0.5f, 0.0f, 1.0f, 1.0f, 0.0f },  // 3
};

// 三角形：0-1-2, 2-1-3
auto pipeline = renderer->createPipeline({
    .topology = phoenix::PrimitiveTopology::TriangleStrip,
    // ...
});

renderer->draw(pipeline, 0, 4);
```

## 🎭 混合模式

### 启用透明度

```cpp
auto pipeline = renderer->createPipeline({
    .shader = shader,
    .blending = {
        .enable = true,
        .srcColor = phoenix::BlendFactor::SrcAlpha,
        .dstColor = phoenix::BlendFactor::OneMinusSrcAlpha,
        .srcAlpha = phoenix::BlendFactor::One,
        .dstAlpha = phoenix::BlendFactor::Zero,
    }
});
```

### 常用混合模式

```cpp
// 正常混合 (透明度)
.srcColor = SrcAlpha, .dstColor = OneMinusSrcAlpha

// 添加混合 (发光效果)
.srcColor = SrcAlpha, .dstColor = One

// 乘法混合 (阴影)
.srcColor = Zero, .dstColor = SrcColor

// 屏幕混合 (变亮)
.srcColor = OneMinusDstColor, .dstColor = One
```

## ⚠️ 常见错误

### 错误 1: 顶点格式不匹配

```cpp
// ❌ 错误 - 着色器期望 vec3，但提供了 vec2
struct Vertex { float x, y; float r, g, b; };
// 着色器：layout(location = 0) in vec3 position;

// ✅ 正确
struct Vertex { float x, y, z; float r, g, b; };
```

### 错误 2: 忘记设置顶点布局

```cpp
// ❌ 错误
auto pipeline = renderer->createPipeline({
    .shader = shader,
    .vertexBuffer = vertexBuffer,
    // 缺少 vertexLayout!
});

// ✅ 正确
auto pipeline = renderer->createPipeline({
    .shader = shader,
    .vertexBuffer = vertexBuffer,
    .vertexLayout = {
        { .format = phoenix::VertexFormat::Float3, .location = 0 },
        { .format = phoenix::VertexFormat::Float3, .location = 1 },
    }
});
```

### 错误 3: 坐标系统错误

```cpp
// ❌ 错误 - 使用像素坐标
Vertex v { 400.0f, 300.0f, 0.0f, ... };  // 不会显示

// ✅ 正确 - 使用归一化设备坐标 (-1 到 1)
Vertex v { 0.0f, 0.5f, 0.0f, ... };  // 屏幕中心上方
```

## 🏃 练习

### 练习 1: 绘制正方形

使用两个三角形绘制一个正方形：

```cpp
Vertex vertices[] = {
    // 三角形 1
    { -0.5f,  0.5f, 0.0f,  1.0f, 0.0f, 0.0f },
    { -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f },
    {  0.5f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f },
    // 三角形 2
    { -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f },
    {  0.5f, -0.5f, 0.0f,  1.0f, 1.0f, 0.0f },
    {  0.5f,  0.5f, 0.0f,  0.0f, 1.0f, 1.0f },
};

renderer->draw(pipeline, 0, 6);
```

### 练习 2: 旋转三角形

使用 uniform 变量旋转三角形：

```cpp
// 顶点着色器
#version 450
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 0) out vec3 fragColor;

layout(std140, binding = 0) uniform Transform {
    float time;
};

void main() {
    float c = cos(time);
    float s = sin(time);
    vec2 rotated = vec2(
        position.x * c - position.y * s,
        position.x * s + position.y * c
    );
    gl_Position = vec4(rotated, position.z, 1.0);
    fragColor = color;
}
```

## 📚 下一步

完成本课后，你应该能够：
- ✅ 定义顶点数据
- ✅ 创建顶点缓冲区
- ✅ 编写基本着色器
- ✅ 绘制三角形

**下一课**: [第四课：顶点缓冲区](lesson-04-vertex-buffer.md)

---
*完整示例代码：examples/basic/03-draw-triangle.cpp*
