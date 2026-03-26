#pragma once

#include "../math/bounding.hpp"
#include "../math/vector3.hpp"
#include <vector>
#include <memory>
#include <cstdint>

namespace phoenix {
namespace scene {

class SceneNode;

/**
 * @brief Bounding Volume Hierarchy for ray tracing and spatial queries
 * 
 * Binary tree structure optimized for:
 * - Ray-scene intersection (ray tracing)
 * - Nearest neighbor queries
 * - Collision detection
 * 
 * Supports SAH (Surface Area Heuristic) for optimal tree construction.
 */
class BVH {
public:
    using Ptr = std::shared_ptr<BVH>;
    using ObjectList = std::vector<SceneNode*>;
    
    // ========================================================================
    // BVH Node (cache-line optimized)
    // ========================================================================
    
    struct alignas(32) Node {
        math::BoundingBox bounds;
        
        union {
            uint32_t firstChild;  // Internal: index of first child
            uint32_t objectIndex; // Leaf: index into object array
        };
        
        union {
            uint32_t objectCount; // Leaf: number of objects
            uint32_t dummy;       // Internal: padding
        };
        
        uint8_t axis;      // Split axis (0=x, 1=y, 2=z)
        bool isLeaf;
        uint16_t padding;
        
        Node() noexcept 
            : bounds()
            , firstChild(0)
            , objectCount(0)
            , axis(0)
            , isLeaf(true)
            , padding(0) {}
        
        bool isLeafNode() const noexcept { return isLeaf; }
        
        uint32_t getChildCount() const noexcept {
            return isLeaf ? 0 : 2;
        }
    };
    
    // ========================================================================
    // Construction
    // ========================================================================
    
    /**
     * @brief Build BVH from objects
     * 
     * @param objects Objects to include
     * @param useSAH Use Surface Area Heuristic for better quality
     * @param maxLeafSize Maximum objects per leaf
     */
    explicit BVH(ObjectList objects, bool useSAH = true, size_t maxLeafSize = 4);
    
    ~BVH() = default;
    
    // Non-copyable, movable
    BVH(const BVH&) = delete;
    BVH& operator=(const BVH&) = delete;
    BVH(BVH&&) noexcept = default;
    BVH& operator=(BVH&&) noexcept = default;
    
    // ========================================================================
    // Ray Casting
    // ========================================================================
    
    /**
     * @brief Ray cast result
     */
    struct RayHit {
        SceneNode* object;
        float t;              // Ray parameter (distance)
        math::Vector3 point;  // Hit point
        math::Vector3 normal; // Surface normal
        
        RayHit() noexcept : object(nullptr), t(FLT_MAX), point(), normal() {}
        
        bool operator<(const RayHit& other) const {
            return t < other.t;
        }
    };
    
    /**
     * @brief Cast ray and return closest hit
     * 
     * @param origin Ray origin
     * @param dir Ray direction (normalized)
     * @param tMin Minimum ray parameter
     * @param tMax Maximum ray parameter
     * @return Closest hit or empty result
     */
    RayHit raycast(const math::Vector3& origin, const math::Vector3& dir,
                   float tMin = 0.0f, float tMax = FLT_MAX) const;
    
    /**
     * @brief Cast ray and return all hits
     * 
     * @return Hits sorted by distance
     */
    std::vector<RayHit> raycastAll(const math::Vector3& origin, const math::Vector3& dir,
                                    float tMin = 0.0f, float tMax = FLT_MAX) const;
    
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
    
    // ========================================================================
    // Properties
    // ========================================================================
    
    size_t getNodeCount() const noexcept { return nodes_.size(); }
    size_t getObjectCount() const noexcept { return objects_.size(); }
    size_t getLeafCount() const noexcept { return leafCount_; }
    
    int getDepth() const noexcept { return depth_; }
    bool usesSAH() const noexcept { return usedSAH_; }
    
    const Node* getRoot() const noexcept {
        return nodes_.empty() ? nullptr : &nodes_[0];
    }
    
    /**
     * @brief Get memory usage in bytes
     */
    size_t getMemoryUsage() const noexcept {
        return nodes_.capacity() * sizeof(Node) + 
               objects_.capacity() * sizeof(SceneNode*);
    }
    
    // ========================================================================
    // Statistics
    // ========================================================================
    
    struct Stats {
        size_t totalNodes = 0;
        size_t leafNodes = 0;
        int maxDepth = 0;
        size_t totalObjects = 0;
        size_t maxObjectsInLeaf = 0;
        float avgObjectsPerLeaf = 0.0f;
        float sahCost = 0.0f; // Surface Area Heuristic cost
    };
    
    Stats getStats() const;
    
    /**
     * @brief Calculate SAH cost for the tree
     */
    float calculateSAHCost() const;
    
private:
    std::vector<Node> nodes_;
    ObjectList objects_;
    std::vector<math::BoundingBox> objectBounds_;
    size_t leafCount_;
    int depth_;
    bool usedSAH_;
    size_t maxLeafSize_;
    
    // Build helper structures
    struct BuildEntry {
        uint32_t nodeIndex;
        uint32_t objectStart;
        uint32_t objectCount;
        math::BoundingBox bounds;
    };
    
    struct CentroidBounds {
        math::BoundingBox bounds;
        
        void extend(const math::Vector3& centroid) {
            bounds.extend(centroid);
        }
    };
    
    uint32_t buildSAH(ObjectList& objects, std::vector<math::BoundingBox>& bounds,
                      uint32_t start, uint32_t count);
    
    uint32_t buildSimple(ObjectList& objects, std::vector<math::BoundingBox>& bounds,
                         uint32_t start, uint32_t count, int depth);
    
    struct SplitInfo {
        int axis;
        uint32_t position;
        float cost;
        
        SplitInfo() : axis(0), position(0), cost(FLT_MAX) {}
    };
    
    SplitInfo findBestSplit(const ObjectList& objects, 
                            const std::vector<math::BoundingBox>& bounds,
                            uint32_t start, uint32_t count,
                            const math::BoundingBox& bounds);
    
    bool rayBoxIntersect(const math::BoundingBox& box, const math::Vector3& origin,
                         const math::Vector3& invDir, float& tMin, float& tMax) const;
    
    void raycastRecursive(uint32_t nodeIndex, const math::Vector3& origin,
                          const math::Vector3& invDir, float& tMin, float& tMax,
                          RayHit& hit) const;
    
    void raycastAllRecursive(uint32_t nodeIndex, const math::Vector3& origin,
                             const math::Vector3& invDir, float tMin, float tMax,
                             std::vector<RayHit>& hits) const;
};

} // namespace scene
} // namespace phoenix
