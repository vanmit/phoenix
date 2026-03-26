#include "../../include/phoenix/scene/scene.hpp"
#include "../../include/phoenix/scene/octree.hpp"
#include "../../include/phoenix/scene/bvh.hpp"
#include <algorithm>
#include <queue>

namespace phoenix {
namespace scene {

// ============================================================================
// Scene Implementation
// ============================================================================

Scene::Scene(const std::string& name)
    : name_(name)
    , root_(std::make_shared<SceneNode>("Root", NodeType::Root))
    , octree_(nullptr)
    , bvh_(nullptr)
    , sceneBounds_(math::BoundingBox::empty())
    , sceneSphere_(math::BoundingSphere::empty())
    , boundsValid_(false)
    , stats_()
    , nodeById_()
    , nodeByName_() {
    
    root_->setFlag(NodeFlags::Visible, true);
    updateNodeLookup();
}

Scene::~Scene() = default;

Scene::Scene(Scene&& other) noexcept
    : name_(std::move(other.name_))
    , root_(std::move(other.root_))
    , octree_(std::move(other.octree_))
    , bvh_(std::move(other.bvh_))
    , sceneBounds_(other.sceneBounds_)
    , sceneSphere_(other.sceneSphere_)
    , boundsValid_(other.boundsValid_)
    , stats_(other.stats_)
    , nodeById_(std::move(other.nodeById_))
    , nodeByName_(std::move(other.nodeByName_)) {
    
    other.boundsValid_ = false;
    other.updateNodeLookup();
}

Scene& Scene::operator=(Scene&& other) noexcept {
    if (this != &other) {
        name_ = std::move(other.name_);
        root_ = std::move(other.root_);
        octree_ = std::move(other.octree_);
        bvh_ = std::move(other.bvh_);
        sceneBounds_ = other.sceneBounds_;
        sceneSphere_ = other.sceneSphere_;
        boundsValid_ = other.boundsValid_;
        stats_ = other.stats_;
        nodeById_ = std::move(other.nodeById_);
        nodeByName_ = std::move(other.nodeByName_);
        
        other.boundsValid_ = false;
        other.updateNodeLookup();
    }
    return *this;
}

// ============================================================================
// Root Node
// ============================================================================

void Scene::setRoot(NodePtr root) {
    root_ = std::move(root);
    boundsValid_ = false;
    updateNodeLookup();
}

// ============================================================================
// Node Management
// ============================================================================

void Scene::addNode(NodePtr node) {
    if (node && root_) {
        root_->addChild(std::move(node));
        boundsValid_ = false;
        updateNodeLookup();
    }
}

void Scene::removeNode(NodePtr node) {
    if (node && root_) {
        root_->removeChild(node);
        boundsValid_ = false;
        updateNodeLookup();
    }
}

void Scene::removeNode(const std::string& name) {
    if (root_) {
        root_->removeChild(name);
        boundsValid_ = false;
        updateNodeLookup();
    }
}

SceneNode* Scene::findNode(const std::string& name) {
    auto it = nodeByName_.find(name);
    return (it != nodeByName_.end()) ? it->second : nullptr;
}

const SceneNode* Scene::findNode(const std::string& name) const {
    auto it = nodeByName_.find(name);
    return (it != nodeByName_.end()) ? it->second : nullptr;
}

SceneNode* Scene::findNodeById(uint32_t id) {
    auto it = nodeById_.find(id);
    return (it != nodeById_.end()) ? it->second : nullptr;
}

const SceneNode* Scene::findNodeById(uint32_t id) const {
    auto it = nodeById_.find(id);
    return (it != nodeById_.end()) ? it->second : nullptr;
}

Scene::NodeList Scene::getNodesByType(NodeType type) const {
    NodeList result;
    
    root_->traverse([&result, type](SceneNode& node) {
        if (node.getType() == type) {
            result.push_back(node.shared_from_this());
        }
    });
    
    return result;
}

Scene::NodeList Scene::getNodesByPredicate(
    const std::function<bool(const SceneNode&)>& predicate) const {
    
    NodeList result;
    
    root_->traverse([&result, &predicate](SceneNode& node) {
        if (predicate(node)) {
            result.push_back(node.shared_from_this());
        }
    });
    
    return result;
}

// ============================================================================
// Traversal
// ============================================================================

void Scene::traverse(SceneVisitor& visitor) {
    if (root_) {
        root_->traverse(visitor);
    }
}

void Scene::traverse(const NodeCallback& callback) {
    if (root_) {
        root_->traverse(callback);
    }
}

void Scene::traverse(const NodeCallback& callback, int maxDepth) {
    if (root_) {
        root_->traverse(callback, maxDepth);
    }
}

// ============================================================================
// Transform Updates
// ============================================================================

void Scene::updateTransforms() {
    if (root_) {
        root_->updateWorldTransform();
    }
    boundsValid_ = false;
}

void Scene::updateDirtyTransforms() {
    if (!root_) return;
    
    // BFS traversal to update dirty transforms
    std::queue<SceneNode*> queue;
    queue.push(root_.get());
    
    while (!queue.empty()) {
        SceneNode* node = queue.front();
        queue.pop();
        
        if (node->getTransform().isWorldDirty()) {
            node->updateWorldTransform();
        }
        
        for (size_t i = 0; i < node->getChildCount(); ++i) {
            queue.push(node->getChild(i));
        }
    }
    
    boundsValid_ = false;
}

// ============================================================================
// Spatial Acceleration
// ============================================================================

void Scene::buildOctree(int maxDepth, float minSize) {
    if (!root_) return;
    
    updateTransforms();
    const auto bounds = calculateBounds();
    
    octree_ = std::make_unique<Octree>(bounds, maxDepth, minSize);
    
    // Insert all mesh nodes
    root_->traverse([this](SceneNode& node) {
        if (node.getType() == NodeType::Mesh && node.isVisible()) {
            octree_->insert(&node, node.getWorldBoundingBox());
        }
    });
}

void Scene::buildBVH(bool useSAH) {
    if (!root_) return;
    
    updateTransforms();
    
    // Collect all renderable nodes
    std::vector<SceneNode*> nodes;
    root_->traverse([&nodes](SceneNode& node) {
        if (node.getType() == NodeType::Mesh && node.isVisible()) {
            nodes.push_back(&node);
        }
    });
    
    if (!nodes.empty()) {
        bvh_ = std::make_unique<BVH>(nodes, useSAH);
    }
}

void Scene::updateSpatialStructures() {
    if (octree_) {
        octree_->clear();
        
        root_->traverse([this](SceneNode& node) {
            if (node.getType() == NodeType::Mesh && node.isVisible()) {
                if (node.hasFlag(NodeFlags::IsDynamic)) {
                    octree_->insert(&node, node.getWorldBoundingBox());
                }
            }
        });
    }
    
    // BVH typically needs full rebuild for dynamic scenes
    if (bvh_) {
        buildBVH(bvh_->usesSAH());
    }
}

// ============================================================================
// Culling
// ============================================================================

void Scene::cullFrustum(const math::Frustum& frustum) {
    if (!root_) return;
    
    root_->traverse([&frustum](SceneNode& node) {
        if (!node.hasFlag(NodeFlags::Visible)) {
            node.setFlag(NodeFlags::Culled, true);
            return;
        }
        
        const auto bounds = node.getWorldBoundingBox();
        const auto intersection = frustum.classifyBox(bounds);
        
        if (intersection == math::Frustum::Intersection::Outside) {
            node.setFlag(NodeFlags::Culled, true);
        } else {
            node.setFlag(NodeFlags::Culled, false);
        }
    });
}

void Scene::cullDistance(const math::Vector3& cameraPos, float maxDistance) {
    if (!root_) return;
    
    const float maxDistSq = maxDistance * maxDistance;
    
    root_->traverse([&cameraPos, maxDistSq](SceneNode& node) {
        if (node.hasFlag(NodeFlags::Culled)) return;
        
        const auto sphere = node.getWorldBoundingSphere();
        const float distSq = (sphere.center - cameraPos).lengthSquared();
        
        if (distSq > maxDistSq * maxDistSq) {
            node.setFlag(NodeFlags::Culled, true);
        }
    });
}

void Scene::cullBackface(const math::Vector3& cameraPos, const math::Vector3& cameraDir) {
    if (!root_) return;
    
    root_->traverse([&cameraPos, &cameraDir](SceneNode& node) {
        if (node.hasFlag(NodeFlags::Culled)) return;
        
        const auto bounds = node.getWorldBoundingBox();
        const Vector3 toNode = bounds.center() - cameraPos;
        
        // Simple backface test based on node direction
        // For mesh nodes, this would use actual face normals
        const float dot = toNode.normalized().dot(cameraDir);
        
        if (dot < -0.9f) {
            // Node is behind camera
            node.setFlag(NodeFlags::Culled, true);
        }
    });
}

void Scene::resetCulling() {
    if (!root_) return;
    
    root_->traverse([](SceneNode& node) {
        node.setFlag(NodeFlags::Culled, false);
    });
}

Scene::NodeList Scene::getVisibleNodes() const {
    NodeList result;
    
    if (root_) {
        root_->traverse([&result](const SceneNode& node) {
            if (node.isVisible()) {
                result.push_back(const_cast<SceneNode&>(node).shared_from_this());
            }
        });
    }
    
    return result;
}

// ============================================================================
// Scene Bounds
// ============================================================================

math::BoundingBox Scene::calculateBounds() const {
    if (!root_) return math::BoundingBox::empty();
    
    math::BoundingBox bounds = math::BoundingBox::empty();
    
    root_->traverse([&bounds](const SceneNode& node) {
        bounds.extend(node.getWorldBoundingBox());
    });
    
    return bounds;
}

math::BoundingSphere Scene::calculateBoundingSphere() const {
    return math::BoundingSphere::fromBox(calculateBounds());
}

void Scene::updateBounds() {
    sceneBounds_ = calculateBounds();
    sceneSphere_ = math::BoundingSphere::fromBox(sceneBounds_);
    boundsValid_ = true;
}

// ============================================================================
// Statistics
// ============================================================================

void Scene::updateStats() {
    stats_ = SceneStats();
    
    if (root_) {
        collectStats(*root_);
    }
    
    if (!boundsValid_) {
        updateBounds();
    }
    
    stats_.sceneBounds = sceneBounds_;
    stats_.sceneSphere = sceneSphere_;
}

void Scene::collectStats(SceneNode& node) {
    ++stats_.totalNodes;
    
    if (node.hasFlag(NodeFlags::Culled)) {
        ++stats_.culledNodes;
    } else if (node.hasFlag(NodeFlags::Visible)) {
        ++stats_.visibleNodes;
    }
    
    switch (node.getType()) {
        case NodeType::Mesh: ++stats_.meshNodes; break;
        case NodeType::Light: ++stats_.lightNodes; break;
        case NodeType::Camera: ++stats_.cameraNodes; break;
        default: break;
    }
    
    if (node.hasFlag(NodeFlags::IsDynamic)) {
        ++stats_.dynamicNodes;
    } else if (node.hasFlag(NodeFlags::IsStatic)) {
        ++stats_.staticNodes;
    }
    
    for (size_t i = 0; i < node.getChildCount(); ++i) {
        collectStats(*node.getChild(i));
    }
}

// ============================================================================
// Serialization
// ============================================================================

bool Scene::saveToGlTF(const std::string& filename) const {
    // TODO: Implement glTF serialization
    (void)filename;
    return false;
}

bool Scene::loadFromGlTF(const std::string& filename) {
    // TODO: Implement glTF deserialization
    (void)filename;
    return false;
}

void Scene::serialize(SceneSerializer& serializer) const {
    serializer.beginScene(*this);
    
    if (root_) {
        root_->serialize(serializer);
    }
    
    serializer.endScene();
}

void Scene::deserialize(SceneDeserializer& deserializer) {
    deserializer.beginScene(*this);
    
    if (root_) {
        root_->deserialize(deserializer);
    }
    
    deserializer.endScene();
    updateNodeLookup();
}

// ============================================================================
// Utility
// ============================================================================

void Scene::clear() {
    root_ = std::make_shared<SceneNode>("Root", NodeType::Root);
    octree_.reset();
    bvh_.reset();
    sceneBounds_ = math::BoundingBox::empty();
    sceneSphere_ = math::BoundingSphere::empty();
    boundsValid_ = false;
    stats_ = SceneStats();
    nodeById_.clear();
    nodeByName_.clear();
}

void Scene::merge(const Scene& other) {
    if (!other.root_) return;
    
    // Clone all children from other scene's root
    for (size_t i = 0; i < other.root_->getChildCount(); ++i) {
        auto child = other.root_->getChild(i);
        auto clone = child->clone();
        root_->addChild(clone);
    }
    
    boundsValid_ = false;
    updateNodeLookup();
}

Scene::Ptr Scene::clone() const {
    auto clonedScene = std::make_shared<Scene>(name_ + "_clone");
    clonedScene->merge(*this);
    return clonedScene;
}

void Scene::optimize() {
    // TODO: Implement scene optimization
    // - Sort nodes by material/state
    // - Batch static geometry
    // - Prepare LOD transitions
}

void Scene::updateNodeLookup() {
    nodeById_.clear();
    nodeByName_.clear();
    
    if (!root_) return;
    
    root_->traverse([this](SceneNode& node) {
        nodeById_[node.getId()] = &node;
        nodeByName_[node.getName()] = &node;
    });
}

} // namespace scene
} // namespace phoenix
