/**
 * @file main.cpp
 * @brief Phoenix Engine 动画系统演示程序
 * 
 * 展示骨骼动画、形变动画、动画混合和粒子系统的使用
 */

#include "phoenix/scene/skeleton.hpp"
#include "phoenix/scene/animator.hpp"
#include "phoenix/scene/morph_animation.hpp"
#include "phoenix/scene/particle_system.hpp"
#include "phoenix/scene/physics.hpp"
#include "phoenix/math/vector3.hpp"
#include "phoenix/math/quaternion.hpp"
#include "phoenix/math/color.hpp"

#include <iostream>
#include <memory>
#include <chrono>
#include <vector>

using namespace phoenix;
using namespace phoenix::scene;
using namespace phoenix::math;

// ============================================================================
// 辅助函数
// ============================================================================

void printSection(const std::string& title) {
    std::cout << "\n========================================\n";
    std::cout << "  " << title << "\n";
    std::cout << "========================================\n\n";
}

void printVector3(const Vector3& v, const std::string& name = "") {
    if (!name.empty()) {
        std::cout << name << ": ";
    }
    std::cout << "(" << v.x << ", " << v.y << ", " << v.z << ")\n";
}

// ============================================================================
// 骨骼动画演示
// ============================================================================

void demoSkeletonAnimation() {
    printSection("骨骼动画演示");
    
    // 创建骨骼
    auto skeleton = std::make_shared<Skeleton>();
    
    // 创建简单的人形骨骼层级
    uint32_t hip = skeleton->addBone("Hip");
    uint32_t spine = skeleton->addBone("Spine", hip);
    uint32_t head = skeleton->addBone("Head", spine);
    
    uint32_t lShoulder = skeleton->addBone("L_Shoulder", spine);
    uint32_t lArm = skeleton->addBone("L_Arm", lShoulder);
    uint32_t lHand = skeleton->addBone("L_Hand", lArm);
    
    uint32_t rShoulder = skeleton->addBone("R_Shoulder", spine);
    uint32_t rArm = skeleton->addBone("R_Arm", rShoulder);
    uint32_t rHand = skeleton->addBone("R_Hand", rArm);
    
    uint32_t lLeg = skeleton->addBone("L_Leg", hip);
    uint32_t lFoot = skeleton->addBone("L_Foot", lLeg);
    
    uint32_t rLeg = skeleton->addBone("R_Leg", hip);
    uint32_t rFoot = skeleton->addBone("R_Foot", rLeg);
    
    std::cout << "创建骨骼数量：" << skeleton->boneCount() << "\n";
    std::cout << "根骨骼数量：" << skeleton->rootBones().size() << "\n\n";
    
    // 创建动画师
    Animator animator;
    animator.setSkeleton(skeleton);
    
    // 创建行走动画剪辑
    auto walkClip = std::make_shared<AnimationClip>("Walk");
    walkClip->duration = 2.0f;
    walkClip->fps = 30.0f;
    walkClip->loopMode = LoopMode::Loop;
    
    // 添加简化的行走关键帧
    AnimationClip::Channel hipChannel;
    hipChannel.boneIndex = hip;
    hipChannel.type = AnimationChannelType::Translation;
    hipChannel.interpolation = InterpolationType::Linear;
    
    // 添加关键帧
    for (int i = 0; i <= 60; ++i) {
        float t = i / 30.0f;
        hipChannel.times.push_back(t);
        
        // 简单的上下移动
        float y = std::sin(t * 3.14159f * 2) * 0.1f;
        hipChannel.values.push_back(Vector3(0, y, 0));
    }
    
    walkClip->channels.push_back(hipChannel);
    
    // 添加动画
    uint32_t walkIndex = animator.addClip(walkClip);
    std::cout << "添加动画剪辑：Walk (索引=" << walkIndex << ")\n";
    
    // 播放动画
    animator.play(walkIndex);
    std::cout << "播放动画...\n\n";
    
    // 模拟动画更新
    for (int frame = 0; frame < 60; ++frame) {
        animator.update(1.0f / 30.0f);
        
        if (frame % 20 == 0) {
            std::cout << "帧 " << frame << ": 时间=" << animator.currentTime() 
                      << "s, 播放=" << (animator.isPlaying() ? "是" : "否") << "\n";
        }
    }
    
    // 动画混合演示
    printSection("动画混合演示");
    
    // 创建跑步动画
    auto runClip = std::make_shared<AnimationClip>("Run");
    runClip->duration = 1.0f;
    runClip->loopMode = LoopMode::Loop;
    uint32_t runIndex = animator.addClip(runClip);
    
    // 从行走混合到跑步
    animator.play(runIndex, 0.5f);
    std::cout << "从行走到跑步的混合...\n";
    
    for (int i = 0; i < 30; ++i) {
        animator.update(1.0f / 30.0f);
    }
    
    std::cout << "混合完成，当前动画：" << animator.currentClipIndex() << "\n";
}

// ============================================================================
// 形变动画演示
// ============================================================================

void demoMorphAnimation() {
    printSection("形变动画演示");
    
    // 创建形变控制器
    auto morphController = std::make_shared<MorphAnimationController>();
    morphController->setVertexCount(8);  // 立方体 8 个顶点
    
    // 创建微笑形变
    MorphTarget smile("Smile");
    smile.positionDeltas = {
        Vector3(0.1f, 0.05f, 0),   // 嘴角左上
        Vector3(-0.1f, 0.05f, 0),  // 嘴角右上
        Vector3(0, 0.1f, 0),       // 上唇中
        Vector3(0, -0.05f, 0),     // 下唇中
        Vector3(0.05f, 0, 0.05f),  // 左脸颊
        Vector3(-0.05f, 0, 0.05f), // 右脸颊
        Vector3(0, 0.02f, 0),      // 鼻尖
        Vector3(0, 0.01f, 0)       // 下巴
    };
    morphController->addMorphTarget(std::move(smile));
    
    // 创建眨眼形变
    MorphTarget blink("Blink");
    blink.positionDeltas = {
        Vector3(0, -0.1f, 0),  // 左眼上
        Vector3(0, -0.1f, 0),  // 右眼上
        Vector3(0, 0, 0),
        Vector3(0, 0, 0),
        Vector3(0, 0, 0),
        Vector3(0, 0, 0),
        Vector3(0, 0, 0),
        Vector3(0, 0, 0)
    };
    morphController->addMorphTarget(std::move(blink));
    
    std::cout << "形变目标数量：" << morphController->morphTargetCount() << "\n";
    std::cout << "顶点数量：" << morphController->vertexCount() << "\n\n";
    
    // 设置权重
    morphController->setWeight(0, 1.0f);  // 微笑
    morphController->setWeight(1, 0.5f);  // 半眨眼
    
    std::cout << "微笑权重：" << morphController->weight(0) << "\n";
    std::cout << "眨眼权重：" << morphController->weight(1) << "\n\n";
    
    // 应用形变
    std::vector<Vector3> positions = {
        Vector3(-1, 1, 0), Vector3(1, 1, 0),
        Vector3(-1, -1, 0), Vector3(1, -1, 0),
        Vector3(-1, 0, 1), Vector3(1, 0, 1),
        Vector3(0, 1, 0), Vector3(0, -1, 0)
    };
    std::vector<Vector3> normals(positions.size(), Vector3(0, 0, 1));
    
    morphController->apply(positions, normals);
    
    std::cout << "应用形变后的顶点位置:\n";
    for (size_t i = 0; i < positions.size(); ++i) {
        std::cout << "  顶点 " << i << ": ";
        printVector3(positions[i]);
    }
    
    // 表情演示
    printSection("表情动画演示");
    
    morphController->resetWeights();
    
    // 添加预定义表情
    morphController->addExpression("Happy", {0}, {1.0f});
    morphController->addExpression("Surprised", {0, 1}, {0.5f, 1.0f});
    
    std::cout << "播放 'Happy' 表情...\n";
    morphController->playExpression("Happy");
    
    // 模拟更新
    for (int i = 0; i < 10; ++i) {
        // 在实际应用中会调用 update
    }
    
    std::cout << "当前微笑权重：" << morphController->weight(0) << "\n";
}

// ============================================================================
// 粒子系统演示
// ============================================================================

void demoParticleSystem() {
    printSection("粒子系统演示");
    
    // 创建粒子系统
    auto particleSystem = std::make_shared<ParticleSystem>();
    particleSystem->initialize(10000, false);  // CPU 模式
    
    // 创建喷泉发射器
    ParticleEmitterConfig fountain;
    fountain.shape = EmitterShape::Circle;
    fountain.position = Vector3(0, 0, 0);
    fountain.direction = Vector3(0, 1, 0);
    fountain.radius = 0.5f;
    fountain.spreadAngle = 0.3f;
    fountain.emissionRate = 100.0f;
    fountain.minSpeed = 5.0f;
    fountain.maxSpeed = 8.0f;
    fountain.minLifetime = 2.0f;
    fountain.maxLifetime = 3.0f;
    fountain.minSize = 0.1f;
    fountain.maxSize = 0.3f;
    fountain.minColor = Color(0.2f, 0.6f, 1.0f, 1.0f);  // 蓝色
    fountain.maxColor = Color(0.8f, 0.9f, 1.0f, 0.5f);  // 淡蓝透明
    
    uint32_t fountainIndex = particleSystem->addEmitter(fountain);
    std::cout << "创建喷泉发射器\n";
    
    // 创建火焰发射器
    ParticleEmitterConfig fire;
    fire.shape = EmitterShape::Cone;
    fire.position = Vector3(5, 0, 0);
    fire.direction = Vector3(0, 1, 0);
    fire.coneAngle = 0.2f;
    fire.coneHeight = 1.0f;
    fire.emissionRate = 200.0f;
    fire.minSpeed = 2.0f;
    fire.maxSpeed = 4.0f;
    fire.minLifetime = 1.0f;
    fire.maxLifetime = 1.5f;
    fire.minSize = 0.3f;
    fire.maxSize = 0.8f;
    fire.minColor = Color(1.0f, 0.5f, 0.0f, 1.0f);  // 橙色
    fire.maxColor = Color(1.0f, 0.2f, 0.0f, 0.0f);  // 红色透明
    
    uint32_t fireIndex = particleSystem->addEmitter(fire);
    std::cout << "创建火焰发射器\n";
    
    // 添加重力
    particleSystem->addForceField(ParticleForceField::gravity(9.81f));
    std::cout << "添加重力场\n\n";
    
    // 启用碰撞
    ParticleCollisionConfig collision;
    collision.enabled = true;
    collision.radius = 0.1f;
    collision.bounce = 0.3f;
    collision.response = ParticleCollisionConfig::Response::Bounce;
    particleSystem->setCollisionConfig(collision);
    std::cout << "启用粒子碰撞\n\n";
    
    // 模拟更新
    std::cout << "模拟粒子系统更新...\n";
    for (int i = 0; i < 100; ++i) {
        particleSystem->update(0.016f);
        
        if (i % 20 == 0) {
            std::cout << "  帧 " << i << ": 活跃粒子数 = " 
                      << particleSystem->activeParticleCount() << "\n";
        }
    }
    
    // 爆发演示
    printSection("粒子爆发演示");
    
    size_t beforeBurst = particleSystem->activeParticleCount();
    std::cout << "爆发前粒子数：" << beforeBurst << "\n";
    
    particleSystem->burst(fireIndex, 500);
    std::cout << "触发火焰爆发 (500 个粒子)\n";
    
    size_t afterBurst = particleSystem->activeParticleCount();
    std::cout << "爆发后粒子数：" << afterBurst << "\n";
    
    // 内存使用
    std::cout << "\n粒子系统内存使用：" << particleSystem->memoryUsage() << " 字节\n";
}

// ============================================================================
// 物理集成演示
// ============================================================================

void demoPhysicsIntegration() {
    printSection("物理集成演示");
    
    // 创建物理世界
    PhysicsWorld physicsWorld;
    
    PhysicsWorldConfig config;
    config.gravity = Vector3(0, -9.81f, 0);
    physicsWorld.initialize(config);
    std::cout << "物理世界初始化完成\n";
    std::cout << "重力：" << physicsWorld.gravity().y << " m/s²\n\n";
    
    // 创建地面
    auto groundShape = CollisionShape::createBox(Vector3(50, 1, 50));
    RigidBodyComponent ground(RigidBodyType::Static, groundShape, 0.0f);
    ground.material.friction = 0.8f;
    uint32_t groundId = physicsWorld.addRigidBody(ground, Vector3(0, -1, 0));
    std::cout << "创建地面 (静态)\n";
    
    // 创建多个球体
    std::vector<uint32_t> ballIds;
    for (int i = 0; i < 5; ++i) {
        auto ballShape = CollisionShape::createSphere(0.5f);
        RigidBodyComponent ball(RigidBodyType::Dynamic, ballShape, 1.0f);
        ball.material.restitution = 0.7f;  // 弹性
        ball.material.friction = 0.3f;
        
        uint32_t id = physicsWorld.addRigidBody(
            ball, 
            Vector3(i * 2 - 4, 5 + i * 2, 0)
        );
        ballIds.push_back(id);
    }
    std::cout << "创建 " << ballIds.size() << " 个球体 (动态)\n\n";
    
    // 模拟物理
    std::cout << "物理模拟:\n";
    for (int frame = 0; frame < 60; ++frame) {
        physicsWorld.update(1.0f / 60.0f);
        
        if (frame % 20 == 0) {
            std::cout << "  帧 " << frame << ":\n";
            for (size_t i = 0; i < ballIds.size(); ++i) {
                Vector3 pos = physicsWorld.getPosition(ballIds[i]);
                Vector3 vel = physicsWorld.getLinearVelocity(ballIds[i]);
                std::cout << "    球 " << i << ": 位置=(" 
                          << pos.x << ", " << pos.y << ", " << pos.z 
                          << "), 速度=(" 
                          << vel.x << ", " << vel.y << ", " << vel.z << ")\n";
            }
        }
    }
    
    // 射线检测演示
    printSection("射线检测演示");
    
    std::vector<RayHit> hits;
    bool hit = physicsWorld.raycast(
        Vector3(0, 10, 0),
        Vector3(0, -10, 0),
        hits
    );
    
    if (hit) {
        std::cout << "射线检测命中 " << hits.size() << " 个物体\n";
        for (size_t i = 0; i < hits.size(); ++i) {
            std::cout << "  命中 " << i << ": 位置=(" 
                      << hits[i].position.x << ", " 
                      << hits[i].position.y << ", " 
                      << hits[i].position.z 
                      << "), 距离=" << hits[i].distance << "\n";
        }
    } else {
        std::cout << "射线检测未命中\n";
    }
    
    // 施加力演示
    printSection("施加力演示");
    
    Vector3 impulse(0, 20, 0);
    physicsWorld.applyImpulse(ballIds[0], impulse);
    std::cout << "对球 0 施加冲量：(0, 20, 0)\n";
    
    Vector3 newVel = physicsWorld.getLinearVelocity(ballIds[0]);
    std::cout << "新速度：(";
    printVector3(newVel);
}

// ============================================================================
// 综合演示
// ============================================================================

void demoIntegratedSystem() {
    printSection("综合系统演示");
    
    // 创建场景
    auto skeleton = std::make_shared<Skeleton>();
    uint32_t root = skeleton->addBone("Character");
    
    Animator animator;
    animator.setSkeleton(skeleton);
    
    // 创建动画
    auto idleClip = std::make_shared<AnimationClip>("Idle");
    idleClip->duration = 2.0f;
    animator.addClip(idleClip);
    
    // 创建粒子系统
    auto particles = std::make_shared<ParticleSystem>();
    particles->initialize(1000);
    
    ParticleEmitterConfig aura;
    aura.shape = EmitterShape::Sphere;
    aura.radius = 1.0f;
    aura.emissionRate = 50.0f;
    aura.minSpeed = 0.5f;
    aura.maxLifetime = 2.0f;
    aura.minColor = Color(1.0f, 1.0f, 0.5f, 0.5f);
    particles->addEmitter(aura);
    
    // 创建物理世界
    PhysicsWorld physics;
    physics.initialize();
    
    std::cout << "创建综合场景:\n";
    std::cout << "  - 角色骨骼：" << skeleton->boneCount() << " 根骨骼\n";
    std::cout << "  - 动画剪辑：" << animator.clipCount() << " 个\n";
    std::cout << "  - 粒子发射器：" << particles->emitterCount() << " 个\n";
    std::cout << "  - 物理世界：已初始化\n\n";
    
    // 模拟游戏循环
    std::cout << "运行游戏循环 (60 帧)...\n";
    
    float deltaTime = 1.0f / 60.0f;
    
    for (int frame = 0; frame < 60; ++frame) {
        // 更新动画
        animator.update(deltaTime);
        
        // 更新粒子
        particles->update(deltaTime);
        
        // 更新物理
        physics.update(deltaTime);
        
        if (frame % 20 == 0) {
            std::cout << "  帧 " << frame << ": "
                      << "动画时间=" << animator.currentTime() << "s, "
                      << "粒子数=" << particles->activeParticleCount() << ", "
                      << "物理对象=" << physics.rigidBodyCount() << "\n";
        }
    }
    
    std::cout << "\n综合演示完成!\n";
}

// ============================================================================
// 主函数
// ============================================================================

int main(int argc, char* argv[]) {
    std::cout << "==============================================\n";
    std::cout << "  Phoenix Engine Phase 3 - 动画系统演示\n";
    std::cout << "==============================================\n";
    
    try {
        // 运行各个演示
        demoSkeletonAnimation();
        demoMorphAnimation();
        demoParticleSystem();
        demoPhysicsIntegration();
        demoIntegratedSystem();
        
        std::cout << "\n==============================================\n";
        std::cout << "  所有演示完成!\n";
        std::cout << "==============================================\n";
        
    } catch (const std::exception& e) {
        std::cerr << "错误：" << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
