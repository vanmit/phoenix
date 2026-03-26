# 🦄 Phoenix Engine - 跨平台三维渲染引擎

[![Release](https://img.shields.io/github/v/release/phoenix-engine/phoenix-engine?label=Release&color=6366f1)](https://github.com/phoenix-engine/phoenix-engine/releases)
[![License](https://img.shields.io/github/license/phoenix-engine/phoenix-engine?color=6366f1)](LICENSE)
[![CI/CD](https://img.shields.io/github/actions/workflow/status/phoenix-engine/phoenix-engine/release.yml?branch=main&label=CI/CD)](.github/workflows/release.yml)
[![Documentation](https://img.shields.io/badge/docs-latest-6366f1)](https://phoenix-engine.github.io)
[![Discord](https://img.shields.io/discord/phoenix-engine?label=Discord&color=7289da)](https://discord.gg/phoenix-engine)

**版本**: 1.0.0  
**状态**: ✅ 稳定发布  
**安全等级**: 军工级  

---

## 🚀 快速开始

### 安装

**Windows:**
```powershell
winget install PhoenixEngine
```

**Linux:**
```bash
# Ubuntu/Debian
sudo apt install phoenix-engine

# AppImage
wget https://github.com/phoenix-engine/phoenix-engine/releases/latest/download/PhoenixEngine.AppImage
chmod +x PhoenixEngine.AppImage
./PhoenixEngine.AppImage
```

**macOS:**
```bash
brew install phoenix-engine
```

**Web:**
```bash
npm install phoenix-engine
```

### 从源码构建

```bash
# 克隆项目
git clone https://github.com/phoenix-engine/phoenix-engine.git
cd phoenix-engine

# 构建
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)

# 运行示例
./examples/basic-render
```

---

## 📋 项目简介

Phoenix Engine 是一个军工级安全标准的跨平台三维渲染引擎，支持：

- **🌍 跨平台**: Windows/Linux/macOS/Android/iOS/Web
- **⚡ 双后端**: C++ 原生 + WebAssembly
- **🎨 多图形 API**: Vulkan/DX12/Metal/OpenGL/WebGPU/WebGL
- **🛡️ 安全核心**: Rust 内存安全 + 形式化验证
- **📦 全数据类型**: glTF/FBX/OBJ/点云/地形

---

## ✨ 核心特性

| 特性 | 描述 |
|------|------|
| 🎯 **PBR 渲染** | 基于物理的渲染，支持金属度/粗糙度工作流 |
| 💡 **全局光照** | IBL、光线追踪（可选）、阴影映射 |
| 🎮 **动画系统** | 骨骼动画、形态目标、动画混合 |
| 🗂️ **场景管理** | 场景图、空间加速、视锥剔除 |
| 📚 **资源系统** | 异步加载、热重载、资源池 |
| 🔒 **安全核心** | Rust 内存安全、AES-256 加密、审计日志 |
| 🌐 **WebAssembly** | 完整引擎编译为 WASM，支持 Web 部署 |

---

## 🏗️ 架构设计

```
┌─────────────────────────────────────────────────────────┐
│                  应用层 (Application)                    │
├─────────────────────────────────────────────────────────┤
│              安全核心层 (Rust - 内存安全)                │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │  安全分配器  │  │  审计日志    │  │  加密模块    │  │
│  └──────────────┘  └──────────────┘  └──────────────┘  │
├─────────────────────────────────────────────────────────┤
│              渲染抽象层 (bgfx - 跨平台)                  │
│  ┌─────┐ ┌─────┐ ┌─────┐ ┌──────┐ ┌──────┐ ┌────────┐ │
│  │Vulkan│ │Metal│ │DX12 │ │OpenGL│ │WebGL2│ │WebGPU  │ │
│  └─────┘ └─────┘ └─────┘ └──────┘ └──────┘ └────────┘ │
├─────────────────────────────────────────────────────────┤
│              场景管理层 (参考 VSG 设计)                  │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │  场景图      │  │  空间加速    │  │  剔除系统    │  │
│  └──────────────┘  └──────────────┘  └──────────────┘  │
└─────────────────────────────────────────────────────────┘
```

---

## 📦 平台支持

| 平台 | 状态 | 最低版本 | 安装包 |
|------|------|----------|--------|
| Windows | ✅ 稳定 | Windows 10 (1903+) | MSI/EXE/ZIP |
| Linux | ✅ 稳定 | Ubuntu 20.04+ | DEB/RPM/AppImage |
| macOS | ✅ 稳定 | macOS 11.0+ | DMG |
| Android | ✅ 稳定 | Android 10+ (API 29) | APK/AAB |
| iOS | ✅ 稳定 | iOS 15.0+ | IPA |
| Web | ✅ 稳定 | WebGPU 浏览器 | NPM |

---

## 🔧 技术栈

### 语言
- **C++17/20**: 性能关键模块
- **Rust 2021**: 安全核心模块
- **GLSL/HLSL/MSL**: 着色器
- **JavaScript/TypeScript**: Web bindings

### 图形 API
| API | 平台 | 状态 |
|-----|------|------|
| Vulkan | Windows/Linux/Android | ✅ 首选 |
| DirectX 12 | Windows | ✅ |
| Metal | macOS/iOS | ✅ |
| OpenGL 4.5 | 所有平台 | ✅ 备选 |
| WebGPU | Web | ✅ |
| WebGL 2.0 | Web | ✅ 备选 |

### 第三方库
| 库 | 用途 | 许可证 |
|----|------|--------|
| bgfx | 渲染抽象 | BSD-2 |
| assimp | 模型加载 | BSD-3 |
| stb_image | 纹理加载 | MIT |
| glm | 数学库 | MIT |
| libsodium | 加密 | ISC |
| wasmtime | WASM 运行时 | Apache-2 |

---

## 🛡️ 安全特性

- **内存安全**: Rust 核心 + C++ 智能指针
- **加密**: AES-256-GCM
- **审计日志**: 不可篡改操作记录
- **形式化验证**: 关键算法 Coq 证明
- **WASM 沙箱**: 隔离执行环境

---

## 📖 文档

- 📘 [在线文档](https://phoenix-engine.github.io)
- 📚 [API 参考](https://phoenix-engine.github.io/api)
- 🎓 [教程](https://phoenix-engine.github.io/tutorials)
- 💻 [示例代码](./examples)
- 📋 [贡献指南](CONTRIBUTING.md)

---

## 🤝 参与贡献

我们欢迎各种形式的贡献！

1. ⭐ Fork 本仓库
2. 🌿 创建特性分支 (`git checkout -b feature/amazing-feature`)
3. 💾 提交变更 (`git commit -m 'Add amazing feature'`)
4. 📤 推送到分支 (`git push origin feature/amazing-feature`)
5. 🔄 开启 Pull Request

详见 [CONTRIBUTING.md](CONTRIBUTING.md)

---

## 📄 许可证

本项目采用 MIT 许可证（第三方库遵循各自许可证）。

详见 [LICENSE](LICENSE)

---

## 📞 联系方式

- **项目主页**: https://github.com/phoenix-engine/phoenix-engine
- **问题反馈**: https://github.com/phoenix-engine/phoenix-engine/issues
- **讨论区**: https://github.com/phoenix-engine/phoenix-engine/discussions
- **Discord**: https://discord.gg/phoenix-engine
- **Twitter**: https://twitter.com/phoenixengine

---

## 🙏 致谢

Phoenix Engine 建立在以下优秀项目之上：

- [bgfx](https://github.com/bkaradzic/bgfx) - 跨平台渲染库
- [assimp](https://github.com/assimp/assimp) - 资产导入库
- [glm](https://github.com/g-truc/glm) - OpenGL 数学库
- [libsodium](https://libsodium.org/) - 加密库
- [wasmtime](https://wasmtime.dev/) - WebAssembly 运行时

---

**最后更新**: 2026-03-26 | **版本**: 1.0.0
