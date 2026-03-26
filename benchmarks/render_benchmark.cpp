/**
 * 渲染性能基准测试 - Phoenix Engine Phase 3
 * 
 * 测试项目:
 * - PBR BRDF 计算性能
 * - 阴影映射性能
 * - 延迟渲染性能
 * - 后处理效果性能
 * - 内存使用
 */

#include <benchmark/benchmark.h>
#include "phoenix/render/PBR.hpp"
#include "phoenix/render/Shadows.hpp"
#include "phoenix/render/PostProcess.hpp"
#include <vector>
#include <random>

using namespace phoenix::render;

// ============================================================================
// PBR BRDF 基准测试
// ============================================================================

static void BM_BRDF_CookTorrance(benchmark::State& state) {
    float N[3] = {0, 0, 1};
    float V[3] = {0, 0, 1};
    float L[3] = {0, 0, 1};
    float H[3] = {0, 0, 1};
    
    PBRMaterialProperties material;
    material.roughness = 0.5f;
    material.metallic = 0.5f;
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(BRDF::cookTorrance(N, V, L, H, material));
    }
}
BENCHMARK(BM_BRDF_CookTorrance);

static void BM_BRDF_DistributionGGX(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(BRDF::distributionGGX(0.5f, 0.5f));
    }
}
BENCHMARK(BM_BRDF_DistributionGGX);

static void BM_BRDF_GeometrySmith(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(BRDF::geometrySmith(0.5f, 0.5f, 0.5f));
    }
}
BENCHMARK(BM_BRDF_GeometrySmith);

static void BM_BRDF_FresnelSchlick(benchmark::State& state) {
    Color F0(0.04f, 0.04f, 0.04f, 1.0f);
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(BRDF::fresnelSchlick(0.5f, F0));
    }
}
BENCHMARK(BM_BRDF_FresnelSchlick);

// ============================================================================
// 阴影基准测试
// ============================================================================

static void BM_Shadow_PCF_Filter_3x3(benchmark::State& state) {
    std::vector<float> shadowMap(1024 * 1024, 0.5f);
    
    PCFConfig config;
    config.kernelSize = 3;
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(ShadowUtils::filterPCF(
            shadowMap.data(), 1024, 1024,
            0.5f, 0.5f,
            1.0f/1024.0f,
            config
        ));
    }
}
BENCHMARK(BM_Shadow_PCF_Filter_3x3);

static void BM_Shadow_PCF_Filter_5x5(benchmark::State& state) {
    std::vector<float> shadowMap(1024 * 1024, 0.5f);
    
    PCFConfig config;
    config.kernelSize = 5;
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(ShadowUtils::filterPCF(
            shadowMap.data(), 1024, 1024,
            0.5f, 0.5f,
            1.0f/1024.0f,
            config
        ));
    }
}
BENCHMARK(BM_Shadow_PCF_Filter_5x5);

static void BM_Shadow_VSM_Visibility(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(ShadowUtils::calculateVSMVisibility(
            0.5f, 0.5f, 0.25f,
            0.0001f, 0.4f
        ));
    }
}
BENCHMARK(BM_Shadow_VSM_Visibility);

static void BM_Shadow_CascadeCalculation(benchmark::State& state) {
    CascadeShadowData cascadeData;
    
    float viewProj[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    float invViewProj[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    
    for (auto _ : state) {
        cascadeData.calculateSplits(0.1f, 100.0f, 0.8f);
        cascadeData.calculateCascadeFrustums(viewProj, invViewProj, 4);
    }
}
BENCHMARK(BM_Shadow_CascadeCalculation);

// ============================================================================
// 色调映射基准测试
// ============================================================================

static void BM_ToneMapping_Reinhard(benchmark::State& state) {
    Color hdr(1.0f, 1.0f, 1.0f, 1.0f);
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(ToneMapping::reinhard(hdr, 1.0f));
    }
}
BENCHMARK(BM_ToneMapping_Reinhard);

static void BM_ToneMapping_ACES(benchmark::State& state) {
    Color hdr(1.0f, 1.0f, 1.0f, 1.0f);
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(ToneMapping::aces(hdr, 1.0f));
    }
}
BENCHMARK(BM_ToneMapping_ACES);

static void BM_ToneMapping_Hable(benchmark::State& state) {
    Color hdr(1.0f, 1.0f, 1.0f, 1.0f);
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(ToneMapping::hable(hdr, 1.0f));
    }
}
BENCHMARK(BM_ToneMapping_Hable);

static void BM_ToneMapping_Apply(benchmark::State& state) {
    Color hdr(1.0f, 1.0f, 1.0f, 1.0f);
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(ToneMapping::apply(
            hdr, ToneMappingAlgorithm::ACES, 1.0f, 2.2f
        ));
    }
}
BENCHMARK(BM_ToneMapping_Apply);

// ============================================================================
// 综合场景基准测试
// ============================================================================

struct RenderScene {
    struct Object {
        float modelMatrix[16];
        PBRMaterialProperties material;
    };
    
    std::vector<Object> objects;
    std::vector<Light> lights;
    
    RenderScene() {
        // 创建 1000 个物体
        for (int i = 0; i < 1000; ++i) {
            Object obj;
            std::memset(obj.modelMatrix, 0, sizeof(obj.modelMatrix));
            obj.modelMatrix[0] = 1.0f;
            obj.modelMatrix[5] = 1.0f;
            obj.modelMatrix[10] = 1.0f;
            obj.modelMatrix[15] = 1.0f;
            
            obj.material.roughness = 0.5f;
            obj.material.metallic = 0.5f;
            
            objects.push_back(obj);
        }
        
        // 创建 16 个光源
        for (int i = 0; i < 16; ++i) {
            Light light;
            light.type = LightType::Point;
            light.position[0] = static_cast<float>(i);
            light.range = 10.0f;
            light.intensity = 1.0f;
            lights.push_back(light);
        }
    }
};

static void BM_Scene_BRDF_Calculation(benchmark::State& state) {
    RenderScene scene;
    
    float N[3] = {0, 0, 1};
    float V[3] = {0, 0, 1};
    float L[3] = {0, 0, 1};
    float H[3] = {0, 0, 1};
    
    for (auto _ : state) {
        for (const auto& obj : scene.objects) {
            benchmark::DoNotOptimize(
                BRDF::cookTorrance(N, V, L, H, obj.material)
            );
        }
    }
    
    state.SetItemsProcessed(state.iterations() * scene.objects.size());
}
BENCHMARK(BM_Scene_BRDF_Calculation);

static void BM_Scene_Light_Culling(benchmark::State& state) {
    RenderScene scene;
    
    float frustumPlanes[24]; // 6 个平面 * 4 个分量
    std::memset(frustumPlanes, 0, sizeof(frustumPlanes));
    
    for (auto _ : state) {
        uint32_t visibleCount = 0;
        for (const auto& light : scene.lights) {
            // 简化剔除测试
            benchmark::DoNotOptimize(light.isInRange(V[3]{0, 0, 0}));
            visibleCount++;
        }
        benchmark::DoNotOptimize(visibleCount);
    }
}
BENCHMARK(BM_Scene_Light_Culling);

// ============================================================================
// 内存基准测试
// ============================================================================

static void BM_Memory_PBRMaterial(benchmark::State& state) {
    for (auto _ : state) {
        PBRMaterial material;
        benchmark::DoNotOptimize(sizeof(material));
    }
}
BENCHMARK(BM_Memory_PBRMaterial);

static void BM_Memory_PBRTextureSet(benchmark::State& state) {
    for (auto _ : state) {
        PBRTextureSet textures;
        benchmark::DoNotOptimize(sizeof(textures));
    }
}
BENCHMARK(BM_Memory_PBRTextureSet);

static void BM_Memory_Light(benchmark::State& state) {
    for (auto _ : state) {
        Light light;
        benchmark::DoNotOptimize(sizeof(light));
    }
}
BENCHMARK(BM_Memory_Light);

// ============================================================================
// 主函数
// ============================================================================

BENCHMARK_MAIN();
