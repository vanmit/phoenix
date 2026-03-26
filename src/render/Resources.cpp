#include "phoenix/render/Resources.hpp"
#include <bgfx/bgfx.h>
#include <cstring>
#include <cmath>

namespace phoenix {
namespace render {

// ==================== VertexBuffer 实现 ====================

VertexBuffer::~VertexBuffer() {
    destroy();
}

bool VertexBuffer::create(RenderDevice& device, const BufferDesc& desc, const VertexLayout& layout) {
    device_ = &device;
    size_ = desc.size;
    stride_ = desc.stride > 0 ? desc.stride : layout.getStride();
    dynamic_ = desc.dynamic;
    
    bgfx::VertexLayout bgfxLayout;
    bgfxLayout.begin();
    
    for (uint32_t i = 0; i < layout.attributeCount; ++i) {
        const auto& attr = layout.attributes[i];
        bgfx::Attrib::Enum attrib = bgfx::Attrib::Position;
        
        switch (attr.semantic) {
            case VertexAttribSemantic::Position: attrib = bgfx::Attrib::Position; break;
            case VertexAttribSemantic::Normal: attrib = bgfx::Attrib::Normal; break;
            case VertexAttribSemantic::Tangent: attrib = bgfx::Attrib::Tangent; break;
            case VertexAttribSemantic::TexCoord0: attrib = bgfx::Attrib::TexCoord0; break;
            case VertexAttribSemantic::TexCoord1: attrib = bgfx::Attrib::TexCoord1; break;
            case VertexAttribSemantic::TexCoord2: attrib = bgfx::Attrib::TexCoord2; break;
            case VertexAttribSemantic::TexCoord3: attrib = bgfx::Attrib::TexCoord3; break;
            case VertexAttribSemantic::Color0: attrib = bgfx::Attrib::Color0; break;
            case VertexAttribSemantic::Color1: attrib = bgfx::Attrib::Color1; break;
            case VertexAttribSemantic::Indices: attrib = bgfx::Attrib::Indices; break;
            case VertexAttribSemantic::Weight: attrib = bgfx::Attrib::Weight; break;
            default: attrib = bgfx::Attrib::TexCoord0; break;
        }
        
        bgfx::AttribType::Enum type = bgfx::AttribType::Float;
        uint8_t num = 3;
        bool normalized = attr.normalized;
        
        switch (attr.format) {
            case VertexAttribFormat::Float1: type = bgfx::AttribType::Float; num = 1; break;
            case VertexAttribFormat::Float2: type = bgfx::AttribType::Float; num = 2; break;
            case VertexAttribFormat::Float3: type = bgfx::AttribType::Float; num = 3; break;
            case VertexAttribFormat::Float4: type = bgfx::AttribType::Float; num = 4; break;
            case VertexAttribFormat::Uint8: type = bgfx::AttribType::Uint8; num = 1; break;
            case VertexAttribFormat::Uint8_4: type = bgfx::AttribType::Uint8; num = 4; break;
            case VertexAttribFormat::Int16: type = bgfx::AttribType::Int16; num = 1; break;
            case VertexAttribFormat::Int16_4: type = bgfx::AttribType::Int16; num = 4; break;
            default: type = bgfx::AttribType::Float; num = 3; break;
        }
        
        bgfxLayout.add(attrib, num, type, normalized);
    }
    
    bgfxLayout.end();
    
    bgfx::VertexBufferHandle handle;
    if (desc.data) {
        handle = bgfx::createVertexBuffer(
            bgfx::makeRef(desc.data, desc.size),
            bgfxLayout
        );
    } else {
        handle = bgfx::createVertexBuffer(desc.size / stride_, bgfxLayout);
    }
    
    if (!handle.idx) {
        return false;
    }
    
    handle_ = BufferHandle(handle.idx);
    return true;
}

void VertexBuffer::destroy() {
    if (handle_.valid()) {
        bgfx::destroy(static_cast<bgfx::VertexBufferHandle::Handle>(handle_.index()));
        handle_ = BufferHandle();
    }
    device_ = nullptr;
}

bool VertexBuffer::update(const void* data, uint32_t size, uint32_t offset) {
    if (!handle_.valid() || !data) return false;
    
    // bgfx 不支持直接更新顶点缓冲，需要重新创建或使用动态缓冲
    BX_UNUSED(size, offset);
    return false;
}

// ==================== IndexBuffer 实现 ====================

IndexBuffer::~IndexBuffer() {
    destroy();
}

bool IndexBuffer::create(RenderDevice& device, const BufferDesc& desc) {
    device_ = &device;
    size_ = desc.size;
    is16Bit_ = (desc.type == BufferType::Index16);
    
    bgfx::IndexBufferHandle handle;
    
    if (desc.data) {
        uint16_t flags = is16Bit_ ? BGFX_BUFFER_INDEX16 : 0;
        if (desc.dynamic) {
            flags |= BGFX_BUFFER_DYNAMIC;
        }
        
        handle = bgfx::createIndexBuffer(
            bgfx::makeRef(desc.data, desc.size),
            flags
        );
    } else {
        uint32_t numIndices = is16Bit_ ? desc.size / 2 : desc.size / 4;
        uint16_t flags = is16Bit_ ? BGFX_BUFFER_INDEX16 : 0;
        if (desc.dynamic) {
            flags |= BGFX_BUFFER_DYNAMIC;
        }
        
        handle = bgfx::createIndexBuffer(numIndices, flags);
    }
    
    if (!handle.idx) {
        return false;
    }
    
    handle_ = BufferHandle(handle.idx);
    return true;
}

void IndexBuffer::destroy() {
    if (handle_.valid()) {
        bgfx::destroy(static_cast<bgfx::IndexBufferHandle::Handle>(handle_.index()));
        handle_ = BufferHandle();
    }
    device_ = nullptr;
}

bool IndexBuffer::update(const void* data, uint32_t size, uint32_t offset) {
    BX_UNUSED(data, size, offset);
    return false;
}

// ==================== UniformBufferObject 实现 ====================

UniformBufferObject::~UniformBufferObject() {
    destroy();
}

bool UniformBufferObject::create(RenderDevice& device, uint32_t size, const void* data) {
    device_ = &device;
    size_ = size;
    
    bgfx::UniformHandle handle = bgfx::createUniform(
        "u_params", bgfx::UniformType::Vec4, size / 16
    );
    
    if (!handle.idx) {
        return false;
    }
    
    handle_ = UniformHandle(handle.idx);
    
    if (data) {
        update(data, size);
    }
    
    return true;
}

void UniformBufferObject::destroy() {
    if (handle_.valid()) {
        bgfx::destroy(static_cast<bgfx::UniformHandle::Handle>(handle_.index()));
        handle_ = UniformHandle();
    }
    device_ = nullptr;
}

bool UniformBufferObject::update(const void* data, uint32_t size) {
    if (!handle_.valid() || !data) return false;
    bgfx::setUniform(static_cast<bgfx::UniformHandle::Handle>(handle_.index()), data);
    BX_UNUSED(size);
    return true;
}

// ==================== Texture 实现 ====================

Texture::~Texture() {
    destroy();
}

bool Texture::create(RenderDevice& device, const TextureDesc& desc) {
    device_ = &device;
    width_ = desc.width;
    height_ = desc.height;
    depth_ = desc.depth;
    mipLevels_ = desc.mipLevels;
    format_ = desc.format;
    type_ = desc.type;
    
    uint16_t bgfxFormat = static_cast<uint16_t>(bgfx::TextureFormat::Count);
    
    // 转换格式
    switch (desc.format) {
        case TextureFormat::RGBA8: bgfxFormat = bgfx::TextureFormat::RGBA8; break;
        case TextureFormat::RGBA16F: bgfxFormat = bgfx::TextureFormat::RGBA16F; break;
        case TextureFormat::RGBA32F: bgfxFormat = bgfx::TextureFormat::RGBA32F; break;
        case TextureFormat::RGB10A2: bgfxFormat = bgfx::TextureFormat::RGB10A2; break;
        case TextureFormat::R8: bgfxFormat = bgfx::TextureFormat::R8; break;
        case TextureFormat::R16F: bgfxFormat = bgfx::TextureFormat::R16F; break;
        case TextureFormat::R32F: bgfxFormat = bgfx::TextureFormat::R32F; break;
        case TextureFormat::DepthStencil: bgfxFormat = bgfx::TextureFormat::D24S8; break;
        case TextureFormat::Depth16: bgfxFormat = bgfx::TextureFormat::D16; break;
        case TextureFormat::Depth24: bgfxFormat = bgfx::TextureFormat::D24; break;
        case TextureFormat::Depth32F: bgfxFormat = bgfx::TextureFormat::D32F; break;
        default: bgfxFormat = bgfx::TextureFormat::RGBA8; break;
    }
    
    uint16_t flags = 0;
    if (desc.renderTarget) flags |= BGFX_TEXTURE_RT;
    if (desc.depthStencil) flags |= BGFX_TEXTURE_RT | BGFX_TEXTURE_MSAA_SAMPLE_4;
    if (desc.generateMips) flags |= BGFX_TEXTURE_COMPUTE_WRITE;
    
    bgfx::TextureHandle handle;
    
    switch (desc.type) {
        case TextureType::Texture2D:
            handle = bgfx::createTexture2D(
                static_cast<uint16_t>(desc.width),
                static_cast<uint16_t>(desc.height),
                desc.mipLevels > 1,
                desc.arraySize,
                static_cast<bgfx::TextureFormat::Enum>(bgfxFormat),
                flags
            );
            break;
            
        case TextureType::Texture3D:
            handle = bgfx::createTexture3D(
                static_cast<uint16_t>(desc.width),
                static_cast<uint16_t>(desc.height),
                static_cast<uint16_t>(desc.depth),
                desc.mipLevels > 1,
                static_cast<bgfx::TextureFormat::Enum>(bgfxFormat),
                flags
            );
            break;
            
        case TextureType::TextureCube:
            handle = bgfx::createTextureCube(
                static_cast<uint16_t>(desc.width),
                desc.mipLevels > 1,
                desc.arraySize,
                static_cast<bgfx::TextureFormat::Enum>(bgfxFormat),
                flags
            );
            break;
            
        default:
            return false;
    }
    
    if (!handle.idx) {
        return false;
    }
    
    handle_ = TextureHandle(handle.idx);
    
    // 上传初始数据
    if (desc.data && desc.dataSize > 0) {
        update(desc.data);
    }
    
    return true;
}

bool Texture::loadFromFile(RenderDevice& device, const std::filesystem::path& path,
                            const TextureLoadOptions& options) {
    // 使用 stb_image 加载
    // 实际实现需要包含 stb_image.h
    
    FILE* file = fopen(path.string().c_str(), "rb");
    if (!file) {
        return false;
    }
    fclose(file);
    
    // 简化实现：返回 false，需要集成 stb_image
    BX_UNUSED(device, options);
    return false;
}

bool Texture::loadFromMemory(RenderDevice& device, const void* data, size_t size,
                              const char* extension, const TextureLoadOptions& options) {
    BX_UNUSED(device, data, size, extension, options);
    return false;
}

void Texture::destroy() {
    if (handle_.valid()) {
        bgfx::destroy(static_cast<bgfx::TextureHandle::Handle>(handle_.index()));
        handle_ = TextureHandle();
    }
    device_ = nullptr;
}

bool Texture::update(const void* data, uint32_t mipLevel, uint32_t arraySlice) {
    if (!handle_.valid() || !data) return false;
    
    bgfx::TextureHandle handle = static_cast<bgfx::TextureHandle::Handle>(handle_.index());
    
    switch (type_) {
        case TextureType::Texture2D:
            bgfx::updateTexture2D(handle, 0, 0, 
                static_cast<uint16_t>(mipLevel), 0, 0,
                static_cast<uint16_t>(width_ >> mipLevel),
                static_cast<uint16_t>(height_ >> mipLevel),
                1, 1,
                bgfx::makeRef(data, width_ * height_ * 4),
                0);
            break;
        default:
            return false;
    }
    
    BX_UNUSED(arraySlice);
    return true;
}

// ==================== CubeTexture 实现 ====================

CubeTexture::~CubeTexture() {
    destroy();
}

bool CubeTexture::create(RenderDevice& device, uint32_t size, TextureFormat format,
                          const std::array<const void*, 6>& faces) {
    device_ = &device;
    size_ = size;
    
    uint16_t bgfxFormat = bgfx::TextureFormat::RGBA8;
    switch (format) {
        case TextureFormat::RGBA8: bgfxFormat = bgfx::TextureFormat::RGBA8; break;
        case TextureFormat::RGBA16F: bgfxFormat = bgfx::TextureFormat::RGBA16F; break;
        default: break;
    }
    
    bgfx::TextureHandle handle = bgfx::createTextureCube(
        static_cast<uint16_t>(size),
        false, 1,
        static_cast<bgfx::TextureFormat::Enum>(bgfxFormat)
    );
    
    if (!handle.idx) {
        return false;
    }
    
    handle_ = TextureHandle(handle.idx);
    
    // 上传六个面
    for (int i = 0; i < 6; ++i) {
        if (faces[i]) {
            bgfx::updateTextureCube(handle, 0, 0, 0, 0,
                static_cast<uint16_t>(size), static_cast<uint16_t>(size),
                1, 1,
                bgfx::makeRef(faces[i], size * size * 4));
        }
    }
    
    return true;
}

bool CubeTexture::loadFromFile(RenderDevice& device, const std::filesystem::path& path,
                                const TextureLoadOptions& options) {
    BX_UNUSED(device, path, options);
    return false;
}

void CubeTexture::destroy() {
    if (handle_.valid()) {
        bgfx::destroy(static_cast<bgfx::TextureHandle::Handle>(handle_.index()));
        handle_ = TextureHandle();
    }
    device_ = nullptr;
}

// ==================== FrameBuffer 实现 ====================

FrameBuffer::~FrameBuffer() {
    destroy();
}

bool FrameBuffer::create(RenderDevice& device, const FrameBufferDesc& desc) {
    device_ = &device;
    width_ = desc.width;
    height_ = desc.height;
    colorAttachments_ = desc.colorAttachments;
    depthAttachment_ = desc.depthAttachment;
    ownAttachments_ = desc.ownAttachments;
    
    std::vector<bgfx::TextureHandle> bgfxTextures;
    for (const auto& tex : desc.colorAttachments) {
        if (tex.valid()) {
            bgfxTextures.push_back(static_cast<bgfx::TextureHandle::Handle>(tex.index()));
        }
    }
    
    bgfx::FrameBufferHandle handle;
    
    if (!bgfxTextures.empty()) {
        handle = bgfx::createFrameBuffer(
            static_cast<uint8_t>(bgfxTextures.size()),
            bgfxTextures.data(),
            desc.ownAttachments
        );
    } else if (desc.depthAttachment.valid()) {
        bgfx::TextureHandle depthHandle = 
            static_cast<bgfx::TextureHandle::Handle>(desc.depthAttachment.index());
        handle = bgfx::createFrameBuffer(1, &depthHandle, desc.ownAttachments);
    }
    
    if (!handle.idx) {
        return false;
    }
    
    handle_ = FrameBufferHandle(handle.idx);
    return true;
}

bool FrameBuffer::createFromTargets(RenderDevice& device, 
                                     const std::vector<RenderTarget*>& colors,
                                     RenderTarget* depth) {
    FrameBufferDesc desc;
    desc.width = colors.empty() ? depth->getWidth() : colors[0]->getWidth();
    desc.height = colors.empty() ? depth->getHeight() : colors[0]->getHeight();
    
    for (auto* rt : colors) {
        if (rt && rt->isValid()) {
            desc.colorAttachments.push_back(rt->getTexture());
        }
    }
    
    if (depth && depth->isValid()) {
        desc.depthAttachment = depth->getTexture();
    }
    
    return create(device, desc);
}

void FrameBuffer::destroy() {
    if (handle_.valid()) {
        bgfx::destroy(static_cast<bgfx::FrameBufferHandle::Handle>(handle_.index()));
        handle_ = FrameBufferHandle();
    }
    
    if (ownAttachments_) {
        for (auto& tex : colorAttachments_) {
            if (tex.valid()) {
                bgfx::destroy(static_cast<bgfx::TextureHandle::Handle>(tex.index()));
            }
        }
        if (depthAttachment_.valid()) {
            bgfx::destroy(static_cast<bgfx::TextureHandle::Handle>(depthAttachment_.index()));
        }
    }
    
    colorAttachments_.clear();
    depthAttachment_ = TextureHandle();
    device_ = nullptr;
}

// ==================== ResourceManager 实现 ====================

ResourceManager::ResourceManager() {}

ResourceManager::~ResourceManager() {
    shutdown();
}

void ResourceManager::initialize(RenderDevice& device) {
    device_ = &device;
}

void ResourceManager::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    vertexBuffers_.clear();
    indexBuffers_.clear();
    textures_.clear();
    frameBuffers_.clear();
    namedTextures_.clear();
    
    device_ = nullptr;
}

VertexBuffer* ResourceManager::createVertexBuffer(const BufferDesc& desc, const VertexLayout& layout) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto buffer = std::make_unique<VertexBuffer>();
    if (!buffer->create(*device_, desc, layout)) {
        return nullptr;
    }
    
    VertexBuffer* ptr = buffer.get();
    vertexBuffers_.push_back(std::move(buffer));
    return ptr;
}

IndexBuffer* ResourceManager::createIndexBuffer(const BufferDesc& desc) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto buffer = std::make_unique<IndexBuffer>();
    if (!buffer->create(*device_, desc)) {
        return nullptr;
    }
    
    IndexBuffer* ptr = buffer.get();
    indexBuffers_.push_back(std::move(buffer));
    return ptr;
}

Texture* ResourceManager::createTexture(const TextureDesc& desc) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto texture = std::make_unique<Texture>();
    if (!texture->create(*device_, desc)) {
        return nullptr;
    }
    
    Texture* ptr = texture.get();
    textures_.push_back(std::move(texture));
    return ptr;
}

Texture* ResourceManager::loadTexture(const std::filesystem::path& path,
                                       const TextureLoadOptions& options) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto texture = std::make_unique<Texture>();
    if (!texture->loadFromFile(*device_, path, options)) {
        return nullptr;
    }
    
    Texture* ptr = texture.get();
    textures_.push_back(std::move(texture));
    return ptr;
}

FrameBuffer* ResourceManager::createFrameBuffer(const FrameBufferDesc& desc) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto fb = std::make_unique<FrameBuffer>();
    if (!fb->create(*device_, desc)) {
        return nullptr;
    }
    
    FrameBuffer* ptr = fb.get();
    frameBuffers_.push_back(std::move(fb));
    return ptr;
}

void ResourceManager::release(VertexBuffer* buffer) {
    if (!buffer) return;
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = std::find_if(vertexBuffers_.begin(), vertexBuffers_.end(),
        [buffer](const std::unique_ptr<VertexBuffer>& ptr) {
            return ptr.get() == buffer;
        });
    
    if (it != vertexBuffers_.end()) {
        vertexBuffers_.erase(it);
    }
}

void ResourceManager::release(IndexBuffer* buffer) {
    if (!buffer) return;
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = std::find_if(indexBuffers_.begin(), indexBuffers_.end(),
        [buffer](const std::unique_ptr<IndexBuffer>& ptr) {
            return ptr.get() == buffer;
        });
    
    if (it != indexBuffers_.end()) {
        indexBuffers_.erase(it);
    }
}

void ResourceManager::release(Texture* texture) {
    if (!texture) return;
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = std::find_if(textures_.begin(), textures_.end(),
        [texture](const std::unique_ptr<Texture>& ptr) {
            return ptr.get() == texture;
        });
    
    if (it != textures_.end()) {
        textures_.erase(it);
    }
    
    // 从命名表移除
    for (auto it2 = namedTextures_.begin(); it2 != namedTextures_.end(); ++it2) {
        if (it2->second == texture) {
            namedTextures_.erase(it2);
            break;
        }
    }
}

void ResourceManager::release(FrameBuffer* buffer) {
    if (!buffer) return;
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = std::find_if(frameBuffers_.begin(), frameBuffers_.end(),
        [buffer](const std::unique_ptr<FrameBuffer>& ptr) {
            return ptr.get() == buffer;
        });
    
    if (it != frameBuffers_.end()) {
        frameBuffers_.erase(it);
    }
}

Texture* ResourceManager::findTexture(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = namedTextures_.find(name);
    return it != namedTextures_.end() ? it->second : nullptr;
}

void ResourceManager::registerTexture(const std::string& name, Texture* texture) {
    std::lock_guard<std::mutex> lock(mutex_);
    namedTextures_[name] = texture;
}

void ResourceManager::unloadAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    vertexBuffers_.clear();
    indexBuffers_.clear();
    textures_.clear();
    frameBuffers_.clear();
    namedTextures_.clear();
}

ResourceManager::Stats ResourceManager::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    Stats stats = {};
    stats.vertexBufferCount = static_cast<uint32_t>(vertexBuffers_.size());
    stats.indexBufferCount = static_cast<uint32_t>(indexBuffers_.size());
    stats.textureCount = static_cast<uint32_t>(textures_.size());
    stats.frameBufferCount = static_cast<uint32_t>(frameBuffers_.size());
    stats.gpuMemoryUsed = 0; // 需要从 bgfx 获取
    
    return stats;
}

// ==================== Mesh 实现 ====================

Mesh::~Mesh() {
    destroy();
}

bool Mesh::create(RenderDevice& device, const MeshData& data) {
    device_ = &device;
    vertexCount_ = data.vertexCount();
    indexCount_ = data.indexCount();
    
    // 计算包围盒
    if (!data.positions.empty()) {
        boundsMin_[0] = boundsMax_[0] = data.positions[0];
        boundsMin_[1] = boundsMax_[1] = data.positions[1];
        boundsMin_[2] = boundsMax_[2] = data.positions[2];
        
        for (size_t i = 3; i < data.positions.size(); i += 3) {
            boundsMin_[0] = std::min(boundsMin_[0], data.positions[i]);
            boundsMax_[0] = std::max(boundsMax_[0], data.positions[i]);
            boundsMin_[1] = std::min(boundsMin_[1], data.positions[i+1]);
            boundsMax_[1] = std::max(boundsMax_[1], data.positions[i+1]);
            boundsMin_[2] = std::min(boundsMin_[2], data.positions[i+2]);
            boundsMax_[2] = std::max(boundsMax_[2], data.positions[i+2]);
        }
    }
    
    // 创建顶点布局
    layout_.begin()
        .addPosition()
        .addNormal()
        .addTexCoord()
        .end();
    
    // 创建顶点数据
    std::vector<float> vertices;
    vertices.reserve(vertexCount_ * 8); // position(3) + normal(3) + texcoord(2)
    
    for (uint32_t i = 0; i < vertexCount_; ++i) {
        // Position
        if (i * 3 < data.positions.size()) {
            vertices.push_back(data.positions[i * 3 + 0]);
            vertices.push_back(data.positions[i * 3 + 1]);
            vertices.push_back(data.positions[i * 3 + 2]);
        } else {
            vertices.push_back(0); vertices.push_back(0); vertices.push_back(0);
        }
        
        // Normal
        if (i * 3 < data.normals.size()) {
            vertices.push_back(data.normals[i * 3 + 0]);
            vertices.push_back(data.normals[i * 3 + 1]);
            vertices.push_back(data.normals[i * 3 + 2]);
        } else {
            vertices.push_back(0); vertices.push_back(1); vertices.push_back(0);
        }
        
        // TexCoord
        if (i * 2 < data.texCoords.size()) {
            vertices.push_back(data.texCoords[i * 2 + 0]);
            vertices.push_back(data.texCoords[i * 2 + 1]);
        } else {
            vertices.push_back(0); vertices.push_back(0);
        }
    }
    
    // 创建顶点缓冲
    BufferDesc vbDesc;
    vbDesc.type = BufferType::Vertex;
    vbDesc.size = static_cast<uint32_t>(vertices.size() * sizeof(float));
    vbDesc.data = vertices.data();
    vbDesc.stride = layout_.getStride();
    
    VertexBuffer vb;
    if (!vb.create(device, vbDesc, layout_)) {
        return false;
    }
    vertexBuffer_ = vb.getHandle();
    
    // 创建索引缓冲
    if (!data.indices.empty()) {
        BufferDesc ibDesc;
        ibDesc.type = BufferType::Index32;
        ibDesc.size = static_cast<uint32_t>(data.indices.size() * sizeof(uint32_t));
        ibDesc.data = data.indices.data();
        
        IndexBuffer ib;
        if (!ib.create(device, ibDesc)) {
            return false;
        }
        indexBuffer_ = ib.getHandle();
    }
    
    return true;
}

bool Mesh::loadFromFile(RenderDevice& device, const std::filesystem::path& path) {
    BX_UNUSED(device, path);
    // 需要集成 assimp 或其他模型加载库
    return false;
}

void Mesh::destroy() {
    if (vertexBuffer_.valid()) {
        bgfx::destroy(static_cast<bgfx::VertexBufferHandle::Handle>(vertexBuffer_.index()));
        vertexBuffer_ = BufferHandle();
    }
    if (indexBuffer_.valid()) {
        bgfx::destroy(static_cast<bgfx::IndexBufferHandle::Handle>(indexBuffer_.index()));
        indexBuffer_ = BufferHandle();
    }
    device_ = nullptr;
}

void Mesh::getBoundingBox(float min[3], float max[3]) const {
    std::memcpy(min, boundsMin_, 3 * sizeof(float));
    std::memcpy(max, boundsMax_, 3 * sizeof(float));
}

// ==================== BuiltinMeshes 实现 ====================

MeshData BuiltinMeshes::createTriangle() {
    MeshData data;
    
    // 简单的三角形
    data.positions = {
        0.0f,  0.5f, 0.0f,
       -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f
    };
    
    data.normals = {
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f
    };
    
    data.texCoords = {
        0.5f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f
    };
    
    data.indices = {0, 1, 2};
    
    return data;
}

MeshData BuiltinMeshes::createQuad() {
    MeshData data;
    
    data.positions = {
       -0.5f,  0.5f, 0.0f,
        0.5f,  0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
       -0.5f, -0.5f, 0.0f
    };
    
    data.normals = {
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f
    };
    
    data.texCoords = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f
    };
    
    data.indices = {
        0, 1, 2,
        0, 2, 3
    };
    
    return data;
}

MeshData BuiltinMeshes::createCube(float size) {
    MeshData data;
    float s = size * 0.5f;
    
    // 8 个顶点
    data.positions = {
        -s, -s, -s,   s, -s, -s,   s,  s, -s,  -s,  s, -s,  // 背面
        -s, -s,  s,   s, -s,  s,   s,  s,  s,  -s,  s,  s   // 正面
    };
    
    data.normals = {
        0, 0, -1,   0, 0, -1,   0, 0, -1,   0, 0, -1,
        0, 0,  1,   0, 0,  1,   0, 0,  1,   0, 0,  1
    };
    
    data.texCoords = {
        0, 0,   1, 0,   1, 1,   0, 1,
        0, 0,   1, 0,   1, 1,   0, 1
    };
    
    // 36 个索引 (6 个面，每个面 2 个三角形)
    data.indices = {
        // 背面
        0, 1, 2,   0, 2, 3,
        // 正面
        4, 6, 5,   4, 7, 6,
        // 上面
        3, 2, 6,   3, 6, 7,
        // 下面
        0, 5, 1,   0, 4, 5,
        // 右面
        1, 5, 6,   1, 6, 2,
        // 左面
        0, 3, 7,   0, 7, 4
    };
    
    return data;
}

MeshData BuiltinMeshes::createSphere(float radius, uint32_t subdivisions) {
    MeshData data;
    
    uint32_t rings = subdivisions;
    uint32_t sectors = subdivisions * 2;
    
    for (uint32_t r = 0; r <= rings; ++r) {
        float v = static_cast<float>(r) / rings;
        float phi = v * 3.14159265f;
        
        for (uint32_t s = 0; s <= sectors; ++s) {
            float u = static_cast<float>(s) / sectors;
            float theta = u * 2.0f * 3.14159265f;
            
            float x = std::cos(theta) * std::sin(phi);
            float y = std::cos(phi);
            float z = std::sin(theta) * std::sin(phi);
            
            data.positions.push_back(x * radius);
            data.positions.push_back(y * radius);
            data.positions.push_back(z * radius);
            
            data.normals.push_back(x);
            data.normals.push_back(y);
            data.normals.push_back(z);
            
            data.texCoords.push_back(u);
            data.texCoords.push_back(v);
        }
    }
    
    // 生成索引
    for (uint32_t r = 0; r < rings; ++r) {
        for (uint32_t s = 0; s < sectors; ++s) {
            uint32_t current = r * (sectors + 1) + s;
            uint32_t next = current + sectors + 1;
            
            data.indices.push_back(current);
            data.indices.push_back(next);
            data.indices.push_back(current + 1);
            
            data.indices.push_back(next);
            data.indices.push_back(next + 1);
            data.indices.push_back(current + 1);
        }
    }
    
    return data;
}

MeshData BuiltinMeshes::createCylinder(float radius, float height, uint32_t segments) {
    MeshData data;
    float halfHeight = height * 0.5f;
    
    // 顶部和底部中心
    data.positions.push_back(0); data.positions.push_back(halfHeight); data.positions.push_back(0);
    data.normals.push_back(0); data.normals.push_back(1); data.normals.push_back(0);
    data.texCoords.push_back(0.5f); data.texCoords.push_back(0.5f);
    
    data.positions.push_back(0); data.positions.push_back(-halfHeight); data.positions.push_back(0);
    data.normals.push_back(0); data.normals.push_back(-1); data.normals.push_back(0);
    data.texCoords.push_back(0.5f); data.texCoords.push_back(0.5f);
    
    // 侧面顶点
    for (uint32_t i = 0; i <= segments; ++i) {
        float angle = 2.0f * 3.14159265f * i / segments;
        float x = std::cos(angle) * radius;
        float z = std::sin(angle) * radius;
        
        // 顶部边缘
        data.positions.push_back(x);
        data.positions.push_back(halfHeight);
        data.positions.push_back(z);
        data.normals.push_back(x / radius);
        data.normals.push_back(0);
        data.normals.push_back(z / radius);
        data.texCoords.push_back(static_cast<float>(i) / segments);
        data.texCoords.push_back(0);
        
        // 底部边缘
        data.positions.push_back(x);
        data.positions.push_back(-halfHeight);
        data.positions.push_back(z);
        data.normals.push_back(x / radius);
        data.normals.push_back(0);
        data.normals.push_back(z / radius);
        data.texCoords.push_back(static_cast<float>(i) / segments);
        data.texCoords.push_back(1);
    }
    
    // 顶部盖
    for (uint32_t i = 0; i < segments; ++i) {
        data.indices.push_back(0);
        data.indices.push_back(2 + i * 2);
        data.indices.push_back(2 + i * 2 + 2);
    }
    
    // 底部盖
    for (uint32_t i = 0; i < segments; ++i) {
        data.indices.push_back(1);
        data.indices.push_back(3 + i * 2 + 2);
        data.indices.push_back(3 + i * 2);
    }
    
    // 侧面
    for (uint32_t i = 0; i < segments; ++i) {
        uint32_t base = 2 + i * 2;
        data.indices.push_back(base);
        data.indices.push_back(base + 2);
        data.indices.push_back(base + 1);
        
        data.indices.push_back(base + 2);
        data.indices.push_back(base + 3);
        data.indices.push_back(base + 1);
    }
    
    return data;
}

MeshData BuiltinMeshes::createPlane(float width, float height, uint32_t subdivisions) {
    MeshData data;
    
    uint32_t vertsPerRow = subdivisions + 1;
    float halfW = width * 0.5f;
    float halfH = height * 0.5f;
    
    for (uint32_t y = 0; y <= subdivisions; ++y) {
        float v = static_cast<float>(y) / subdivisions;
        float py = halfH - v * height;
        
        for (uint32_t x = 0; x <= subdivisions; ++x) {
            float u = static_cast<float>(x) / subdivisions;
            float px = -halfW + u * width;
            
            data.positions.push_back(px);
            data.positions.push_back(0);
            data.positions.push_back(py);
            
            data.normals.push_back(0);
            data.normals.push_back(1);
            data.normals.push_back(0);
            
            data.texCoords.push_back(u);
            data.texCoords.push_back(v);
        }
    }
    
    for (uint32_t y = 0; y < subdivisions; ++y) {
        for (uint32_t x = 0; x < subdivisions; ++x) {
            uint32_t base = y * vertsPerRow + x;
            
            data.indices.push_back(base);
            data.indices.push_back(base + vertsPerRow);
            data.indices.push_back(base + 1);
            
            data.indices.push_back(base + vertsPerRow);
            data.indices.push_back(base + vertsPerRow + 1);
            data.indices.push_back(base + 1);
        }
    }
    
    return data;
}

} // namespace render
} // namespace phoenix
