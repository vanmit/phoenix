#pragma once

#include "Types.hpp"
#include <memory>
#include <string>
#include <vector>
#include <array>
#include <atomic>

// bgfx 前向声明
namespace bgfx {
    struct Init;
    class RendererType;
}

namespace phoenix {
namespace render {

// 前向声明
struct VertexLayout;
class CommandBuffer;

/**
 * @brief 渲染后端类型
 */
enum class RenderBackend {
    Vulkan,
    DirectX11,
    DirectX12,
    Metal,
    OpenGL,
    OpenGL_ES,
    WebGL,
    WebGL2,
    Null
};

/**
 * @brief 渲染设备信息
 */
struct DeviceInfo {
    RenderBackend backend;
    std::string backendName;
    std::string vendor;
    std::string device;
    std::string driver;
    
    uint32_t maxTextureSize2D;
    uint32_t maxTextureSizeCube;
    uint32_t maxTextureSize3D;
    uint32_t maxTextureLayers;
    uint32_t maxVertexStreams;
    uint32_t maxUniforms;
    uint32_t maxShaderSamplers;
    uint32_t maxComputeBindings;
    
    bool supportsCompute;
    bool supportsGeometryShaders;
    bool supportsTessellation;
    bool supportsInstancing;
    bool supportsConservativeRaster;
    bool supports3DTextures;
    bool supportsCubeArrayTextures;
    bool supportsOcclusionQuery;
    bool supportsTimestampQuery;
};

/**
 * @brief 交换链配置
 */
struct SwapChainConfig {
    void* windowHandle = nullptr;
    uint32_t width = 1920;
    uint32_t height = 1080;
    TextureFormat format = TextureFormat::RGBA8;
    uint32_t sampleCount = 1;
    bool vsync = true;
    bool srgb = true;
    bool depthBuffer = true;
    TextureFormat depthFormat = TextureFormat::Depth24;
};

/**
 * @brief 设备初始化配置
 */
struct DeviceConfig {
    RenderBackend backend = RenderBackend::Vulkan;
    bool enableValidation = false;
    bool enableDebugInfo = true;
    uint32_t maxFrameLatency = 2;
    uint32_t transientBufferSize = 128 * 1024 * 1024; // 128MB
    uint32_t persistentBufferSize = 64 * 1024 * 1024;  // 64MB
    uint32_t textureCacheSize = 256 * 1024 * 1024;     // 256MB
    
    // 回调函数
    using LogCallback = std::function<void(const char* message)>;
    LogCallback logCallback;
};

/**
 * @brief 帧统计信息
 */
struct FrameStats {
    uint64_t frameNumber;
    float frameTime;        // 毫秒
    float gpuTime;          // 毫秒
    uint32_t drawCalls;
    uint32_t triangleCount;
    uint32_t vertexCount;
    uint32_t textureBindings;
    uint32_t uniformBindings;
    uint32_t transientMemoryUsed;
    uint32_t persistentMemoryUsed;
};

/**
 * @brief 渲染设备 - bgfx 封装层
 * 
 * 提供跨平台渲染后端抽象，支持:
 * - Vulkan / DX11 / DX12 / Metal / OpenGL / WebGL
 * - 强类型句柄系统防止悬空引用
 * - 零动态分配 (帧间复用)
 * - 线程安全命令缓冲录制
 */
class RenderDevice {
public:
    RenderDevice();
    ~RenderDevice();

    // 禁止拷贝
    RenderDevice(const RenderDevice&) = delete;
    RenderDevice& operator=(const RenderDevice&) = delete;

    // 允许移动
    RenderDevice(RenderDevice&& other) noexcept;
    RenderDevice& operator=(RenderDevice&& other) noexcept;

    /**
     * @brief 初始化渲染设备
     * @param config 设备配置
     * @param swapChain 交换链配置
     * @return 是否成功
     */
    bool initialize(const DeviceConfig& config, const SwapChainConfig& swapChain);

    /**
     * @brief 关闭渲染设备
     */
    void shutdown();

    /**
     * @brief 检查设备是否有效
     */
    [[nodiscard]] bool isValid() const { return initialized_; }

    /**
     * @brief 获取设备信息
     */
    [[nodiscard]] const DeviceInfo& getDeviceInfo() const { return deviceInfo_; }

    /**
     * @brief 获取当前后端类型
     */
    [[nodiscard]] RenderBackend getBackend() const { return deviceInfo_.backend; }

    // ==================== 交换链管理 ====================

    /**
     * @brief 创建交换链
     */
    [[nodiscard]] DeviceHandle createSwapChain(const SwapChainConfig& config);

    /**
     * @brief 销毁交换链
     */
    void destroySwapChain(DeviceHandle handle);

    /**
     * @brief 获取主交换链
     */
    [[nodiscard]] DeviceHandle getMainSwapChain() const { return mainSwapChain_; }

    /**
     * @brief 调整交换链大小
     */
    void resizeSwapChain(DeviceHandle handle, uint32_t width, uint32_t height);

    // ==================== 帧管理 ====================

    /**
     * @brief 开始新帧
     * @param viewId 视图 ID
     * @param viewport 视口
     */
    void beginFrame(uint32_t viewId = 0, const Viewport& viewport = {});

    /**
     * @brief 结束帧并提交
     * @param vsync 是否垂直同步
     * @return 帧统计信息
     */
    [[nodiscard]] FrameStats endFrame(bool vsync = true);

    /**
     * @brief 获取当前帧号
     */
    [[nodiscard]] uint64_t getFrameNumber() const { return frameNumber_; }

    // ==================== 清除操作 ====================

    /**
     * @brief 清除视图
     * @param viewId 视图 ID
     * @param flags 清除标志
     * @param color 清除颜色
     * @param depth 清除深度
     * @param stencil 清除模板值
     */
    void clear(uint32_t viewId, ClearFlags flags, const Color& color, 
               float depth = 1.0f, uint8_t stencil = 0);

    // ==================== 视图管理 ====================

    /**
     * @brief 设置视图矩形
     */
    void setViewRect(uint32_t viewId, const Rect& rect);

    /**
     * @brief 设置视图视口
     */
    void setViewViewport(uint32_t viewId, const Viewport& viewport);

    /**
     * @brief 设置视图帧缓冲
     */
    void setViewFrameBuffer(uint32_t viewId, FrameBufferHandle fb);

    /**
     * @brief 设置视图清除状态
     */
    void setViewClear(uint32_t viewId, ClearFlags flags, const Color& color,
                      float depth = 1.0f, uint8_t stencil = 0);

    // ==================== 提交 ====================

    /**
     * @brief 提交绘制调用
     * @param viewId 视图 ID
     * @param program 着色器程序
     * @param flags 提交标志
     */
    void submit(uint32_t viewId, ProgramHandle program, SubmitFlags flags = SubmitFlags::None);

    /**
     * @brief 提交计算调用
     */
    void submitCompute(uint32_t viewId, ProgramHandle program, uint32_t numX, uint32_t numY = 1, uint32_t numZ = 1);

    // ==================== 调试 ====================

    /**
     * @brief 设置调试名称
     */
    void setDebugName(DeviceHandle handle, const char* name);
    void setDebugName(ShaderHandle handle, const char* name);
    void setDebugName(BufferHandle handle, const char* name);
    void setDebugName(TextureHandle handle, const char* name);
    void setDebugName(FrameBufferHandle handle, const char* name);

    /**
     * @brief 添加调试文本
     */
    void debugTextClear();
    void debugTextPrintf(uint32_t viewId, uint16_t x, uint16_t y, uint8_t color, const char* format, ...);

    /**
     * @brief 截图
     */
    void requestScreenshot(const char* filename);

    // ==================== 平台相关 ====================

    /**
     * @brief 设置原生窗口句柄
     */
    void setWindowHandle(void* windowHandle);

    /**
     * @brief 获取原生渲染句柄 (用于 ImGui 等集成)
     */
    [[nodiscard]] void* getNativeRenderHandle() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    std::atomic<bool> initialized_{false};
    std::atomic<uint64_t> frameNumber_{0};
    
    DeviceInfo deviceInfo_;
    DeviceHandle mainSwapChain_;
    
    void initDeviceInfo();
    void translateInit(bgfx::Init& init, const DeviceConfig& config);
};

/**
 * @brief 顶点布局构建器
 */
struct VertexLayout {
    std::array<VertexAttrib, 16> attributes{};
    uint32_t attributeCount = 0;
    uint32_t stride = 0;
    uint32_t hash = 0;

    VertexLayout& begin(uint32_t stream = 0);
    VertexLayout& add(VertexAttribSemantic semantic, VertexAttribFormat format, bool normalized = false);
    VertexLayout& addPosition(VertexAttribFormat format = VertexAttribFormat::Float3);
    VertexLayout& addNormal(VertexAttribFormat format = VertexAttribFormat::Float3);
    VertexLayout& addTangent(VertexAttribFormat format = VertexAttribFormat::Float3);
    VertexLayout& addTexCoord(uint32_t index = 0, VertexAttribFormat format = VertexAttribFormat::Float2);
    VertexLayout& addColor(uint32_t index = 0, VertexAttribFormat format = VertexAttribFormat::Float4);
    VertexLayout& end();

    [[nodiscard]] uint32_t getStride() const { return stride; }
    [[nodiscard]] uint32_t getHash() const { return hash; }
};

} // namespace render
} // namespace phoenix
