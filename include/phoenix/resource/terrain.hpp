#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <array>

#include "../math/vector.hpp"
#include "../math/color.hpp"
#include "types.hpp"
#include "texture.hpp"

namespace phoenix::resource {

/**
 * @brief Terrain tile information for streaming
 */
struct TerrainTile {
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t size = 0;  // Tile size in vertices
    
    std::vector<float> heightData;
    std::vector<math::Color4> splatData;  // RGBA = terrain type weights
    
    BoundingVolume bounds;
    bool isLoaded = false;
    bool isLoading = false;
    
    std::string heightmapPath;
    std::string splatmapPath;
};

/**
 * @brief Terrain layer definition
 */
struct TerrainLayer {
    std::string name;
    int32_t diffuseTexture = -1;
    int32_t normalTexture = -1;
    int32_t roughnessTexture = -1;
    int32_t heightTexture = -1;
    
    float roughness = 0.5f;
    float metallic = 0.0f;
    float normalScale = 1.0f;
    float heightScale = 0.1f;
    
    float minSlope = 0.0f;
    float maxSlope = 90.0f;
    float minHeight = -FLT_MAX;
    float maxHeight = FLT_MAX;
};

/**
 * @brief Vegetation instance
 */
struct VegetationInstance {
    math::Vector3 position;
    math::Quaternion rotation;
    math::Vector3 scale{1.0f, 1.0f, 1.0f};
    uint32_t meshIndex = 0;
    float density = 1.0f;
    uint8_t variation = 0;
};

/**
 * @brief Vegetation distribution map
 */
struct VegetationDistribution {
    std::string name;
    uint32_t meshIndex = 0;
    
    // Density map (grayscale texture)
    int32_t densityTexture = -1;
    float densityScale = 1.0f;
    
    // Distribution parameters
    float minSlope = 0.0f;
    float maxSlope = 45.0f;
    float minHeight = 0.0f;
    float maxHeight = FLT_MAX;
    
    // Randomization
    float scaleVariation = 0.2f;
    float rotationVariation = 360.0f;
    uint32_t numVariations = 1;
    
    std::vector<VegetationInstance> instances;
};

/**
 * @brief GeoTIFF metadata
 */
struct GeoTIFFMetadata {
    // Geographic coordinate system
    double originX = 0.0;
    double originY = 0.0;
    double pixelSizeX = 1.0;
    double pixelSizeY = 1.0;
    
    // Projection info
    int32_t epsgCode = 0;  // 0 = unknown
    std::string projectionWKT;
    
    // Tie points
    std::vector<double> tiePoints;
    
    // Model transformation
    std::array<double, 16> modelTransform;
    
    // No data value
    double noDataValue = -9999.0;
    
    // Min/max elevation
    double minElevation = 0.0;
    double maxElevation = 0.0;
};

/**
 * @brief Terrain chunk for streaming large terrains
 */
struct TerrainChunk {
    uint32_t chunkX = 0;
    uint32_t chunkY = 0;
    uint32_t resolution = 0;
    
    // Height data (CPU side)
    std::vector<float> heights;
    
    // GPU buffer handles
    void* vertexBuffer = nullptr;
    void* indexBuffer = nullptr;
    
    // LOD
    uint32_t currentLOD = 0;
    uint32_t maxLOD = 5;
    
    // State
    bool isLoaded = false;
    bool needsUpdate = false;
    float screenSpaceError = 0.0f;
    
    // Neighbor references for stitching
    TerrainChunk* neighbors[4] = {nullptr, nullptr, nullptr, nullptr};  // N, E, S, W
};

/**
 * @brief Terrain resource for large-scale streaming
 */
class Terrain {
public:
    std::string name;
    std::string sourcePath;
    AssetType assetType = AssetType::Unknown;
    
    // Terrain dimensions
    uint32_t width = 0;
    uint32_t depth = 0;
    float verticalScale = 1.0f;
    float verticalOffset = 0.0f;
    
    // Height data
    std::vector<float> heightData;
    std::vector<math::Vector3> normalData;
    
    // Splatmap for texture blending
    std::vector<math::Color4> splatData;  // Per-vertex weights
    
    // Layers
    std::vector<TerrainLayer> layers;
    
    // Vegetation
    std::vector<VegetationDistribution> vegetation;
    
    // Streaming
    bool isStreamed = false;
    uint32_t chunkSize = 64;  // Vertices per chunk
    uint32_t chunksPerSide = 0;
    std::vector<std::vector<TerrainChunk>> chunks;
    
    // GeoTIFF metadata
    GeoTIFFMetadata geoMetadata;
    
    // Bounding volume
    BoundingVolume bounds;
    
    // State
    LoadState loadState = LoadState::Pending;
    std::string loadError;
    MemoryBudget memoryUsage;
    
    // Native handles
    void* nativeHandle = nullptr;
    
    Terrain() = default;
    ~Terrain() = default;
    
    // Move semantics
    Terrain(Terrain&&) noexcept = default;
    Terrain& operator=(Terrain&&) noexcept = default;
    
    // Copy semantics
    Terrain(const Terrain&) = default;
    Terrain& operator=(const Terrain&) = default;
    
    /**
     * @brief Calculate memory usage
     */
    size_t calculateMemoryUsage() const;
    
    /**
     * @brief Calculate normal at given position
     */
    math::Vector3 calculateNormal(uint32_t x, uint32_t y) const;
    
    /**
     * @brief Get height at given position
     */
    float getHeight(uint32_t x, uint32_t y) const;
    
    /**
     * @brief Get height at world position (with interpolation)
     */
    float getHeightAtWorldPosition(const math::Vector3& worldPos) const;
    
    /**
     * @brief Sample splat weights at position
     */
    math::Color4 sampleSplat(uint32_t x, uint32_t y) const;
    
    /**
     * @brief Build chunks for streaming
     */
    void buildChunks(uint32_t chunkSize = 64);
    
    /**
     * @brief Update LOD based on camera distance
     */
    void updateLOD(const math::Vector3& cameraPos, float maxError = 4.0f);
    
    /**
     * @brief Generate normals from heightmap
     */
    void generateNormals();
    
    /**
     * @brief Validate terrain data
     */
    ValidationResult validate() const;
    
    /**
     * @brief Get terrain bounds in world space
     */
    BoundingVolume getWorldBounds() const;
    
    /**
     * @brief Convert world position to terrain UV
     */
    math::Vector2 worldToUV(const math::Vector3& worldPos) const;
    
    /**
     * @brief Convert terrain UV to world position
     */
    math::Vector3 uvToWorld(const math::Vector2& uv, float height) const;
};

} // namespace phoenix::resource
