#include "phoenix/render/PostProcess.hpp"
#include <cstring>
#include <cmath>
#include <algorithm>
#include <random>

namespace phoenix {
namespace render {

// ============================================================================
// FXAAConfig Implementation
// ============================================================================

FXAAConfig FXAAConfig::getPreset(Quality q) {
    FXAAConfig config;
    config.quality = q;
    
    switch (q) {
        case Quality::Low:
            config.subpixelQuality = 0.0f;
            config.edgeThreshold = 0.25f;
            config.edgeThresholdMin = 0.125f;
            config.iterations = 4;
            break;
        case Quality::Medium:
            config.subpixelQuality = 0.5f;
            config.edgeThreshold = 0.166f;
            config.edgeThresholdMin = 0.0833f;
            config.iterations = 8;
            break;
        case Quality::High:
            config.subpixelQuality = 0.75f;
            config.edgeThreshold = 0.125f;
            config.edgeThresholdMin = 0.0625f;
            config.iterations = 12;
            break;
        case Quality::Ultra:
            config.subpixelQuality = 0.9f;
            config.edgeThreshold = 0.0625f;
            config.edgeThresholdMin = 0.0312f;
            config.iterations = 16;
            break;
        case Quality::Extreme:
            config.subpixelQuality = 1.0f;
            config.edgeThreshold = 0.0312f;
            config.edgeThresholdMin = 0.0156f;
            config.iterations = 24;
            break;
    }
    
    return config;
}

// ============================================================================
// BloomEffect Implementation
// ============================================================================

bool BloomEffect::initialize(RenderDevice& device, ShaderCompiler& compiler) {
    device_ = &device;
    
    // 创建着色器程序
    // thresholdProgram_ = compiler.createBloomThresholdShader(...);
    // blurProgram_ = compiler.createBloomBlurShader(...);
    // combineProgram_ = compiler.createBloomCombineShader(...);
    
    return true;
}

void BloomEffect::shutdown() {
    for (auto& fb : mipFrameBuffers_) {
        // bgfx::destroy(fb);
    }
    mipChain_.clear();
    
    bloomUniforms_.reset();
    device_ = nullptr;
}

void BloomEffect::render(RenderDevice& device, TextureHandle input,
                         TextureHandle output, uint32_t viewId) {
    if (!config_.enabled) {
        return;
    }
    
    // 1. 阈值提取 (提取高亮部分)
    // device.setTexture(0, input);
    // device.submit(viewId, thresholdProgram_);
    
    // 2. 降采样 + 模糊 (多次迭代)
    uint32_t width = width_;
    uint32_t height = height_;
    
    for (uint32_t i = 0; i < config_.iterations && i < BLOOM_MIP_LEVELS; ++i) {
        width = std::max(1u, width / 2);
        height = std::max(1u, height / 2);
        
        // 降采样
        // device.setViewport(width, height);
        // device.submit(viewId + 1 + i, blurProgram_);
    }
    
    // 3. 上采样 + 叠加
    // device.setTexture(0, input);
    // device.setTexture(1, mipChain_[0]);
    // device.submit(viewId + BLOOM_MIP_LEVELS + 1, combineProgram_);
}

void BloomEffect::resize(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;
    
    // 重新创建 mip 链纹理
    mipChain_.clear();
    
    uint32_t w = width;
    uint32_t h = height;
    
    for (uint32_t i = 0; i < config_.iterations && i < BLOOM_MIP_LEVELS; ++i) {
        w = std::max(1u, w / 2);
        h = std::max(1u, h / 2);
        
        TextureDesc desc;
        desc.width = w;
        desc.height = h;
        desc.format = TextureFormat::RGBA16F;
        desc.renderTarget = true;
        
        // Texture tex;
        // tex.create(*device_, desc);
        // mipChain_.push_back(tex.getHandle());
    }
}

// ============================================================================
// ToneMappingEffect Implementation
// ============================================================================

bool ToneMappingEffect::initialize(RenderDevice& device, ShaderCompiler& compiler) {
    device_ = &device;
    
    // 创建色调映射着色器
    // toneMappingProgram_ = compiler.createToneMappingShader(...);
    
    toneMappingUniforms_.initialize(128);
    
    return true;
}

void ToneMappingEffect::shutdown() {
    toneMappingUniforms_.reset();
    device_ = nullptr;
}

void ToneMappingEffect::render(RenderDevice& device, TextureHandle input,
                                TextureHandle output, uint32_t viewId) {
    // 更新 Uniform
    toneMappingUniforms_.reset();
    toneMappingUniforms_.writeFloat(0, config_.exposure);
    toneMappingUniforms_.writeFloat(4, config_.gamma);
    toneMappingUniforms_.writeFloat(8, config_.whitePoint);
    toneMappingUniforms_.writeFloat(12, config_.contrast);
    toneMappingUniforms_.writeFloat(16, config_.saturation);
    toneMappingUniforms_.writeFloat(20, static_cast<float>(config_.algorithm));
    
    // device.setTexture(0, input);
    // device.submit(viewId, toneMappingProgram_);
}

void ToneMappingEffect::resize(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;
}

// ============================================================================
// SSAOEffect Implementation
// ============================================================================

bool SSAOEffect::initialize(RenderDevice& device, ShaderCompiler& compiler) {
    device_ = &device;
    
    // 生成采样核
    generateSampleKernel();
    
    // 生成噪声纹理
    generateNoiseTexture();
    
    // 创建着色器程序
    // ssaoProgram_ = compiler.createSSAOShader(...);
    // blurProgram_ = compiler.createSSAOBlurShader(...);
    
    // 创建 AO 纹理
    TextureDesc desc;
    desc.width = width_;
    desc.height = height_;
    desc.format = TextureFormat::R8;
    desc.renderTarget = true;
    
    // aoTexture_.create(device, desc);
    // blurredAoTexture_.create(device, desc);
    
    // 创建帧缓冲
    // aoFrameBuffer_.create(device, ...);
    // blurFrameBuffer_.create(device, ...);
    
    ssaoUniforms_.initialize(256);
    
    return true;
}

void SSAOEffect::shutdown() {
    aoTexture_ = TextureHandle();
    blurredAoTexture_ = TextureHandle();
    noiseTexture_ = TextureHandle();
    
    aoFrameBuffer_ = FrameBufferHandle();
    blurFrameBuffer_ = FrameBufferHandle();
    
    ssaoUniforms_.reset();
    device_ = nullptr;
}

void SSAOEffect::render(RenderDevice& device, TextureHandle input,
                        TextureHandle output, uint32_t viewId) {
    if (!config_.enabled) {
        return;
    }
    
    // 更新 Uniform
    ssaoUniforms_.reset();
    ssaoUniforms_.writeFloat(0, config_.radius);
    ssaoUniforms_.writeFloat(4, config_.bias);
    ssaoUniforms_.writeFloat(8, config_.intensity);
    ssaoUniforms_.writeFloat(12, config_.scale);
    ssaoUniforms_.writeFloat(16, static_cast<float>(config_.sampleCount));
    
    // 绑定 G-Buffer 纹理
    // device.setTexture(0, normalTexture_);
    // device.setTexture(1, depthTexture_);
    // device.setTexture(2, noiseTexture_);
    
    // 渲染 SSAO
    // device.submit(viewId, ssaoProgram_);
    
    // 可选：模糊
    if (config_.useBlur) {
        // device.setTexture(0, aoTexture_);
        // device.submit(viewId + 1, blurProgram_);
    }
}

void SSAOEffect::resize(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;
    
    // 重新创建 AO 纹理
    // (实现略)
}

void SSAOEffect::setGBufferTextures(TextureHandle normal, TextureHandle depth) {
    normalTexture_ = normal;
    depthTexture_ = depth;
}

void SSAOEffect::generateSampleKernel() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    for (uint32_t i = 0; i < SSAOConstants::SSAO_KERNEL_SIZE; ++i) {
        // 随机半球方向
        float x = dist(gen) * 2.0f - 1.0f;
        float y = dist(gen) * 2.0f - 1.0f;
        float z = dist(gen);
        
        sampleKernel[i][0] = x;
        sampleKernel[i][1] = y;
        sampleKernel[i][2] = z;
        
        // 归一化
        const float len = std::sqrt(x*x + y*y + z*z);
        sampleKernel[i][0] /= len;
        sampleKernel[i][1] /= len;
        sampleKernel[i][2] /= len;
        
        // 非线性分布 (更多样本靠近中心)
        const float scale = static_cast<float>(i) / SSAOConstants::SSAO_KERNEL_SIZE;
        const float lerpValue = scale * scale;
        sampleKernel[i][0] *= lerpValue;
        sampleKernel[i][1] *= lerpValue;
        sampleKernel[i][2] *= lerpValue * 0.1f + 0.9f;
    }
}

void SSAOEffect::generateNoiseTexture() {
    // 创建 4x4 噪声纹理
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    for (uint32_t i = 0; i < SSAOConstants::SSAO_NOISE_SIZE * 
                            SSAOConstants::SSAO_NOISE_SIZE; ++i) {
        const float x = dist(gen) * 2.0f - 1.0f;
        const float y = dist(gen) * 2.0f - 1.0f;
        const float z = 0.0f;
        
        noiseMatrix[i * 4 + 0] = x;
        noiseMatrix[i * 4 + 1] = y;
        noiseMatrix[i * 4 + 2] = z;
        noiseMatrix[i * 4 + 3] = 1.0f;
    }
    
    // 创建噪声纹理
    // noiseTexture_.create(device, ...);
}

// ============================================================================
// FXAAEffect Implementation
// ============================================================================

bool FXAAEffect::initialize(RenderDevice& device, ShaderCompiler& compiler) {
    device_ = &device;
    
    // 创建 FXAA 着色器
    // fxaaProgram_ = compiler.createFXAAShader(...);
    
    fxaaUniforms_.initialize(64);
    
    return true;
}

void FXAAEffect::shutdown() {
    fxaaUniforms_.reset();
    device_ = nullptr;
}

void FXAAEffect::render(RenderDevice& device, TextureHandle input,
                        TextureHandle output, uint32_t viewId) {
    if (!config_.enabled) {
        return;
    }
    
    // 更新 Uniform
    fxaaUniforms_.reset();
    fxaaUniforms_.writeFloat(0, 1.0f / width_);
    fxaaUniforms_.writeFloat(4, 1.0f / height_);
    fxaaUniforms_.writeFloat(8, config_.subpixelQuality);
    fxaaUniforms_.writeFloat(12, config_.edgeThreshold);
    fxaaUniforms_.writeFloat(16, config_.edgeThresholdMin);
    fxaaUniforms_.writeFloat(20, static_cast<float>(config_.iterations));
    
    // device.setTexture(0, input);
    // device.submit(viewId, fxaaProgram_);
}

void FXAAEffect::resize(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;
}

// ============================================================================
// ColorGradingEffect Implementation
// ============================================================================

bool ColorGradingEffect::initialize(RenderDevice& device, ShaderCompiler& compiler) {
    device_ = &device;
    
    // 创建颜色分级着色器
    // colorGradingProgram_ = compiler.createColorGradingShader(...);
    
    colorGradingUniforms_.initialize(256);
    
    return true;
}

void ColorGradingEffect::shutdown() {
    colorGradingUniforms_.reset();
    device_ = nullptr;
}

void ColorGradingEffect::render(RenderDevice& device, TextureHandle input,
                                 TextureHandle output, uint32_t viewId) {
    if (!config_.enabled) {
        return;
    }
    
    // 更新 Uniform
    colorGradingUniforms_.reset();
    
    // 基本调整
    colorGradingUniforms_.writeFloat(0, config_.temperature);
    colorGradingUniforms_.writeFloat(4, config_.tint);
    colorGradingUniforms_.writeFloat(8, config_.saturation);
    colorGradingUniforms_.writeFloat(12, config_.contrast);
    colorGradingUniforms_.writeFloat(16, config_.brightness);
    colorGradingUniforms_.writeFloat(20, config_.gamma);
    
    // Lift/Gamma/Gain
    colorGradingUniforms_.writeVec4(24, config_.lift.r, config_.lift.g, 
                                     config_.lift.b, 0);
    colorGradingUniforms_.writeVec4(40, config_.gamma_.r, config_.gamma_.g,
                                     config_.gamma_.b, 0);
    colorGradingUniforms_.writeVec4(56, config_.gain.r, config_.gain.g,
                                     config_.gain.b, 0);
    
    // device.setTexture(0, input);
    // device.submit(viewId, colorGradingProgram_);
}

void ColorGradingEffect::resize(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;
}

// ============================================================================
// PostProcessChain Implementation
// ============================================================================

PostProcessChain::PostProcessChain() = default;

PostProcessChain::~PostProcessChain() {
    shutdown();
}

bool PostProcessChain::initialize(RenderDevice& device, ShaderCompiler& compiler) {
    device_ = &device;
    
    // 创建 ping-pong 纹理
    TextureDesc desc;
    desc.width = width_;
    desc.height = height_;
    desc.format = TextureFormat::RGBA16F;
    desc.renderTarget = true;
    
    // pingPongA_.create(device, desc);
    // pingPongB_.create(device, desc);
    
    return true;
}

void PostProcessChain::shutdown() {
    effects_.clear();
    pingPongA_ = TextureHandle();
    pingPongB_ = TextureHandle();
    device_ = nullptr;
}

void PostProcessChain::addEffect(std::unique_ptr<PostProcessEffect> effect) {
    effects_.push_back(std::move(effect));
}

void PostProcessChain::removeEffect(PostProcessEffectType type) {
    effects_.erase(
        std::remove_if(effects_.begin(), effects_.end(),
            [type](const std::unique_ptr<PostProcessEffect>& e) {
                return e->getType() == type;
            }),
        effects_.end()
    );
}

void PostProcessChain::render(RenderDevice& device, TextureHandle input,
                               TextureHandle output, uint32_t viewId) {
    if (effects_.empty()) {
        return;
    }
    
    TextureHandle currentInput = input;
    TextureHandle currentOutput = pingPongA_;
    
    for (size_t i = 0; i < effects_.size(); ++i) {
        auto& effect = effects_[i];
        
        if (!effect->isEnabled()) {
            continue;
        }
        
        const bool isLast = (i == effects_.size() - 1);
        
        effect->render(device, currentInput, 
                       isLast ? output : currentOutput,
                       viewId + static_cast<uint32_t>(i));
        
        // 交换 ping-pong
        std::swap(currentInput, currentOutput);
    }
}

void PostProcessChain::resize(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;
    
    for (auto& effect : effects_) {
        effect->resize(width, height);
    }
}

TextureHandle PostProcessChain::getIntermediateTexture(uint32_t index) const {
    return (index % 2 == 0) ? pingPongA_ : pingPongB_;
}

// ============================================================================
// PostProcessStack Implementation
// ============================================================================

PostProcessStack::PostProcessStack() = default;

PostProcessStack::~PostProcessStack() {
    shutdown();
}

bool PostProcessStack::initialize(RenderDevice& device, ShaderCompiler& compiler) {
    device_ = &device;
    compiler_ = &compiler;
    
    // 创建效果
    bloomEffect_ = std::make_unique<BloomEffect>();
    toneMappingEffect_ = std::make_unique<ToneMappingEffect>();
    ssaoEffect_ = std::make_unique<SSAOEffect>();
    fxaaEffect_ = std::make_unique<FXAAEffect>();
    colorGradingEffect_ = std::make_unique<ColorGradingEffect>();
    
    // 初始化各个效果
    if (!bloomEffect_->initialize(device, compiler)) {
        return false;
    }
    if (!toneMappingEffect_->initialize(device, compiler)) {
        return false;
    }
    if (!ssaoEffect_->initialize(device, compiler)) {
        return false;
    }
    if (!fxaaEffect_->initialize(device, compiler)) {
        return false;
    }
    if (!colorGradingEffect_->initialize(device, compiler)) {
        return false;
    }
    
    // 创建全屏四边形
    createFullscreenQuad();
    
    return true;
}

void PostProcessStack::shutdown() {
    bloomEffect_.reset();
    toneMappingEffect_.reset();
    ssaoEffect_.reset();
    fxaaEffect_.reset();
    colorGradingEffect_.reset();
    
    for (auto& tex : intermediateTextures_) {
        // bgfx::destroy(tex);
    }
    intermediateTextures_.clear();
    
    device_ = nullptr;
    compiler_ = nullptr;
}

void PostProcessStack::resize(uint32_t width, uint32_t height) {
    bloomEffect_->resize(width, height);
    toneMappingEffect_->resize(width, height);
    ssaoEffect_->resize(width, height);
    fxaaEffect_->resize(width, height);
    colorGradingEffect_->resize(width, height);
}

void PostProcessStack::render(RenderDevice& device, TextureHandle input,
                               TextureHandle output, uint32_t viewId) {
    stats_.activeEffects = 0;
    stats_.passes = 0;
    
    TextureHandle currentInput = input;
    TextureHandle currentOutput = output;
    
    // 1. SSAO (需要 G-Buffer)
    if (ssaoEffect_->isEnabled()) {
        // ssaoEffect_->render(device, currentInput, currentOutput, viewId++);
        stats_.activeEffects++;
        stats_.passes++;
    }
    
    // 2. Bloom
    if (bloomEffect_->isEnabled()) {
        // bloomEffect_->render(device, currentInput, currentOutput, viewId++);
        stats_.activeEffects++;
        stats_.passes++;
    }
    
    // 3. 色调映射 (总是应用)
    // toneMappingEffect_->render(device, currentInput, currentOutput, viewId++);
    stats_.passes++;
    
    // 4. FXAA
    if (fxaaEffect_->isEnabled()) {
        // fxaaEffect_->render(device, currentInput, currentOutput, viewId++);
        stats_.activeEffects++;
        stats_.passes++;
    }
    
    // 5. 颜色分级
    if (colorGradingEffect_->isEnabled()) {
        // colorGradingEffect_->render(device, currentInput, currentOutput, viewId++);
        stats_.activeEffects++;
        stats_.passes++;
    }
}

void PostProcessStack::setGBufferTextures(TextureHandle normal, TextureHandle depth) {
    ssaoEffect_->setGBufferTextures(normal, depth);
}

void PostProcessStack::createFullscreenQuad() {
    // 创建全屏四边形顶点缓冲
    // (实现略)
}

void PostProcessStack::updateUniforms() {
    // 更新所有效果的 Uniform
}

// ============================================================================
// ToneMapping Implementation
// ============================================================================

namespace ToneMapping {

Color reinhard(const Color& hdr, float exposure) {
    const float r = hdr.r * exposure;
    const float g = hdr.g * exposure;
    const float b = hdr.b * exposure;
    
    return Color(
        r / (1.0f + r),
        g / (1.0f + g),
        b / (1.0f + b),
        hdr.a
    );
}

Color reinhard2(const Color& hdr, float exposure, float whitePoint) {
    const float r = hdr.r * exposure;
    const float g = hdr.g * exposure;
    const float b = hdr.b * exposure;
    
    const float whiteSq = whitePoint * whitePoint;
    
    return Color(
        r * (1.0f + r / whiteSq) / (1.0f + r * r / whiteSq),
        g * (1.0f + g / whiteSq) / (1.0f + g * g / whiteSq),
        b * (1.0f + b / whiteSq) / (1.0f + b * b / whiteSq),
        hdr.a
    );
}

Color aces(const Color& hdr, float exposure) {
    // ACES 电影曲线近似
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    
    const float r = hdr.r * exposure;
    const float g = hdr.g * exposure;
    const float b_ = hdr.b * exposure;
    
    return Color(
        (r * (a * r + b)) / (r * (c * r + d) + e),
        (g * (a * g + b)) / (g * (c * g + d) + e),
        (b_ * (a * b_ + b)) / (b_ * (c * b_ + d) + e),
        hdr.a
    );
}

Color acesApprox(const Color& hdr, float exposure) {
    const float r = hdr.r * exposure;
    const float g = hdr.g * exposure;
    const float b = hdr.b * exposure;
    
    const float a = 0.6f * 2.51f;
    const float b_ = 0.6f * 0.03f;
    const float c = 0.6f * 2.43f;
    const float d = 0.6f * 0.59f;
    const float e = 0.6f * 0.14f;
    
    return Color(
        std::clamp((r * (a * r + b_)) / (r * (c * r + d) + e), 0.0f, 1.0f),
        std::clamp((g * (a * g + b_)) / (g * (c * g + d) + e), 0.0f, 1.0f),
        std::clamp((b * (a * b + b_)) / (b * (c * b + d) + e), 0.0f, 1.0f),
        hdr.a
    );
}

Color uncharted2(const Color& hdr, float exposure) {
    const float A = 0.15f;
    const float B = 0.50f;
    const float C = 0.10f;
    const float D = 0.20f;
    const float E = 0.02f;
    const float F = 0.30f;
    
    const auto uncharted2Tonemap = [=](float x) -> float {
        return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
    };
    
    const float W = 11.2f;
    const float exposureBias = 2.0f;
    
    const float currX = hdr.r * exposure * exposureBias;
    const float currY = hdr.g * exposure * exposureBias;
    const float currZ = hdr.b * exposure * exposureBias;
    
    const float whiteX = uncharted2Tonemap(W);
    const float whiteY = uncharted2Tonemap(W);
    const float whiteZ = uncharted2Tonemap(W);
    
    return Color(
        uncharted2Tonemap(currX) / whiteX,
        uncharted2Tonemap(currY) / whiteY,
        uncharted2Tonemap(currZ) / whiteZ,
        hdr.a
    );
}

Color hejlDawson(const Color& hdr, float exposure) {
    const float r = hdr.r * exposure;
    const float g = hdr.g * exposure;
    const float b = hdr.b * exposure;
    
    const auto curve = [](float x) -> float {
        return std::max(0.0f, std::min(1.0f, (x * (2.51f * x + 0.03f)) / 
                                        (x * (2.43f * x + 0.59f) + 0.14f)));
    };
    
    return Color(curve(r), curve(g), curve(b), hdr.a);
}

Color hable(const Color& hdr, float exposure) {
    const float A = 0.2f;
    const float B = 0.29f;
    const float C = 0.24f;
    const float D = 0.272f;
    const float E = 0.02f;
    const float F = 0.3f;
    
    const auto hableTonemap = [=](float x) -> float {
        return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
    };
    
    const float W = 5.0f;
    
    const float currX = hdr.r * exposure;
    const float currY = hdr.g * exposure;
    const float currZ = hdr.b * exposure;
    
    const float whiteX = hableTonemap(W);
    const float whiteY = hableTonemap(W);
    const float whiteZ = hableTonemap(W);
    
    return Color(
        hableTonemap(currX) / whiteX,
        hableTonemap(currY) / whiteY,
        hableTonemap(currZ) / whiteZ,
        hdr.a
    );
}

Color neutral(const Color& hdr, float exposure) {
    const float r = hdr.r * exposure;
    const float g = hdr.g * exposure;
    const float b = hdr.b * exposure;
    
    const float startCompression = 0.8f - 0.04f;
    const float desaturation = 0.15f;
    
    const auto neutralTonemap = [=](float x) -> float {
        if (x < startCompression) {
            return x;
        } else {
            const float compressed = std::log2(x) - std::log2(startCompression);
            const float factor = 1.0f - desaturation;
            return startCompression + compressed * factor;
        }
    };
    
    return Color(
        neutralTonemap(r),
        neutralTonemap(g),
        neutralTonemap(b),
        hdr.a
    );
}

Color apply(const Color& hdr, ToneMappingAlgorithm algorithm,
            float exposure, float gamma) {
    Color result;
    
    switch (algorithm) {
        case ToneMappingAlgorithm::Reinhard:
            result = reinhard(hdr, exposure);
            break;
        case ToneMappingAlgorithm::Reinhard2:
            result = reinhard2(hdr, exposure, 1.0f);
            break;
        case ToneMappingAlgorithm::ACES:
            result = aces(hdr, exposure);
            break;
        case ToneMappingAlgorithm::ACESApprox:
            result = acesApprox(hdr, exposure);
            break;
        case ToneMappingAlgorithm::Uncharted2:
            result = uncharted2(hdr, exposure);
            break;
        case ToneMappingAlgorithm::HejlDawson:
            result = hejlDawson(hdr, exposure);
            break;
        case ToneMappingAlgorithm::Hable:
            result = hable(hdr, exposure);
            break;
        case ToneMappingAlgorithm::Neutral:
            result = neutral(hdr, exposure);
            break;
        default:
            result = hdr;
    }
    
    // 应用 gamma 校正
    const float invGamma = 1.0f / gamma;
    result.r = std::pow(std::clamp(result.r, 0.0f, 1.0f), invGamma);
    result.g = std::pow(std::clamp(result.g, 0.0f, 1.0f), invGamma);
    result.b = std::pow(std::clamp(result.b, 0.0f, 1.0f), invGamma);
    
    return result;
}

} // namespace ToneMapping

} // namespace render
} // namespace phoenix
