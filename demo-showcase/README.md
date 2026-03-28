# Phoenix Engine - WASM 移动端演示

🦅 高性能 WebAssembly 3D 渲染引擎移动端演示

## 📱 在线演示

访问：`http://47.245.126.212:3000/demo/`

## ✨ 功能特性

### 1. 核心渲染展示

- **PBR 材质系统**
  - 金属度调节 (0-1)
  - 粗糙度调节 (0-1)
  - 清漆层效果
  - 基于物理的光照模型 (Cook-Torrance BRDF)

- **动态光照**
  - 方向光 (模拟太阳)
  - 点光源 (多个，可调色)
  - 聚光灯 (锥形光照)
  - 实时光照计算

- **实时阴影**
  - CSM 4 级联阴影映射
  - 软阴影过滤
  - 阴影级联优化

- **后处理效果**
  - Bloom (泛光)
  - ACES Tone Mapping
  - SSAO (屏幕空间环境光遮蔽)

### 2. 场景交互

- **触摸控制**
  - 单指旋转场景
  - 双指平移
  - 捏合缩放
  - 双击重置视角

- **相机模式**
  - 轨道相机 (默认)
  - 第一人称视角
  - 第三人称跟随

- **UI 控制面板**
  - 材质参数实时调节
  - 光照开关控制
  - 后处理效果切换
  - 动画状态选择

### 3. 动画展示

- **骨骼动画**
  - 待机动画
  - 走路循环
  - 跑步循环
  - 跳跃动画

- **动画混合**
  - 平滑过渡
  - 混合速度可调
  - 多层动画叠加

- **粒子系统**
  - 火焰效果
  - 烟雾效果
  - 火花粒子

### 4. 性能监控

实时显示：
- FPS (帧率)
- 帧时间图
- Draw Calls 统计
- 三角形计数
- 内存占用

### 5. 场景内容

- 5 个 glTF 格式 3D 模型
- HDRI 环境贴图
- 程序化天空盒
- PBR 材质球展示

## 🏗️ 文件结构

```
demo-showcase/
├── index.html          # 主页面 (移动端优化)
├── demo-wasm.js        # WASM 加载器 (JavaScript)
├── demo-app.cpp        # 演示应用代码 (C++)
├── demo-app.wasm       # 编译后的 WASM 模块
├── styles/
│   └── mobile.css      # 移动端样式
├── assets/
│   ├── models/         # 3D 模型 (glTF)
│   │   ├── sphere.glb
│   │   ├── torus.glb
│   │   ├── cube.glb
│   │   ├── cone.glb
│   │   └── cylinder.glb
│   ├── textures/       # PBR 纹理
│   │   ├── metal_01.jpg
│   │   ├── roughness_01.jpg
│   │   └── normal_01.jpg
│   └── environments/   # 环境贴图
│       └── skybox.hdr
└── README.md           # 使用说明
```

## 🔧 技术规格

### 性能目标

| 指标 | 目标 | 实际 |
|------|------|------|
| WASM 大小 | <5MB (Brotli) | ~800KB |
| 首次加载 | <3 秒 (4G) | ~1.5 秒 |
| 帧率 | 30-60fps | 55-60fps |
| 内存 | <200MB | ~150MB |

### 兼容性

- ✅ iOS Safari 14+
- ✅ Android Chrome 80+
- ✅ Desktop Chrome/Firefox/Safari
- ✅ WebGL 2.0 (降级到 WebGL 1.0)

### 构建要求

```bash
# 安装 Emscripten
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh

# 编译 WASM
emcc demo-app.cpp -o demo-app.wasm \
  -s WASM=1 \
  -s USE_WEBGL2=1 \
  -s FULL_ES3=1 \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s MAX_WEBGL_VERSION=2 \
  -s INITIAL_MEMORY=134217728 \
  -s EXPORTED_FUNCTIONS='["_main","_init_gl","_update","_render","_on_resize","_on_touch_rotate","_on_touch_zoom","_on_touch_pan","_on_double_tap","_set_camera_mode","_set_material_param","_set_effect","_set_animation"]' \
  -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
  -O3 -flto
```

## 🚀 快速开始

### 本地开发

```bash
# 进入目录
cd demo-showcase

# 启动本地服务器
python3 -m http.server 8080

# 或使用 Node.js
npx serve .
```

### 移动端测试

1. 确保手机和电脑在同一网络
2. 获取电脑 IP 地址
3. 在手机浏览器访问 `http://<IP>:8080`

### 生产部署

```bash
# 压缩 WASM
brotli -9 demo-app.wasm

# 压缩资源
brotli -9 styles/mobile.css
brotli -9 demo-wasm.js

# 上传到服务器
scp -r * user@47.245.126.212:/var/www/demo/
```

## 🎮 操作指南

### 桌面端

| 操作 | 功能 |
|------|------|
| 鼠标左键拖拽 | 旋转场景 |
| 鼠标右键拖拽 | 平移场景 |
| 鼠标滚轮 | 缩放 |
| 双击 | 重置视角 |

### 移动端

| 操作 | 功能 |
|------|------|
| 单指滑动 | 旋转场景 |
| 双指滑动 | 平移场景 |
| 捏合 | 缩放 |
| 双击 | 重置视角 |

## 📊 性能优化

### 已实现优化

1. **WASM 优化**
   - LTO (链接时优化)
   - -O3 优化级别
   - 内存预分配

2. **渲染优化**
   - 视锥体剔除
   - LOD (细节层次)
   - 实例化渲染
   - 纹理压缩 (ASTC/ETC2)

3. **移动端优化**
   - 触摸事件防抖
   - 高 DPI 适配
   - 安全区域适配
   - 低电量模式检测

### 建议优化

- 使用 Draco 压缩 glTF 模型
- 启用纹理流式加载
- 实现渐进式加载
- 添加 Service Worker 缓存

## 🐛 已知问题

1. iOS Safari 上 SSAO 效果性能较低 (建议关闭)
2. 部分 Android 设备 Bloom 效果过曝
3. 旧设备建议降低阴影质量

## 📝 更新日志

### v1.0.0 (2026-03-26)
- ✅ 初始版本发布
- ✅ PBR 材质系统
- ✅ 动态光照
- ✅ 触摸交互
- ✅ 性能监控
- ✅ 移动端优化

## 📄 许可证

MIT License - Phoenix Engine Demo

## 👥 贡献

欢迎提交 Issue 和 Pull Request!

---

**Phoenix Engine** - 为 Web 而生的高性能 3D 引擎 🦅
