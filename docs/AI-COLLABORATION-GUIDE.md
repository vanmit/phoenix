# Phoenix Engine - AI 协同开发指南

**版本**: 1.0  
**更新日期**: 2026-03-29  
**项目**: Phoenix Engine 跨平台三维渲染引擎  

---

## 📖 目录

1. [项目概述](#项目概述)
2. [AI Agent 团队架构](#ai-agent-团队架构)
3. [核心提示词库](#核心提示词库)
4. [开发规范](#开发规范)
5. [审查流程](#审查流程)
6. [快速上手](#快速上手)

---

## 项目概述

### 项目信息
- **名称**: Phoenix Engine (凤凰引擎)
- **目标**: 军工级安全的跨平台三维渲染引擎
- **技术栈**: C++20, Rust, bgfx, WebGPU, WebAssembly
- **周期**: 25 周 (约 6 个月)
- **GitHub**: https://github.com/vanmit/phoenix

### 核心特性
- 🛡️ **安全优先**: Rust 安全核心，形式化验证
- 🎮 **ECS 架构**: 数据导向设计
- 🎨 **跨平台渲染**: bgfx + WebGPU
- 🔒 **军工级安全**: CWE/MISRA 合规，Common Criteria EAL4+

---

## AI Agent 团队架构

### 组织架构图

```
┌─────────────────────────────────────────────────┐
│           项目总指挥 (Main Agent)                │
│            当前 AI 助理 / 协调员                   │
└─────────────────────────────────────────────────┘
                    │
    ┌───────────────┼───────────────┐
    ▼               ▼               ▼
┌─────────┐   ┌─────────┐   ┌─────────┐
│架构设计组│   │核心开发组│   │安全审计组│
│ 1 Agent │   │ 3 Agents│   │ 1 Agent │
└─────────┘   └─────────┘   └─────────┘
```

### Agent 角色列表

| 代号 | 角色 | 职责 | 技能 |
|------|------|------|------|
| `architect-01` | 架构师 | 系统架构、UML 建模 | coding-agent, searxng |
| `renderer-01` | 渲染开发 | 渲染管线、着色器 | coding-agent, agent-browser |
| `scene-01` | 场景开发 | 场景图、ECS、剔除 | coding-agent, searxng |
| `security-01` | 安全开发 | Rust 核心、加密、审计 | coding-agent, skill-vetter |
| `audit-01` | 安全审计 | 代码审查、渗透测试 | skill-vetter, searxng |
| `test-01` | 测试工程师 | 单元测试、基准测试 | coding-agent |
| `docs-01` | 文档工程师 | API 文档、教程 | coding-agent |

---

## 核心提示词库

### 1. 架构设计提示词

#### 提示词 1.1: 数学库实现
```
你是一位资深图形引擎架构师，请实现一个军工级安全的 3D 数学库。

要求:
1. 类型定义:
   - Vec2/Vec3/Vec4，支持 float 和 double
   - Mat3/Mat4，列主序存储
   - Quaternion (四元数)
   - Transform (位置 + 旋转 + 缩放)

2. SIMD 优化:
   - 使用 SSE/AVX (x86) 和 NEON (ARM) 指令集
   - 运行时 CPU 能力检测
   - 提供 fallback 纯 C++ 实现

3. 安全要求:
   - 所有运算检查数值范围
   - 防止 NaN/Inf 传播
   - 矩阵求逆检查行列式 (接近 0 时报错)
   - 四元数归一化验证

4. 测试要求:
   - 单元测试覆盖所有运算
   - 精度测试 (与 GLM 参考值对比)
   - 性能基准测试 (vs GLM)

输出 C++20 代码，包含完整头文件和实现文件。
使用概念 (concepts) 约束模板参数。
```

#### 提示词 1.2: 渲染设备抽象层
```
设计一个跨平台渲染设备抽象层，参考 bgfx 架构。

核心接口:
```cpp
class RenderDevice {
public:
  // 资源创建 (强类型句柄)
  TextureHandle createTexture(const TextureDesc& desc);
  BufferHandle createBuffer(const BufferDesc& desc);
  ShaderHandle createShader(const ShaderDesc& desc);
  PipelineHandle createPipeline(const PipelineDesc& desc);
  
  // 渲染命令
  void beginFrame();
  void submit(ViewHandle view, const RenderItem* items, uint32_t count);
  void endFrame();
  
  // 资源管理
  void destroy(TextureHandle handle);
  void waitIdle();
};
```

后端支持:
- Vulkan (首选，功能最全)
- DirectX 12 (Windows)
- Metal (macOS/iOS)
- OpenGL 4.5+ (兼容模式)
- WebGPU (WASM)

安全设计:
- 资源句柄包含代数防止悬空引用
- 所有 API 调用返回错误码 (Result<T>)
- 资源泄漏检测 (调试模式)
- 参数验证 (范围、对齐、兼容性)

多线程支持:
- 命令缓冲可并行录制
- 资源创建线程安全
- 帧延迟删除队列

输出完整的头文件定义和实现框架。
包含错误处理和审计日志。
```

#### 提示词 1.3: 场景图系统
```
实现一个高性能场景图系统，参考 VSG 设计。

核心节点类型:
```cpp
class Node : public std::enable_shared_from_this<Node> {
public:
  Transform localTransform;
  Transform worldTransform;
  std::vector<std::shared_ptr<Node>> children;
  Node* parent = nullptr;
  AABB boundingBox;
  uint32_t dirtyFlags = 0;
  
  virtual void accept(Visitor& visitor);
  void updateWorldTransform();
  void markDirty();
};
```

空间加速结构:
- 八叉树用于大规模场景
- BVH 用于静态物体
- 动态更新支持 (增量重建)

剔除系统:
- 视锥剔除 (Frustum Culling)
- 遮挡剔除 (Hi-Z Buffer)
- 距离剔除 (LOD 距离阈值)

LOD 系统:
- 基于距离自动切换
- 屏幕空间误差估算
- 渐变过渡 (morph)

输出完整实现，包含性能优化考虑。
使用访问者模式实现遍历。
```

---

### 2. 安全开发提示词

#### 提示词 2.1: Rust 安全核心
```
实现一个军工级安全的内存分配器，使用 Rust 编写。

功能要求:
1. 边界检查: 分配时验证大小和 alignment
2. 代数验证: 防止释放后使用 (UAF)
3. 审计日志: 所有分配/释放操作记录 HMAC 保护日志
4. 加密存储: 敏感数据使用 AES-256-GCM 加密
5. Zeroizing: 释放时自动清零内存

FFI 接口:
```rust
#[no_mangle]
pub extern "C" fn secure_allocate(
    size: usize, 
    alignment: usize
) -> *mut c_void;

#[no_mangle]
pub extern "C" fn secure_deallocate(
    ptr: *mut c_void, 
    size: usize
);
```

形式化验证:
- 使用 Coq 证明内存安全
- 提取为可验证 C 代码

输出完整的 Rust 实现和 FFI 绑定。
```

#### 提示词 2.2: WASM 沙箱
```
设计一个安全的 WebAssembly 模块加载和执行系统。

模块加载:
1. 验证 WASM 数字签名 (RSA-4096/ECDSA-P384)
2. 配置导入函数白名单
3. 设置内存限制 (默认 64MB)

执行环境:
```cpp
class WASMSandbox {
public:
  struct Config {
    uint32_t memoryLimitMB = 64;
    uint32_t executionTimeoutMs = 100;
    std::vector<std::string> allowedImports;
    bool enableSIMD = true;
    bool enableThreads = false;
  };
  
  bool loadModule(const uint8_t* data, size_t size, const Config& cfg);
  bool callFunction(const char* name, const Args& args, Result& result);
  void terminate(); // 强制终止执行
};
```

安全检查:
- 禁止无限循环 (执行超时)
- 禁止越界内存访问
- 禁止未授权的导入调用
- 资源配额监控

资源隔离:
- 独立线性内存
- 独立函数表
- 资源通过句柄传递，不暴露指针

输出基于 wasmtime 或 wasmer 的实现。
包含完整的错误处理和审计日志。
```

---

### 3. 渲染开发提示词

#### 提示词 3.1: PBR 材质系统
```
实现一个基于物理的渲染 (PBR) 材质系统。

材质模型:
- Metallic-Roughness 工作流 (glTF 兼容)
- Specular-Glossiness 工作流 (可选)
- Clear Coat (清漆层)
- Anisotropic (各向异性)
- Subsurface (次表面散射，近似)

着色器实现:
```glsl
// PBR BRDF 核心
vec3 pbrBRDF(vec3 N, vec3 V, vec3 L, Material mat) {
  vec3 H = normalize(V + L);
  float NdotL = max(dot(N, L), 0.0);
  float NdotV = max(dot(N, V), 0.0);
  float NdotH = max(dot(N, H), 0.0);
  float VdotH = max(dot(V, H), 0.0);
  
  // Cook-Torrance 微表面模型
  float D = distributionGGX(N, H, mat.roughness);
  float G = geometrySmith(N, V, L, mat.roughness);
  vec3 F = fresnelSchlick(VdotH, mat.F0);
  
  vec3 specular = (D * G * F) / (4.0 * NdotV * NdotL + 0.001);
  vec3 diffuse = (vec3(1.0) - F) * mat.albedo / PI;
  
  return (diffuse + specular) * NdotL;
}
```

光照支持:
- 方向光 (平行光)
- 点光源
- 聚光灯
- IBL (图像基光照)

纹理支持:
- Albedo/Base Color
- Normal Map
- Metallic/Roughness
- AO (环境光遮蔽)
- Emissive

输出完整的 GLSL 着色器代码和 C++ 材质类。
```

#### 提示词 3.2: 阴影系统
```
实现级联阴影映射 (CSM) 系统。

级联分割:
- 根据相机远平面分割为 4 个级联
- 对数分割策略 (近处高分辨率)
- 每级联独立阴影贴图

阴影贴图生成:
```cpp
class CascadeShadowMap {
public:
  struct Cascade {
    Mat4 viewProj;
    AABB frustumBounds;
    float splitNear, splitFar;
    TextureHandle depthMap;
  };
  
  void update(const Camera& camera, const Light& light);
  void render(const Scene& scene);
  float sampleShadow(const Vec3& worldPos, uint32_t cascadeIdx);
};
```

优化技术:
- PCF (Percentage Closer Filtering) 软阴影
- VSM (Variance Shadow Maps) 减少锯齿
- CSM 级联混合 (避免接缝)
- 阴影剔除 (只渲染可见物体)

安全考虑:
- 阴影贴图尺寸限制 (防止显存耗尽)
- 级联数量限制 (性能预算)
- 深度精度验证 (避免阴影痤疮)

输出完整的 C++ 实现和 GLSL 着色器。
```

---

### 4. 测试开发提示词

#### 提示词 4.1: 单元测试
```
为 Phoenix Engine 数学库编写完整的单元测试。

测试框架：Google Test (gtest)

测试覆盖:
1. Vector 运算:
   - 加减乘除
   - 点积、叉积
   - 归一化
   - 插值 (lerp, slerp)

2. Matrix 运算:
   - 乘法、求逆
   - 转置
   - 行列式
   - 变换矩阵 (平移、旋转、缩放)

3. Quaternion 运算:
   - 乘法、共轭
   - 归一化
   - 角轴转换
   - 欧拉角转换

4. 边界测试:
   - 零向量
   - 单位矩阵
   - NaN/Inf 处理
   - 除零保护

5. 性能测试:
   - 与 GLM 对比
   - SIMD 优化验证

输出完整的 test_math.cpp 文件。
包含 100+ 个测试用例。
```

#### 提示词 4.2: 性能基准测试
```
为 Phoenix Engine 渲染系统建立性能基准测试。

测试项目:
1. 渲染性能:
   - FPS 测试 (不同场景复杂度)
   - Draw Calls 统计
   - 三角形吞吐量
   - 显存占用

2. 内存性能:
   - 分配速度 (vs std::malloc)
   - 内存碎片率
   - 缓存命中率

3. 加载性能:
   - 资源加载时间
   - 异步加载效率
   - 缓存命中率

4. 压力测试:
   - 10K 物体渲染
   - 24 小时稳定性
   - 内存泄漏检测

测试框架:
- Google Benchmark
- Valgrind (内存检查)
- RenderDoc (GPU 分析)

输出完整的 benchmark 代码和测试报告模板。
```

---

### 5. 文档编写提示词

#### 提示词 5.1: API 文档
```
为 Phoenix Engine 渲染系统生成 API 文档。

文档工具：Doxygen + Sphinx

文档结构:
1. 概述
   - 模块介绍
   - 架构图
   - 快速开始

2. 核心类
   - RenderDevice
   - Texture, Buffer, Shader
   - Pipeline, RenderPass

3. API 参考
   - 函数签名
   - 参数说明
   - 返回值
   - 错误码
   - 使用示例

4. 最佳实践
   - 性能优化
   - 安全注意事项
   - 常见问题

输出 Doxygen 配置的注释代码和 Sphinx 配置文件。
```

#### 提示词 5.2: 教程编写
```
编写 Phoenix Engine 快速入门教程。

目标读者：有 C++ 基础的游戏开发者

教程大纲:
1. 安装与构建
   - 环境要求
   - 编译步骤
   - 第一个程序

2. 基础概念
   - 引擎初始化
   - 场景管理
   - 实体组件系统

3. 渲染基础
   - 创建窗口
   - 加载模型
   - 设置相机
   - 光照基础

4. 进阶主题
   - PBR 材质
   - 阴影系统
   - 后处理效果

5. 部署发布
   - 跨平台打包
   - Web 部署
   - 性能优化

输出完整的 Markdown 教程，包含代码示例和截图说明。
```

---

## 开发规范

### 代码规范

#### C++ 编码规范
- **标准**: C++20
- **命名**: 
  - 类名：PascalCase (`RenderDevice`)
  - 函数：camelCase (`createTexture`)
  - 变量：snake_case (`texture_count`)
  - 常量：UPPER_CASE (`MAX_TEXTURES`)
- **注释**: Doxygen 风格
- **错误处理**: 返回 `Result<T>`，不使用异常

#### Rust 编码规范
- **标准**: Rust 2021 Edition
- **命名**: 遵循 Rust 官方规范
- **安全**: 最小化 `unsafe` 使用
- **测试**: 所有公共 API 必须有测试

### 安全规范

#### 输入验证
- 所有外部输入必须验证
- 边界检查强制执行
- 禁止裸指针传递

#### 内存安全
- 使用智能指针
- RAII 资源管理
- 禁止内存泄漏

#### 审计日志
- 所有敏感操作记录日志
- HMAC 保护日志完整性
- 不可篡改日志链

---

## 审查流程

### 代码审查流程

```
1. 开发完成
   ↓
2. 自测通过
   ↓
3. 提交审查请求
   ↓
4. 安排审查 Agent
   ↓
5. 审查通过？───否──→ 返工修复
   ↓是
6. 合并到 main 分支
   ↓
7. 更新进度文档
```

### 审查检查清单

#### 架构审查
- [ ] 设计模式合理
- [ ] 模块解耦良好
- [ ] 性能影响可接受
- [ ] 无循环依赖

#### 安全审查
- [ ] 输入验证完整
- [ ] 边界检查正确
- [ ] 无内存泄漏
- [ ] 审计日志完整

#### 代码质量审查
- [ ] 符合编码规范
- [ ] 单元测试覆盖 >80%
- [ ] 文档完整
- [ ] 无编译器警告

---

## 快速上手

### 新成员加入流程

1. **阅读文档**
   - `README.md` - 项目概述
   - `docs/manual/installation.md` - 安装指南
   - `docs/manual/building.md` - 构建指南

2. **配置环境**
   ```bash
   # 克隆项目
   git clone https://github.com/vanmit/phoenix.git
   cd phoenix-engine
   
   # 安装依赖
   ./scripts/setup.sh
   
   # 构建项目
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Debug
   cmake --build .
   ```

3. **运行测试**
   ```bash
   cd build
   ctest --output-on-failure
   ```

4. **开始开发**
   - 查看 `NEXT_TASKS.md` 了解当前任务
   - 选择合适的提示词开始开发
   - 完成后提交审查

### 常用命令

```bash
# 构建
cmake --build build --config Release

# 测试
cd build && ctest

# 清理
./scripts/cleanup-and-organize.sh

# 查看进度
cat FIX_PROGRESS.md

# 查看下一步任务
cat NEXT_TASKS.md
```

---

## 资源链接

- **GitHub**: https://github.com/vanmit/phoenix
- **文档**: `docs/` 目录
- **示例**: `examples/` 目录
- **审查报告**: `reviews/` 目录
- **开发方案**: `docs/archive/phase-reports/`

---

## 联系与支持

- **问题反馈**: GitHub Issues
- **讨论**: GitHub Discussions
- **文档贡献**: 提交 PR 到 `docs/` 目录

---

*最后更新：2026-03-29*  
*维护者：Phoenix Engine Team*
