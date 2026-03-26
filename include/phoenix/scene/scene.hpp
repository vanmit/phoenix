#pragma once

#include "scene_node.hpp"
#include "../math/frustum.hpp"
#include <vector>
#include <string>
#include <functional>
#include <memory>

namespace phoenix {
namespace scene {

// Forward declarations
class Octree;
class BVH;
class SceneSerializer;
class SceneDeserializer;

/**
 * @brief Scene statistics for profiling
 */
struct SceneStats {
    size_t totalNodes = 0;
    size_t visibleNodes = 0;
    size_t culledNodes = 0;
    size_t meshNodes = 0;
    size_t lightNodes = 0;
    size_t cameraNodes = 0;
    size_t dynamicNodes = 0;
    size_t staticNodes = 0;
    
    math::BoundingBox sceneBounds;
    math::BoundingSphere sceneSphere;
    
    double updateMs = 0.0;
    double cullMs = 0.0;
    double renderMs = 0.0;
};

/**
 * @brief Main scene class managing the scene graph
 * 
 * Provides:
 * - Root node for scene graph hierarchy
 * - Spatial acceleration structures (octree, BVH)
 * - Culling systems (frustum, occlusion, distance)
 * - Scene serialization (glTF compatible)
 * - Statistics and profiling
 */
class Scene {
public:
    using Ptr = std::shared_ptr<Scene>;
    using NodePtr = SceneNode::Ptr;
    using NodeList = std::vector<NodePtr>;
    using NodeCallback = std::function<void(SceneNode&)>;
    
    // ========================================================================
    // Construction
    // ========================================================================
    
    explicit Scene(const std::string& name = "Scene");
    virtual ~Scene();
    
    // Non-copyable, movable
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    Scene(Scene&&) noexcept;
    Scene& operator=(Scene&&) noexcept;
    
    // ========================================================================
    // Identity
    // ========================================================================
    
    const std::string& getName() const noexcept { return name_; }
    void setName(const std::string& name) noexcept { name_ = name; }
    
    // ========================================================================
    // Root Node
    // ========================================================================
    
    SceneNode* getRoot() noexcept { return root_.get(); }
    const SceneNode* getRoot() const noexcept { return root_.get(); }
    
    /**
     * @brief Set a new root node
     */
    void setRoot(NodePtr root);
    
    // ========================================================================
    // Node Management
    // ========================================================================
    
    /**
     * @brief Add node to scene root
     */
    void addNode(NodePtr node);
    
    /**
     * @brief Remove node from scene
     */
    void removeNode(NodePtr node);
    void removeNode(const std::string& name);
    
    /**
     * @brief Find node by name (recursive search)
     */
    SceneNode* findNode(const std::string& name);
    const SceneNode* findNode(const std::string& name) const;
    
    /**
     * @brief Find node by ID
     */
    SceneNode* findNodeById(uint32_t id);
    const SceneNode* findNodeById(uint32_t id) const;
    
    /**
     * @brief Get all nodes of a specific type
     */
    NodeList getNodesByType(NodeType type) const;
    
    /**
     * @brief Get all nodes matching a predicate
     */
    NodeList getNodesByPredicate(const std::function<bool(const SceneNode&)>& predicate) const;
    
    // ========================================================================
    // Traversal
    // ========================================================================
    
    /**
     * @brief Traverse entire scene with visitor
     */
    void traverse(SceneVisitor& visitor);
    
    /**
     * @brief Traverse with callback
     */
    void traverse(const NodeCallback& callback);
    
    /**
     * @brief Traverse with callback and max depth
     */
    void traverse(const NodeCallback& callback, int maxDepth);
    
    // ========================================================================
    // Transform Updates
    // ========================================================================
    
    /**
     * @brief Update all transforms in the scene
     * 
     * Call this before rendering or culling.
     */
    void updateTransforms();
    
    /**
     * @brief Update only dirty transforms
     * 
     * More efficient than updateTransforms() for incremental updates.
     */
    void updateDirtyTransforms();
    
    // ========================================================================
    // Spatial Acceleration
    // ========================================================================
    
    /**
     * @brief Build octree for spatial queries
     * 
     * @param maxDepth Maximum octree depth
     * @param minSize Minimum node size
     */
    void buildOctree(int maxDepth = 8, float minSize = 0.1f);
    
    /**
     * @brief Build BVH for ray tracing
     * 
     * @param useSAH Use Surface Area Heuristic for better quality
     */
    void buildBVH(bool useSAH = true);
    
    /**
     * @brief Update spatial structures for dynamic objects
     */
    void updateSpatialStructures();
    
    /**
     * @brief Get octree (if built)
     */
    Octree* getOctree() noexcept { return octree_.get(); }
    const Octree* getOctree() const noexcept { return octree_.get(); }
    
    /**
     * @brief Get BVH (if built)
     */
    BVH* getBVH() noexcept { return bvh_.get(); }
    const BVH* getBVH() const noexcept { return bvh_.get(); }
    
    // ========================================================================
    // Culling
    // ========================================================================
    
    /**
     * @brief Perform frustum culling
     * 
     * Marks nodes as culled if outside the view frustum.
     */
    void cullFrustum(const math::Frustum& frustum);
    
    /**
     * @brief Perform distance culling
     * 
     * Culls nodes beyond maxDistance from camera position.
     */
    void cullDistance(const math::Vector3& cameraPos, float maxDistance);
    
    /**
     * @brief Perform backface culling
     * 
     * Culls nodes facing away from camera.
     */
    void cullBackface(const math::Vector3& cameraPos, const math::Vector3& cameraDir);
    
    /**
     * @brief Reset culling state for all nodes
     */
    void resetCulling();
    
    /**
     * @brief Get visible nodes after culling
     */
    NodeList getVisibleNodes() const;
    
    // ========================================================================
    // Scene Bounds
    // ========================================================================
    
    /**
     * @brief Calculate scene bounding box
     */
    math::BoundingBox calculateBounds() const;
    
    /**
     * @brief Calculate scene bounding sphere
     */
    math::BoundingSphere calculateBoundingSphere() const;
    
    /**
     * @brief Get cached scene bounds
     */
    const math::BoundingBox& getBounds() const noexcept { return sceneBounds_; }
    const math::BoundingSphere& getBoundingSphere() const noexcept { return sceneSphere_; }
    
    // ========================================================================
    // Statistics
    // ========================================================================
    
    /**
     * @brief Update scene statistics
     */
    void updateStats();
    
    /**
     * @brief Get current statistics
     */
    const SceneStats& getStats() const noexcept { return stats_; }
    
    /**
     * @brief Get total node count
     */
    size_t getNodeCount() const noexcept { return stats_.totalNodes; }
    
    // ========================================================================
    // Serialization
    // ========================================================================
    
    /**
     * @brief Serialize scene to glTF format
     */
    bool saveToGlTF(const std::string& filename) const;
    
    /**
     * @brief Load scene from glTF format
     */
    bool loadFromGlTF(const std::string& filename);
    
    /**
     * @brief Serialize scene (binary)
     */
    void serialize(SceneSerializer& serializer) const;
    
    /**
     * @brief Deserialize scene (binary)
     */
    void deserialize(SceneDeserializer& deserializer);
    
    // ========================================================================
    // Utility
    // ========================================================================
    
    /**
     * @brief Clear entire scene
     */
    void clear();
    
    /**
     * @brief Merge another scene into this one
     */
    void merge(const Scene& other);
    
    /**
     * @brief Create a deep copy of the scene
     */
    Ptr clone() const;
    
    /**
     * @brief Optimize scene for rendering
     * 
     * - Sort nodes by material/state
     * - Batch static geometry
     * - Prepare LOD transitions
     */
    void optimize();
    
protected:
    std::string name_;
    NodePtr root_;
    
    // Spatial structures
    std::unique_ptr<Octree> octree_;
    std::unique_ptr<BVH> bvh_;
    
    // Cached bounds
    math::BoundingBox sceneBounds_;
    math::BoundingSphere sceneSphere_;
    bool boundsValid_;
    
    // Statistics
    SceneStats stats_;
    
    // Node lookup
    std::unordered_map<uint32_t, SceneNode*> nodeById_;
    std::unordered_map<std::string, SceneNode*> nodeByName_;
    
    void updateNodeLookup();
    void updateBounds();
    void collectStats(SceneNode& node);
};

} // namespace scene
} // namespace phoenix
