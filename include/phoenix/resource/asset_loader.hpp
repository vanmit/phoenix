#pragma once

#include <string>
#include <memory>
#include <functional>
#include <future>
#include <unordered_map>
#include <set>

#include "types.hpp"
#include "mesh.hpp"
#include "texture.hpp"
#include "point_cloud.hpp"
#include "terrain.hpp"

namespace phoenix::resource {

/**
 * @brief Base class for all asset loaders
 */
class AssetLoader {
public:
    virtual ~AssetLoader() = default;
    
    /**
     * @brief Get supported file extensions
     */
    virtual std::set<std::string> getSupportedExtensions() const = 0;
    
    /**
     * @brief Get asset type for this loader
     */
    virtual AssetType getAssetType() const = 0;
    
    /**
     * @brief Check if loader can handle given file
     */
    virtual bool canLoad(const std::string& path) const;
    
    /**
     * @brief Validate file format and security
     */
    virtual ValidationResult validate(const std::string& path) const = 0;
    
    /**
     * @brief Get estimated memory usage before loading
     */
    virtual size_t estimateMemoryUsage(const std::string& path) const = 0;
};

/**
 * @brief glTF 2.0 loader using tinygltf
 */
class GLTFLoader : public AssetLoader {
public:
    struct Config {
        bool loadAnimations = true;
        bool loadSkins = true;
        bool loadMorphTargets = true;
        bool generateMissingNormals = true;
        bool generateMissingTangents = true;
        bool limitExternalURIs = true;  // Security: prevent loading from arbitrary URLs
        std::set<std::string> allowedExternalDomains;  // Whitelist for external URIs
        bool supportDraco = false;  // Draco compression (requires draco library)
        size_t maxBufferSize = 512 * 1024 * 1024;  // 512MB max buffer
    };
    
    GLTFLoader(const Config& config = Config());
    
    std::set<std::string> getSupportedExtensions() const override;
    AssetType getAssetType() const override;
    ValidationResult validate(const std::string& path) const override;
    size_t estimateMemoryUsage(const std::string& path) const override;
    
    /**
     * @brief Load glTF file asynchronously
     */
    std::future<std::unique_ptr<Mesh>> loadAsync(const std::string& path);
    
    /**
     * @brief Load glTF file synchronously
     */
    std::unique_ptr<Mesh> load(const std::string& path);
    
    /**
     * @brief Load glTF from memory buffer
     */
    std::unique_ptr<Mesh> loadFromMemory(const uint8_t* data, size_t size, 
                                         const std::string& basePath = "");
    
private:
    Config m_config;
    
    bool validateURI(const std::string& uri) const;
    void processNode(const void* node, Mesh& mesh, int32_t parentJoint);
    void processMesh(const void* mesh, Mesh& outMesh, const std::string& basePath);
    void processMaterial(const void* material, Material& outMaterial);
    void processAnimation(const void* animation, Mesh& outMesh);
    void processSkin(const void* skin, Mesh& outMesh);
};

/**
 * @brief FBX loader using Assimp
 */
class FBXLoader : public AssetLoader {
public:
    struct Config {
        bool triangulate = true;
        bool generateNormals = true;
        bool generateTangents = true;
        bool generateUVs = true;
        bool optimizeMesh = true;
        bool loadAnimations = true;
        bool loadMaterials = true;
        float scale = 1.0f;
    };
    
    FBXLoader(const Config& config = Config());
    
    std::set<std::string> getSupportedExtensions() const override;
    AssetType getAssetType() const override;
    ValidationResult validate(const std::string& path) const override;
    size_t estimateMemoryUsage(const std::string& path) const override;
    
    /**
     * @brief Load FBX file asynchronously
     */
    std::future<std::unique_ptr<Mesh>> loadAsync(const std::string& path);
    
    /**
     * @brief Load FBX file synchronously
     */
    std::unique_ptr<Mesh> load(const std::string& path);
    
private:
    Config m_config;
};

/**
 * @brief OBJ loader with custom parser
 */
class OBJLoader : public AssetLoader {
public:
    struct Config {
        bool generateNormals = true;
        bool generateTangents = true;
        bool triangulate = true;
        bool loadMaterials = true;  // Load accompanying .mtl file
    };
    
    OBJLoader(const Config& config = Config());
    
    std::set<std::string> getSupportedExtensions() const override;
    AssetType getAssetType() const override;
    ValidationResult validate(const std::string& path) const override;
    size_t estimateMemoryUsage(const std::string& path) const override;
    
    /**
     * @brief Load OBJ file asynchronously
     */
    std::future<std::unique_ptr<Mesh>> loadAsync(const std::string& path);
    
    /**
     * @brief Load OBJ file synchronously
     */
    std::unique_ptr<Mesh> load(const std::string& path);
    
private:
    Config m_config;
    
    struct VertexKey {
        int32_t position;
        int32_t normal;
        int32_t texcoord;
        
        bool operator==(const VertexKey& other) const {
            return position == other.position && 
                   normal == other.normal && 
                   texcoord == other.texcoord;
        }
    };
    
    struct VertexKeyHash {
        size_t operator()(const VertexKey& key) const {
            return ((size_t)key.position << 20) ^ 
                   ((size_t)key.normal << 10) ^ 
                   (size_t)key.texcoord;
        }
    };
};

/**
 * @brief STL loader for 3D printing models
 */
class STLLoader : public AssetLoader {
public:
    struct Config {
        bool binary = true;  // Prefer binary format
        bool generateNormals = true;
        bool centerMesh = true;
    };
    
    STLLoader(const Config& config = Config());
    
    std::set<std::string> getSupportedExtensions() const override;
    AssetType getAssetType() const override;
    ValidationResult validate(const std::string& path) const override;
    size_t estimateMemoryUsage(const std::string& path) const override;
    
    /**
     * @brief Load STL file asynchronously
     */
    std::future<std::unique_ptr<Mesh>> loadAsync(const std::string& path);
    
    /**
     * @brief Load STL file synchronously
     */
    std::unique_ptr<Mesh> load(const std::string& path);
    
private:
    Config m_config;
    
    std::unique_ptr<Mesh> loadBinary(const std::string& path);
    std::unique_ptr<Mesh> loadAscii(const std::string& path);
};

/**
 * @brief Texture loader supporting multiple formats
 */
class TextureLoader : public AssetLoader {
public:
    struct Config {
        bool generateMipMaps = true;
        bool sRGB = true;
        TextureFormat targetFormat = TextureFormat::RGBA8;
        bool decompressToRGBA = false;  // Decompress BC/ASTC to RGBA
        uint32_t maxDimension = 0;  // 0 = no limit
    };
    
    TextureLoader(const Config& config = Config());
    
    std::set<std::string> getSupportedExtensions() const override;
    AssetType getAssetType() const override;
    ValidationResult validate(const std::string& path) const override;
    size_t estimateMemoryUsage(const std::string& path) const override;
    
    /**
     * @brief Load texture asynchronously
     */
    std::future<std::unique_ptr<Texture>> loadAsync(const std::string& path);
    
    /**
     * @brief Load texture synchronously
     */
    std::unique_ptr<Texture> load(const std::string& path);
    
    /**
     * @brief Load texture from memory
     */
    std::unique_ptr<Texture> loadFromMemory(const uint8_t* data, size_t size, 
                                            AssetType type);
    
private:
    Config m_config;
    
    std::unique_ptr<Texture> loadPNG(const std::string& path);
    std::unique_ptr<Texture> loadJPEG(const std::string& path);
    std::unique_ptr<Texture> loadKTX2(const std::string& path);
    std::unique_ptr<Texture> loadDDS(const std::string& path);
    std::unique_ptr<Texture> loadBasis(const std::string& path);
};

/**
 * @brief Point cloud loader
 */
class PointCloudLoader : public AssetLoader {
public:
    struct Config {
        bool buildOctree = true;
        uint8_t octreeMaxDepth = 10;
        uint32_t maxPointsPerNode = 100;
        bool enableStreaming = true;
        float streamDistance = 100.0f;
        bool decompressLAZ = true;
    };
    
    PointCloudLoader(const Config& config = Config());
    
    std::set<std::string> getSupportedExtensions() const override;
    AssetType getAssetType() const override;
    ValidationResult validate(const std::string& path) const override;
    size_t estimateMemoryUsage(const std::string& path) const override;
    
    /**
     * @brief Load point cloud asynchronously
     */
    std::future<std::unique_ptr<PointCloud>> loadAsync(const std::string& path);
    
    /**
     * @brief Load point cloud synchronously
     */
    std::unique_ptr<PointCloud> load(const std::string& path);
    
private:
    Config m_config;
    
    std::unique_ptr<PointCloud> loadLAS(const std::string& path);
    std::unique_ptr<PointCloud> loadLAZ(const std::string& path);
    std::unique_ptr<PointCloud> loadPCD(const std::string& path);
};

/**
 * @brief Terrain loader
 */
class TerrainLoader : public AssetLoader {
public:
    struct Config {
        bool useStreaming = true;
        uint32_t chunkSize = 64;
        float verticalScale = 1.0f;
        float verticalOffset = 0.0f;
        bool generateNormals = true;
        bool loadSplatmap = true;
        bool loadVegetation = true;
    };
    
    TerrainLoader(const Config& config = Config());
    
    std::set<std::string> getSupportedExtensions() const override;
    AssetType getAssetType() const override;
    ValidationResult validate(const std::string& path) const override;
    size_t estimateMemoryUsage(const std::string& path) const override;
    
    /**
     * @brief Load terrain asynchronously
     */
    std::future<std::unique_ptr<Terrain>> loadAsync(const std::string& path);
    
    /**
     * @brief Load terrain synchronously
     */
    std::unique_ptr<Terrain> load(const std::string& path);
    
private:
    Config m_config;
    
    std::unique_ptr<Terrain> loadGeoTIFF(const std::string& path);
    std::unique_ptr<Terrain> loadHeightmapRAW(const std::string& path);
};

} // namespace phoenix::resource
