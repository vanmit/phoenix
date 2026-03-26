# 视频教程脚本与大纲

## 📹 屏幕录制脚本

### 视频 1: Phoenix Engine 快速入门 (5 分钟)

#### 开场 (0:00-0:30)
```
[画面：引擎 logo 动画]
旁白：欢迎学习 Phoenix Engine - 高性能跨平台 3D 渲染引擎。
在本教程中，我们将在 5 分钟内创建第一个 3D 应用程序。
```

#### 安装 (0:30-1:30)
```
[画面：终端操作录屏]
旁白：首先，克隆仓库并安装依赖。

操作演示：
git clone https://github.com/phoenix-engine/phoenix.git
cd phoenix
./scripts/install-deps.sh

旁白：Phoenix Engine 支持 Windows、macOS、Linux、
Android、iOS 和 WebAssembly 平台。
```

#### 构建 (1:30-2:30)
```
[画面：CMake 配置和构建]
旁白：使用 CMake 配置项目。

操作演示：
mkdir build && cd build
cmake ..
make -j$(nproc)

旁白：构建完成后，我们可以运行示例程序。
```

#### 第一个程序 (2:30-4:00)
```
[画面：代码编辑器]
旁白：让我们创建第一个程序。

代码演示：
#include <phoenix/phoenix.hpp>

int main() {
    phoenix::Application app("My App");
    auto window = app.createWindow(800, 600);
    
    while (app.isRunning()) {
        app.pollEvents();
        window->clear({0.1f, 0.1f, 0.2f, 1.0f});
        window->swapBuffers();
    }
    return 0;
}

[画面：程序运行，显示窗口]
旁白：运行程序，我们看到了一个窗口。
```

#### 结束 (4:00-5:00)
```
[画面：文档网站]
旁白：更多教程和文档，请访问我们的网站。
感谢观看！

[画面：社交媒体链接、GitHub 链接]
```

---

### 视频 2: 渲染第一个三角形 (8 分钟)

#### 开场 (0:00-0:30)
```
[画面：三角形旋转动画]
旁白：在本教程中，我们将学习如何绘制第一个三角形。
```

#### 顶点数据 (0:30-2:00)
```
[画面：代码编辑器]
旁白：首先，定义顶点数据。

代码演示：
struct Vertex {
    float x, y, z;  // 位置
    float r, g, b;  // 颜色
};

Vertex vertices[] = {
    { 0.0f,  0.5f, 0.0f,  1.0f, 0.0f, 0.0f },
    { 0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f },
    {-0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f },
};
```

#### 创建缓冲区 (2:00-3:30)
```
旁白：创建顶点缓冲区。

代码演示：
auto vertexBuffer = renderer->createVertexBuffer(
    vertices,
    sizeof(vertices),
    phoenix::BufferUsage::Static
);
```

#### 着色器 (3:30-5:30)
```
旁白：编写顶点和片段着色器。

顶点着色器：
#version 450
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(position, 1.0);
    fragColor = color;
}

片段着色器：
#version 450
layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}
```

#### 绘制 (5:30-7:00)
```
旁白：创建管线并绘制。

代码演示：
auto pipeline = renderer->createPipeline({
    .shader = shader,
    .vertexBuffer = vertexBuffer,
});

while (app.isRunning()) {
    app.pollEvents();
    renderer->clear({0.1f, 0.1f, 0.2f, 1.0f});
    renderer->draw(pipeline, 0, 3);
    renderer->present();
}
```

#### 结束 (7:00-8:00)
```
[画面：旋转的彩色三角形]
旁白：看，我们成功绘制了一个彩色三角形！
下一集，我们将学习纹理映射。
```

---

### 视频 3: 场景系统入门 (10 分钟)

#### 场景图概念 (0:00-2:00)
```
[画面：场景图可视化]
旁白：场景图是组织 3D 对象的层次结构。

Scene
├── Camera
├── Lights
│   ├── DirectionalLight
│   └── PointLight
└── Objects
    ├── Player
    ├── Enemy1
    └── Enemy2
```

#### 创建场景 (2:00-4:00)
```
代码演示：
auto scene = std::make_unique<phoenix::Scene>();

// 创建相机
auto camera = scene->createCamera();
camera->setPosition({0, 5, 10});
camera->lookAt({0, 0, 0});

// 创建光源
auto light = scene->createLight(LightType::Directional);
light->setPosition({10, 10, 10});
light->lookAt({0, 0, 0});
```

#### 加载模型 (4:00-6:00)
```
代码演示：
// 加载 GLTF 模型
auto model = scene->loadModel("models/character.glb");
model->setPosition({0, 0, 0});

// 添加动画
auto animator = model->addComponent<Animator>();
animator->play("Idle");
```

#### 场景遍历 (6:00-8:00)
```
代码演示：
scene->traverse([](SceneNode& node) {
    if (node.hasComponent<Mesh>()) {
        renderer->draw(node.get<Mesh>());
    }
});
```

#### 结束 (8:00-10:00)
```
[画面：完整场景演示]
旁白：场景系统让我们轻松管理复杂场景。
下一集，我们将学习动画系统。
```

---

## 📋 演示视频大纲

### 视频系列 1: 基础教程 (10 集)

1. **安装与配置** (5 分钟)
   - 系统要求
   - 依赖安装
   - 构建项目
   - 运行示例

2. **创建窗口** (5 分钟)
   - Application 类
   - 窗口配置
   - 事件处理
   - 主循环

3. **渲染基础** (8 分钟)
   - 渲染设备
   - 顶点缓冲区
   - 着色器
   - 绘制调用

4. **纹理映射** (10 分钟)
   - 纹理加载
   - UV 坐标
   - 纹理采样
   - 纹理过滤

5. **光照基础** (10 分钟)
   - 光源类型
   - 环境光
   - 漫反射
   - 镜面反射

6. **相机系统** (8 分钟)
   - 相机类型
   - 视图矩阵
   - 投影矩阵
   - 相机控制

7. **场景图** (10 分钟)
   - 节点层次
   - 变换继承
   - 场景遍历
   - 组件系统

8. **模型加载** (10 分钟)
   - GLTF 格式
   - 网格加载
   - 材质加载
   - 纹理加载

9. **动画基础** (12 分钟)
   - 关键帧动画
   - 骨骼动画
   - 动画混合
   - 状态机

10. **性能优化** (15 分钟)
    - 批处理
    - LOD 系统
    - 剔除
    - 性能分析

### 视频系列 2: 进阶教程 (5 集)

1. **PBR 渲染** (15 分钟)
2. **阴影技术** (15 分钟)
3. **后期处理** (15 分钟)
4. **物理集成** (15 分钟)
5. **跨平台部署** (20 分钟)

### 视频系列 3: 实战项目 (5 集)

1. **第一人称视角** (20 分钟)
2. **第三人称视角** (20 分钟)
3. **RTS 相机** (20 分钟)
4. **UI 系统** (20 分钟)
5. **完整游戏 Demo** (30 分钟)

---

## 🎬 录制指南

### 软件要求
- OBS Studio (录屏)
- DaVinci Resolve (剪辑)
- Audacity (音频)

### 录制设置
- 分辨率：1920x1080
- 帧率：60 FPS
- 比特率：10 Mbps
- 音频：48kHz, 192kbps

### 最佳实践
1. 使用高对比度主题
2. 放大代码字体
3. 避免快速移动
4. 添加字幕
5. 背景音乐音量低于旁白

---

*最后更新：2026-03-26*
