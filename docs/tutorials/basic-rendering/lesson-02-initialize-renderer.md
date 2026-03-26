# 第二课：初始化渲染器

学习如何初始化和配置 Phoenix Engine 渲染器。

## 🎯 学习目标

- 理解渲染设备抽象
- 初始化渲染器
- 配置渲染参数
- 创建渲染管线

## 📝 代码示例

### 完整渲染器初始化

```cpp
#include <phoenix/phoenix.hpp>
#include <iostream>

int main() {
    // 1. 创建应用程序
    phoenix::Application app("Renderer Initialization");
    
    // 2. 创建窗口
    auto window = app.createWindow(800, 600, "Renderer Demo");
    
    // 3. 创建渲染设备
    auto renderer = phoenix::RenderDevice::create();
    
    // 4. 配置渲染设备
    phoenix::DeviceConfig config;
    config.vsync = true;              // 启用垂直同步
    config.doubleBuffering = true;    // 双缓冲
    config.depthBuffer = true;        // 深度缓冲
    config.stencilBuffer = false;     // 模板缓冲
    config.msaaSamples = 4;           // 4x 抗锯齿
    
    // 5. 初始化渲染器
    if (!renderer->initialize(window, config)) {
        std::cerr << "Failed to initialize renderer!" << std::endl;
        return -1;
    }
    
    std::cout << "Renderer initialized successfully!" << std::endl;
    std::cout << "Backend: " << renderer->getBackendName() << std::endl;
    
    // 6. 主循环
    while (app.isRunning()) {
        app.pollEvents();
        
        // 清屏 (包括深度缓冲)
        renderer->clear({
            .color = {0.1f, 0.1f, 0.2f, 1.0f},
            .depth = 1.0f,
            .stencil = 0,
            .clearColor = true,
            .clearDepth = true,
            .clearStencil = false
        });
        
        // 呈现帧
        renderer->present();
    }
    
    return 0;
}
```

## 📖 渲染设备后端

Phoenix Engine 支持多种渲染后端：

### 后端选择

```cpp
// 自动选择最佳后端 (默认)
auto renderer = phoenix::RenderDevice::create();

// 强制使用特定后端
auto renderer = phoenix::RenderDevice::create(
    phoenix::RenderBackend::Vulkan
);

auto renderer = phoenix::RenderDevice::create(
    phoenix::RenderBackend::Metal
);

auto renderer = phoenix::RenderDevice::create(
    phoenix::RenderBackend::DX12
);

auto renderer = phoenix::RenderDevice::create(
    phoenix::RenderBackend::WebGPU
);
```

### 检查后端支持

```cpp
if (phoenix::RenderDevice::isBackendSupported(
    phoenix::RenderBackend::Vulkan)) {
    std::cout << "Vulkan is supported!" << std::endl;
}

// 获取可用后端列表
auto backends = phoenix::RenderDevice::getAvailableBackends();
for (const auto& backend : backends) {
    std::cout << "Available: " << backend << std::endl;
}
```

## ⚙️ 设备配置

### 完整配置选项

```cpp
phoenix::DeviceConfig config;

// 显示设置
config.vsync = true;              // 垂直同步
config.doubleBuffering = true;    // 双缓冲
config.maxFPS = 60;               // 最大帧率

// 缓冲设置
config.depthBuffer = true;        // 深度缓冲 (32 位)
config.stencilBuffer = false;     // 模板缓冲 (8 位)
config.depthBits = 24;
config.stencilBits = 8;

// 抗锯齿
config.msaaSamples = 4;           // MSAA 采样数 (0, 2, 4, 8)

// 颜色格式
config.colorFormat = phoenix::ColorFormat::RGBA8;
config.sRGB = true;               // sRGB 颜色空间

// 性能选项
config.asyncResourceLoading = true;  // 异步资源加载
config.debugMode = false;            // 调试模式
```

### 性能预设

```cpp
// 高质量预设
phoenix::DeviceConfig highQuality;
highQuality.msaaSamples = 8;
highQuality.sRGB = true;
highQuality.asyncResourceLoading = true;

// 性能预设
phoenix::DeviceConfig performance;
performance.msaaSamples = 0;  // 无抗锯齿
performance.maxFPS = 144;
performance.asyncResourceLoading = true;

// 移动设备预设
phoenix::DeviceConfig mobile;
mobile.msaaSamples = 2;
mobile.maxFPS = 60;
mobile.asyncResourceLoading = true;
```

## 🎨 清除操作

### 清除颜色和深度

```cpp
// 简单清除 (仅颜色)
renderer->clear({0.1f, 0.1f, 0.2f, 1.0f});

// 完整清除
phoenix::ClearDesc clearDesc;
clearDesc.color = {0.1f, 0.1f, 0.2f, 1.0f};
clearDesc.depth = 1.0f;
clearDesc.stencil = 0;
clearDesc.clearColor = true;
clearDesc.clearDepth = true;
clearDesc.clearStencil = false;

renderer->clear(clearDesc);

// 部分清除 (仅深度)
renderer->clearDepth(1.0f);

// 部分清除 (仅颜色)
renderer->clearColor({0.0f, 0.0f, 0.0f, 1.0f});
```

## 📊 渲染状态

### 查询渲染器信息

```cpp
std::cout << "Backend: " << renderer->getBackendName() << std::endl;
std::cout << "Driver Version: " << renderer->getDriverVersion() << std::endl;
std::cout << "Device Name: " << renderer->getDeviceName() << std::endl;
std::cout << "VRAM: " << renderer->getVRAMSize() / (1024*1024) << " MB" << std::endl;
std::cout << "Max Texture Size: " << renderer->getMaxTextureSize() << std::endl;
std::cout << "Max Uniform Buffers: " << renderer->getMaxUniformBuffers() << std::endl;
```

### 性能统计

```cpp
auto stats = renderer->getStats();
std::cout << "FPS: " << stats.fps << std::endl;
std::cout << "Frame Time: " << stats.frameTime << " ms" << std::endl;
std::cout << "Draw Calls: " << stats.drawCalls << std::endl;
std::cout << "Triangles: " << stats.triangles << std::endl;
```

## 🔧 错误处理

### 初始化错误处理

```cpp
auto renderer = phoenix::RenderDevice::create();

if (!renderer) {
    std::cerr << "Failed to create render device!" << std::endl;
    return -1;
}

phoenix::DeviceConfig config;
auto result = renderer->initialize(window, config);

if (!result) {
    std::cerr << "Initialization failed: " 
              << renderer->getLastError() << std::endl;
    
    // 尝试回退到另一个后端
    if (renderer->getBackend() == phoenix::RenderBackend::Vulkan) {
        std::cout << "Trying OpenGL fallback..." << std::endl;
        renderer = phoenix::RenderDevice::create(
            phoenix::RenderBackend::OpenGL
        );
        result = renderer->initialize(window, config);
    }
    
    if (!result) {
        return -1;
    }
}
```

## 🎮 完整示例

### 带错误处理的渲染器

```cpp
#include <phoenix/phoenix.hpp>
#include <iostream>

class RendererApp {
public:
    bool initialize() {
        // 创建窗口
        window = app.createWindow(800, 600, "Renderer App");
        if (!window) {
            std::cerr << "Failed to create window" << std::endl;
            return false;
        }
        
        // 创建渲染器
        renderer = phoenix::RenderDevice::create();
        if (!renderer) {
            std::cerr << "Failed to create render device" << std::endl;
            return false;
        }
        
        // 配置
        phoenix::DeviceConfig config;
        config.vsync = true;
        config.msaaSamples = 4;
        config.depthBuffer = true;
        
        // 初始化
        if (!renderer->initialize(window, config)) {
            std::cerr << "Failed to initialize: " 
                      << renderer->getLastError() << std::endl;
            return false;
        }
        
        std::cout << "Renderer: " << renderer->getBackendName() << std::endl;
        return true;
    }
    
    void run() {
        while (app.isRunning()) {
            app.pollEvents();
            
            renderer->clear({0.1f, 0.1f, 0.2f, 1.0f});
            
            // 渲染逻辑在这里
            
            renderer->present();
        }
    }
    
private:
    phoenix::Application app;
    std::shared_ptr<phoenix::Window> window;
    std::shared_ptr<phoenix::RenderDevice> renderer;
};

int main() {
    RendererApp app;
    if (!app.initialize()) {
        return -1;
    }
    app.run();
    return 0;
}
```

## ⚠️ 常见错误

### 错误 1: 忘记初始化

```cpp
// ❌ 错误
auto renderer = phoenix::RenderDevice::create();
renderer->clear({0.0f, 0.0f, 0.0f, 1.0f});  // 崩溃！

// ✅ 正确
auto renderer = phoenix::RenderDevice::create();
renderer->initialize(window, config);
renderer->clear({0.0f, 0.0f, 0.0f, 1.0f});
```

### 错误 2: 配置不当

```cpp
// ❌ 错误 - 在移动设备上使用 8x MSAA
config.msaaSamples = 8;  // 性能问题

// ✅ 正确 - 根据平台选择
#ifdef PHOENIX_PLATFORM_MOBILE
config.msaaSamples = 2;
#else
config.msaaSamples = 4;
#endif
```

### 错误 3: 不清除深度缓冲

```cpp
// ❌ 错误 - 3D 渲染需要深度缓冲
renderer->clear({0.0f, 0.0f, 0.0f, 1.0f});

// ✅ 正确
phoenix::ClearDesc desc;
desc.color = {0.0f, 0.0f, 0.0f, 1.0f};
desc.depth = 1.0f;
desc.clearColor = true;
desc.clearDepth = true;
renderer->clear(desc);
```

## 🏃 练习

### 练习 1: 后端选择器

创建一个程序，让用户选择渲染后端：

```cpp
std::cout << "Available backends:" << std::endl;
auto backends = phoenix::RenderDevice::getAvailableBackends();
for (size_t i = 0; i < backends.size(); i++) {
    std::cout << i << ": " << backends[i] << std::endl;
}

int choice;
std::cin >> choice;

auto renderer = phoenix::RenderDevice::create(backends[choice]);
```

### 练习 2: 性能测试

比较不同 MSAA 设置的性能：

```cpp
for (int msaa : {0, 2, 4, 8}) {
    config.msaaSamples = msaa;
    renderer->initialize(window, config);
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; i++) {
        renderer->clear({0.0f, 0.0f, 0.0f, 1.0f});
        renderer->present();
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start
    ).count();
    
    std::cout << "MSAA " << msaa << ": " 
              << (duration / 1000.0) << " FPS" << std::endl;
}
```

## 📚 下一步

完成本课后，你应该能够：
- ✅ 创建和配置渲染设备
- ✅ 选择适当的渲染后端
- ✅ 处理初始化错误
- ✅ 查询渲染器状态

**下一课**: [第三课：绘制三角形](lesson-03-draw-triangle.md)

---
*完整示例代码：examples/basic/02-renderer-init.cpp*
