/**
 * @file test_scene_graph.cpp
 * @brief Phoenix Engine Scene Graph Tests
 * 
 * Comprehensive test suite for scene graph functionality including:
 * - Node hierarchy
 * - Transform propagation
 * - Visitor pattern
 * - Serialization
 */

#include <gtest/gtest.h>
#include <memory>
#include <cmath>

#include "../../../include/phoenix/scene/scene_node.hpp"
#include "../../../include/phoenix/scene/scene.hpp"
#include "../../../include/phoenix/scene/transform.hpp"
#include "../../../include/phoenix/math/matrix4.hpp"
#include "../../../include/phoenix/math/quaternion.hpp"

using namespace phoenix;
using namespace phoenix::scene;
using namespace phoenix::math;

// ============================================================================
// Test Visitor
// ============================================================================

class CountingVisitor : public SceneVisitor {
public:
    int enterCount = 0;
    int leaveCount = 0;
    int visitCount = 0;
    
    bool enter(SceneNode& node) override {
        ++enterCount;
        (void)node;
        return true;
    }
    
    void leave(SceneNode& node) override {
        ++leaveCount;
        (void)node;
    }
    
    void visit(SceneNode& node) override {
        ++visitCount;
        (void)node;
    }
};

// ============================================================================
// Transform Tests
// ============================================================================

/**
 * @test Test default transform construction
 */
TEST(TransformTest, DefaultConstruction) {
    Transform t;
    
    EXPECT_EQ(t.getPosition(), Vector3::zero());
    EXPECT_EQ(t.getRotation(), Quaternion::identity());
    EXPECT_EQ(t.getScale(), Vector3::one());
    EXPECT_TRUE(t.isWorldDirty());
}

/**
 * @test Test transform setters and getters
 */
TEST(TransformTest, SettersAndGetters) {
    Transform t;
    
    const Vector3 pos(1.0f, 2.0f, 3.0f);
    const Quaternion rot = Quaternion::fromAxisAngle(Vector3::unitY(), 1.57f);
    const Vector3 scale(2.0f, 2.0f, 2.0f);
    
    t.setPosition(pos);
    t.setRotation(rot);
    t.setScale(scale);
    
    EXPECT_EQ(t.getPosition(), pos);
    EXPECT_EQ(t.getRotation().x, rot.x);
    EXPECT_EQ(t.getScale(), scale);
}

/**
 * @test Test local matrix generation
 */
TEST(TransformTest, LocalMatrix) {
    Transform t;
    t.setPosition(Vector3(1.0f, 0.0f, 0.0f));
    t.setScale(Vector3(2.0f, 2.0f, 2.0f));
    
    const Matrix4 local = t.getLocalMatrix();
    
    // Check translation
    EXPECT_FLOAT_EQ(local[12], 1.0f);
    
    // Check scale
    EXPECT_FLOAT_EQ(local[0], 2.0f);
    EXPECT_FLOAT_EQ(local[5], 2.0f);
    EXPECT_FLOAT_EQ(local[10], 2.0f);
}

/**
 * @test Test world matrix update with parent
 */
TEST(TransformTest, WorldMatrixWithParent) {
    Transform parent;
    Transform child;
    
    parent.setPosition(Vector3(10.0f, 0.0f, 0.0f));
    child.setPosition(Vector3(5.0f, 0.0f, 0.0f));
    
    parent.updateWorldMatrix();
    child.updateWorldMatrix(parent.getWorldMatrix());
    
    const Vector3 childWorldPos(
        child.getWorldMatrix()[12],
        child.getWorldMatrix()[13],
        child.getWorldMatrix()[14]
    );
    
    // Child should be at parent position + local position
    EXPECT_FLOAT_EQ(childWorldPos.x, 15.0f);
    EXPECT_FLOAT_EQ(childWorldPos.y, 0.0f);
    EXPECT_FLOAT_EQ(childWorldPos.z, 0.0f);
}

/**
 * @test Test dirty flag propagation
 */
TEST(TransformTest, DirtyFlags) {
    Transform t;
    EXPECT_TRUE(t.isWorldDirty());
    
    t.updateWorldMatrix();
    EXPECT_FALSE(t.isWorldDirty());
    
    t.setPosition(Vector3(1.0f, 0.0f, 0.0f));
    EXPECT_TRUE(t.isWorldDirty());
}

// ============================================================================
// SceneNode Tests
// ============================================================================

/**
 * @test Test node creation
 */
TEST(SceneNodeTest, Creation) {
    auto node = std::make_shared<SceneNode>("TestNode");
    
    EXPECT_EQ(node->getName(), "TestNode");
    EXPECT_EQ(node->getType(), NodeType::Unknown);
    EXPECT_TRUE(node->hasFlag(NodeFlags::Visible));
    EXPECT_EQ(node->getParent(), nullptr);
    EXPECT_EQ(node->getChildCount(), 0);
}

/**
 * @test Test node hierarchy
 */
TEST(SceneNodeTest, Hierarchy) {
    auto parent = std::make_shared<SceneNode>("Parent");
    auto child1 = std::make_shared<SceneNode>("Child1");
    auto child2 = std::make_shared<SceneNode>("Child2");
    
    parent->addChild(child1);
    parent->addChild(child2);
    
    EXPECT_EQ(parent->getChildCount(), 2);
    EXPECT_EQ(child1->getParent(), parent.get());
    EXPECT_EQ(child2->getParent(), parent.get());
    
    // Find child by name
    EXPECT_EQ(parent->findChild("Child1"), child1.get());
    EXPECT_EQ(parent->findChild("Child2"), child2.get());
    EXPECT_EQ(parent->findChild("NonExistent"), nullptr);
}

/**
 * @test Test node removal
 */
TEST(SceneNodeTest, Removal) {
    auto parent = std::make_shared<SceneNode>("Parent");
    auto child = std::make_shared<SceneNode>("Child");
    
    parent->addChild(child);
    EXPECT_EQ(parent->getChildCount(), 1);
    
    parent->removeChild(child);
    EXPECT_EQ(parent->getChildCount(), 0);
    EXPECT_EQ(child->getParent(), nullptr);
}

/**
 * @test Test remove from parent
 */
TEST(SceneNodeTest, RemoveFromParent) {
    auto parent = std::make_shared<SceneNode>("Parent");
    auto child = std::make_shared<SceneNode>("Child");
    
    parent->addChild(child);
    child->removeFromParent();
    
    EXPECT_EQ(parent->getChildCount(), 0);
    EXPECT_EQ(child->getParent(), nullptr);
}

/**
 * @test Test transform propagation in hierarchy
 */
TEST(SceneNodeTest, TransformPropagation) {
    auto parent = std::make_shared<SceneNode>("Parent");
    auto child = std::make_shared<SceneNode>("Child");
    
    parent->setPosition(Vector3(10.0f, 0.0f, 0.0f));
    child->setPosition(Vector3(5.0f, 0.0f, 0.0f));
    
    parent->addChild(child);
    
    parent->updateWorldTransform();
    
    const auto childWorldPos = child->getWorldMatrix().transformPoint(Vector3::zero());
    
    EXPECT_FLOAT_EQ(childWorldPos.x, 15.0f);
}

/**
 * @test Test visitor pattern
 */
TEST(SceneNodeTest, VisitorPattern) {
    auto root = std::make_shared<SceneNode>("Root");
    auto child1 = std::make_shared<SceneNode>("Child1");
    auto child2 = std::make_shared<SceneNode>("Child2");
    auto grandchild = std::make_shared<SceneNode>("Grandchild");
    
    root->addChild(child1);
    root->addChild(child2);
    child1->addChild(grandchild);
    
    CountingVisitor visitor;
    root->traverse(visitor);
    
    // Should visit all 4 nodes
    EXPECT_EQ(visitor.visitCount, 4);
    EXPECT_EQ(visitor.enterCount, 4);
    EXPECT_EQ(visitor.leaveCount, 4);
}

/**
 * @test Test node depth
 */
TEST(SceneNodeTest, Depth) {
    auto root = std::make_shared<SceneNode>("Root");
    auto child = std::make_shared<SceneNode>("Child");
    auto grandchild = std::make_shared<SceneNode>("Grandchild");
    
    root->addChild(child);
    child->addChild(grandchild);
    
    EXPECT_EQ(root->getDepth(), 0);
    EXPECT_EQ(child->getDepth(), 1);
    EXPECT_EQ(grandchild->getDepth(), 2);
}

/**
 * @test Test node count
 */
TEST(SceneNodeTest, NodeCount) {
    auto root = std::make_shared<SceneNode>("Root");
    auto child1 = std::make_shared<SceneNode>("Child1");
    auto child2 = std::make_shared<SceneNode>("Child2");
    
    root->addChild(child1);
    root->addChild(child2);
    
    EXPECT_EQ(root->getNodeCount(), 3);
}

/**
 * @test Test descendant check
 */
TEST(SceneNodeTest, DescendantCheck) {
    auto root = std::make_shared<SceneNode>("Root");
    auto child = std::make_shared<SceneNode>("Child");
    auto grandchild = std::make_shared<SceneNode>("Grandchild");
    
    root->addChild(child);
    child->addChild(grandchild);
    
    EXPECT_TRUE(grandchild->isDescendantOf(*root));
    EXPECT_TRUE(grandchild->isDescendantOf(*child));
    EXPECT_FALSE(root->isDescendantOf(*grandchild));
}

/**
 * @test Test clone
 */
TEST(SceneNodeTest, Clone) {
    auto original = std::make_shared<SceneNode>("Original");
    original->setPosition(Vector3(5.0f, 0.0f, 0.0f));
    original->setScale(Vector3(2.0f, 2.0f, 2.0f));
    
    auto child = std::make_shared<SceneNode>("Child");
    original->addChild(child);
    
    auto clone = original->clone();
    
    EXPECT_EQ(clone->getName(), "Original");
    EXPECT_EQ(clone->getPosition(), Vector3(5.0f, 0.0f, 0.0f));
    EXPECT_EQ(clone->getScale(), Vector3(2.0f, 2.0f, 2.0f));
    EXPECT_EQ(clone->getChildCount(), 1);
    
    // Clone should be independent
    clone->setName("Cloned");
    EXPECT_NE(clone->getName(), original->getName());
}

// ============================================================================
// Scene Tests
// ============================================================================

/**
 * @test Test scene creation
 */
TEST(SceneTest, Creation) {
    Scene scene("TestScene");
    
    EXPECT_EQ(scene.getName(), "TestScene");
    EXPECT_NE(scene.getRoot(), nullptr);
    EXPECT_EQ(scene.getNodeCount(), 1); // Just root
}

/**
 * @test Test scene node addition
 */
TEST(SceneTest, NodeAddition) {
    Scene scene;
    auto node = std::make_shared<SceneNode>("Node");
    
    scene.addNode(node);
    
    EXPECT_EQ(scene.getNodeCount(), 2); // Root + node
    EXPECT_NE(scene.findNode("Node"), nullptr);
}

/**
 * @test Test scene find node
 */
TEST(SceneTest, FindNode) {
    Scene scene;
    
    auto node1 = std::make_shared<SceneNode>("Node1");
    auto node2 = std::make_shared<SceneNode>("Node2");
    
    scene.addNode(node1);
    scene.addNode(node2);
    
    EXPECT_EQ(scene.findNode("Node1"), node1.get());
    EXPECT_EQ(scene.findNode("Node2"), node2.get());
    EXPECT_EQ(scene.findNode("NonExistent"), nullptr);
}

/**
 * @test Test scene traversal
 */
TEST(SceneTest, Traversal) {
    Scene scene;
    
    auto node1 = std::make_shared<SceneNode>("Node1");
    auto node2 = std::make_shared<SceneNode>("Node2");
    
    scene.addNode(node1);
    scene.addNode(node2);
    
    int count = 0;
    scene.traverse([&count](SceneNode&) {
        ++count;
    });
    
    EXPECT_EQ(count, 3); // Root + 2 nodes
}

/**
 * @test Test scene bounds
 */
TEST(SceneTest, Bounds) {
    Scene scene;
    
    auto node = std::make_shared<SceneNode>("Node");
    node->setPosition(Vector3(10.0f, 0.0f, 0.0f));
    node->setLocalBoundingBox(BoundingBox(Vector3(-1.0f, -1.0f, -1.0f),
                                           Vector3(1.0f, 1.0f, 1.0f)));
    
    scene.addNode(node);
    scene.updateTransforms();
    
    const auto bounds = scene.calculateBounds();
    
    EXPECT_TRUE(bounds.isValid());
    EXPECT_GE(bounds.max.x, 9.0f); // At least 10 - 1
}

/**
 * @test Test scene statistics
 */
TEST(SceneTest, Statistics) {
    Scene scene;
    
    auto mesh1 = std::make_shared<SceneNode>("Mesh1", NodeType::Mesh);
    auto mesh2 = std::make_shared<SceneNode>("Mesh2", NodeType::Mesh);
    auto light = std::make_shared<SceneNode>("Light", NodeType::Light);
    
    mesh1->setFlag(NodeFlags::IsStatic, true);
    mesh2->setFlag(NodeFlags::IsDynamic, true);
    
    scene.addNode(mesh1);
    scene.addNode(mesh2);
    scene.addNode(light);
    
    scene.updateStats();
    
    const auto& stats = scene.getStats();
    
    EXPECT_EQ(stats.totalNodes, 4); // Root + 3 nodes
    EXPECT_EQ(stats.meshNodes, 2);
    EXPECT_EQ(stats.lightNodes, 1);
    EXPECT_EQ(stats.staticNodes, 1);
    EXPECT_EQ(stats.dynamicNodes, 1);
}

/**
 * @test Test scene clear
 */
TEST(SceneTest, Clear) {
    Scene scene;
    
    scene.addNode(std::make_shared<SceneNode>("Node1"));
    scene.addNode(std::make_shared<SceneNode>("Node2"));
    
    scene.clear();
    
    EXPECT_EQ(scene.getNodeCount(), 1); // Just root
    EXPECT_EQ(scene.findNode("Node1"), nullptr);
}

/**
 * @test Test scene merge
 */
TEST(SceneTest, Merge) {
    Scene scene1;
    Scene scene2;
    
    scene1.addNode(std::make_shared<SceneNode>("Node1"));
    scene2.addNode(std::make_shared<SceneNode>("Node2"));
    
    scene1.merge(scene2);
    
    EXPECT_NE(scene1.findNode("Node1"), nullptr);
    EXPECT_NE(scene1.findNode("Node2"), nullptr);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
