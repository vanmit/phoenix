#pragma once

#include "Types.hpp"
#include "RenderDevice.hpp"
#include "Shader.hpp"
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>

namespace phoenix {
namespace render {

// 前向声明
class RenderPass;
class RenderTarget;

/**
 * @brief G-Buffer 配置
 */
struct GBufferConfig {
    uint32_t width = 0;
    uint32_t height = 0;
    TextureFormat albedo = TextureFormat::RGBA8;
    TextureFormat normal = TextureFormat::RGBA16F;
    TextureFormat material = TextureFormat::RGBA8; // [roughness, metallic, ao, emissive]
    TextureFormat depth = TextureFormat::Depth24;
    bool useMRT = true;
    uint32_t sampleCount = 1;
};

/**
 * @brief 渲染目标描述
 */
struct RenderTargetDesc {
    uint32_t width;
    uint32_t height;
    TextureFormat format;
    uint32_t sampleCount = 1;
    bool isDepth = false;
    bool isCube = false;
    bool isTransient = true; // 帧间不保留
};

/**
 * @brief 渲染目标
 */
class RenderTarget {
public:
    RenderTarget() = default;
    ~RenderTarget();

    /**
     * @brief 创建渲染目标
     */
    bool create(RenderDevice& device, const RenderTargetDesc& desc);

    /**
     * @brief 销毁渲染目标
     */
    void destroy();

    /**
     * @brief 获取纹理句柄
     */
    [[nodiscard]] TextureHandle getTexture() const { return texture_; }

    /**
     * @brief 获取帧缓冲句柄
     */
    [[nodiscard]] FrameBufferHandle getFrameBuffer() const { return frameBuffer_; }

    /**
     * @brief 获取宽度
     */
    [[nodiscard]] uint32_t getWidth() const { return width_; }

    /**
     * @brief 获取高度
     */
    [[nodiscard]] uint32_t getHeight() const { return height_; }

    /**
     * @brief 是否有效
     */
    [[nodiscard]] bool isValid() const { return texture_.valid(); }

private:
    RenderDevice* device_ = nullptr;
    TextureHandle texture_;
    FrameBufferHandle frameBuffer_;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    bool isDepth_ = false;
};

/**
 * @brief G-Buffer (用于延迟渲染)
 */
class GBuffer {
public:
    GBuffer() = default;
    ~GBuffer();

    /**
     * @brief 创建 G-Buffer
     */
    bool create(RenderDevice& device, const GBufferConfig& config);

    /**
     * @brief 销毁 G-Buffer
     */
    void destroy();

    /**
     * @brief 调整大小
     */
    void resize(uint32_t width, uint32_t height);

    /**
     * @brief 获取各通道纹理
     */
    [[nodiscard]] TextureHandle getAlbedo() const { return albedo_; }
    [[nodiscard]] TextureHandle getNormal() const { return normal_; }
    [[nodiscard]] TextureHandle getMaterial() const { return material_; }
    [[nodiscard]] TextureHandle getDepth() const { return depth_; }

    /**
     * @brief 获取帧缓冲
     */
    [[nodiscard]] FrameBufferHandle getFrameBuffer() const { return frameBuffer_; }

    /**
     * @brief 获取宽度
     */
    [[nodiscard]] uint32_t getWidth() const { return width_; }

    /**
     * @brief 获取高度
     */
    [[nodiscard]] uint32_t getHeight() const { return height_; }

private:
    RenderDevice* device_ = nullptr;
    TextureHandle albedo_;
    TextureHandle normal_;
    TextureHandle material_;
    TextureHandle depth_;
    FrameBufferHandle frameBuffer_;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
};

/**
 * @brief 渲染通道类型
 */
enum class RenderPassType {
    Geometry,      // 几何渲染
    Shadow,        // 阴影
    Skybox,        // 天空盒
    PostProcess,   // 后处理
    Composite,     // 合成 (延迟渲染)
    UI,            // UI
    Debug          // 调试
};

/**
 * @brief 渲染通道描述
 */
struct RenderPassDesc {
    RenderPassType type;
    std::string name;
    uint32_t viewId;
    ClearFlags clearFlags = ClearFlags::All;
    Color clearColor = Color(0, 0, 0, 1);
    float clearDepth = 1.0f;
    uint8_t clearStencil = 0;
    RenderState renderState;
    FrameBufferHandle frameBuffer;
    bool hasDepth = true;
    bool hasStencil = false;
};

/**
 * @brief 渲染通道
 */
class RenderPass {
public:
    RenderPass() = default;
    
    /**
     * @brief 创建渲染通道
     */
    void create(const RenderPassDesc& desc);

    /**
     * @brief 开始渲染通道
     */
    void begin(RenderDevice& device);

    /**
     * @brief 结束渲染通道
     */
    void end(RenderDevice& device);

    /**
     * @brief 设置视口
     */
    void setViewport(const Viewport& viewport);

    /**
     * @brief 设置 scissor
     */
    void setScissor(const Rect& rect);

    /**
     * @brief 获取视图 ID
     */
    [[nodiscard]] uint32_t getViewId() const { return desc_.viewId; }

    /**
     * @brief 获取描述
     */
    [[nodiscard]] const RenderPassDesc& getDesc() const { return desc_; }

private:
    RenderPassDesc desc_;
    Viewport viewport_;
    Rect scissor_;
    bool active_ = false;
};

/**
 * @brief 绘制调用描述
 */
struct DrawCall {
    ProgramHandle program;
    BufferHandle vertexBuffer;
    BufferHandle indexBuffer;
    uint32_t firstVertex = 0;
    uint32_t vertexCount = 0;
    uint32_t firstIndex = 0;
    uint32_t indexCount = 0;
    uint32_t instanceCount = 1;
    uint32_t startIndexLocation = 0;
    uint32_t baseVertexLocation = 0;
    uint32_t startInstanceLocation = 0;
    Material* material = nullptr;
    UniformBuffer* uniforms = nullptr;
    uint64_t state = 0; // bgfx 状态
};

/**
 * @brief 命令缓冲 - 支持多线程录制
 */
class CommandBuffer {
public:
    CommandBuffer();
    ~CommandBuffer();

    /**
     * @brief 重置命令缓冲
     */
    void reset();

    /**
     * @brief 录制绘制命令
     */
    void draw(const DrawCall& drawCall);

    /**
     * @brief 录制清除命令
     */
    void clear(uint32_t viewId, ClearFlags flags, const Color& color, 
               float depth = 1.0f, uint8_t stencil = 0);

    /**
     * @brief 录制提交命令
     */
    void submit(uint32_t viewId, ProgramHandle program, SubmitFlags flags = SubmitFlags::None);

    /**
     * @brief 设置变换矩阵
     */
    void setTransform(const float* matrix, uint64_t handle);

    /**
     * @brief 设置 Uniform
     */
    void setUniform(UniformHandle handle, const void* data, uint16_t num);

    /**
     * @brief 设置纹理
     */
    void setTexture(uint8_t stage, TextureHandle texture, uint32_t flags = 0);

    /**
     * @brief 是否有效
     */
    [[nodiscard]] bool isValid() const { return initialized_; }

private:
    struct Command {
        enum Type { Draw, Clear, Submit, SetTransform, SetUniform, SetTexture };
        Type type;
        union {
            DrawCall drawCall;
            struct {
                uint32_t viewId;
                ClearFlags flags;
                Color color;
                float depth;
                uint8_t stencil;
            } clear;
            struct {
                uint32_t viewId;
                ProgramHandle program;
                SubmitFlags flags;
            } submit;
            struct {
                uint64_t transformHandle;
                float matrix[16];
            } transform;
            struct {
                UniformHandle handle;
                uint16_t num;
                uint8_t* data;
            } uniform;
            struct {
                uint8_t stage;
                TextureHandle texture;
                uint32_t flags;
            } texture;
        };
    };

    std::vector<Command> commands_;
    std::vector<uint8_t> uniformData_;
    bool initialized_ = false;
    static constexpr size_t INITIAL_COMMAND_CAPACITY = 1024;
    static constexpr size_t INITIAL_UNIFORM_CAPACITY = 64 * 1024;
};

/**
 * @brief 命令缓冲池 - 帧间复用
 */
class CommandBufferPool {
public:
    CommandBufferPool(size_t initialSize = 4);
    ~CommandBufferPool();

    /**
     * @brief 获取命令缓冲
     */
    CommandBuffer* acquire();

    /**
     * @brief 释放命令缓冲
     */
    void release(CommandBuffer* buffer);

    /**
     * @brief 重置所有缓冲
     */
    void reset();

private:
    std::vector<std::unique_ptr<CommandBuffer>> buffers_;
    std::vector<CommandBuffer*> available_;
    std::mutex mutex_;
};

/**
 * @brief 前向渲染器
 */
class ForwardRenderer {
public:
    ForwardRenderer();
    ~ForwardRenderer();

    /**
     * @brief 初始化渲染器
     */
    bool initialize(RenderDevice& device);

    /**
     * @brief 关闭渲染器
     */
    void shutdown();

    /**
     * @brief 开始帧
     */
    void beginFrame();

    /**
     * @brief 添加绘制调用
     */
    void addDrawCall(const DrawCall& drawCall);

    /**
     * @brief 添加渲染通道
     */
    void addRenderPass(const RenderPass& pass);

    /**
     * @brief 结束帧并提交
     */
    void endFrame();

    /**
     * @brief 设置相机矩阵
     */
    void setViewProjection(const float* view, const float* projection);

    /**
     * @brief 设置环境光
     */
    void setAmbientLight(const Color& color);

    /**
     * @brief 设置方向光
     */
    void setDirectionalLight(const float* direction, const Color& color, float intensity);

private:
    RenderDevice* device_ = nullptr;
    CommandBufferPool commandPool_;
    std::vector<RenderPass> renderPasses_;
    std::vector<DrawCall> drawCalls_;
    
    float viewMatrix_[16];
    float projectionMatrix_[16];
    Color ambientLight_;
    
    struct DirectionalLight {
        float direction[3];
        Color color;
        float intensity;
    } directionalLight_;
};

/**
 * @brief 延迟渲染器
 */
class DeferredRenderer {
public:
    DeferredRenderer();
    ~DeferredRenderer();

    /**
     * @brief 初始化渲染器
     */
    bool initialize(RenderDevice& device, const GBufferConfig& config);

    /**
     * @brief 关闭渲染器
     */
    void shutdown();

    /**
     * @brief 调整 G-Buffer 大小
     */
    void resize(uint32_t width, uint32_t height);

    /**
     * @brief 开始几何通道
     */
    void beginGeometryPass();

    /**
     * @brief 结束几何通道
     */
    void endGeometryPass();

    /**
     * @brief 开始光照通道
     */
    void beginLightingPass();

    /**
     * @brief 结束光照通道
     */
    void endLightingPass();

    /**
     * @brief 添加几何绘制调用
     */
    void addGeometryDrawCall(const DrawCall& drawCall);

    /**
     * @brief 添加光源
     */
    void addPointLight(const float* position, const Color& color, float radius, float intensity);
    void addSpotLight(const float* position, const float* direction, 
                      const Color& color, float radius, float intensity, float spotAngle);

    /**
     * @brief 获取 G-Buffer
     */
    [[nodiscard]] GBuffer& getGBuffer() { return gBuffer_; }

    /**
     * @brief 获取最终输出纹理
     */
    [[nodiscard]] TextureHandle getOutputTexture() const { return outputTexture_; }

private:
    RenderDevice* device_ = nullptr;
    GBuffer gBuffer_;
    RenderTarget outputTarget_;
    TextureHandle outputTexture_;
    
    ProgramHandle geometryProgram_;
    ProgramHandle lightingProgram_;
    
    struct Light {
        enum Type { Point, Spot };
        Type type;
        float position[3];
        float direction[3];
        Color color;
        float radius;
        float intensity;
        float spotAngle;
    };
    
    std::vector<Light> lights_;
    CommandBufferPool commandPool_;
};

/**
 * @brief 渲染状态管理
 */
class RenderStateManager {
public:
    /**
     * @brief 获取默认状态
     */
    [[nodiscard]] static const RenderState& getDefaultState();

    /**
     * @brief 获取不透明状态
     */
    [[nodiscard]] static const RenderState& getOpaqueState();

    /**
     * @brief 获取透明状态
     */
    [[nodiscard]] static const RenderState& getTransparentState();

    /**
     * @brief 获取阴影状态
     */
    [[nodiscard]] static const RenderState& getShadowState();

    /**
     * @brief 获取 UI 状态
     */
    [[nodiscard]] static const RenderState& getUIState();

    /**
     * @brief 混合状态
     */
    [[nodiscard]] static uint64_t blendState(BlendFactor src, BlendFactor dst, 
                                              BlendOp op = BlendOp::Add);

    /**
     * @brief 深度状态
     */
    [[nodiscard]] static uint64_t depthState(DepthFunc func, bool write = true);

    /**
     * @brief 剔除状态
     */
    [[nodiscard]] static uint64_t cullState(CullMode mode, FrontFace face = FrontFace::CCW);

    /**
     * @brief 组合状态
     */
    [[nodiscard]] static uint64_t makeState(const RenderState& state);
};

} // namespace render
} // namespace phoenix
