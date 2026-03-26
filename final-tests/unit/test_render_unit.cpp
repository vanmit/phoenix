/**
 * Phoenix Engine - Render System Unit Tests
 * 
 * 测试渲染系统核心功能
 * 目标覆盖率：>90%
 */

#include <gtest/gtest.h>
#include <vector>
#include <memory>

// Phoenix Engine Render 头文件
#include "phoenix/render/device.hpp"
#include "phoenix/render/pipeline.hpp"
#include "phoenix/render/resource.hpp"
#include "phoenix/render/shader.hpp"
#include "phoenix/render/texture.hpp"
#include "phoenix/render/buffer.hpp"
#include "phoenix/render/draw_call.hpp"

using namespace phoenix::render;

// ============================================================================
// Device 测试
// ============================================================================

TEST(DeviceTest, Creation) {
    DeviceConfig config;
    config.width = 1920;
    config.height = 1080;
    config.vsync = true;
    
    auto device = Device::create(config);
    
    EXPECT_NE(device, nullptr);
    EXPECT_EQ(device->getWidth(), 1920);
    EXPECT_EQ(device->getHeight(), 1080);
}

TEST(DeviceTest, Swapchain) {
    DeviceConfig config;
    config.width = 1920;
    config.height = 1080;
    
    auto device = Device::create(config);
    
    EXPECT_TRUE(device->hasSwapchain());
    EXPECT_EQ(device->getSwapchainFormat(), TextureFormat::RGBA8);
}

TEST(DeviceTest, Resize) {
    DeviceConfig config;
    config.width = 1920;
    config.height = 1080;
    
    auto device = Device::create(config);
    
    device->resize(2560, 1440);
    
    EXPECT_EQ(device->getWidth(), 2560);
    EXPECT_EQ(device->getHeight(), 1440);
}

// ============================================================================
// Pipeline 测试
// ============================================================================

TEST(PipelineTest, GraphicsPipelineCreation) {
    GraphicsPipelineConfig config;
    config.vertexShader = "shaders/vertex.spv";
    config.fragmentShader = "shaders/fragment.spv";
    config.primitiveTopology = PrimitiveTopology::TriangleList;
    config.depthTest = true;
    config.blendEnable = false;
    
    auto pipeline = GraphicsPipeline::create(config);
    
    EXPECT_NE(pipeline, nullptr);
    EXPECT_TRUE(pipeline->isValid());
}

TEST(PipelineTest, ComputePipelineCreation) {
    ComputePipelineConfig config;
    config.computeShader = "shaders/compute.spv";
    config.workGroupSize = {64, 1, 1};
    
    auto pipeline = ComputePipeline::create(config);
    
    EXPECT_NE(pipeline, nullptr);
    EXPECT_TRUE(pipeline->isValid());
}

TEST(PipelineTest, PipelineLayout) {
    PipelineLayoutConfig layoutConfig;
    layoutConfig.addBinding(0, DescriptorType::UniformBuffer);
    layoutConfig.addBinding(1, DescriptorType::Sampler);
    layoutConfig.addBinding(2, DescriptorType::SampledImage);
    
    auto layout = PipelineLayout::create(layoutConfig);
    
    EXPECT_NE(layout, nullptr);
    EXPECT_EQ(layout->getBindingCount(), 3);
}

// ============================================================================
// Resource 测试
// ============================================================================

TEST(ResourceTest, BufferCreation) {
    BufferConfig config;
    config.size = 1024;
    config.usage = BufferUsage::Vertex | BufferUsage::TransferDst;
    config.memoryType = MemoryType::DeviceLocal;
    
    auto buffer = Buffer::create(config);
    
    EXPECT_NE(buffer, nullptr);
    EXPECT_EQ(buffer->getSize(), 1024);
    EXPECT_TRUE(buffer->hasUsage(BufferUsage::Vertex));
}

TEST(ResourceTest, BufferUpload) {
    BufferConfig config;
    config.size = 256;
    config.usage = BufferUsage::TransferDst;
    
    auto buffer = Buffer::create(config);
    
    std::vector<uint8_t> data(256, 0xAB);
    buffer->upload(data.data(), data.size());
    
    // 验证上传成功（需要实现 download 方法）
    EXPECT_TRUE(buffer->isValid());
}

TEST(ResourceTest, TextureCreation) {
    TextureConfig config;
    config.width = 1024;
    config.height = 1024;
    config.format = TextureFormat::RGBA8;
    config.usage = TextureUsage::Sampled | TextureUsage::TransferDst;
    config.mipLevels = 10;
    
    auto texture = Texture::create(config);
    
    EXPECT_NE(texture, nullptr);
    EXPECT_EQ(texture->getWidth(), 1024);
    EXPECT_EQ(texture->getHeight(), 1024);
    EXPECT_EQ(texture->getMipLevels(), 10);
}

TEST(ResourceTest, TextureUpload) {
    TextureConfig config;
    config.width = 256;
    config.height = 256;
    config.format = TextureFormat::RGBA8;
    
    auto texture = Texture::create(config);
    
    std::vector<uint8_t> data(256 * 256 * 4, 0xFF);
    texture->upload(data.data(), data.size());
    
    EXPECT_TRUE(texture->isValid());
}

TEST(ResourceTest, TextureMipmaps) {
    TextureConfig config;
    config.width = 256;
    config.height = 256;
    config.format = TextureFormat::RGBA8;
    config.mipLevels = 8;
    config.generateMipmaps = true;
    
    auto texture = Texture::create(config);
    
    EXPECT_EQ(texture->getMipLevels(), 8);
    EXPECT_TRUE(texture->hasMipmaps());
}

// ============================================================================
// Shader 测试
// ============================================================================

TEST(ShaderTest, VertexShaderCompilation) {
    const char* vertexSource = R"(
        #version 450
        layout(location = 0) in vec3 position;
        layout(location = 1) in vec3 color;
        layout(location = 0) out vec3 fragColor;
        
        void main() {
            gl_Position = vec4(position, 1.0);
            fragColor = color;
        }
    )";
    
    ShaderConfig config;
    config.source = vertexSource;
    config.stage = ShaderStage::Vertex;
    
    auto shader = Shader::create(config);
    
    EXPECT_NE(shader, nullptr);
    EXPECT_TRUE(shader->isValid());
}

TEST(ShaderTest, FragmentShaderCompilation) {
    const char* fragmentSource = R"(
        #version 450
        layout(location = 0) in vec3 fragColor;
        layout(location = 0) out vec4 outColor;
        
        void main() {
            outColor = vec4(fragColor, 1.0);
        }
    )";
    
    ShaderConfig config;
    config.source = fragmentSource;
    config.stage = ShaderStage::Fragment;
    
    auto shader = Shader::create(config);
    
    EXPECT_NE(shader, nullptr);
    EXPECT_TRUE(shader->isValid());
}

TEST(ShaderTest, ComputeShaderCompilation) {
    const char* computeSource = R"(
        #version 450
        layout(local_size_x = 64) in;
        
        layout(std430, binding = 0) buffer Data {
            float data[];
        };
        
        void main() {
            uint idx = gl_GlobalInvocationID.x;
            data[idx] = float(idx);
        }
    )";
    
    ShaderConfig config;
    config.source = computeSource;
    config.stage = ShaderStage::Compute;
    
    auto shader = Shader::create(config);
    
    EXPECT_NE(shader, nullptr);
    EXPECT_TRUE(shader->isValid());
}

TEST(ShaderTest, ShaderReflection) {
    const char* source = R"(
        #version 450
        layout(location = 0) in vec3 position;
        layout(location = 1) in vec2 texCoord;
        layout(location = 0) out vec2 fragTexCoord;
        
        layout(set = 0, binding = 0) uniform Transform {
            mat4 modelViewProjection;
        };
        
        void main() {
            gl_Position = modelViewProjection * vec4(position, 1.0);
            fragTexCoord = texCoord;
        }
    )";
    
    ShaderConfig config;
    config.source = source;
    config.stage = ShaderStage::Vertex;
    
    auto shader = Shader::create(config);
    
    auto reflection = shader->reflect();
    
    EXPECT_EQ(reflection.inputBindings.size(), 2);
    EXPECT_EQ(reflection.uniformBindings.size(), 1);
}

// ============================================================================
// DrawCall 测试
// ============================================================================

TEST(DrawCallTest, BasicDraw) {
    DrawCall drawCall;
    drawCall.setPrimitiveType(PrimitiveType::Triangles);
    drawCall.setVertexCount(3);
    drawCall.setInstanceCount(1);
    
    EXPECT_EQ(drawCall.getVertexCount(), 3);
    EXPECT_EQ(drawCall.getInstanceCount(), 1);
}

TEST(DrawCallTest, IndexedDraw) {
    DrawCall drawCall;
    drawCall.setPrimitiveType(PrimitiveType::Triangles);
    drawCall.setIndexCount(36);
    drawCall.setIndexType(IndexType::UInt16);
    
    EXPECT_EQ(drawCall.getIndexCount(), 36);
    EXPECT_EQ(drawCall.getIndexType(), IndexType::UInt16);
}

TEST(DrawCallTest, InstancedDraw) {
    DrawCall drawCall;
    drawCall.setPrimitiveType(PrimitiveType::Triangles);
    drawCall.setVertexCount(6);
    drawCall.setInstanceCount(100);
    
    EXPECT_EQ(drawCall.getVertexCount(), 6);
    EXPECT_EQ(drawCall.getInstanceCount(), 100);
}

TEST(DrawCallTest, IndirectDraw) {
    DrawCall drawCall;
    drawCall.setPrimitiveType(PrimitiveType::Triangles);
    drawCall.setIndirect(true);
    drawCall.setIndirectBuffer(Buffer::create(BufferConfig{1024, BufferUsage::Indirect}));
    
    EXPECT_TRUE(drawCall.isIndirect());
    EXPECT_NE(drawCall.getIndirectBuffer(), nullptr);
}

// ============================================================================
// 渲染状态测试
// ============================================================================

TEST(RenderStateTest, DepthStencilState) {
    DepthStencilStateConfig config;
    config.depthTestEnable = true;
    config.depthWriteEnable = true;
    config.depthCompareOp = CompareOp::Less;
    config.stencilTestEnable = false;
    
    auto state = DepthStencilState::create(config);
    
    EXPECT_NE(state, nullptr);
    EXPECT_TRUE(state->isDepthTestEnabled());
    EXPECT_TRUE(state->isDepthWriteEnabled());
}

TEST(RenderStateTest, BlendState) {
    BlendStateConfig config;
    config.blendEnable = true;
    config.srcColorBlendFactor = BlendFactor::SrcAlpha;
    config.dstColorBlendFactor = BlendFactor::OneMinusSrcAlpha;
    config.colorBlendOp = BlendOp::Add;
    
    auto state = BlendState::create(config);
    
    EXPECT_NE(state, nullptr);
    EXPECT_TRUE(state->isBlendEnabled());
}

TEST(RenderStateTest, RasterizerState) {
    RasterizerStateConfig config;
    config.cullMode = CullMode::Back;
    config.frontFace = FrontFace::CounterClockwise;
    config.lineWidth = 1.0f;
    
    auto state = RasterizerState::create(config);
    
    EXPECT_NE(state, nullptr);
    EXPECT_EQ(state->getCullMode(), CullMode::Back);
}

// ============================================================================
// 查询系统测试
// ============================================================================

TEST(QueryTest, OcclusionQuery) {
    QueryConfig config;
    config.type = QueryType::Occlusion;
    
    auto query = Query::create(config);
    
    EXPECT_NE(query, nullptr);
    EXPECT_EQ(query->getType(), QueryType::Occlusion);
}

TEST(QueryTest, TimestampQuery) {
    QueryConfig config;
    config.type = QueryType::Timestamp;
    
    auto query = Query::create(config);
    
    EXPECT_NE(query, nullptr);
    EXPECT_EQ(query->getType(), QueryType::Timestamp);
}

TEST(QueryTest, QueryPool) {
    QueryPoolConfig config;
    config.type = QueryType::Timestamp;
    config.count = 100;
    
    auto pool = QueryPool::create(config);
    
    EXPECT_NE(pool, nullptr);
    EXPECT_EQ(pool->getQueryCount(), 100);
}

// ============================================================================
// 同步对象测试
// ============================================================================

TEST(SyncTest, Fence) {
    auto fence = Fence::create();
    
    EXPECT_NE(fence, nullptr);
    EXPECT_FALSE(fence->isSignaled());
    
    fence->signal();
    EXPECT_TRUE(fence->isSignaled());
    
    fence->reset();
    EXPECT_FALSE(fence->isSignaled());
}

TEST(SyncTest, Semaphore) {
    auto semaphore = Semaphore::create();
    
    EXPECT_NE(semaphore, nullptr);
}

TEST(SyncTest, Barrier) {
    BarrierConfig config;
    config.srcAccessMask = AccessMask::ShaderWrite;
    config.dstAccessMask = AccessMask::ShaderRead;
    config.srcStageMask = PipelineStage::FragmentShader;
    config.dstStageMask = PipelineStage::VertexShader;
    
    auto barrier = Barrier::create(config);
    
    EXPECT_NE(barrier, nullptr);
}

// ============================================================================
// 主测试入口
// ============================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
