# Phoenix Engine 安装指南

详细的安装说明，支持所有平台。

## 📋 系统要求

### 最低要求

| 组件 | 要求 |
|------|------|
| 操作系统 | Windows 10 / macOS 10.15 / Linux (Ubuntu 20.04) / Android 8.0 / iOS 13 |
| CPU | 4 核心，2.0 GHz |
| 内存 | 4 GB RAM |
| 显卡 | Vulkan 1.2 / Metal / DX12 支持 |
| 存储 | 2 GB 可用空间 |

### 推荐配置

| 组件 | 要求 |
|------|------|
| 操作系统 | Windows 11 / macOS 12+ / Linux (Ubuntu 22.04) |
| CPU | 8 核心，3.0 GHz+ |
| 内存 | 16 GB RAM |
| 显卡 | 独立显卡，8GB VRAM |
| 存储 | SSD，5 GB 可用空间 |

---

## 🐧 Linux 安装

### Ubuntu/Debian

```bash
# 1. 安装系统依赖
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    libvulkan-dev \
    libglfw3-dev \
    libglm-dev \
    libassimp-dev \
    libpng-dev \
    libjpeg-dev \
    libfreetype6-dev \
    libx11-dev \
    libxcursor-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxi-dev

# 2. 克隆仓库
git clone https://github.com/phoenix-engine/phoenix.git
cd phoenix

# 3. 构建
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# 4. 安装 (可选)
sudo make install
```

### Fedora/RHEL

```bash
sudo dnf install -y \
    gcc-c++ \
    cmake \
    git \
    vulkan-devel \
    vulkan-loader-devel \
    glfw-devel \
    glm-devel \
    assimp-devel \
    libpng-devel \
    libjpeg-turbo-devel \
    freetype-devel
```

### Arch Linux

```bash
sudo pacman -S \
    base-devel \
    cmake \
    git \
    vulkan-icd-loader \
    glfw-x11 \
    glm \
    assimp \
    libpng \
    libjpeg-turbo \
    freetype2
```

---

## 🍎 macOS 安装

### 使用 Homebrew

```bash
# 1. 安装 Homebrew (如果未安装)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 2. 安装依赖
brew install cmake git glm assimp libpng jpeg freetype

# 3. 克隆仓库
git clone https://github.com/phoenix-engine/phoenix.git
cd phoenix

# 4. 构建
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(sysctl -n hw.ncpu)
```

### 使用 Metal

Phoenix Engine 在 macOS 上自动使用 Metal 后端，无需额外配置。

---

## 🪟 Windows 安装

### 使用 Visual Studio

```powershell
# 1. 安装 Visual Studio 2019/2022
# 确保安装 "使用 C++ 的桌面开发" 工作负载

# 2. 安装 Vulkan SDK
# 从 https://vulkan.lunarg.com/ 下载并安装

# 3. 使用 Git 克隆
git clone https://github.com/phoenix-engine/phoenix.git
cd phoenix

# 4. 使用 CMake GUI 或命令行
mkdir build && cd build
cmake -G "Visual Studio 16 2019" ..
# 或
cmake -G "Visual Studio 17 2022" ..

# 5. 构建
cmake --build . --config Release
```

### 使用 MSYS2

```bash
# 1. 安装 MSYS2
# 从 https://www.msys2.org/ 下载

# 2. 安装依赖
pacman -S \
    mingw-w64-x86_64-toolchain \
    mingw-w64-x86_64-cmake \
    mingw-w64-x86_64-git \
    mingw-w64-x86_64-vulkan-loader \
    mingw-w64-x86_64-glfw \
    mingw-w64-x86_64-glm \
    mingw-w64-x86_64-assimp

# 3. 构建
mkdir build && cd build
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

---

## 🤖 Android 安装

### 前置要求

- Android Studio Arctic Fox+
- NDK r21+
- CMake 3.16+

### 步骤

```bash
# 1. 安装 Android Studio 和 NDK

# 2. 克隆仓库
git clone https://github.com/phoenix-engine/phoenix.git
cd phoenix

# 3. 打开 Android 项目
# 在 Android Studio 中打开 examples/android/

# 4. 配置 NDK 路径
# File > Settings > Build, Execution, Deployment > CMake
# 设置 NDK 路径

# 5. 构建并运行
# 点击 Run 按钮
```

### 手动构建

```bash
# 设置环境变量
export ANDROID_NDK=/path/to/ndk
export ANDROID_SDK=/path/to/sdk

# 构建
mkdir build-android && cd build-android
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-26 \
    -DCMAKE_BUILD_TYPE=Release

make -j$(nproc)
```

---

## 🍎 iOS 安装

### 前置要求

- macOS 12+
- Xcode 14+
- CMake 3.16+

### 步骤

```bash
# 1. 安装 Xcode Command Line Tools
xcode-select --install

# 2. 安装依赖
brew install cmake git

# 3. 克隆仓库
git clone https://github.com/phoenix-engine/phoenix.git
cd phoenix

# 4. 构建 iOS 版本
mkdir build-ios && cd build-ios
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/ios.toolchain.cmake \
    -DPLATFORM=OS64 \
    -DCMAKE_BUILD_TYPE=Release

make -j$(sysctl -n hw.ncpu)
```

### 在 Xcode 中使用

1. 打开 `examples/ios/PhoenixDemo.xcodeproj`
2. 选择目标设备
3. 点击 Run

---

## 🌐 WebAssembly 安装

### 前置要求

- Emscripten SDK
- Node.js 16+

### 步骤

```bash
# 1. 安装 Emscripten
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh

# 2. 克隆 Phoenix Engine
git clone https://github.com/phoenix-engine/phoenix.git
cd phoenix

# 3. 构建 WebAssembly
mkdir build-wasm && cd build-wasm
emcmake cmake .. -DCMAKE_BUILD_TYPE=Release
emmake make -j$(nproc)

# 4. 运行
emrun --no_browser --port 8080 .
# 或
python3 -m http.server 8080
# 访问 http://localhost:8080
```

---

## ✅ 验证安装

### 运行测试

```bash
cd build
ctest --output-on-failure
```

### 运行示例

```bash
# 基础示例
./examples/basic/hello-world

# 渲染示例
./examples/rendering/basic-render

# 动画示例
./examples/animation/animation-demo
```

### 检查版本

```bash
./examples/basic/hello-world --version
# 输出：Phoenix Engine v1.0.0
```

---

## 🔧 故障排除

### 常见问题

**CMake 找不到 Vulkan**
```bash
# Linux
export VULKAN_SDK=/path/to/vulkan-sdk

# Windows
set VULKAN_SDK=C:\VulkanSDK\1.3.250.0
```

**编译错误：找不到头文件**
```bash
# 检查依赖是否安装完整
# Linux
sudo apt-get install libvulkan-dev libglfw3-dev libglm-dev

# macOS
brew install vulkan-loader glfw glm
```

**链接错误：未定义的引用**
```bash
# 确保链接正确的库
cmake .. -DCMAKE_BUILD_TYPE=Release
make clean
make -j$(nproc)
```

---

## 📞 获取帮助

- 📖 [快速入门](quickstart.md)
- 📖 [构建指南](building.md)
- 💬 [GitHub Discussions](https://github.com/phoenix-engine/phoenix/discussions)
- 🐛 [报告问题](https://github.com/phoenix-engine/phoenix/issues)

---
*最后更新：2026-03-26*
