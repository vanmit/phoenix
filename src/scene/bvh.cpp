#include "../../include/phoenix/scene/bvh.hpp"
#include "../../include/phoenix/scene/scene_node.hpp"
#include <algorithm>
#include <numeric>
#include <stack>

namespace phoenix {
namespace scene {

// ============================================================================
// BVH Implementation
// ============================================================================

BVH::BVH(ObjectList objects, bool useSAH, size_t maxLeafSize)
    : nodes_()
    , objects_(std::move(objects))
    , objectBounds_()
    , leafCount_(0)
    , depth_(0)
    , usedSAH_(useSAH)
    , maxLeafSize_(maxLeafSize) {
    
    if (objects_.empty()) return;
    
    // Compute bounding boxes for all objects
    objectBounds_.resize(objects_.size());
    for (size_t i = 0; i < objects_.size(); ++i) {
        objectBounds_[i] = objects_[i]->getWorldBoundingBox();
    }
    
    // Reserve space (worst case: 2n-1 nodes for n objects)
    nodes_.reserve(objects_.size() * 2 - 1);
    
    // Build tree
    if (useSAH) {
        buildSAH(objects_, objectBounds_, 0, static_cast<uint32_t>(objects_.size()));
    } else {
        buildSimple(objects_, objectBounds_, 0, static_cast<uint32_t>(objects_.size()), 0);
    }
}

uint32_t BVH::buildSAH(ObjectList& objects, std::vector<math::BoundingBox>& bounds,
                       uint32_t start, uint32_t count) {
    // Compute bounds for this node
    math::BoundingBox nodeBounds = math::BoundingBox::empty();
    for (uint32_t i = start; i < start + count; ++i) {
        nodeBounds.extend(bounds[i]);
    }
    
    // Create node
    uint32_t nodeIndex = static_cast<uint32_t>(nodes_.size());
    nodes_.emplace_back();
    Node& node = nodes_.back();
    node.bounds = nodeBounds;
    node.isLeaf = false;
    
    // Find best split using SAH
    SplitInfo split = findBestSplit(objects, bounds, start, count, nodeBounds);
    
    // Create leaf if no good split found or max depth reached
    if (split.cost >= count * nodeBounds.surfaceArea() || 
        count <= maxLeafSize_) {
        node.isLeaf = true;
        node.objectIndex = start;
        node.objectCount = count;
        ++leafCount_;
        return nodeIndex;
    }
    
    // Partition objects based on split
    const int axis = split.axis;
    const float splitPos = nodeBounds.min[axis] + 
                          nodeBounds.size()[axis] * (split.position / static_cast<float>(count));
    
    uint32_t left = start;
    uint32_t right = start + count - 1;
    
    while (left <= right) {
        const float centroid = (bounds[left].min[axis] + bounds[left].max[axis]) * 0.5f;
        if (centroid < splitPos) {
            ++left;
        } else {
            std::swap(objects[left], objects[right]);
            std::swap(bounds[left], bounds[right]);
            --right;
        }
    }
    
    const uint32_t mid = left;
    const uint32_t leftCount = mid - start;
    const uint32_t rightCount = start + count - mid;
    
    // Handle edge case where all objects go to one side
    if (leftCount == 0 || rightCount == 0) {
        node.isLeaf = true;
        node.objectIndex = start;
        node.objectCount = count;
        ++leafCount_;
        return nodeIndex;
    }
    
    // Recursively build children
    node.axis = static_cast<uint8_t>(axis);
    node.firstChild = static_cast<uint32_t>(nodes_.size());
    
    buildSAH(objects, bounds, start, leftCount);
    buildSAH(objects, bounds, mid, rightCount);
    
    return nodeIndex;
}

uint32_t BVH::buildSimple(ObjectList& objects, std::vector<math::BoundingBox>& bounds,
                          uint32_t start, uint32_t count, int depth) {
    // Compute bounds
    math::BoundingBox nodeBounds = math::BoundingBox::empty();
    for (uint32_t i = start; i < start + count; ++i) {
        nodeBounds.extend(bounds[i]);
    }
    
    // Create node
    uint32_t nodeIndex = static_cast<uint32_t>(nodes_.size());
    nodes_.emplace_back();
    Node& node = nodes_.back();
    node.bounds = nodeBounds;
    node.isLeaf = false;
    
    depth_ = std::max(depth_, depth);
    
    // Create leaf if small enough
    if (count <= maxLeafSize_) {
        node.isLeaf = true;
        node.objectIndex = start;
        node.objectCount = count;
        ++leafCount_;
        return nodeIndex;
    }
    
    // Find longest axis
    const math::Vector3 extent = nodeBounds.size();
    int axis = 0;
    if (extent.y > extent.x && extent.y > extent.z) axis = 1;
    else if (extent.z > extent.x) axis = 2;
    
    // Sort by centroid on split axis
    std::sort(objects.begin() + start, objects.begin() + start + count,
        [&bounds, axis](SceneNode* a, SceneNode* b) {
            const size_t ia = &a - &objects[0];
            const size_t ib = &b - &objects[0];
            const float ca = (bounds[ia].min[axis] + bounds[ia].max[axis]) * 0.5f;
            const float cb = (bounds[ib].min[axis] + bounds[ib].max[axis]) * 0.5f;
            return ca < cb;
        });
    
    std::sort(bounds.begin() + start, bounds.begin() + start + count,
        [axis](const math::BoundingBox& a, const math::BoundingBox& b) {
            const float ca = (a.min[axis] + a.max[axis]) * 0.5f;
            const float cb = (b.min[axis] + b.max[axis]) * 0.5f;
            return ca < cb;
        });
    
    // Split in half
    const uint32_t mid = start + count / 2;
    const uint32_t leftCount = mid - start;
    const uint32_t rightCount = start + count - mid;
    
    node.axis = static_cast<uint8_t>(axis);
    node.firstChild = static_cast<uint32_t>(nodes_.size());
    
    buildSimple(objects, bounds, start, leftCount, depth + 1);
    buildSimple(objects, bounds, mid, rightCount, depth + 1);
    
    return nodeIndex;
}

BVH::SplitInfo BVH::findBestSplit(const ObjectList& objects,
                                   const std::vector<math::BoundingBox>& bounds,
                                   uint32_t start, uint32_t count,
                                   const math::BoundingBox& nodeBounds) {
    SplitInfo best;
    
    const float nodeSA = nodeBounds.surfaceArea();
    if (nodeSA < 1e-8f) return best;
    
    // Constants for SAH
    constexpr float traversalCost = 1.0f;
    constexpr float intersectionCost = 1.0f;
    
    // Try each axis
    for (int axis = 0; axis < 3; ++axis) {
        // Compute bounds for left and right partitions
        std::vector<math::BoundingBox> leftBounds(count - 1);
        std::vector<math::BoundingBox> rightBounds(count - 1);
        
        math::BoundingBox leftAccum = math::BoundingBox::empty();
        for (uint32_t i = 0; i < count - 1; ++i) {
            leftAccum.extend(bounds[start + i]);
            leftBounds[i] = leftAccum;
        }
        
        math::BoundingBox rightAccum = math::BoundingBox::empty();
        for (uint32_t i = count - 1; i > 0; --i) {
            rightAccum.extend(bounds[start + i]);
            rightBounds[i - 1] = rightAccum;
        }
        
        // Evaluate split positions
        for (uint32_t i = 1; i < count; ++i) {
            const float leftSA = leftBounds[i - 1].surfaceArea();
            const float rightSA = rightBounds[i].surfaceArea();
            
            const float cost = traversalCost + 
                              intersectionCost * (
                                  leftSA * i + 
                                  rightSA * (count - i)
                              ) / nodeSA;
            
            if (cost < best.cost) {
                best.axis = axis;
                best.position = i;
                best.cost = cost;
            }
        }
    }
    
    return best;
}

BVH::RayHit BVH::raycast(const math::Vector3& origin, const math::Vector3& dir,
                         float tMin, float tMax) const {
    if (nodes_.empty()) return RayHit();
    
    const math::Vector3 invDir(
        dir.x != 0 ? 1.0f / dir.x : FLT_MAX,
        dir.y != 0 ? 1.0f / dir.y : FLT_MAX,
        dir.z != 0 ? 1.0f / dir.z : FLT_MAX
    );
    
    RayHit hit;
    hit.t = tMax;
    
    raycastRecursive(0, origin, invDir, tMin, hit.t, hit);
    
    if (hit.object) {
        hit.point = origin + dir * hit.t;
    }
    
    return hit;
}

std::vector<BVH::RayHit> BVH::raycastAll(const math::Vector3& origin, 
                                          const math::Vector3& dir,
                                          float tMin, float tMax) const {
    std::vector<RayHit> hits;
    
    if (nodes_.empty()) return hits;
    
    const math::Vector3 invDir(
        dir.x != 0 ? 1.0f / dir.x : FLT_MAX,
        dir.y != 0 ? 1.0f / dir.y : FLT_MAX,
        dir.z != 0 ? 1.0f / dir.z : FLT_MAX
    );
    
    raycastAllRecursive(0, origin, invDir, tMin, tMax, hits);
    
    std::sort(hits.begin(), hits.end());
    
    return hits;
}

void BVH::raycastRecursive(uint32_t nodeIndex, const math::Vector3& origin,
                           const math::Vector3& invDir, float& tMin, float& tMax,
                           RayHit& hit) const {
    const Node& node = nodes_[nodeIndex];
    
    // Ray-box intersection
    float t0, t1;
    if (!rayBoxIntersect(node.bounds, origin, invDir, t0, t1)) return;
    
    t0 = std::max(t0, tMin);
    t1 = std::min(t1, tMax);
    
    if (t0 > t1) return;
    
    if (node.isLeaf) {
        // Test all objects in leaf
        for (uint32_t i = 0; i < node.objectCount; ++i) {
            SceneNode* obj = objects_[node.objectIndex + i];
            const auto& bounds = objectBounds_[node.objectIndex + i];
            
            float objT0, objT1;
            if (rayBoxIntersect(bounds, origin, invDir, objT0, objT1)) {
                objT0 = std::max(objT0, tMin);
                objT1 = std::min(objT1, tMax);
                
                if (objT0 < hit.t) {
                    hit.object = obj;
                    hit.t = objT0;
                }
            }
        }
    } else {
        // Visit children (could optimize with distance sorting)
        raycastRecursive(node.firstChild, origin, invDir, t0, t1, hit);
        raycastRecursive(node.firstChild + 1, origin, invDir, t0, t1, hit);
    }
}

void BVH::raycastAllRecursive(uint32_t nodeIndex, const math::Vector3& origin,
                              const math::Vector3& invDir, float tMin, float tMax,
                              std::vector<RayHit>& hits) const {
    const Node& node = nodes_[nodeIndex];
    
    float t0, t1;
    if (!rayBoxIntersect(node.bounds, origin, invDir, t0, t1)) return;
    
    t0 = std::max(t0, tMin);
    t1 = std::min(t1, tMax);
    
    if (t0 > t1) return;
    
    if (node.isLeaf) {
        for (uint32_t i = 0; i < node.objectCount; ++i) {
            SceneNode* obj = objects_[node.objectIndex + i];
            const auto& bounds = objectBounds_[node.objectIndex + i];
            
            float objT0, objT1;
            if (rayBoxIntersect(bounds, origin, invDir, objT0, objT1)) {
                objT0 = std::max(objT0, tMin);
                objT1 = std::min(objT1, tMax);
                
                if (objT0 < objT1) {
                    RayHit hit;
                    hit.object = obj;
                    hit.t = objT0;
                    hit.point = origin + math::Vector3(dir.x * objT0, dir.y * objT0, dir.z * objT0);
                    hits.push_back(hit);
                }
            }
        }
    } else {
        raycastAllRecursive(node.firstChild, origin, invDir, t0, t1, hits);
        raycastAllRecursive(node.firstChild + 1, origin, invDir, t0, t1, hits);
    }
}

bool BVH::rayBoxIntersect(const math::BoundingBox& box, const math::Vector3& origin,
                          const math::Vector3& invDir, float& tMin, float& tMax) const {
    const float t1 = (box.min.x - origin.x) * invDir.x;
    const float t2 = (box.max.x - origin.x) * invDir.x;
    const float t3 = (box.min.y - origin.y) * invDir.y;
    const float t4 = (box.max.y - origin.y) * invDir.y;
    const float t5 = (box.min.z - origin.z) * invDir.z;
    const float t6 = (box.max.z - origin.z) * invDir.z;
    
    tMin = std::max({std::min(t1, t2), std::min(t3, t4), std::min(t5, t6)});
    tMax = std::min({std::max(t1, t2), std::max(t3, t4), std::max(t5, t6)});
    
    return tMax >= tMin && tMax >= 0;
}

BVH::ObjectList BVH::query(const math::BoundingBox& bounds) const {
    ObjectList result;
    
    if (nodes_.empty() || !nodes_[0].bounds.intersects(bounds)) {
        return result;
    }
    
    std::stack<uint32_t> stack;
    stack.push(0);
    
    while (!stack.empty()) {
        const uint32_t nodeIndex = stack.top();
        stack.pop();
        
        const Node& node = nodes_[nodeIndex];
        
        if (!node.bounds.intersects(bounds)) continue;
        
        if (node.isLeaf) {
            for (uint32_t i = 0; i < node.objectCount; ++i) {
                if (objectBounds_[node.objectIndex + i].intersects(bounds)) {
                    result.push_back(objects_[node.objectIndex + i]);
                }
            }
        } else {
            stack.push(node.firstChild);
            stack.push(node.firstChild + 1);
        }
    }
    
    return result;
}

BVH::ObjectList BVH::query(const math::BoundingSphere& sphere) const {
    ObjectList result;
    
    if (nodes_.empty() || !nodes_[0].bounds.intersectsSphere(sphere.center, sphere.radius)) {
        return result;
    }
    
    std::stack<uint32_t> stack;
    stack.push(0);
    
    while (!stack.empty()) {
        const uint32_t nodeIndex = stack.top();
        stack.pop();
        
        const Node& node = nodes_[nodeIndex];
        
        if (!node.bounds.intersectsSphere(sphere.center, sphere.radius)) continue;
        
        if (node.isLeaf) {
            for (uint32_t i = 0; i < node.objectCount; ++i) {
                if (objectBounds_[node.objectIndex + i].intersectsSphere(sphere.center, sphere.radius)) {
                    result.push_back(objects_[node.objectIndex + i]);
                }
            }
        } else {
            stack.push(node.firstChild);
            stack.push(node.firstChild + 1);
        }
    }
    
    return result;
}

BVH::ObjectList BVH::query(const math::Vector3& point) const {
    ObjectList result;
    
    if (nodes_.empty() || !nodes_[0].bounds.contains(point)) {
        return result;
    }
    
    std::stack<uint32_t> stack;
    stack.push(0);
    
    while (!stack.empty()) {
        const uint32_t nodeIndex = stack.top();
        stack.pop();
        
        const Node& node = nodes_[nodeIndex];
        
        if (!node.bounds.contains(point)) continue;
        
        if (node.isLeaf) {
            for (uint32_t i = 0; i < node.objectCount; ++i) {
                if (objectBounds_[node.objectIndex + i].contains(point)) {
                    result.push_back(objects_[node.objectIndex + i]);
                }
            }
        } else {
            stack.push(node.firstChild);
            stack.push(node.firstChild + 1);
        }
    }
    
    return result;
}

BVH::Stats BVH::getStats() const {
    Stats stats;
    
    if (nodes_.empty()) return stats;
    
    stats.totalNodes = nodes_.size();
    stats.totalObjects = objects_.size();
    
    // BFS to compute depth and leaf stats
    std::stack<std::pair<uint32_t, int>> stack;
    stack.push({0, 0});
    
    while (!stack.empty()) {
        auto [nodeIndex, depth] = stack.top();
        stack.pop();
        
        const Node& node = nodes_[nodeIndex];
        
        if (node.isLeaf) {
            ++stats.leafNodes;
            stats.maxObjectsInLeaf = std::max(stats.maxObjectsInLeaf, 
                                               static_cast<size_t>(node.objectCount));
        }
        
        stats.maxDepth = std::max(stats.maxDepth, depth);
        
        if (!node.isLeaf) {
            stack.push({node.firstChild, depth + 1});
            stack.push({node.firstChild + 1, depth + 1});
        }
    }
    
    if (stats.leafNodes > 0) {
        stats.avgObjectsPerLeaf = static_cast<float>(stats.totalObjects) / stats.leafNodes;
    }
    
    stats.sahCost = calculateSAHCost();
    
    return stats;
}

float BVH::calculateSAHCost() const {
    if (nodes_.empty()) return 0.0f;
    
    constexpr float traversalCost = 1.0f;
    constexpr float intersectionCost = 1.0f;
    
    std::stack<uint32_t> stack;
    stack.push(0);
    
    float totalCost = 0.0f;
    
    while (!stack.empty()) {
        const uint32_t nodeIndex = stack.top();
        stack.pop();
        
        const Node& node = nodes_[nodeIndex];
        const float nodeSA = node.bounds.surfaceArea();
        
        if (node.isLeaf) {
            totalCost += intersectionCost * node.objectCount * nodeSA;
        } else {
            totalCost += traversalCost * nodeSA;
            stack.push(node.firstChild);
            stack.push(node.firstChild + 1);
        }
    }
    
    const float rootSA = nodes_[0].bounds.surfaceArea();
    return (rootSA > 1e-8f) ? totalCost / rootSA : 0.0f;
}

} // namespace scene
} // namespace phoenix
