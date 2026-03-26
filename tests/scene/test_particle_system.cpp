#include <gtest/gtest.h>
#include "phoenix/scene/particle_system.hpp"
#include "phoenix/math/vector3.hpp"
#include "phoenix/math/color.hpp"
#include <memory>

using namespace phoenix;
using namespace phoenix::scene;
using namespace phoenix::math;

// ============================================================================
// 粒子数据结构测试
// ============================================================================

TEST(ParticleTest, DefaultConstruction) {
    Particle p;
    
    EXPECT_FLOAT_EQ(p.position.x, 0.0f);
    EXPECT_FLOAT_EQ(p.lifetime, 0.0f);
    EXPECT_FLOAT_EQ(p.invLifetime, 0.0f);
    EXPECT_FLOAT_EQ(p.color.w, 1.0f);
    EXPECT_FLOAT_EQ(p.size.x, 1.0f);
}

TEST(ParticleTest, ParticleAlignment) {
    // 验证粒子结构是 16 字节对齐的（GPU 友好）
    EXPECT_EQ(sizeof(Particle) % 16, 0);
}

// ============================================================================
// 发射器配置测试
// ============================================================================

TEST(EmitterConfigTest, DefaultValues) {
    ParticleEmitterConfig config;
    
    EXPECT_EQ(config.shape, EmitterShape::Point);
    EXPECT_FLOAT_EQ(config.emissionRate, 10.0f);
    EXPECT_FLOAT_EQ(config.minSpeed, 1.0f);
    EXPECT_FLOAT_EQ(config.maxSpeed, 10.0f);
    EXPECT_FLOAT_EQ(config.minLifetime, 1.0f);
    EXPECT_FLOAT_EQ(config.maxLifetime, 3.0f);
}

TEST(EmitterConfigTest, SphereEmitter) {
    ParticleEmitterConfig config;
    config.shape = EmitterShape::Sphere;
    config.radius = 5.0f;
    
    EXPECT_EQ(config.shape, EmitterShape::Sphere);
    EXPECT_FLOAT_EQ(config.radius, 5.0f);
}

TEST(EmitterConfigTest, BoxEmitter) {
    ParticleEmitterConfig config;
    config.shape = EmitterShape::Box;
    config.halfExtents = Vector3(2.0f, 3.0f, 4.0f);
    
    EXPECT_EQ(config.shape, EmitterShape::Box);
    EXPECT_FLOAT_EQ(config.halfExtents.x, 2.0f);
    EXPECT_FLOAT_EQ(config.halfExtents.y, 3.0f);
    EXPECT_FLOAT_EQ(config.halfExtents.z, 4.0f);
}

TEST(EmitterConfigTest, ConeEmitter) {
    ParticleEmitterConfig config;
    config.shape = EmitterShape::Cone;
    config.coneAngle = 0.5f;
    config.coneHeight = 3.0f;
    
    EXPECT_EQ(config.shape, EmitterShape::Cone);
    EXPECT_FLOAT_EQ(config.coneAngle, 0.5f);
    EXPECT_FLOAT_EQ(config.coneHeight, 3.0f);
}

TEST(EmitterConfigTest, BurstSettings) {
    ParticleEmitterConfig config;
    config.burstCount = 50;
    config.burstInterval = 2.0f;
    
    EXPECT_FLOAT_EQ(config.burstCount, 50.0f);
    EXPECT_FLOAT_EQ(config.burstInterval, 2.0f);
}

TEST(EmitterConfigTest, ColorOverLifetime) {
    ParticleEmitterConfig config;
    config.minColor = Color(1.0f, 0.0f, 0.0f, 1.0f);  // 红色
    config.maxColor = Color(0.0f, 0.0f, 1.0f, 0.0f);  // 透明蓝色
    
    EXPECT_FLOAT_EQ(config.minColor.r, 1.0f);
    EXPECT_FLOAT_EQ(config.minColor.g, 0.0f);
    EXPECT_FLOAT_EQ(config.maxColor.b, 1.0f);
    EXPECT_FLOAT_EQ(config.maxColor.a, 0.0f);
}

// ============================================================================
// 力场测试
// ============================================================================

TEST(ForceFieldTest, GravityField) {
    auto field = ParticleForceField::gravity(9.81f);
    
    EXPECT_EQ(field.type, ParticleForceField::Type::Gravity);
    EXPECT_FLOAT_EQ(field.strength, 9.81f);
    EXPECT_FLOAT_EQ(field.direction.y, -1.0f);
}

TEST(ForceFieldTest, WindField) {
    auto field = ParticleForceField::wind(Vector3(1, 0, 0), 5.0f);
    
    EXPECT_EQ(field.type, ParticleForceField::Type::Wind);
    EXPECT_FLOAT_EQ(field.strength, 5.0f);
    EXPECT_FLOAT_EQ(field.direction.x, 1.0f);
}

TEST(ForceFieldTest, VortexField) {
    auto field = ParticleForceField::vortex(Vector3(0, 0, 0), 2.0f);
    
    EXPECT_EQ(field.type, ParticleForceField::Type::Vortex);
    EXPECT_FLOAT_EQ(field.strength, 2.0f);
}

TEST(ForceFieldTest, CustomField) {
    ParticleForceField field;
    field.type = ParticleForceField::Type::Attractor;
    field.position = Vector3(10, 0, 0);
    field.strength = 100.0f;
    field.range = 50.0f;
    field.falloff = 2.0f;
    
    EXPECT_EQ(field.type, ParticleForceField::Type::Attractor);
    EXPECT_FLOAT_EQ(field.position.x, 10.0f);
    EXPECT_FLOAT_EQ(field.strength, 100.0f);
    EXPECT_FLOAT_EQ(field.range, 50.0f);
}

// ============================================================================
// 粒子系统测试
// ============================================================================

TEST(ParticleSystemTest, CreateAndDestroy) {
    ParticleSystem system;
    // 简单的创建和销毁测试
}

TEST(ParticleSystemTest, Initialize) {
    ParticleSystem system;
    
    bool success = system.initialize(1000, false);
    
    EXPECT_TRUE(success);
    EXPECT_TRUE(system.isInitialized());
    EXPECT_FALSE(system.isUsingGPU());
}

TEST(ParticleSystemTest, Shutdown) {
    ParticleSystem system;
    system.initialize(1000);
    
    system.shutdown();
    
    EXPECT_FALSE(system.isInitialized());
}

TEST(ParticleSystemTest, AddEmitter) {
    ParticleSystem system;
    system.initialize(1000);
    
    ParticleEmitterConfig config;
    config.emissionRate = 20.0f;
    
    uint32_t index = system.addEmitter(config);
    
    EXPECT_EQ(index, 0u);
    EXPECT_EQ(system.emitterCount(), 1u);
}

TEST(ParticleSystemTest, RemoveEmitter) {
    ParticleSystem system;
    system.initialize(1000);
    
    system.addEmitter(ParticleEmitterConfig());
    system.addEmitter(ParticleEmitterConfig());
    
    EXPECT_EQ(system.emitterCount(), 2u);
    
    system.removeEmitter(0);
    
    EXPECT_EQ(system.emitterCount(), 1u);
}

TEST(ParticleSystemTest, GetEmitter) {
    ParticleSystem system;
    system.initialize(1000);
    
    ParticleEmitterConfig config;
    config.emissionRate = 50.0f;
    system.addEmitter(config);
    
    auto* emitter = system.getEmitter(0);
    ASSERT_NE(emitter, nullptr);
    EXPECT_FLOAT_EQ(emitter->emissionRate, 50.0f);
    
    auto* nullEmitter = system.getEmitter(100);
    EXPECT_EQ(nullEmitter, nullptr);
}

TEST(ParticleSystemTest, AddForceField) {
    ParticleSystem system;
    system.initialize(1000);
    
    auto field = ParticleForceField::gravity();
    uint32_t index = system.addForceField(field);
    
    EXPECT_EQ(index, 0u);
    EXPECT_EQ(system.forceFieldCount(), 1u);
}

TEST(ParticleSystemTest, PlayPauseStop) {
    ParticleSystem system;
    system.initialize(1000);
    
    EXPECT_TRUE(system.isPlaying());
    
    system.pause();
    EXPECT_FALSE(system.isPlaying());
    
    system.play();
    EXPECT_TRUE(system.isPlaying());
    
    system.stop();
    EXPECT_FALSE(system.isPlaying());
}

TEST(ParticleSystemTest, Burst) {
    ParticleSystem system;
    system.initialize(1000);
    
    ParticleEmitterConfig config;
    config.shape = EmitterShape::Point;
    config.emissionRate = 0.0f;  // 不自动发射
    system.addEmitter(config);
    
    system.burst(0, 50);
    
    EXPECT_EQ(system.activeParticleCount(), 50u);
}

TEST(ParticleSystemTest, Clear) {
    ParticleSystem system;
    system.initialize(1000);
    
    ParticleEmitterConfig config;
    system.addEmitter(config);
    system.burst(0, 100);
    
    EXPECT_EQ(system.activeParticleCount(), 100u);
    
    system.clear();
    
    EXPECT_EQ(system.activeParticleCount(), 0u);
}

TEST(ParticleSystemTest, Update) {
    ParticleSystem system;
    system.initialize(1000);
    
    ParticleEmitterConfig config;
    config.emissionRate = 100.0f;
    config.minLifetime = 2.0f;
    config.maxLifetime = 2.0f;
    system.addEmitter(config);
    
    // 添加重力
    system.addForceField(ParticleForceField::gravity(9.81f));
    
    // 更新
    system.update(0.016f);
    
    // 应该有粒子被发射
    EXPECT_GT(system.activeParticleCount(), 0u);
}

TEST(ParticleSystemTest, MultipleEmitters) {
    ParticleSystem system;
    system.initialize(1000);
    
    // 添加多个发射器
    for (int i = 0; i < 5; ++i) {
        ParticleEmitterConfig config;
        config.position = Vector3(i * 5.0f, 0, 0);
        config.emissionRate = 10.0f;
        system.addEmitter(config);
    }
    
    EXPECT_EQ(system.emitterCount(), 5u);
    
    system.update(0.1f);
    
    // 所有发射器都应该发射粒子
    EXPECT_GT(system.activeParticleCount(), 0u);
}

TEST(ParticleSystemTest, ParticleLifetime) {
    ParticleSystem system;
    system.initialize(100);
    
    ParticleEmitterConfig config;
    config.emissionRate = 1000.0f;  // 高速发射
    config.minLifetime = 0.1f;
    config.maxLifetime = 0.1f;
    system.addEmitter(config);
    
    // 发射一些粒子
    system.update(0.05f);
    size_t count1 = system.activeParticleCount();
    
    // 等待粒子死亡
    system.update(0.2f);
    size_t count2 = system.activeParticleCount();
    
    // 老粒子应该已经死亡
    EXPECT_LT(count2, count1);
}

// ============================================================================
// 碰撞配置测试
// ============================================================================

TEST(ParticleCollisionTest, DefaultConfig) {
    ParticleCollisionConfig config;
    
    EXPECT_FALSE(config.enabled);
    EXPECT_FLOAT_EQ(config.radius, 0.1f);
    EXPECT_FLOAT_EQ(config.damping, 0.5f);
    EXPECT_FLOAT_EQ(config.bounce, 0.3f);
}

TEST(ParticleCollisionTest, EnableCollision) {
    ParticleSystem system;
    system.initialize(1000);
    
    ParticleCollisionConfig config;
    config.enabled = true;
    config.bounce = 0.8f;
    config.response = ParticleCollisionConfig::Response::Bounce;
    
    system.setCollisionConfig(config);
    
    auto& result = system.collisionConfig();
    EXPECT_TRUE(result.enabled);
    EXPECT_FLOAT_EQ(result.bounce, 0.8f);
}

TEST(ParticleCollisionTest, CollisionResponses) {
    // 测试不同的碰撞响应模式
    ParticleCollisionConfig bounceConfig;
    bounceConfig.response = ParticleCollisionConfig::Response::Bounce;
    
    ParticleCollisionConfig stickConfig;
    stickConfig.response = ParticleCollisionConfig::Response::Stick;
    
    ParticleCollisionConfig killConfig;
    killConfig.response = ParticleCollisionConfig::Response::Kill;
    
    EXPECT_EQ(bounceConfig.response, ParticleCollisionConfig::Response::Bounce);
    EXPECT_EQ(stickConfig.response, ParticleCollisionConfig::Response::Stick);
    EXPECT_EQ(killConfig.response, ParticleCollisionConfig::Response::Kill);
}

// ============================================================================
// 渲染模式测试
// ============================================================================

TEST(ParticleRenderModeTest, SetAndGet) {
    ParticleSystem system;
    system.initialize(1000);
    
    system.setRenderMode(ParticleRenderMode::StretchedBillboard);
    
    EXPECT_EQ(system.renderMode(), ParticleRenderMode::StretchedBillboard);
}

TEST(ParticleRenderModeTest, AllModes) {
    // 测试所有渲染模式
    std::vector<ParticleRenderMode> modes = {
        ParticleRenderMode::Billboard,
        ParticleRenderMode::StretchedBillboard,
        ParticleRenderMode::Mesh,
        ParticleRenderMode::HorizontalBillboard,
        ParticleRenderMode::VerticalBillboard
    };
    
    for (auto mode : modes) {
        ParticleSystem system;
        system.initialize(100);
        system.setRenderMode(mode);
        EXPECT_EQ(system.renderMode(), mode);
    }
}

// ============================================================================
// 性能测试
// ============================================================================

TEST(ParticlePerformanceTest, ManyParticles) {
    ParticleSystem system;
    system.initialize(10000);
    
    ParticleEmitterConfig config;
    config.emissionRate = 10000.0f;
    config.minLifetime = 10.0f;
    config.maxLifetime = 10.0f;
    system.addEmitter(config);
    
    // 更新应该能处理大量粒子
    system.update(0.016f);
    
    EXPECT_LE(system.activeParticleCount(), 10000u);
}

TEST(ParticlePerformanceTest, ManyEmitters) {
    ParticleSystem system;
    system.initialize(10000);
    
    // 添加多个发射器
    for (int i = 0; i < 20; ++i) {
        ParticleEmitterConfig config;
        config.position = Vector3(i * 2.0f, 0, 0);
        config.emissionRate = 50.0f;
        system.addEmitter(config);
    }
    
    system.update(0.016f);
    
    EXPECT_EQ(system.emitterCount(), 20u);
}

TEST(ParticlePerformanceTest, ManyForceFields) {
    ParticleSystem system;
    system.initialize(1000);
    
    // 添加多个力场
    for (int i = 0; i < 10; ++i) {
        ParticleForceField field;
        field.type = ParticleForceField::Type::Attractor;
        field.position = Vector3(i * 5.0f, 0, 0);
        field.strength = 10.0f;
        system.addForceField(field);
    }
    
    system.update(0.016f);
    
    EXPECT_EQ(system.forceFieldCount(), 10u);
}

TEST(ParticlePerformanceTest, MemoryUsage) {
    ParticleSystem system;
    system.initialize(10000);
    
    for (int i = 0; i < 10; ++i) {
        system.addEmitter(ParticleEmitterConfig());
    }
    
    size_t usage = system.memoryUsage();
    EXPECT_GT(usage, 0);
}

// ============================================================================
// GPU 数据测试
// ============================================================================

TEST(GPUParticleDataTest, StructureSize) {
    GPUParticleData data;
    
    // 验证 GPU 数据结构大小
    EXPECT_GT(sizeof(data), 0);
}

TEST(GPUParticleDataTest, DefaultValues) {
    GPUParticleData data;
    
    EXPECT_EQ(data.particleCount, 0u);
    EXPECT_EQ(data.maxParticles, 0u);
    EXPECT_FLOAT_EQ(data.deltaTime, 0.0f);
    EXPECT_FLOAT_EQ(data.simulationTime, 0.0f);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
