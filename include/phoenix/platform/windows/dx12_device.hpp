#pragma once

#include "../platform_types.hpp"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <memory>
#include <vector>
#include <string>

namespace phoenix {
namespace platform {
namespace windows {

using Microsoft::WRL::ComPtr;

// ============================================================================
// DX12 配置
// ============================================================================

struct DX12Config {
    bool enableDebugLayer = false;
    bool enableGPUBasedValidation = false;
    bool enableExperimentalFeatures = false;
    bool enableRaytracing = false;
    bool enableMeshShaders = false;
    bool enableSamplerFeedback = false;
    
    uint32_t frameLatency = 2;
    uint32_t rtvDescriptorCount = 1024;
    uint32_t dsvDescriptorCount = 64;
    uint32_t cbvSrvUavDescriptorCount = 1024 * 32;
    uint32_t samplerDescriptorCount = 2048;
    
    uint32_t uploadBufferSize = 256 * 1024 * 1024;  // 256MB
};

// ============================================================================
// DX12 设备信息
// ============================================================================

struct DX12DeviceInfo {
    std::string deviceName;
    std::string driverVersion;
    std::string vendor;
    
    uint32_t vendorId = 0;
    uint32_t deviceId = 0;
    uint32_t revisionId = 0;
    
    uint64_t dedicatedVideoMemory = 0;
    uint64_t dedicatedSystemMemory = 0;
    uint64_t sharedSystemMemory = 0;
    
    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    
    bool supportsRaytracing = false;
    bool supportsMeshShaders = false;
    bool supportsSamplerFeedback = false;
    bool supportsVariableRateShading = false;
    bool supportsDirectXRaytracingTier1_1 = false;
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
struct ResourceViewHandle : public ResourceHandle {};
struct PipelineStateHandle : public ResourceHandle {};
struct RootSignatureHandle : public ResourceHandle {};

// ============================================================================
// 描述符堆
// ============================================================================

class DescriptorHeap {
public:
    bool initialize(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count);
    void shutdown();
    
    [[nodiscard]] ID3D12DescriptorHeap* getHeap() const { return heap_.Get(); }
    [[nodiscard]] uint32_t getHandleIncrementSize() const { return handleIncrementSize_; }
    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE getCPUDescriptorHandle(uint32_t index) const;
    [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE getGPUDescriptorHandle(uint32_t index) const;
    [[nodiscard]] uint32_t allocate();
    void free(uint32_t index);
    
private:
    ComPtr<ID3D12DescriptorHeap> heap_;
    uint32_t handleIncrementSize_ = 0;
    uint32_t capacity_ = 0;
    std::vector<bool> allocated_;
};

// ============================================================================
// DX12 渲染设备
// ============================================================================

class DX12RenderDevice {
public:
    DX12RenderDevice();
    ~DX12RenderDevice();
    
    // 禁止拷贝
    DX12RenderDevice(const DX12RenderDevice&) = delete;
    DX12RenderDevice& operator=(const DX12RenderDevice&) = delete;
    
    /**
     * @brief 初始化设备
     */
    bool initialize(const DX12Config& config);
    
    /**
     * @brief 关闭设备
     */
    void shutdown();
    
    /**
     * @brief 检查设备是否有效
     */
    [[nodiscard]] bool isValid() const { return initialized_; }
    
    /**
     * @brief 获取 D3D12 设备
     */
    [[nodiscard]] ID3D12Device* getDevice() const { return device_.Get(); }
    
    /**
     * @brief 获取命令队列
     */
    [[nodiscard]] ID3D12CommandQueue* getCommandQueue() const { return commandQueue_.Get(); }
    
    /**
     * @brief 获取设备信息
     */
    [[nodiscard]] const DX12DeviceInfo& getDeviceInfo() const { return deviceInfo_; }
    
    // ==================== 交换链 ====================
    
    /**
     * @brief 创建交换链
     */
    bool createSwapChain(HWND hwnd, uint32_t width, uint32_t height, DXGI_FORMAT format, bool vsync);
    
    /**
     * @brief 获取当前后端缓冲区
     */
    [[nodiscard]] ID3D12Resource* getCurrentBackBuffer() const;
    
    /**
     * @brief 获取当前 RTV
     */
    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE getCurrentRTV() const;
    
    /**
     * @brief 呈现
     */
    HRESULT present(uint32_t syncInterval, uint32_t flags);
    
    /**
     * @brief 调整大小
     */
    HRESULT resize(uint32_t width, uint32_t height);
    
    // ==================== 帧管理 ====================
    
    /**
     * @brief 开始帧
     */
    void beginFrame();
    
    /**
     * @brief 结束帧
     */
    void endFrame();
    
    /**
     * @brief 获取当前帧索引
     */
    [[nodiscard]] uint32_t getCurrentFrameIndex() const { return frameIndex_; }
    
    /**
     * @brief 获取帧延迟对象
     */
    [[nodiscard]] HANDLE getFrameLatencyHandle() const { return frameLatencyEvent_; }
    
    // ==================== 命令列表 ====================
    
    /**
     * @brief 获取当前命令列表
     */
    [[nodiscard]] ID3D12GraphicsCommandList* getCommandList() const { return commandLists_[frameIndex_].Get(); }
    
    /**
     * @brief 重置命令列表
     */
    void resetCommandList();
    
    /**
     * @brief 关闭命令列表
     */
    void closeCommandList();
    
    /**
     * @brief 执行命令列表
     */
    void executeCommandList();
    
    // ==================== 资源管理 ====================
    
    /**
     * @brief 创建提交资源
     */
    [[nodiscard]] ComPtr<ID3D12Resource> createCommittedResource(
        const D3D12_HEAP_PROPERTIES& heapProps,
        D3D12_HEAP_FLAGS heapFlags,
        const D3D12_RESOURCE_DESC& resourceDesc,
        D3D12_RESOURCE_STATES initialState,
        const D3D12_CLEAR_VALUE* pOptimizedClearValue = nullptr
    );
    
    /**
     * @brief 创建提交缓冲资源
     */
    [[nodiscard]] ComPtr<ID3D12Resource> createCommittedBuffer(
        uint64_t size,
        D3D12_HEAP_TYPE heapType,
        D3D12_RESOURCE_STATES initialState
    );
    
    /**
     * @brief 创建提交纹理资源
     */
    [[nodiscard]] ComPtr<ID3D12Resource> createCommittedTexture(
        uint32_t width,
        uint32_t height,
        uint32_t mipLevels,
        uint32_t arraySize,
        DXGI_FORMAT format,
        D3D12_HEAP_TYPE heapType,
        D3D12_RESOURCE_STATES initialState
    );
    
    /**
     * @brief 创建上传堆
     */
    [[nodiscard]] ComPtr<ID3D12Resource> createUploadHeap(uint64_t size);
    
    // ==================== 描述符 ====================
    
    /**
     * @brief 创建 RTV
     */
    void createRenderTargetView(ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC* desc, D3D12_CPU_DESCRIPTOR_HANDLE rtv);
    
    /**
     * @brief 创建 DSV
     */
    void createDepthStencilView(ID3D12Resource* resource, const D3D12_DEPTH_STENCIL_VIEW_DESC* desc, D3D12_CPU_DESCRIPTOR_HANDLE dsv);
    
    /**
     * @brief 创建 CBV/SRV/UAV
     */
    void createConstantBufferView(const D3D12_CONSTANT_BUFFER_VIEW_DESC* desc, D3D12_CPU_DESCRIPTOR_HANDLE cbv);
    void createShaderResourceView(ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC* desc, D3D12_CPU_DESCRIPTOR_HANDLE srv);
    void createUnorderedAccessView(ID3D12Resource* resource, ID3D12Resource* counterResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC* desc, D3D12_CPU_DESCRIPTOR_HANDLE uav);
    
    /**
     * @brief 创建采样器
     */
    void createSampler(const D3D12_SAMPLER_DESC* desc, D3D12_CPU_DESCRIPTOR_HANDLE sampler);
    
    // ==================== Root Signature ====================
    
    /**
     * @brief 创建 Root Signature
     */
    [[nodiscard]] ComPtr<ID3D12RootSignature> createRootSignature(const D3D12_ROOT_SIGNATURE_DESC* desc);
    
    // ==================== PSO ====================
    
    /**
     * @brief 创建图形 PSO
     */
    [[nodiscard]] ComPtr<ID3D12PipelineState> createGraphicsPipelineState(
        const D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc
    );
    
    /**
     * @brief 创建计算 PSO
     */
    [[nodiscard]] ComPtr<ID3D12PipelineState> createComputePipelineState(
        const D3D12_COMPUTE_PIPELINE_STATE_DESC* desc
    );
    
    // ==================== 同步 ====================
    
    /**
     * @brief 创建栅栏
     */
    [[nodiscard]] ComPtr<ID3D12Fence> createFence(uint64_t initialValue, D3D12_FENCE_FLAGS flags);
    
    /**
     * @brief 信号栅栏
     */
    HRESULT signalFence(ID3D12Fence* fence, uint64_t value);
    
    /**
     * @brief 等待栅栏
     */
    HRESULT waitForFence(ID3D12Fence* fence, uint64_t value, uint64_t timeout = INFINITE);
    
    // ==================== 资源屏障 ====================
    
    /**
     * @brief 转换资源状态
     */
    void transitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter);
    
    // ==================== 调试 ====================
    
    /**
     * @brief 设置对象名称
     */
    void setName(ID3D12Object* object, const wchar_t* name);
    void setName(ID3D12Object* object, const char* name);

private:
    ComPtr<ID3D12Device> device_;
    ComPtr<IDXGISwapChain3> swapChain_;
    ComPtr<ID3D12CommandQueue> commandQueue_;
    ComPtr<ID3D12Fence> fence_;
    
    // 帧资源
    static constexpr uint32_t FrameCount = 3;
    ComPtr<ID3D12CommandAllocator> commandAllocators_[FrameCount];
    ComPtr<ID3D12GraphicsCommandList> commandLists_[FrameCount];
    ComPtr<ID3D12Resource> renderTargets_[FrameCount];
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[FrameCount];
    
    // 描述符堆
    ComPtr<ID3D12DescriptorHeap> rtvHeap_;
    ComPtr<ID3D12DescriptorHeap> dsvHeap_;
    ComPtr<ID3D12DescriptorHeap> cbvSrvUavHeap_;
    ComPtr<ID3D12DescriptorHeap> samplerHeap_;
    
    // 同步
    uint64_t fenceValues_[FrameCount] = {};
    HANDLE frameLatencyEvent_ = nullptr;
    uint32_t frameIndex_ = 0;
    
    // 配置和信息
    DX12Config config_;
    DX12DeviceInfo deviceInfo_;
    bool initialized_ = false;
    
    // 交换链状态
    uint32_t swapChainWidth_ = 0;
    uint32_t swapChainHeight_ = 0;
    DXGI_FORMAT swapChainFormat_ = DXGI_FORMAT_UNKNOWN;
    bool swapChainVsync_ = true;
    
    bool createDevice();
    bool createCommandQueue();
    bool createDescriptorHeaps();
    bool createFrameResources();
    void initDeviceInfo();
    void waitForGPU();
};

} // namespace windows
} // namespace platform
} // namespace phoenix
