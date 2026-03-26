#pragma once

#include "../platform_types.hpp"
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include <memory>
#include <vector>
#include <string>

namespace phoenix {
namespace platform {
namespace macos {

// ============================================================================
// Metal 配置
// ============================================================================

struct MetalConfig {
    bool enableGPUValidation = false;
    bool enableShaderValidation = true;
    bool enableAPIValidation = true;
    bool enableCaptureSupport = false;
    bool enableRasterizationRateMaps = false;
    bool enableTileShaders = false;
    
    uint32_t desiredMaxBufferLength = 3;
    uint32_t desiredFramebufferWidth = 1920;
    uint32_t desiredFramebufferHeight = 1080;
    
    MTLStorageMode preferredStorageMode = MTLStorageModePrivate;
    MTLCPUCacheMode preferredCPUCacheMode = MTLCPUCacheModeDefaultCache;
};

// ============================================================================
// Metal 设备信息
// ============================================================================

struct MetalDeviceInfo {
    std::string name;
    std::string registryID;
    
    MTLDeviceLocation deviceLocation = MTLDeviceLocationBuiltIn;
    MTLDeviceLowPowerCategory lowPowerCategory = MTLDeviceLowPowerCategoryUnknown;
    
    uint64_t recommendedMaxWorkingSetSize = 0;
    uint64_t maxWorkingSetSize = 0;
    
    uint32_t maxThreadsPerThreadgroup = 0;
    uint32_t maxThreadgroupMemoryLength = 0;
    uint32_t maxArgumentBufferSamplerCount = 0;
    
    bool supportsRaytracing = false;
    bool supportsMeshShaders = false;
    bool supportsTileShaders = false;
    bool supportsPrimitiveShaders = false;
    bool supportsObjectBuffers = false;
    bool supportsArgumentBuffers = false;
    bool supportsRasterizationRateMaps = false;
    bool supportsIndirectCommandBuffers = false;
    bool supportsTextureBarriers = false;
    bool supportsShaderBarycentricCoordinates = false;
    bool supportsShaderQuadGrid = false;
    bool supportsPerStageReadWriteResources = false;
    bool supportsBinaryArchives = false;
    bool supportsPipelineBinaryInfo = false;
    bool supportsCounterSampling = false;
    bool supportsVertexAmplification = false;
    bool supportsMultisampleArray = false;
    bool supportsTextureSampleCount2D = false;
    bool supportsTextureSampleCountCube = false;
};

// ============================================================================
// 资源句柄
// ============================================================================

struct ResourceHandle {
    uint64_t id = 0;
    bool isValid() const { return id != 0; }
};

struct BufferHandle : public ResourceHandle {};
struct TextureHandle : public ResourceHandle {};
struct SamplerHandle : public ResourceHandle {};
struct PipelineHandle : public ResourceHandle {};
struct LibraryHandle : public ResourceHandle {};

// ============================================================================
// Metal 渲染设备
// ============================================================================

class MetalRenderDevice {
public:
    MetalRenderDevice();
    ~MetalRenderDevice();
    
    // 禁止拷贝
    MetalRenderDevice(const MetalRenderDevice&) = delete;
    MetalRenderDevice& operator=(const MetalRenderDevice&) = delete;
    
    /**
     * @brief 初始化设备
     */
    bool initialize(const MetalConfig& config);
    
    /**
     * @brief 关闭设备
     */
    void shutdown();
    
    /**
     * @brief 检查设备是否有效
     */
    [[nodiscard]] bool isValid() const { return initialized_; }
    
    /**
     * @brief 获取 Metal 设备
     */
    [[nodiscard]] id<MTLDevice> getDevice() const { return device_; }
    
    /**
     * @brief 获取命令队列
     */
    [[nodiscard]] id<MTLCommandQueue> getCommandQueue() const { return commandQueue_; }
    
    /**
     * @brief 获取设备信息
     */
    [[nodiscard]] const MetalDeviceInfo& getDeviceInfo() const { return deviceInfo_; }
    
    // ==================== 交换链 ====================
    
    /**
     * @brief 创建 Metal 层
     */
    bool createMetalLayer(void* view);  // NSView* or UIView*
    
    /**
     * @brief 配置 Metal 层
     */
    bool configureMetalLayer(uint32_t width, uint32_t height, MTLPixelFormat format, bool vsync);
    
    /**
     * @brief 获取 Metal 层
     */
    [[nodiscard]] CAMetalLayer* getMetalLayer() const { return metalLayer_; }
    
    /**
     * @brief 获取当前可绘制纹理
     */
    [[nodiscard]] id<CAMetalDrawable> getCurrentDrawable() const;
    
    /**
     * @brief 获取当前纹理
     */
    [[nodiscard]] id<MTLTexture> getCurrentTexture() const;
    
    /**
     * @brief 呈现
     */
    void present(id<CAMetalDrawable> drawable);
    
    // ==================== 帧管理 ====================
    
    /**
     * @brief 开始帧
     */
    id<MTLCommandBuffer> beginFrame();
    
    /**
     * @brief 结束帧
     */
    void endFrame(id<MTLCommandBuffer> commandBuffer);
    
    /**
     * @brief 等待 GPU 完成
     */
    void waitForGPU();
    
    // ==================== 命令缓冲 ====================
    
    /**
     * @brief 创建命令缓冲
     */
    id<MTLCommandBuffer> createCommandBuffer();
    
    /**
     * @brief 创建命令编码器
     */
    id<MTLRenderCommandEncoder> createRenderCommandEncoder(id<MTLRenderPassDescriptor> passDescriptor);
    id<MTLComputeCommandEncoder> createComputeCommandEncoder(id<MTLCommandBuffer> commandBuffer);
    id<MTLBlitCommandEncoder> createBlitCommandEncoder(id<MTLCommandBuffer> commandBuffer);
    
    // ==================== 缓冲区 ====================
    
    /**
     * @brief 创建缓冲区
     */
    [[nodiscard]] BufferHandle createBuffer(NSUInteger length, MTLResourceOptions options, const void* data = nullptr);
    
    /**
     * @brief 创建缓冲区 (共享)
     */
    [[nodiscard]] BufferHandle createSharedBuffer(NSUInteger length, const void* data = nullptr);
    
    /**
     * @brief 创建缓冲区 (私有)
     */
    [[nodiscard]] BufferHandle createPrivateBuffer(NSUInteger length);
    
    /**
     * @brief 销毁缓冲区
     */
    void destroyBuffer(BufferHandle handle);
    
    /**
     * @brief 获取原生缓冲区
     */
    [[nodiscard]] id<MTLBuffer> getBuffer(BufferHandle handle) const;
    
    // ==================== 纹理 ====================
    
    /**
     * @brief 创建纹理
     */
    [[nodiscard]] TextureHandle createTexture(const MTLTextureDescriptor* descriptor);
    
    /**
     * @brief 创建纹理 (从数据)
     */
    [[nodiscard]] TextureHandle createTextureFromData(uint32_t width, uint32_t height, MTLPixelFormat format, const void* data);
    
    /**
     * @brief 创建纹理视图
     */
    [[nodiscard]] id<MTLTexture> createTextureView(id<MTLTexture> texture, const MTLTextureViewDescriptor* descriptor);
    
    /**
     * @brief 销毁纹理
     */
    void destroyTexture(TextureHandle handle);
    
    /**
     * @brief 获取原生纹理
     */
    [[nodiscard]] id<MTLTexture> getTexture(TextureHandle handle) const;
    
    // ==================== 采样器 ====================
    
    /**
     * @brief 创建采样器
     */
    [[nodiscard]] SamplerHandle createSampler(const MTLSamplerDescriptor* descriptor);
    
    /**
     * @brief 销毁采样器
     */
    void destroySampler(SamplerHandle handle);
    
    /**
     * @brief 获取原生采样器
     */
    [[nodiscard]] id<MTLSamplerState> getSampler(SamplerHandle handle) const;
    
    // ==================== 库和函数 ====================
    
    /**
     * @brief 从文件加载库
     */
    [[nodiscard]] LibraryHandle loadLibrary(const char* path);
    
    /**
     * @brief 从源编译库
     */
    [[nodiscard]] LibraryHandle compileLibrary(const char* source, const char* entryPoint);
    
    /**
     * @brief 获取函数
     */
    [[nodiscard]] id<MTLFunction> getFunction(LibraryHandle library, const char* name);
    
    /**
     * @brief 销毁库
     */
    void destroyLibrary(LibraryHandle handle);
    
    // ==================== 管线 ====================
    
    /**
     * @brief 创建渲染管线
     */
    [[nodiscard]] PipelineHandle createRenderPipeline(const MTLRenderPipelineDescriptor* descriptor);
    
    /**
     * @brief 创建计算管线
     */
    [[nodiscard]] PipelineHandle createComputePipeline(id<MTLFunction> computeFunction);
    
    /**
     * @brief 创建计算管线 (可并发)
     */
    [[nodiscard]] PipelineHandle createComputePipelineConcurrent(id<MTLFunction> computeFunction);
    
    /**
     * @brief 销毁管线
     */
    void destroyPipeline(PipelineHandle handle);
    
    /**
     * @brief 获取原生渲染管线
     */
    [[nodiscard]] id<MTLRenderPipelineState> getRenderPipeline(PipelineHandle handle) const;
    
    /**
     * @brief 获取原生计算管线
     */
    [[nodiscard]] id<MTLComputePipelineState> getComputePipeline(PipelineHandle handle) const;
    
    // ==================== 描述符 ====================
    
    /**
     * @brief 创建参数缓冲区布局
     */
    id<MTLArgumentEncoder> createArgumentEncoder(NSArray<id<MTLArgumentDescriptor>>* arguments);
    
    // ==================== 同步 ====================
    
    /**
     * @brief 创建栅栏
     */
    id<MTLFence> createFence();
    
    /**
     * @brief 创建事件
     */
    id<MTLEvent> createEvent();
    
    /**
     * @brief 创建共享事件
     */
    id<MTLSharedEvent> createSharedEvent();
    
    // ==================== 资源复制 ====================
    
    /**
     * @brief 复制缓冲区
     */
    void copyBuffer(id<MTLBuffer> src, NSUInteger srcOffset, id<MTLBuffer> dst, NSUInteger dstOffset, NSUInteger size);
    
    /**
     * @brief 复制纹理
     */
    void copyTexture(id<MTLTexture> src, MTLRegion srcRegion, NSUInteger srcLevel, NSUInteger srcSlice,
                     id<MTLTexture> dst, MTLPoint dstOrigin, NSUInteger dstLevel, NSUInteger dstSlice,
                     NSUInteger width, NSUInteger height, NSUInteger depth);
    
    /**
     * @brief 从缓冲区复制纹理
     */
    void copyBufferToTexture(id<MTLBuffer> src, NSUInteger srcOffset, NSUInteger srcBytesPerRow, NSUInteger srcBytesPerImage,
                             id<MTLTexture> dst, NSUInteger dstLevel, NSUInteger dstSlice, MTLRegion dstRegion);
    
    /**
     * @brief 生成 Mipmaps
     */
    void generateMipmaps(id<MTLTexture> texture);
    
    // ==================== 调试 ====================
    
    /**
     * @brief 设置对象标签
     */
    void setLabel(id<MTLResource> resource, const char* label);
    void setLabel(id<MTLCommandBuffer> commandBuffer, const char* label);
    void setLabel(id<MTLRenderCommandEncoder> encoder, const char* label);
    void setLabel(id<MTLComputeCommandEncoder> encoder, const char* label);
    
    /**
     * @brief 推送调试组
     */
    void pushDebugGroup(id<MTLCommandBuffer> commandBuffer, const char* label);
    
    /**
     * @brief 弹出调试组
     */
    void popDebugGroup(id<MTLCommandBuffer> commandBuffer);
    
    /**
     * @brief 插入调试信号
     */
    void insertDebugSignpost(id<MTLCommandBuffer> commandBuffer, const char* label);

private:
    id<MTLDevice> device_ = nil;
    id<MTLCommandQueue> commandQueue_ = nil;
    CAMetalLayer* metalLayer_ = nil;
    
    MetalConfig config_;
    MetalDeviceInfo deviceInfo_;
    bool initialized_ = false;
    
    bool createDevice();
    bool createCommandQueue();
    void initDeviceInfo();
    
    // 资源池
    struct ResourcePool {
        std::vector<BufferHandle> buffers;
        std::vector<TextureHandle> textures;
        std::vector<SamplerHandle> samplers;
        std::vector<PipelineHandle> pipelines;
        std::vector<LibraryHandle> libraries;
    };
    
    ResourcePool resourcePool_;
};

} // namespace macos
} // namespace platform
} // namespace phoenix
