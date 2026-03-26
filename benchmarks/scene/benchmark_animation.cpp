/**
 * @file benchmark_animation.cpp
 * @brief Phoenix Engine 动画系统性能基准测试
 * 
 * 测试目标：
 * - 100+ 角色同时动画 60fps
 * - 千级骨骼动画性能
 * - 内存预算 <256MB
 */

#include "phoenix/scene/skeleton.hpp"
#include "phoenix/scene/animator.hpp"
#include "phoenix/scene/morph_animation.hpp"
#include "phoenix/scene/particle_system.hpp"
#include "phoenix/math/vector3.hpp"
#include "phoenix/math/quaternion.hpp"

#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <memory>
#include <iomanip>

using namespace phoenix;
using namespace phoenix::scene;
using namespace phoenix::math;

// ============================================================================
// 性能测试工具
// ============================================================================

class BenchmarkTimer {
public:
    void start(const std::string& name) {
        currentName_ = name;
        startTime_ = std::chrono::high_resolution_clock::now();
    }
    
    void stop() {
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            endTime - startTime_
        ).count();
        
        results_.push_back({currentName_, duration});
    }
    
    void printResults() const {
        std::cout << "\n========================================\n";
        std::cout << "  性能测试结果\n";
        std::cout << "========================================\n\n";
        
        for (const auto& result : results_) {
            double ms = result.duration / 1000.0;
            std::cout << std::left << std::setw(40) << result.name 
                      << ": " << std::fixed << std::setprecision(3) 
                      << ms << " ms\n";
        }
        
        std::cout << "\n";
    }
    
    void clear() {
        results_.clear();
    }
    
private:
    struct Result {
        std::string name;
        long long duration;  // microseconds
    };
    
    std::string currentName_;
    std::chrono::high_resolution_clock::time_point startTime_;
    std::vector<Result> results_;
};

BenchmarkTimer g_timer;

// ============================================================================
// 骨骼性能测试
// ============================================================================

void benchmarkSkeletonCreation(size_t boneCount) {
    std::cout << "\n测试：创建 " << boneCount << " 根骨骼\n";
    
    g_timer.start("Skeleton Creation (" + std::to_string(boneCount) + " bones)");
    
    auto skeleton = std::make_shared<Skeleton>();
    
    // 创建层级骨骼
    uint32_t parent = skeleton->addBone("Root");
    for (size_t i = 1; i < boneCount; ++i) {
        uint32_t child = skeleton->addBone("Bone" + std::to_string(i), parent);
        parent = child;
    }
    
    g_timer.stop();
    
    std::cout << "  创建时间：完成\n";
    std::cout << "  骨骼数量：" << skeleton->boneCount() << "\n";
    std::cout << "  内存使用：" << skeleton->memoryUsage() << " 字节\n";
}

void benchmarkSkeletonUpdate(size_t boneCount, int iterations) {
    std::cout << "\n测试：更新 " << boneCount << " 根骨骼 (" << iterations << " 次)\n";
    
    // 创建骨骼
    auto skeleton = std::make_shared<Skeleton>();
    uint32_t parent = skeleton->addBone("Root");
    for (size_t i = 1; i < boneCount; ++i) {
        uint32_t child = skeleton->addBone("Bone" + std::to_string(i), parent);
        parent = child;
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> posDist(-1.0f, 1.0f);
    std::uniform_real_distribution<float> rotDist(0.0f, 6.28f);
    
    g_timer.start("Skeleton Update (" + std::to_string(boneCount) + " bones)");
    
    for (int iter = 0; iter < iterations; ++iter) {
        // 更新所有骨骼
        for (size_t i = 0; i < skeleton->boneCount(); ++i) {
            Vector3 pos(posDist(gen), posDist(gen), posDist(gen));
            Quaternion rot = Quaternion::fromAxisAngle(
                Vector3(posDist(gen), posDist(gen), posDist(gen)).normalized(),
                rotDist(gen)
            );
            Vector3 scale(1.0f);
            
            skeleton->updateBonePose(static_cast<uint32_t>(i), pos, rot, scale);
        }
        
        // 计算最终矩阵
        skeleton->calculateFinalMatrices();
    }
    
    g_timer.stop();
    
    double avgTime = g_timer.results_.back().duration / 1000.0 / iterations;
    std::cout << "  平均每次更新：" << avgTime << " ms\n";
    std::cout << "  总时间：" << (g_timer.results_.back().duration / 1000.0) << " ms\n";
}

void benchmarkMatrixCalculation(size_t boneCount) {
    std::cout << "\n测试：矩阵计算 (" << boneCount << " 根骨骼)\n";
    
    auto skeleton = std::make_shared<Skeleton>();
    
    // 创建深层级
    uint32_t parent = skeleton->addBone("Root");
    for (size_t i = 1; i < boneCount; ++i) {
        uint32_t child = skeleton->addBone("Bone" + std::to_string(i), parent);
        parent = child;
    }
    
    // 设置初始姿态
    for (size_t i = 0; i < skeleton->boneCount(); ++i) {
        skeleton->updateBonePose(
            static_cast<uint32_t>(i),
            Vector3(0.1f * i, 0, 0),
            Quaternion::identity(),
            Vector3(1)
        );
    }
    
    const int iterations = 1000;
    
    g_timer.start("Matrix Calculation (" + std::to_string(boneCount) + " bones)");
    
    for (int i = 0; i < iterations; ++i) {
        skeleton->calculateFinalMatrices();
    }
    
    g_timer.stop();
    
    double avgTime = g_timer.results_.back().duration / 1000.0 / iterations;
    std::cout << "  平均每次计算：" << avgTime << " ms\n";
    std::cout << "  每秒可更新：" << (1000.0 / avgTime) << " 次\n";
}

// ============================================================================
// 动画师性能测试
// ============================================================================

void benchmarkAnimatorUpdate(size_t characterCount, size_t bonesPerCharacter) {
    std::cout << "\n测试：" << characterCount << " 个角色动画 (每角色 " 
              << bonesPerCharacter << " 根骨骼)\n";
    
    std::vector<Animator> animators;
    animators.reserve(characterCount);
    
    // 创建角色
    for (size_t c = 0; c < characterCount; ++c) {
        auto skeleton = std::make_shared<Skeleton>();
        
        uint32_t parent = skeleton->addBone("Root");
        for (size_t i = 1; i < bonesPerCharacter; ++i) {
            uint32_t child = skeleton->addBone("Bone" + std::to_string(i), parent);
            parent = child;
        }
        
        Animator animator;
        animator.setSkeleton(skeleton);
        
        // 添加动画剪辑
        auto clip = std::make_shared<AnimationClip>("Anim");
        clip->duration = 2.0f;
        animator.addClip(clip);
        animator.play(0);
        
        animators.push_back(std::move(animator));
    }
    
    std::cout << "  创建 " << characterCount << " 个角色完成\n";
    
    const int frames = 600;  // 10 秒 @ 60fps
    float deltaTime = 1.0f / 60.0f;
    
    g_timer.start("Animator Update (" + std::to_string(characterCount) + " characters)");
    
    for (int frame = 0; frame < frames; ++frame) {
        for (auto& animator : animators) {
            animator.update(deltaTime);
        }
    }
    
    g_timer.stop();
    
    double totalTime = g_timer.results_.back().duration / 1000.0;
    double fps = frames / (totalTime / 1000.0);
    
    std::cout << "  总时间：" << totalTime << " ms\n";
    std::cout << "  平均帧时间：" << (totalTime / frames) << " ms\n";
    std::cout << "  FPS: " << fps << "\n";
    
    if (fps >= 60.0) {
        std::cout << "  ✓ 达到 60fps 目标!\n";
    } else {
        std::cout << "  ✗ 未达到 60fps 目标\n";
    }
}

void benchmarkAnimationBlending(size_t layerCount) {
    std::cout << "\n测试：动画混合 (" << layerCount << " 层)\n";
    
    auto skeleton = std::make_shared<Skeleton>();
    for (size_t i = 0; i < 50; ++i) {
        skeleton->addBone("Bone" + std::to_string(i));
    }
    
    Animator animator;
    animator.setSkeleton(skeleton);
    
    // 添加多个动画层
    for (size_t i = 0; i < layerCount; ++i) {
        auto clip = std::make_shared<AnimationClip>("Layer" + std::to_string(i));
        clip->duration = 2.0f;
        animator.addClip(clip);
        animator.addLayer(static_cast<uint32_t>(i), 1.0f / layerCount);
    }
    
    const int iterations = 1000;
    
    g_timer.start("Animation Blending (" + std::to_string(layerCount) + " layers)");
    
    for (int i = 0; i < iterations; ++i) {
        animator.update(0.016f);
    }
    
    g_timer.stop();
    
    double avgTime = g_timer.results_.back().duration / 1000.0 / iterations;
    std::cout << "  平均每次混合：" << avgTime << " ms\n";
}

// ============================================================================
// 形变动画性能测试
// ============================================================================

void benchmarkMorphAnimation(size_t vertexCount, size_t morphTargetCount) {
    std::cout << "\n测试：形变动画 (" << vertexCount << " 顶点，" 
              << morphTargetCount << " 个形变目标)\n";
    
    MorphAnimationController controller;
    controller.setVertexCount(vertexCount);
    
    // 添加形变目标
    for (size_t i = 0; i < morphTargetCount; ++i) {
        MorphTarget target("Morph" + std::to_string(i));
        target.positionDeltas.resize(vertexCount, Vector3(0.1f, 0, 0));
        target.normalDeltas.resize(vertexCount, Vector3(0, 0.1f, 0));
        controller.addMorphTarget(std::move(target));
    }
    
    // 设置权重
    for (size_t i = 0; i < morphTargetCount; ++i) {
        controller.setWeight(static_cast<uint32_t>(i), 0.5f);
    }
    
    // 准备顶点数据
    std::vector<Vector3> positions(vertexCount, Vector3(0));
    std::vector<Vector3> normals(vertexCount, Vector3(0, 1, 0));
    
    const int iterations = 100;
    
    g_timer.start("Morph Animation (" + std::to_string(vertexCount) + " vertices)");
    
    for (int i = 0; i < iterations; ++i) {
        controller.apply(positions, normals);
    }
    
    g_timer.stop();
    
    double avgTime = g_timer.results_.back().duration / 1000.0 / iterations;
    std::cout << "  平均每次应用：" << avgTime << " ms\n";
    std::cout << "  内存使用：" << controller.memoryUsage() << " 字节\n";
}

// ============================================================================
// 粒子系统性能测试
// ============================================================================

void benchmarkParticleSystem(size_t maxParticles, size_t emitterCount) {
    std::cout << "\n测试：粒子系统 (最大 " << maxParticles << " 粒子，" 
              << emitterCount << " 个发射器)\n";
    
    ParticleSystem system;
    system.initialize(maxParticles, false);  // CPU 模式
    
    // 添加发射器
    for (size_t i = 0; i < emitterCount; ++i) {
        ParticleEmitterConfig config;
        config.position = Vector3(i * 5.0f, 0, 0);
        config.emissionRate = 100.0f;
        config.minLifetime = 5.0f;
        config.maxLifetime = 10.0f;
        system.addEmitter(config);
    }
    
    // 添加力场
    system.addForceField(ParticleForceField::gravity());
    
    const int frames = 600;  // 10 秒
    float deltaTime = 1.0f / 60.0f;
    
    g_timer.start("Particle System (" + std::to_string(maxParticles) + " particles)");
    
    for (int frame = 0; frame < frames; ++frame) {
        system.update(deltaTime);
    }
    
    g_timer.stop();
    
    double totalTime = g_timer.results_.back().duration / 1000.0;
    double fps = frames / (totalTime / 1000.0);
    
    std::cout << "  总时间：" << totalTime << " ms\n";
    std::cout << "  平均帧时间：" << (totalTime / frames) << " ms\n";
    std::cout << "  FPS: " << fps << "\n";
    std::cout << "  最终粒子数：" << system.activeParticleCount() << "\n";
    std::cout << "  内存使用：" << system.memoryUsage() << " 字节\n";
}

// ============================================================================
// 内存测试
// ============================================================================

void benchmarkMemoryUsage() {
    std::cout << "\n========================================\n";
    std::cout << "  内存使用测试\n";
    std::cout << "========================================\n";
    
    size_t totalMemory = 0;
    
    // 创建 100 个角色，每个 50 根骨骼
    std::vector<std::shared_ptr<Skeleton>> skeletons;
    for (int i = 0; i < 100; ++i) {
        auto skeleton = std::make_shared<Skeleton>();
        uint32_t parent = skeleton->addBone("Root");
        for (size_t j = 1; j < 50; ++j) {
            uint32_t child = skeleton->addBone("Bone" + std::to_string(j), parent);
            parent = child;
        }
        skeletons.push_back(skeleton);
        totalMemory += skeleton->memoryUsage();
    }
    
    std::cout << "100 个角色 (50 骨骼/角色) 内存：" << (totalMemory / 1024 / 1024) << " MB\n";
    
    // 粒子系统
    ParticleSystem particles;
    particles.initialize(10000);
    totalMemory += particles.memoryUsage();
    
    std::cout << "粒子系统 (10000 粒子) 内存：" << (particles.memoryUsage() / 1024 / 1024) << " MB\n";
    
    // 形变动画
    MorphAnimationController morph;
    morph.setVertexCount(5000);
    for (int i = 0; i < 20; ++i) {
        MorphTarget target;
        target.positionDeltas.resize(5000, Vector3(0.1f));
        morph.addMorphTarget(std::move(target));
    }
    totalMemory += morph.memoryUsage();
    
    std::cout << "形变动画 (5000 顶点，20 目标) 内存：" << (morph.memoryUsage() / 1024 / 1024) << " MB\n";
    
    std::cout << "\n总内存使用：" << (totalMemory / 1024 / 1024) << " MB\n";
    
    if (totalMemory < 256 * 1024 * 1024) {
        std::cout << "✓ 符合 <256MB 内存预算!\n";
    } else {
        std::cout << "✗ 超出 256MB 内存预算\n";
    }
}

// ============================================================================
// 主函数
// ============================================================================

int main(int argc, char* argv[]) {
    std::cout << "==============================================\n";
    std::cout << "  Phoenix Engine Phase 3\n";
    std::cout << "  动画系统性能基准测试\n";
    std::cout << "==============================================\n";
    
    try {
        // 骨骼性能测试
        std::cout << "\n========================================";
        std::cout << "\n  骨骼系统性能测试";
        std::cout << "\n========================================\n";
        
        benchmarkSkeletonCreation(100);
        benchmarkSkeletonCreation(1000);
        benchmarkSkeletonCreation(5000);
        
        benchmarkSkeletonUpdate(100, 100);
        benchmarkSkeletonUpdate(1000, 10);
        
        benchmarkMatrixCalculation(100);
        benchmarkMatrixCalculation(1000);
        
        // 动画师性能测试
        std::cout << "\n========================================";
        std::cout << "\n  动画师性能测试";
        std::cout << "\n========================================\n";
        
        benchmarkAnimatorUpdate(10, 50);
        benchmarkAnimatorUpdate(50, 50);
        benchmarkAnimatorUpdate(100, 50);  // 目标：100+ 角色 60fps
        
        benchmarkAnimationBlending(5);
        benchmarkAnimationBlending(10);
        
        // 形变动画测试
        std::cout << "\n========================================";
        std::cout << "\n  形变动画性能测试";
        std::cout << "\n========================================\n";
        
        benchmarkMorphAnimation(1000, 10);
        benchmarkMorphAnimation(5000, 20);
        
        // 粒子系统测试
        std::cout << "\n========================================";
        std::cout << "\n  粒子系统性能测试";
        std::cout << "\n========================================\n";
        
        benchmarkParticleSystem(1000, 5);
        benchmarkParticleSystem(10000, 10);
        
        // 内存测试
        benchmarkMemoryUsage();
        
        // 打印总结果
        g_timer.printResults();
        
        std::cout << "\n==============================================\n";
        std::cout << "  基准测试完成!\n";
        std::cout << "==============================================\n";
        
    } catch (const std::exception& e) {
        std::cerr << "错误：" << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
