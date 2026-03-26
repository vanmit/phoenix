#include <gtest/gtest.h>
#include "phoenix/render/Types.hpp"
#include "phoenix/render/RenderDevice.hpp"
#include "phoenix/render/Shader.hpp"
#include "phoenix/render/Pipeline.hpp"
#include "phoenix/render/Resources.hpp"

using namespace phoenix::render;

// ==================== Handle 测试 ====================

TEST(HandleTest, DefaultConstruction) {
    DeviceHandle handle;
    EXPECT_FALSE(handle.valid());
    EXPECT_EQ(handle.index(), DeviceHandle::INVALID_INDEX);
}

TEST(HandleTest, ExplicitConstruction) {
    DeviceHandle handle(42);
    EXPECT_TRUE(handle.valid());
    EXPECT_EQ(handle.index(), 42u);
}

TEST(HandleTest, Equality) {
    DeviceHandle h1(10);
    DeviceHandle h2(10);
    DeviceHandle h3(20);
    
    EXPECT_EQ(h1, h2);
    EXPECT_NE(h1, h3);
}

TEST(HandleTest, Comparison) {
    DeviceHandle h1(10);
    DeviceHandle h2(20);
    
    EXPECT_LT(h1, h2);
    EXPECT_GT(h2, h1);
}

TEST(HandleTest, BoolConversion) {
    DeviceHandle invalid;
    DeviceHandle valid(0);
    
    EXPECT_FALSE(!invalid);
    EXPECT_TRUE(!valid);
}

// ==================== RenderState 测试 ====================

TEST(RenderStateTest, DefaultState) {
    RenderState state = RenderState::defaultState();
    
    EXPECT_FALSE(state.blendEnable);
    EXPECT_TRUE(state.depthTest);
    EXPECT_TRUE(state.depthWrite);
    EXPECT_EQ(state.depthFunc, DepthFunc::LessEqual);
    EXPECT_EQ(state.cullMode, CullMode::Back);
}

TEST(RenderStateTest, OpaqueState) {
    RenderState state = RenderState::opaqueState();
    
    EXPECT_FALSE(state.blendEnable);
    EXPECT_TRUE(state.depthTest);
    EXPECT_TRUE(state.depthWrite);
    EXPECT_EQ(state.cullMode, CullMode::Back);
}

TEST(RenderStateTest, TransparentState) {
    RenderState state = RenderState::transparentState();
    
    EXPECT_TRUE(state.blendEnable);
    EXPECT_EQ(state.srcBlend, BlendFactor::SrcAlpha);
    EXPECT_EQ(state.dstBlend, BlendFactor::InvSrcAlpha);
    EXPECT_TRUE(state.depthTest);
    EXPECT_FALSE(state.depthWrite);
}

// ==================== Color 测试 ====================

TEST(ColorTest, DefaultConstruction) {
    Color c;
    EXPECT_FLOAT_EQ(c.r, 0.0f);
    EXPECT_FLOAT_EQ(c.g, 0.0f);
    EXPECT_FLOAT_EQ(c.b, 0.0f);
    EXPECT_FLOAT_EQ(c.a, 1.0f);
}

TEST(ColorTest, ParameterizedConstruction) {
    Color c(0.5f, 0.25f, 0.75f, 0.8f);
    EXPECT_FLOAT_EQ(c.r, 0.5f);
    EXPECT_FLOAT_EQ(c.g, 0.25f);
    EXPECT_FLOAT_EQ(c.b, 0.75f);
    EXPECT_FLOAT_EQ(c.a, 0.8f);
}

TEST(ColorTest, PresetColors) {
    Color black = Color::black();
    EXPECT_FLOAT_EQ(black.r, 0.0f);
    EXPECT_FLOAT_EQ(black.g, 0.0f);
    EXPECT_FLOAT_EQ(black.b, 0.0f);
    
    Color white = Color::white();
    EXPECT_FLOAT_EQ(white.r, 1.0f);
    EXPECT_FLOAT_EQ(white.g, 1.0f);
    EXPECT_FLOAT_EQ(white.b, 1.0f);
}

// ==================== VertexLayout 测试 ====================

TEST(VertexLayoutTest, BasicLayout) {
    VertexLayout layout;
    layout.begin()
        .addPosition(VertexAttribFormat::Float3)
        .addNormal(VertexAttribFormat::Float3)
        .addTexCoord(0, VertexAttribFormat::Float2)
        .end();
    
    EXPECT_EQ(layout.getStride(), 32u); // 12 + 12 + 8
    EXPECT_GT(layout.getHash(), 0u);
}

TEST(VertexLayoutTest, PBRLayout) {
    VertexLayout layout;
    layout.begin()
        .addPosition(VertexAttribFormat::Float3)
        .addNormal(VertexAttribFormat::Float3)
        .addTangent(VertexAttribFormat::Float3)
        .addTexCoord(0, VertexAttribFormat::Float2)
        .addTexCoord(1, VertexAttribFormat::Float2)
        .addColor(0, VertexAttribFormat::Float4)
        .end();
    
    EXPECT_EQ(layout.getStride(), 56u); // 12+12+12+8+8+16
}

// ==================== Viewport 测试 ====================

TEST(ViewportTest, DefaultConstruction) {
    Viewport vp;
    EXPECT_EQ(vp.x, 0u);
    EXPECT_EQ(vp.y, 0u);
    EXPECT_EQ(vp.width, 0u);
    EXPECT_EQ(vp.height, 0u);
    EXPECT_FLOAT_EQ(vp.minDepth, 0.0f);
    EXPECT_FLOAT_EQ(vp.maxDepth, 1.0f);
}

TEST(ViewportTest, FullScreen) {
    Viewport vp;
    vp.x = 0;
    vp.y = 0;
    vp.width = 1920;
    vp.height = 1080;
    vp.minDepth = 0.0f;
    vp.maxDepth = 1.0f;
    
    EXPECT_EQ(vp.width, 1920u);
    EXPECT_EQ(vp.height, 1080u);
}

// ==================== ClearFlags 测试 ====================

TEST(ClearFlagsTest, IndividualFlags) {
    EXPECT_TRUE(static_cast<uint8_t>(ClearFlags::Color) & static_cast<uint8_t>(ClearFlags::Color));
    EXPECT_TRUE(static_cast<uint8_t>(ClearFlags::Depth) & static_cast<uint8_t>(ClearFlags::Depth));
    EXPECT_TRUE(static_cast<uint8_t>(ClearFlags::Stencil) & static_cast<uint8_t>(ClearFlags::Stencil));
}

TEST(ClearFlagsTest, CombinedFlags) {
    ClearFlags flags = ClearFlags::Color | ClearFlags::Depth;
    EXPECT_TRUE(flags & ClearFlags::Color);
    EXPECT_TRUE(flags & ClearFlags::Depth);
    EXPECT_FALSE(flags & ClearFlags::Stencil);
    
    ClearFlags all = ClearFlags::All;
    EXPECT_TRUE(all & ClearFlags::Color);
    EXPECT_TRUE(all & ClearFlags::Depth);
    EXPECT_TRUE(all & ClearFlags::Stencil);
}

// ==================== UniformBuffer 测试 ====================

TEST(UniformBufferTest, Initialization) {
    UniformBuffer ub;
    ub.initialize(256);
    
    EXPECT_EQ(ub.size(), 256u);
    EXPECT_NE(ub.data(), nullptr);
}

TEST(UniformBufferTest, WriteFloat) {
    UniformBuffer ub;
    ub.initialize(64);
    
    ub.writeFloat(0, 3.14f);
    ub.writeFloat(4, 2.71f);
    
    const float* data = static_cast<const float*>(ub.data());
    EXPECT_FLOAT_EQ(data[0], 3.14f);
    EXPECT_FLOAT_EQ(data[1], 2.71f);
}

TEST(UniformBufferTest, WriteVec3) {
    UniformBuffer ub;
    ub.initialize(64);
    
    ub.writeVec3(0, 1.0f, 2.0f, 3.0f);
    
    const float* data = static_cast<const float*>(ub.data());
    EXPECT_FLOAT_EQ(data[0], 1.0f);
    EXPECT_FLOAT_EQ(data[1], 2.0f);
    EXPECT_FLOAT_EQ(data[2], 3.0f);
}

TEST(UniformBufferTest, WriteMat4) {
    UniformBuffer ub;
    ub.initialize(128);
    
    float matrix[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    
    ub.writeMat4(0, matrix);
    
    const float* data = static_cast<const float*>(ub.data());
    for (int i = 0; i < 16; ++i) {
        EXPECT_FLOAT_EQ(data[i], matrix[i]);
    }
}

TEST(UniformBufferTest, Reset) {
    UniformBuffer ub;
    ub.initialize(64);
    ub.writeFloat(0, 1.0f);
    ub.reset();
    
    // 重置后应该可以重新写入
    ub.writeFloat(0, 2.0f);
    const float* data = static_cast<const float*>(ub.data());
    EXPECT_FLOAT_EQ(data[0], 2.0f);
}

// ==================== MeshData 测试 ====================

TEST(MeshDataTest, Triangle) {
    MeshData data = BuiltinMeshes::createTriangle();
    
    EXPECT_EQ(data.vertexCount(), 3u);
    EXPECT_EQ(data.indexCount(), 3u);
    EXPECT_EQ(data.positions.size(), 9u);  // 3 vertices * 3 components
    EXPECT_EQ(data.normals.size(), 9u);
    EXPECT_EQ(data.texCoords.size(), 6u);  // 3 vertices * 2 components
}

TEST(MeshDataTest, Quad) {
    MeshData data = BuiltinMeshes::createQuad();
    
    EXPECT_EQ(data.vertexCount(), 4u);
    EXPECT_EQ(data.indexCount(), 6u);  // 2 triangles
}

TEST(MeshDataTest, Cube) {
    MeshData data = BuiltinMeshes::createCube(2.0f);
    
    EXPECT_EQ(data.vertexCount(), 8u);
    EXPECT_EQ(data.indexCount(), 36u);  // 6 faces * 2 triangles * 3 indices
}

TEST(MeshDataTest, Sphere) {
    MeshData data = BuiltinMeshes::createSphere(1.0f, 16);
    
    EXPECT_GT(data.vertexCount(), 0u);
    EXPECT_GT(data.indexCount(), 0u);
    
    // 验证顶点在球面上 (半径为 1)
    for (size_t i = 0; i < data.positions.size(); i += 3) {
        float x = data.positions[i];
        float y = data.positions[i + 1];
        float z = data.positions[i + 2];
        float length = std::sqrt(x*x + y*y + z*z);
        EXPECT_NEAR(length, 1.0f, 0.001f);
    }
}

TEST(MeshDataTest, Cylinder) {
    MeshData data = BuiltinMeshes::createCylinder(1.0f, 2.0f, 32);
    
    EXPECT_GT(data.vertexCount(), 0u);
    EXPECT_GT(data.indexCount(), 0u);
}

TEST(MeshDataTest, Plane) {
    MeshData data = BuiltinMeshes::createPlane(10.0f, 10.0f, 10);
    
    EXPECT_EQ(data.vertexCount(), 121u);  // (10+1) * (10+1)
    EXPECT_GT(data.indexCount(), 0u);
}

// ==================== ResourcePool 测试 ====================

TEST(ResourcePoolTest, Allocation) {
    ResourcePool<Texture, TextureHandle, 16> pool;
    
    EXPECT_EQ(pool.available(), 16u);
    
    TextureHandle h1 = pool.allocate();
    EXPECT_TRUE(h1.valid());
    EXPECT_EQ(pool.available(), 15u);
    
    TextureHandle h2 = pool.allocate();
    EXPECT_TRUE(h2.valid());
    EXPECT_EQ(pool.available(), 14u);
}

TEST(ResourcePoolTest, Free) {
    ResourcePool<Texture, TextureHandle, 16> pool;
    
    TextureHandle h1 = pool.allocate();
    TextureHandle h2 = pool.allocate();
    
    EXPECT_EQ(pool.available(), 14u);
    
    pool.free(h1);
    EXPECT_EQ(pool.available(), 15u);
    
    pool.free(h2);
    EXPECT_EQ(pool.available(), 16u);
}

TEST(ResourcePoolTest, InvalidHandle) {
    ResourcePool<Texture, TextureHandle, 16> pool;
    
    TextureHandle invalid;
    EXPECT_FALSE(invalid.valid());
    
    // 分配所有资源
    std::vector<TextureHandle> handles;
    for (size_t i = 0; i < 16; ++i) {
        handles.push_back(pool.allocate());
    }
    
    // 第 17 个应该失败
    TextureHandle h17 = pool.allocate();
    EXPECT_FALSE(h17.valid());
}

TEST(ResourcePoolTest, Get) {
    ResourcePool<Texture, TextureHandle, 16> pool;
    
    TextureHandle h = pool.allocate();
    Texture* tex = pool.get(h);
    
    EXPECT_NE(tex, nullptr);
    
    // 修改资源
    tex->~Texture();
    new(tex) Texture();
}

// ==================== BoundingBox 测试 ====================

TEST(BoundingBoxTest, CubeBounds) {
    MeshData data = BuiltinMeshes::createCube(2.0f);
    
    float min[3], max[3];
    
    // 手动计算包围盒
    min[0] = min[1] = min[2] = std::numeric_limits<float>::max();
    max[0] = max[1] = max[2] = std::numeric_limits<float>::lowest();
    
    for (size_t i = 0; i < data.positions.size(); i += 3) {
        min[0] = std::min(min[0], data.positions[i]);
        min[1] = std::min(min[1], data.positions[i + 1]);
        min[2] = std::min(min[2], data.positions[i + 2]);
        max[0] = std::max(max[0], data.positions[i]);
        max[1] = std::max(max[1], data.positions[i + 1]);
        max[2] = std::max(max[2], data.positions[i + 2]);
    }
    
    EXPECT_FLOAT_EQ(min[0], -1.0f);
    EXPECT_FLOAT_EQ(min[1], -1.0f);
    EXPECT_FLOAT_EQ(min[2], -1.0f);
    EXPECT_FLOAT_EQ(max[0], 1.0f);
    EXPECT_FLOAT_EQ(max[1], 1.0f);
    EXPECT_FLOAT_EQ(max[2], 1.0f);
}

// ==================== RenderTarget 测试 ====================

TEST(RenderTargetDescTest, ColorTarget) {
    RenderTargetDesc desc;
    desc.width = 1920;
    desc.height = 1080;
    desc.format = TextureFormat::RGBA8;
    desc.isDepth = false;
    
    EXPECT_EQ(desc.width, 1920u);
    EXPECT_EQ(desc.height, 1080u);
    EXPECT_FALSE(desc.isDepth);
}

TEST(RenderTargetDescTest, DepthTarget) {
    RenderTargetDesc desc;
    desc.width = 1920;
    desc.height = 1080;
    desc.format = TextureFormat::Depth24;
    desc.isDepth = true;
    
    EXPECT_TRUE(desc.isDepth);
}

// ==================== GBufferConfig 测试 ====================

TEST(GBufferConfigTest, DefaultConfig) {
    GBufferConfig config;
    
    EXPECT_EQ(config.width, 0u);
    EXPECT_EQ(config.height, 0u);
    EXPECT_EQ(config.albedo, TextureFormat::RGBA8);
    EXPECT_EQ(config.normal, TextureFormat::RGBA16F);
    EXPECT_EQ(config.material, TextureFormat::RGBA8);
    EXPECT_TRUE(config.useMRT);
}

TEST(GBufferConfigTest, FullHD) {
    GBufferConfig config;
    config.width = 1920;
    config.height = 1080;
    
    EXPECT_EQ(config.width, 1920u);
    EXPECT_EQ(config.height, 1080u);
}

// ==================== DrawCall 测试 ====================

TEST(DrawCallTest, DefaultValues) {
    DrawCall drawCall;
    
    EXPECT_EQ(drawCall.firstVertex, 0u);
    EXPECT_EQ(drawCall.vertexCount, 0u);
    EXPECT_EQ(drawCall.firstIndex, 0u);
    EXPECT_EQ(drawCall.indexCount, 0u);
    EXPECT_EQ(drawCall.instanceCount, 1u);
    EXPECT_EQ(drawCall.material, nullptr);
    EXPECT_EQ(drawCall.uniforms, nullptr);
}

TEST(DrawCallTest, IndexedDraw) {
    DrawCall drawCall;
    drawCall.vertexBuffer = BufferHandle(1);
    drawCall.indexBuffer = BufferHandle(2);
    drawCall.program = ProgramHandle(3);
    drawCall.firstIndex = 0;
    drawCall.indexCount = 36;
    drawCall.instanceCount = 1;
    
    EXPECT_TRUE(drawCall.vertexBuffer.valid());
    EXPECT_TRUE(drawCall.indexBuffer.valid());
    EXPECT_TRUE(drawCall.program.valid());
    EXPECT_EQ(drawCall.indexCount, 36u);
}

TEST(DrawCallTest, InstancedDraw) {
    DrawCall drawCall;
    drawCall.instanceCount = 100;
    drawCall.indexCount = 6;
    
    EXPECT_EQ(drawCall.instanceCount, 100u);
}

// ==================== Rect 测试 ====================

TEST(RectTest, DefaultConstruction) {
    Rect rect;
    EXPECT_EQ(rect.x, 0u);
    EXPECT_EQ(rect.y, 0u);
    EXPECT_EQ(rect.width, 0u);
    EXPECT_EQ(rect.height, 0u);
}

TEST(RectTest, ScissorRect) {
    Rect rect;
    rect.x = 100;
    rect.y = 100;
    rect.width = 800;
    rect.height = 600;
    
    EXPECT_EQ(rect.x, 100u);
    EXPECT_EQ(rect.y, 100u);
    EXPECT_EQ(rect.width, 800u);
    EXPECT_EQ(rect.height, 600u);
}

// ==================== MaterialParam 测试 ====================

TEST(MaterialParamTest, FloatValue) {
    MaterialParam param;
    param.name = "u_time";
    param.type = UniformType::Float1;
    param.value.f[0] = 1.5f;
    
    EXPECT_EQ(param.name, "u_time");
    EXPECT_FLOAT_EQ(param.value.f[0], 1.5f);
}

TEST(MaterialParamTest, Vec4Value) {
    MaterialParam param;
    param.name = "u_color";
    param.type = UniformType::Float4;
    param.value.f[0] = 1.0f;
    param.value.f[1] = 0.5f;
    param.value.f[2] = 0.25f;
    param.value.f[3] = 0.8f;
    
    EXPECT_FLOAT_EQ(param.value.f[0], 1.0f);
    EXPECT_FLOAT_EQ(param.value.f[3], 0.8f);
}

// ==================== SpirvOptions 测试 ====================

TEST(SpirvOptionsTest, DefaultOptions) {
    SpirvOptions options;
    
    EXPECT_EQ(options.optimization, SpirvOptions::OptimizerLevel::Performance);
    EXPECT_FALSE(options.debugInfo);
    EXPECT_TRUE(options.stripDebugInfo);
    EXPECT_EQ(options.entryPoint, "main");
    EXPECT_EQ(options.stage, ShaderStage::Vertex);
}

TEST(SpirvOptionsTest, DebugOptions) {
    SpirvOptions options;
    options.optimization = SpirvOptions::OptimizerLevel::None;
    options.debugInfo = true;
    options.stripDebugInfo = false;
    options.defines.push_back("DEBUG=1");
    
    EXPECT_EQ(options.optimization, SpirvOptions::OptimizerLevel::None);
    EXPECT_TRUE(options.debugInfo);
    EXPECT_FALSE(options.stripDebugInfo);
}

// ==================== 主测试入口 ====================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
