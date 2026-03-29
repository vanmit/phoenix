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
    
    // 创建 Bloom 着色器程序
    thresholdProgram_ = compiler.createProgram(
        "assets/shaders/postprocess/fullscreen_quad.glsl",
        "assets/shaders/postprocess/bloom_threshold.glsl",
        "BloomThreshold"
    );
    
    blurProgram_ = compiler.createProgram(
        "assets/shaders/postprocess/fullscreen_quad.glsl",
        "assets/shaders/postprocess/bloom_blur.glsl",
        "BloomBlur"
    );
    
    combineProgram_ = compiler.createProgram(
        "assets/shaders/postprocess/fullscreen_quad.glsl",
        "assets/shaders/postprocess/bloom_combine.glsl",
        "BloomCombine"
    );
    
    if (!thresholdProgram_.isValid() || !blurProgram_.isValid() || !combineProgram_.isValid()) {
        return false;
    }
    
    // 初始化 Uniform 缓冲
    bloomUniforms_.initialize(64);
    
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
    bloomUniforms_.reset();
    bloomUniforms_.writeFloat(0, config_.threshold);
    bloomUniforms_.writeFloat(4, config_.thresholdSoft);
    bloomUniforms_.writeFloat(8, config_.intensity);
    bloomUniforms_.writeFloat(12, config_.knee);
    bloomUniforms_.writeVec4(16, config_.tint[0], config_.tint[1], config_.tint[2], 1.0f);
    
    device.setTexture(0, input);
    device.setUniformBuffer(0, bloomUniforms_);
    device.setViewport(width_, height_);
    device.submit(viewId, thresholdProgram_);
    
    // 2. 降采样 + 模糊 (多次迭代)
    uint32_t width = width_;
    uint32_t height = height_;
    
    for (uint32_t i = 0; i < config_.iterations && i < BLOOM_MIP_LEVELS; ++i) {
        width = std::max(1u, width / 2);
        height = std::max(1u, height / 2);
        
        // 水平模糊
        bloomUniforms_.reset();
        bloomUniforms_.writeFloat(0, 1.0f / width);
        bloomUniforms_.writeFloat(4, 1.0f / height);
        bloomUniforms_.writeFloat(8, 0.0f); // 水平方向
        bloomUniforms_.writeFloat(12, config_.blurRadius);
        
        device.setViewport(width, height);
        device.setUniformBuffer(0, bloomUniforms_);
        device.submit(viewId + 1 + i * 2, blurProgram_);
        
        // 垂直模糊
        bloomUniforms_.reset();
        bloomUniforms_.writeFloat(0, 1.0f / width);
        bloomUniforms_.writeFloat(4, 1.0f / height);
        bloomUniforms_.writeFloat(8, 1.0f); // 垂直方向
        bloomUniforms_.writeFloat(12, config_.blurRadius);
        
        device.submit(viewId + 2 + i * 2, blurProgram_);
    }
    
    // 3. 上采样 + 叠加
    bloomUniforms_.reset();
    bloomUniforms_.writeFloat(0, config_.intensity);
    bloomUniforms_.writeFloat(4, config_.scatter);
    
    device.setTexture(0, input);
    device.setTexture(1, mipChain_.empty() ? input : mipChain_[0]);
    device.setViewport(width_, height_);
    device.submit(viewId + BLOOM_MIP_LEVELS * 2 + 1, combineProgram_);
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
        
        Texture tex;
        tex.create(*device_, desc);
        mipChain_.push_back(tex.getHandle());
    }
}

// ============================================================================
// ToneMappingEffect Implementation
// ============================================================================

bool ToneMappingEffect::initialize(RenderDevice& device, ShaderCompiler& compiler) {
    device_ = &device;
    
    // 创建色调映射着色器
    toneMappingProgram_ = compiler.createProgram(
        "assets/shaders/postprocess/fullscreen_quad.glsl",
        "assets/shaders/postprocess/tonemapping.glsl",
        "ToneMapping"
    );
    
    if (!toneMappingProgram_.isValid()) {
        return false;
    }
    
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
    toneMappingUniforms_.writeInt(20, static_cast<int>(config_.algorithm));
    
    device.setTexture(0, input);
    device.setUniformBuffer(0, toneMappingUniforms_);
    device.setViewport(width_, height_);
    device.submit(viewId, toneMappingProgram_);
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
    ssaoProgram_ = compiler.createProgram(
        "assets/shaders/postprocess/fullscreen_quad.glsl",
        "assets/shaders/postprocess/ssao.glsl",
        "SSAO"
    );
    
    blurProgram_ = compiler.createProgram(
        "assets/shaders/postprocess/fullscreen_quad.glsl",
        "assets/shaders/postprocess/ssao_blur.glsl",
        "SSAOBlur"
    );
    
    if (!ssaoProgram_.isValid()) {
        return false;
    }
    
    // 创建 AO 纹理
    TextureDesc desc;
    desc.width = width_;
    desc.height = height_;
    desc.format = TextureFormat::R8;
    desc.renderTarget = true;
    
    aoTexture_.create(device, desc);
    blurredAoTexture_.create(device, desc);
    
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
    ssaoUniforms_.writeFloat(16, config_.sharpness);
    ssaoUniforms_.writeInt(20, static_cast<int>(config_.sampleCount));
    ssaoUniforms_.writeInt(24, config_.useNoise ? 1 : 0);
    ssaoUniforms_.writeInt(28, config_.useNormals ? 1 : 0);
    ssaoUniforms_.writeFloat(32, config_.normalThreshold);
    
    // 写入采样核
    for (uint32_t i = 0; i < SSAOConstants::SSAO_KERNEL_SIZE; ++i) {
        ssaoUniforms_.writeVec4(64 + i * 16, 
                                sampleKernel[i][0], sampleKernel[i][1], 
                                sampleKernel[i][2], 1.0f);
    }
    
    // 绑定 G-Buffer 纹理
    device.setTexture(0, normalTexture_);
    device.setTexture(1, depthTexture_);
    device.setTexture(2, noiseTexture_);
    device.setUniformBuffer(0, ssaoUniforms_);
    
    // 渲染 SSAO
    device.setViewport(width_, height_);
    device.submit(viewId, ssaoProgram_);
    
    // 可选：模糊
    if (config_.useBlur && blurProgram_.isValid()) {
        ssaoUniforms_.reset();
        ssaoUniforms_.writeFloat(0, 1.0f / width_);
        ssaoUniforms_.writeFloat(4, 1.0f / height_);
        ssaoUniforms_.writeInt(8, 0); // 水平
        ssaoUniforms_.writeInt(12, config_.blurIterations);
        ssaoUniforms_.writeFloat(16, config_.sharpness);
        
        device.setTexture(0, aoTexture_);
        device.setTexture(1, depthTexture_);
        device.submit(viewId + 1, blurProgram_);
    }
}

void SSAOEffect::resize(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;
    
    // 重新创建 AO 纹理
    TextureDesc desc;
    desc.width = width;
    desc.height = height;
    desc.format = TextureFormat::R8;
    desc.renderTarget = true;
    
    aoTexture_.create(*device_, desc);
    blurredAoTexture_.create(*device_, desc);
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
        if (len > 0.001f) {
            sampleKernel[i][0] /= len;
            sampleKernel[i][1] /= len;
            sampleKernel[i][2] /= len;
        }
        
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
    
    std::vector<float> noiseData(
        SSAOConstants::SSAO_NOISE_SIZE * SSAOConstants::SSAO_NOISE_SIZE * 4
    );
    
    for (uint32_t i = 0; i < SSAOConstants::SSAO_NOISE_SIZE * 
                            SSAOConstants::SSAO_NOISE_SIZE; ++i) {
        const float x = dist(gen) * 2.0f - 1.0f;
        const float y = dist(gen) * 2.0f - 1.0f;
        const float z = 0.0f;
        
        noiseData[i * 4 + 0] = x;
        noiseData[i * 4 + 1] = y;
        noiseData[i * 4 + 2] = z;
        noiseData[i * 4 + 3] = 1.0f;
    }
    
    // 创建噪声纹理
    TextureDesc desc;
    desc.width = SSAOConstants::SSAO_NOISE_SIZE;
    desc.height = SSAOConstants::SSAO_NOISE_SIZE;
    desc.format = TextureFormat::RGBA8;
    desc.data = noiseData.data();
    desc.dataSize = static_cast<uint32_t>(noiseData.size() * sizeof(float));
    
    noiseTexture_.create(*device_, desc);
}

// ============================================================================
// FXAAEffect Implementation
// ============================================================================

bool FXAAEffect::initialize(RenderDevice& device, ShaderCompiler& compiler) {
    device_ = &device;
    
    // 创建 FXAA 着色器 (简化版本)
    // 实际项目中需要完整的 FXAA shader
    fxaaProgram_ = compiler.createProgram(
        "assets/shaders/postprocess/fullscreen_quad.glsl",
        "assets/shaders/postprocess/fxaa.glsl",
        "FXAA"
    );
    
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
    
    device.setTexture(0, input);
    device.setUniformBuffer(0, fxaaUniforms_);
    device.setViewport(width_, height_);
    device.submit(viewId, fxaaProgram_);
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
    colorGradingProgram_ = compiler.createProgram(
        "assets/shaders/postprocess/fullscreen_quad.glsl",
        "assets/shaders/postprocess/colorgrading.glsl",
        "ColorGrading"
    );
    
    if (!colorGradingProgram_.isValid()) {
        return false;
    }
    
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
    
    // 阴影/中间调/高光
    colorGradingUniforms_.writeVec4(72, config_.shadows[0], config_.shadows[1],
                                     config_.shadows[2], 0);
    colorGradingUniforms_.writeVec4(88, config_.midtones[0], config_.midtones[1],
                                     config_.midtones[2], 0);
    colorGradingUniforms_.writeVec4(104, config_.highlights[0], config_.highlights[1],
                                     config_.highlights[2], 0);
    
    // LUT 设置
    colorGradingUniforms_.writeInt(120, config_.useLUT ? 1 : 0);
    colorGradingUniforms_.writeFloat(124, config_.lutIntensity);
    
    // 通道混合矩阵
    for (int i = 0; i < 3; i++) {
        colorGradingUniforms_.writeVec4(128 + i * 16,
                                        config_.channelMix[i][0],
                                        config_.channelMix[i][1],
                                        config_.channelMix[i][2],
                                        0);
    }
    
    device.setTexture(0, input);
    if (config_.useLUT && config_.lutTexture.isValid()) {
        device.setTexture(1, config_.lutTexture);
    }
    device.setUniformBuffer(0, colorGradingUniforms_);
    device.setViewport(width_, height_);
    device.submit(viewId, colorGradingProgram_);
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
    
    pingPongA_.create(device, desc);
    pingPongB_.create(device, desc);
    
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
    taaEffect_ = std::make_unique<TAAEffect>();
    motionBlurEffect_ = std::make_unique<MotionBlurEffect>();
    depthOfFieldEffect_ = std::make_unique<DepthOfFieldEffect>();
    
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
    if (!taaEffect_->initialize(device, compiler)) {
        return false;
    }
    if (!motionBlurEffect_->initialize(device, compiler)) {
        return false;
    }
    if (!depthOfFieldEffect_->initialize(device, compiler)) {
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
    taaEffect_.reset();
    motionBlurEffect_.reset();
    depthOfFieldEffect_.reset();
    
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
    taaEffect_->resize(width, height);
    motionBlurEffect_->resize(width, height);
    depthOfFieldEffect_->resize(width, height);
}

void PostProcessStack::render(RenderDevice& device, TextureHandle input,
                               TextureHandle output, uint32_t viewId) {
    stats_.activeEffects = 0;
    stats_.passes = 0;
    
    TextureHandle currentInput = input;
    TextureHandle currentOutput = output;
    
    // 1. TAA (时间性抗锯齿 - 最早应用)
    if (taaEffect_->isEnabled()) {
        taaEffect_->render(device, currentInput, currentOutput, viewId++);
        stats_.activeEffects++;
        stats_.passes++;
    }
    
    // 2. SSAO (需要 G-Buffer)
    if (ssaoEffect_->isEnabled()) {
        ssaoEffect_->render(device, currentInput, currentOutput, viewId++);
        stats_.activeEffects++;
        stats_.passes++;
    }
    
    // 3. 运动模糊 (在景深之前)
    if (motionBlurEffect_->isEnabled()) {
        motionBlurEffect_->render(device, currentInput, currentOutput, viewId++);
        stats_.activeEffects++;
        stats_.passes++;
    }
    
    // 4. 景深效果
    if (depthOfFieldEffect_->isEnabled()) {
        depthOfFieldEffect_->render(device, currentInput, currentOutput, viewId++);
        stats_.activeEffects++;
        stats_.passes++;
    }
    
    // 5. Bloom
    if (bloomEffect_->isEnabled()) {
        bloomEffect_->render(device, currentInput, currentOutput, viewId++);
        stats_.activeEffects++;
        stats_.passes++;
    }
    
    // 6. 色调映射 (总是应用)
    toneMappingEffect_->render(device, currentInput, currentOutput, viewId++);
    stats_.passes++;
    
    // 7. FXAA (仅在 TAA 未启用时使用)
    if (fxaaEffect_->isEnabled() && !taaEffect_->isEnabled()) {
        fxaaEffect_->render(device, currentInput, currentOutput, viewId++);
        stats_.activeEffects++;
        stats_.passes++;
    }
    
    // 8. 颜色分级 (最后应用)
    if (colorGradingEffect_->isEnabled()) {
        colorGradingEffect_->render(device, currentInput, currentOutput, viewId++);
        stats_.activeEffects++;
        stats_.passes++;
    }
}

void PostProcessStack::setQualityPreset(QualityPreset preset) {
    // 根据质量档位调整所有效果的参数
    switch (preset) {
        case QualityPreset::Low:
            // 低质量：禁用高级效果，减少采样
            taaConfig_.enabled = false;
            motionBlurConfig_.enabled = false;
            depthOfFieldConfig_.enabled = false;
            depthOfFieldConfig_.quality = DepthOfFieldConfig::Quality::Low;
            motionBlurConfig_.sampleCount = 4;
            bloomConfig_.iterations = 2;
            ssaoConfig_.sampleCount = 8;
            break;
            
        case QualityPreset::Medium:
            // 中等质量：启用基本效果
            taaConfig_.enabled = true;
            taaConfig_.blendFactor = 0.15f;
            motionBlurConfig_.enabled = true;
            motionBlurConfig_.sampleCount = 8;
            depthOfFieldConfig_.enabled = true;
            depthOfFieldConfig_.quality = DepthOfFieldConfig::Quality::Medium;
            bloomConfig_.iterations = 3;
            ssaoConfig_.sampleCount = 12;
            break;
            
        case QualityPreset::High:
            // 高质量：启用所有效果
            taaConfig_.enabled = true;
            taaConfig_.blendFactor = 0.1f;
            motionBlurConfig_.enabled = true;
            motionBlurConfig_.sampleCount = 16;
            depthOfFieldConfig_.enabled = true;
            depthOfFieldConfig_.quality = DepthOfFieldConfig::Quality::High;
            bloomConfig_.iterations = 4;
            ssaoConfig_.sampleCount = 16;
            break;
            
        case QualityPreset::Ultra:
            // 极致质量：最高采样，启用所有高级特性
            taaConfig_.enabled = true;
            taaConfig_.blendFactor = 0.08f;
            taaConfig_.useNeighborhoodClamping = true;
            motionBlurConfig_.enabled = true;
            motionBlurConfig_.sampleCount = 32;
            depthOfFieldConfig_.enabled = true;
            depthOfFieldConfig_.quality = DepthOfFieldConfig::Quality::Ultra;
            depthOfFieldConfig_.sampleCount = 64;
            bloomConfig_.iterations = 6;
            ssaoConfig_.sampleCount = 32;
            break;
    }
    
    // 应用配置到效果
    enableTAA(taaConfig_.enabled);
    enableMotionBlur(motionBlurConfig_.enabled);
    enableDepthOfField(depthOfFieldConfig_.enabled);
}

void PostProcessStack::setGBufferTextures(TextureHandle normal, TextureHandle depth) {
    ssaoEffect_->setGBufferTextures(normal, depth);
}

void PostProcessStack::createFullscreenQuad() {
    // 创建全屏四边形顶点缓冲
    // 简化实现：使用程序化生成的 fullscreen quad
    // 实际项目中需要创建顶点缓冲和索引缓冲
}

void PostProcessStack::updateUniforms() {
    // 更新所有效果的 Uniform
}

// ============================================================================
// ToneMapping Implementation
// ============================================================================

// ============================================================================
// TAAEffect Implementation
// ============================================================================

bool TAAEffect::initialize(RenderDevice& device, ShaderCompiler& compiler) {
    device_ = &device;
    
    // 创建 TAA 着色器程序
    taaProgram_ = compiler.createProgram(
        "assets/shaders/postprocess/taa_vertex.glsl",
        "assets/shaders/postprocess/taa.glsl",
        "TAA"
    );
    
    if (!taaProgram_.isValid()) {
        return false;
    }
    
    // 初始化 Uniform 缓冲
    taaUniforms_.initialize(256);
    
    // 创建历史帧纹理
    createHistoryTexture();
    
    historyValid_ = false;
    frameCount_ = 0;
    
    return true;
}

void TAAEffect::shutdown() {
    historyTexture_ = TextureHandle();
    historyFrameBuffer_ = FrameBufferHandle();
    taaUniforms_.reset();
    device_ = nullptr;
    historyValid_ = false;
}

void TAAEffect::render(RenderDevice& device, TextureHandle input,
                       TextureHandle output, uint32_t viewId) {
    if (!config_.enabled || !historyValid_) {
        // 历史无效时，直接输出当前帧
        // 实际项目中应该复制输入到输出
        resetHistory();
        return;
    }
    
    // 更新 Uniforms
    updateUniforms();
    
    // 绑定纹理
    device.setTexture(0, input);
    device.setTexture(1, historyTexture_);
    device.setTexture(2, motionVectorTexture_);
    device.setTexture(3, depthTexture_);
    device.setUniformBuffer(0, taaUniforms_);
    
    // 渲染 TAA
    device.setViewport(width_, height_);
    device.submit(viewId, taaProgram_);
    
    // 更新历史帧 (将当前输出复制到历史纹理)
    // 注意：这里需要访问底层 bgfx 纹理 handle
    // 在实际项目中，应该在 RenderDevice 中暴露 blit/copy 方法
    // 或者在这里直接调用 bgfx::blit
    // bgfx::blit(historyTexture_.getHandle(), output);
    
    frameCount_++;
}

void TAAEffect::resize(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;
    
    // 重新创建历史纹理
    createHistoryTexture();
    historyValid_ = false;
}

void TAAEffect::resetHistory() {
    historyValid_ = false;
    frameCount_ = 0;
}

void TAAEffect::setTransformMatrices(const math::Matrix4& prevViewProj, 
                                      const math::Matrix4& currViewProj,
                                      const math::Matrix4& inverseProjection) {
    prevViewProj_ = prevViewProj;
    currViewProj_ = currViewProj;
    inverseProjection_ = inverseProjection;
    historyValid_ = true;
}

void TAAEffect::createHistoryTexture() {
    if (width_ == 0 || height_ == 0) return;
    
    TextureDesc desc;
    desc.width = width_;
    desc.height = height_;
    desc.format = TextureFormat::RGBA16F;  // 使用浮点格式保持精度
    desc.renderTarget = true;
    desc.usage = TextureUsage::Sampled | TextureUsage::RenderTarget;
    
    historyTexture_.create(*device_, desc);
    
    // 创建帧缓冲
    // historyFrameBuffer_.create(*device_, desc);
}

void TAAEffect::updateUniforms() {
    taaUniforms_.reset();
    
    // 纹素大小
    taaUniforms_.writeVec2(0, 1.0f / width_, 1.0f / height_);
    
    // TAA 参数
    taaUniforms_.writeFloat(8, config_.blendFactor);
    taaUniforms_.writeFloat(12, config_.sharpness);
    taaUniforms_.writeFloat(16, config_.varianceThreshold);
    taaUniforms_.writeFloat(20, config_.motionBlurFactor);
    
    // 抖动
    taaUniforms_.writeVec4(24, 0.0f, 0.0f, 0.0f, 0.0f);  // 实际项目中应该使用抖动模式
    
    // 变换矩阵
    // 注意：需要转换为 float 数组
    taaUniforms_.writeMat4(40, prevViewProj_.data.data());
    taaUniforms_.writeMat4(104, currViewProj_.data.data());
    taaUniforms_.writeMat4(168, inverseProjection_.data.data());
}

// ============================================================================
// MotionBlurEffect Implementation
// ============================================================================

bool MotionBlurEffect::initialize(RenderDevice& device, ShaderCompiler& compiler) {
    device_ = &device;
    
    // 创建运动模糊着色器程序
    motionBlurProgram_ = compiler.createProgram(
        "assets/shaders/postprocess/fullscreen_quad.glsl",
        "assets/shaders/postprocess/motionblur.glsl",
        "MotionBlur"
    );
    
    if (!motionBlurProgram_.isValid()) {
        return false;
    }
    
    // 初始化 Uniform 缓冲
    motionBlurUniforms_.initialize(128);
    
    return true;
}

void MotionBlurEffect::shutdown() {
    motionBlurUniforms_.reset();
    device_ = nullptr;
}

void MotionBlurEffect::render(RenderDevice& device, TextureHandle input,
                               TextureHandle output, uint32_t viewId) {
    if (!config_.enabled) {
        return;
    }
    
    // 更新 Uniforms
    updateUniforms();
    
    // 绑定纹理
    device.setTexture(0, input);
    device.setTexture(1, motionVectorTexture_);
    device.setTexture(2, depthTexture_);
    device.setUniformBuffer(0, motionBlurUniforms_);
    
    // 渲染运动模糊
    device.setViewport(width_, height_);
    device.submit(viewId, motionBlurProgram_);
}

void MotionBlurEffect::resize(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;
}

void MotionBlurEffect::updateUniforms() {
    motionBlurUniforms_.reset();
    
    // 纹素大小
    motionBlurUniforms_.writeVec2(0, 1.0f / width_, 1.0f / height_);
    
    // 运动模糊参数
    motionBlurUniforms_.writeFloat(8, 1.0f / config_.shutterSpeed);
    motionBlurUniforms_.writeFloat(12, config_.intensity);
    motionBlurUniforms_.writeInt(16, static_cast<int>(config_.sampleCount));
    motionBlurUniforms_.writeFloat(20, config_.maxBlurDistance);
    motionBlurUniforms_.writeInt(24, config_.useCameraMotion ? 1 : 0);
    
    // 相机速度
    motionBlurUniforms_.writeVec3(28, cameraVelocity_.x, cameraVelocity_.y, cameraVelocity_.z);
    
    // 深度平面
    motionBlurUniforms_.writeFloat(40, 0.1f);  // near plane
    motionBlurUniforms_.writeFloat(44, 1000.0f);  // far plane
}

// ============================================================================
// DepthOfFieldEffect Implementation
// ============================================================================

bool DepthOfFieldEffect::initialize(RenderDevice& device, ShaderCompiler& compiler) {
    device_ = &device;
    
    // 创建景深着色器程序
    dofProgram_ = compiler.createProgram(
        "assets/shaders/postprocess/fullscreen_quad.glsl",
        "assets/shaders/postprocess/dof.glsl",
        "DepthOfField"
    );
    
    if (!dofProgram_.isValid()) {
        return false;
    }
    
    // 初始化 Uniform 缓冲
    dofUniforms_.initialize(256);
    
    return true;
}

void DepthOfFieldEffect::shutdown() {
    cocTexture_ = TextureHandle();
    cocFrameBuffer_ = FrameBufferHandle();
    dofUniforms_.reset();
    device_ = nullptr;
}

void DepthOfFieldEffect::render(RenderDevice& device, TextureHandle input,
                                 TextureHandle output, uint32_t viewId) {
    if (!config_.enabled) {
        return;
    }
    
    // 可选：预计算 CoC 纹理
    if (config_.quality == DepthOfFieldConfig::Quality::Ultra) {
        calculateCoCTexture(device, viewId);
    }
    
    // 更新 Uniforms
    updateUniforms();
    
    // 绑定纹理
    device.setTexture(0, input);
    device.setTexture(1, depthTexture_);
    device.setTexture(2, cocTexture_);
    device.setUniformBuffer(0, dofUniforms_);
    
    // 渲染景深
    device.setViewport(width_, height_);
    device.submit(viewId, dofProgram_);
}

void DepthOfFieldEffect::resize(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;
    
    // 根据质量调整采样数
    switch (config_.quality) {
        case DepthOfFieldConfig::Quality::Low:
            config_.sampleCount = 8;
            break;
        case DepthOfFieldConfig::Quality::Medium:
            config_.sampleCount = 16;
            break;
        case DepthOfFieldConfig::Quality::High:
            config_.sampleCount = 32;
            break;
        case DepthOfFieldConfig::Quality::Ultra:
            config_.sampleCount = 64;
            break;
    }
}

void DepthOfFieldEffect::updateUniforms() {
    dofUniforms_.reset();
    
    // 纹素大小
    dofUniforms_.writeVec2(0, 1.0f / width_, 1.0f / height_);
    
    // 镜头参数
    dofUniforms_.writeFloat(8, config_.focalDistance);
    dofUniforms_.writeFloat(12, config_.aperture);
    dofUniforms_.writeFloat(16, config_.focalLength);
    dofUniforms_.writeFloat(20, config_.maxCoC);
    
    // 采样设置
    dofUniforms_.writeInt(24, static_cast<int>(config_.sampleCount));
    dofUniforms_.writeFloat(28, config_.nearBlur);
    dofUniforms_.writeFloat(32, config_.farBlur);
    
    // 传感器尺寸
    dofUniforms_.writeVec2(36, config_.sensorSize.x, config_.sensorSize.y);
    
    // Bokeh 设置
    dofUniforms_.writeInt(44, static_cast<int>(config_.bokehShape));
    dofUniforms_.writeFloat(48, config_.bokehRotation);
    dofUniforms_.writeFloat(52, config_.useVignette ? config_.vignetteIntensity : 0.0f);
}

void DepthOfFieldEffect::calculateCoCTexture(RenderDevice& device, uint32_t viewId) {
    // 预计算 CoC 纹理 (用于 Ultra 质量)
    // 1. 绑定深度纹理
    device.setTexture(0, depthTexture_);
    
    // 2. 设置 CoC 参数 Uniform
    dofUniforms_.reset();
    dofUniforms_.writeFloat(0, config_.focalDistance);
    dofUniforms_.writeFloat(4, config_.aperture);
    dofUniforms_.writeFloat(8, config_.focalLength);
    dofUniforms_.writeFloat(12, config_.maxCoC);
    
    // 3. 执行 CoC 计算着色器 (需要在 ShaderCompiler 中创建 compute program)
    // device.submit(viewId, cocComputeProgram_);
    
    // 4. 等待计算完成
    // bgfx::frame();
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
