# 第一课：创建第一个窗口

学习如何创建 Phoenix Engine 应用程序窗口。

## 🎯 学习目标

- 理解 Phoenix Engine 应用结构
- 创建应用程序实例
- 创建窗口
- 处理窗口事件

## 📝 代码示例

### 最小窗口程序

```cpp
#include <phoenix/phoenix.hpp>
#include <iostream>

int main() {
    // 1. 创建应用程序实例
    phoenix::Application app("My First Window");
    
    // 2. 创建窗口
    // 参数：宽度，高度，标题
    auto window = app.createWindow(800, 600, "Hello Phoenix");
    
    if (!window) {
        std::cerr << "Failed to create window!" << std::endl;
        return -1;
    }
    
    std::cout << "Window created successfully!" << std::endl;
    std::cout << "Size: " << window->getWidth() << "x" 
              << window->getHeight() << std::endl;
    
    // 3. 主循环
    while (app.isRunning()) {
        // 处理窗口事件
        app.pollEvents();
        
        // 清屏 (深蓝色)
        window->clear({0.1f, 0.1f, 0.3f, 1.0f});
        
        // 交换缓冲区
        window->swapBuffers();
    }
    
    return 0;
}
```

### CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.16)
project(FirstWindow)

# 查找 Phoenix Engine
find_package(Phoenix REQUIRED)

# 创建可执行文件
add_executable(first-window main.cpp)

# 链接 Phoenix Engine
target_link_libraries(first-window 
    Phoenix::Core
    Phoenix::Platform
)

# 设置 C++ 标准
target_compile_features(first-window PRIVATE cxx_std_17)
```

### 构建和运行

```bash
# 创建构建目录
mkdir build && cd build

# 配置
cmake ..

# 构建
make

# 运行
./first-window
```

## 📖 代码解析

### 1. 包含头文件

```cpp
#include <phoenix/phoenix.hpp>
```

这是 Phoenix Engine 的主头文件，包含了所有核心功能。

### 2. 创建应用程序

```cpp
phoenix::Application app("My First Window");
```

`Application` 类是应用程序的入口点，负责：
- 初始化引擎
- 管理窗口
- 处理事件循环
- 管理资源生命周期

### 3. 创建窗口

```cpp
auto window = app.createWindow(800, 600, "Hello Phoenix");
```

`createWindow` 方法参数：
- `width`: 窗口宽度 (像素)
- `height`: 窗口高度 (像素)
- `title`: 窗口标题

### 4. 主循环

```cpp
while (app.isRunning()) {
    app.pollEvents();
    window->clear({0.1f, 0.1f, 0.3f, 1.0f});
    window->swapBuffers();
}
```

主循环执行以下步骤：
1. `isRunning()`: 检查应用是否应该继续运行
2. `pollEvents()`: 处理窗口事件 (键盘、鼠标、关闭等)
3. `clear()`: 清屏，使用 RGBA 颜色
4. `swapBuffers()`: 交换前后缓冲区，显示渲染结果

## 🎨 颜色值

`clear()` 方法使用 RGBA 颜色：

```cpp
// 红色
{1.0f, 0.0f, 0.0f, 1.0f}

// 绿色
{0.0f, 1.0f, 0.0f, 1.0f}

// 蓝色
{0.0f, 0.0f, 1.0f, 1.0f}

// 黑色
{0.0f, 0.0f, 0.0f, 1.0f}

// 白色
{1.0f, 1.0f, 1.0f, 1.0f}

// 半透明
{1.0f, 0.0f, 0.0f, 0.5f}  // 50% 透明红色
```

## 🔧 窗口配置

### 全屏窗口

```cpp
auto window = app.createWindow(
    1920, 1080, 
    "Fullscreen App",
    phoenix::WindowFlags::Fullscreen
);
```

### 无边框窗口

```cpp
auto window = app.createWindow(
    800, 600,
    "Borderless",
    phoenix::WindowFlags::Borderless
);
```

### 可调整大小

```cpp
auto window = app.createWindow(
    800, 600,
    "Resizable",
    phoenix::WindowFlags::Resizable
);
```

### 组合标志

```cpp
auto flags = phoenix::WindowFlags::Resizable | 
             phoenix::WindowFlags::Maximized;
             
auto window = app.createWindow(
    800, 600,
    "Combined Flags",
    flags
);
```

## 📊 窗口属性

### 获取窗口信息

```cpp
std::cout << "Width: " << window->getWidth() << std::endl;
std::cout << "Height: " << window->getHeight() << std::endl;
std::cout << "Title: " << window->getTitle() << std::endl;
std::cout << "Fullscreen: " << window->isFullscreen() << std::endl;
```

### 设置窗口属性

```cpp
window->setTitle("New Title");
window->setSize(1024, 768);
window->setPosition(100, 100);
window->setFullscreen(true);
```

## 🎮 事件处理

### 窗口关闭事件

```cpp
app.onWindowClose([&]() {
    std::cout << "Window is closing!" << std::endl;
});
```

### 窗口大小改变事件

```cpp
app.onWindowResize([](int width, int height) {
    std::cout << "Window resized: " << width << "x" << height << std::endl;
});
```

### 键盘事件

```cpp
app.onKeyPress([](int key) {
    if (key == phoenix::Key::Escape) {
        std::cout << "Escape pressed!" << std::endl;
    }
});
```

## ⚠️ 常见错误

### 错误 1: 忘记包含头文件

```cpp
// ❌ 错误
#include <phoenix.hpp>

// ✅ 正确
#include <phoenix/phoenix.hpp>
```

### 错误 2: 忘记处理事件

```cpp
// ❌ 错误 - 窗口会无响应
while (app.isRunning()) {
    window->clear({0.0f, 0.0f, 0.0f, 1.0f});
    window->swapBuffers();
}

// ✅ 正确
while (app.isRunning()) {
    app.pollEvents();  // 必须调用
    window->clear({0.0f, 0.0f, 0.0f, 1.0f});
    window->swapBuffers();
}
```

### 错误 3: 窗口创建失败不检查

```cpp
// ❌ 错误
auto window = app.createWindow(800, 600, "Test");
// 直接使用 window，可能导致崩溃

// ✅ 正确
auto window = app.createWindow(800, 600, "Test");
if (!window) {
    std::cerr << "Failed to create window!" << std::endl;
    return -1;
}
```

## 🏃 练习

### 练习 1: 创建彩色窗口

创建一个程序，每秒改变窗口背景颜色：

```cpp
#include <phoenix/phoenix.hpp>
#include <chrono>

int main() {
    phoenix::Application app("Color Cycle");
    auto window = app.createWindow(800, 600);
    
    float time = 0.0f;
    
    while (app.isRunning()) {
        app.pollEvents();
        
        // 使用正弦波创建平滑的颜色循环
        time += 0.01f;
        float r = (std::sin(time) + 1.0f) / 2.0f;
        float g = (std::sin(time + 2.0f) + 1.0f) / 2.0f;
        float b = (std::sin(time + 4.0f) + 1.0f) / 2.0f;
        
        window->clear({r, g, b, 1.0f});
        window->swapBuffers();
    }
    
    return 0;
}
```

### 练习 2: 响应式窗口

创建一个窗口，根据窗口大小显示不同颜色：

```cpp
app.onWindowResize([&](int width, int height) {
    if (width > 1000) {
        window->clear({0.0f, 1.0f, 0.0f, 1.0f});  // 大窗口 - 绿色
    } else {
        window->clear({1.0f, 0.0f, 0.0f, 1.0f});  // 小窗口 - 红色
    }
});
```

## 📚 下一步

完成本课后，你应该能够：
- ✅ 创建 Phoenix Engine 应用程序
- ✅ 创建和配置窗口
- ✅ 处理基本窗口事件

**下一课**: [第二课：初始化渲染器](lesson-02-initialize-renderer.md)

---
*完整示例代码：examples/basic/01-first-window.cpp*
