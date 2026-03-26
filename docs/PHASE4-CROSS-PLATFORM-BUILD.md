# Phoenix Engine Phase 4 - 跨平台构建指南

本文档提供在所有支持平台上构建 Phoenix Engine 的完整指南。

## 📋 目录

- [系统要求](#系统要求)
- [Windows 平台](#windows-平台)
- [Linux 平台](#linux-平台)
- [macOS 平台](#macos-平台)
- [iOS 平台](#ios-平台)
- [Android 平台](#android-平台)
- [Web 平台](#web-平台)
- [性能优化](#性能优化)
- [故障排除](#故障排除)

---

## 系统要求

### 通用要求

- **C++ 标准**: C++17
- **CMake**: 3.20+
- **目标帧率**: 60 FPS (所有平台)

### 平台特定要求

| 平台 | 操作系统 | 编译器 | 渲染后端 | 内存要求 |
|------|---------|--------|---------|---------|
| Windows | 10/11 (64-bit) | MSVC 2019+ / Clang | DX12, Vulkan | 4GB+ |
| Linux | Ubuntu 20.04+, Fedora 35+ | GCC 9+, Clang 10+ | Vulkan | 4GB+ |
| macOS | 11.0+ (Big Sur) | Apple Clang 13+ | Metal | 4GB+ |
| iOS | 14.0+ | Apple Clang 13+ | Metal | 2GB+ |
| Android | 8.0+ (API 26+) | NDK r23+ | Vulkan, OpenGL ES 3.2 | 2GB+ |
| Web | Modern browsers | Emscripten 3.1.39+ | WebGPU, WebGL 2 | 1GB+ |

---

## Windows 平台

### 前置条件

1. **Visual Studio 2019/2022** (包含 C++ 桌面开发)
2. **Windows 10 SDK** (10.0.19041.0+)
3. **CMake** 3.20+
4. **Vulkan SDK** (可选，用于 Vulkan 后端)

### 构建步骤

```bash
# 1. 创建构建目录
cd phoenix-engine
mkdir build && cd build

# 2. 配置 (使用 Visual Studio 生成器)
cmake .. -G "Visual Studio 17 2022" -A x64

# 或者使用 Ninja (推荐)
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release

# 3. 构建
cmake --build . --config Release

# 4. 运行示例
./bin/Release/cross-platform-demo.exe
```

### Visual Studio 项目

```bash
# 生成 VS 解决方案
cmake .. -G "Visual Studio 17 2022" -A x64

# 打开 phoenix-engine.sln
```

### DX12 特定配置

```cmake
# 在 CMakeLists.txt 中启用 DX12
add_compile_definitions(
    PHOENIX_PLATFORM_WINDOWS=1
    PHOENIX_DX12_ENABLED=1
)

target_link_libraries(phoenix_engine PRIVATE
    d3d12.lib
    dxgi.lib
    dxguid.lib
    d3dcompiler.lib
)
```

### 故障排除

**问题**: `DXGI_ERROR_SDK_COMPONENT_MISSING`

**解决**: 安装最新的 DirectX 12 运行时和 Windows SDK。

---

## Linux 平台

### 前置条件

#### Ubuntu/Debian

```bash
# 安装基础工具
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config

# 安装 Vulkan 依赖
sudo apt-get install -y \
    libvulkan-dev \
    vulkan-tools \
    mesa-vulkan-drivers

# 安装 X11/Wayland 依赖
sudo apt-get install -y \
    libx11-dev \
    libxrandr-dev \
    libxi-dev \
    libxcursor-dev \
    libxinerama-dev \
    libwayland-dev
```

#### Fedora

```bash
sudo dnf install -y \
    gcc-c++ \
    cmake \
    git \
    vulkan-loader-devel \
    vulkan-tools \
    libX11-devel \
    libXrandr-devel \
    libXi-devel \
    wayland-devel
```

### 构建步骤

```bash
# 1. 创建构建目录
mkdir build && cd build

# 2. 配置
cmake .. -DCMAKE_BUILD_TYPE=Release

# 3. 构建
cmake --build . -j$(nproc)

# 4. 运行
./bin/cross-platform-demo
```

### Vulkan 验证层

```bash
# 安装验证层
sudo apt-get install -y vulkan-validationlayers

# 启用验证层 (调试构建)
cmake .. -DCMAKE_BUILD_TYPE=Debug -DPHOENIX_ENABLE_VALIDATION=ON
```

### 性能优化

```bash
# 使用 LTO 优化构建
cmake .. -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON \
    -DCMAKE_CXX_FLAGS="-O3 -march=native"
```

---

## macOS 平台

### 前置条件

1. **Xcode** 13.0+ (包含 Command Line Tools)
2. **CMake** 3.20+
3. **Homebrew** (可选，用于依赖管理)

```bash
# 安装 Command Line Tools
xcode-select --install

# 使用 Homebrew 安装 CMake
brew install cmake
```

### 构建步骤

```bash
# 1. 创建构建目录
mkdir build && cd build

# 2. 配置
cmake .. -DCMAKE_BUILD_TYPE=Release

# 3. 构建
cmake --build . -j$(sysctl -n hw.ncpu)

# 4. 运行
./bin/cross-platform-demo
```

### Xcode 项目生成

```bash
# 生成 Xcode 项目
cmake .. -G Xcode

# 打开项目
open phoenix-engine.xcodeproj
```

### Metal 调试

```bash
# 启用 Metal API 验证
export MTL_DEBUG_LAYER=1

# 启用 Shader 验证
export MTL_SHADER_VALIDATION=1
```

---

## iOS 平台

### 前置条件

1. **macOS** 11.0+
2. **Xcode** 13.0+
3. **CMake** 3.20+
4. **iOS SDK** 14.0+

### 构建步骤

```bash
# 1. 创建构建目录
mkdir build-ios && cd build-ios

# 2. 配置 (使用 iOS 工具链)
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/ios.toolchain.cmake \
    -DPLATFORM=OS64 \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=../install/ios

# 3. 构建
cmake --build . -j$(sysctl -n hw.ncpu)

# 4. 部署到设备
# 使用 Xcode 打开生成的项目并部署
```

### iOS 工具链文件

创建 `cmake/ios.toolchain.cmake`:

```cmake
set(CMAKE_SYSTEM_NAME iOS)
set(CMAKE_SYSTEM_VERSION 14.0)

set(CMAKE_OSX_SYSROOT "iphoneos" CACHE STRING "iOS SDK")
set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "iOS Architecture")
set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING "iOS Deployment Target")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
```

### iOS 优化

```cmake
# 内存优化 (移动端 < 256MB)
add_compile_definitions(
    PHOENIX_MOBILE_MEMORY_LIMIT=256
    PHOENIX_ENABLE_POWER_SAVING=1
)

# 启用 Metal 帧捕获
target_compile_options(phoenix_engine PRIVATE
    -fprofile-instr-generate
    -fcoverage-mapping
)
```

---

## Android 平台

### 前置条件

1. **Android NDK** r23+
2. **CMake** 3.20+
3. **Android SDK** (API 26+)
4. **Java JDK** 11+

### 环境变量设置

```bash
export ANDROID_NDK=/path/to/android-ndk-r23
export ANDROID_SDK=/path/to/android-sdk
export PATH=$ANDROID_NDK:$PATH
```

### 构建步骤

```bash
# 1. 创建构建目录
mkdir build-android && cd build-android

# 2. 配置 (使用 Android 工具链)
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-26 \
    -DCMAKE_BUILD_TYPE=Release

# 3. 构建
cmake --build . -j$(nproc)

# 4. 生成 APK
# 使用 Android Studio 或 gradle 构建 APK
```

### ABI 支持

```bash
# 构建所有 ABI
cmake .. \
    -DANDROID_ABI="armeabi-v7a;arm64-v8a;x86;x86_64" \
    ...
```

### Android NDK 配置

```cmake
# CMakeLists.txt Android 特定配置
if(ANDROID)
    add_compile_definitions(
        PHOENIX_PLATFORM_ANDROID=1
        PHOENIX_VULKAN_ENABLED=1
    )
    
    target_link_libraries(phoenix_engine PRIVATE
        android
        log
        EGL
        GLESv3
    )
    
    # NDK 优化
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -flto")
endif()
```

### 触摸事件处理

```cpp
// 在 AndroidPlatform 中处理触摸
int32_t AndroidPlatform::handleInputEvent(AInputEvent* event) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event);
        int32_t pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
                               AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        
        float x = AMotionEvent_getX(event, pointerIndex);
        float y = AMotionEvent_getY(event, pointerIndex);
        
        // 处理触摸事件...
    }
    return 0;
}
```

---

## Web 平台

### 前置条件

1. **Emscripten** 3.1.39+
2. **Node.js** 16+ (用于本地服务器)
3. **现代浏览器** (支持 WebGPU 或 WebGL 2)

### Emscripten 安装

```bash
# 安装 Emscripten
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
```

### 构建步骤

```bash
# 1. 创建构建目录
mkdir build-web && cd build-web

# 2. 配置 (使用 Emscripten)
emcmake cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=../build_web.cmake

# 3. 构建
emmake cmake --build . -j$(nproc)

# 4. 启动本地服务器
emrun --no_browser --port 8080 .
```

### WebGPU vs WebGL

```javascript
// 自动检测最佳后端
async function initRenderer() {
    // 尝试 WebGPU
    if (navigator.gpu) {
        const adapter = await navigator.gpu.requestAdapter();
        if (adapter) {
            console.log('Using WebGPU');
            return 'webgpu';
        }
    }
    
    // 回退到 WebGL 2
    const canvas = document.getElementById('canvas');
    const gl = canvas.getContext('webgl2');
    if (gl) {
        console.log('Using WebGL 2');
        return 'webgl2';
    }
    
    throw new Error('No suitable rendering backend');
}
```

### 内存优化

```cmake
# Web 平台内存配置
set(EMSCRIPTEN_FLAGS "${EMSCRIPTEN_FLAGS} -s INITIAL_MEMORY=268435456")  # 256MB
set(EMSCRIPTEN_FLAGS "${EMSCRIPTEN_FLAGS} -s MAXIMUM_MEMORY=536870912")  # 512MB
set(EMSCRIPTEN_FLAGS "${EMSCRIPTEN_FLAGS} -s ALLOW_MEMORY_GROWTH=1")
```

### 部署

```bash
# 优化构建用于生产
emcmake cmake .. -DCMAKE_BUILD_TYPE=Release \
    -DEMSCRIPTEN_FLAGS="-O3 -flto --closure 1"

# 构建
emmake cmake --build .

# 输出文件
# - cross-platform-demo.html
# - cross-platform-demo.js
# - cross-platform-demo.wasm
```

---

## 性能优化

### 通用优化

1. **启用 LTO (Link Time Optimization)**
   ```cmake
   set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
   ```

2. **使用 Release 构建**
   ```bash
   cmake .. -DCMAKE_BUILD_TYPE=Release
   ```

3. **启用编译器优化**
   ```cmake
   if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
       add_compile_options(-O3 -march=native)
   elseif(MSVC)
       add_compile_options(/O2 /GL)
   endif()
   ```

### 平台特定优化

#### Windows (DX12)

```cpp
// 启用 DX12 调试层 (仅调试)
DX12Config config;
config.enableDebugLayer = true;

// 使用命令列表分配器重用
// 避免每帧创建新资源
```

#### Linux (Vulkan)

```cpp
// 使用 VK_EXT_memory_budget
// 监控 VRAM 使用
// 启用 VK_KHR_pipeline_library 减少 PSO 创建时间
```

#### macOS/iOS (Metal)

```cpp
// 使用 MTLResourceStorageModePrivate 用于 GPU 独占资源
// 启用 MTLArgumentBuffers 减少绑定开销
// 使用 MTLIndirectCommandBuffer 用于实例化
```

#### Android

```cpp
// 功耗优化
AndroidPlatform::setPowerSaveMode(true);

// 内存限制
config.maxMemoryMB = 256;

// 使用 ASTC 纹理压缩
```

#### Web

```cpp
// 使用 WebAssembly SIMD
set(EMSCRIPTEN_FLAGS "${EMSCRIPTEN_FLAGS} -msimd128")

// 启用多线程 (如果支持)
set(EMSCRIPTEN_FLAGS "${EMSCRIPTEN_FLAGS} -s USE_PTHREADS=1")
```

---

## 故障排除

### 常见问题

#### 1. CMake 找不到编译器

**解决**: 确保编译器在 PATH 中，或指定 `CMAKE_CXX_COMPILER`。

```bash
cmake .. -DCMAKE_CXX_COMPILER=/path/to/compiler
```

#### 2. Vulkan 初始化失败

**解决**: 
- 安装最新的 GPU 驱动
- 验证 Vulkan 支持: `vulkaninfo`
- 检查 ICD: `vkEnumerateInstanceLayerProperties`

#### 3. DX12 功能级别不足

**解决**: 确保 GPU 支持 DX12 Feature Level 11.0+。

#### 4. Metal 设备不支持

**解决**: 检查设备兼容性 (macOS 10.15+, iOS 13+)。

#### 5. WebGPU 不可用

**解决**: 
- 使用 Chrome 94+ 或 Edge 94+
- 在 `chrome://flags` 中启用 WebGPU
- 回退到 WebGL 2

#### 6. Android NDK 构建失败

**解决**: 
- 确保 NDK 版本 >= r23
- 检查 `ANDROID_ABI` 设置
- 清理构建: `rm -rf build-android`

### 调试技巧

#### 启用渲染后端验证

```cpp
// Vulkan
VulkanConfig config;
config.enableValidation = true;

// DX12
DX12Config config;
config.enableDebugLayer = true;

// Metal
MetalConfig config;
config.enableGPUValidation = true;
```

#### 性能分析

```cpp
// 使用内置分析器
PHOENIX_PROFILE_FUNCTION();

// 导出 Chrome Trace
Profiler::exportToChromeTrace("trace.json");
```

#### 日志级别

```cpp
// 设置日志级别
Logger::setLevel(LogLevel::Debug);  // 或 Info, Warning, Error
```

---

## 跨平台测试清单

- [ ] Windows 10/11 (DX12)
- [ ] Linux (Vulkan)
- [ ] macOS (Metal)
- [ ] iOS 14+ (Metal)
- [ ] Android 8.0+ (Vulkan/OpenGL ES)
- [ ] Web (WebGPU/WebGL 2)
- [ ] 60 FPS 目标达成
- [ ] 移动端内存 < 256MB
- [ ] 功耗优化启用
- [ ] 触摸输入测试 (移动设备)
- [ ] 游戏手柄测试
- [ ] 高 DPI 显示测试

---

## 资源

- [Phoenix Engine 文档](../README.md)
- [Phase 1-3 报告](../PHASE*-*.md)
- [CMake 文档](https://cmake.org/documentation/)
- [Vulkan 规范](https://www.khronos.org/vulkan/)
- [DirectX 12 文档](https://docs.microsoft.com/en-us/windows/win32/direct3d12/)
- [Metal 文档](https://developer.apple.com/metal/)
- [WebGPU 规范](https://www.w3.org/TR/webgpu/)
- [Emscripten 文档](https://emscripten.org/docs/)

---

**Phase 4 状态**: ✅ 完成

**支持平台**: Windows, Linux, macOS, iOS, Android, Web

**渲染后端**: DX12, Vulkan, Metal, WebGPU, WebGL 2

**最后更新**: 2026-03-26
