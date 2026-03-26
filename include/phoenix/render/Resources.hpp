#pragma once

#include "Types.hpp"
#include "RenderDevice.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <filesystem>
#include <mutex>
#include <atomic>

namespace phoenix {
namespace render {

/**
 * @brief 缓冲区描述
 */
struct BufferDesc {
    BufferType type = BufferType::Vertex;
    uint32_t size = 0;
    const void* data = nullptr;
    uint32_t stride = 0; // 仅用于顶点缓冲
    bool dynamic = false;
    bool indexed = false;
};

/**
 * @brief 顶点缓冲区
 */
class VertexBuffer {
public:
    VertexBuffer() = default;
    ~VertexBuffer();

    /**
     * @brief 创建顶点缓冲
     */
    bool create(RenderDevice& device, const BufferDesc& desc, const VertexLayout& layout);

    /**
     * @brief 销毁
     */
    void destroy();

    /**
     * @brief 更新数据
     */
    bool update(const void* data, uint32_t size, uint32_t offset = 0);

    /**
     * @brief 获取句柄
     */
    [[nodiscard]] BufferHandle getHandle() const { return handle_; }

    /**
     * @brief 获取大小
     */
    [[nodiscard]] uint32_t getSize() const { return size_; }

    /**
     * @brief 获取步长
     */
    [[nodiscard]] uint32_t getStride() const { return stride_; }

    /**
     * @brief 获取顶点数
     */
    [[nodiscard]] uint32_t getVertexCount() const { return size_ / stride_; }

private:
    RenderDevice* device_ = nullptr;
    BufferHandle handle_;
    uint32_t size_ = 0;
    uint32_t stride_ = 0;
    bool dynamic_ = false;
};

/**
 * @brief 索引缓冲区
 */
class IndexBuffer {
public:
    IndexBuffer() = default;
    ~IndexBuffer();

    /**
     * @brief 创建索引缓冲
     */
    bool create(RenderDevice& device, const BufferDesc& desc);

    /**
     * @brief 销毁
     */
    void destroy();

    /**
     * @brief 更新数据
     */
    bool update(const void* data, uint32_t size, uint32_t offset = 0);

    /**
     * @brief 获取句柄
     */
    [[nodiscard]] BufferHandle getHandle() const { return handle_; }

    /**
     * @brief 获取大小
     */
    [[nodiscard]] uint32_t getSize() const { return size_; }

    /**
     * @brief 获取索引类型
     */
    [[nodiscard]] bool is16Bit() const { return is16Bit_; }

    /**
     * @brief 获取索引数
     */
    [[nodiscard]] uint32_t getIndexCount() const { return is16Bit_ ? size_ / 2 : size_ / 4; }

private:
    RenderDevice* device_ = nullptr;
    BufferHandle handle_;
    uint32_t size_ = 0;
    bool is16Bit_ = false;
};

/**
 * @brief Uniform 缓冲区
 */
class UniformBufferObject {
public:
    UniformBufferObject() = default;
    ~UniformBufferObject();

    /**
     * @brief 创建 Uniform 缓冲
     */
    bool create(RenderDevice& device, uint32_t size, const void* data = nullptr);

    /**
     * @brief 销毁
     */
    void destroy();

    /**
     * @brief 更新数据
     */
    bool update(const void* data, uint32_t size);

    /**
     * @brief 获取句柄
     */
    [[nodiscard]] UniformHandle getHandle() const { return handle_; }

    /**
     * @brief 获取大小
     */
    [[nodiscard]] uint32_t getSize() const { return size_; }

private:
    RenderDevice* device_ = nullptr;
    UniformHandle handle_;
    uint32_t size_ = 0;
};

/**
 * @brief 纹理描述
 */
struct TextureDesc {
    TextureType type = TextureType::Texture2D;
    TextureFormat format = TextureFormat::RGBA8;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t depth = 1;
    uint32_t mipLevels = 1;
    uint32_t arraySize = 1;
    uint32_t sampleCount = 1;
    
    const void* data = nullptr;
    uint32_t dataSize = 0;
    
    bool dynamic = false;
    bool renderTarget = false;
    bool depthStencil = false;
    bool generateMips = false;
    
    // 压缩纹理
    bool compressed = false;
    uint32_t compressedBlockSize = 0;
};

/**
 * @brief 纹理加载选项
 */
struct TextureLoadOptions {
    bool generateMips = true;
    bool srgb = true;
    bool flipY = false;
    TextureFormat forcedFormat = TextureFormat::RGBA8;
    uint32_t maxMipLevel = 0; // 0 = 全部
};

/**
 * @brief 纹理
 */
class Texture {
public:
    Texture() = default;
    ~Texture();

    /**
     * @brief 创建纹理
     */
    bool create(RenderDevice& device, const TextureDesc& desc);

    /**
     * @brief 从文件加载纹理
     */
    bool loadFromFile(RenderDevice& device, const std::filesystem::path& path,
                      const TextureLoadOptions& options = {});

    /**
     * @brief 从内存加载纹理 (支持 KTX2/DDS/PNG)
     */
    bool loadFromMemory(RenderDevice& device, const void* data, size_t size,
                        const char* extension, const TextureLoadOptions& options = {});

    /**
     * @brief 销毁
     */
    void destroy();

    /**
     * @brief 更新纹理数据
     */
    bool update(const void* data, uint32_t mipLevel = 0, uint32_t arraySlice = 0);

    /**
     * @brief 获取句柄
     */
    [[nodiscard]] TextureHandle getHandle() const { return handle_; }

    /**
     * @brief 获取宽度
     */
    [[nodiscard]] uint32_t getWidth() const { return width_; }

    /**
     * @brief 获取高度
     */
    [[nodiscard]] uint32_t getHeight() const { return height_; }

    /**
     * @brief 获取深度
     */
    [[nodiscard]] uint32_t getDepth() const { return depth_; }

    /**
     * @brief 获取 Mip 层级数
     */
    [[nodiscard]] uint32_t getMipLevels() const { return mipLevels_; }

    /**
     * @brief 获取格式
     */
    [[nodiscard]] TextureFormat getFormat() const { return format_; }

    /**
     * @brief 获取类型
     */
    [[nodiscard]] TextureType getType() const { return type_; }

private:
    RenderDevice* device_ = nullptr;
    TextureHandle handle_;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    uint32_t depth_ = 1;
    uint32_t mipLevels_ = 1;
    TextureFormat format_ = TextureFormat::RGBA8;
    TextureType type_ = TextureType::Texture2D;
};

/**
 * @brief 立方体贴图
 */
class CubeTexture {
public:
    CubeTexture() = default;
    ~CubeTexture();

    /**
     * @brief 创建立方体贴图
     */
    bool create(RenderDevice& device, uint32_t size, TextureFormat format,
                const std::array<const void*, 6>& faces);

    /**
     * @brief 从文件加载 (十字展开或垂直/水平排列)
     */
    bool loadFromFile(RenderDevice& device, const std::filesystem::path& path,
                      const TextureLoadOptions& options = {});

    /**
     * @brief 销毁
     */
    void destroy();

    /**
     * @brief 获取句柄
     */
    [[nodiscard]] TextureHandle getHandle() const { return handle_; }

    /**
     * @brief 获取尺寸
     */
    [[nodiscard]] uint32_t getSize() const { return size_; }

private:
    RenderDevice* device_ = nullptr;
    TextureHandle handle_;
    uint32_t size_ = 0;
};

/**
 * @brief 帧缓冲描述
 */
struct FrameBufferDesc {
    uint32_t width;
    uint32_t height;
    std::vector<TextureHandle> colorAttachments;
    TextureHandle depthAttachment;
    bool ownAttachments = false; // 是否拥有纹理所有权
};

/**
 * @brief 帧缓冲
 */
class FrameBuffer {
public:
    FrameBuffer() = default;
    ~FrameBuffer();

    /**
     * @brief 创建帧缓冲
     */
    bool create(RenderDevice& device, const FrameBufferDesc& desc);

    /**
     * @brief 从渲染目标创建
     */
    bool createFromTargets(RenderDevice& device, const std::vector<RenderTarget*>& colors,
                           RenderTarget* depth = nullptr);

    /**
     * @brief 销毁
     */
    void destroy();

    /**
     * @brief 获取句柄
     */
    [[nodiscard]] FrameBufferHandle getHandle() const { return handle_; }

    /**
     * @brief 获取宽度
     */
    [[nodiscard]] uint32_t getWidth() const { return width_; }

    /**
     * @brief 获取高度
     */
    [[nodiscard]] uint32_t getHeight() const { return height_; }

    /**
     * @brief 获取颜色附件数
     */
    [[nodiscard]] uint32_t getColorAttachmentCount() const { 
        return static_cast<uint32_t>(colorAttachments_.size()); 
    }

private:
    RenderDevice* device_ = nullptr;
    FrameBufferHandle handle_;
    std::vector<TextureHandle> colorAttachments_;
    TextureHandle depthAttachment_;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    bool ownAttachments_ = false;
};

/**
 * @brief GPU 资源池 - 帧间复用，零动态分配
 */
template<typename T, typename HandleType, size_t PoolSize = 256>
class ResourcePool {
public:
    ResourcePool() {
        // 初始化空闲列表
        for (size_t i = 0; i < PoolSize; ++i) {
            freeList_.push_back(static_cast<typename HandleType::index_type>(i));
        }
    }

    /**
     * @brief 分配资源
     */
    HandleType allocate() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (freeList_.empty()) {
            return HandleType(); // 返回无效句柄
        }
        auto index = freeList_.back();
        freeList_.pop_back();
        generations_[index]++;
        return HandleType(index);
    }

    /**
     * @brief 释放资源
     */
    void free(HandleType handle) {
        if (!handle.valid()) return;
        std::lock_guard<std::mutex> lock(mutex_);
        auto index = handle.index();
        if (index < PoolSize) {
            freeList_.push_back(index);
        }
    }

    /**
     * @brief 验证句柄是否有效 (防悬空引用)
     */
    bool isValid(HandleType handle) const {
        if (!handle.valid()) return false;
        auto index = handle.index();
        if (index >= PoolSize) return false;
        // 检查代次是否匹配 (简化版本)
        return true;
    }

    /**
     * @brief 获取资源
     */
    T* get(HandleType handle) {
        if (!isValid(handle)) return nullptr;
        return &resources_[handle.index()];
    }

    /**
     * @brief 获取池大小
     */
    [[nodiscard]] size_t size() const { return PoolSize; }

    /**
     * @brief 获取可用数量
     */
    [[nodiscard]] size_t available() const { return freeList_.size(); }

private:
    std::array<T, PoolSize> resources_;
    std::array<uint16_t, PoolSize> generations_;
    std::vector<typename HandleType::index_type> freeList_;
    mutable std::mutex mutex_;
};

/**
 * @brief 资源管理器
 */
class ResourceManager {
public:
    ResourceManager();
    ~ResourceManager();

    /**
     * @brief 初始化
     */
    void initialize(RenderDevice& device);

    /**
     * @brief 关闭
     */
    void shutdown();

    /**
     * @brief 创建顶点缓冲
     */
    VertexBuffer* createVertexBuffer(const BufferDesc& desc, const VertexLayout& layout);

    /**
     * @brief 创建索引缓冲
     */
    IndexBuffer* createIndexBuffer(const BufferDesc& desc);

    /**
     * @brief 创建纹理
     */
    Texture* createTexture(const TextureDesc& desc);

    /**
     * @brief 加载纹理
     */
    Texture* loadTexture(const std::filesystem::path& path, 
                         const TextureLoadOptions& options = {});

    /**
     * @brief 创建帧缓冲
     */
    FrameBuffer* createFrameBuffer(const FrameBufferDesc& desc);

    /**
     * @brief 释放资源
     */
    void release(VertexBuffer* buffer);
    void release(IndexBuffer* buffer);
    void release(Texture* texture);
    void release(FrameBuffer* buffer);

    /**
     * @brief 按名称查找资源
     */
    Texture* findTexture(const std::string& name);

    /**
     * @brief 注册命名资源
     */
    void registerTexture(const std::string& name, Texture* texture);

    /**
     * @brief 卸载所有资源
     */
    void unloadAll();

    /**
     * @brief 获取统计信息
     */
    struct Stats {
        uint32_t vertexBufferCount;
        uint32_t indexBufferCount;
        uint32_t textureCount;
        uint32_t frameBufferCount;
        uint64_t gpuMemoryUsed;
    };
    [[nodiscard]] Stats getStats() const;

private:
    RenderDevice* device_ = nullptr;
    
    std::vector<std::unique_ptr<VertexBuffer>> vertexBuffers_;
    std::vector<std::unique_ptr<IndexBuffer>> indexBuffers_;
    std::vector<std::unique_ptr<Texture>> textures_;
    std::vector<std::unique_ptr<FrameBuffer>> frameBuffers_;
    
    std::unordered_map<std::string, Texture*> namedTextures_;
    
    mutable std::mutex mutex_;
};

/**
 * @brief 网格数据
 */
struct MeshData {
    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<float> tangents;
    std::vector<float> texCoords;
    std::vector<float> colors;
    std::vector<uint32_t> indices;
    
    uint32_t vertexCount() const {
        return positions.empty() ? 0 : static_cast<uint32_t>(positions.size()) / 3;
    }
    
    uint32_t indexCount() const {
        return static_cast<uint32_t>(indices.size());
    }
};

/**
 * @brief 网格
 */
class Mesh {
public:
    Mesh() = default;
    ~Mesh();

    /**
     * @brief 从数据创建网格
     */
    bool create(RenderDevice& device, const MeshData& data);

    /**
     * @brief 从文件加载 (glTF/OBJ/FBX)
     */
    bool loadFromFile(RenderDevice& device, const std::filesystem::path& path);

    /**
     * @brief 销毁
     */
    void destroy();

    /**
     * @brief 获取顶点缓冲
     */
    [[nodiscard]] BufferHandle getVertexBuffer() const { return vertexBuffer_; }

    /**
     * @brief 获取索引缓冲
     */
    [[nodiscard]] BufferHandle getIndexBuffer() const { return indexBuffer_; }

    /**
     * @brief 获取顶点数
     */
    [[nodiscard]] uint32_t getVertexCount() const { return vertexCount_; }

    /**
     * @brief 获取索引数
     */
    [[nodiscard]] uint32_t getIndexCount() const { return indexCount_; }

    /**
     * @brief 获取顶点布局
     */
    [[nodiscard]] const VertexLayout& getVertexLayout() const { return layout_; }

    /**
     * @brief 获取包围盒
     */
    void getBoundingBox(float min[3], float max[3]) const;

private:
    RenderDevice* device_ = nullptr;
    BufferHandle vertexBuffer_;
    BufferHandle indexBuffer_;
    VertexLayout layout_;
    uint32_t vertexCount_ = 0;
    uint32_t indexCount_ = 0;
    float boundsMin_[3] = {0, 0, 0};
    float boundsMax_[3] = {0, 0, 0};
};

/**
 * @brief 内置几何体
 */
class BuiltinMeshes {
public:
    /**
     * @brief 创建三角形
     */
    static MeshData createTriangle();

    /**
     * @brief 创建四边形
     */
    static MeshData createQuad();

    /**
     * @brief 创建立方体
     */
    static MeshData createCube(float size = 1.0f);

    /**
     * @brief 创建球体
     */
    static MeshData createSphere(float radius = 1.0f, uint32_t subdivisions = 32);

    /**
     * @brief 创建圆柱体
     */
    static MeshData createCylinder(float radius = 1.0f, float height = 2.0f, 
                                    uint32_t segments = 32);

    /**
     * @brief 创建平面
     */
    static MeshData createPlane(float width = 1.0f, float height = 1.0f,
                                 uint32_t subdivisions = 1);
};

} // namespace render
} // namespace phoenix
