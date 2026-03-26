#include "phoenix/render/RenderDevice.hpp"
#include <cstring>
#include <cstdio>
#include <cstdarg>

// bgfx 包含
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

namespace phoenix {
namespace render {

// ==================== 内部实现 ====================

struct RenderDevice::Impl {
    bgfx::Init bgfxInit;
    bool initialized = false;
};

// ==================== RenderDevice 实现 ====================

RenderDevice::RenderDevice() : impl_(std::make_unique<Impl>()) {
    impl_->bgfxInit.type = bgfx::RendererType::Count;
    impl_->bgfxInit.vendorId = BGFX_PCI_ID_NONE;
    impl_->bgfxInit.deviceId = 0;
    impl_->bgfxInit.callback = nullptr;
    impl_->bgfxInit.allocator = nullptr;
}

RenderDevice::~RenderDevice() {
    shutdown();
}

RenderDevice::RenderDevice(RenderDevice&& other) noexcept 
    : impl_(std::move(other.impl_))
    , initialized_(other.initialized_.load())
    , frameNumber_(other.frameNumber_.load())
    , deviceInfo_(other.deviceInfo_)
    , mainSwapChain_(other.mainSwapChain_) 
{
    other.initialized_ = false;
    other.mainSwapChain_ = DeviceHandle();
}

RenderDevice& RenderDevice::operator=(RenderDevice&& other) noexcept {
    if (this != &other) {
        shutdown();
        impl_ = std::move(other.impl_);
        initialized_ = other.initialized_.load();
        frameNumber_ = other.frameNumber_.load();
        deviceInfo_ = other.deviceInfo_;
        mainSwapChain_ = other.mainSwapChain_;
        other.initialized_ = false;
        other.mainSwapChain_ = DeviceHandle();
    }
    return *this;
}

void RenderDevice::translateInit(bgfx::Init& init, const DeviceConfig& config) {
    // 转换后端类型
    switch (config.backend) {
        case RenderBackend::Vulkan:
            init.type = bgfx::RendererType::Vulkan;
            deviceInfo_.backend = RenderBackend::Vulkan;
            deviceInfo_.backendName = "Vulkan";
            break;
        case RenderBackend::DirectX11:
            init.type = bgfx::RendererType::Direct3D11;
            deviceInfo_.backend = RenderBackend::DirectX11;
            deviceInfo_.backendName = "Direct3D 11";
            break;
        case RenderBackend::DirectX12:
            init.type = bgfx::RendererType::Direct3D12;
            deviceInfo_.backend = RenderBackend::DirectX12;
            deviceInfo_.backendName = "Direct3D 12";
            break;
        case RenderBackend::Metal:
            init.type = bgfx::RendererType::Metal;
            deviceInfo_.backend = RenderBackend::Metal;
            deviceInfo_.backendName = "Metal";
            break;
        case RenderBackend::OpenGL:
            init.type = bgfx::RendererType::OpenGL;
            deviceInfo_.backend = RenderBackend::OpenGL;
            deviceInfo_.backendName = "OpenGL";
            break;
        case RenderBackend::OpenGL_ES:
            init.type = bgfx::RendererType::OpenGLES;
            deviceInfo_.backend = RenderBackend::OpenGL_ES;
            deviceInfo_.backendName = "OpenGL ES";
            break;
        case RenderBackend::WebGL:
            init.type = bgfx::RendererType::WebGL;
            deviceInfo_.backend = RenderBackend::WebGL;
            deviceInfo_.backendName = "WebGL";
            break;
        case RenderBackend::WebGL2:
            init.type = bgfx::RendererType::WebGL2;
            deviceInfo_.backend = RenderBackend::WebGL2;
            deviceInfo_.backendName = "WebGL 2";
            break;
        case RenderBackend::Null:
            init.type = bgfx::RendererType::Noop;
            deviceInfo_.backend = RenderBackend::Null;
            deviceInfo_.backendName = "Null";
            break;
        default:
            init.type = bgfx::RendererType::Vulkan;
            deviceInfo_.backend = RenderBackend::Vulkan;
            deviceInfo_.backendName = "Vulkan";
            break;
    }

    init.resolution.width = BGFX_RESET_NONE;
    init.resolution.height = BGFX_RESET_NONE;
    init.resolution.format = bgfx::TextureFormat::Count;
    init.resolution.sampleCount = 1;
    
    init.debug = config.enableDebugInfo;
    init.profile = config.enableDebugInfo;
    
    init.maxFrameLatency = config.maxFrameLatency;
    
    // 回调
    if (config.logCallback) {
        struct Callback : public bgfx::CallbackI {
            DeviceConfig::LogCallback callback;
            void fatal(const char* _filePath, uint16_t _line, bgfx::Fatal::Enum _code, 
                      const char* _str) override {
                if (callback) {
                    char buffer[1024];
                    snprintf(buffer, sizeof(buffer), "FATAL [%s:%d] %s", _filePath, _line, _str);
                    callback(buffer);
                }
                BX_UNUSED(_code);
            }
            void traceVargs(const char* _filePath, uint16_t _line, const char* _format, 
                           va_list _argList) override {
                if (callback) {
                    char buffer[1024];
                    vsnprintf(buffer, sizeof(buffer), _format, _argList);
                    callback(buffer);
                }
            }
            void profiler(const char* _name, uint64_t _abegin, uint64_t _aend, 
                         uint64_t _begin, uint64_t _end) override {
                BX_UNUSED(_name, _abegin, _aend, _begin, _end);
            }
            uint32_t cacheReadSize(void* _handle, void* _data, uint32_t _size) override {
                BX_UNUSED(_handle, _data, _size);
                return 0;
            }
            void cacheWrite(void* _handle, const void* _data, uint32_t _size) override {
                BX_UNUSED(_handle, _data, _size);
            }
            void screenShot(const char* _filePath, uint32_t _width, uint32_t _height, 
                           uint32_t _pitch, const void* _data, uint32_t _size, 
                           bool _yflip) override {
                BX_UNUSED(_filePath, _width, _height, _pitch, _data, _size, _yflip);
            }
            void captureBegin(uint32_t _width, uint32_t _height, uint32_t _pitch, 
                             bgfx::TextureFormat::Enum _format, bool _yflip) override {
                BX_UNUSED(_width, _height, _pitch, _format, _yflip);
            }
            void captureEnd() override {}
            void captureFrame(const void* _data, uint32_t _size) override {
                BX_UNUSED(_data, _size);
            }
        };
        // 注意：实际使用中需要管理回调对象的生命周期
    }
}

void RenderDevice::initDeviceInfo() {
    const bgfx::Caps* caps = bgfx::getCaps();
    
    deviceInfo_.vendor = caps->vendorName;
    deviceInfo_.device = caps->deviceName;
    deviceInfo_.driver = caps->driverName;
    
    deviceInfo_.maxTextureSize2D = caps->limits.maxTextureSize;
    deviceInfo_.maxTextureSizeCube = caps->limits.maxTextureSize;
    deviceInfo_.maxTextureSize3D = caps->limits.maxTextureDimension;
    deviceInfo_.maxTextureLayers = caps->limits.maxTextureLayers;
    deviceInfo_.maxVertexStreams = caps->limits.maxVertexStreams;
    deviceInfo_.maxUniforms = caps->limits.maxUniforms;
    deviceInfo_.maxShaderSamplers = caps->limits.maxShaderSamplers;
    deviceInfo_.maxComputeBindings = caps->limits.maxComputeBindings;
    
    deviceInfo_.supportsCompute = (caps->supported & BGFX_CAPS_COMPUTE) != 0;
    deviceInfo_.supportsGeometryShaders = (caps->supported & BGFX_CAPS_GEOMETRY_SHADER) != 0;
    deviceInfo_.supportsTessellation = (caps->supported & BGFX_CAPS_TESSELLATION) != 0;
    deviceInfo_.supportsInstancing = (caps->supported & BGFX_CAPS_INSTANCING) != 0;
    deviceInfo_.supportsConservativeRaster = (caps->supported & BGFX_CAPS_CONSERVATIVE_RASTER) != 0;
    deviceInfo_.supports3DTextures = (caps->supported & BGFX_CAPS_TEXTURE_3D) != 0;
    deviceInfo_.supportsCubeArrayTextures = (caps->supported & BGFX_CAPS_TEXTURE_CUBE_ARRAY) != 0;
    deviceInfo_.supportsOcclusionQuery = (caps->supported & BGFX_CAPS_OCCLUSION_QUERY) != 0;
    deviceInfo_.supportsTimestampQuery = (caps->supported & BGFX_CAPS_TIMESTAMP_QUERY) != 0;
}

bool RenderDevice::initialize(const DeviceConfig& config, const SwapChainConfig& swapChain) {
    if (initialized_) {
        return false;
    }

    translateInit(impl_->bgfxInit, config);
    
    // 设置交换链
    impl_->bgfxInit.platformData.nwh = swapChain.windowHandle;
    
    if (!bgfx::init(impl_->bgfxInit)) {
        return false;
    }
    
    initDeviceInfo();
    
    // 设置调试名称
    bgfx::setDebug(BGFX_DEBUG_TEXT);
    
    // 设置视图矩形
    bgfx::setViewRect(0, 0, 0, swapChain.width, swapChain.height);
    
    mainSwapChain_ = DeviceHandle(0); // bgfx 默认交换链
    initialized_ = true;
    
    return true;
}

void RenderDevice::shutdown() {
    if (initialized_) {
        bgfx::shutdown();
        initialized_ = false;
        mainSwapChain_ = DeviceHandle();
    }
}

DeviceHandle RenderDevice::createSwapChain(const SwapChainConfig& config) {
    if (!initialized_) {
        return DeviceHandle();
    }
    
    bgfx::PlatformData pd;
    pd.nwh = config.windowHandle;
    bgfx::setPlatformData(pd);
    
    // bgfx 自动处理交换链创建
    return DeviceHandle(0);
}

void RenderDevice::destroySwapChain(DeviceHandle handle) {
    // bgfx 自动管理交换链
    BX_UNUSED(handle);
}

void RenderDevice::resizeSwapChain(DeviceHandle handle, uint32_t width, uint32_t height) {
    BX_UNUSED(handle);
    bgfx::reset(width, height, BGFX_RESET_VSYNC);
}

void RenderDevice::beginFrame(uint32_t viewId, const Viewport& viewport) {
    if (!initialized_) return;
    
    bgfx::touch(viewId);
    
    if (viewport.width > 0 && viewport.height > 0) {
        bgfx::setViewRect(viewId, viewport.x, viewport.y, viewport.width, viewport.height);
        bgfx::setViewViewport(viewId, viewport.x, viewport.y, viewport.width, viewport.height);
    }
    
    frameNumber_++;
}

FrameStats RenderDevice::endFrame(bool vsync) {
    FrameStats stats = {};
    stats.frameNumber = frameNumber_;
    
    const bgfx::Stats* bgfxStats = bgfx::getStats();
    stats.frameTime = static_cast<float>(bgfxStats->cpuTimeFrame) / 1000000.0f;
    stats.gpuTime = static_cast<float>(bgfxStats->gpuTimeFrame) / 1000000.0f;
    stats.drawCalls = bgfxStats->numDrawCalls;
    stats.triangleCount = bgfxStats->numPrimitives;
    stats.vertexCount = bgfxStats->numVertices;
    stats.textureBindings = bgfxStats->numTextureBindings;
    stats.uniformBindings = bgfxStats->numUniformBindings;
    stats.transientMemoryUsed = static_cast<uint32_t>(bgfxStats->transientMemoryUsed);
    stats.persistentMemoryUsed = static_cast<uint32_t>(bgfxStats->persistentMemoryUsed);
    
    bgfx::frame(vsync);
    
    return stats;
}

void RenderDevice::clear(uint32_t viewId, ClearFlags flags, const Color& color,
                         float depth, uint8_t stencil) {
    uint16_t clearFlags = 0;
    
    if (flags & ClearFlags::Color) {
        clearFlags |= BGFX_CLEAR_COLOR;
    }
    if (flags & ClearFlags::Depth) {
        clearFlags |= BGFX_CLEAR_DEPTH;
    }
    if (flags & ClearFlags::Stencil) {
        clearFlags |= BGFX_CLEAR_STENCIL;
    }
    
    uint32_t rgba = 0;
    rgba |= static_cast<uint32_t>(color.r * 255.0f) << 0;
    rgba |= static_cast<uint32_t>(color.g * 255.0f) << 8;
    rgba |= static_cast<uint32_t>(color.b * 255.0f) << 16;
    rgba |= static_cast<uint32_t>(color.a * 255.0f) << 24;
    
    bgfx::clear(viewId, clearFlags, rgba, depth, stencil);
}

void RenderDevice::setViewRect(uint32_t viewId, const Rect& rect) {
    bgfx::setViewRect(viewId, rect.x, rect.y, rect.width, rect.height);
}

void RenderDevice::setViewViewport(uint32_t viewId, const Viewport& viewport) {
    bgfx::setViewViewport(viewId, viewport.x, viewport.y, viewport.width, viewport.height);
}

void RenderDevice::setViewFrameBuffer(uint32_t viewId, FrameBufferHandle fb) {
    if (fb.valid()) {
        bgfx::setViewFrameBuffer(viewId, static_cast<bgfx::FrameBufferHandle::Handle>(fb.index()));
    }
}

void RenderDevice::setViewClear(uint32_t viewId, ClearFlags flags, const Color& color,
                                float depth, uint8_t stencil) {
    uint16_t clearFlags = 0;
    
    if (flags & ClearFlags::Color) {
        clearFlags |= BGFX_CLEAR_COLOR;
    }
    if (flags & ClearFlags::Depth) {
        clearFlags |= BGFX_CLEAR_DEPTH;
    }
    if (flags & ClearFlags::Stencil) {
        clearFlags |= BGFX_CLEAR_STENCIL;
    }
    
    uint32_t rgba = 0;
    rgba |= static_cast<uint32_t>(color.r * 255.0f) << 0;
    rgba |= static_cast<uint32_t>(color.g * 255.0f) << 8;
    rgba |= static_cast<uint32_t>(color.b * 255.0f) << 16;
    rgba |= static_cast<uint32_t>(color.a * 255.0f) << 24;
    
    bgfx::setViewClear(viewId, clearFlags, rgba, depth, stencil);
}

void RenderDevice::submit(uint32_t viewId, ProgramHandle program, SubmitFlags flags) {
    if (!program.valid()) return;
    
    uint16_t bgfxFlags = 0;
    if (flags & SubmitFlags::Stereo) bgfxFlags |= BGFX_SUBMIT_STEREO;
    if (flags & SubmitFlags::Capture) bgfxFlags |= BGFX_SUBMIT_CAPTURE;
    if (flags & SubmitFlags::Compute) bgfxFlags |= BGFX_SUBMIT_COMPUTE;
    
    bgfx::submit(viewId, static_cast<bgfx::ProgramHandle::Handle>(program.index()), 0, bgfxFlags);
}

void RenderDevice::submitCompute(uint32_t viewId, ProgramHandle program, 
                                  uint32_t numX, uint32_t numY, uint32_t numZ) {
    if (!program.valid()) return;
    bgfx::dispatch(viewId, static_cast<bgfx::ProgramHandle::Handle>(program.index()), 
                   numX, numY, numZ);
}

void RenderDevice::setDebugName(DeviceHandle handle, const char* name) {
    if (handle.valid()) {
        bgfx::setName(static_cast<bgfx::FrameBufferHandle::Handle>(handle.index()), name);
    }
}

void RenderDevice::setDebugName(ShaderHandle handle, const char* name) {
    if (handle.valid()) {
        bgfx::setName(static_cast<bgfx::ShaderHandle::Handle>(handle.index()), name);
    }
}

void RenderDevice::setDebugName(BufferHandle handle, const char* name) {
    if (handle.valid()) {
        bgfx::setName(static_cast<bgfx::VertexBufferHandle::Handle>(handle.index()), name);
    }
}

void RenderDevice::setDebugName(TextureHandle handle, const char* name) {
    if (handle.valid()) {
        bgfx::setName(static_cast<bgfx::TextureHandle::Handle>(handle.index()), name);
    }
}

void RenderDevice::setDebugName(FrameBufferHandle handle, const char* name) {
    if (handle.valid()) {
        bgfx::setName(static_cast<bgfx::FrameBufferHandle::Handle>(handle.index()), name);
    }
}

void RenderDevice::debugTextClear() {
    bgfx::dbgTextClear();
}

void RenderDevice::debugTextPrintf(uint32_t viewId, uint16_t x, uint16_t y, 
                                    uint8_t color, const char* format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    bgfx::dbgTextPrintf(x, y, color, "%s", buffer);
    BX_UNUSED(viewId);
}

void RenderDevice::requestScreenshot(const char* filename) {
    bgfx::requestScreenShot(filename);
}

void RenderDevice::setWindowHandle(void* windowHandle) {
    bgfx::PlatformData pd;
    pd.nwh = windowHandle;
    bgfx::setPlatformData(pd);
}

void* RenderDevice::getNativeRenderHandle() const {
    // 返回 bgfx 内部句柄用于 ImGui 等集成
    return nullptr; // bgfx 不直接暴露原生句柄
}

// ==================== VertexLayout 实现 ====================

VertexLayout& VertexLayout::begin(uint32_t stream) {
    BX_UNUSED(stream);
    attributeCount = 0;
    stride = 0;
    hash = 0;
    return *this;
}

VertexLayout& VertexLayout::add(VertexAttribSemantic semantic, 
                                 VertexAttribFormat format, 
                                 bool normalized) {
    if (attributeCount >= attributes.size()) {
        return *this;
    }
    
    VertexAttrib& attr = attributes[attributeCount++];
    attr.semantic = semantic;
    attr.stream = 0;
    attr.format = format;
    attr.normalized = normalized;
    attr.offset = static_cast<uint8_t>(stride);
    
    // 计算步长
    switch (format) {
        case VertexAttribFormat::Float1: stride += 4; break;
        case VertexAttribFormat::Float2: stride += 8; break;
        case VertexAttribFormat::Float3: stride += 12; break;
        case VertexAttribFormat::Float4: stride += 16; break;
        case VertexAttribFormat::Uint8: stride += 1; break;
        case VertexAttribFormat::Uint8_4: stride += 4; break;
        case VertexAttribFormat::Int16: stride += 2; break;
        case VertexAttribFormat::Int16_4: stride += 8; break;
        case VertexAttribFormat::Int32: stride += 4; break;
        case VertexAttribFormat::Int32_4: stride += 16; break;
    }
    
    // 更新哈希
    hash ^= static_cast<uint32_t>(semantic) * 0x12345679;
    hash ^= static_cast<uint32_t>(format) * 0x98765431;
    
    return *this;
}

VertexLayout& VertexLayout::addPosition(VertexAttribFormat format) {
    return add(VertexAttribSemantic::Position, format);
}

VertexLayout& VertexLayout::addNormal(VertexAttribFormat format) {
    return add(VertexAttribSemantic::Normal, format);
}

VertexLayout& VertexLayout::addTangent(VertexAttribFormat format) {
    return add(VertexAttribSemantic::Tangent, format);
}

VertexLayout& VertexLayout::addTexCoord(uint32_t index, VertexAttribFormat format) {
    switch (index) {
        case 0: return add(VertexAttribSemantic::TexCoord0, format);
        case 1: return add(VertexAttribSemantic::TexCoord1, format);
        case 2: return add(VertexAttribSemantic::TexCoord2, format);
        case 3: return add(VertexAttribSemantic::TexCoord3, format);
        default: return add(VertexAttribSemantic::TexCoord0, format);
    }
}

VertexLayout& VertexLayout::addColor(uint32_t index, VertexAttribFormat format) {
    switch (index) {
        case 0: return add(VertexAttribSemantic::Color0, format);
        case 1: return add(VertexAttribSemantic::Color1, format);
        default: return add(VertexAttribSemantic::Color0, format);
    }
}

VertexLayout& VertexLayout::end() {
    // 最终哈希计算
    hash ^= stride * 0xABCDEF01;
    return *this;
}

} // namespace render
} // namespace phoenix
