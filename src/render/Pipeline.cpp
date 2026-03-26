#include "phoenix/render/Pipeline.hpp"
#include <bgfx/bgfx.h>
#include <cstring>
#include <algorithm>

namespace phoenix {
namespace render {

// ==================== RenderTarget 实现 ====================

RenderTarget::~RenderTarget() {
    destroy();
}

bool RenderTarget::create(RenderDevice& device, const RenderTargetDesc& desc) {
    device_ = &device;
    width_ = desc.width;
    height_ = desc.height;
    isDepth_ = desc.isDepth;
    
    bgfx::TextureHandle handle;
    
    if (desc.isDepth) {
        handle = bgfx::createTexture2D(
            static_cast<uint16_t>(desc.width),
            static_cast<uint16_t>(desc.height),
            false, 1,
            bgfx::TextureFormat::D24S8,
            BGFX_TEXTURE_RT | BGFX_TEXTURE_MSAA_SAMPLE_4
        );
    } else {
        handle = bgfx::createTexture2D(
            static_cast<uint16_t>(desc.width),
            static_cast<uint16_t>(desc.height),
            false, 1,
            bgfx::TextureFormat::RGBA8,
            BGFX_TEXTURE_RT | BGFX_TEXTURE_MSAA_SAMPLE_4
        );
    }
    
    if (!handle.idx) {
        return false;
    }
    
    texture_ = TextureHandle(handle.idx);
    
    // 创建帧缓冲
    bgfx::FrameBufferHandle fb = bgfx::createFrameBuffer(1, &handle, true);
    if (fb.idx) {
        frameBuffer_ = FrameBufferHandle(fb.idx);
    }
    
    return true;
}

void RenderTarget::destroy() {
    if (frameBuffer_.valid()) {
        bgfx::destroy(static_cast<bgfx::FrameBufferHandle::Handle>(frameBuffer_.index()));
        frameBuffer_ = FrameBufferHandle();
    }
    if (texture_.valid()) {
        bgfx::destroy(static_cast<bgfx::TextureHandle::Handle>(texture_.index()));
        texture_ = TextureHandle();
    }
    device_ = nullptr;
}

// ==================== GBuffer 实现 ====================

GBuffer::~GBuffer() {
    destroy();
}

bool GBuffer::create(RenderDevice& device, const GBufferConfig& config) {
    device_ = &device;
    width_ = config.width;
    height_ = config.height;
    
    // 创建 G-Buffer 纹理
    albedo_ = TextureHandle(bgfx::createTexture2D(
        static_cast<uint16_t>(width_), static_cast<uint16_t>(height_),
        false, 1, bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_RT
    ).idx);
    
    normal_ = TextureHandle(bgfx::createTexture2D(
        static_cast<uint16_t>(width_), static_cast<uint16_t>(height_),
        false, 1, bgfx::TextureFormat::RGBA16F, BGFX_TEXTURE_RT
    ).idx);
    
    material_ = TextureHandle(bgfx::createTexture2D(
        static_cast<uint16_t>(width_), static_cast<uint16_t>(height_),
        false, 1, bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_RT
    ).idx);
    
    // 深度纹理
    depth_ = TextureHandle(bgfx::createTexture2D(
        static_cast<uint16_t>(width_), static_cast<uint16_t>(height_),
        false, 1, bgfx::TextureFormat::D24S8, BGFX_TEXTURE_RT | BGFX_TEXTURE_MSAA_SAMPLE_4
    ).idx);
    
    // 创建帧缓冲
    std::array<bgfx::TextureHandle, 4> textures = {
        static_cast<bgfx::TextureHandle::Handle>(albedo_.index()),
        static_cast<bgfx::TextureHandle::Handle>(normal_.index()),
        static_cast<bgfx::TextureHandle::Handle>(material_.index()),
        static_cast<bgfx::TextureHandle::Handle>(depth_.index())
    };
    
    frameBuffer_ = FrameBufferHandle(bgfx::createFrameBuffer(
        static_cast<uint8_t>(textures.size()), textures.data(), true
    ).idx);
    
    return frameBuffer_.valid();
}

void GBuffer::destroy() {
    if (frameBuffer_.valid()) {
        bgfx::destroy(static_cast<bgfx::FrameBufferHandle::Handle>(frameBuffer_.index()));
        frameBuffer_ = FrameBufferHandle();
    }
    
    auto destroyTexture = [](TextureHandle& h) {
        if (h.valid()) {
            bgfx::destroy(static_cast<bgfx::TextureHandle::Handle>(h.index()));
            h = TextureHandle();
        }
    };
    
    destroyTexture(albedo_);
    destroyTexture(normal_);
    destroyTexture(material_);
    destroyTexture(depth_);
    
    device_ = nullptr;
}

void GBuffer::resize(uint32_t width, uint32_t height) {
    if (device_) {
        destroy();
        GBufferConfig config;
        config.width = width;
        config.height = height;
        create(*device_, config);
    }
}

// ==================== RenderPass 实现 ====================

void RenderPass::create(const RenderPassDesc& desc) {
    desc_ = desc;
    viewport_ = {};
    scissor_ = {};
    active_ = false;
}

void RenderPass::begin(RenderDevice& device) {
    active_ = true;
    
    // 设置清除
    device.setViewClear(desc_.viewId, desc_.clearFlags, desc_.clearColor,
                        desc_.clearDepth, desc_.clearStencil);
    
    // 设置帧缓冲
    if (desc_.frameBuffer.valid()) {
        device.setViewFrameBuffer(desc_.viewId, desc_.frameBuffer);
    }
}

void RenderPass::end(RenderDevice& device) {
    BX_UNUSED(device);
    active_ = false;
}

void RenderPass::setViewport(const Viewport& viewport) {
    viewport_ = viewport;
}

void RenderPass::setScissor(const Rect& rect) {
    scissor_ = rect;
}

// ==================== CommandBuffer 实现 ====================

CommandBuffer::CommandBuffer() {
    commands_.reserve(INITIAL_COMMAND_CAPACITY);
    uniformData_.reserve(INITIAL_UNIFORM_CAPACITY);
    initialized_ = true;
}

CommandBuffer::~CommandBuffer() {
    reset();
}

void CommandBuffer::reset() {
    commands_.clear();
    uniformData_.clear();
}

void CommandBuffer::draw(const DrawCall& drawCall) {
    Command cmd;
    cmd.type = Command::Draw;
    cmd.drawCall = drawCall;
    commands_.push_back(cmd);
}

void CommandBuffer::clear(uint32_t viewId, ClearFlags flags, const Color& color,
                          float depth, uint8_t stencil) {
    Command cmd;
    cmd.type = Command::Clear;
    cmd.clear.viewId = viewId;
    cmd.clear.flags = flags;
    cmd.clear.color = color;
    cmd.clear.depth = depth;
    cmd.clear.stencil = stencil;
    commands_.push_back(cmd);
}

void CommandBuffer::submit(uint32_t viewId, ProgramHandle program, SubmitFlags flags) {
    Command cmd;
    cmd.type = Command::Submit;
    cmd.submit.viewId = viewId;
    cmd.submit.program = program;
    cmd.submit.flags = flags;
    commands_.push_back(cmd);
}

void CommandBuffer::setTransform(const float* matrix, uint64_t handle) {
    Command cmd;
    cmd.type = Command::SetTransform;
    cmd.transform.transformHandle = handle;
    std::memcpy(cmd.transform.matrix, matrix, 16 * sizeof(float));
    commands_.push_back(cmd);
}

void CommandBuffer::setUniform(UniformHandle handle, const void* data, uint16_t num) {
    Command cmd;
    cmd.type = Command::SetUniform;
    cmd.uniform.handle = handle;
    cmd.uniform.num = num;
    
    size_t offset = uniformData_.size();
    size_t size = num * 16; // 假设最大为 mat4
    uniformData_.resize(offset + size);
    std::memcpy(uniformData_.data() + offset, data, size);
    
    cmd.uniform.data = uniformData_.data() + offset;
    commands_.push_back(cmd);
}

void CommandBuffer::setTexture(uint8_t stage, TextureHandle texture, uint32_t flags) {
    Command cmd;
    cmd.type = Command::SetTexture;
    cmd.texture.stage = stage;
    cmd.texture.texture = texture;
    cmd.texture.flags = flags;
    commands_.push_back(cmd);
}

// ==================== CommandBufferPool 实现 ====================

CommandBufferPool::CommandBufferPool(size_t initialSize) {
    for (size_t i = 0; i < initialSize; ++i) {
        buffers_.push_back(std::make_unique<CommandBuffer>());
        available_.push_back(buffers_.back().get());
    }
}

CommandBufferPool::~CommandBufferPool() {
    buffers_.clear();
    available_.clear();
}

CommandBuffer* CommandBufferPool::acquire() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (available_.empty()) {
        buffers_.push_back(std::make_unique<CommandBuffer>());
        available_.push_back(buffers_.back().get());
    }
    CommandBuffer* buffer = available_.back();
    available_.pop_back();
    buffer->reset();
    return buffer;
}

void CommandBufferPool::release(CommandBuffer* buffer) {
    if (!buffer) return;
    std::lock_guard<std::mutex> lock(mutex_);
    available_.push_back(buffer);
}

void CommandBufferPool::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& buffer : buffers_) {
        buffer->reset();
    }
    available_.clear();
    for (auto& buffer : buffers_) {
        available_.push_back(buffer.get());
    }
}

// ==================== ForwardRenderer 实现 ====================

ForwardRenderer::ForwardRenderer() {
    std::memset(viewMatrix_, 0, sizeof(viewMatrix_));
    std::memset(projectionMatrix_, 0, sizeof(projectionMatrix_));
    ambientLight_ = Color(0.1f, 0.1f, 0.1f, 1.0f);
    std::memset(&directionalLight_, 0, sizeof(directionalLight_));
}

ForwardRenderer::~ForwardRenderer() {
    shutdown();
}

bool ForwardRenderer::initialize(RenderDevice& device) {
    device_ = &device;
    return true;
}

void ForwardRenderer::shutdown() {
    device_ = nullptr;
    commandPool_.reset();
    renderPasses_.clear();
    drawCalls_.clear();
}

void ForwardRenderer::beginFrame() {
    drawCalls_.clear();
}

void ForwardRenderer::addDrawCall(const DrawCall& drawCall) {
    drawCalls_.push_back(drawCall);
}

void ForwardRenderer::addRenderPass(const RenderPass& pass) {
    renderPasses_.push_back(pass);
}

void ForwardRenderer::endFrame() {
    if (!device_) return;
    
    // 处理绘制调用
    for (const auto& drawCall : drawCalls_) {
        // 实际实现需要设置 uniform、纹理等
        device_->submit(0, drawCall.program);
    }
}

void ForwardRenderer::setViewProjection(const float* view, const float* projection) {
    std::memcpy(viewMatrix_, view, 16 * sizeof(float));
    std::memcpy(projectionMatrix_, projection, 16 * sizeof(float));
}

void ForwardRenderer::setAmbientLight(const Color& color) {
    ambientLight_ = color;
}

void ForwardRenderer::setDirectionalLight(const float* direction, const Color& color, float intensity) {
    std::memcpy(directionalLight_.direction, direction, 3 * sizeof(float));
    directionalLight_.color = color;
    directionalLight_.intensity = intensity;
}

// ==================== DeferredRenderer 实现 ====================

DeferredRenderer::DeferredRenderer() : device_(nullptr), outputTexture_() {}

DeferredRenderer::~DeferredRenderer() {
    shutdown();
}

bool DeferredRenderer::initialize(RenderDevice& device, const GBufferConfig& config) {
    device_ = &device;
    
    if (!gBuffer_.create(device, config)) {
        return false;
    }
    
    // 创建输出目标
    RenderTargetDesc outputDesc;
    outputDesc.width = config.width;
    outputDesc.height = config.height;
    outputDesc.format = TextureFormat::RGBA8;
    outputDesc.isDepth = false;
    outputTarget_.create(device, outputDesc);
    outputTexture_ = outputTarget_.getTexture();
    
    return true;
}

void DeferredRenderer::shutdown() {
    gBuffer_.destroy();
    outputTarget_.destroy();
    device_ = nullptr;
    lights_.clear();
}

void DeferredRenderer::resize(uint32_t width, uint32_t height) {
    gBuffer_.resize(width, height);
}

void DeferredRenderer::beginGeometryPass() {
    // 设置 G-Buffer 为渲染目标
}

void DeferredRenderer::endGeometryPass() {
    // 结束几何通道
}

void DeferredRenderer::beginLightingPass() {
    // 开始光照通道
}

void DeferredRenderer::endLightingPass() {
    // 结束光照通道，合成最终图像
}

void DeferredRenderer::addGeometryDrawCall(const DrawCall& drawCall) {
    BX_UNUSED(drawCall);
    // 添加到几何绘制队列
}

void DeferredRenderer::addPointLight(const float* position, const Color& color,
                                      float radius, float intensity) {
    Light light;
    light.type = Light::Point;
    std::memcpy(light.position, position, 3 * sizeof(float));
    light.color = color;
    light.radius = radius;
    light.intensity = intensity;
    lights_.push_back(light);
}

void DeferredRenderer::addSpotLight(const float* position, const float* direction,
                                     const Color& color, float radius, 
                                     float intensity, float spotAngle) {
    Light light;
    light.type = Light::Spot;
    std::memcpy(light.position, position, 3 * sizeof(float));
    std::memcpy(light.direction, direction, 3 * sizeof(float));
    light.color = color;
    light.radius = radius;
    light.intensity = intensity;
    light.spotAngle = spotAngle;
    lights_.push_back(light);
}

// ==================== RenderStateManager 实现 ====================

const RenderState& RenderStateManager::getDefaultState() {
    static RenderState state;
    return state;
}

const RenderState& RenderStateManager::getOpaqueState() {
    static RenderState state = RenderState::opaqueState();
    return state;
}

const RenderState& RenderStateManager::getTransparentState() {
    static RenderState state = RenderState::transparentState();
    return state;
}

const RenderState& RenderStateManager::getShadowState() {
    static RenderState state;
    state.depthTest = true;
    state.depthWrite = true;
    state.depthFunc = DepthFunc::LessEqual;
    state.cullMode = CullMode::Front;
    return state;
}

const RenderState& RenderStateManager::getUIState() {
    static RenderState state;
    state.blendEnable = true;
    state.srcBlend = BlendFactor::SrcAlpha;
    state.dstBlend = BlendFactor::InvSrcAlpha;
    state.depthTest = false;
    state.depthWrite = false;
    state.cullMode = CullMode::None;
    return state;
}

uint64_t RenderStateManager::blendState(BlendFactor src, BlendFactor dst, BlendOp op) {
    BX_UNUSED(src, dst, op);
    // 转换为 bgfx 状态
    return 0;
}

uint64_t RenderStateManager::depthState(DepthFunc func, bool write) {
    BX_UNUSED(func, write);
    return 0;
}

uint64_t RenderStateManager::cullState(CullMode mode, FrontFace face) {
    BX_UNUSED(mode, face);
    return 0;
}

uint64_t RenderStateManager::makeState(const RenderState& state) {
    uint64_t bgfxState = 0;
    
    // 混合
    if (state.blendEnable) {
        bgfxState |= BGFX_STATE_BLEND_ALPHA;
    }
    
    // 深度测试
    if (state.depthTest) {
        bgfxState |= BGFX_STATE_DEPTH_TEST_LESS;
        if (state.depthWrite) {
            bgfxState |= BGFX_STATE_WRITE_Z;
        }
    }
    
    // 剔除
    switch (state.cullMode) {
        case CullMode::None:
            bgfxState |= BGFX_STATE_CULL_NONE;
            break;
        case CullMode::Front:
            bgfxState |= BGFX_STATE_CULL_CW;
            break;
        case CullMode::Back:
            bgfxState |= BGFX_STATE_CULL_CCW;
            break;
    }
    
    return bgfxState;
}

} // namespace render
} // namespace phoenix
