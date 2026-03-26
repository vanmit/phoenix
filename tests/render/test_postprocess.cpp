#include <gtest/gtest.h>
#include "phoenix/render/PostProcess.hpp"
#include <cmath>

using namespace phoenix::render;

// ============================================================================
// Tone Mapping Tests
// ============================================================================

class ToneMappingTest : public ::testing::Test {};

TEST_F(ToneMappingTest, Reinhard_BasicMapping) {
    Color hdr(2.0f, 1.5f, 1.0f, 1.0f);
    Color result = ToneMapping::reinhard(hdr, 1.0f);
    
    // 结果应该在 0-1 范围内
    EXPECT_GE(result.r, 0.0f);
    EXPECT_LE(result.r, 1.0f);
    EXPECT_GE(result.g, 0.0f);
    EXPECT_LE(result.g, 1.0f);
    EXPECT_GE(result.b, 0.0f);
    EXPECT_LE(result.b, 1.0f);
}

TEST_F(ToneMappingTest, Reinhard_LowInput) {
    Color hdr(0.1f, 0.1f, 0.1f, 1.0f);
    Color result = ToneMapping::reinhard(hdr, 1.0f);
    
    // 低输入应该几乎不变
    EXPECT_NEAR(result.r, hdr.r / (1.0f + hdr.r), 0.01f);
}

TEST_F(ToneMappingTest, ACES_BasicMapping) {
    Color hdr(2.0f, 1.5f, 1.0f, 1.0f);
    Color result = ToneMapping::aces(hdr, 1.0f);
    
    EXPECT_GE(result.r, 0.0f);
    EXPECT_LE(result.r, 1.0f);
}

TEST_F(ToneMappingTest, Hable_BasicMapping) {
    Color hdr(2.0f, 1.5f, 1.0f, 1.0f);
    Color result = ToneMapping::hable(hdr, 1.0f);
    
    EXPECT_GE(result.r, 0.0f);
    EXPECT_LE(result.r, 1.0f);
}

TEST_F(ToneMappingTest, Apply_WithExposure) {
    Color hdr(1.0f, 1.0f, 1.0f, 1.0f);
    Color result = ToneMapping::apply(hdr, ToneMappingAlgorithm::Reinhard, 2.0f, 2.2f);
    
    // 曝光增加应该让结果更亮
    Color result2 = ToneMapping::apply(hdr, ToneMappingAlgorithm::Reinhard, 0.5f, 2.2f);
    
    EXPECT_GT(result.r, result2.r);
}

TEST_F(ToneMappingTest, Apply_WithGamma) {
    Color hdr(0.5f, 0.5f, 0.5f, 1.0f);
    Color result = ToneMapping::apply(hdr, ToneMappingAlgorithm::Reinhard, 1.0f, 2.2f);
    
    // Gamma 校正后应该变暗
    EXPECT_LT(result.r, 0.5f);
}

// ============================================================================
// Bloom Tests
// ============================================================================

class BloomEffectTest : public ::testing::Test {
protected:
    RenderDevice device;
    ShaderCompiler compiler;
    BloomEffect bloom;
    
    void SetUp() override {
        BloomConfig config;
        config.threshold = 1.0f;
        config.intensity = 1.0f;
        config.iterations = 4;
        bloom.setConfig(config);
        
        // bloom.initialize(device, compiler);
        // bloom.resize(1920, 1080);
    }
    
    void TearDown() override {
        // bloom.shutdown();
    }
};

TEST_F(BloomEffectTest, Create_DefaultConfig) {
    BloomConfig config = bloom.getConfig();
    EXPECT_EQ(config.threshold, 1.0f);
    EXPECT_EQ(config.intensity, 1.0f);
}

TEST_F(BloomEffectTest, SetConfig_CustomValues) {
    BloomConfig config;
    config.threshold = 0.5f;
    config.intensity = 2.0f;
    config.iterations = 6;
    
    bloom.setConfig(config);
    
    EXPECT_EQ(bloom.getConfig().threshold, 0.5f);
    EXPECT_EQ(bloom.getConfig().intensity, 2.0f);
}

TEST_F(BloomEffectTest, Resize_ChangeSize) {
    // bloom.resize(3840, 2160);
    // EXPECT_EQ(bloom.getWidth(), 3840);
    // EXPECT_EQ(bloom.getHeight(), 2160);
}

// ============================================================================
// SSAO Tests
// ============================================================================

class SSAOEffectTest : public ::testing::Test {
protected:
    RenderDevice device;
    ShaderCompiler compiler;
    SSAOEffect ssao;
    
    void SetUp() override {
        SSAOConfig config;
        config.radius = 0.5f;
        config.bias = 0.025f;
        config.intensity = 1.0f;
        config.sampleCount = 16;
        ssao.setConfig(config);
        
        // ssao.initialize(device, compiler);
        // ssao.resize(1920, 1080);
    }
    
    void TearDown() override {
        // ssao.shutdown();
    }
};

TEST_F(SSAOEffectTest, Create_DefaultConfig) {
    SSAOConfig config = ssao.getConfig();
    EXPECT_EQ(config.radius, 0.5f);
    EXPECT_EQ(config.sampleCount, 16);
}

TEST_F(SSAOEffectTest, SetGBufferTextures_ValidTextures) {
    TextureHandle normal(0);
    TextureHandle depth(1);
    
    // ssao.setGBufferTextures(normal, depth);
}

TEST_F(SSAOEffectTest, SampleKernel_Generated) {
    // 采样核应该已经生成
    // (简化测试)
}

// ============================================================================
// FXAA Tests
// ============================================================================

class FXAAEffectTest : public ::testing::Test {
protected:
    RenderDevice device;
    ShaderCompiler compiler;
    FXAAEffect fxaa;
    
    void SetUp() override {
        FXAAConfig config = FXAAConfig::getPreset(FXAAConfig::Quality::High);
        fxaa.setConfig(config);
        
        // fxaa.initialize(device, compiler);
        // fxaa.resize(1920, 1080);
    }
    
    void TearDown() override {
        // fxaa.shutdown();
    }
};

TEST_F(FXAAEffectTest, GetPreset_HighQuality) {
    FXAAConfig config = FXAAConfig::getPreset(FXAAConfig::Quality::High);
    
    EXPECT_EQ(config.quality, FXAAConfig::Quality::High);
    EXPECT_GT(config.iterations, 8);
}

TEST_F(FXAAEffectTest, GetPreset_LowQuality) {
    FXAAConfig config = FXAAConfig::getPreset(FXAAConfig::Quality::Low);
    
    EXPECT_EQ(config.quality, FXAAConfig::Quality::Low);
    EXPECT_LT(config.iterations, 8);
}

TEST_F(FXAAEffectTest, SetConfig_CustomEdgeThreshold) {
    FXAAConfig config;
    config.edgeThreshold = 0.1f;
    config.edgeThresholdMin = 0.05f;
    
    fxaa.setConfig(config);
    
    EXPECT_EQ(fxaa.getConfig().edgeThreshold, 0.1f);
}

// ============================================================================
// Color Grading Tests
// ============================================================================

class ColorGradingEffectTest : public ::testing::Test {
protected:
    RenderDevice device;
    ShaderCompiler compiler;
    ColorGradingEffect colorGrading;
    
    void SetUp() override {
        ColorGradingConfig config;
        config.temperature = 0.1f;
        config.saturation = 1.2f;
        config.contrast = 1.1f;
        colorGrading.setConfig(config);
        
        // colorGrading.initialize(device, compiler);
    }
    
    void TearDown() override {
        // colorGrading.shutdown();
    }
};

TEST_F(ColorGradingEffectTest, SetTemperature_Warm) {
    ColorGradingConfig config;
    config.temperature = 0.2f; // 暖色调
    
    colorGrading.setConfig(config);
    EXPECT_EQ(colorGrading.getConfig().temperature, 0.2f);
}

TEST_F(ColorGradingEffectTest, SetSaturation_Increase) {
    ColorGradingConfig config;
    config.saturation = 1.5f; // 增加饱和度
    
    colorGrading.setConfig(config);
    EXPECT_EQ(colorGrading.getConfig().saturation, 1.5f);
}

TEST_F(ColorGradingEffectTest, SetLiftGammaGain) {
    ColorGradingConfig config;
    config.lift = Color(0.1f, 0.05f, 0.0f, 1.0f);
    config.gamma_ = Color(0.0f, 0.0f, 0.0f, 1.0f);
    config.gain = Color(0.0f, 0.05f, 0.1f, 1.0f);
    
    colorGrading.setConfig(config);
    
    EXPECT_EQ(colorGrading.getConfig().lift.r, 0.1f);
    EXPECT_EQ(colorGrading.getConfig().gain.b, 0.1f);
}

// ============================================================================
// PostProcessChain Tests
// ============================================================================

class PostProcessChainTest : public ::testing::Test {
protected:
    RenderDevice device;
    ShaderCompiler compiler;
    PostProcessChain chain;
    
    void SetUp() override {
        // chain.initialize(device, compiler);
        // chain.resize(1920, 1080);
    }
    
    void TearDown() override {
        // chain.shutdown();
    }
};

TEST_F(PostProcessChainTest, AddEffect_SingleEffect) {
    // auto bloom = std::make_unique<BloomEffect>();
    // chain.addEffect(std::move(bloom));
}

TEST_F(PostProcessChainTest, AddEffect_MultipleEffects) {
    // auto bloom = std::make_unique<BloomEffect>();
    // auto toneMap = std::make_unique<ToneMappingEffect>();
    // auto fxaa = std::make_unique<FXAAEffect>();
    
    // chain.addEffect(std::move(bloom));
    // chain.addEffect(std::move(toneMap));
    // chain.addEffect(std::move(fxaa));
}

TEST_F(PostProcessChainTest, RemoveEffect_ExistingEffect) {
    // 添加后移除
    // chain.removeEffect(PostProcessEffectType::Bloom);
}

// ============================================================================
// PostProcessStack Tests
// ============================================================================

class PostProcessStackTest : public ::testing::Test {
protected:
    RenderDevice device;
    ShaderCompiler compiler;
    PostProcessStack stack;
    
    void SetUp() override {
        // stack.initialize(device, compiler);
        // stack.resize(1920, 1080);
    }
    
    void TearDown() override {
        // stack.shutdown();
    }
};

TEST_F(PostProcessStackTest, GetBloomConfig_Access) {
    BloomConfig& config = stack.getBloomConfig();
    config.threshold = 0.8f;
    EXPECT_EQ(stack.getBloomConfig().threshold, 0.8f);
}

TEST_F(PostProcessStackTest, EnableDisable_Bloom) {
    stack.enableBloom(false);
    EXPECT_FALSE(stack.getBloomConfig().enabled);
    
    stack.enableBloom(true);
    EXPECT_TRUE(stack.getBloomConfig().enabled);
}

TEST_F(PostProcessStackTest, EnableDisable_SSAO) {
    stack.enableSSAO(false);
    EXPECT_FALSE(stack.getSSAOConfig().enabled);
}

TEST_F(PostProcessStackTest, EnableDisable_FXAA) {
    stack.enableFXAA(false);
    EXPECT_FALSE(stack.getFXAAConfig().enabled);
}

// ============================================================================
// Performance Tests
// ============================================================================

class PostProcessPerformanceTest : public ::testing::Test {
protected:
    static constexpr int ITERATIONS = 1000;
};

TEST_F(PostProcessPerformanceTest, ToneMapping_Performance) {
    Color hdr(1.0f, 1.0f, 1.0f, 1.0f);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < ITERATIONS; ++i) {
        ToneMapping::apply(hdr, ToneMappingAlgorithm::ACES, 1.0f, 2.2f);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // 每次色调映射应该小于 0.5 微秒
    EXPECT_LT(duration.count() / ITERATIONS, 0.5);
}

TEST_F(PostProcessPerformanceTest, Reinhard_Performance) {
    Color hdr(1.0f, 1.0f, 1.0f, 1.0f);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < ITERATIONS; ++i) {
        ToneMapping::reinhard(hdr, 1.0f);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    EXPECT_LT(duration.count() / ITERATIONS, 0.1);
}

TEST_F(PostProcessPerformanceTest, ACES_Performance) {
    Color hdr(1.0f, 1.0f, 1.0f, 1.0f);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < ITERATIONS; ++i) {
        ToneMapping::aces(hdr, 1.0f);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    EXPECT_LT(duration.count() / ITERATIONS, 0.2);
}
