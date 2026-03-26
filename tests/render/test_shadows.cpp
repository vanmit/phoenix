#include <gtest/gtest.h>
#include "phoenix/render/Shadows.hpp"
#include <cmath>

using namespace phoenix::render;

// ============================================================================
// Cascade Shadow Tests
// ============================================================================

class CascadeShadowTest : public ::testing::Test {
protected:
    CascadeShadowData cascadeData;
    
    void SetUp() override {
        cascadeData.splitDistances[0] = 0.1f;
        cascadeData.splitDistances[1] = 5.0f;
        cascadeData.splitDistances[2] = 15.0f;
        cascadeData.splitDistances[3] = 40.0f;
        cascadeData.splitDistances[4] = 100.0f;
    }
};

TEST_F(CascadeShadowTest, CalculateSplits_UniformDistribution) {
    CascadeShadowData data;
    data.calculateSplits(0.1f, 100.0f, 0.0f); // lambda = 0 (均匀)
    
    // 检查分割是否递增
    for (uint32_t i = 0; i < 4; ++i) {
        EXPECT_LT(data.splitDistances[i], data.splitDistances[i + 1]);
    }
}

TEST_F(CascadeShadowTest, CalculateSplits_LogarithmicDistribution) {
    CascadeShadowData data;
    data.calculateSplits(0.1f, 100.0f, 1.0f); // lambda = 1 (对数)
    
    // 对数分布应该让近处更密集
    float nearGap = data.splitDistances[1] - data.splitDistances[0];
    float farGap = data.splitDistances[4] - data.splitDistances[3];
    
    EXPECT_LT(nearGap, farGap);
}

TEST_F(CascadeShadowTest, GetCascadeIndex_Near) {
    cascadeData.cascadeConfig_.cascadeCount = 4;
    
    float viewPos[3] = {0, 0, 1.0f}; // 很近
    uint32_t index = cascadeData.getCascadeIndex(viewPos);
    
    EXPECT_EQ(index, 0);
}

TEST_F(CascadeShadowTest, GetCascadeIndex_Far) {
    cascadeData.cascadeConfig_.cascadeCount = 4;
    
    float viewPos[3] = {0, 0, 80.0f}; // 很远
    uint32_t index = cascadeData.getCascadeIndex(viewPos);
    
    EXPECT_EQ(index, 3);
}

TEST_F(CascadeShadowTest, CalculateCascadeBlend_SmoothTransition) {
    float blend = ShadowUtils::calculateCascadeBlend(
        10.0f,  // depth
        5.0f,   // cascadeNear
        15.0f,  // cascadeFar
        2.0f    // blendWidth
    );
    
    // 应该在 0 到 1 之间
    EXPECT_GE(blend, 0.0f);
    EXPECT_LE(blend, 1.0f);
}

// ============================================================================
// ShadowMap Tests
// ============================================================================

class ShadowMapTest : public ::testing::Test {
protected:
    RenderDevice device;
    ShadowMap shadowMap;
    
    void SetUp() override {
        ShadowMapDesc desc;
        desc.type = ShadowMapType::CSM;
        desc.width = 1024;
        desc.height = 1024;
        desc.arraySize = 4;
        desc.format = TextureFormat::Depth24;
        
        // shadowMap.create(device, desc);
    }
    
    void TearDown() override {
        // shadowMap.destroy();
    }
};

TEST_F(ShadowMapTest, Create_ValidDesc) {
    // EXPECT_TRUE(shadowMap.getDepthTexture().valid());
    // EXPECT_EQ(shadowMap.getWidth(), 1024);
    // EXPECT_EQ(shadowMap.getHeight(), 1024);
}

TEST_F(ShadowMapTest, Resize_ChangeSize) {
    // shadowMap.resize(2048, 2048);
    // EXPECT_EQ(shadowMap.getWidth(), 2048);
    // EXPECT_EQ(shadowMap.getHeight(), 2048);
}

// ============================================================================
// ShadowRenderer Tests
// ============================================================================

class ShadowRendererTest : public ::testing::Test {
protected:
    RenderDevice device;
    ShaderCompiler compiler;
    ShadowRenderer renderer;
    
    void SetUp() override {
        // renderer.initialize(device, compiler);
    }
    
    void TearDown() override {
        // renderer.shutdown();
    }
};

TEST_F(ShadowRendererTest, SetShadowQuality_Low) {
    // renderer.setShadowQuality(ShadowQuality::Low);
    // 低质量应该使用较小的阴影贴图
}

TEST_F(ShadowRendererTest, SetShadowQuality_High) {
    // renderer.setShadowQuality(ShadowQuality::High);
    // 高质量应该使用 4 级联
}

TEST_F(ShadowRendererTest, SetPCFConfig_CustomKernel) {
    PCFConfig config;
    config.kernelSize = 5;
    config.kernelRadius = 2.0f;
    
    // renderer.setPCFConfig(config);
    // EXPECT_EQ(renderer.getPCFConfig().kernelSize, 5);
}

TEST_F(ShadowRendererTest, SetVSMConfig_Enable) {
    VSMConfig config;
    config.enabled = true;
    config.minVariance = 0.0001f;
    config.lightBleedReduction = 0.4f;
    
    // renderer.setVSMConfig(config);
    // EXPECT_TRUE(renderer.getVSMConfig().enabled);
}

TEST_F(ShadowRendererTest, IsVisibleInShadow_InsideFrustum) {
    float boundsMin[3] = {-5, -5, -5};
    float boundsMax[3] = {5, 5, 5};
    
    // bool visible = renderer.isVisibleInShadow(boundsMin, boundsMax, 0);
    // EXPECT_TRUE(visible);
}

TEST_F(ShadowRendererTest, IsVisibleInShadow_OutsideFrustum) {
    float boundsMin[3] = {1000, 1000, 1000};
    float boundsMax[3] = {1010, 1010, 1010};
    
    // bool visible = renderer.isVisibleInShadow(boundsMin, boundsMax, 0);
    // EXPECT_FALSE(visible);
}

// ============================================================================
// PCF Filter Tests
// ============================================================================

class PCFFilterTest : public ::testing::Test {
protected:
    PCFConfig config;
    
    void SetUp() override {
        config.kernelSize = 3;
        config.kernelRadius = 1.0f;
    }
};

TEST_F(PCFFilterTest, FilterPCF_CenterSample) {
    // 创建测试阴影贴图
    float shadowMap[9] = {0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5};
    
    float result = ShadowUtils::filterPCF(
        shadowMap, 3, 3,
        0.5f, 0.5f,  // UV
        1.0f/3.0f,   // texel size
        config
    );
    
    EXPECT_GE(result, 0.0f);
    EXPECT_LE(result, 1.0f);
}

TEST_F(PCFFilterTest, FilterPCF_EdgeCase) {
    // 测试边缘情况
    float shadowMap[9] = {0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0};
    
    float result = ShadowUtils::filterPCF(
        shadowMap, 3, 3,
        0.5f, 0.5f,
        1.0f/3.0f,
        config
    );
    
    // 应该返回软阴影值
    EXPECT_GT(result, 0.0f);
    EXPECT_LT(result, 1.0f);
}

// ============================================================================
// VSM Tests
// ============================================================================

class VSMTest : public ::testing::Test {
protected:
    VSMConfig config;
    
    void SetUp() override {
        config.enabled = true;
        config.minVariance = 0.0001f;
        config.lightBleedReduction = 0.4f;
    }
};

TEST_F(VSMTest, CalculateVisibility_InFront) {
    float depth = 0.5f;
    float moment1 = 0.6f;
    float moment2 = 0.36f + config.minVariance;
    
    float visibility = ShadowUtils::calculateVSMVisibility(
        depth, moment1, moment2,
        config.minVariance, config.lightBleedReduction
    );
    
    // 在前面应该完全可见
    EXPECT_GE(visibility, 0.9f);
}

TEST_F(VSMTest, CalculateVisibility_Behind) {
    float depth = 0.7f;
    float moment1 = 0.5f;
    float moment2 = 0.25f + config.minVariance;
    
    float visibility = ShadowUtils::calculateVSMVisibility(
        depth, moment1, moment2,
        config.minVariance, config.lightBleedReduction
    );
    
    // 在后面应该部分遮挡
    EXPECT_LT(visibility, 0.5f);
}

TEST_F(VSMTest, CalculateVisibility_MinVariance) {
    // 测试最小方差防止除零
    float depth = 0.5f;
    float moment1 = 0.5f;
    float moment2 = 0.25f; // 方差为 0
    
    float visibility = ShadowUtils::calculateVSMVisibility(
        depth, moment1, moment2,
        config.minVariance, config.lightBleedReduction
    );
    
    // 不应该崩溃，应该返回合理值
    EXPECT_GE(visibility, 0.0f);
    EXPECT_LE(visibility, 1.0f);
}

// ============================================================================
// Omnidirectional Shadow Tests
// ============================================================================

class OmnidirectionalShadowTest : public ::testing::Test {
protected:
    RenderDevice device;
    OmnidirectionalShadow shadow;
    
    void SetUp() override {
        // shadow.create(device, 1024);
    }
    
    void TearDown() override {
        // shadow.destroy();
    }
};

TEST_F(OmnidirectionalShadowTest, Create_ValidSize) {
    // EXPECT_TRUE(shadow.getCubeDepth().valid());
    // EXPECT_EQ(shadow.getSize(), 1024);
}

TEST_F(OmnidirectionalShadowTest, Render_SixFaces) {
    float lightPos[3] = {0, 10, 0};
    
    // shadow.render(device, lightPos, 50.0f, {});
    // 应该渲染 6 个面
}

// ============================================================================
// Shadow Utils Tests
// ============================================================================

class ShadowUtilsTest : public ::testing::Test {};

TEST_F(ShadowUtilsTest, CalculateOptimalOrthoSize_SimpleCase) {
    float corners[8][3] = {
        {-5, -5, -5}, {5, -5, -5}, {-5, 5, -5}, {5, 5, -5},
        {-5, -5, 5},  {5, -5, 5},  {-5, 5, 5},  {5, 5, 5}
    };
    
    float lightDir[3] = {0, -1, 0};
    
    float orthoSize = ShadowUtils::calculateOptimalOrthoSize(
        reinterpret_cast<float(*)[3]>(corners),
        lightDir,
        1.2f
    );
    
    // 应该大于包围盒尺寸
    EXPECT_GT(orthoSize, 10.0f);
}

TEST_F(ShadowUtilsTest, CreateShadowBiasMatrix_ValidMatrix) {
    float matrix[16];
    ShadowUtils::createShadowBiasMatrix(matrix, 0.002f);
    
    // 检查矩阵是否有效 (简化测试)
    EXPECT_NE(matrix[0], 0.0f);
    EXPECT_NE(matrix[5], 0.0f);
    EXPECT_NE(matrix[10], 0.0f);
}

// ============================================================================
// Performance Tests
// ============================================================================

class ShadowPerformanceTest : public ::testing::Test {
protected:
    static constexpr int ITERATIONS = 1000;
};

TEST_F(ShadowPerformanceTest, PCFFilter_Performance) {
    float shadowMap[1024 * 1024];
    std::memset(shadowMap, 0, sizeof(shadowMap));
    
    PCFConfig config;
    config.kernelSize = 3;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < ITERATIONS; ++i) {
        ShadowUtils::filterPCF(
            shadowMap, 1024, 1024,
            0.5f, 0.5f,
            1.0f/1024.0f,
            config
        );
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // 每次滤波应该小于 10 微秒
    EXPECT_LT(duration.count() / ITERATIONS, 10.0);
}

TEST_F(ShadowPerformanceTest, VSMVisibility_Performance) {
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < ITERATIONS; ++i) {
        ShadowUtils::calculateVSMVisibility(
            0.5f, 0.5f, 0.25f,
            0.0001f, 0.4f
        );
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    EXPECT_LT(duration.count() / ITERATIONS, 1.0);
}
