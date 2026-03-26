# Phoenix Engine 文档中心

欢迎使用 Phoenix Engine - 高性能跨平台 3D 渲染引擎

## 📚 文档导航

### 🔧 API 参考
- [Doxygen API 文档](api/doxygen/README.md) - 完整的 API 参考
- [Sphinx 文档站点](api/sphinx/README.md) - 可搜索的文档站点

### 📖 用户手册
- [快速入门](manual/quickstart.md) - 5 分钟开始使用
- [安装指南](manual/installation.md) - 所有平台安装说明
- [构建指南](manual/building.md) - 从源码构建
- [部署指南](manual/deployment.md) - 生产环境部署

### 🎓 教程系列
#### 基础渲染教程 (10 课)
1. [第一课：创建第一个窗口](tutorials/basic-rendering/lesson-01-first-window.md)
2. [第二课：初始化渲染器](tutorials/basic-rendering/lesson-02-initialize-renderer.md)
3. [第三课：绘制三角形](tutorials/basic-rendering/lesson-03-draw-triangle.md)
4. [第四课：顶点缓冲区](tutorials/basic-rendering/lesson-04-vertex-buffer.md)
5. [第五课：索引缓冲区](tutorials/basic-rendering/lesson-05-index-buffer.md)
6. [第六课：纹理映射](tutorials/basic-rendering/lesson-06-texture-mapping.md)
7. [第七课：光照基础](tutorials/basic-rendering/lesson-07-lighting-basics.md)
8. [第八课：材质系统](tutorials/basic-rendering/lesson-08-material-system.md)
9. [第九课：相机控制](tutorials/basic-rendering/lesson-09-camera-control.md)
10. [第十课：完整场景](tutorials/basic-rendering/lesson-10-complete-scene.md)

#### 场景系统教程 (5 课)
1. [第一课：场景图基础](tutorials/scene-system/lesson-01-scene-graph.md)
2. [第二课：场景节点](tutorials/scene-system/lesson-02-scene-nodes.md)
3. [第三课：场景序列化](tutorials/scene-system/lesson-03-serialization.md)
4. [第四课：GLTF 加载](tutorials/scene-system/lesson-04-gltf-loading.md)
5. [第五课：场景管理](tutorials/scene-system/lesson-05-scene-management.md)

#### 动画系统教程 (5 课)
1. [第一课：骨骼动画基础](tutorials/animation-system/lesson-01-skeletal-animation.md)
2. [第二课：关键帧动画](tutorials/animation-system/lesson-02-keyframe-animation.md)
3. [第三课：形态动画](tutorials/animation-system/lesson-03-morph-animation.md)
4. [第四课：动画状态机](tutorials/animation-system/lesson-04-animation-state-machine.md)
5. [第五课：动画混合](tutorials/animation-system/lesson-05-animation-blending.md)

#### 着色器编写教程 (5 课)
1. [第一课：着色器基础](tutorials/shader-writing/lesson-01-shader-basics.md)
2. [第二课：顶点着色器](tutorials/shader-writing/lesson-02-vertex-shader.md)
3. [第三课：片段着色器](tutorials/shader-writing/lesson-03-fragment-shader.md)
4. [第四课：计算着色器](tutorials/shader-writing/lesson-04-compute-shader.md)
5. [第五课：着色器优化](tutorials/shader-writing/lesson-05-shader-optimization.md)

#### 性能优化教程 (5 课)
1. [第一课：性能分析基础](tutorials/performance-optimization/lesson-01-profiling.md)
2. [第二课：批处理优化](tutorials/performance-optimization/lesson-02-batching.md)
3. [第三课：LOD 系统](tutorials/performance-optimization/lesson-03-lod.md)
4. [第四课：内存优化](tutorials/performance-optimization/lesson-04-memory.md)
5. [第五课：移动端优化](tutorials/performance-optimization/lesson-05-mobile.md)

### 💻 示例程序
#### 基础示例 (5 个)
- [Hello World](examples/basic/01-hello-world.md)
- [窗口创建](examples/basic/02-window-creation.md)
- [清屏](examples/basic/03-clear-screen.md)
- [输入处理](examples/basic/04-input-handling.md)
- [时间系统](examples/basic/05-time-system.md)

#### 渲染示例 (5 个)
- [基础渲染](examples/rendering/01-basic-rendering.md)
- [PBR 渲染](examples/rendering/02-pbr-rendering.md)
- [阴影渲染](examples/rendering/03-shadow-rendering.md)
- [后期处理](examples/rendering/04-post-processing.md)
- [延迟渲染](examples/rendering/05-deferred-rendering.md)

#### 动画示例 (3 个)
- [骨骼动画](examples/animation/01-skeletal-animation.md)
- [形态动画](examples/animation/02-morph-animation.md)
- [动画混合](examples/animation/03-animation-blending.md)

#### 物理示例 (3 个)
- [刚体物理](examples/physics/01-rigid-body.md)
- [碰撞检测](examples/physics/02-collision-detection.md)
- [物理模拟](examples/physics/03-physics-simulation.md)

#### 跨平台示例 (4 个)
- [Android 示例](examples/cross-platform/01-android.md)
- [iOS 示例](examples/cross-platform/02-ios.md)
- [WebAssembly 示例](examples/cross-platform/03-wasm.md)
- [桌面跨平台](examples/cross-platform/04-desktop.md)

### 📋 技术文档
- [架构设计](technical/architecture.md)
- [性能优化指南](technical/performance-guide.md)
- [安全最佳实践](technical/security-best-practices.md)
- [FAQ 与故障排除](technical/faq-troubleshooting.md)

### 🎬 视频教程
- [屏幕录制脚本](video-scripts/recording-script.md)
- [演示视频大纲](video-scripts/demo-outline.md)

## 📊 文档统计
- API 覆盖率：100%
- 教程数量：30+
- 示例程序：20+
- 文档格式：HTML/PDF

## 🚀 快速开始

```bash
# 克隆仓库
git clone https://github.com/phoenix-engine/phoenix.git

# 安装依赖
./scripts/install-deps.sh

# 构建
mkdir build && cd build
cmake ..
make -j$(nproc)

# 运行示例
./examples/basic/hello-world
```

## 📞 支持
- GitHub Issues: [报告问题](https://github.com/phoenix-engine/phoenix/issues)
- 讨论区：[GitHub Discussions](https://github.com/phoenix-engine/phoenix/discussions)
- 邮件：support@phoenix-engine.dev

---
*Phoenix Engine v1.0 - 高性能跨平台 3D 渲染引擎*
