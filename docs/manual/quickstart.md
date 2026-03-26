# Phoenix Engine 快速入门指南

5 分钟快速开始使用 Phoenix Engine

## 🚀 第一步：安装

### 系统要求

- **操作系统**: Windows 10+, macOS 10.15+, Linux (Ubuntu 20.04+), Android 8.0+, iOS 13+
- **编译器**: GCC 9+, Clang 10+, MSVC 2019+
- **CMake**: 3.16+
- **图形 API**: Vulkan 1.2+, Metal, DX12, 或 WebGPU

### 快速安装

```bash
# 克隆仓库
git clone https://github.com/phoenix-engine/phoenix.git
cd phoenix

# 安装依赖 (Linux)
./scripts/install-deps.sh

# 构建
mkdir build && cd build
cmake ..
make -j$(nproc)

# 运行示例
./examples/basic/hello-world
```

## 📝 第二步：创建第一个程序

### 最小示例

```cpp
#include <phoenix/phoenix.hpp>

int main() {
    // 1. 创建应用
    phoenix::Application app("My First App");
    
    // 2. 创建窗口
    auto window = app.createWindow(800, 600, "Hello Phoenix");
    
    // 3. 初始化渲染器
    auto renderer = phoenix::RenderDevice::create();
    renderer->initialize(window);
    
    // 4. 主循环
    while (app.isRunning()) {
        // 处理输入
        app.pollEvents();
        
        // 清屏
        renderer->clear({0.1f, 0.1f, 0.2f, 1.0f});
        
        // 渲染
        renderer->present();
    }
    
    return 0;
}
```

### 编译运行

```bash
# CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(MyApp)

find_package(Phoenix REQUIRED)

add_executable(myapp main.cpp)
target_link_libraries(myapp Phoenix::Core)
```

```bash
mkdir build && cd build
cmake ..
make
./myapp
```

## 🎨 第三步：绘制三角形

```cpp
#include <phoenix/phoenix.hpp>

int main() {
    phoenix::Application app("Triangle Demo");
    auto window = app.createWindow(800, 600);
    auto renderer = phoenix::RenderDevice::create();
    renderer->initialize(window);
    
    // 顶点数据
    struct Vertex {
        float x, y, z;
        float r, g, b;
    };
    
    Vertex vertices[] = {
        { 0.0f,  0.5f, 0.0f, 1.0f, 0.0f, 0.0f },
        { 0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f },
        {-0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f },
    };
    
    // 创建顶点缓冲区
    auto vertexBuffer = renderer->createVertexBuffer(
        vertices, sizeof(vertices)
    );
    
    // 创建着色器
    auto shader = renderer->createShader(
        "shaders/triangle.vert",
        "shaders/triangle.frag"
    );
    
    // 创建管线
    auto pipeline = renderer->createPipeline({
        .shader = shader,
        .vertexBuffer = vertexBuffer,
    });
    
    // 主循环
    while (app.isRunning()) {
        app.pollEvents();
        renderer->clear({0.1f, 0.1f, 0.2f, 1.0f});
        renderer->draw(pipeline, 0, 3);
        renderer->present();
    }
    
    return 0;
}
```

## 📚 第四步：学习更多

### 推荐学习路径

1. ✅ **基础渲染教程** - 10 课掌握渲染基础
2. 📖 **场景系统教程** - 5 课学习场景管理
3. 🎭 **动画系统教程** - 5 课掌握动画系统
4. 🎨 **着色器编写教程** - 5 课学习着色器编程
5. ⚡ **性能优化教程** - 5 课优化性能

### 示例程序

查看 `examples/` 目录获取完整示例：

```bash
# 基础示例
examples/basic/hello-world
examples/basic/window-creation

# 渲染示例
examples/rendering/basic-render
examples/rendering/pbr-demo

# 动画示例
examples/animation/animation-demo
```

## 🆘 遇到问题？

### 常见问题

**Q: 编译失败，找不到 Vulkan 头文件**
```bash
# Ubuntu/Debian
sudo apt-get install libvulkan-dev

# macOS (使用 MoltenVK)
brew install molten-vk

# Windows
# 从 https://vulkan.lunarg.com/ 下载 SDK
```

**Q: 运行时黑屏**
- 检查显卡驱动是否最新
- 确认 Vulkan/Metal/DX12 支持
- 查看日志：`PHOENIX_LOG_LEVEL=debug ./myapp`

**Q: 性能低下**
- 启用性能分析：`PHOENIX_PROFILER=1`
- 查看性能报告：`docs/mobile/PERFORMANCE-OPTIMIZATION-GUIDE.md`

### 获取帮助

- 📖 [完整文档](README.md)
- 💬 [GitHub Discussions](https://github.com/phoenix-engine/phoenix/discussions)
- 🐛 [报告问题](https://github.com/phoenix-engine/phoenix/issues)

---
*下一步：阅读 [安装指南](installation.md) 了解详细安装步骤*
