#include <gtest/gtest.h>
#include "phoenix/scene/physics.hpp"
#include "phoenix/math/vector3.hpp"
#include "phoenix/math/quaternion.hpp"
#include <memory>

using namespace phoenix;
using namespace phoenix::scene;
using namespace phoenix::math;

// ============================================================================
// 碰撞形状测试
// ============================================================================

TEST(CollisionShapeTest, CreateSphere) {
    auto shape = CollisionShape::createSphere(2.5f);
    
    EXPECT_EQ(shape->type, CollisionShapeType::Sphere);
    EXPECT_FLOAT_EQ(shape->radius, 2.5f);
}

TEST(CollisionShapeTest, CreateBox) {
    Vector3 halfExtents(1.0f, 2.0f, 3.0f);
    auto shape = CollisionShape::createBox(halfExtents);
    
    EXPECT_EQ(shape->type, CollisionShapeType::Box);
    EXPECT_FLOAT_EQ(shape->halfExtents.x, 1.0f);
    EXPECT_FLOAT_EQ(shape->halfExtents.y, 2.0f);
    EXPECT_FLOAT_EQ(shape->halfExtents.z, 3.0f);
}

TEST(CollisionShapeTest, CreateCapsule) {
    auto shape = CollisionShape::createCapsule(0.5f, 2.0f);
    
    EXPECT_EQ(shape->type, CollisionShapeType::Capsule);
    EXPECT_FLOAT_EQ(shape->radius, 0.5f);
    EXPECT_FLOAT_EQ(shape->height, 2.0f);
}

TEST(CollisionShapeTest, CreateCylinder) {
    auto shape = CollisionShape::createCylinder(1.0f, 3.0f);
    
    EXPECT_EQ(shape->type, CollisionShapeType::Cylinder);
    EXPECT_FLOAT_EQ(shape->radius, 1.0f);
    EXPECT_FLOAT_EQ(shape->height, 3.0f);
}

TEST(CollisionShapeTest, CreateConvexHull) {
    std::vector<Vector3> vertices = {
        Vector3(-1, -1, -1),
        Vector3(1, -1, -1),
        Vector3(1, 1, -1),
        Vector3(-1, 1, -1),
        Vector3(-1, -1, 1),
        Vector3(1, -1, 1),
        Vector3(1, 1, 1),
        Vector3(-1, 1, 1)
    };
    
    auto shape = CollisionShape::createConvexHull(vertices);
    
    EXPECT_EQ(shape->type, CollisionShapeType::ConvexHull);
    EXPECT_EQ(shape->vertices.size(), 8u);
}

TEST(CollisionShapeTest, CreateMesh) {
    std::vector<Vector3> vertices = {
        Vector3(0, 0, 0),
        Vector3(1, 0, 0),
        Vector3(0, 1, 0)
    };
    std::vector<uint32_t> indices = {0, 1, 2};
    
    auto shape = CollisionShape::createMesh(vertices, indices);
    
    EXPECT_EQ(shape->type, CollisionShapeType::Mesh);
    EXPECT_EQ(shape->vertices.size(), 3u);
    EXPECT_EQ(shape->indices.size(), 3u);
}

// ============================================================================
// 物理材质测试
// ============================================================================

TEST(PhysicsMaterialTest, DefaultConstruction) {
    PhysicsMaterial mat;
    
    EXPECT_FLOAT_EQ(mat.friction, 0.5f);
    EXPECT_FLOAT_EQ(mat.restitution, 0.0f);
    EXPECT_FLOAT_EQ(mat.density, 1.0f);
    EXPECT_FLOAT_EQ(mat.linearDamping, 0.0f);
    EXPECT_FLOAT_EQ(mat.angularDamping, 0.0f);
}

TEST(PhysicsMaterialTest, CustomConstruction) {
    PhysicsMaterial mat(0.8f, 0.5f, 2.0f);
    
    EXPECT_FLOAT_EQ(mat.friction, 0.8f);
    EXPECT_FLOAT_EQ(mat.restitution, 0.5f);
    EXPECT_FLOAT_EQ(mat.density, 2.0f);
}

TEST(PhysicsMaterialTest, PresetMaterials) {
    auto metal = PhysicsMaterial::metal();
    EXPECT_NEAR(metal.friction, 0.3f, 0.01f);
    EXPECT_NEAR(metal.density, 7.8f, 0.1f);
    
    auto wood = PhysicsMaterial::wood();
    EXPECT_NEAR(wood.friction, 0.5f, 0.01f);
    EXPECT_NEAR(wood.density, 0.7f, 0.1f);
    
    auto rubber = PhysicsMaterial::rubber();
    EXPECT_NEAR(rubber.friction, 0.8f, 0.01f);
    EXPECT_NEAR(rubber.restitution, 0.7f, 0.01f);
    
    auto ice = PhysicsMaterial::ice();
    EXPECT_NEAR(ice.friction, 0.05f, 0.01f);
}

// ============================================================================
// 刚体组件测试
// ============================================================================

TEST(RigidBodyComponentTest, DefaultConstruction) {
    RigidBodyComponent body;
    
    EXPECT_EQ(body.bodyType, RigidBodyType::Dynamic);
    EXPECT_FLOAT_EQ(body.mass, 1.0f);
    EXPECT_EQ(body.isTrigger, false);
    EXPECT_EQ(body.collisionGroup, 1u);
    EXPECT_EQ(body.collisionMask, 0xFFFFFFFFu);
}

TEST(RigidBodyComponentTest, CustomConstruction) {
    auto shape = CollisionShape::createSphere(1.0f);
    RigidBodyComponent body(RigidBodyType::Static, shape, 0.0f);
    
    EXPECT_EQ(body.bodyType, RigidBodyType::Static);
    EXPECT_FLOAT_EQ(body.mass, 0.0f);
    EXPECT_NE(body.collisionShape, nullptr);
}

TEST(RigidBodyComponentTest, CCDSettings) {
    RigidBodyComponent body;
    
    body.enableCCD = true;
    body.ccdMotionThreshold = 0.1f;
    body.ccdSweptSphereRadius = 0.05f;
    
    EXPECT_TRUE(body.enableCCD);
    EXPECT_FLOAT_EQ(body.ccdMotionThreshold, 0.1f);
    EXPECT_FLOAT_EQ(body.ccdSweptSphereRadius, 0.05f);
}

// ============================================================================
// 物理世界测试
// ============================================================================

TEST(PhysicsWorldTest, CreateAndDestroy) {
    PhysicsWorld world;
    // 简单的创建和销毁测试
}

TEST(PhysicsWorldTest, Initialize) {
    PhysicsWorld world;
    
    PhysicsWorldConfig config;
    config.gravity = Vector3(0, -9.81f, 0);
    config.maxObjects = 1000;
    
    bool success = world.initialize(config);
    
    EXPECT_TRUE(success);
    EXPECT_TRUE(world.isInitialized());
}

TEST(PhysicsWorldTest, Shutdown) {
    PhysicsWorld world;
    world.initialize();
    
    world.shutdown();
    
    EXPECT_FALSE(world.isInitialized());
}

TEST(PhysicsWorldTest, SetGravity) {
    PhysicsWorld world;
    world.initialize();
    
    Vector3 newGravity(0, -20.0f, 0);
    world.setGravity(newGravity);
    
    Vector3 gravity = world.gravity();
    EXPECT_FLOAT_EQ(gravity.y, -20.0f);
}

TEST(PhysicsWorldTest, AddRigidBody) {
    PhysicsWorld world;
    world.initialize();
    
    auto shape = CollisionShape::createSphere(1.0f);
    RigidBodyComponent body(RigidBodyType::Dynamic, shape, 1.0f);
    
    uint32_t id = world.addRigidBody(body, Vector3(0, 10, 0), Quaternion::identity());
    
    EXPECT_NE(id, UINT32_MAX);
    EXPECT_EQ(world.rigidBodyCount(), 1u);
}

TEST(PhysicsWorldTest, RemoveRigidBody) {
    PhysicsWorld world;
    world.initialize();
    
    auto shape = CollisionShape::createSphere(1.0f);
    RigidBodyComponent body(RigidBodyType::Dynamic, shape, 1.0f);
    
    uint32_t id = world.addRigidBody(body);
    EXPECT_EQ(world.rigidBodyCount(), 1u);
    
    world.removeRigidBody(id);
    EXPECT_EQ(world.rigidBodyCount(), 0u);
}

TEST(PhysicsWorldTest, SetTransform) {
    PhysicsWorld world;
    world.initialize();
    
    auto shape = CollisionShape::createSphere(1.0f);
    RigidBodyComponent body(RigidBodyType::Static, shape, 0.0f);
    
    uint32_t id = world.addRigidBody(body);
    
    Vector3 newPos(5.0f, 10.0f, 15.0f);
    Quaternion newRot = Quaternion::fromAxisAngle(Vector3::unitY(), 1.57f);
    
    world.setTransform(id, newPos, newRot);
    
    Vector3 pos = world.getPosition(id);
    EXPECT_FLOAT_EQ(pos.x, 5.0f);
    EXPECT_FLOAT_EQ(pos.y, 10.0f);
    EXPECT_FLOAT_EQ(pos.z, 15.0f);
}

TEST(PhysicsWorldTest, ApplyForce) {
    PhysicsWorld world;
    world.initialize();
    
    auto shape = CollisionShape::createSphere(1.0f);
    RigidBodyComponent body(RigidBodyType::Dynamic, shape, 1.0f);
    
    uint32_t id = world.addRigidBody(body);
    
    Vector3 force(0, 100.0f, 0);
    world.applyForce(id, force);
    
    // 力应该被应用（具体效果取决于物理引擎）
}

TEST(PhysicsWorldTest, ApplyImpulse) {
    PhysicsWorld world;
    world.initialize();
    
    auto shape = CollisionShape::createSphere(1.0f);
    RigidBodyComponent body(RigidBodyType::Dynamic, shape, 1.0f);
    
    uint32_t id = world.addRigidBody(body);
    
    Vector3 impulse(0, 10.0f, 0);
    world.applyImpulse(id, impulse);
}

TEST(PhysicsWorldTest, SetVelocity) {
    PhysicsWorld world;
    world.initialize();
    
    auto shape = CollisionShape::createSphere(1.0f);
    RigidBodyComponent body(RigidBodyType::Dynamic, shape, 1.0f);
    
    uint32_t id = world.addRigidBody(body);
    
    Vector3 velocity(5.0f, 0, 0);
    world.setLinearVelocity(id, velocity);
    
    Vector3 result = world.getLinearVelocity(id);
    EXPECT_FLOAT_EQ(result.x, 5.0f);
}

TEST(PhysicsWorldTest, Update) {
    PhysicsWorld world;
    world.initialize();
    
    auto shape = CollisionShape::createSphere(1.0f);
    RigidBodyComponent body(RigidBodyType::Dynamic, shape, 1.0f);
    
    uint32_t id = world.addRigidBody(body, Vector3(0, 10, 0));
    
    // 更新物理
    world.update(0.016f);
    
    // 物体应该因重力下落
    Vector3 pos = world.getPosition(id);
    EXPECT_LT(pos.y, 10.0f);
}

TEST(PhysicsWorldTest, MultipleRigidBodies) {
    PhysicsWorld world;
    world.initialize();
    
    for (int i = 0; i < 10; ++i) {
        auto shape = CollisionShape::createSphere(1.0f);
        RigidBodyComponent body(RigidBodyType::Dynamic, shape, 1.0f);
        world.addRigidBody(body, Vector3(i * 2.0f, 10.0f, 0));
    }
    
    EXPECT_EQ(world.rigidBodyCount(), 10u);
}

TEST(PhysicsWorldTest, StaticVsDynamic) {
    PhysicsWorld world;
    world.initialize();
    
    // 静态物体
    auto staticShape = CollisionShape::createBox(Vector3(10, 1, 10));
    RigidBodyComponent staticBody(RigidBodyType::Static, staticShape, 0.0f);
    uint32_t staticId = world.addRigidBody(staticBody, Vector3(0, 0, 0));
    
    // 动态物体
    auto dynamicShape = CollisionShape::createSphere(1.0f);
    RigidBodyComponent dynamicBody(RigidBodyType::Dynamic, dynamicShape, 1.0f);
    uint32_t dynamicId = world.addRigidBody(dynamicBody, Vector3(0, 5, 0));
    
    // 更新
    world.update(0.016f);
    
    // 静态物体不应该移动
    Vector3 staticPos = world.getPosition(staticId);
    EXPECT_FLOAT_EQ(staticPos.y, 0.0f);
    
    // 动态物体应该下落
    Vector3 dynamicPos = world.getPosition(dynamicId);
    EXPECT_LT(dynamicPos.y, 5.0f);
}

// ============================================================================
// 射线检测测试
// ============================================================================

TEST(RaycastTest, RaycastEmptyWorld) {
    PhysicsWorld world;
    world.initialize();
    
    std::vector<RayHit> hits;
    bool result = world.raycast(Vector3(0, 10, 0), Vector3(0, 0, 0), hits);
    
    EXPECT_FALSE(result);
    EXPECT_TRUE(hits.empty());
}

TEST(RaycastTest, RaycastWithObject) {
    PhysicsWorld world;
    world.initialize();
    
    // 添加一个球体
    auto shape = CollisionShape::createSphere(1.0f);
    RigidBodyComponent body(RigidBodyType::Static, shape, 0.0f);
    world.addRigidBody(body, Vector3(0, 5, 0));
    
    // 射线检测
    std::vector<RayHit> hits;
    bool result = world.raycast(Vector3(0, 10, 0), Vector3(0, 0, 0), hits);
    
    // 应该命中
    EXPECT_TRUE(result);
    EXPECT_FALSE(hits.empty());
}

TEST(RaycastTest, RaycastClosest) {
    PhysicsWorld world;
    world.initialize();
    
    // 添加多个物体
    for (int i = 0; i < 3; ++i) {
        auto shape = CollisionShape::createSphere(0.5f);
        RigidBodyComponent body(RigidBodyType::Static, shape, 0.0f);
        world.addRigidBody(body, Vector3(0, 8 - i * 2, 0));
    }
    
    RayHit hit;
    bool result = world.raycastClosest(Vector3(0, 10, 0), Vector3(0, 0, 0), hit);
    
    EXPECT_TRUE(result);
    // 应该命中最近的物体
    EXPECT_LE(hit.distance, 1.0f);
}

// ============================================================================
// 碰撞事件测试
// ============================================================================

TEST(CollisionEventTest, EventConstruction) {
    Vector3 point(1, 2, 3);
    Vector3 normal(0, 1, 0);
    
    CollisionEvent event(1, 2, point, normal, 5.0f, true);
    
    EXPECT_EQ(event.objectA, 1u);
    EXPECT_EQ(event.objectB, 2u);
    EXPECT_FLOAT_EQ(event.contactPoint.x, 1.0f);
    EXPECT_FLOAT_EQ(event.contactNormal.y, 1.0f);
    EXPECT_FLOAT_EQ(event.impulse, 5.0f);
    EXPECT_TRUE(event.isEnter);
}

TEST(PhysicsWorldTest, CollisionCallback) {
    PhysicsWorld world;
    world.initialize();
    
    bool callbackCalled = false;
    
    world.setCollisionCallback([&callbackCalled](const CollisionEvent& event) {
        callbackCalled = true;
    });
    
    // 添加两个可能碰撞的物体
    auto shape1 = CollisionShape::createSphere(1.0f);
    RigidBodyComponent body1(RigidBodyType::Dynamic, shape1, 1.0f);
    world.addRigidBody(body1, Vector3(0, 2, 0));
    
    auto shape2 = CollisionShape::createSphere(1.0f);
    RigidBodyComponent body2(RigidBodyType::Dynamic, shape2, 1.0f);
    world.addRigidBody(body2, Vector3(0, 1, 0));
    
    // 更新
    world.update(0.016f);
    
    // 回调可能被调用（取决于碰撞）
}

// ============================================================================
// 性能测试
// ============================================================================

TEST(PhysicsPerformanceTest, ManyRigidBodies) {
    PhysicsWorld world;
    world.initialize();
    
    const int count = 100;
    
    for (int i = 0; i < count; ++i) {
        auto shape = CollisionShape::createSphere(0.5f);
        RigidBodyComponent body(RigidBodyType::Dynamic, shape, 1.0f);
        world.addRigidBody(body, Vector3(i * 1.5f, 10.0f, 0));
    }
    
    EXPECT_EQ(world.rigidBodyCount(), count);
    
    // 更新应该能处理
    world.update(0.016f);
}

TEST(PhysicsPerformanceTest, ManyRaycasts) {
    PhysicsWorld world;
    world.initialize();
    
    // 添加一些物体
    for (int i = 0; i < 10; ++i) {
        auto shape = CollisionShape::createBox(Vector3(1, 1, 1));
        RigidBodyComponent body(RigidBodyType::Static, shape, 0.0f);
        world.addRigidBody(body, Vector3(i * 3, 0, 0));
    }
    
    // 多次射线检测
    for (int i = 0; i < 100; ++i) {
        std::vector<RayHit> hits;
        world.raycast(Vector3(0, 5, 0), Vector3(0, -5, 0), hits);
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
