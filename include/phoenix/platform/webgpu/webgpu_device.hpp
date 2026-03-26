#pragma once

#include "../platform_types.hpp"
#include <memory>
#include <string>
#include <vector>
#include <functional>

// WebGPU 前向声明
struct WGPUInstance;
struct WGPUAdapter;
struct WGPUDevice;
struct WGPUSurface;
struct WGPUSwapChain;
struct WGPUQueue;
struct WGPUCommandEncoder;
struct WGPUCommandBuffer;
struct WGPURenderPassEncoder;
struct WGPUComputePassEncoder;
struct WGPURenderBundleEncoder;
struct WGPUBuffer;
struct WGPUTexture;
struct WGPUTextureView;
struct WGPUSampler;
struct WGPUBindGroupLayout;
struct WGPUBindGroup;
struct WGPUPipelineLayout;
struct WGPURenderPipeline;
struct WGPUComputePipeline;
struct WGPUShaderModule;
struct WGPUQuerySet;

namespace phoenix {
namespace platform {
namespace webgpu {

// ============================================================================
// WebGPU 配置
// ============================================================================

struct WebGPUConfig {
    bool enableValidation = false;
    bool enableDebugInfo = true;
    bool enableTimestampQueries = false;
    bool enableOcclusionQueries = false;
    uint32_t maxBufferSize = 256 * 1024 * 1024;    // 256MB
    uint32_t maxTextureSize = 16384;
    uint32_t maxUniformBufferBindingSize = 65536;
    uint32_t maxStorageBufferBindingSize = 128 * 1024 * 1024;
    uint32_t maxSamplersPerShaderStage = 16;
    uint32_t maxSampledTexturesPerShaderStage = 16;
    uint32_t maxStorageTexturesPerShaderStage = 4;
    uint32_t maxUniformBuffersPerShaderStage = 12;
    uint32_t maxStorageBuffersPerShaderStage = 8;
    uint32_t maxColorAttachments = 8;
    uint32_t maxComputeWorkgroupStorageSize = 32768;
    uint32_t maxComputeInvocationsPerWorkgroup = 256;
    uint32_t maxComputeWorkgroupSizeX = 256;
    uint32_t maxComputeWorkgroupSizeY = 256;
    uint32_t maxComputeWorkgroupSizeZ = 64;
};

// ============================================================================
// WebGPU 设备信息
// ============================================================================

struct WebGPUDeviceInfo {
    std::string vendor;
    std::string architecture;
    std::string device;
    std::string driverDescription;
    std::string backendType;
    
    uint32_t vendorId = 0;
    uint32_t deviceId = 0;
    
    bool isSoftware = false;
    bool supportsDepthClipControl = false;
    bool supportsDepth32FloatStencil8 = false;
    bool supportsTimestampQuery = false;
    bool supportsPipelineStatisticsQuery = false;
    bool supportsTextureCompressionBC = false;
    bool supportsTextureCompressionETC2 = false;
    bool supportsTextureCompressionASTC = false;
    bool supportsIndirectFirstInstance = false;
    bool supportsShaderF16 = false;
    bool supportsRG11B10Ufloat = false;
    bool supportsFloat32Filterable = false;
};

// ============================================================================
// 资源句柄
// ============================================================================

struct ResourceHandle {
    uint32_t id = 0;
    bool isValid() const { return id != 0; }
};

struct BufferHandle : public ResourceHandle {};
struct TextureHandle : public ResourceHandle {};
struct TextureViewHandle : public ResourceHandle {};
struct SamplerHandle : public ResourceHandle {};
struct BindGroupLayoutHandle : public ResourceHandle {};
struct BindGroupHandle : public ResourceHandle {};
struct PipelineLayoutHandle : public ResourceHandle {};
struct RenderPipelineHandle : public ResourceHandle {};
struct ComputePipelineHandle : public ResourceHandle {};
struct ShaderModuleHandle : public ResourceHandle {};
struct QuerySetHandle : public ResourceHandle {};

// ============================================================================
// 缓冲区描述
// ============================================================================

enum class BufferUsage : uint32_t {
    None = 0x0000,
    MapRead = 0x0001,
    MapWrite = 0x0002,
    CopySrc = 0x0004,
    CopyDst = 0x0008,
    Index = 0x0010,
    Vertex = 0x0020,
    Uniform = 0x0040,
    Storage = 0x0080,
    Indirect = 0x0100,
    QueryResolve = 0x0200,
};

inline BufferUsage operator|(BufferUsage a, BufferUsage b) {
    return static_cast<BufferUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

struct BufferDesc {
    uint64_t size = 0;
    BufferUsage usage = BufferUsage::None;
    bool mappedAtCreation = false;
    const void* initialData = nullptr;
};

// ============================================================================
// 纹理描述
// ============================================================================

enum class TextureDimension : uint8_t {
    Dimension1D,
    Dimension2D,
    Dimension3D,
};

enum class TextureFormat : uint8_t {
    // 8-bit formats
    R8Unorm,
    R8Snorm,
    R8Uint,
    R8Sint,
    
    // 16-bit formats
    R16Uint,
    R16Sint,
    R16Float,
    RG8Unorm,
    RG8Snorm,
    RG8Uint,
    RG8Sint,
    
    // 32-bit formats
    R32Uint,
    R32Sint,
    R32Float,
    RG16Uint,
    RG16Sint,
    RG16Float,
    RGBA8Unorm,
    RGBA8UnormSrgb,
    RGBA8Snorm,
    RGBA8Uint,
    RGBA8Sint,
    BGRA8Unorm,
    BGRA8UnormSrgb,
    RGB9E5Ufloat,
    
    // 64-bit formats
    RG32Uint,
    RG32Sint,
    RG32Float,
    RGBA16Uint,
    RGBA16Sint,
    RGBA16Float,
    
    // 128-bit formats
    RGBA32Uint,
    RGBA32Sint,
    RGBA32Float,
    
    // Depth/stencil formats
    Depth32Float,
    Depth24Plus,
    Depth24PlusStencil8,
    Depth32FloatStencil8,
    
    // Compression formats
    BC1RGBAUnorm,
    BC1RGBAUnormSrgb,
    BC2RGBAUnorm,
    BC2RGBAUnormSrgb,
    BC3RGBAUnorm,
    BC3RGBAUnormSrgb,
    BC4RUnorm,
    BC4RSnorm,
    BC5RGUnorm,
    BC5RGSnorm,
    BC6HRGBUfloat,
    BC6HRGBFloat,
    BC7RGBAUnorm,
    BC7RGBAUnormSrgb,
    ETC2RGB8Unorm,
    ETC2RGB8UnormSrgb,
    ETC2RGB8A1Unorm,
    ETC2RGB8A1UnormSrgb,
    ETC2RGBA8Unorm,
    ETC2RGBA8UnormSrgb,
    EACR11Unorm,
    EACR11Snorm,
    EACRG11Unorm,
    EACRG11Snorm,
    ASTC4x4Unorm,
    ASTC4x4UnormSrgb,
    ASTC5x4Unorm,
    ASTC5x4UnormSrgb,
    ASTC5x5Unorm,
    ASTC5x5UnormSrgb,
    ASTC6x5Unorm,
    ASTC6x5UnormSrgb,
    ASTC6x6Unorm,
    ASTC6x6UnormSrgb,
    ASTC8x5Unorm,
    ASTC8x5UnormSrgb,
    ASTC8x6Unorm,
    ASTC8x6UnormSrgb,
    ASTC8x8Unorm,
    ASTC8x8UnormSrgb,
    ASTC10x5Unorm,
    ASTC10x5UnormSrgb,
    ASTC10x6Unorm,
    ASTC10x6UnormSrgb,
    ASTC10x8Unorm,
    ASTC10x8UnormSrgb,
    ASTC10x10Unorm,
    ASTC10x10UnormSrgb,
    ASTC12x10Unorm,
    ASTC12x10UnormSrgb,
    ASTC12x12Unorm,
    ASTC12x12UnormSrgb,
};

struct TextureDesc {
    TextureFormat format = TextureFormat::RGBA8Unorm;
    TextureDimension dimension = TextureDimension::Dimension2D;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t depthOrArrayLayers = 1;
    uint32_t mipLevelCount = 1;
    uint32_t sampleCount = 1;
    uint32_t usage = 0;  // TextureUsage flags
};

// ============================================================================
// WebGPU 渲染设备
// ============================================================================

class WebGPURenderDevice {
public:
    WebGPURenderDevice();
    ~WebGPURenderDevice();
    
    // 禁止拷贝
    WebGPURenderDevice(const WebGPURenderDevice&) = delete;
    WebGPURenderDevice& operator=(const WebGPURenderDevice&) = delete;
    
    /**
     * @brief 初始化设备
     */
    bool initialize(const WebGPUConfig& config);
    
    /**
     * @brief 关闭设备
     */
    void shutdown();
    
    /**
     * @brief 检查设备是否有效
     */
    [[nodiscard]] bool isValid() const { return initialized_; }
    
    /**
     * @brief 获取设备信息
     */
    [[nodiscard]] const WebGPUDeviceInfo& getDeviceInfo() const { return deviceInfo_; }
    
    /**
     * @brief 获取原生 WebGPU 设备
     */
    [[nodiscard]] WGPUDevice getDevice() const { return device_; }
    
    /**
     * @brief 获取 WebGPU 队列
     */
    [[nodiscard]] WGPUQueue getQueue() const { return queue_; }
    
    // ==================== 交换链管理 ====================
    
    /**
     * @brief 创建表面
     */
    bool createSurface(void* windowHandle);
    
    /**
     * @brief 配置交换链
     */
    bool configureSwapChain(uint32_t width, uint32_t height, TextureFormat format, bool vsync = true);
    
    /**
     * @brief 获取当前纹理视图
     */
    [[nodiscard]] WGPUTextureView getCurrentTextureView();
    
    /**
     * @brief 呈现帧
     */
    void present();
    
    /**
     * @brief 调整交换链大小
     */
    void resizeSwapChain(uint32_t width, uint32_t height);
    
    // ==================== 缓冲区管理 ====================
    
    /**
     * @brief 创建缓冲区
     */
    [[nodiscard]] BufferHandle createBuffer(const BufferDesc& desc);
    
    /**
     * @brief 销毁缓冲区
     */
    void destroyBuffer(BufferHandle handle);
    
    /**
     * @brief 映射缓冲区读取
     */
    bool mapBufferRead(BufferHandle handle, std::function<void(const void* data, uint64_t size)> callback);
    
    /**
     * @brief 映射缓冲区写入
     */
    bool mapBufferWrite(BufferHandle handle, const void* data, uint64_t size);
    
    /**
     * @brief 取消映射缓冲区
     */
    void unmapBuffer(BufferHandle handle);
    
    /**
     * @brief 写入缓冲区
     */
    void writeBuffer(BufferHandle handle, uint64_t offset, const void* data, uint64_t size);
    
    /**
     * @brief 复制缓冲区
     */
    void copyBuffer(BufferHandle src, BufferHandle dst, uint64_t size, uint64_t srcOffset = 0, uint64_t dstOffset = 0);
    
    // ==================== 纹理管理 ====================
    
    /**
     * @brief 创建纹理
     */
    [[nodiscard]] TextureHandle createTexture(const TextureDesc& desc);
    
    /**
     * @brief 销毁纹理
     */
    void destroyTexture(TextureHandle handle);
    
    /**
     * @brief 创建纹理视图
     */
    [[nodiscard]] TextureViewHandle createTextureView(TextureHandle texture);
    
    /**
     * @brief 销毁纹理视图
     */
    void destroyTextureView(TextureViewHandle handle);
    
    /**
     * @brief 写入纹理
     */
    void writeTexture(TextureHandle texture, const void* data, uint64_t size, 
                      uint32_t mipLevel = 0, uint32_t arrayLayer = 0);
    
    /**
     * @brief 生成 Mipmaps
     */
    void generateMipmaps(TextureHandle texture);
    
    // ==================== 采样器 ====================
    
    /**
     * @brief 创建采样器
     */
    [[nodiscard]] SamplerHandle createSampler();
    
    /**
     * @brief 销毁采样器
     */
    void destroySampler(SamplerHandle handle);
    
    // ==================== 着色器 ====================
    
    /**
     * @brief 从 WGSL 创建着色器模块
     */
    [[nodiscard]] ShaderModuleHandle createShaderModuleWGSL(const char* wgslSource);
    
    /**
     * @brief 从 SPIR-V 创建着色器模块
     */
    [[nodiscard]] ShaderModuleHandle createShaderModuleSPIRV(const uint32_t* spirv, uint32_t size);
    
    /**
     * @brief 销毁着色器模块
     */
    void destroyShaderModule(ShaderModuleHandle handle);
    
    // ==================== 绑定组 ====================
    
    /**
     * @brief 创建绑定组布局
     */
    [[nodiscard]] BindGroupLayoutHandle createBindGroupLayout();
    
    /**
     * @brief 创建绑定组
     */
    [[nodiscard]] BindGroupHandle createBindGroup(BindGroupLayoutHandle layout);
    
    /**
     * @brief 销毁绑定组布局
     */
    void destroyBindGroupLayout(BindGroupLayoutHandle handle);
    
    /**
     * @brief 销毁绑定组
     */
    void destroyBindGroup(BindGroupHandle handle);
    
    // ==================== 管线 ====================
    
    /**
     * @brief 创建渲染管线
     */
    [[nodiscard]] RenderPipelineHandle createRenderPipeline();
    
    /**
     * @brief 创建计算管线
     */
    [[nodiscard]] ComputePipelineHandle createComputePipeline();
    
    /**
     * @brief 销毁渲染管线
     */
    void destroyRenderPipeline(RenderPipelineHandle handle);
    
    /**
     * @brief 销毁计算管线
     */
    void destroyComputePipeline(ComputePipelineHandle handle);
    
    // ==================== 命令编码 ====================
    
    /**
     * @brief 开始命令编码
     */
    void beginCommandEncoding();
    
    /**
     * @brief 开始渲染通道
     */
    void beginRenderPass();
    
    /**
     * @brief 开始计算通道
     */
    void beginComputePass();
    
    /**
     * @brief 结束渲染通道
     */
    void endRenderPass();
    
    /**
     * @brief 结束计算通道
     */
    void endComputePass();
    
    /**
     * @brief 提交命令
     */
    void submitCommands();
    
    /**
     * @brief 结束命令编码
     */
    void endCommandEncoding();
    
    // ==================== 绘制 ====================
    
    /**
     * @brief 设置管线
     */
    void setRenderPipeline(RenderPipelineHandle pipeline);
    void setComputePipeline(ComputePipelineHandle pipeline);
    
    /**
     * @brief 设置绑定组
     */
    void setBindGroup(uint32_t index, BindGroupHandle group);
    
    /**
     * @brief 设置顶点缓冲区
     */
    void setVertexBuffer(uint32_t slot, BufferHandle buffer, uint64_t offset = 0, uint64_t size = 0);
    
    /**
     * @brief 设置索引缓冲区
     */
    void setIndexBuffer(BufferHandle buffer, uint64_t offset = 0, uint64_t size = 0);
    
    /**
     * @brief 绘制
     */
    void draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t baseInstance = 0);
    void drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t baseVertex = 0, uint32_t baseInstance = 0);
    void drawIndirect(BufferHandle buffer, uint64_t offset);
    void drawIndexedIndirect(BufferHandle buffer, uint64_t offset);
    
    /**
     * @brief 调度计算
     */
    void dispatchCompute(uint32_t workgroupCountX, uint32_t workgroupCountY = 1, uint32_t workgroupCountZ = 1);
    void dispatchComputeIndirect(BufferHandle buffer, uint64_t offset);
    
    // ==================== 查询 ====================
    
    /**
     * @brief 创建查询集
     */
    [[nodiscard]] QuerySetHandle createQuerySet();
    void destroyQuerySet(QuerySetHandle handle);
    void beginOcclusionQuery(QuerySetHandle querySet, uint32_t queryIndex);
    void endOcclusionQuery();
    void resolveQuerySet(QuerySetHandle querySet, uint32_t firstQuery, uint32_t queryCount, BufferHandle dst, uint64_t dstOffset);
    
    // ==================== 调试 ====================
    
    /**
     * @brief 设置对象标签
     */
    void setLabel(void* object, const char* label);
    
    /**
     * @brief 插入调试标记
     */
    void insertDebugMarker(const char* markerLabel);
    
    /**
     * @brief 推送调试组
     */
    void pushDebugGroup(const char* groupLabel);
    
    /**
     * @brief 弹出调试组
     */
    void popDebugGroup();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    WGPUInstance instance_ = nullptr;
    WGPUAdapter adapter_ = nullptr;
    WGPUDevice device_ = nullptr;
    WGPUSurface surface_ = nullptr;
    WGPUSwapChain swapChain_ = nullptr;
    WGPUQueue queue_ = nullptr;
    WGPUCommandEncoder commandEncoder_ = nullptr;
    WGPURenderPassEncoder renderPassEncoder_ = nullptr;
    WGPUComputePassEncoder computePassEncoder_ = nullptr;
    
    WebGPUConfig config_;
    WebGPUDeviceInfo deviceInfo_;
    bool initialized_ = false;
    
    bool selectAdapter();
    bool createDevice();
    void initDeviceInfo();
};

} // namespace webgpu
} // namespace platform
} // namespace phoenix
