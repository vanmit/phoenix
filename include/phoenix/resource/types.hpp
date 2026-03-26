#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <future>
#include <unordered_map>
#include <variant>

#include "../math/vector.hpp"
#include "../math/matrix.hpp"
#include "../math/color.hpp"

namespace phoenix::resource {

/**
 * @brief Asset type enumeration for resource identification
 */
enum class AssetType : uint8_t {
    Unknown = 0,
    Model_GLTF,
    Model_GLTF_BINARY,
    Model_FBX,
    Model_OBJ,
    Model_STL,
    Texture_PNG,
    Texture_JPEG,
    Texture_KTX2,
    Texture_DDS,
    Texture_BASIS,
    PointCloud_LAS,
    PointCloud_LAZ,
    PointCloud_PCD,
    Terrain_GeoTIFF,
    Terrain_Heightmap,
    Shader_GLSL,
    Shader_SPIRV,
    Audio_WAV,
    Audio_OGG
};

/**
 * @brief Resource loading state
 */
enum class LoadState : uint8_t {
    Pending = 0,
    Loading,
    Loaded,
    Failed,
    Unloaded
};

/**
 * @brief Level of Detail structure
 */
struct LODLevel {
    float distance = 0.0f;          // Distance threshold
    float screenSpaceError = 0.0f;  // Screen space error metric
    uint32_t vertexCount = 0;       // Vertex count at this LOD
    uint32_t indexCount = 0;        // Index count at this LOD
    
    LODLevel() = default;
    LODLevel(float dist, float sse, uint32_t verts, uint32_t indices)
        : distance(dist), screenSpaceError(sse), vertexCount(verts), indexCount(indices) {}
};

/**
 * @brief Bounding volume for culling and LOD
 */
struct BoundingVolume {
    math::Vector3 center;
    float radius = 0.0f;
    math::Vector3 min;
    math::Vector3 max;
    
    BoundingVolume() : min(FLT_MAX), max(-FLT_MAX) {}
    
    void expand(const math::Vector3& point) {
        min = math::Vector3::min(min, point);
        max = math::Vector3::max(max, point);
        center = (min + max) * 0.5f;
        radius = math::length(max - center);
    }
    
    void expand(const BoundingVolume& other) {
        expand(other.min);
        expand(other.max);
    }
    
    bool isValid() const { return radius > 0.0f; }
};

/**
 * @brief Memory budget for resource loading
 */
struct MemoryBudget {
    size_t maxMemory = 1024 * 1024 * 1024;  // 1GB default
    size_t currentUsage = 0;
    size_t peakUsage = 0;
    
    bool canAllocate(size_t bytes) const {
        return (currentUsage + bytes) <= maxMemory;
    }
    
    void allocate(size_t bytes) {
        currentUsage += bytes;
        peakUsage = std::max(peakUsage, currentUsage);
    }
    
    void deallocate(size_t bytes) {
        currentUsage = std::max(0LL, static_cast<int64_t>(currentUsage) - static_cast<int64_t>(bytes));
    }
};

/**
 * @brief Async load callback
 */
using LoadCallback = std::function<void(AssetType type, const std::string& path, LoadState state)>;

/**
 * @brief Security validation result
 */
struct ValidationResult {
    bool isValid = false;
    std::string error;
    std::vector<std::string> warnings;
    
    static ValidationResult success() { return ValidationResult{true, "", {}}; }
    static ValidationResult failure(const std::string& msg) { return ValidationResult{false, msg, {}}; }
};

} // namespace phoenix::resource
