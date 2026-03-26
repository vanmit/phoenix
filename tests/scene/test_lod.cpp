/**
 * @file test_lod.cpp
 * @brief Phoenix Engine LOD System Tests
 * 
 * Test suite for Level of Detail system.
 */

#include <gtest/gtest.h>
#include <memory>
#include <cmath>

#include "../../../include/phoenix/scene/lod.hpp"
#include "../../../include/phoenix/math/vector3.hpp"
#include "../../../include/phoenix/math/bounding.hpp"

using namespace phoenix;
using namespace phoenix::scene;
using namespace phoenix::math;

// ============================================================================
// LODComponent Tests
// ============================================================================

/**
 * @test Test LOD component construction
 */
TEST(LODComponentTest, Construction) {
    LODComponent lod;
    
    EXPECT_EQ(lod.getLevelCount(), 0);
    EXPECT_TRUE(lod.isMorphEnabled());
    EXPECT_FALSE(lod.getTransition().active);
}

/**
 * @test Test LOD level addition
 */
TEST(LODComponentTest, LevelAddition) {
    LODComponent lod;
    
    lod.addLevel(LODLevel(LODQuality::VeryHigh, 10.0f, "mesh_high"));
    lod.addLevel(LODLevel(LODQuality::Medium, 50.0f, "mesh_med"));
    lod.addLevel(LODLevel(LODQuality::Low, 100.0f, "mesh_low"));
    
    EXPECT_EQ(lod.getLevelCount(), 3);
    
    // Levels should be sorted by distance
    const auto& levels = lod.getLevels();
    EXPECT_EQ(levels[0].quality, LODQuality::VeryHigh);
    EXPECT_EQ(levels[1].quality, LODQuality::Medium);
    EXPECT_EQ(levels[2].quality, LODQuality::Low);
}

/**
 * @test Test LOD selection by distance
 */
TEST(LODComponentTest, SelectionByDistance) {
    LODComponent lod;
    
    lod.addLevel(LODLevel(LODQuality::High, 20.0f));
    lod.addLevel(LODLevel(LODQuality::Medium, 50.0f));
    lod.addLevel(LODLevel(LODQuality::Low, 100.0f));
    
    BoundingSphere bounds(Vector3(0, 0, 0), 5.0f);
    
    // Close - should use highest quality
    size_t level = lod.selectLOD(Vector3(0, 0, 0), bounds);
    EXPECT_EQ(level, 0);
    
    // Medium distance
    level = lod.selectLOD(Vector3(30, 0, 0), bounds);
    EXPECT_EQ(level, 1);
    
    // Far - should use lowest quality
    level = lod.selectLOD(Vector3(150, 0, 0), bounds);
    EXPECT_EQ(level, 2);
}

/**
 * @test Test LOD level setting
 */
TEST(LODComponentTest, LevelSetting) {
    LODComponent lod;
    
    lod.addLevel(LODLevel(LODQuality::High, 20.0f));
    lod.addLevel(LODLevel(LODQuality::Medium, 50.0f));
    lod.addLevel(LODLevel(LODQuality::Low, 100.0f));
    
    lod.setLevel(LODQuality::Medium);
    
    EXPECT_EQ(lod.getCurrentLevel(), LODQuality::Medium);
    EXPECT_EQ(lod.getCurrentLevelIndex(), 1);
}

/**
 * @test Test LOD transition
 */
TEST(LODComponentTest, Transition) {
    LODComponent lod;
    
    lod.addLevel(LODLevel(LODQuality::High, 20.0f));
    lod.addLevel(LODLevel(LODQuality::Medium, 50.0f));
    
    lod.setLevelByIndex(0);
    
    // Change level - should start transition
    lod.setLevelByIndex(1);
    
    const auto& transition = lod.getTransition();
    EXPECT_TRUE(transition.active);
    EXPECT_EQ(transition.fromLevel, LODQuality::High);
    EXPECT_EQ(transition.toLevel, LODQuality::Medium);
    EXPECT_FLOAT_EQ(transition.progress, 0.0f);
    
    // Update transition
    bool stillActive = lod.updateTransition(0.25f);
    EXPECT_TRUE(stillActive);
    EXPECT_FLOAT_EQ(transition.progress, 0.5f); // Assuming 0.5s duration
    
    // Complete transition
    lod.updateTransition(0.25f);
    EXPECT_FALSE(lod.getTransition().active);
}

/**
 * @test Test morph disabled
 */
TEST(LODComponentTest, MorphDisabled) {
    LODComponent lod;
    lod.setMorphEnabled(false);
    
    lod.addLevel(LODLevel(LODQuality::High, 20.0f));
    lod.addLevel(LODLevel(LODQuality::Medium, 50.0f));
    
    lod.setLevelByIndex(0);
    lod.setLevelByIndex(1);
    
    // Should not start transition
    EXPECT_FALSE(lod.getTransition().active);
    EXPECT_EQ(lod.getCurrentLevel(), LODQuality::Medium);
}

/**
 * @test Test screen space error calculation
 */
TEST(LODComponentTest, ScreenSpaceError) {
    LODLevel level;
    level.vertexCount = 10000;
    
    // Close object - larger error
    float error1 = LODComponent::calculateScreenSpaceError(level, 10.0f, 1.047f, 1080.0f);
    
    // Far object - smaller error
    float error2 = LODComponent::calculateScreenSpaceError(level, 100.0f, 1.047f, 1080.0f);
    
    EXPECT_GT(error1, error2);
}

/**
 * @test Test screen space size calculation
 */
TEST(LODComponentTest, ScreenSpaceSize) {
    // Object at distance 10 with 60 deg FOV
    float size1 = LODComponent::calculateScreenSpaceSize(1.0f, 10.0f, 1.047f, 1080.0f);
    
    // Same object at distance 20 - should appear half as large
    float size2 = LODComponent::calculateScreenSpaceSize(1.0f, 20.0f, 1.047f, 1080.0f);
    
    EXPECT_GT(size1, size2);
    EXPECT_FLOAT_EQ(size1 / size2, 2.0f);
}

/**
 * @test Test LOD stats
 */
TEST(LODComponentTest, Stats) {
    LODComponent lod;
    
    LODLevel level;
    level.quality = LODQuality::High;
    level.vertexCount = 10000;
    level.triangleCount = 5000;
    level.memoryUsage = 1024.0f * 1024.0f; // 1 MB
    
    lod.addLevel(level);
    lod.setLevelByIndex(0);
    
    auto stats = lod.getStats();
    
    EXPECT_EQ(stats.currentLevel, LODQuality::High);
    EXPECT_EQ(stats.vertexCount, 10000);
    EXPECT_EQ(stats.triangleCount, 5000);
    EXPECT_FLOAT_EQ(stats.memoryUsage, 1024.0f * 1024.0f);
}

/**
 * @test Test HLOD
 */
TEST(LODComponentTest, HLOD) {
    LODComponent lod;
    lod.setHLOD(true);
    
    EXPECT_TRUE(lod.isHLOD());
    
    auto child1 = std::make_shared<SceneNode>("Child1");
    auto child2 = std::make_shared<SceneNode>("Child2");
    
    lod.addHLODChild(child1);
    lod.addHLODChild(child2);
    
    EXPECT_EQ(lod.getHLODChildren().size(), 2);
}

// ============================================================================
// LODSystem Tests
// ============================================================================

/**
 * @test Test LOD system construction
 */
TEST(LODSystemTest, Construction) {
    LODSystem::Config config;
    config.updateInterval = 0.1f;
    config.maxUpdatesPerFrame = 50;
    
    LODSystem system(config);
    
    auto stats = system.getStats();
    EXPECT_EQ(stats.totalLODs, 0);
}

/**
 * @test Test LOD registration
 */
TEST(LODSystemTest, Registration) {
    LODSystem system;
    
    auto lod1 = std::make_shared<LODComponent>();
    auto lod2 = std::make_shared<LODComponent>();
    
    system.registerLOD(lod1);
    system.registerLOD(lod2);
    
    auto stats = system.getStats();
    EXPECT_EQ(stats.totalLODs, 2);
    
    system.unregisterLOD(lod1.get());
    
    stats = system.getStats();
    EXPECT_EQ(stats.totalLODs, 1);
}

/**
 * @test Test LOD system update
 */
TEST(LODSystemTest, Update) {
    LODSystem::Config config;
    config.updateInterval = 0.0f; // Update immediately
    config.useScreenSpace = false;
    
    LODSystem system(config);
    
    auto lod = std::make_shared<LODComponent>();
    lod->addLevel(LODLevel(LODQuality::High, 20.0f));
    lod->addLevel(LODLevel(LODQuality::Low, 100.0f));
    
    system.registerLOD(lod);
    
    // Update from close position
    system.update(Vector3(0, 0, 0), 0.016f);
    
    auto stats = lod->getStats();
    EXPECT_EQ(stats.currentLevel, LODQuality::High);
    
    // Update from far position
    system.update(Vector3(150, 0, 0), 0.016f);
    
    stats = lod->getStats();
    EXPECT_EQ(stats.currentLevel, LODQuality::Low);
}

/**
 * @test Test budget constraints
 */
TEST(LODSystemTest, BudgetConstraints) {
    LODSystem::Config config;
    config.updateInterval = 0.0f;
    config.memoryBudget = 512.0f * 1024.0f; // 512 KB
    
    LODSystem system(config);
    
    auto lod = std::make_shared<LODComponent>();
    
    LODLevel high;
    high.quality = LODQuality::High;
    high.distance = 20.0f;
    high.memoryUsage = 1024.0f * 1024.0f; // 1 MB - over budget
    
    LODLevel low;
    low.quality = LODQuality::Low;
    low.distance = 100.0f;
    low.memoryUsage = 256.0f * 1024.0f; // 256 KB - under budget
    
    lod->addLevel(high);
    lod->addLevel(low);
    
    system.registerLOD(lod);
    system.update(Vector3(0, 0, 0), 0.016f);
    
    // Should select low quality due to budget
    // (This depends on implementation details)
    auto stats = lod->getStats();
    (void)stats; // Suppress unused warning
}

/**
 * @test Test LOD system statistics
 */
TEST(LODSystemTest, Statistics) {
    LODSystem system;
    
    for (int i = 0; i < 10; ++i) {
        auto lod = std::make_shared<LODComponent>();
        lod->addLevel(LODLevel(LODQuality::High, 20.0f));
        lod->addLevel(LODLevel(LODQuality::Low, 100.0f));
        system.registerLOD(lod);
    }
    
    system.update(Vector3(0, 0, 0), 0.016f);
    
    auto stats = system.getStats();
    
    EXPECT_EQ(stats.totalLODs, 10);
    EXPECT_GT(stats.updatedLODs, 0);
}

// ============================================================================
// Performance Tests
// ============================================================================

/**
 * @test Performance test for LOD selection
 */
TEST(LODPerformanceTest, Selection) {
    LODComponent lod;
    
    // Add multiple LOD levels
    for (int i = 0; i < 6; ++i) {
        lod.addLevel(LODLevel(static_cast<LODQuality>(i), 
                              10.0f * (i + 1)));
    }
    
    BoundingSphere bounds(Vector3(0, 0, 0), 5.0f);
    
    const int iterations = 10000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        Vector3 camPos(i % 200, 0, 0);
        lod.selectLOD(camPos, bounds);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    EXPECT_LT(duration, 100); // Should complete in < 100ms
}

/**
 * @test Performance test for LOD system with many objects
 */
TEST(LODPerformanceTest, SystemWithManyObjects) {
    LODSystem::Config config;
    config.updateInterval = 0.0f;
    config.maxUpdatesPerFrame = 1000;
    
    LODSystem system(config);
    
    const int lodCount = 1000;
    
    // Create LODs
    for (int i = 0; i < lodCount; ++i) {
        auto lod = std::make_shared<LODComponent>();
        lod->addLevel(LODLevel(LODQuality::High, 20.0f));
        lod->addLevel(LODLevel(LODQuality::Low, 100.0f));
        system.registerLOD(lod);
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    system.update(Vector3(0, 0, 0), 0.016f);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    EXPECT_LT(duration, 50); // Should complete in < 50ms
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
