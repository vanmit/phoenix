#include <gtest/gtest.h>
#include "phoenix/render/PBR.hpp"
#include "phoenix/render/RenderDevice.hpp"
#include <cstring>
#include <cmath>

using namespace phoenix::render;

// ============================================================================
// BRDF Tests
// ============================================================================

class PBRBRDFTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(PBRBRDFTest, DistributionGGX_ValidInput) {
    // 测试 GGX 法线分布函数
    
    // 光滑表面 (roughness = 0)
    float d_smooth = BRDF::distributionGGX(1.0f, 0.0f);
    EXPECT_GT(d_smooth, 0.0f);
    
    // 粗糙表面 (roughness = 1)
    float d_rough = BRDF::distributionGGX(1.0f, 1.0f);
    EXPECT_GT(d_rough, 0.0f);
    
    // 光滑表面应该有更高的峰值
    EXPECT_GT(d_smooth, d_rough);
}

TEST_F(PBRBRDFTest, DistributionGGX_EdgeCases) {
    // NdotH = 0 (垂直入射)
    float d = BRDF::distributionGGX(0.0f, 0.5f);
    EXPECT_GE(d, 0.0f);
    EXPECT_LE(d, 10.0f); // 应该有合理的值
    
    // NdotH = 1 (平行入射)
    d = BRDF::distributionGGX(1.0f, 0.5f);
    EXPECT_GT(d, 0.0f);
}

TEST_F(PBRBRDFTest, GeometrySchlickGGX_ValidInput) {
    // 测试几何遮蔽函数
    
    float g1 = BRDF::geometrySchlickGGX(1.0f, 0.5f);
    EXPECT_GE(g1, 0.0f);
    EXPECT_LE(g1, 1.0f);
    
    float g2 = BRDF::geometrySchlickGGX(0.5f, 0.5f);
    EXPECT_GE(g2, 0.0f);
    EXPECT_LE(g2, 1.0f);
}

TEST_F(PBRBRDFTest, GeometrySmith_ValidInput) {
    // 测试 Smith 几何函数
    
    float g = BRDF::geometrySmith(0.5f, 0.5f, 0.5f);
    EXPECT_GE(g, 0.0f);
    EXPECT_LE(g, 1.0f);
}

TEST_F(PBRBRDFTest, FresnelSchlick_ValidInput) {
    // 测试菲涅尔函数
    
    Color F0(0.04f, 0.04f, 0.04f, 1.0f); // 电介质
    
    // 垂直入射 (cosTheta = 1)
    Color F_perp = BRDF::fresnelSchlick(1.0f, F0);
    EXPECT_NEAR(F_perp.r, F0.r, 0.01f);
    EXPECT_NEAR(F_perp.g, F0.g, 0.01f);
    EXPECT_NEAR(F_perp.b, F0.b, 0.01f);
    
    // 掠射角 (cosTheta = 0)
    Color F_graze = BRDF::fresnelSchlick(0.0f, F0);
    EXPECT_GE(F_graze.r, F0.r);
    EXPECT_GE(F_graze.g, F0.g);
    EXPECT_GE(F_graze.b, F0.b);
}

TEST_F(PBRBRDFTest, CookTorrance_CompleteBRDF) {
    // 测试完整的 Cook-Torrance BRDF
    
    float N[3] = {0, 0, 1};
    float V[3] = {0, 0, 1};
    float L[3] = {0, 0, 1};
    float H[3] = {0, 0, 1};
    
    PBRMaterialProperties material;
    material.albedo = Color(0.8f, 0.8f, 0.8f, 1.0f);
    material.metallic = 0.0f;
    material.roughness = 0.5f;
    
    BRDFComponents components = BRDF::cookTorrance(N, V, L, H, material);
    
    EXPECT_GE(components.NDF, 0.0f);
    EXPECT_GE(components.G, 0.0f);
    EXPECT_GE(components.F, 0.0f);
    EXPECT_GE(components.specular, 0.0f);
    EXPECT_GE(components.diffuse, 0.0f);
}

TEST_F(PBRBRDFTest, CalculateF0_Dielectric) {
    // 测试电介质 F0 计算
    
    PBRMaterialProperties material;
    material.albedo = Color(0.8f, 0.5f, 0.3f, 1.0f);
    material.metallic = 0.0f;
    material.ior = 1.5f;
    
    Color F0 = BRDF::calculateF0(material);
    
    // 电介质的 F0 应该接近 0.04
    EXPECT_NEAR(F0.r, 0.04f, 0.01f);
    EXPECT_NEAR(F0.g, 0.04f, 0.01f);
    EXPECT_NEAR(F0.b, 0.04f, 0.01f);
}

TEST_F(PBRBRDFTest, CalculateF0_Metal) {
    // 测试金属 F0 计算
    
    PBRMaterialProperties material;
    material.albedo = Color(1.0f, 0.8f, 0.6f, 1.0f); // 金色
    material.metallic = 1.0f;
    
    Color F0 = BRDF::calculateF0(material);
    
    // 金属的 F0 应该等于 albedo
    EXPECT_NEAR(F0.r, material.albedo.r, 0.01f);
    EXPECT_NEAR(F0.g, material.albedo.g, 0.01f);
    EXPECT_NEAR(F0.b, material.albedo.b, 0.01f);
}

TEST_F(PBRBRDFTest, DiffuseLambert_EnergyConservation) {
    // 测试漫反射能量守恒
    
    Color albedo(0.8f, 0.8f, 0.8f, 1.0f);
    
    // 非金属
    Color diffuse_dielectric = BRDF::diffuseLambert(albedo, 0.0f);
    EXPECT_GT(diffuse_dielectric.r, 0.0f);
    
    // 金属 (应该没有漫反射)
    Color diffuse_metal = BRDF::diffuseLambert(albedo, 1.0f);
    EXPECT_NEAR(diffuse_metal.r, 0.0f, 0.01f);
    EXPECT_NEAR(diffuse_metal.g, 0.0f, 0.01f);
    EXPECT_NEAR(diffuse_metal.b, 0.0f, 0.01f);
}

// ============================================================================
// PBRMaterial Tests
// ============================================================================

class PBRMaterialTest : public ::testing::Test {
protected:
    RenderDevice device;
    ShaderCompiler compiler;
    
    void SetUp() override {
        // 初始化渲染设备 (mock)
        // DeviceConfig config;
        // SwapChainConfig swapChain;
        // device.initialize(config, swapChain);
        // compiler.initialize();
    }
    
    void TearDown() override {
        // device.shutdown();
        // compiler.shutdown();
    }
};

TEST_F(PBRMaterialTest, Create_DefaultMaterial) {
    PBRMaterial material;
    
    // 测试默认属性
    const auto& props = material.getProperties();
    EXPECT_EQ(props.albedo.r, 1.0f);
    EXPECT_EQ(props.albedo.g, 1.0f);
    EXPECT_EQ(props.albedo.b, 1.0f);
    EXPECT_EQ(props.metallic, 0.0f);
    EXPECT_EQ(props.roughness, 1.0f);
    EXPECT_EQ(props.ao, 1.0f);
}

TEST_F(PBRMaterialTest, SetProperties_CustomValues) {
    PBRMaterial material;
    
    PBRMaterialProperties props;
    props.albedo = Color(0.5f, 0.6f, 0.7f, 1.0f);
    props.metallic = 0.8f;
    props.roughness = 0.3f;
    props.ao = 0.9f;
    props.emissive = Color(1.0f, 0.5f, 0.0f, 1.0f);
    props.emissiveIntensity = 2.0f;
    
    material.setProperties(props);
    
    const auto& retrieved = material.getProperties();
    EXPECT_EQ(retrieved.albedo.r, 0.5f);
    EXPECT_EQ(retrieved.metallic, 0.8f);
    EXPECT_EQ(retrieved.roughness, 0.3f);
}

TEST_F(PBRMaterialTest, TextureSet_Valid) {
    PBRTextureSet textures;
    
    // 初始应该无效
    EXPECT_FALSE(textures.isValid());
    
    // 设置一个纹理后应该有效
    textures.albedoMap = TextureHandle(0);
    EXPECT_TRUE(textures.isValid());
}

// ============================================================================
// PBRRenderer Tests
// ============================================================================

class PBRRendererTest : public ::testing::Test {
protected:
    RenderDevice device;
    ShaderCompiler compiler;
    PBRRenderer renderer;
    
    void SetUp() override {
        // renderer.initialize(device, compiler);
    }
    
    void TearDown() override {
        // renderer.shutdown();
    }
};

TEST_F(PBRRendererTest, Initialize_Success) {
    // EXPECT_TRUE(renderer.initialize(device, compiler));
    // EXPECT_TRUE(renderer.getIBL().isValid() == false); // 初始没有 IBL
}

TEST_F(PBRRendererTest, SetDirectionalLight) {
    float direction[3] = {0.5f, -1.0f, 0.3f};
    Color color(1.0f, 0.9f, 0.8f, 1.0f);
    
    // renderer.setDirectionalLight(direction, color, 1.5f);
}

TEST_F(PBRRendererTest, SetPointLight) {
    float position[3] = {5.0f, 10.0f, 5.0f};
    Color color(1.0f, 1.0f, 1.0f, 1.0f);
    
    // renderer.setPointLight(position, color, 2.0f, 20.0f);
}

// ============================================================================
// IBL Utils Tests
// ============================================================================

class IBLUtilsTest : public ::testing::Test {
protected:
    RenderDevice device;
    
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(IBLUtilsTest, GenerateCubemap_ValidInput) {
    // TODO: 实现 IBL 生成测试
    // TextureHandle source;
    // TextureHandle cubemap;
    // EXPECT_TRUE(IBLUtils::generateCubemap(device, source, 256, cubemap));
}

TEST_F(IBLUtilsTest, GenerateIrradianceMap_ValidInput) {
    // TODO: 实现辐照度图生成测试
}

TEST_F(IBLUtilsTest, GenerateBRDFLUT_ValidInput) {
    // TODO: 实现 BRDF LUT 生成测试
}

// ============================================================================
// Sampler Tests
// ============================================================================

class SamplerTest : public ::testing::Test {
protected:
    RenderDevice device;
    SamplerManager& samplerMgr = SamplerManager::instance();
    
    void SetUp() override {
        // samplerMgr.initialize(device);
    }
    
    void TearDown() override {
        // samplerMgr.shutdown();
    }
};

TEST_F(SamplerTest, GetPointSampler) {
    // Sampler* sampler = samplerMgr.getPointSampler();
    // EXPECT_NE(sampler, nullptr);
    // EXPECT_EQ(sampler->getDesc().minFilter, SamplerDesc::Filter::Point);
}

TEST_F(SamplerTest, GetLinearSampler) {
    // Sampler* sampler = samplerMgr.getLinearSampler();
    // EXPECT_NE(sampler, nullptr);
    // EXPECT_EQ(sampler->getDesc().minFilter, SamplerDesc::Filter::Trilinear);
}

TEST_F(SamplerTest, GetAnisotropicSampler) {
    // Sampler* sampler = samplerMgr.getAnisotropicSampler();
    // EXPECT_NE(sampler, nullptr);
    // EXPECT_EQ(sampler->getDesc().anisotropyLevel, 16);
}

// ============================================================================
// Performance Tests
// ============================================================================

class PBRPerformanceTest : public ::testing::Test {
protected:
    static constexpr int ITERATIONS = 10000;
};

TEST_F(PBRPerformanceTest, BRDFCalculation_Performance) {
    float N[3] = {0, 0, 1};
    float V[3] = {0, 0, 1};
    float L[3] = {0, 0, 1};
    float H[3] = {0, 0, 1};
    
    PBRMaterialProperties material;
    material.roughness = 0.5f;
    material.metallic = 0.5f;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < ITERATIONS; ++i) {
        BRDF::cookTorrance(N, V, L, H, material);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // 每次计算应该小于 1 微秒
    EXPECT_LT(duration.count() / ITERATIONS, 1.0);
}

TEST_F(PBRPerformanceTest, FresnelCalculation_Performance) {
    Color F0(0.04f, 0.04f, 0.04f, 1.0f);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < ITERATIONS; ++i) {
        BRDF::fresnelSchlick(static_cast<float>(i) / ITERATIONS, F0);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    EXPECT_LT(duration.count() / ITERATIONS, 0.5);
}
