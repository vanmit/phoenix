# 🦄 Phoenix Engine - 跨平台三维渲染引擎

**版本**: 0.1.0-alpha  
**状态**: 开发中  
**安全等级**: 军工级  

---

## 🚀 快速开始

```bash
# 克隆项目
git clone https://github.com/your-org/phoenix-engine.git
cd phoenix-engine

# 构建
mkdir build && cd build
cmake ..
cmake --build .

# 运行示例
./examples/basic-render
```

---

## 📋 项目简介

Phoenix Engine 是一个军工级安全标准的跨平台三维渲染引擎，支持：

- **跨平台**: Windows/Linux/macOS/Android/iOS/Web
- **双后端**: C++ 原生 + WebAssembly
- **多图形 API**: Vulkan/DX12/Metal/OpenGL/WebGPU
- **安全核心**: Rust 内存安全 + 形式化验证
- **全数据类型**: glTF/FBX/OBJ/点云/地形

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

## 📦 目录结构

```
phoenix-engine/
├── CMakeLists.txt              # 主构建配置
├── include/phoenix/            # 公共头文件
│   ├── core/                   # 核心类型
│   ├── math/                   # 数学库
│   ├── render/                 # 渲染接口
│   ├── scene/                  # 场景管理
│   ├── resource/               # 资源管理
│   └── security/               # 安全接口
├── src/                        # 源代码
│   ├── core/
│   ├── math/
│   ├── render/
│   ├── scene/
│   └── resource/
├── tests/                      # 单元测试
│   ├── test_math.cpp
│   ├── test_security.cpp
│   └── ...
├── examples/                   # 示例程序
│   ├── basic-render/
│   ├── pbr-demo/
│   └── wasm-demo/
├── rust-security-core/         # Rust 安全核心
│   ├── Cargo.toml
│   ├── src/
│   │   ├── lib.rs
│   │   ├── allocator.rs
│   │   ├── crypto.rs
│   │   └── audit.rs
│   └── include/
│       └── security_core.h
├── docs/                       # 文档
│   ├── API-DESIGN.md
│   ├── CODING-STANDARDS.md
│   └── ...
└── third-party/                # 第三方库
    ├── bgfx/
    ├── assimp/
    └── ...
```

---

## 🔧 技术栈

### 语言
- **C++17/20**: 性能关键模块
- **Rust 2021**: 安全核心模块
- **GLSL/HLSL**: 着色器
- **JavaScript/TypeScript**: Web bindings

### 图形 API
- Vulkan (首选)
- DirectX 12
- Metal
- OpenGL 4.5
- WebGPU
- WebGL 2.0

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

## 📅 开发计划

| Phase | 周期 | 主要内容 |
|-------|------|----------|
| 1 | Week 1-3 | 基础架构、Rust 安全核心、测试框架 |
| 2 | Week 4-7 | 渲染核心、bgfx 集成、着色器系统 |
| 3 | Week 8-10 | 场景图、空间加速、剔除系统 |
| 4 | Week 11-14 | PBR 材质、阴影、延迟渲染 |
| 5 | Week 15-17 | 跨平台适配、WebGPU、移动端 |
| 6 | Week 18-20 | 安全加固、形式化验证、渗透测试 |
| 7 | Week 21-23 | 数据类型扩展、点云、地形 |
| 8 | Week 24-25 | 文档、示例、交付 |

---

## 📖 文档

- [项目方案](http://47.245.126.212:3000/dev/3d-engine-plan.html)
- [团队组织方案](./TEAM-ORGANIZATION.md)
- [API 设计](./docs/API-DESIGN.md)
- [编码规范](./docs/CODING-STANDARDS.md)

---

## 🤝 参与贡献

1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/amazing-feature`)
3. 提交变更 (`git commit -m 'Add amazing feature'`)
4. 推送到分支 (`git push origin feature/amazing-feature`)
5. 开启 Pull Request

---

## 📄 许可证

本项目采用 MIT 许可证（第三方库遵循各自许可证）。

---

## 📞 联系方式

- **项目主页**: https://github.com/your-org/phoenix-engine
- **问题反馈**: https://github.com/your-org/phoenix-engine/issues
- **讨论区**: https://github.com/your-org/phoenix-engine/discussions

---

**最后更新**: 2026-03-26
