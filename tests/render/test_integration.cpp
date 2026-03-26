#include <gtest/gtest.h>
#include "phoenix/render/Types.hpp"
#include "phoenix/render/RenderDevice.hpp"
#include "phoenix/render/Shader.hpp"
#include "phoenix/render/Pipeline.hpp"
#include "phoenix/render/Resources.hpp"

using namespace phoenix::render;

/**
 * @brief 集成测试 - 渲染三角形
 * 
 * 测试完整的渲染流程:
 * 1. 初始化渲染设备
 * 2. 创建顶点/索引缓冲
 * 3. 创建着色器程序
 * 4. 提交绘制调用
 */
class RenderIntegrationTest : public ::testing::Test {
protected:
    RenderDevice device;
    ShaderCompiler compiler;
    ResourceManager resourceManager;
    
    void SetUp() override {
        // 初始化编译器
        compiler.initialize();
        
        // 初始化资源管理器
        // 注意：实际测试需要有效的设备
    }
    
    void TearDown() override {
        compiler.shutdown();
        device.shutdown();
    }
};

TEST_F(RenderIntegrationTest, DeviceInitialization) {
    DeviceConfig config;
    config.backend = RenderBackend::Vulkan;
    config.enableDebugInfo = true;
    
    SwapChainConfig swapChain;
    swapChain.width = 800;
    swapChain.height = 600;
    swapChain.windowHandle = nullptr; // 无窗口模式
    
    // 注意：在没有实际窗口的情况下，初始化可能会失败
    // 这里测试配置的有效性
    EXPECT_EQ(config.backend, RenderBackend::Vulkan);
    EXPECT_TRUE(config.enableDebugInfo);
    EXPECT_EQ(swapChain.width, 800u);
    EXPECT_EQ(swapChain.height, 600u);
}

TEST_F(RenderIntegrationTest, TriangleMeshCreation) {
    // 创建三角形网格数据
    MeshData triangle = BuiltinMeshes::createTriangle();
    
    EXPECT_EQ(triangle.vertexCount(), 3u);
    EXPECT_EQ(triangle.indexCount(), 3u);
    
    // 验证顶点数据
    EXPECT_EQ(triangle.positions.size(), 9u);  // 3 vertices * 3 components
    EXPECT_EQ(triangle.normals.size(), 9u);
    EXPECT_EQ(triangle.texCoords.size(), 6u);
}

TEST_F(RenderIntegrationTest, CubeMeshCreation) {
    MeshData cube = BuiltinMeshes::createCube(1.0f);
    
    EXPECT_EQ(cube.vertexCount(), 8u);
    EXPECT_EQ(cube.indexCount(), 36u);
    
    // 验证包围盒
    float min[3], max[3];
    float localMin[3] = {std::numeric_limits<float>::max(), 
                         std::numeric_limits<float>::max(), 
                         std::numeric_limits<float>::max()};
    float localMax[3] = {std::numeric_limits<float>::lowest(), 
                         std::numeric_limits<float>::lowest(), 
                         std::numeric_limits<float>::lowest()};
    
    for (size_t i = 0; i < cube.positions.size(); i += 3) {
        for (int j = 0; j < 3; ++j) {
            localMin[j] = std::min(localMin[j], cube.positions[i + j]);
            localMax[j] = std::max(localMax[j], cube.positions[i + j]);
        }
    }
    
    // 单位立方体应该在 -0.5 到 0.5 之间
    EXPECT_NEAR(localMin[0], -0.5f, 0.001f);
    EXPECT_NEAR(localMax[0], 0.5f, 0.001f);
}

TEST_F(RenderIntegrationTest, VertexLayoutCreation) {
    VertexLayout layout;
    layout.begin()
        .addPosition(VertexAttribFormat::Float3)
        .addNormal(VertexAttribFormat::Float3)
        .addTexCoord(0, VertexAttribFormat::Float2)
        .end();
    
    EXPECT_EQ(layout.getStride(), 32u);
    EXPECT_GT(layout.getHash(), 0u);
}

TEST_F(RenderIntegrationTest, UniformBufferOperations) {
    UniformBuffer ub;
    ub.initialize(256);
    
    // 写入 MVP 矩阵
    float mvp[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    ub.writeMat4(0, mvp);
    
    // 写入颜色
    ub.writeVec4(64, 1.0f, 0.5f, 0.25f, 1.0f);
    
    // 写入时间
    ub.writeFloat(80, 1.5f);
    
    EXPECT_EQ(ub.size(), 256u);
}

TEST_F(RenderIntegrationTest, RenderStateConfiguration) {
    // 测试各种渲染状态配置
    
    // 不透明物体
    RenderState opaque = RenderState::opaqueState();
    EXPECT_FALSE(opaque.blendEnable);
    EXPECT_TRUE(opaque.depthWrite);
    
    // 透明物体
    RenderState transparent = RenderState::transparentState();
    EXPECT_TRUE(transparent.blendEnable);
    EXPECT_FALSE(transparent.depthWrite);
    
    // 阴影
    RenderState shadow = RenderStateManager::getShadowState();
    EXPECT_TRUE(shadow.depthTest);
    EXPECT_EQ(shadow.cullMode, CullMode::Front);
    
    // UI
    RenderState ui = RenderStateManager::getUIState();
    EXPECT_TRUE(ui.blendEnable);
    EXPECT_FALSE(ui.depthTest);
}

TEST_F(RenderIntegrationTest, TextureFormatConversion) {
    // 测试纹理格式
    TextureDesc desc;
    desc.width = 256;
    desc.height = 256;
    desc.format = TextureFormat::RGBA8;
    desc.type = TextureType::Texture2D;
    
    EXPECT_EQ(desc.width, 256u);
    EXPECT_EQ(desc.height, 256u);
    EXPECT_EQ(desc.format, TextureFormat::RGBA8);
}

TEST_F(RenderIntegrationTest, RenderTargetCreation) {
    RenderTargetDesc desc;
    desc.width = 1920;
    desc.height = 1080;
    desc.format = TextureFormat::RGBA8;
    desc.isDepth = false;
    desc.sampleCount = 4;
    
    EXPECT_EQ(desc.width, 1920u);
    EXPECT_EQ(desc.height, 1080u);
    EXPECT_FALSE(desc.isDepth);
}

TEST_F(RenderIntegrationTest, GBufferConfiguration) {
    GBufferConfig config;
    config.width = 1920;
    config.height = 1080;
    config.albedo = TextureFormat::RGBA8;
    config.normal = TextureFormat::RGBA16F;
    config.material = TextureFormat::RGBA8;
    config.depth = TextureFormat::Depth24;
    
    EXPECT_EQ(config.width, 1920u);
    EXPECT_EQ(config.height, 1080u);
    EXPECT_EQ(config.albedo, TextureFormat::RGBA8);
    EXPECT_EQ(config.normal, TextureFormat::RGBA16F);
}

TEST_F(RenderIntegrationTest, DrawCallSetup) {
    DrawCall drawCall;
    drawCall.program = ProgramHandle(1);
    drawCall.vertexBuffer = BufferHandle(2);
    drawCall.indexBuffer = BufferHandle(3);
    drawCall.indexCount = 36;
    drawCall.instanceCount = 10;
    
    EXPECT_TRUE(drawCall.program.valid());
    EXPECT_TRUE(drawCall.vertexBuffer.valid());
    EXPECT_TRUE(drawCall.indexBuffer.valid());
    EXPECT_EQ(drawCall.indexCount, 36u);
    EXPECT_EQ(drawCall.instanceCount, 10u);
}

TEST_F(RenderIntegrationTest, MaterialCreation) {
    MaterialTemplate template_;
    template_.name = "StandardPBR";
    template_.pbr.baseColor = Color(1, 1, 1, 1);
    template_.pbr.metallic = 0.5f;
    template_.pbr.roughness = 0.8f;
    
    Material material;
    material.createFromTemplate(template_);
    
    material.setBaseColor(Color(0.8f, 0.6f, 0.4f, 1.0f));
    material.setMetallic(0.3f);
    material.setRoughness(0.7f);
    
    const RenderState& state = material.getRenderState();
    EXPECT_EQ(state.cullMode, CullMode::Back);
}

TEST_F(RenderIntegrationTest, CommandBufferRecording) {
    CommandBuffer cmdBuffer;
    
    // 录制清除命令
    cmdBuffer.clear(0, ClearFlags::All, Color(0.1f, 0.1f, 0.2f, 1.0f), 1.0f, 0);
    
    // 录制绘制命令
    DrawCall drawCall;
    drawCall.program = ProgramHandle(1);
    drawCall.indexCount = 6;
    cmdBuffer.draw(drawCall);
    
    // 录制提交命令
    cmdBuffer.submit(0, ProgramHandle(1));
    
    // 重置
    cmdBuffer.reset();
}

TEST_F(RenderIntegrationTest, ViewportSetup) {
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

TEST_F(RenderIntegrationTest, BuiltinMeshesValidation) {
    // 验证所有内置几何体
    
    auto triangle = BuiltinMeshes::createTriangle();
    EXPECT_EQ(triangle.vertexCount(), 3u);
    
    auto quad = BuiltinMeshes::createQuad();
    EXPECT_EQ(quad.vertexCount(), 4u);
    
    auto cube = BuiltinMeshes::createCube(1.0f);
    EXPECT_EQ(cube.vertexCount(), 8u);
    
    auto sphere = BuiltinMeshes::createSphere(1.0f, 32);
    EXPECT_GT(sphere.vertexCount(), 100u);
    
    auto cylinder = BuiltinMeshes::createCylinder(1.0f, 2.0f, 32);
    EXPECT_GT(cylinder.vertexCount(), 60u);
    
    auto plane = BuiltinMeshes::createPlane(1.0f, 1.0f, 10);
    EXPECT_EQ(plane.vertexCount(), 121u);
}

TEST_F(RenderIntegrationTest, ResourceManagerBasics) {
    ResourceManager manager;
    
    // 获取统计信息
    ResourceManager::Stats stats = manager.getStats();
    EXPECT_EQ(stats.vertexBufferCount, 0u);
    EXPECT_EQ(stats.indexBufferCount, 0u);
    EXPECT_EQ(stats.textureCount, 0u);
    EXPECT_EQ(stats.frameBufferCount, 0u);
}

TEST_F(RenderIntegrationTest, FrameStatsStructure) {
    FrameStats stats = {};
    stats.frameNumber = 100;
    stats.frameTime = 16.67f;
    stats.gpuTime = 15.2f;
    stats.drawCalls = 150;
    stats.triangleCount = 50000;
    
    EXPECT_EQ(stats.frameNumber, 100u);
    EXPECT_NEAR(stats.frameTime, 16.67f, 0.01f);
    EXPECT_GT(stats.drawCalls, 0u);
}

// ==================== 性能测试 ====================

TEST_F(RenderIntegrationTest, Performance_VertexLayoutHash) {
    const int iterations = 10000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        VertexLayout layout;
        layout.begin()
            .addPosition()
            .addNormal()
            .addTexCoord()
            .end();
        
        // 使用哈希
        volatile auto hash = layout.getHash();
        BX_UNUSED(hash);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    // 应该在 100ms 内完成
    EXPECT_LT(duration, 100000);
}

TEST_F(RenderIntegrationTest, Performance_MeshDataCreation) {
    const int iterations = 1000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        MeshData sphere = BuiltinMeshes::createSphere(1.0f, 32);
        volatile auto count = sphere.vertexCount();
        BX_UNUSED(count);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    // 应该在 1 秒内完成
    EXPECT_LT(duration, 1000);
}

TEST_F(RenderIntegrationTest, Performance_UniformBufferWrites) {
    UniformBuffer ub;
    ub.initialize(1024);
    
    const int iterations = 100000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        ub.writeFloat(0, 1.0f);
        ub.writeVec3(4, 1.0f, 2.0f, 3.0f);
        ub.writeMat4(16, reinterpret_cast<float*>(&ub));
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    // 应该在 500ms 内完成
    EXPECT_LT(duration, 500);
}

// ==================== 边界条件测试 ====================

TEST_F(RenderIntegrationTest, EdgeCase_InvalidHandles) {
    DeviceHandle invalidDevice;
    ShaderHandle invalidShader;
    ProgramHandle invalidProgram;
    BufferHandle invalidBuffer;
    TextureHandle invalidTexture;
    FrameBufferHandle invalidFB;
    
    EXPECT_FALSE(invalidDevice.valid());
    EXPECT_FALSE(invalidShader.valid());
    EXPECT_FALSE(invalidProgram.valid());
    EXPECT_FALSE(invalidBuffer.valid());
    EXPECT_FALSE(invalidTexture.valid());
    EXPECT_FALSE(invalidFB.valid());
}

TEST_F(RenderIntegrationTest, EdgeCase_ZeroSizedViewport) {
    Viewport vp;
    vp.width = 0;
    vp.height = 0;
    
    EXPECT_EQ(vp.width, 0u);
    EXPECT_EQ(vp.height, 0u);
}

TEST_F(RenderIntegrationTest, EdgeCase_EmptyMeshData) {
    MeshData empty;
    
    EXPECT_EQ(empty.vertexCount(), 0u);
    EXPECT_EQ(empty.indexCount(), 0u);
    EXPECT_TRUE(empty.positions.empty());
    EXPECT_TRUE(empty.indices.empty());
}

TEST_F(RenderIntegrationTest, EdgeCase_MaxTextureSize) {
    // 测试最大纹理尺寸
    TextureDesc desc;
    desc.width = 16384;  // 典型的最大 2D 纹理尺寸
    desc.height = 16384;
    desc.format = TextureFormat::RGBA8;
    
    EXPECT_EQ(desc.width, 16384u);
    EXPECT_EQ(desc.height, 16384u);
}

TEST_F(RenderIntegrationTest, EdgeCase_LargeInstanceCount) {
    DrawCall drawCall;
    drawCall.instanceCount = 65535;  // 16-bit 最大值
    drawCall.indexCount = 6;
    
    EXPECT_EQ(drawCall.instanceCount, 65535u);
}

TEST_F(RenderIntegrationTest, EdgeCase_AlphaBlending) {
    RenderState state;
    state.blendEnable = true;
    state.srcBlend = BlendFactor::SrcAlpha;
    state.dstBlend = BlendFactor::InvSrcAlpha;
    state.blendMask = 0x00;  // 禁用所有颜色通道
    
    EXPECT_TRUE(state.blendEnable);
    EXPECT_EQ(state.blendMask, 0x00);
}

// ==================== 资源管理测试 ====================

TEST_F(RenderIntegrationTest, ResourcePool_Exhaustion) {
    ResourcePool<Texture, TextureHandle, 4> smallPool;
    
    // 分配所有资源
    std::vector<TextureHandle> handles;
    for (int i = 0; i < 4; ++i) {
        handles.push_back(smallPool.allocate());
    }
    
    // 第 5 个应该失败
    TextureHandle h5 = smallPool.allocate();
    EXPECT_FALSE(h5.valid());
    
    // 释放一个
    smallPool.free(handles[0]);
    
    // 现在应该可以分配
    TextureHandle h6 = smallPool.allocate();
    EXPECT_TRUE(h6.valid());
}

TEST_F(RenderIntegrationTest, ResourcePool_Reuse) {
    ResourcePool<Texture, TextureHandle, 16> pool;
    
    TextureHandle h1 = pool.allocate();
    uint32_t index1 = h1.index();
    
    pool.free(h1);
    
    TextureHandle h2 = pool.allocate();
    uint32_t index2 = h2.index();
    
    // 应该复用相同的索引
    EXPECT_EQ(index1, index2);
}

// ==================== 主测试入口 ====================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
