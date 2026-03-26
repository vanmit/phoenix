/**
 * @file test_ecs.cpp
 * @brief Phoenix Engine ECS Tests
 * 
 * Test suite for Entity Component System implementation.
 */

#include <gtest/gtest.h>
#include <memory>

#include "../../../include/phoenix/scene/ecs.hpp"
#include "../../../include/phoenix/scene/transform.hpp"

using namespace phoenix;
using namespace phoenix::scene;

// ============================================================================
// Test Components
// ============================================================================

struct TestComponent {
    int value;
    float floatValue;
    
    TestComponent() : value(0), floatValue(0.0f) {}
    TestComponent(int v, float f) : value(v), floatValue(f) {}
    
    bool operator==(const TestComponent& other) const {
        return value == other.value && floatValue == other.floatValue;
    }
};

struct VelocityComponent {
    float vx, vy, vz;
    
    VelocityComponent() : vx(0), vy(0), vz(0) {}
    VelocityComponent(float x, float y, float z) : vx(x), vy(y), vz(z) {}
};

// ============================================================================
// Test System
// ============================================================================

class TestSystem : public System {
public:
    int updateCount = 0;
    float lastDeltaTime = 0;
    
    void update(float deltaTime) override {
        ++updateCount;
        lastDeltaTime = deltaTime;
    }
};

// ============================================================================
// Entity Tests
// ============================================================================

/**
 * @test Test entity construction
 */
TEST(EntityTest, Construction) {
    Entity e;
    EXPECT_FALSE(e.isValid());
    EXPECT_EQ(e.id(), 0);
    
    Entity e2(42);
    EXPECT_TRUE(e2.isValid());
    EXPECT_EQ(e2.id(), 42);
    EXPECT_EQ(e2.index(), 42);
    EXPECT_EQ(e2.generation(), 0);
}

/**
 * @test Test entity equality
 */
TEST(EntityTest, Equality) {
    Entity e1(42);
    Entity e2(42);
    Entity e3(43);
    
    EXPECT_EQ(e1, e2);
    EXPECT_NE(e1, e3);
    EXPECT_LT(e1, e3);
}

/**
 * @test Test entity invalid
 */
TEST(EntityTest, Invalid) {
    Entity invalid = Entity::invalid();
    EXPECT_FALSE(invalid.isValid());
    EXPECT_EQ(invalid.id(), 0);
}

// ============================================================================
// EntityManager Tests
// ============================================================================

/**
 * @test Test entity creation
 */
TEST(EntityManagerTest, Creation) {
    EntityManager em;
    
    Entity e1 = em.create();
    Entity e2 = em.create();
    
    EXPECT_TRUE(e1.isValid());
    EXPECT_TRUE(e2.isValid());
    EXPECT_NE(e1, e2);
    EXPECT_EQ(em.getAliveCount(), 2);
}

/**
 * @test Test entity destruction
 */
TEST(EntityManagerTest, Destruction) {
    EntityManager em;
    
    Entity e1 = em.create();
    Entity e2 = em.create();
    
    em.destroy(e1);
    
    EXPECT_FALSE(em.isAlive(e1));
    EXPECT_TRUE(em.isAlive(e2));
    EXPECT_EQ(em.getAliveCount(), 1);
}

/**
 * @test Test entity recycling
 */
TEST(EntityManagerTest, Recycling) {
    EntityManager em;
    
    Entity e1 = em.create();
    em.destroy(e1);
    
    Entity e2 = em.create();
    
    // Should reuse the same index
    EXPECT_EQ(e1.index(), e2.index());
    EXPECT_NE(e1.generation(), e2.generation());
}

/**
 * @test Test stale entity detection
 */
TEST(EntityManagerTest, StaleDetection) {
    EntityManager em;
    
    Entity e1 = em.create();
    em.destroy(e1);
    
    Entity e2 = em.create(); // Reuses index
    
    // Old handle should be invalid
    EXPECT_FALSE(em.isAlive(e1));
    EXPECT_TRUE(em.isAlive(e2));
}

// ============================================================================
// ComponentManager Tests
// ============================================================================

/**
 * @test Test component registration
 */
TEST(ComponentManagerTest, Registration) {
    ComponentManager cm;
    cm.registerComponent<TestComponent>();
    cm.registerComponent<VelocityComponent>();
}

/**
 * @test Test component add and get
 */
TEST(ComponentManagerTest, AddAndGet) {
    EntityManager em;
    ComponentManager cm;
    
    cm.registerComponent<TestComponent>();
    
    Entity e = em.create();
    TestComponent comp(42, 3.14f);
    
    cm.addComponent<TestComponent>(e, comp);
    
    const auto& retrieved = cm.getComponent<TestComponent>(e);
    EXPECT_EQ(retrieved.value, 42);
    EXPECT_FLOAT_EQ(retrieved.floatValue, 3.14f);
}

/**
 * @test Test component has
 */
TEST(ComponentManagerTest, HasComponent) {
    EntityManager em;
    ComponentManager cm;
    
    cm.registerComponent<TestComponent>();
    
    Entity e1 = em.create();
    Entity e2 = em.create();
    
    cm.addComponent<TestComponent>(e1, TestComponent());
    
    EXPECT_TRUE(cm.hasComponent<TestComponent>(e1));
    EXPECT_FALSE(cm.hasComponent<TestComponent>(e2));
}

/**
 * @test Test component removal
 */
TEST(ComponentManagerTest, Removal) {
    EntityManager em;
    ComponentManager cm;
    
    cm.registerComponent<TestComponent>();
    
    Entity e = em.create();
    cm.addComponent<TestComponent>(e, TestComponent());
    
    EXPECT_TRUE(cm.hasComponent<TestComponent>(e));
    
    cm.removeComponent<TestComponent>(e);
    
    EXPECT_FALSE(cm.hasComponent<TestComponent>(e));
}

/**
 * @test Test entity destruction cleans components
 */
TEST(ComponentManagerTest, EntityDestructionCleanup) {
    EntityManager em;
    ComponentManager cm;
    
    cm.registerComponent<TestComponent>();
    
    Entity e = em.create();
    cm.addComponent<TestComponent>(e, TestComponent());
    
    cm.entityDestroyed(e);
    
    EXPECT_FALSE(cm.hasComponent<TestComponent>(e));
}

// ============================================================================
// ECSWorld Tests
// ============================================================================

/**
 * @test Test world creation
 */
TEST(ECSWorldTest, Creation) {
    ECSWorld world;
    EXPECT_EQ(world.getEntityCount(), 0);
}

/**
 * @test Test entity operations
 */
TEST(ECSWorldTest, EntityOperations) {
    ECSWorld world;
    
    Entity e1 = world.createEntity();
    Entity e2 = world.createEntity();
    
    EXPECT_TRUE(world.isEntityAlive(e1));
    EXPECT_TRUE(world.isEntityAlive(e2));
    EXPECT_EQ(world.getEntityCount(), 2);
    
    world.destroyEntity(e1);
    
    EXPECT_FALSE(world.isEntityAlive(e1));
    EXPECT_TRUE(world.isEntityAlive(e2));
    EXPECT_EQ(world.getEntityCount(), 1);
}

/**
 * @test Test component operations
 */
TEST(ECSWorldTest, ComponentOperations) {
    ECSWorld world;
    world.registerComponent<TestComponent>();
    world.registerComponent<VelocityComponent>();
    
    Entity e = world.createEntity();
    
    world.addComponent<TestComponent>(e, TestComponent(10, 2.5f));
    world.addComponent<VelocityComponent>(e, VelocityComponent(1, 2, 3));
    
    EXPECT_TRUE(world.hasComponent<TestComponent>(e));
    EXPECT_TRUE(world.hasComponent<VelocityComponent>(e));
    
    const auto& tc = world.getComponent<TestComponent>(e);
    EXPECT_EQ(tc.value, 10);
    EXPECT_FLOAT_EQ(tc.floatValue, 2.5f);
    
    const auto& vc = world.getComponent<VelocityComponent>(e);
    EXPECT_FLOAT_EQ(vc.vx, 1);
    EXPECT_FLOAT_EQ(vc.vy, 2);
    EXPECT_FLOAT_EQ(vc.vz, 3);
}

/**
 * @test Test system registration and update
 */
TEST(ECSWorldTest, SystemUpdate) {
    ECSWorld world;
    
    auto& system = world.registerSystem<TestSystem>();
    
    world.update(0.016f);
    
    EXPECT_EQ(system.updateCount, 1);
    EXPECT_FLOAT_EQ(system.lastDeltaTime, 0.016f);
    
    world.update(0.032f);
    
    EXPECT_EQ(system.updateCount, 2);
    EXPECT_FLOAT_EQ(system.lastDeltaTime, 0.032f);
}

/**
 * @test Test query by signature
 */
TEST(ECSWorldTest, QueryBySignature) {
    ECSWorld world;
    world.registerComponent<TestComponent>();
    world.registerComponent<VelocityComponent>();
    
    // Create entities with different components
    Entity e1 = world.createEntity();
    Entity e2 = world.createEntity();
    Entity e3 = world.createEntity();
    
    world.addComponent<TestComponent>(e1, TestComponent());
    world.addComponent<TestComponent>(e2, TestComponent());
    world.addComponent<VelocityComponent>(e2, VelocityComponent());
    world.addComponent<VelocityComponent>(e3, VelocityComponent());
    
    // Query entities with TestComponent
    ComponentSignature testSig;
    testSig.set(world.getComponentType<TestComponent>());
    
    auto results = world.getEntitiesWithSignature(testSig);
    
    EXPECT_EQ(results.size(), 2);
}

/**
 * @test Test event subscription
 */
TEST(ECSWorldTest, EventSubscription) {
    ECSWorld world;
    world.registerComponent<TestComponent>();
    
    int createCount = 0;
    int destroyCount = 0;
    
    world.subscribe(EventType::EntityCreated, [&createCount](const Event&) {
        ++createCount;
    });
    
    world.subscribe(EventType::EntityDestroyed, [&destroyCount](const Event&) {
        ++destroyCount;
    });
    
    Entity e = world.createEntity();
    EXPECT_EQ(createCount, 1);
    
    world.destroyEntity(e);
    EXPECT_EQ(destroyCount, 1);
}

/**
 * @test Test component events
 */
TEST(ECSWorldTest, ComponentEvents) {
    ECSWorld world;
    world.registerComponent<TestComponent>();
    
    int addCount = 0;
    int removeCount = 0;
    
    world.subscribe(EventType::ComponentAdded, [&addCount](const Event&) {
        ++addCount;
    });
    
    world.subscribe(EventType::ComponentRemoved, [&removeCount](const Event&) {
        ++removeCount;
    });
    
    Entity e = world.createEntity();
    world.addComponent<TestComponent>(e, TestComponent());
    
    EXPECT_EQ(addCount, 1);
    
    world.removeComponent<TestComponent>(e);
    
    EXPECT_EQ(removeCount, 1);
}

// ============================================================================
// Performance Tests
// ============================================================================

/**
 * @test Performance test for entity creation
 */
TEST(ECSPerformanceTest, EntityCreation) {
    ECSWorld world;
    
    const int count = 10000;
    std::vector<Entity> entities;
    entities.reserve(count);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < count; ++i) {
        entities.push_back(world.createEntity());
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    EXPECT_EQ(world.getEntityCount(), count);
    EXPECT_LT(duration, 100); // Should complete in < 100ms
}

/**
 * @test Performance test for component access
 */
TEST(ECSPerformanceTest, ComponentAccess) {
    ECSWorld world;
    world.registerComponent<TransformComponent>();
    
    const int count = 1000;
    std::vector<Entity> entities;
    
    for (int i = 0; i < count; ++i) {
        Entity e = world.createEntity();
        world.addComponent<TransformComponent>(e, TransformComponent());
        entities.push_back(e);
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Access all components
    for (int iter = 0; iter < 100; ++iter) {
        for (const auto& e : entities) {
            volatile auto& comp = world.getComponent<TransformComponent>(e);
            (void)comp;
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    EXPECT_LT(duration, 500); // Should complete in < 500ms
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
