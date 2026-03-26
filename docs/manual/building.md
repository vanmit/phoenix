# Phoenix Engine 构建指南

从源码构建 Phoenix Engine 的详细指南。

## 📋 构建选项

### CMake 选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `PHOENIX_BUILD_EXAMPLES` | ON | 构建示例程序 |
| `PHOENIX_BUILD_TESTS` | ON | 构建测试套件 |
| `PHOENIX_BUILD_DOCS` | OFF | 构建文档 |
| `PHOENIX_ENABLE_VULKAN` | ON | 启用 Vulkan 后端 |
| `PHOENIX_ENABLE_METAL` | ON | 启用 Metal 后端 |
| `PHOENIX_ENABLE_DX12` | ON | 启用 DX12 后端 |
| `PHOENIX_ENABLE_WEBGPU` | ON | 启用 WebGPU 后端 |
| `PHOENIX_ENABLE_OPENGL` | OFF | 启用 OpenGL 后端 (兼容模式) |
| `PHOENIX_ENABLE_PROFILER` | ON | 启用性能分析器 |
| `PHOENIX_ENABLE_TRACING` | OFF | 启用追踪功能 |
| `PHOENIX_OPTIMIZE_FOR_MOBILE` | OFF | 移动端优化 |

## 🔨 标准构建

### Linux/macOS

```bash
# 1. 创建构建目录
mkdir build && cd build

# 2. 配置
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DPHOENIX_BUILD_EXAMPLES=ON \
    -DPHOENIX_BUILD_TESTS=ON \
    -DPHOENIX_ENABLE_PROFILER=ON

# 3. 构建
make -j$(nproc)

# 4. 测试
ctest --output-on-failure

# 5. 安装 (可选)
sudo make install
```

### Windows (Visual Studio)

```powershell
# 1. 创建构建目录
mkdir build && cd build

# 2. 配置
cmake .. `
    -G "Visual Studio 17 2022" `
    -DCMAKE_BUILD_TYPE=Release `
    -DPHOENIX_BUILD_EXAMPLES=ON `
    -DPHOENIX_BUILD_TESTS=ON

# 3. 构建
cmake --build . --config Release

# 4. 测试
ctest -C Release --output-on-failure
```

### Windows (MinGW)

```bash
mkdir build && cd build
cmake .. \
    -G "MinGW Makefiles" \
    -DCMAKE_BUILD_TYPE=Release \
    -DPHOENIX_BUILD_EXAMPLES=ON
make -j$(nproc)
```

## 📱 移动端构建

### Android

```bash
# 设置环境变量
export ANDROID_NDK=/path/to/android-ndk-r25
export ANDROID_SDK=/path/to/android-sdk

# 构建 arm64-v8a
mkdir build-android && cd build-android
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-26 \
    -DCMAKE_BUILD_TYPE=Release \
    -DPHOENIX_OPTIMIZE_FOR_MOBILE=ON

make -j$(nproc)

# 构建 armeabi-v7a (32 位)
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=armeabi-v7a \
    -DANDROID_PLATFORM=android-24 \
    -DCMAKE_BUILD_TYPE=Release

make -j$(nproc)
```

### iOS

```bash
# 构建 iOS 64 位
mkdir build-ios && cd build-ios
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/ios.toolchain.cmake \
    -DPLATFORM=OS64 \
    -DCMAKE_BUILD_TYPE=Release \
    -DPHOENIX_OPTIMIZE_FOR_MOBILE=ON

make -j$(sysctl -n hw.ncpu)

# 构建 iOS Simulator
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/ios.toolchain.cmake \
    -DPLATFORM=SIMULATOR64 \
    -DCMAKE_BUILD_TYPE=Release

make -j$(sysctl -n hw.ncpu)
```

## 🌐 WebAssembly 构建

```bash
# 激活 Emscripten
source /path/to/emsdk/emsdk_env.sh

# 配置
mkdir build-wasm && cd build-wasm
emcmake cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DPHOENIX_ENABLE_WEBGPU=ON \
    -DPHOENIX_ENABLE_VULKAN=OFF \
    -DPHOENIX_ENABLE_METAL=OFF \
    -DPHOENIX_ENABLE_DX12=OFF

# 构建
emmake make -j$(nproc)

# 优化构建 (减小文件大小)
emmake make -j$(nproc) \
    CFLAGS="-Oz" \
    LDFLAGS="-s WASM=1 -s ALLOW_MEMORY_GROWTH=1"
```

## ⚡ 优化构建

### 发布构建 (最大优化)

```bash
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_FLAGS_RELEASE="-O3 -DNDEBUG" \
    -DCMAKE_CXX_FLAGS_RELEASE="-O3 -DNDEBUG" \
    -DPHOENIX_ENABLE_TRACING=OFF \
    -DPHOENIX_ENABLE_PROFILER=OFF
```

### 调试构建

```bash
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_C_FLAGS_DEBUG="-g -O0 -DDEBUG" \
    -DCMAKE_CXX_FLAGS_DEBUG="-g -O0 -DDEBUG" \
    -DPHOENIX_ENABLE_TRACING=ON \
    -DPHOENIX_ENABLE_PROFILER=ON
```

### 性能分析构建

```bash
cmake .. \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_C_FLAGS_RELWITHDEBINFO="-O2 -g -DNDEBUG" \
    -DCMAKE_CXX_FLAGS_RELWITHDEBINFO="-O2 -g -DNDEBUG" \
    -DPHOENIX_ENABLE_PROFILER=ON \
    -DPHOENIX_ENABLE_TRACING=ON
```

## 🧪 测试

### 运行所有测试

```bash
cd build
ctest --output-on-failure
```

### 运行特定测试

```bash
# 运行渲染测试
ctest -R Render --output-on-failure

# 运行场景测试
ctest -R Scene --output-on-failure

# 运行性能测试
ctest -R Performance --output-on-failure
```

### 生成测试覆盖率

```bash
# 需要 gcov 和 lcov
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DPHOENIX_ENABLE_COVERAGE=ON

make -j$(nproc)
make coverage

# 查看报告
firefox coverage/index.html
```

## 📦 打包

### 创建安装包

```bash
# Linux
cpack -G DEB
cpack -G RPM

# macOS
cpack -G DragNDrop

# Windows
cpack -G NSIS
cpack -G ZIP
```

### 自定义打包配置

```cmake
# CMakeLists.txt
set(CPACK_PACKAGE_NAME "Phoenix Engine")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_VENDOR "Phoenix Engine Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "High Performance 3D Rendering Engine")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Phoenix Engine")
```

## 🔍 构建故障排除

### 常见问题

**CMake 配置失败**
```bash
# 清理构建目录
rm -rf build
mkdir build && cd build

# 重新配置，显示详细信息
cmake .. -DCMAKE_BUILD_TYPE=Release --debug-output
```

**编译错误**
```bash
# 检查编译器版本
gcc --version
clang --version

# 确保使用支持的版本
# GCC 9+, Clang 10+, MSVC 2019+
```

**链接错误**
```bash
# 检查依赖库
ldd ./examples/basic/hello-world  # Linux
otool -L ./examples/basic/hello-world  # macOS

# 重新构建依赖
make clean
make -j$(nproc)
```

**内存不足**
```bash
# 减少并行构建任务数
make -j2

# 或增加交换空间
sudo fallocate -l 4G /swapfile
sudo chmod 600 /swapfile
sudo mkswap /swapfile
sudo swapon /swapfile
```

## 📊 构建性能

### 典型构建时间

| 配置 | 时间 | 系统 |
|------|------|------|
| Debug | 5-10 分钟 | Ryzen 7 5800X, 32GB RAM |
| Release | 3-5 分钟 | Ryzen 7 5800X, 32GB RAM |
| Android | 8-15 分钟 | M1 Pro, 16GB RAM |
| iOS | 5-10 分钟 | M1 Pro, 16GB RAM |
| WebAssembly | 10-20 分钟 | Intel i7, 16GB RAM |

### 优化构建速度

```bash
# 使用 ccache
sudo apt-get install ccache
export CC="ccache gcc"
export CXX="ccache g++"

# 使用 ninja (比 make 快 20-30%)
cmake .. -G Ninja
ninja -j$(nproc)

# 预编译头文件
cmake .. -DPHOENIX_USE_PCH=ON
```

---
*最后更新：2026-03-26*
