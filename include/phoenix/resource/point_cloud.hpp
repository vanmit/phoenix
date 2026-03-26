#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <memory>

#include "../math/vector.hpp"
#include "../math/color.hpp"
#include "types.hpp"

namespace phoenix::resource {

/**
 * @brief Point cloud attribute types
 */
enum class PointAttribute : uint8_t {
    Position = 0,
    Color = 1,
    Normal = 2,
    Intensity = 3,
    ReturnNumber = 4,
    Classification = 5,
    GPS_Time = 6,
    RGB = 7,
    NIR = 8,
    Waveform = 9,
    UserDefined = 10
};

/**
 * @brief Point cloud point structure
 */
struct PointCloudPoint {
    math::Vector3 position;
    math::Color3 color{1.0f, 1.0f, 1.0f};
    math::Vector3 normal{0.0f, 0.0f, 0.0f};
    float intensity = 0.0f;
    uint8_t returnNumber = 0;
    uint8_t classification = 0;
    double gpsTime = 0.0;
    float nir = 0.0f;
    
    // For LAZ compression
    uint8_t scanAngleRank = 0;
    uint16_t pointSourceId = 0;
};

/**
 * @brief LAS/LAZ header information
 */
struct LASHeader {
    char fileSignature[4] = {'L', 'A', 'S', '\0'};
    uint16_t fileSourceId = 0;
    uint16_t globalEncoding = 0;
    
    // Project ID
    uint32_t projectId1 = 0;
    uint16_t projectId2 = 0;
    uint16_t projectId3 = 0;
    uint8_t projectId4[8] = {0};
    
    uint8_t versionMajor = 1;
    uint8_t versionMinor = 2;
    char systemIdentifier[32] = {0};
    char generatingSoftware[32] = {0};
    
    uint16_t fileCreationDay = 0;
    uint16_t fileCreationYear = 0;
    
    uint16_t headerSize = 0;
    uint32_t pointDataOffset = 0;
    uint32_t pointDataRecordLength = 0;
    
    uint32_t numberOfPoints = 0;
    uint32_t numberOfPointsByReturn[5] = {0};
    
    double xScale = 0.0;
    double yScale = 0.0;
    double zScale = 0.0;
    double xOffset = 0.0;
    double yOffset = 0.0;
    double zOffset = 0.0;
    
    double xMin = 0.0, xMax = 0.0;
    double yMin = 0.0, yMax = 0.0;
    double zMin = 0.0, zMax = 0.0;
    
    // Variable length records
    std::vector<std::vector<uint8_t>> vlrData;
    
    bool isCompressed = false;  // LAZ vs LAS
};

/**
 * @brief PCD header information
 */
struct PCDHeader {
    std::string version = "0.7";
    std::vector<std::string> fields;
    std::vector<int32_t> size;
    std::vector<std::string> type;  // F=float, I=int, U=uint
    uint32_t points = 0;
    uint32_t width = 0;
    uint32_t height = 1;  // 1 = unordered, >1 = organized
    std::string viewpoint;
    std::string dataMode;  // ascii or binary
};

/**
 * @brief LOD configuration for point clouds
 */
struct PointCloudLOD {
    float distanceThreshold = 0.0f;
    float pointDensity = 1.0f;  // 0.0-1.0
    uint32_t maxPoints = 0;
    bool useSpatialFiltering = true;
};

/**
 * @brief Point cloud octree node for spatial indexing
 */
struct OctreeNode {
    math::Vector3 center;
    float size = 0.0f;
    uint32_t pointStart = 0;
    uint32_t pointCount = 0;
    uint32_t children[8] = {UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX,
                            UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX};
    uint32_t parent = UINT32_MAX;
    uint8_t depth = 0;
    
    bool isLeaf() const { return children[0] == UINT32_MAX; }
};

/**
 * @brief Point cloud resource for billion-point rendering
 */
class PointCloud {
public:
    std::string name;
    std::string sourcePath;
    AssetType assetType = AssetType::Unknown;
    
    // Point data
    std::vector<PointCloudPoint> points;
    uint64_t totalPoints = 0;
    
    // Spatial indexing
    std::vector<OctreeNode> octree;
    uint32_t rootIndex = 0;
    uint8_t maxDepth = 10;
    
    // LOD configuration
    std::vector<PointCloudLOD> lodConfig;
    
    // Bounding volume
    BoundingVolume bounds;
    
    // State
    LoadState loadState = LoadState::Pending;
    std::string loadError;
    MemoryBudget memoryUsage;
    
    // Streaming state
    bool isStreamed = false;
    std::string streamPath;
    uint64_t loadedPoints = 0;
    
    // Native handle
    void* nativeHandle = nullptr;
    
    PointCloud() = default;
    ~PointCloud() = default;
    
    // Move semantics
    PointCloud(PointCloud&&) noexcept = default;
    PointCloud& operator=(PointCloud&&) noexcept = default;
    
    // Copy semantics (expensive for large clouds)
    PointCloud(const PointCloud&) = default;
    PointCloud& operator=(const PointCloud&) = default;
    
    /**
     * @brief Calculate memory usage
     */
    size_t calculateMemoryUsage() const;
    
    /**
     * @brief Build octree for spatial indexing
     */
    void buildOctree(uint8_t maxDepth = 10, uint32_t maxPointsPerNode = 100);
    
    /**
     * @brief Get points visible from camera position at given quality
     */
    std::vector<const PointCloudPoint*> getVisiblePoints(
        const math::Vector3& cameraPos,
        const math::Vector3& cameraDir,
        float fov,
        float quality = 1.0f,
        uint32_t maxPoints = 1000000
    ) const;
    
    /**
     * @brief Calculate LOD based on distance
     */
    PointCloudLOD calculateLOD(float distance, float quality = 1.0f) const;
    
    /**
     * @brief Validate point cloud data
     */
    ValidationResult validate() const;
    
    /**
     * @brief Downsample point cloud
     */
    void downsample(float voxelSize);
    
    /**
     * @brief Get point count at specific LOD
     */
    uint64_t getPointCountAtLOD(uint32_t lodIndex) const;
};

} // namespace phoenix::resource
