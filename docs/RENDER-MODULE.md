# Phoenix Engine - 渲染模块文档

## 概述

渲染模块是 Phoenix Engine 的核心组件，提供跨平台的图形渲染功能。

## 架构设计

```
┌─────────────────────────────────────────────────────────┐
│                    应用层 (Application)                  │
├─────────────────────────────────────────────────────────┤
│              渲染管线层 (Pipeline)                       │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │ Forward      │  │ Deferred     │  │  Compute     │  │
│  │ Renderer     │  │ Renderer     │  │  Pipeline    │  │
│  └──────────────┘  └──────────────┘  └──────────────┘  │
├─────────────────────────────────────────────────────────┤
│              资源管理层 (Resources)                      │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │  Buffers     │  │  Textures    │  │  Materials   │  │
│  └──────────────┘  └──────────────┘  └──────────────┘  │
├─────────────────────────────────────────────────────────┤
│              着色器系统 (Shader)                         │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │  SPIR-V      │  │  Hot Reload  │  │  Uniforms    │  │
│  └──────────────┘  └──────────────┘  └──────────────┘  │
├─────────────────────────────────────────────────────────┤
│              设备抽象层 (RenderDevice)                   │
│  ┌─────┐ ┌─────┐ ┌─────┐ ┌──────┐ ┌──────┐ ┌────────┐ │
│  │Vulkan│ │DX11 │ │Metal│ │OpenGL│ │WebGL2│ │  bgfx  │ │
│  └─────┘ └─────┘ └─────┘ └──────┘ └──────┘ └────────┘ │
└─────────────────────────────────────────────────────────┘
```

## 核心组件

### 1. RenderDevice (渲染设备)

渲染设备是 bgfx 的封装层，提供:
- 跨平台后端抽象 (Vulkan/DX11/Metal/OpenGL/WebGL)
- 强类型句柄系统 (防止悬空引用)
- 设备信息查询
- 交换链管理

**使用示例**:
```cpp
#include "phoenix/render/RenderDevice.hpp"

using namespace phoenix::render;

// 创建设备
RenderDevice device;

// 配置
DeviceConfig config;
config.backend = RenderBackend::Vulkan;
config.enableDebugInfo = true;

SwapChainConfig swapChain;
swapChain.width = 1920;
swapChain.height = 1080;
swapChain.windowHandle = nativeWindowHandle;

// 初始化
if (!device.initialize(config, swapChain)) {
    // 错误处理
}

// 渲染循环
while (running) {
    device.beginFrame(0);
    device.clear(0, ClearFlags::All, Color(0, 0, 0, 1));
    // ... 绘制调用
    FrameStats stats = device.endFrame(true);
}

// 关闭
device.shutdown();
```

### 2. 强类型句柄系统

防止悬空引用和类型混淆:

```cpp
// 句柄类型
DeviceHandle      device;      // 交换链/设备
ShaderHandle      shader;      // 着色器
ProgramHandle     program;     // 着色器程序
BufferHandle      buffer;      // 顶点/索引缓冲
TextureHandle     texture;     // 纹理
FrameBufferHandle framebuffer; // 帧缓冲

// 安全检查
if (texture.valid()) {
    // 使用纹理
}

// 防止类型混淆
device.createSwapChain(config);  // 返回 DeviceHandle
// device.createSwapChain(config) != TextureHandle (编译错误)
```

### 3. 着色器系统

支持 SPIR-V 编译和热重载:

```cpp
#include "phoenix/render/Shader.hpp"

ShaderCompiler compiler;
compiler.initialize();

// 从 SPIR-V 创建着色器
std::vector<uint32_t> spirv = loadSpirvFile("shader.spv");
ShaderHandle vs = compiler.createShaderFromSpirv(device, spirv, ShaderStage::Vertex);
ShaderHandle fs = compiler.createShaderFromSpirv(device, spirv2, ShaderStage::Fragment);

// 创建程序
ProgramHandle program = compiler.createProgram(device, vs, fs);

// 热重载
compiler.registerHotReloadCallback(vs, [](ShaderHandle h, bool success) {
    if (success) {
        std::cout << "Shader reloaded successfully\n";
    }
});
compiler.hotReloadShader(vs, "new_shader.spv");
```

### 4. 渲染管线

#### 前向渲染器

```cpp
ForwardRenderer renderer;
renderer.initialize(device);

renderer.beginFrame();

DrawCall drawCall;
drawCall.program = program;
drawCall.vertexBuffer = vertexBuffer->getHandle();
drawCall.indexBuffer = indexBuffer->getHandle();
drawCall.indexCount = 36;

renderer.addDrawCall(drawCall);
renderer.endFrame();
```

#### 延迟渲染器

```cpp
DeferredRenderer renderer;

GBufferConfig config;
config.width = 1920;
config.height = 1080;
renderer.initialize(device, config);

// 几何通道
renderer.beginGeometryPass();
renderer.addGeometryDrawCall(drawCall);
renderer.endGeometryPass();

// 光照通道
renderer.beginLightingPass();
renderer.addPointLight(position, color, radius, intensity);
renderer.endLightingPass();

// 获取最终输出
TextureHandle output = renderer.getOutputTexture();
```

### 5. 资源管理

#### 顶点/索引缓冲

```cpp
#include "phoenix/render/Resources.hpp"

ResourceManager resourceManager;
resourceManager.initialize(device);

// 创建顶点布局
VertexLayout layout;
layout.begin()
    .addPosition(VertexAttribFormat::Float3)
    .addNormal(VertexAttribFormat::Float3)
    .addTexCoord(0, VertexAttribFormat::Float2)
    .end();

// 创建顶点缓冲
BufferDesc vbDesc;
vbDesc.type = BufferType::Vertex;
vbDesc.size = vertexDataSize;
vbDesc.data = vertexData;
vbDesc.stride = layout.getStride();

VertexBuffer* vb = resourceManager.createVertexBuffer(vbDesc, layout);

// 创建索引缓冲
BufferDesc ibDesc;
ibDesc.type = BufferType::Index32;
ibDesc.size = indexDataSize;
ibDesc.data = indexData;

IndexBuffer* ib = resourceManager.createIndexBuffer(ibDesc);
```

#### 纹理加载

```cpp
// 从文件加载
Texture* texture = resourceManager.loadTexture("textures/albedo.png");

// 从内存加载 (KTX2/DDS/PNG)
Texture* compressedTexture = resourceManager.loadTextureFromMemory(
    ktxData, ktxSize, ".ktx2"
);

// 创建立方体贴图
CubeTexture cubeMap;
cubeMap.create(device, 1024, TextureFormat::RGBA8, faceData);
```

#### 内置几何体

```cpp
// 创建基本几何体
MeshData triangle = BuiltinMeshes::createTriangle();
MeshData cube = BuiltinMeshes::createCube(1.0f);
MeshData sphere = BuiltinMeshes::createSphere(1.0f, 32);
MeshData cylinder = BuiltinMeshes::createCylinder(1.0f, 2.0f, 32);
MeshData plane = BuiltinMeshes::createPlane(10.0f, 10.0f, 10);

// 创建网格
Mesh mesh;
mesh.create(device, cube);
```

### 6. 渲染状态管理

```cpp
// 使用预设状态
RenderState opaque = RenderState::opaqueState();
RenderState transparent = RenderState::transparentState();
RenderState shadow = RenderStateManager::getShadowState();
RenderState ui = RenderStateManager::getUIState();

// 自定义状态
RenderState custom;
custom.blendEnable = true;
custom.srcBlend = BlendFactor::SrcAlpha;
custom.dstBlend = BlendFactor::InvSrcAlpha;
custom.depthTest = true;
custom.depthWrite = false;
custom.cullMode = CullMode::Back;
```

### 7. 多线程命令缓冲

```cpp
CommandBufferPool pool(4); // 4 个缓冲

// 线程 1
CommandBuffer* cmd1 = pool.acquire();
cmd1->clear(0, ClearFlags::All, Color(0, 0, 0, 1));
cmd1->draw(drawCall1);

// 线程 2
CommandBuffer* cmd2 = pool.acquire();
cmd2->draw(drawCall2);
cmd2->draw(drawCall3);

// 主线程提交
// ... 处理命令缓冲

pool.release(cmd1);
pool.release(cmd2);
```

## 技术特性

### 零动态分配

帧间复用资源池:
```cpp
template<typename T, typename HandleType, size_t PoolSize = 256>
class ResourcePool {
    // 预分配资源数组
    std::array<T, PoolSize> resources_;
    // 空闲列表
    std::vector<index_type> freeList_;
    // 代次检查 (防悬空引用)
    std::array<uint16_t, PoolSize> generations_;
};
```

### 线程安全

- 命令缓冲池使用互斥锁
- 资源管理器使用互斥锁
- 无锁句柄系统

### WASM 支持

- 通过 bgfx WebGL2 后端
- Emscripten 编译配置
- 零成本抽象

## 性能优化

1. **批处理**: 合并相同材质的绘制调用
2. **实例化**: 支持 GPU 实例化渲染
3. **多线程**: 命令缓冲并行录制
4. **资源池**: 帧间复用，避免动态分配
5. **延迟渲染**: G-Buffer 配置

## 构建说明

```bash
# 创建构建目录
mkdir build && cd build

# 配置
cmake .. -DCMAKE_BUILD_TYPE=Release

# 构建
cmake --build . -j8

# 运行测试
ctest --output-on-failure

# 运行示例
./bin/basic-render
```

## API 参考

详细 API 文档请参考:
- `include/phoenix/render/Types.hpp` - 类型定义
- `include/phoenix/render/RenderDevice.hpp` - 渲染设备
- `include/phoenix/render/Shader.hpp` - 着色器系统
- `include/phoenix/render/Pipeline.hpp` - 渲染管线
- `include/phoenix/render/Resources.hpp` - 资源管理

## 示例程序

- `examples/basic-render/` - 基础渲染示例 (三角形/立方体)

## 单元测试

- `tests/render/test_render_device.cpp` - 渲染设备测试
- `tests/render/test_integration.cpp` - 集成测试

运行测试:
```bash
ctest -R RenderTests --output-on-failure
```

## 许可证

MIT License
