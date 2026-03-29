# WASM Demo 重构计划

## 问题分析

根据审核报告，当前 WASM Demo 存在以下问题：

1. **架构问题**: demo-app.cpp 使用原生 WebGL (GLES3) 而非 Phoenix Engine API
2. **API 使用**: 6 个 Demo 中 0 个正确使用 Phoenix Engine WASM 模块
3. **回退机制**: 所有 Demo 回退到纯 WebGL/JavaScript 实现
4. **文件路径**: demo-wasm.js 中 WASM 文件路径错误 (已修复)

## 重构目标

1. ✅ 使用 Phoenix Engine API (bgfx 渲染后端)
2. ✅ 保持原有功能 (PBR 渲染、相机控制、光照、粒子系统)
3. ✅ 改进 WASM 加载失败错误提示
4. ✅ 确保跨平台兼容

## 重构方案

### 方案 A: 使用 Phoenix Engine C++ API (推荐)

**优点**:
- 与引擎架构一致
- 使用 bgfx 统一渲染后端
- 更好的性能优化

**实现**:
```cpp
#include <phoenix/core/Engine.hpp>
#include <phoenix/render/RenderDevice.hpp>
#include <phoenix/scene/Scene.hpp>

class PhoenixWasmDemo {
private:
    phoenix::Engine* engine;
    phoenix::scene::Scene* scene;
    
public:
    void init(int width, int height);
    void update(float dt);
    void render();
    void shutdown();
};
```

### 方案 B: 使用 bgfx 直接渲染

**优点**:
- 更轻量级
- 直接控制渲染管线

**实现**:
```cpp
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

class PhoenixWasmDemo {
private:
    bgfx::RendererType::Enum rendererType;
    
public:
    void init(int width, int height);
    void frame();
    void shutdown();
};
```

## 文件修改清单

### 1. demo-showcase/demo-app.cpp
- [ ] 移除原生 WebGL/GLES3 代码
- [ ] 使用 Phoenix Engine API 初始化
- [ ] 使用 bgfx 渲染后端
- [ ] 实现基础渲染循环
- [ ] 实现相机控制
- [ ] 实现简单场景
- [ ] 实现光照系统

### 2. demo-showcase/demo-wasm.js
- [ ] 使用 PhoenixEngine 类 (来自 phoenix-wasm-api.js)
- [ ] 改进 WASM 加载错误处理
- [ ] 添加明确的错误提示
- [ ] 保留 UI 控制面板集成
- [ ] 保留触摸手势支持

### 3. demo-showcase/index.html
- [ ] 确保正确引用 phoenix-engine.js
- [ ] 添加 WASM 加载失败回退 UI

## 验收标准

1. ✅ Demo 使用 Phoenix Engine API (非原生 WebGL)
2. ✅ 功能与原版一致 (PBR、光照、相机、粒子)
3. ✅ WASM 加载失败时有明确错误提示
4. ✅ 跨平台兼容 (Chrome/Firefox/Safari, iOS/Android)

## 时间估算

- 架构分析: 30 分钟
- demo-app.cpp 重写: 2 小时
- demo-wasm.js 修复: 1 小时
- 验证测试: 30 分钟
- **总计**: 4 小时
