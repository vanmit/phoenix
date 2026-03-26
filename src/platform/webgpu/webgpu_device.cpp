#include "phoenix/platform/webgpu/webgpu_device.hpp"
#include <cstring>
#include <cstdio>

#if defined(PHOENIX_PLATFORM_WEB)
#include <emscripten/emscripten.h>
#include <emscripten/html5_webgpu.h>
#endif

namespace phoenix {
namespace platform {
namespace webgpu {

// ============================================================================
// 内部实现
// ============================================================================

struct WebGPURenderDevice::Impl {
    WebGPUConfig config;
    WebGPUDeviceInfo deviceInfo;
    
    // 资源池
    std::vector<BufferHandle> buffers;
    std::vector<TextureHandle> textures;
    std::vector<SamplerHandle> samplers;
    std::vector<ShaderModuleHandle> shaderModules;
    std::vector<BindGroupLayoutHandle> bindGroupLayouts;
    std::vector<BindGroupHandle> bindGroups;
    std::vector<RenderPipelineHandle> renderPipelines;
    std::vector<ComputePipelineHandle> computePipelines;
    
    // 当前状态
    WGPUTextureView currentTextureView = nullptr;
    bool inRenderPass = false;
    bool inComputePass = false;
};

// ============================================================================
// WebGPU 错误回调
// ============================================================================

static void webgpuErrorCallback(WGPUErrorType type, const char* message, void* userdata) {
    const char* typeStr = "Unknown";
    switch (type) {
        case WGPUErrorType_Validation: typeStr = "Validation"; break;
        case WGPUErrorType_OutOfMemory: typeStr = "OutOfMemory"; break;
        case WGPUErrorType_Internal: typeStr = "Internal"; break;
        case WGPUErrorType_Unknown: typeStr = "Unknown"; break;
        case WGPUErrorType_DeviceLost: typeStr = "DeviceLost"; break;
    }
    std::fprintf(stderr, "[WebGPU Error] %s: %s\n", typeStr, message);
}

// ============================================================================
// 构造函数/析构函数
// ============================================================================

WebGPURenderDevice::WebGPURenderDevice()
    : impl_(std::make_unique<Impl>()) {
}

WebGPURenderDevice::~WebGPURenderDevice() {
    shutdown();
}

// ============================================================================
// 初始化
// ============================================================================

bool WebGPURenderDevice::initialize(const WebGPUConfig& config) {
    config_ = config;
    
#if defined(PHOENIX_PLATFORM_WEB)
    // Emscripten WebGPU
    instance_ = reinterpret_cast<WGPUInstance>(emscripten_webgpu_get_instance());
    if (!instance_) {
        std::fprintf(stderr, "[WebGPU] Failed to get Emscripten WebGPU instance\n");
        return false;
    }
#else
    // 原生 WebGPU (wgpu-native)
    // 需要链接 dawn-native 或 wgpu-native
    // 这里使用占位实现
    std::fprintf(stderr, "[WebGPU] Native WebGPU not implemented yet\n");
    return false;
#endif
    
    // 选择适配器
    if (!selectAdapter()) {
        std::fprintf(stderr, "[WebGPU] Failed to select adapter\n");
        return false;
    }
    
    // 创建设备
    if (!createDevice()) {
        std::fprintf(stderr, "[WebGPU] Failed to create device\n");
        return false;
    }
    
    // 初始化设备信息
    initDeviceInfo();
    
    // 设置错误回调
    wgpuDeviceSetUncapturedErrorCallback(device_, webgpuErrorCallback, nullptr);
    
    // 获取队列
    queue_ = wgpuDeviceGetQueue(device_);
    
    initialized_ = true;
    return true;
}

void WebGPURenderDevice::shutdown() {
    if (!initialized_) {
        return;
    }
    
    // 释放所有资源
    impl_->buffers.clear();
    impl_->textures.clear();
    impl_->samplers.clear();
    impl_->shaderModules.clear();
    impl_->bindGroupLayouts.clear();
    impl_->bindGroups.clear();
    impl_->renderPipelines.clear();
    impl_->computePipelines.clear();
    
    // 释放 WebGPU 对象
    if (renderPassEncoder_) {
        wgpuRenderPassEncoderEndPass(renderPassEncoder_);
        wgpuRenderPassEncoderRelease(renderPassEncoder_);
        renderPassEncoder_ = nullptr;
    }
    
    if (computePassEncoder_) {
        wgpuComputePassEncoderEndPass(computePassEncoder_);
        wgpuComputePassEncoderRelease(computePassEncoder_);
        computePassEncoder_ = nullptr;
    }
    
    if (commandEncoder_) {
        wgpuCommandEncoderRelease(commandEncoder_);
        commandEncoder_ = nullptr;
    }
    
    if (swapChain_) {
        wgpuSwapChainRelease(swapChain_);
        swapChain_ = nullptr;
    }
    
    if (surface_) {
        wgpuSurfaceRelease(surface_);
        surface_ = nullptr;
    }
    
    if (queue_) {
        wgpuQueueRelease(queue_);
        queue_ = nullptr;
    }
    
    if (device_) {
        wgpuDeviceRelease(device_);
        device_ = nullptr;
    }
    
    if (adapter_) {
        wgpuAdapterRelease(adapter_);
        adapter_ = nullptr;
    }
    
    if (instance_) {
        wgpuInstanceRelease(instance_);
        instance_ = nullptr;
    }
    
    initialized_ = false;
}

// ============================================================================
// 适配器选择
// ============================================================================

bool WebGPURenderDevice::selectAdapter() {
    struct AdapterData {
        WGPUAdapter adapter = nullptr;
        bool found = false;
    };
    
    AdapterData adapterData;
    
    auto onAdapterCallback = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, const char* message, void* userdata) {
        auto* data = reinterpret_cast<AdapterData*>(userdata);
        if (status == WGPURequestAdapterStatus_Success) {
            data->adapter = adapter;
            data->found = true;
        } else {
            std::fprintf(stderr, "[WebGPU] Adapter request failed: %s\n", message ? message : "Unknown error");
        }
    };
    
#if defined(PHOENIX_PLATFORM_WEB)
    WGPURequestAdapterOptions options{};
    options.powerPreference = config_.enablePowerSaving ? WGPUPowerPreference_LowPower : WGPUPowerPreference_HighPerformance;
    options.compatibleSurface = surface_;
    emscripten_webgpu_request_adapter(&options, onAdapterCallback, &adapterData);
#else
    // 原生实现
#endif
    
    if (!adapterData.found || !adapterData.adapter) {
        return false;
    }
    
    adapter_ = adapterData.adapter;
    return true;
}

// ============================================================================
// 设备创建
// ============================================================================

bool WebGPURenderDevice::createDevice() {
    struct DeviceData {
        WGPUDevice device = nullptr;
        bool created = false;
    };
    
    DeviceData deviceData;
    
    // 配置设备特性
    WGPUFeatureName requiredFeatures[] = {
        WGPUFeatureName_TimestampQuery,
        WGPUFeatureName_DepthClipControl,
        WGPUFeatureName_Depth32FloatStencil8,
    };
    
    WGPUDeviceDescriptor desc{};
    desc.requiredFeatureCount = config_.enableTimestampQueries ? 3 : 1;
    desc.requiredFeatures = requiredFeatures;
    desc.deviceLostCallback = [](WGPUDeviceLostReason reason, const char* message, void* userdata) {
        std::fprintf(stderr, "[WebGPU] Device lost: %s\n", message ? message : "Unknown reason");
    };
    
    auto onDeviceCallback = [](WGPURequestDeviceStatus status, WGPUDevice device, const char* message, void* userdata) {
        auto* data = reinterpret_cast<DeviceData*>(userdata);
        if (status == WGPURequestDeviceStatus_Success) {
            data->device = device;
            data->created = true;
        } else {
            std::fprintf(stderr, "[WebGPU] Device request failed: %s\n", message ? message : "Unknown error");
        }
    };
    
    wgpuAdapterRequestDevice(adapter_, &desc, onDeviceCallback, &deviceData);
    
    if (!deviceData.created || !deviceData.device) {
        return false;
    }
    
    device_ = deviceData.device;
    return true;
}

// ============================================================================
// 设备信息初始化
// ============================================================================

void WebGPURenderDevice::initDeviceInfo() {
    WGPUSupportedLimits supportedLimits{};
    if (wgpuAdapterGetLimits(adapter_, &supportedLimits)) {
        deviceInfo_.maxTextureSize = supportedLimits.limits.maxTextureDimension2D;
        deviceInfo_.maxUniformBufferBindingSize = supportedLimits.limits.maxUniformBufferBindingSize;
        deviceInfo_.maxStorageBufferBindingSize = supportedLimits.limits.maxStorageBufferBindingSize;
        deviceInfo_.maxSamplersPerShaderStage = supportedLimits.limits.maxSamplersPerShaderStage;
        deviceInfo_.maxSampledTexturesPerShaderStage = supportedLimits.limits.maxSampledTexturesPerShaderStage;
        deviceInfo_.maxStorageTexturesPerShaderStage = supportedLimits.limits.maxStorageTexturesPerShaderStage;
        deviceInfo_.maxUniformBuffersPerShaderStage = supportedLimits.limits.maxUniformBuffersPerShaderStage;
        deviceInfo_.maxStorageBuffersPerShaderStage = supportedLimits.limits.maxStorageBuffersPerShaderStage;
        deviceInfo_.maxColorAttachments = supportedLimits.limits.maxColorAttachments;
        deviceInfo_.maxComputeWorkgroupStorageSize = supportedLimits.limits.maxComputeWorkgroupStorageSize;
        deviceInfo_.maxComputeInvocationsPerWorkgroup = supportedLimits.limits.maxComputeInvocationsPerWorkgroup;
        deviceInfo_.maxComputeWorkgroupSizeX = supportedLimits.limits.maxComputeWorkgroupSizeX;
        deviceInfo_.maxComputeWorkgroupSizeY = supportedLimits.limits.maxComputeWorkgroupSizeY;
        deviceInfo_.maxComputeWorkgroupSizeZ = supportedLimits.limits.maxComputeWorkgroupSizeZ;
    }
    
    // 获取适配器信息
    WGPUAdapterInfo adapterInfo{};
    if (wgpuAdapterGetInfo(adapter_, &adapterInfo)) {
        deviceInfo_.vendor = adapterInfo.vendor ? adapterInfo.vendor : "Unknown";
        deviceInfo_.architecture = adapterInfo.architecture ? adapterInfo.architecture : "Unknown";
        deviceInfo_.device = adapterInfo.device ? adapterInfo.device : "Unknown";
        deviceInfo_.driverDescription = adapterInfo.driverDescription ? adapterInfo.driverDescription : "Unknown";
        deviceInfo_.backendType = adapterInfo.backendType == WGPUBackendType_D3D12 ? "DirectX12" :
                                  adapterInfo.backendType == WGPUBackendType_Metal ? "Metal" :
                                  adapterInfo.backendType == WGPUBackendType_Vulkan ? "Vulkan" :
                                  adapterInfo.backendType == WGPUBackendType_GL ? "OpenGL" : "Unknown";
        wgpuAdapterInfoFreeMembers(adapterInfo);
    }
}

// ============================================================================
// 表面和交换链
// ============================================================================

bool WebGPURenderDevice::createSurface(void* windowHandle) {
#if defined(PHOENIX_PLATFORM_WEB)
    int32_t canvasId = reinterpret_cast<intptr_t>(windowHandle);
    if (canvasId <= 0) {
        canvasId = emscripten_webgpu_get_current_canvas();
    }
    surface_ = reinterpret_cast<WGPUSurface>(emscripten_webgpu_get_surface(canvasId, instance_));
    return surface_ != nullptr;
#else
    // 原生平台实现
    return false;
#endif
}

bool WebGPURenderDevice::configureSwapChain(uint32_t width, uint32_t height, TextureFormat format, bool vsync) {
    if (!surface_) {
        return false;
    }
    
    WGPUTextureFormat wgpuFormat = static_cast<WGPUTextureFormat>(format);
    
    WGPUSwapChainDescriptor desc{};
    desc.usage = WGPUTextureUsage_RenderAttachment;
    desc.format = wgpuFormat;
    desc.width = width;
    desc.height = height;
    desc.presentMode = vsync ? WGPUPresentMode_Fifo : WGPUPresentMode_Immediate;
    
    if (swapChain_) {
        wgpuSwapChainRelease(swapChain_);
    }
    
    swapChain_ = wgpuDeviceCreateSwapChain(device_, surface_, &desc);
    return swapChain_ != nullptr;
}

WGPUTextureView WebGPURenderDevice::getCurrentTextureView() {
    if (!swapChain_) {
        return nullptr;
    }
    
    if (impl_->currentTextureView) {
        wgpuTextureViewRelease(impl_->currentTextureView);
    }
    
    WGPUTexture texture = wgpuSwapChainGetCurrentTexture(swapChain_);
    if (!texture) {
        return nullptr;
    }
    
    WGPUTextureViewDescriptor viewDesc{};
    impl_->currentTextureView = wgpuTextureCreateView(texture, &viewDesc);
    return impl_->currentTextureView;
}

void WebGPURenderDevice::present() {
    // WebGPU 的 present 在 submit 时自动进行
}

void WebGPURenderDevice::resizeSwapChain(uint32_t width, uint32_t height) {
    configureSwapChain(width, height, TextureFormat::BGRA8Unorm, true);
}

// ============================================================================
// 缓冲区管理
// ============================================================================

BufferHandle WebGPURenderDevice::createBuffer(const BufferDesc& desc) {
    BufferHandle handle;
    handle.id = static_cast<uint32_t>(impl_->buffers.size()) + 1;
    
    WGPUBufferUsage usage = WGPUBufferUsage_None;
    if (desc.usage & BufferUsage::MapRead) usage |= WGPUBufferUsage_MapRead;
    if (desc.usage & BufferUsage::MapWrite) usage |= WGPUBufferUsage_MapWrite;
    if (desc.usage & BufferUsage::CopySrc) usage |= WGPUBufferUsage_CopySrc;
    if (desc.usage & BufferUsage::CopyDst) usage |= WGPUBufferUsage_CopyDst;
    if (desc.usage & BufferUsage::Index) usage |= WGPUBufferUsage_Index;
    if (desc.usage & BufferUsage::Vertex) usage |= WGPUBufferUsage_Vertex;
    if (desc.usage & BufferUsage::Uniform) usage |= WGPUBufferUsage_Uniform;
    if (desc.usage & BufferUsage::Storage) usage |= WGPUBufferUsage_Storage;
    if (desc.usage & BufferUsage::Indirect) usage |= WGPUBufferUsage_Indirect;
    if (desc.usage & BufferUsage::QueryResolve) usage |= WGPUBufferUsage_QueryResolve;
    
    WGPUBufferDescriptor bufferDesc{};
    bufferDesc.usage = usage;
    bufferDesc.size = desc.size;
    bufferDesc.mappedAtCreation = desc.mappedAtCreation;
    bufferDesc.label = nullptr;
    
    WGPUBuffer buffer = wgpuDeviceCreateBuffer(device_, &bufferDesc);
    if (buffer) {
        impl_->buffers.push_back(handle);
        
        if (desc.initialData && desc.mappedAtCreation) {
            void* mapped = wgpuBufferGetMappedRange(buffer, 0, desc.size);
            std::memcpy(mapped, desc.initialData, desc.size);
            wgpuBufferUnmap(buffer);
        }
    }
    
    return handle;
}

void WebGPURenderDevice::destroyBuffer(BufferHandle handle) {
    // 实现资源销毁
}

bool WebGPURenderDevice::mapBufferRead(BufferHandle handle, std::function<void(const void* data, uint64_t size)> callback) {
    // 实现映射读取
    return false;
}

bool WebGPURenderDevice::mapBufferWrite(BufferHandle handle, const void* data, uint64_t size) {
    // 实现映射写入
    return false;
}

void WebGPURenderDevice::unmapBuffer(BufferHandle handle) {
    // 实现取消映射
}

void WebGPURenderDevice::writeBuffer(BufferHandle handle, uint64_t offset, const void* data, uint64_t size) {
    // 实现缓冲区写入
}

void WebGPURenderDevice::copyBuffer(BufferHandle src, BufferHandle dst, uint64_t size, uint64_t srcOffset, uint64_t dstOffset) {
    // 实现缓冲区复制
}

// ============================================================================
// 纹理管理
// ============================================================================

TextureHandle WebGPURenderDevice::createTexture(const TextureDesc& desc) {
    TextureHandle handle;
    handle.id = static_cast<uint32_t>(impl_->textures.size()) + 1;
    impl_->textures.push_back(handle);
    return handle;
}

void WebGPURenderDevice::destroyTexture(TextureHandle handle) {
    // 实现纹理销毁
}

TextureViewHandle WebGPURenderDevice::createTextureView(TextureHandle texture) {
    TextureViewHandle handle;
    handle.id = static_cast<uint32_t>(impl_->textures.size()) + 1;
    return handle;
}

void WebGPURenderDevice::destroyTextureView(TextureViewHandle handle) {
    // 实现纹理视图销毁
}

void WebGPURenderDevice::writeTexture(TextureHandle texture, const void* data, uint64_t size, uint32_t mipLevel, uint32_t arrayLayer) {
    // 实现纹理写入
}

void WebGPURenderDevice::generateMipmaps(TextureHandle texture) {
    // 实现 mipmap 生成
}

// ============================================================================
// 采样器
// ============================================================================

SamplerHandle WebGPURenderDevice::createSampler() {
    SamplerHandle handle;
    handle.id = static_cast<uint32_t>(impl_->samplers.size()) + 1;
    impl_->samplers.push_back(handle);
    return handle;
}

void WebGPURenderDevice::destroySampler(SamplerHandle handle) {
    // 实现采样器销毁
}

// ============================================================================
// 着色器
// ============================================================================

ShaderModuleHandle WebGPURenderDevice::createShaderModuleWGSL(const char* wgslSource) {
    ShaderModuleHandle handle;
    handle.id = static_cast<uint32_t>(impl_->shaderModules.size()) + 1;
    
    WGPUShaderModuleWGSLDescriptor wgslDesc{};
    wgslDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
    wgslDesc.code = wgslSource;
    
    WGPUShaderModuleDescriptor desc{};
    desc.nextInChain = &wgslDesc.chain;
    desc.label = nullptr;
    
    WGPUShaderModule module = wgpuDeviceCreateShaderModule(device_, &desc);
    if (module) {
        impl_->shaderModules.push_back(handle);
    }
    
    return handle;
}

ShaderModuleHandle WebGPURenderDevice::createShaderModuleSPIRV(const uint32_t* spirv, uint32_t size) {
    ShaderModuleHandle handle;
    handle.id = static_cast<uint32_t>(impl_->shaderModules.size()) + 1;
    
    WGPUShaderModuleSPIRVDescriptor spirvDesc{};
    spirvDesc.chain.sType = WGPUSType_ShaderModuleSPIRVDescriptor;
    spirvDesc.codeSize = size;
    spirvDesc.code = spirv;
    
    WGPUShaderModuleDescriptor desc{};
    desc.nextInChain = &spirvDesc.chain;
    
    WGPUShaderModule module = wgpuDeviceCreateShaderModule(device_, &desc);
    if (module) {
        impl_->shaderModules.push_back(handle);
    }
    
    return handle;
}

void WebGPURenderDevice::destroyShaderModule(ShaderModuleHandle handle) {
    // 实现着色器模块销毁
}

// ============================================================================
// 绑定组
// ============================================================================

BindGroupLayoutHandle WebGPURenderDevice::createBindGroupLayout() {
    BindGroupLayoutHandle handle;
    handle.id = static_cast<uint32_t>(impl_->bindGroupLayouts.size()) + 1;
    impl_->bindGroupLayouts.push_back(handle);
    return handle;
}

BindGroupHandle WebGPURenderDevice::createBindGroup(BindGroupLayoutHandle layout) {
    BindGroupHandle handle;
    handle.id = static_cast<uint32_t>(impl_->bindGroups.size()) + 1;
    impl_->bindGroups.push_back(handle);
    return handle;
}

void WebGPURenderDevice::destroyBindGroupLayout(BindGroupLayoutHandle handle) {}
void WebGPURenderDevice::destroyBindGroup(BindGroupHandle handle) {}

// ============================================================================
// 管线
// ============================================================================

RenderPipelineHandle WebGPURenderDevice::createRenderPipeline() {
    RenderPipelineHandle handle;
    handle.id = static_cast<uint32_t>(impl_->renderPipelines.size()) + 1;
    impl_->renderPipelines.push_back(handle);
    return handle;
}

ComputePipelineHandle WebGPURenderDevice::createComputePipeline() {
    ComputePipelineHandle handle;
    handle.id = static_cast<uint32_t>(impl_->computePipelines.size()) + 1;
    impl_->computePipelines.push_back(handle);
    return handle;
}

void WebGPURenderDevice::destroyRenderPipeline(RenderPipelineHandle handle) {}
void WebGPURenderDevice::destroyComputePipeline(ComputePipelineHandle handle) {}

// ============================================================================
// 命令编码
// ============================================================================

void WebGPURenderDevice::beginCommandEncoding() {
    if (commandEncoder_) {
        wgpuCommandEncoderRelease(commandEncoder_);
    }
    
    WGPUCommandEncoderDescriptor desc{};
    desc.label = nullptr;
    commandEncoder_ = wgpuDeviceCreateCommandEncoder(device_, &desc);
}

void WebGPURenderDevice::beginRenderPass() {
    if (!commandEncoder_) {
        return;
    }
    
    WGPUTextureView attachment = getCurrentTextureView();
    if (!attachment) {
        return;
    }
    
    WGPURenderPassColorAttachment colorAttachment{};
    colorAttachment.view = attachment;
    colorAttachment.resolveTarget = nullptr;
    colorAttachment.loadOp = WGPULoadOp_Clear;
    colorAttachment.storeOp = WGPUStoreOp_Store;
    colorAttachment.clearValue = {0.0f, 0.0f, 0.0f, 1.0f};
    
    WGPURenderPassDescriptor desc{};
    desc.colorAttachmentCount = 1;
    desc.colorAttachments = &colorAttachment;
    desc.depthStencilAttachment = nullptr;
    desc.occlusionQuerySet = nullptr;
    
    renderPassEncoder_ = wgpuCommandEncoderBeginRenderPass(commandEncoder_, &desc);
    impl_->inRenderPass = true;
}

void WebGPURenderDevice::beginComputePass() {
    if (!commandEncoder_) {
        return;
    }
    
    WGPUComputePassDescriptor desc{};
    desc.label = nullptr;
    computePassEncoder_ = wgpuCommandEncoderBeginComputePass(commandEncoder_, &desc);
    impl_->inComputePass = true;
}

void WebGPURenderDevice::endRenderPass() {
    if (renderPassEncoder_ && impl_->inRenderPass) {
        wgpuRenderPassEncoderEndPass(renderPassEncoder_);
        wgpuRenderPassEncoderRelease(renderPassEncoder_);
        renderPassEncoder_ = nullptr;
        impl_->inRenderPass = false;
    }
}

void WebGPURenderDevice::endComputePass() {
    if (computePassEncoder_ && impl_->inComputePass) {
        wgpuComputePassEncoderEndPass(computePassEncoder_);
        wgpuComputePassEncoderRelease(computePassEncoder_);
        computePassEncoder_ = nullptr;
        impl_->inComputePass = false;
    }
}

void WebGPURenderDevice::submitCommands() {
    endRenderPass();
    endComputePass();
    
    if (commandEncoder_) {
        WGPUCommandBufferDescriptor desc{};
        desc.label = nullptr;
        WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(commandEncoder_, &desc);
        
        if (commandBuffer) {
            wgpuQueueSubmit(queue_, 1, &commandBuffer);
            wgpuCommandBufferRelease(commandBuffer);
        }
        
        wgpuCommandEncoderRelease(commandEncoder_);
        commandEncoder_ = nullptr;
    }
}

void WebGPURenderDevice::endCommandEncoding() {
    submitCommands();
}

// ============================================================================
// 绘制
// ============================================================================

void WebGPURenderDevice::setRenderPipeline(RenderPipelineHandle pipeline) {
    if (renderPassEncoder_) {
        // wgpuRenderPassEncoderSetPipeline(renderPassEncoder_, ...);
    }
}

void WebGPURenderDevice::setComputePipeline(ComputePipelineHandle pipeline) {
    if (computePassEncoder_) {
        // wgpuComputePassEncoderSetPipeline(computePassEncoder_, ...);
    }
}

void WebGPURenderDevice::setBindGroup(uint32_t index, BindGroupHandle group) {
    if (renderPassEncoder_) {
        // wgpuRenderPassEncoderSetBindGroup(renderPassEncoder_, index, ...);
    }
    if (computePassEncoder_) {
        // wgpuComputePassEncoderSetBindGroup(computePassEncoder_, index, ...);
    }
}

void WebGPURenderDevice::setVertexBuffer(uint32_t slot, BufferHandle buffer, uint64_t offset, uint64_t size) {
    if (renderPassEncoder_) {
        // wgpuRenderPassEncoderSetVertexBuffer(renderPassEncoder_, slot, ...);
    }
}

void WebGPURenderDevice::setIndexBuffer(BufferHandle buffer, uint64_t offset, uint64_t size) {
    if (renderPassEncoder_) {
        // wgpuRenderPassEncoderSetIndexBuffer(renderPassEncoder_, ...);
    }
}

void WebGPURenderDevice::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t baseInstance) {
    if (renderPassEncoder_) {
        wgpuRenderPassEncoderDraw(renderPassEncoder_, vertexCount, instanceCount, firstVertex, baseInstance);
    }
}

void WebGPURenderDevice::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t baseInstance) {
    if (renderPassEncoder_) {
        wgpuRenderPassEncoderDrawIndexed(renderPassEncoder_, indexCount, instanceCount, firstIndex, baseVertex, baseInstance);
    }
}

void WebGPURenderDevice::drawIndirect(BufferHandle buffer, uint64_t offset) {
    if (renderPassEncoder_) {
        // wgpuRenderPassEncoderDrawIndirect(renderPassEncoder_, ...);
    }
}

void WebGPURenderDevice::drawIndexedIndirect(BufferHandle buffer, uint64_t offset) {
    if (renderPassEncoder_) {
        // wgpuRenderPassEncoderDrawIndexedIndirect(renderPassEncoder_, ...);
    }
}

void WebGPURenderDevice::dispatchCompute(uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ) {
    if (computePassEncoder_) {
        wgpuComputePassEncoderDispatchWorkgroups(computePassEncoder_, workgroupCountX, workgroupCountY, workgroupCountZ);
    }
}

void WebGPURenderDevice::dispatchComputeIndirect(BufferHandle buffer, uint64_t offset) {
    if (computePassEncoder_) {
        // wgpuComputePassEncoderDispatchWorkgroupsIndirect(computePassEncoder_, ...);
    }
}

// ============================================================================
// 查询
// ============================================================================

QuerySetHandle WebGPURenderDevice::createQuerySet() {
    QuerySetHandle handle;
    handle.id = static_cast<uint32_t>(impl_->buffers.size()) + 1;
    return handle;
}

void WebGPURenderDevice::destroyQuerySet(QuerySetHandle handle) {}
void WebGPURenderDevice::beginOcclusionQuery(QuerySetHandle querySet, uint32_t queryIndex) {}
void WebGPURenderDevice::endOcclusionQuery() {}
void WebGPURenderDevice::resolveQuerySet(QuerySetHandle querySet, uint32_t firstQuery, uint32_t queryCount, BufferHandle dst, uint64_t dstOffset) {}

// ============================================================================
// 调试
// ============================================================================

void WebGPURenderDevice::setLabel(void* object, const char* label) {
    // WebGPU 对象标签设置
}

void WebGPURenderDevice::insertDebugMarker(const char* markerLabel) {
    if (renderPassEncoder_) {
        wgpuRenderPassEncoderInsertDebugMarker(renderPassEncoder_, markerLabel);
    }
    if (computePassEncoder_) {
        wgpuComputePassEncoderInsertDebugMarker(computePassEncoder_, markerLabel);
    }
}

void WebGPURenderDevice::pushDebugGroup(const char* groupLabel) {
    if (renderPassEncoder_) {
        wgpuRenderPassEncoderPushDebugGroup(renderPassEncoder_, groupLabel);
    }
    if (computePassEncoder_) {
        wgpuComputePassEncoderPushDebugGroup(computePassEncoder_, groupLabel);
    }
}

void WebGPURenderDevice::popDebugGroup() {
    if (renderPassEncoder_) {
        wgpuRenderPassEncoderPopDebugGroup(renderPassEncoder_);
    }
    if (computePassEncoder_) {
        wgpuComputePassEncoderPopDebugGroup(computePassEncoder_);
    }
}

} // namespace webgpu
} // namespace platform
} // namespace phoenix
