#pragma once

#include "../math/bounding.hpp"
#include "../math/vector3.hpp"
#include <array>
#include <vector>
#include <memory>
#include <functional>

namespace phoenix {
namespace scene {

class SceneNode;

/**
 * @brief Octree spatial partitioning structure
 * 
 * Efficiently organizes 3D objects for:
 * - Frustum culling
 * - Ray casting
 * - Collision detection
 * - LOD selection
 * 
 * Uses loose octree for better handling of objects spanning node boundaries.
 */
class Octree {
public:
    using Ptr = std::shared_ptr<Octree>;
    using ObjectList = std::vector<SceneNode*>;
    using QueryCallback = std::function<void(SceneNode*)>;
    
    // ========================================================================
    // Octree Node
    // ========================================================================
    
    struct alignas(32) Node {
        math::BoundingBox bounds;
        Node* parent;
        std::array<Node*, 8> children;
        ObjectList objects;
        
        uint8_t depth;
        bool isLeaf;
        uint16_t objectCount;
        
        Node() noexcept 
            : bounds()
            , parent(nullptr)
            , children{}
            , objects()
            , depth(0)
            , isLeaf(true)
            , objectCount(0) {
            children.fill(nullptr);
        }
        
        Node(const math::BoundingBox& b, uint8_t d) noexcept
            : bounds(b)
            , parent(nullptr)
            , children{}
            , objects()
            , depth(d)
            , isLeaf(true)
            , objectCount(0) {
            children.fill(nullptr);
        }
        
        ~Node() {
            for (auto* child : children) {
                delete child;
            }
        }
        
        bool hasChildren() const noexcept {
            return children[0] != nullptr;
        }
        
        size_t getTotalObjectCount() const {
            size_t count = objects.size();
            for (auto* child : children) {
                if (child) count += child->getTotalObjectCount();
            }
            return count;
        }
    };
    
    // ========================================================================
    // Construction
    // ========================================================================
    
    /**
     * @brief Create octree with specified bounds
     * 
     * @param bounds Root node bounds
     * @param maxDepth Maximum subdivision depth
     * @param minSize Minimum node size (stops subdivision)
     * @param maxObjectsPerNode Max objects before subdivision
     */
    explicit Octree(const math::BoundingBox& bounds, 
                    int maxDepth = 8,
                    float minSize = 0.1f,
                    size_t maxObjectsPerNode = 4);
    
    ~Octree();
    
    // Non-copyable, movable
    Octree(const Octree&) = delete;
    Octree& operator=(const Octree&) = delete;
    Octree(Octree&&) noexcept;
    Octree& operator=(Octree&&) noexcept;
    
    // ========================================================================
    // Object Management
    // ========================================================================
    
    /**
     * @brief Insert object into octree
     * 
     * @param obj Object to insert
     * @param bounds Object's bounding box
     */
    void insert(SceneNode* obj, const math::BoundingBox& bounds);
    
    /**
     * @brief Remove object from octree
     */
    void remove(SceneNode* obj);
    
    /**
     * @brief Update object's position
     * 
     * More efficient than remove + insert for moving objects.
     */
    void update(SceneNode* obj, const math::BoundingBox& newBounds);
    
    /**
     * @brief Clear all objects
     */
    void clear();
    
    // ========================================================================
    // Queries
    // ========================================================================
    
    /**
     * @brief Query objects intersecting a box
     */
    ObjectList query(const math::BoundingBox& bounds) const;
    
    /**
     * @brief Query objects intersecting a sphere
     */
    ObjectList query(const math::BoundingSphere& sphere) const;
    
    /**
     * @brief Query objects intersecting a point
     */
    ObjectList query(const math::Vector3& point) const;
    
    /**
     * @brief Query with callback (avoids allocation)
     */
    void query(const math::BoundingBox& bounds, const QueryCallback& callback) const;
    
    /**
     * @brief Ray cast query
     * 
     * @param rayOrigin Ray origin
     * @param rayDir Ray direction (normalized)
     * @param maxDist Maximum ray distance
     * @return Objects hit by ray, sorted by distance
     */
    ObjectList raycast(const math::Vector3& rayOrigin, 
                       const math::Vector3& rayDir,
                       float maxDist = FLT_MAX) const;
    
    // ========================================================================
    // Properties
    // ========================================================================
    
    const Node* getRoot() const noexcept { return root_; }
    Node* getRoot() noexcept { return root_; }
    
    const math::BoundingBox& getBounds() const noexcept { return root_->bounds; }
    
    int getMaxDepth() const noexcept { return maxDepth_; }
    float getMinSize() const noexcept { return minSize_; }
    
    size_t getObjectCount() const noexcept { return totalObjects_; }
    size_t getNodeCount() const noexcept { return nodeCount_; }
    
    /**
     * @brief Get memory usage in bytes
     */
    size_t getMemoryUsage() const;
    
    // ========================================================================
    // Visualization
    // ========================================================================
    
    /**
     * @brief Get all leaf nodes for debugging
     */
    std::vector<const Node*> getLeafNodes() const;
    
    /**
     * @brief Get node statistics
     */
    struct Stats {
        size_t totalNodes = 0;
        size_t leafNodes = 0;
        size_t maxDepth = 0;
        size_t totalObjects = 0;
        size_t maxObjectsInNode = 0;
        float avgObjectsPerLeaf = 0.0f;
    };
    
    Stats getStats() const;
    
private:
    Node* root_;
    int maxDepth_;
    float minSize_;
    size_t maxObjectsPerNode_;
    size_t totalObjects_;
    size_t nodeCount_;
    
    // Object to node mapping for fast removal
    std::unordered_map<SceneNode*, Node*> objectNodeMap_;
    
    void subdivide(Node* node);
    Node* findContainingNode(Node* node, const math::BoundingBox& bounds);
    void insertIntoNode(Node* node, SceneNode* obj, const math::BoundingBox& bounds);
    bool intersectsNode(const Node* node, const math::BoundingBox& bounds) const;
    bool intersectsNode(const Node* node, const math::BoundingSphere& sphere) const;
    
    void queryRecursive(const Node* node, const math::BoundingBox& bounds, 
                        const QueryCallback& callback) const;
    void queryRecursive(const Node* node, const math::BoundingSphere& sphere,
                        const QueryCallback& callback) const;
    
    void raycastRecursive(const Node* node, const math::Vector3& origin,
                          const math::Vector3& dir, float maxDist,
                          ObjectList& results) const;
};

// ============================================================================
// Inline Implementations
// ============================================================================

inline Octree::Octree(Octree&& other) noexcept
    : root_(other.root_)
    , maxDepth_(other.maxDepth_)
    , minSize_(other.minSize_)
    , maxObjectsPerNode_(other.maxObjectsPerNode_)
    , totalObjects_(other.totalObjects_)
    , nodeCount_(other.nodeCount_)
    , objectNodeMap_(std::move(other.objectNodeMap_)) {
    
    other.root_ = nullptr;
    other.totalObjects_ = 0;
    other.nodeCount_ = 0;
}

inline Octree& Octree::operator=(Octree&& other) noexcept {
    if (this != &other) {
        delete root_;
        
        root_ = other.root_;
        maxDepth_ = other.maxDepth_;
        minSize_ = other.minSize_;
        maxObjectsPerNode_ = other.maxObjectsPerNode_;
        totalObjects_ = other.totalObjects_;
        nodeCount_ = other.nodeCount_;
        objectNodeMap_ = std::move(other.objectNodeMap_);
        
        other.root_ = nullptr;
        other.totalObjects_ = 0;
        other.nodeCount_ = 0;
    }
    return *this;
}

inline Octree::~Octree() {
    delete root_;
}

inline void Octree::clear() {
    delete root_;
    
    const float size = std::max({
        root_->bounds.size().x,
        root_->bounds.size().y,
        root_->bounds.size().z
    });
    
    root_ = new Node(root_->bounds, 0);
    totalObjects_ = 0;
    nodeCount_ = 1;
    objectNodeMap_.clear();
}

} // namespace scene
} // namespace phoenix
