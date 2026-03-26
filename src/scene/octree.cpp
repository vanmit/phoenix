#include "../../include/phoenix/scene/octree.hpp"
#include "../../include/phoenix/scene/scene_node.hpp"
#include <algorithm>
#include <cmath>
#include <queue>

namespace phoenix {
namespace scene {

// ============================================================================
// Octree Implementation
// ============================================================================

Octree::Octree(const math::BoundingBox& bounds, int maxDepth, 
               float minSize, size_t maxObjectsPerNode)
    : root_(new Node(bounds, 0))
    , maxDepth_(maxDepth)
    , minSize_(minSize)
    , maxObjectsPerNode_(maxObjectsPerNode)
    , totalObjects_(0)
    , nodeCount_(1)
    , objectNodeMap_() {
}

void Octree::insert(SceneNode* obj, const math::BoundingBox& bounds) {
    if (!root_ || !bounds.isValid()) return;
    
    // Check if bounds fit in root
    if (!root_->bounds.intersects(bounds)) {
        // Expand root if necessary
        math::BoundingBox newBounds = root_->bounds;
        newBounds.extend(bounds);
        
        Node* newRoot = new Node(newBounds, 0);
        newRoot->children[0] = root_;
        root_->parent = newRoot;
        root_->depth = 1;
        root_ = newRoot;
        ++nodeCount_;
    }
    
    insertIntoNode(root_, obj, bounds);
}

void Octree::insertIntoNode(Node* node, SceneNode* obj, const math::BoundingBox& bounds) {
    // If node has children, try to insert into them
    if (node->hasChildren()) {
        bool inserted = false;
        
        for (auto* child : node->children) {
            if (child && intersectsNode(child, bounds)) {
                insertIntoNode(child, obj, bounds);
                inserted = true;
            }
        }
        
        if (inserted) return;
    }
    
    // Insert into this node
    node->objects.push_back(obj);
    node->objectCount = static_cast<uint16_t>(node->objects.size());
    objectNodeMap_[obj] = node;
    ++totalObjects_;
    
    // Subdivide if necessary
    if (node->isLeaf && 
        node->depth < static_cast<uint8_t>(maxDepth_) &&
        node->objects.size() > maxObjectsPerNode_) {
        
        const float minDim = std::min({
            node->bounds.size().x,
            node->bounds.size().y,
            node->bounds.size().z
        });
        
        if (minDim > minSize_) {
            subdivide(node);
        }
    }
}

void Octree::subdivide(Node* node) {
    if (!node->isLeaf) return;
    
    const math::Vector3 center = node->bounds.center();
    const math::Vector3 extents = node->bounds.extents();
    
    // Create 8 children
    for (int i = 0; i < 8; ++i) {
        math::Vector3 minCorner, maxCorner;
        
        // Determine octant
        minCorner.x = (i & 1) ? center.x : node->bounds.min.x;
        minCorner.y = (i & 2) ? center.y : node->bounds.min.y;
        minCorner.z = (i & 4) ? center.z : node->bounds.min.z;
        
        maxCorner.x = (i & 1) ? node->bounds.max.x : center.x;
        maxCorner.y = (i & 2) ? node->bounds.max.y : center.y;
        maxCorner.z = (i & 4) ? node->bounds.max.z : center.z;
        
        Node* child = new Node(math::BoundingBox(minCorner, maxCorner), node->depth + 1);
        child->parent = node;
        node->children[i] = child;
        ++nodeCount_;
    }
    
    node->isLeaf = false;
    
    // Redistribute objects to children
    ObjectList objectsToRedistribute = std::move(node->objects);
    node->objects.clear();
    node->objectCount = 0;
    
    for (SceneNode* obj : objectsToRedistribute) {
        // Find object's bounds (would need to store this)
        // For now, keep in parent node
        node->objects.push_back(obj);
    }
}

void Octree::remove(SceneNode* obj) {
    auto it = objectNodeMap_.find(obj);
    if (it == objectNodeMap_.end()) return;
    
    Node* node = it->second;
    
    auto objIt = std::find(node->objects.begin(), node->objects.end(), obj);
    if (objIt != node->objects.end()) {
        node->objects.erase(objIt);
        node->objectCount = static_cast<uint16_t>(node->objects.size());
        --totalObjects_;
    }
    
    objectNodeMap_.erase(it);
}

void Octree::update(SceneNode* obj, const math::BoundingBox& newBounds) {
    remove(obj);
    insert(obj, newBounds);
}

Octree::ObjectList Octree::query(const math::BoundingBox& bounds) const {
    ObjectList result;
    
    if (!root_ || !root_->bounds.intersects(bounds)) {
        return result;
    }
    
    queryRecursive(root_, bounds, [&result](SceneNode* obj) {
        result.push_back(obj);
    });
    
    return result;
}

Octree::ObjectList Octree::query(const math::BoundingSphere& sphere) const {
    ObjectList result;
    
    if (!root_ || !root_->bounds.intersectsSphere(sphere.center, sphere.radius)) {
        return result;
    }
    
    queryRecursive(root_, sphere, [&result](SceneNode* obj) {
        result.push_back(obj);
    });
    
    return result;
}

Octree::ObjectList Octree::query(const math::Vector3& point) const {
    ObjectList result;
    
    if (!root_ || !root_->bounds.contains(point)) {
        return result;
    }
    
    // Find leaf containing point
    const Node* current = root_;
    while (current->hasChildren()) {
        bool found = false;
        
        for (auto* child : current->children) {
            if (child && child->bounds.contains(point)) {
                current = child;
                found = true;
                break;
            }
        }
        
        if (!found) break;
    }
    
    // Collect objects from this node and all parents
    while (current) {
        result.insert(result.end(), current->objects.begin(), current->objects.end());
        current = current->parent;
    }
    
    return result;
}

void Octree::query(const math::BoundingBox& bounds, const QueryCallback& callback) const {
    if (!root_ || !root_->bounds.intersects(bounds)) return;
    queryRecursive(root_, bounds, callback);
}

void Octree::queryRecursive(const Node* node, const math::BoundingBox& bounds,
                            const QueryCallback& callback) const {
    if (!node || !intersectsNode(node, bounds)) return;
    
    // Report objects in this node
    for (SceneNode* obj : node->objects) {
        callback(obj);
    }
    
    // Recurse into children
    if (node->hasChildren()) {
        for (auto* child : node->children) {
            queryRecursive(child, bounds, callback);
        }
    }
}

void Octree::queryRecursive(const Node* node, const math::BoundingSphere& sphere,
                            const QueryCallback& callback) const {
    if (!node || !intersectsNode(node, sphere)) return;
    
    for (SceneNode* obj : node->objects) {
        callback(obj);
    }
    
    if (node->hasChildren()) {
        for (auto* child : node->children) {
            queryRecursive(child, sphere, callback);
        }
    }
}

Octree::ObjectList Octree::raycast(const math::Vector3& rayOrigin,
                                    const math::Vector3& rayDir,
                                    float maxDist) const {
    ObjectList results;
    
    if (!root_) return results;
    
    raycastRecursive(root_, rayOrigin, rayDir, maxDist, results);
    
    // Sort by distance (would need actual intersection distances)
    // For now, just return in traversal order
    
    return results;
}

void Octree::raycastRecursive(const Node* node, const math::Vector3& origin,
                              const math::Vector3& dir, float maxDist,
                              ObjectList& results) const {
    if (!node) return;
    
    // Simple ray-box intersection test
    const math::Vector3 invDir(
        dir.x != 0 ? 1.0f / dir.x : FLT_MAX,
        dir.y != 0 ? 1.0f / dir.y : FLT_MAX,
        dir.z != 0 ? 1.0f / dir.z : FLT_MAX
    );
    
    const float t1 = (node->bounds.min.x - origin.x) * invDir.x;
    const float t2 = (node->bounds.max.x - origin.x) * invDir.x;
    const float t3 = (node->bounds.min.y - origin.y) * invDir.y;
    const float t4 = (node->bounds.max.y - origin.y) * invDir.y;
    const float t5 = (node->bounds.min.z - origin.z) * invDir.z;
    const float t6 = (node->bounds.max.z - origin.z) * invDir.z;
    
    const float tmin = std::max({std::min(t1, t2), std::min(t3, t4), std::min(t5, t6)});
    const float tmax = std::min({std::max(t1, t2), std::max(t3, t4), std::max(t5, t6)});
    
    // Ray misses box
    if (tmax < 0 || tmin > tmax || tmin > maxDist) return;
    
    // Report objects in this node
    for (SceneNode* obj : node->objects) {
        results.push_back(obj);
    }
    
    // Recurse into children (could optimize with sorted traversal)
    if (node->hasChildren()) {
        for (auto* child : node->children) {
            raycastRecursive(child, origin, dir, maxDist, results);
        }
    }
}

bool Octree::intersectsNode(const Node* node, const math::BoundingBox& bounds) const {
    return node->bounds.intersects(bounds);
}

bool Octree::intersectsNode(const Node* node, const math::BoundingSphere& sphere) const {
    return node->bounds.intersectsSphere(sphere.center, sphere.radius);
}

size_t Octree::getMemoryUsage() const {
    size_t bytes = sizeof(Octree);
    bytes += nodeCount_ * sizeof(Node);
    
    // Object list overhead
    bytes += totalObjects_ * sizeof(SceneNode*);
    
    // Map overhead
    bytes += objectNodeMap_.size() * (sizeof(SceneNode*) + sizeof(Node*) + 16);
    
    return bytes;
}

std::vector<const Octree::Node*> Octree::getLeafNodes() const {
    std::vector<const Node*> leaves;
    
    if (!root_) return leaves;
    
    std::queue<const Node*> queue;
    queue.push(root_);
    
    while (!queue.empty()) {
        const Node* node = queue.front();
        queue.pop();
        
        if (node->isLeaf) {
            leaves.push_back(node);
        } else {
            for (auto* child : node->children) {
                if (child) queue.push(child);
            }
        }
    }
    
    return leaves;
}

Octree::Stats Octree::getStats() const {
    Stats stats;
    
    if (!root_) return stats;
    
    std::queue<const Node*> queue;
    queue.push(root_);
    
    while (!queue.empty()) {
        const Node* node = queue.front();
        queue.pop();
        
        ++stats.totalNodes;
        
        if (node->isLeaf) {
            ++stats.leafNodes;
            stats.totalObjects += node->objects.size();
            stats.maxObjectsInNode = std::max(stats.maxObjectsInNode, node->objects.size());
        }
        
        stats.maxDepth = std::max(stats.maxDepth, static_cast<size_t>(node->depth));
        
        if (node->hasChildren()) {
            for (auto* child : node->children) {
                if (child) queue.push(child);
            }
        }
    }
    
    if (stats.leafNodes > 0) {
        stats.avgObjectsPerLeaf = static_cast<float>(stats.totalObjects) / stats.leafNodes;
    }
    
    return stats;
}

} // namespace scene
} // namespace phoenix
