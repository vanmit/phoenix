#include "../../include/phoenix/resource/point_cloud.hpp"
#include <algorithm>
#include <cmath>
#include <queue>

namespace phoenix::resource {

// ============ PointCloud ============

size_t PointCloud::calculateMemoryUsage() const {
    size_t total = points.size() * sizeof(PointCloudPoint);
    total += octree.size() * sizeof(OctreeNode);
    return total;
}

void PointCloud::buildOctree(uint8_t maxDepth, uint32_t maxPointsPerNode) {
    if (points.empty()) return;
    
    octree.clear();
    octree.reserve(1024);
    
    // Calculate bounds
    math::Vector3 minPos(FLT_MAX);
    math::Vector3 maxPos(-FLT_MAX);
    
    for (const auto& pt : points) {
        minPos = math::Vector3::min(minPos, pt.position);
        maxPos = math::Vector3::max(maxPos, pt.position);
    }
    
    math::Vector3 center = (minPos + maxPos) * 0.5f;
    float size = math::length(maxPos - minPos) * 0.5f;
    
    // Create root node
    OctreeNode root;
    root.center = center;
    root.size = size;
    root.pointStart = 0;
    root.pointCount = static_cast<uint32_t>(points.size());
    root.depth = 0;
    
    octree.push_back(root);
    
    // Build octree recursively
    std::vector<uint32_t> pointIndices(points.size());
    for (size_t i = 0; i < points.size(); ++i) {
        pointIndices[i] = static_cast<uint32_t>(i);
    }
    
    buildOctreeRecursive(0, pointIndices, maxDepth, maxPointsPerNode);
}

void PointCloud::buildOctreeRecursive(uint32_t nodeIndex, 
                                       const std::vector<uint32_t>& pointIndices,
                                       uint8_t maxDepth,
                                       uint32_t maxPointsPerNode) {
    if (nodeIndex >= octree.size()) return;
    
    OctreeNode& node = octree[nodeIndex];
    
    // Stop conditions
    if (node.depth >= maxDepth || pointIndices.size() <= maxPointsPerNode) {
        node.pointCount = static_cast<uint32_t>(pointIndices.size());
        return;
    }
    
    // Calculate child centers
    float halfSize = node.size * 0.5f;
    
    // Partition points into children
    std::vector<std::vector<uint32_t>> childPoints(8);
    
    for (uint32_t idx : pointIndices) {
        const auto& pt = points[idx];
        
        int childIndex = 0;
        if (pt.position.x >= node.center.x) childIndex |= 1;
        if (pt.position.y >= node.center.y) childIndex |= 2;
        if (pt.position.z >= node.center.z) childIndex |= 4;
        
        childPoints[childIndex].push_back(idx);
    }
    
    // Create child nodes
    for (int i = 0; i < 8; ++i) {
        if (childPoints[i].empty()) continue;
        
        OctreeNode child;
        child.parent = nodeIndex;
        child.depth = node.depth + 1;
        child.size = halfSize;
        
        // Calculate child center
        child.center.x = node.center.x + ((i & 1) ? halfSize : -halfSize);
        child.center.y = node.center.y + ((i & 2) ? halfSize : -halfSize);
        child.center.z = node.center.z + ((i & 4) ? halfSize : -halfSize);
        
        uint32_t childIndex = static_cast<uint32_t>(octree.size());
        octree.push_back(child);
        node.children[i] = childIndex;
        
        buildOctreeRecursive(childIndex, childPoints[i], maxDepth, maxPointsPerNode);
    }
}

std::vector<const PointCloudPoint*> PointCloud::getVisiblePoints(
    const math::Vector3& cameraPos,
    const math::Vector3& cameraDir,
    float fov,
    float quality,
    uint32_t maxPoints
) const {
    std::vector<const PointCloudPoint*> result;
    result.reserve(std::min(static_cast<size_t>(maxPoints), points.size()));
    
    if (points.empty()) return result;
    
    // Calculate LOD based on quality
    PointCloudLOD lod = calculateLOD(math::length(cameraPos - bounds.center), quality);
    
    // Simple frustum culling approach
    float cosHalfFov = std::cos(fov * 0.5f);
    
    // Collect points from octree
    std::vector<uint32_t> nodeQueue = {rootIndex};
    
    while (!nodeQueue.empty() && result.size() < maxPoints) {
        uint32_t nodeIdx = nodeQueue.back();
        nodeQueue.pop_back();
        
        const OctreeNode& node = octree[nodeIdx];
        
        // Distance check
        float distToNode = math::length(cameraPos - node.center);
        float nodeRadius = node.size * 0.866f;  // sqrt(3)/2 for cube
        
        if (distToNode - nodeRadius > lod.distanceThreshold) {
            continue;  // Too far
        }
        
        // Frustum check (simplified)
        math::Vector3 toNode = node.center - cameraPos;
        float dotProduct = math::dot(math::normalize(toNode), cameraDir);
        
        if (dotProduct < cosHalfFov) {
            continue;  // Outside frustum
        }
        
        if (node.isLeaf()) {
            // Add points from leaf node
            uint32_t count = 0;
            uint32_t stride = std::max(1u, static_cast<uint32_t>(1.0f / lod.pointDensity));
            
            for (uint32_t i = node.pointStart; 
                 i < node.pointStart + node.pointCount && count < maxPoints - result.size();
                 i += stride) {
                result.push_back(&points[i]);
                ++count;
            }
        } else {
            // Add children to queue
            for (int i = 0; i < 8; ++i) {
                if (node.children[i] != UINT32_MAX) {
                    nodeQueue.push_back(node.children[i]);
                }
            }
        }
    }
    
    return result;
}

PointCloudLOD PointCloud::calculateLOD(float distance, float quality) const {
    PointCloudLOD lod;
    
    if (lodConfig.empty()) {
        // Default LOD configuration
        lod.distanceThreshold = distance * 0.1f;
        lod.pointDensity = std::clamp(1.0f - (distance / 1000.0f), 0.1f, 1.0f);
        lod.pointDensity *= quality;
        lod.maxPoints = 1000000;
    } else {
        // Find appropriate LOD level
        for (const auto& config : lodConfig) {
            if (distance <= config.distanceThreshold) {
                lod = config;
                lod.pointDensity *= quality;
                return lod;
            }
        }
        lod = lodConfig.back();
        lod.pointDensity *= quality;
    }
    
    return lod;
}

ValidationResult PointCloud::validate() const {
    if (points.empty()) {
        return ValidationResult::failure("Point cloud has no points");
    }
    
    // Check for NaN/Inf
    for (size_t i = 0; i < std::min(points.size(), size_t(1000)); ++i) {
        const auto& pt = points[i];
        
        if (std::isnan(pt.position.x) || std::isinf(pt.position.x) ||
            std::isnan(pt.position.y) || std::isinf(pt.position.y) ||
            std::isnan(pt.position.z) || std::isinf(pt.position.z)) {
            return ValidationResult::failure("Point " + std::to_string(i) + " contains NaN/Inf");
        }
    }
    
    // Validate octree
    if (!octree.empty()) {
        for (size_t i = 0; i < octree.size(); ++i) {
            const auto& node = octree[i];
            
            if (node.parent != UINT32_MAX && node.parent >= octree.size()) {
                return ValidationResult::failure("Octree node " + std::to_string(i) + 
                                               " has invalid parent");
            }
            
            for (int j = 0; j < 8; ++j) {
                if (node.children[j] != UINT32_MAX && node.children[j] >= octree.size()) {
                    return ValidationResult::failure("Octree node " + std::to_string(i) + 
                                                   " has invalid child " + std::to_string(j));
                }
            }
        }
    }
    
    return ValidationResult::success();
}

void PointCloud::downsample(float voxelSize) {
    if (points.empty() || voxelSize <= 0.0f) return;
    
    // Voxel grid downsampling
    struct VoxelKey {
        int32_t x, y, z;
        
        bool operator==(const VoxelKey& other) const {
            return x == other.x && y == other.y && z == other.z;
        }
    };
    
    struct VoxelKeyHash {
        size_t operator()(const VoxelKey& key) const {
            return ((size_t)key.x << 20) ^ ((size_t)key.y << 10) ^ (size_t)key.z;
        }
    };
    
    std::unordered_map<VoxelKey, std::vector<size_t>, VoxelKeyHash> voxelMap;
    
    // Group points by voxel
    for (size_t i = 0; i < points.size(); ++i) {
        const auto& pt = points[i];
        VoxelKey key;
        key.x = static_cast<int32_t>(std::floor(pt.position.x / voxelSize));
        key.y = static_cast<int32_t>(std::floor(pt.position.y / voxelSize));
        key.z = static_cast<int32_t>(std::floor(pt.position.z / voxelSize));
        
        voxelMap[key].push_back(i);
    }
    
    // Average points in each voxel
    std::vector<PointCloudPoint> downsampled;
    downsampled.reserve(voxelMap.size());
    
    for (const auto& [key, indices] : voxelMap) {
        PointCloudPoint avg;
        
        for (size_t idx : indices) {
            const auto& pt = points[idx];
            avg.position += pt.position;
            avg.color += pt.color;
            avg.normal += pt.normal;
            avg.intensity += pt.intensity;
        }
        
        float invCount = 1.0f / indices.size();
        avg.position *= invCount;
        avg.color *= invCount;
        avg.normal = math::normalize(avg.normal);
        avg.intensity *= invCount;
        
        downsampled.push_back(avg);
    }
    
    points = std::move(downsampled);
    totalPoints = points.size();
}

uint64_t PointCloud::getPointCountAtLOD(uint32_t lodIndex) const {
    if (lodIndex >= lodConfig.size()) {
        return totalPoints;
    }
    
    return static_cast<uint64_t>(totalPoints * lodConfig[lodIndex].pointDensity);
}

} // namespace phoenix::resource
