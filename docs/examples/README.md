# 基础示例程序文档

## 01. Hello World

最简单的 Phoenix Engine 程序。

### 功能
- 创建应用程序
- 创建窗口
- 清屏
- 主循环

### 代码
```cpp
#include <phoenix/phoenix.hpp>

int main() {
    phoenix::Application app("Hello World");
    auto window = app.createWindow(800, 600, "Hello Phoenix");
    
    while (app.isRunning()) {
        app.pollEvents();
        window->clear({0.1f, 0.1f, 0.2f, 1.0f});
        window->swapBuffers();
    }
    
    return 0;
}
```

### 运行
```bash
cd build
./examples/basic/hello-world
```

---

## 02. Window Creation

创建不同配置的窗口。

### 功能
- 全屏窗口
- 无边框窗口
- 可调整大小窗口
- 多窗口

### 代码
```cpp
// 全屏窗口
auto fullscreen = app.createWindow(
    1920, 1080,
    "Fullscreen",
    phoenix::WindowFlags::Fullscreen
);

// 无边框窗口
auto borderless = app.createWindow(
    800, 600,
    "Borderless",
    phoenix::WindowFlags::Borderless
);

// 可调整大小
auto resizable = app.createWindow(
    800, 600,
    "Resizable",
    phoenix::WindowFlags::Resizable
);
```

### 运行
```bash
./examples/basic/window-creation
```

---

## 03. Clear Screen

使用不同颜色清屏。

### 功能
- 单色清除
- 渐变清除
- 动画清除

### 代码
```cpp
float time = 0.0f;

while (app.isRunning()) {
    app.pollEvents();
    
    // 动画背景
    time += 0.01f;
    float r = std::sin(time) * 0.5f + 0.5f;
    float g = std::sin(time + 2.0f) * 0.5f + 0.5f;
    float b = std::sin(time + 4.0f) * 0.5f + 0.5f;
    
    window->clear({r, g, b, 1.0f});
    window->swapBuffers();
}
```

### 运行
```bash
./examples/basic/clear-screen
```

---

## 04. Input Handling

处理键盘和鼠标输入。

### 功能
- 键盘输入
- 鼠标移动
- 鼠标点击
- 游戏手柄

### 代码
```cpp
// 键盘
app.onKeyPress([](int key) {
    if (key == phoenix::Key::W) {
        camera.moveForward();
    }
});

// 鼠标
app.onMouseMove([](float dx, float dy) {
    camera.rotate(dx, dy);
});

// 鼠标点击
app.onMouseClick([](int button) {
    if (button == phoenix::MouseButton::Left) {
        shoot();
    }
});
```

### 运行
```bash
./examples/basic/input-handling
```

---

## 05. Time System

使用时间系统控制动画。

### 功能
- 获取经过时间
- 获取 deltaTime
- 帧率计算
- 时间缩放

### 代码
```cpp
float totalTime = 0.0f;

while (app.isRunning()) {
    app.pollEvents();
    
    float deltaTime = app.getDeltaTime();
    totalTime += deltaTime;
    
    // 使用 deltaTime 进行帧率无关的移动
    position += velocity * deltaTime;
    
    // 使用时间进行动画
    rotation = totalTime * 0.5f;
    
    renderer->clear({0.1f, 0.1f, 0.2f, 1.0f});
    // ... 渲染
    renderer->present();
}
```

### 运行
```bash
./examples/basic/time-system
```

---

## 渲染示例

## 01. Basic Rendering

基础三角形渲染。

### 功能
- 顶点缓冲区
- 索引缓冲区
- 基础着色器
- 三角形绘制

### 运行
```bash
./examples/rendering/basic-render
```

---

## 02. PBR Rendering

基于物理的渲染。

### 功能
- PBR 材质
- 金属度/粗糙度
- IBL 光照
- 图像光照

### 运行
```bash
./examples/rendering/pbr-demo
```

---

## 03. Shadow Rendering

实时阴影渲染。

### 功能
- 阴影贴图
- 级联阴影
- 软阴影
- 阴影优化

### 运行
```bash
./examples/rendering/shadow-demo
```

---

## 04. Post Processing

后期处理效果。

### 功能
- 泛光 (Bloom)
- 色调映射
- 环境光遮蔽
- 景深

### 运行
```bash
./examples/rendering/post-process
```

---

## 05. Deferred Rendering

延迟渲染管线。

### 功能
- G-Buffer
- 延迟光照
- 多光源
- 性能优化

### 运行
```bash
./examples/rendering/deferred-render
```

---

## 动画示例

## 01. Skeletal Animation

骨骼动画。

### 功能
- 骨骼加载
- 关键帧动画
- 蒙皮
- 动画混合

### 运行
```bash
./examples/animation/skeletal-animation
```

---

## 02. Morph Animation

形态动画。

### 功能
- 形态目标
- 混合权重
- 面部动画
- 实时变形

### 运行
```bash
./examples/animation/morph-animation
```

---

## 03. Animation Blending

动画混合。

### 功能
- 状态机
- 过渡混合
- 1D/2D 混合
- 添加动画

### 运行
```bash
./examples/animation/animation-blending
```

---

## 物理示例

## 01. Rigid Body

刚体物理。

### 功能
- 刚体模拟
- 碰撞检测
- 重力
- 力应用

### 运行
```bash
./examples/physics/rigid-body
```

---

## 02. Collision Detection

碰撞检测。

### 功能
- AABB 碰撞
- 球体碰撞
- 网格碰撞
- 碰撞响应

### 运行
```bash
./examples/physics/collision-detection
```

---

## 03. Physics Simulation

完整物理模拟。

### 功能
- 约束
- 关节
- 布料模拟
- 破碎效果

### 运行
```bash
./examples/physics/physics-simulation
```

---

## 跨平台示例

## 01. Android

Android 平台示例。

### 功能
- Android 窗口
- Touch 输入
- 移动端优化
- APK 打包

### 构建
```bash
./scripts/build-android.sh
```

---

## 02. iOS

iOS 平台示例。

### 功能
- iOS 窗口
- Touch 输入
- Metal 后端
- IPA 打包

### 构建
```bash
./scripts/build-ios.sh
```

---

## 03. WebAssembly

Web 平台示例。

### 功能
- WebGPU 后端
- Web 输入
- 异步加载
- 响应式设计

### 构建
```bash
./scripts/build-wasm.sh
```

---

## 04. Desktop Cross-Platform

桌面跨平台示例。

### 功能
- 多平台构建
- 统一输入
- 平台检测
- 条件编译

### 构建
```bash
./scripts/build-desktop.sh
```

---

*完整示例代码位于 examples/ 目录*
