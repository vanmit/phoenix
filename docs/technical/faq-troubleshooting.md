# Phoenix Engine FAQ 与故障排除

## ❓ 常见问题

### 安装问题

**Q: CMake 找不到 Vulkan SDK**

```bash
# 解决方案
# Linux
export VULKAN_SDK=/path/to/vulkan-sdk

# Windows
set VULKAN_SDK=C:\VulkanSDK\1.3.250.0

# macOS (使用 MoltenVK)
brew install molten-vk
export VULKAN_SDK=$(brew --prefix molten-vk)
```

**Q: 编译错误 "undefined reference to vkCreateInstance"**

```bash
# 确保链接 Vulkan 库
# CMakeLists.txt
find_package(Vulkan REQUIRED)
target_link_libraries(your_app Vulkan::Vulkan)

# 或手动指定
target_link_libraries(your_app vulkan)
```

**Q: Android 构建失败 "NDK not found"**

```bash
# 设置 NDK 路径
export ANDROID_NDK=/path/to/android-ndk-r25

# 或在 CMake 中指定
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake
```

### 运行时问题

**Q: 应用启动后黑屏**

可能原因：
1. 显卡驱动过旧
2. Vulkan/Metal 不支持
3. 着色器编译失败

解决方案：
```bash
# 1. 更新显卡驱动

# 2. 检查图形 API 支持
vulkaninfo  # Linux
dxdiag      # Windows

# 3. 启用调试日志
export PHOENIX_LOG_LEVEL=debug
./your_app

# 4. 尝试回退到 OpenGL
export PHOENIX_RENDER_BACKEND=opengl
```

**Q: 帧率很低**

可能原因：
1. 垂直同步启用
2. 分辨率过高
3. 质量设置过高

解决方案：
```cpp
// 禁用垂直同步
config.vsync = false;

// 降低分辨率
config.renderScale = 0.75f;

// 降低质量
config.msaaSamples = 0;
config.shadows = false;
config.bloom = false;

// 限制帧率
config.maxFPS = 60;
```

**Q: 内存泄漏**

调试方法：
```cpp
// 启用内存跟踪
export PHOENIX_MEMORY_TRACKER=1

// 或使用 Valgrind (Linux)
valgrind --leak-check=full ./your_app

// 或使用 AddressSanitizer
cmake .. -DCMAKE_BUILD_TYPE=Debug \
         -DPHOENIX_ENABLE_ASAN=ON
```

### 渲染问题

**Q: 模型显示为黑色**

可能原因：
1. 法线方向错误
2. 光照未设置
3. 材质未正确配置

解决方案：
```cpp
// 1. 检查法线
// 确保法线指向正确方向

// 2. 添加光照
auto light = scene.createLight(LightType::Directional);
light->setColor({1.0f, 1.0f, 1.0f});
light->setIntensity(1.0f);

// 3. 检查材质
material->setAlbedoTexture(diffuseTexture);
material->setNormalTexture(normalTexture);
```

**Q: 纹理显示异常**

可能原因：
1. UV 坐标错误
2. 纹理格式不支持
3. 纹理未正确加载

解决方案：
```cpp
// 1. 检查 UV 坐标
// UV 范围应为 [0, 1]

// 2. 使用支持的格式
// PNG, JPG, DDS, KTX

// 3. 检查纹理加载
auto texture = renderer->loadTexture("path/to/texture.png");
if (!texture) {
    std::cerr << "Failed to load texture" << std::endl;
}

// 4. 检查纹理参数
texture->setWrapMode(WrapMode::Repeat);
texture->setFilterMode(FilterMode::Linear);
```

**Q: 阴影渲染不正确**

可能原因：
1. 阴影贴图分辨率过低
2. 阴影偏移设置不当
3. 光源配置错误

解决方案：
```cpp
// 1. 增加阴影贴图分辨率
config.shadowResolution = 2048;  // 或 4096

// 2. 调整阴影偏移
shadowConfig.bias = 0.005f;
shadowConfig.normalBias = 0.01f;

// 3. 检查光源
light->setCastShadows(true);
light->setShadowNear(0.1f);
light->setShadowFar(100.0f);
```

### 性能问题

**Q: CPU 使用率过高**

解决方案：
```cpp
// 1. 减少绘制调用
// 使用批处理

// 2. 使用多线程
// 并行录制命令缓冲

// 3. 降低更新频率
// 非关键对象降低更新频率

// 4. 分析性能
export PHOENIX_PROFILER=1
```

**Q: GPU 使用率过低**

可能原因：
1. CPU 瓶颈
2. 垂直同步限制
3. 场景过于简单

解决方案：
```cpp
// 1. 分析瓶颈
auto stats = renderer->getStats();
if (stats.cpuTime > stats.gpuTime) {
    // CPU 瓶颈
}

// 2. 禁用垂直同步 (测试用)
config.vsync = false;

// 3. 增加场景复杂度
// 添加更多对象、光照、效果
```

### 跨平台问题

**Q: Windows 上运行正常，Linux 上报错**

常见问题：
1. 路径分隔符
2. 大小写敏感
3. 依赖库缺失

解决方案：
```cpp
// 1. 使用跨平台路径
fs::path path = fs::path("folder") / "file.txt";

// 2. 注意大小写
// Linux 文件系统大小写敏感

// 3. 检查依赖
ldd ./your_app | grep "not found"
```

**Q: macOS 上 Metal 性能差**

解决方案：
```cpp
// 1. 使用 Metal 优化
config.metalOptimized = true;

// 2. 减少状态切换
// 批量绘制调用

// 3. 使用纹理压缩
// PVRTC 或 ASTC 格式
```

## 🔧 调试工具

### 1. 内置调试功能

```cpp
// 启用调试模式
config.debugMode = true;

// 启用验证层
config.enableValidation = true;

// 启用 GPU 调试
config.gpuDebug = true;
```

### 2. 日志系统

```bash
# 设置日志级别
export PHOENIX_LOG_LEVEL=debug    # debug, info, warning, error
export PHOENIX_LOG_FILE=app.log   # 日志文件
export PHOENIX_LOG_TO_CONSOLE=1   # 输出到控制台
```

### 3. 性能分析器

```cpp
// 启用性能分析
phoenix::Profiler::enable(true);

// 设置输出
phoenix::Profiler::setOutputFile("trace.json");

// 查看结果
// 使用 Chrome DevTools 打开 trace.json
```

### 4. 内存分析器

```cpp
// 启用内存跟踪
phoenix::MemoryTracker::enable(true);

// 获取报告
auto report = phoenix::MemoryTracker::getReport();
std::cout << "Total allocated: " << report.total << std::endl;
std::cout << "Peak usage: " << report.peak << std::endl;
```

## 🆘 获取帮助

### 1. 收集信息

报告问题时提供：
- 操作系统和版本
- 显卡型号和驱动版本
- Phoenix Engine 版本
- 错误日志
- 复现步骤

### 2. 日志位置

```bash
# Linux
~/.local/share/phoenix/logs/

# macOS
~/Library/Logs/Phoenix/

# Windows
%APPDATA%\Phoenix\logs\

# Android
/logcat | grep phoenix
```

### 3. 联系支持

- GitHub Issues: https://github.com/phoenix-engine/phoenix/issues
- Discussions: https://github.com/phoenix-engine/phoenix/discussions
- Email: support@phoenix-engine.dev

## 📋 故障排除检查清单

### 启动问题
- [ ] 检查系统要求
- [ ] 更新显卡驱动
- [ ] 验证 Vulkan/Metal 支持
- [ ] 检查依赖库
- [ ] 查看错误日志

### 渲染问题
- [ ] 检查着色器编译
- [ ] 验证纹理加载
- [ ] 检查光照设置
- [ ] 验证相机配置
- [ ] 检查渲染管线状态

### 性能问题
- [ ] 启用性能分析器
- [ ] 检查绘制调用数量
- [ ] 检查内存使用
- [ ] 检查 GPU 使用率
- [ ] 检查 CPU 使用率

### 崩溃问题
- [ ] 启用 AddressSanitizer
- [ ] 检查空指针
- [ ] 检查数组边界
- [ ] 检查资源生命周期
- [ ] 查看崩溃日志

---
*最后更新：2026-03-26*
