# Phoenix Engine 架构设计文档

## 📋 概述

Phoenix Engine 是一个高性能跨平台 3D 渲染引擎，采用现代化架构设计。

## 🏗️ 系统架构

### 整体架构图

```
┌─────────────────────────────────────────────────────────────┐
│                      Application Layer                       │
│                    (用户应用程序代码)                          │
├─────────────────────────────────────────────────────────────┤
│                      Phoenix Engine API                      │
│  ┌─────────────┬─────────────┬─────────────┬─────────────┐  │
│  │   Scene     │   Render    │  Resource   │   Platform  │  │
│  │   Module    │   Module    │   Module    │   Module    │  │
│  └─────────────┴─────────────┴─────────────┴─────────────┘  │
├─────────────────────────────────────────────────────────────┤
│                    Platform Abstraction Layer                │
│  ┌──────────┬──────────┬──────────┬──────────┬──────────┐   │
│  │ Vulkan   │  Metal   │   DX12   │ WebGPU   │  OpenGL  │   │
│  └──────────┴──────────┴──────────┴──────────┴──────────┘   │
├─────────────────────────────────────────────────────────────┤
│                     Hardware Layer                           │
│                    (GPU / CPU / Memory)                      │
└─────────────────────────────────────────────────────────────┘
```

## 📦 核心模块

### 1. Platform 模块

负责平台抽象和系统服务。

```
phoenix/platform/
├── application.hpp      # 应用程序主类
├── window.hpp           # 窗口管理
├── input.hpp            # 输入处理
├── timer.hpp            # 时间系统
├── platform_types.hpp   # 平台类型定义
├── windows/             # Windows 平台实现
│   ├── dx12_device.hpp
│   └── win32_window.hpp
├── linux/               # Linux 平台实现
│   └── vulkan_device.hpp
├── macos/               # macOS 平台实现
│   └── metal_device.hpp
└── webgpu/              # WebGPU 实现
    └── webgpu_device.hpp
```

**核心类:**
- `Application` - 应用程序入口点
- `Window` - 窗口抽象
- `Input` - 输入处理
- `Timer` - 时间管理

### 2. Render 模块

负责图形渲染功能。

```
phoenix/render/
├── RenderDevice.hpp     # 渲染设备接口
├── Pipeline.hpp         # 渲染管线
├── Shader.hpp           # 着色器管理
├── Resources.hpp        # 渲染资源
├── Types.hpp            # 渲染类型定义
├── PBR.hpp              # PBR 渲染
├── Shadows.hpp          # 阴影系统
├── PostProcess.hpp      # 后期处理
└── DeferredRenderer.hpp # 延迟渲染器
```

**核心类:**
- `RenderDevice` - 渲染设备抽象
- `Pipeline` - 渲染管线状态
- `Shader` - 着色器程序
- `Texture` - 纹理资源
- `Buffer` - 缓冲区资源

### 3. Scene 模块

负责场景管理和空间组织。

```
phoenix/scene/
├── scene.hpp            # 场景主类
├── scene_node.hpp       # 场景节点
├── scene_serializer.hpp # 场景序列化
├── gltf_loader.hpp      # GLTF 加载器
├── physics.hpp          # 物理系统
├── bvh.hpp              # BVH 加速结构
├── octree.hpp           # 八叉树
├── skeleton.hpp         # 骨骼系统
└── morph_animation.hpp  # 形态动画
```

**核心类:**
- `Scene` - 场景容器
- `SceneNode` - 场景图节点
- `Camera` - 相机
- `Light` - 光源
- `Mesh` - 网格

### 4. Resource 模块

负责资源管理和加载。

```
phoenix/resource/
├── resource_manager.hpp # 资源管理器
├── asset_loader.hpp     # 资产加载器
├── mesh.hpp             # 网格资源
├── texture.hpp          # 纹理资源
├── types.hpp            # 资源类型
├── terrain.hpp          # 地形资源
└── point_cloud.hpp      # 点云资源
```

**核心类:**
- `ResourceManager` - 资源管理
- `AssetLoader` - 资产加载
- `Mesh` - 网格数据
- `Texture` - 纹理数据

### 5. Math 模块

提供数学工具和类型。

```
phoenix/math/
├── vector2.hpp          # 2D 向量
├── vector3.hpp          # 3D 向量
├── vector4.hpp          # 4D 向量
├── matrix4.hpp          # 4x4 矩阵
├── quaternion.hpp       # 四元数
├── color.hpp            # 颜色
├── bounding.hpp         # 包围盒
└── frustum.hpp          # 视锥体
```

## 🔄 数据流

### 渲染流程

```
1. 应用程序创建 Scene
         ↓
2. Scene 包含 SceneNode 和 Mesh
         ↓
3. RenderDevice 创建 Pipeline
         ↓
4. 遍历 Scene，收集可看见对象
         ↓
5. 按材质/着色器排序
         ↓
6. 批量绘制调用
         ↓
7. 呈现到屏幕
```

### 资源加载流程

```
1. 请求资源 (异步)
         ↓
2. ResourceManager 检查缓存
         ↓
3. 如未缓存，AssetLoader 加载
         ↓
4. 解析文件格式 (GLTF/OBJ/FBX)
         ↓
5. 创建 GPU 资源 (Buffer/Texture)
         ↓
6. 返回资源句柄
         ↓
7. 回调通知完成
```

## 🎯 设计原则

### 1. 数据导向设计

```cpp
// ❌ 面向对象 (缓存不友好)
class GameObject {
    Transform transform;
    Mesh* mesh;
    Material* material;
    GameObject* next;  // 指针追逐
};

// ✅ 数据导向 (缓存友好)
struct TransformComponent {
    float positions[MAX_ENTITIES][3];
    float rotations[MAX_ENTITIES][4];
    float scales[MAX_ENTITIES][3];
};

struct MeshComponent {
    uint32_t meshIds[MAX_ENTITIES];
};
```

### 2. 资源所有权

```cpp
// 使用智能指针管理资源
class Scene {
    std::vector<std::unique_ptr<SceneNode>> nodes;
    std::unordered_map<std::string, 
        std::shared_ptr<Texture>> textures;
};

// 资源句柄 (轻量级引用)
class TextureHandle {
    uint32_t id;
    ResourceManager* manager;
};
```

### 3. 跨平台抽象

```cpp
// 统一接口
class RenderDevice {
public:
    virtual bool initialize(Window* window) = 0;
    virtual void clear(const Color& color) = 0;
    virtual void draw(Pipeline* pipeline) = 0;
    virtual void present() = 0;
};

// 平台实现
class VulkanDevice : public RenderDevice {
    // Vulkan 具体实现
};

class MetalDevice : public RenderDevice {
    // Metal 具体实现
};
```

## 📊 内存管理

### 内存池

```cpp
class MemoryPool {
public:
    MemoryPool(size_t blockSize, size_t blockCount);
    
    void* allocate(size_t size);
    void deallocate(void* ptr);
    
    size_t getUsedMemory() const;
    size_t getFreeMemory() const;
    
private:
    std::vector<char> memory_;
    std::vector<size_t> freeList_;
};
```

### 帧分配器

```cpp
class FrameAllocator {
public:
    void* allocate(size_t size, size_t alignment);
    
    void beginFrame();
    void endFrame();
    
private:
    std::vector<char> buffer_;
    size_t offset_;
};

// 使用
FrameAllocator frameAlloc(1024 * 1024);  // 1MB

void renderFrame() {
    frameAlloc.beginFrame();
    
    // 分配临时内存
    void* data = frameAlloc.allocate(256, 16);
    
    // ... 使用数据
    
    frameAlloc.endFrame();  // 重置，不释放
}
```

## 🔒 线程安全

### 渲染线程模型

```
┌─────────────────┐
│  Main Thread    │  ← 输入、逻辑、场景更新
└────────┬────────┘
         │
         ↓ (命令缓冲)
┌─────────────────┐
│ Render Thread   │  ← 渲染命令执行
└────────┬────────┘
         │
         ↓
┌─────────────────┐
│   GPU Driver    │
└─────────────────┘
```

### 命令缓冲

```cpp
class CommandBuffer {
public:
    void begin();
    void end();
    
    void draw(Pipeline* pipeline, uint32_t vertexCount);
    void setViewport(float x, float y, float w, float h);
    void setScissor(int x, int y, int w, int h);
    
private:
    std::vector<Command> commands_;
};

// 多线程录制
CommandBuffer cmd1, cmd2, cmd3;

std::thread t1([&]() { recordCommands(cmd1); });
std::thread t2([&]() { recordCommands(cmd2); });
std::thread t3([&]() { recordCommands(cmd3); });

t1.join(); t2.join(); t3.join();

// 提交到 GPU
renderer->submit({&cmd1, &cmd2, &cmd3});
```

## ⚡ 性能优化

### 批处理策略

```cpp
class BatchRenderer {
public:
    void submit(Mesh* mesh, Material* mat, Transform& transform);
    void flush();
    
private:
    struct Batch {
        Material* material;
        std::vector<DrawCall> drawCalls;
    };
    
    std::unordered_map<Material*, Batch> batches_;
};

// 使用
BatchRenderer batcher;

for (auto& object : scene.objects) {
    batcher.submit(
        object.mesh,
        object.material,
        object.transform
    );
}

batcher.flush();  // 按材质分组，减少状态切换
```

### LOD 系统

```cpp
class LODGroup {
public:
    void update(const Camera& camera);
    Mesh* getCurrentMesh() const;
    
private:
    struct LODLevel {
        Mesh* mesh;
        float distance;
    };
    
    std::vector<LODLevel> levels_;
    int currentLevel_;
};

// 根据距离选择 LOD
void LODGroup::update(const Camera& camera) {
    float distance = camera.distanceTo(origin_);
    
    for (int i = levels_.size() - 1; i >= 0; i--) {
        if (distance > levels_[i].distance) {
            currentLevel_ = i;
            return;
        }
    }
    currentLevel_ = 0;
}
```

## 🛡️ 安全考虑

### 资源验证

```cpp
class ResourceManager {
public:
    Texture* loadTexture(const std::string& path) {
        // 验证路径
        if (!isValidPath(path)) {
            logError("Invalid path: " + path);
            return nullptr;
        }
        
        // 验证文件大小
        size_t size = getFileSize(path);
        if (size > MAX_TEXTURE_SIZE) {
            logError("Texture too large: " + path);
            return nullptr;
        }
        
        // 加载...
    }
    
private:
    static constexpr size_t MAX_TEXTURE_SIZE = 256 * 1024 * 1024;
};
```

### 输入验证

```cpp
class InputHandler {
public:
    void setSensitivity(float value) {
        // 验证输入范围
        sensitivity_ = std::clamp(value, 0.0f, 10.0f);
    }
    
    void setFOV(float value) {
        // 验证 FOV 范围
        fov_ = std::clamp(value, 1.0f, 179.0f);
    }
};
```

## 📈 扩展性

### 插件系统

```cpp
class IPlugin {
public:
    virtual ~IPlugin() = default;
    virtual const char* getName() const = 0;
    virtual bool initialize(Engine* engine) = 0;
    virtual void shutdown() = 0;
};

class PluginManager {
public:
    void loadPlugin(const std::string& path);
    void unloadPlugin(const std::string& name);
    
private:
    std::unordered_map<std::string, 
        std::unique_ptr<IPlugin>> plugins_;
};
```

### 自定义渲染通道

```cpp
class RenderPass {
public:
    virtual void execute(RenderContext& ctx) = 0;
};

class ShadowPass : public RenderPass {
    void execute(RenderContext& ctx) override {
        // 阴影渲染逻辑
    }
};

class GBufferPass : public RenderPass {
    void execute(RenderContext& ctx) override {
        // G-Buffer 填充
    }
};

class LightingPass : public RenderPass {
    void execute(RenderContext& ctx) override {
        // 光照计算
    }
};

// 使用
renderer->addPass(std::make_unique<ShadowPass>());
renderer->addPass(std::make_unique<GBufferPass>());
renderer->addPass(std::make_unique<LightingPass>());
```

---
*最后更新：2026-03-26*
