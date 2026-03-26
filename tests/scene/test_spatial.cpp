/**
 * @file test_spatial.cpp
 * @brief Phoenix Engine Spatial Structure Tests
 * 
 * Test suite for Octree and BVH spatial acceleration structures.
 */

#include <gtest/gtest.h>
#include <memory>
#include <random>
#include <chrono>

#include "../../../include/phoenix/scene/octree.hpp"
#include "../../../include/phoenix/scene/bvh.hpp"
#include "../../../include/phoenix/scene/scene_node.hpp"
#include "../../../include/phoenix/math/vector3.hpp"
#include "../../../include/phoenix/math/bounding.hpp"

using namespace phoenix;
using namespace phoenix::scene;
using namespace phoenix::math;

// ============================================================================
// Test Fixtures
// ============================================================================

class OctreeTest : public ::testing::Test {
protected:
    std::vector<SceneNode::Ptr> nodes_;
    BoundingBox rootBounds;
    
    void SetUp() override {
        rootBounds = BoundingBox(Vector3(-100, -100, -100),
                                  Vector3(100, 100, 100));
    }
    
    SceneNode::Ptr createNode(const std::string& name, const Vector3& pos) {
        auto node = std::make_shared<SceneNode>(name, NodeType::Mesh);
        node->setPosition(pos);
        node->setLocalBoundingBox(BoundingBox(pos - Vector3(1, 1, 1),
                                               pos + Vector3(1, 1, 1)));
        nodes_.push_back(node);
        return node;
    }
};

class BVHTest : public ::testing::Test {
protected:
    std::vector<SceneNode::Ptr> nodes_;
    
    SceneNode::Ptr createNode(const std::string& name, const BoundingBox& bounds) {
        auto node = std::make_shared<SceneNode>(name, NodeType::Mesh);
        node->setLocalBoundingBox(bounds);
        nodes_.push_back(node);
        return node;
    }
};

// ============================================================================
// Octree Tests
// ============================================================================

/**
 * @test Test octree construction
 */
TEST_F(OctreeTest, Construction) {
    Octree octree(rootBounds, 4, 1.0f, 4);
    
    EXPECT_EQ(octree.getObjectCount(), 0);
    EXPECT_GE(octree.getNodeCount(), 1);
    EXPECT_EQ(octree.getBounds(), rootBounds);
}

/**
 * @test Test octree insertion
 */
TEST_F(OctreeTest, Insertion) {
    Octree octree(rootBounds, 4, 1.0f, 4);
    
    auto node1 = createNode("Node1", Vector3(10, 10, 10));
    auto node2 = createNode("Node2", Vector3(-10, -10, -10));
    
    octree.insert(node1.get(), node1->getLocalBoundingBox());
    octree.insert(node2.get(), node2->getLocalBoundingBox());
    
    EXPECT_EQ(octree.getObjectCount(), 2);
}

/**
 * @test Test octree point query
 */
TEST_F(OctreeTest, PointQuery) {
    Octree octree(rootBounds, 4, 1.0f, 4);
    
    auto node1 = createNode("Node1", Vector3(10, 10, 10));
    auto node2 = createNode("Node2", Vector3(-10, -10, -10));
    
    octree.insert(node1.get(), node1->getLocalBoundingBox());
    octree.insert(node2.get(), node2->getLocalBoundingBox());
    
    // Query near node1
    auto results = octree.query(Vector3(10, 10, 10));
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0], node1.get());
    
    // Query near node2
    results = octree.query(Vector3(-10, -10, -10));
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0], node2.get());
    
    // Query empty space
    results = octree.query(Vector3(0, 50, 0));
    EXPECT_EQ(results.size(), 0);
}

/**
 * @test Test octree box query
 */
TEST_F(OctreeTest, BoxQuery) {
    Octree octree(rootBounds, 4, 1.0f, 4);
    
    auto node1 = createNode("Node1", Vector3(10, 10, 10));
    auto node2 = createNode("Node2", Vector3(20, 20, 20));
    auto node3 = createNode("Node3", Vector3(-10, -10, -10));
    
    octree.insert(node1.get(), node1->getLocalBoundingBox());
    octree.insert(node2.get(), node2->getLocalBoundingBox());
    octree.insert(node3.get(), node3->getLocalBoundingBox());
    
    // Query that intersects node1 and node2
    BoundingBox queryBox(Vector3(5, 5, 5), Vector3(25, 25, 25));
    auto results = octree.query(queryBox);
    
    EXPECT_EQ(results.size(), 2);
}

/**
 * @test Test octree sphere query
 */
TEST_F(OctreeTest, SphereQuery) {
    Octree octree(rootBounds, 4, 1.0f, 4);
    
    auto node1 = createNode("Node1", Vector3(10, 10, 10));
    auto node2 = createNode("Node2", Vector3(-10, -10, -10));
    
    octree.insert(node1.get(), node1->getLocalBoundingBox());
    octree.insert(node2.get(), node2->getLocalBoundingBox());
    
    // Query sphere around node1
    BoundingSphere sphere(Vector3(10, 10, 10), 5.0f);
    auto results = octree.query(sphere);
    
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0], node1.get());
}

/**
 * @test Test octree removal
 */
TEST_F(OctreeTest, Removal) {
    Octree octree(rootBounds, 4, 1.0f, 4);
    
    auto node1 = createNode("Node1", Vector3(10, 10, 10));
    auto node2 = createNode("Node2", Vector3(-10, -10, -10));
    
    octree.insert(node1.get(), node1->getLocalBoundingBox());
    octree.insert(node2.get(), node2->getLocalBoundingBox());
    
    EXPECT_EQ(octree.getObjectCount(), 2);
    
    octree.remove(node1.get());
    
    EXPECT_EQ(octree.getObjectCount(), 1);
    
    auto results = octree.query(Vector3(10, 10, 10));
    EXPECT_EQ(results.size(), 0);
}

/**
 * @test Test octree update
 */
TEST_F(OctreeTest, Update) {
    Octree octree(rootBounds, 4, 1.0f, 4);
    
    auto node = createNode("Node", Vector3(10, 10, 10));
    octree.insert(node.get(), node->getLocalBoundingBox());
    
    // Move node
    node->setPosition(Vector3(50, 50, 50));
    const BoundingBox newBounds = node->getLocalBoundingBox();
    
    octree.update(node.get(), newBounds);
    
    // Old position should be empty
    auto results = octree.query(Vector3(10, 10, 10));
    EXPECT_EQ(results.size(), 0);
    
    // New position should have node
    results = octree.query(Vector3(50, 50, 50));
    EXPECT_EQ(results.size(), 1);
}

/**
 * @test Test octree statistics
 */
TEST_F(OctreeTest, Statistics) {
    Octree octree(rootBounds, 8, 1.0f, 4);
    
    // Insert many nodes
    for (int i = 0; i < 100; ++i) {
        const Vector3 pos(
            (i % 10) * 10 - 50,
            (i / 10) * 10 - 50,
            0
        );
        auto node = createNode("Node" + std::to_string(i), pos);
        octree.insert(node.get(), node->getLocalBoundingBox());
    }
    
    auto stats = octree.getStats();
    
    EXPECT_GT(stats.totalNodes, 1);
    EXPECT_EQ(stats.totalObjects, 100);
    EXPECT_GT(stats.leafNodes, 0);
}

/**
 * @test Test octree with many objects (performance)
 */
TEST_F(OctreeTest, PerformanceManyObjects) {
    Octree octree(rootBounds, 8, 1.0f, 16);
    
    const int objectCount = 10000;
    
    // Insert objects
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < objectCount; ++i) {
        const Vector3 pos(
            (rand() % 200) - 100,
            (rand() % 200) - 100,
            (rand() % 200) - 100
        );
        auto node = createNode("Node" + std::to_string(i), pos);
        octree.insert(node.get(), node->getLocalBoundingBox());
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto insertTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    // Query performance
    start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000; ++i) {
        const Vector3 pos(
            (rand() % 200) - 100,
            (rand() % 200) - 100,
            (rand() % 200) - 100
        );
        octree.query(pos);
    }
    
    end = std::chrono::high_resolution_clock::now();
    auto queryTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    // Should complete in reasonable time
    EXPECT_LT(insertTime, 1000); // < 1 second for 10k inserts
    EXPECT_LT(queryTime, 500);   // < 0.5 second for 1k queries
    
    EXPECT_EQ(octree.getObjectCount(), objectCount);
}

// ============================================================================
// BVH Tests
// ============================================================================

/**
 * @test Test BVH construction
 */
TEST_F(BVHTest, Construction) {
    std::vector<SceneNode*> nodes;
    
    for (int i = 0; i < 10; ++i) {
        auto node = createNode("Node" + std::to_string(i),
            BoundingBox(Vector3(i, 0, 0), Vector3(i + 1, 1, 1)));
        nodes.push_back(node.get());
    }
    
    BVH bvh(nodes, true);
    
    EXPECT_EQ(bvh.getObjectCount(), 10);
    EXPECT_GE(bvh.getNodeCount(), 10);
    EXPECT_GT(bvh.getLeafCount(), 0);
}

/**
 * @test Test BVH raycast
 */
TEST_F(BVHTest, Raycast) {
    std::vector<SceneNode*> nodes;
    
    // Create a line of boxes along X axis
    for (int i = 0; i < 5; ++i) {
        auto node = createNode("Node" + std::to_string(i),
            BoundingBox(Vector3(i * 2, -1, -1), Vector3(i * 2 + 1, 1, 1)));
        nodes.push_back(node.get());
    }
    
    BVH bvh(nodes, false); // Simple build for predictability
    
    // Ray along X axis
    Vector3 origin(-10, 0, 0);
    Vector3 dir(1, 0, 0);
    
    auto hit = bvh.raycast(origin, dir);
    
    EXPECT_NE(hit.object, nullptr);
    EXPECT_LT(hit.t, 100); // Should hit first box
}

/**
 * @test Test BVH raycast miss
 */
TEST_F(BVHTest, RaycastMiss) {
    std::vector<SceneNode*> nodes;
    
    auto node = createNode("Node",
        BoundingBox(Vector3(-1, -1, -1), Vector3(1, 1, 1)));
    nodes.push_back(node.get());
    
    BVH bvh(nodes, false);
    
    // Ray that misses the box
    Vector3 origin(-10, 10, 0);
    Vector3 dir(1, 0, 0);
    
    auto hit = bvh.raycast(origin, dir);
    
    EXPECT_EQ(hit.object, nullptr);
}

/**
 * @test Test BVH box query
 */
TEST_F(BVHTest, BoxQuery) {
    std::vector<SceneNode*> nodes;
    
    for (int i = 0; i < 10; ++i) {
        auto node = createNode("Node" + std::to_string(i),
            BoundingBox(Vector3(i, 0, 0), Vector3(i + 1, 1, 1)));
        nodes.push_back(node.get());
    }
    
    BVH bvh(nodes, false);
    
    // Query overlapping several boxes
    BoundingBox queryBox(Vector3(3, -1, -1), Vector3(7, 2, 2));
    auto results = bvh.query(queryBox);
    
    EXPECT_GE(results.size(), 3);
    EXPECT_LE(results.size(), 5);
}

/**
 * @test Test BVH SAH vs Simple build
 */
TEST_F(BVHTest, SAHvsSimple) {
    std::vector<SceneNode*> nodes;
    
    // Create clustered objects
    for (int i = 0; i < 100; ++i) {
        const Vector3 pos(
            (rand() % 50),
            (rand() % 50),
            (rand() % 50)
        );
        auto node = createNode("Node" + std::to_string(i),
            BoundingBox(pos, pos + Vector3(1, 1, 1)));
        nodes.push_back(node.get());
    }
    
    BVH sahBvh(nodes, true);
    BVH simpleBvh(nodes, false);
    
    // SAH should produce better (lower) cost
    auto sahStats = sahBvh.getStats();
    auto simpleStats = simpleBvh.getStats();
    
    EXPECT_LT(sahStats.sahCost, simpleStats.sahCost);
}

/**
 * @test Test BVH statistics
 */
TEST_F(BVHTest, Statistics) {
    std::vector<SceneNode*> nodes;
    
    for (int i = 0; i < 100; ++i) {
        const Vector3 pos(i, 0, 0);
        auto node = createNode("Node" + std::to_string(i),
            BoundingBox(pos, pos + Vector3(1, 1, 1)));
        nodes.push_back(node.get());
    }
    
    BVH bvh(nodes, true);
    auto stats = bvh.getStats();
    
    EXPECT_EQ(stats.totalObjects, 100);
    EXPECT_GT(stats.leafNodes, 0);
    EXPECT_GT(stats.totalNodes, 0);
    EXPECT_GT(stats.sahCost, 0);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
