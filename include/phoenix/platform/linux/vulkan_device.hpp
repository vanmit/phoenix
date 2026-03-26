#pragma once

#include "../platform_types.hpp"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <string>
#include <functional>

namespace phoenix {
namespace platform {
namespace linux_platform {

// ============================================================================
// Vulkan 配置
// ============================================================================

struct VulkanConfig {
    bool enableValidation = false;
    bool enableDebugUtils = true;
    bool enableSyncValidation = false;
    bool enableGPUBasedValidation = false;
    
    uint32_t desiredPresentMode = 0;  // 0=FIFO, 1=FIFO_RELAXED, 2=MAILBOX, 3=IMMEDIATE
    uint32_t imageCount = 3;
    uint32_t maxFramesInFlight = 2;
    
    // 特性
    bool enableRayTracing = false;
    bool enableMeshShaders = false;
    bool enableVariableRateShading = false;
    bool enableTimelineSemaphore = true;
    bool enableBufferDeviceAddress = true;
};

// ============================================================================
// Vulkan 设备信息
// ============================================================================

struct VulkanDeviceInfo {
    std::string deviceName;
    std::string driverVersion;
    std::string apiVersion;
    std::string vendor;
    
    uint32_t vendorId = 0;
    uint32_t deviceId = 0;
    uint32_t driverVersionRaw = 0;
    uint32_t apiVersionRaw = 0;
    
    VkPhysicalDeviceType deviceType = VK_PHYSICAL_DEVICE_TYPE_OTHER;
    
    uint64_t dedicatedMemory = 0;
    uint64_t sharedMemory = 0;
    
    VkPhysicalDeviceFeatures features{};
    VkPhysicalDeviceProperties properties{};
    
    bool supportsRayTracing = false;
    bool supportsMeshShaders = false;
    bool supportsVariableRateShading = false;
    bool supportsTimelineSemaphore = false;
    bool supportsBufferDeviceAddress = false;
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
struct ImageHandle : public ResourceHandle {};
struct ImageViewHandle : public ResourceHandle {};
struct SamplerHandle : public ResourceHandle {};
struct PipelineHandle : public ResourceHandle {};
struct PipelineLayoutHandle : public ResourceHandle {};
struct DescriptorSetLayoutHandle : public ResourceHandle {};
struct DescriptorSetHandle : public ResourceHandle {};

// ============================================================================
// Vulkan 渲染设备
// ============================================================================

class VulkanRenderDevice {
public:
    VulkanRenderDevice();
    ~VulkanRenderDevice();
    
    // 禁止拷贝
    VulkanRenderDevice(const VulkanRenderDevice&) = delete;
    VulkanRenderDevice& operator=(const VulkanRenderDevice&) = delete;
    
    /**
     * @brief 初始化设备
     */
    bool initialize(const VulkanConfig& config);
    
    /**
     * @brief 关闭设备
     */
    void shutdown();
    
    /**
     * @brief 检查设备是否有效
     */
    [[nodiscard]] bool isValid() const { return initialized_; }
    
    /**
     * @brief 获取 Vulkan 实例
     */
    [[nodiscard]] VkInstance getInstance() const { return instance_; }
    
    /**
     * @brief 获取物理设备
     */
    [[nodiscard]] VkPhysicalDevice getPhysicalDevice() const { return physicalDevice_; }
    
    /**
     * @brief 获取逻辑设备
     */
    [[nodiscard]] VkDevice getDevice() const { return device_; }
    
    /**
     * @brief 获取设备信息
     */
    [[nodiscard]] const VulkanDeviceInfo& getDeviceInfo() const { return deviceInfo_; }
    
    // ==================== 表面 ====================
    
    /**
     * @brief 创建表面
     */
    bool createSurface(void* display, void* window);  // Display*, Window
    
    /**
     * @brief 创建 Wayland 表面
     */
    bool createWaylandSurface(void* display, void* surface);  // wl_display*, wl_surface*
    
    /**
     * @brief 销毁表面
     */
    void destroySurface();
    
    // ==================== 交换链 ====================
    
    /**
     * @brief 创建交换链
     */
    bool createSwapChain(uint32_t width, uint32_t height, VkFormat format, bool vsync);
    
    /**
     * @brief 重新创建交换链
     */
    bool recreateSwapChain();
    
    /**
     * @brief 获取交换链图像计数
     */
    [[nodiscard]] uint32_t getSwapChainImageCount() const { return swapChainImageCount_; }
    
    /**
     * @brief 获取当前图像索引
     */
    [[nodiscard]] uint32_t getCurrentImageIndex() const { return currentImageIndex_; }
    
    /**
     * @brief 获取当前图像
     */
    [[nodiscard]] VkImage getCurrentImage() const;
    
    /**
     * @brief 获取当前图像视图
     */
    [[nodiscard]] VkImageView getCurrentImageView() const;
    
    /**
     * @brief 呈现
     */
    VkResult present();
    
    // ==================== 帧管理 ====================
    
    /**
     * @brief 开始帧
     */
    VkResult beginFrame();
    
    /**
     * @brief 结束帧
     */
    VkResult endFrame();
    
    /**
     * @brief 等待 GPU 空闲
     */
    void waitForGPU();
    
    /**
     * @brief 等待帧完成
     */
    void waitForFrame(uint32_t frameIndex);
    
    // ==================== 命令缓冲 ====================
    
    /**
     * @brief 获取当前命令缓冲
     */
    [[nodiscard]] VkCommandBuffer getCurrentCommandBuffer() const;
    
    /**
     * @brief 开始命令缓冲录制
     */
    void beginCommandBuffer();
    
    /**
     * @brief 结束命令缓冲录制
     */
    void endCommandBuffer();
    
    // ==================== 同步对象 ====================
    
    /**
     * @brief 获取当前信号量
     */
    [[nodiscard]] VkSemaphore getCurrentImageAvailableSemaphore() const;
    [[nodiscard]] VkSemaphore getCurrentRenderFinishedSemaphore() const;
    [[nodiscard]] VkFence getCurrentInFlightFence() const;
    
    // ==================== 队列 ====================
    
    /**
     * @brief 获取图形队列
     */
    [[nodiscard]] VkQueue getGraphicsQueue() const { return graphicsQueue_; }
    
    /**
     * @brief 获取呈现队列
     */
    [[nodiscard]] VkQueue getPresentQueue() const { return presentQueue_; }
    
    /**
     * @brief 获取计算队列
     */
    [[nodiscard]] VkQueue getComputeQueue() const { return computeQueue_; }
    
    /**
     * @brief 获取传输队列
     */
    [[nodiscard]] VkQueue getTransferQueue() const { return transferQueue_; }
    
    // ==================== 内存管理 ====================
    
    /**
     * @brief 分配内存
     */
    VkResult allocateMemory(VkMemoryRequirements* memRequirements, VkMemoryPropertyFlags properties, VkDeviceMemory* memory);
    
    /**
     * @brief 查找内存类型
     */
    [[nodiscard]] uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    
    // ==================== 缓冲区 ====================
    
    /**
     * @brief 创建缓冲区
     */
    [[nodiscard]] BufferHandle createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    
    /**
     * @brief 创建缓冲并分配内存
     */
    [[nodiscard]] BufferHandle createBufferWithMemory(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkDeviceMemory* memory, VkDeviceSize* offset);
    
    /**
     * @brief 销毁缓冲区
     */
    void destroyBuffer(BufferHandle handle);
    
    /**
     * @brief 复制缓冲区
     */
    void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size, VkCommandBuffer commandBuffer = VK_NULL_HANDLE);
    
    // ==================== 图像 ====================
    
    /**
     * @brief 创建图像
     */
    [[nodiscard]] ImageHandle createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
    
    /**
     * @brief 创建图像视图
     */
    [[nodiscard]] ImageViewHandle createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels = 1, uint32_t arrayLayers = 1);
    
    /**
     * @brief 销毁图像
     */
    void destroyImage(ImageHandle handle);
    
    /**
     * @brief 销毁图像视图
     */
    void destroyImageView(ImageViewHandle handle);
    
    // ==================== 纹理 ====================
    
    /**
     * @brief 创建纹理
     */
    [[nodiscard]] TextureHandle createTexture(uint32_t width, uint32_t height, const void* pixels, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM);
    
    /**
     * @brief 生成 Mipmaps
     */
    void generateMipmaps(VkImage image, VkFormat format, int32_t width, int32_t height, uint32_t mipLevels);
    
    // ==================== 采样器 ====================
    
    /**
     * @brief 创建采样器
     */
    [[nodiscard]] SamplerHandle createSampler(const VkSamplerCreateInfo* createInfo);
    
    /**
     * @brief 销毁采样器
     */
    void destroySampler(SamplerHandle handle);
    
    // ==================== 描述符 ====================
    
    /**
     * @brief 创建描述符集布局
     */
    [[nodiscard]] DescriptorSetLayoutHandle createDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings);
    
    /**
     * @brief 创建描述符池
     */
    [[nodiscard]] VkDescriptorPool createDescriptorPool(const std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t maxSets);
    
    /**
     * @brief 创建描述符集
     */
    [[nodiscard]] DescriptorSetHandle createDescriptorSet(VkDescriptorPool pool, DescriptorSetLayoutHandle layout);
    
    /**
     * @brief 更新描述符集
     */
    void updateDescriptorSets(VkWriteDescriptorSet* writes, uint32_t writeCount, VkCopyDescriptorSet* copies = nullptr, uint32_t copyCount = 0);
    
    // ==================== 管线 ====================
    
    /**
     * @brief 创建管线布局
     */
    [[nodiscard]] PipelineLayoutHandle createPipelineLayout(DescriptorSetLayoutHandle* setLayouts, uint32_t setLayoutCount, const VkPushConstantRange* pushConstantRanges = nullptr, uint32_t pushConstantRangeCount = 0);
    
    /**
     * @brief 创建图形管线
     */
    [[nodiscard]] PipelineHandle createGraphicsPipeline(const VkGraphicsPipelineCreateInfo* createInfo);
    
    /**
     * @brief 创建计算管线
     */
    [[nodiscard]] PipelineHandle createComputePipeline(const VkComputePipelineCreateInfo* createInfo);
    
    /**
     * @brief 销毁管线
     */
    void destroyPipeline(PipelineHandle handle);
    
    /**
     * @brief 销毁管线布局
     */
    void destroyPipelineLayout(PipelineLayoutHandle handle);
    
    // ==================== 着色器模块 ====================
    
    /**
     * @brief 创建着色器模块
     */
    [[nodiscard]] VkShaderModule createShaderModule(const uint32_t* code, size_t size);
    
    /**
     * @brief 销毁着色器模块
     */
    void destroyShaderModule(VkShaderModule module);
    
    // ==================== 资源屏障 ====================
    
    /**
     * @brief 插入图像屏障
     */
    void insertImageMemoryBarrier(VkImage image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
                                   VkImageLayout oldLayout, VkImageLayout newLayout,
                                   VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
                                   VkImageSubresourceRange subresourceRange);
    
    /**
     * @brief 插入缓冲区屏障
     */
    void insertBufferMemoryBarrier(VkBuffer buffer, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
                                    VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
                                    VkDeviceSize offset, VkDeviceSize size);
    
    // ==================== 调试 ====================
    
    /**
     * @brief 设置对象名称
     */
    void setObjectName(uint64_t object, VkObjectType objectType, const char* name);
    
    /**
     * @brief 开始调试区域
     */
    void beginDebugRegion(const char* name, const float* color);
    
    /**
     * @brief 结束调试区域
     */
    void endDebugRegion();
    
    /**
     * @brief 插入调试标签
     */
    void insertDebugLabel(const char* label, const float* color);

private:
    VkInstance instance_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    
    VkSwapchainKHR swapChain_ = VK_NULL_HANDLE;
    std::vector<VkImage> swapChainImages_;
    std::vector<VkImageView> swapChainImageViews_;
    uint32_t swapChainImageCount_ = 0;
    uint32_t currentImageIndex_ = 0;
    uint32_t swapChainWidth_ = 0;
    uint32_t swapChainHeight_ = 0;
    
    // 队列
    uint32_t graphicsQueueFamily_ = 0;
    uint32_t presentQueueFamily_ = 0;
    uint32_t computeQueueFamily_ = 0;
    uint32_t transferQueueFamily_ = 0;
    
    VkQueue graphicsQueue_ = VK_NULL_HANDLE;
    VkQueue presentQueue_ = VK_NULL_HANDLE;
    VkQueue computeQueue_ = VK_NULL_HANDLE;
    VkQueue transferQueue_ = VK_NULL_HANDLE;
    
    // 帧资源
    struct FrameResources {
        VkCommandPool commandPool = VK_NULL_HANDLE;
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
        VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
        VkFence inFlightFence = VK_NULL_HANDLE;
    };
    
    std::vector<FrameResources> frameResources_;
    uint32_t currentFrame_ = 0;
    
    // 配置和信息
    VulkanConfig config_;
    VulkanDeviceInfo deviceInfo_;
    bool initialized_ = false;
    
    // 调试
    VkDebugUtilsMessengerEXT debugMessenger_ = VK_NULL_HANDLE;
    bool hasDebugUtils_ = false;
    
    // 扩展
    std::vector<const char*> enabledExtensions_;
    std::vector<const char*> enabledDeviceExtensions_;
    
    bool createInstance();
    bool setupDebugMessenger();
    bool selectPhysicalDevice();
    bool createLogicalDevice();
    bool createSwapChainSupport();
    void initDeviceInfo();
    
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                         VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                         const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                         void* pUserData);
};

} // namespace linux_platform
} // namespace platform
} // namespace phoenix
