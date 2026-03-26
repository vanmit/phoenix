#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <future>

#include "types.hpp"
#include "asset_loader.hpp"
#include "mesh.hpp"
#include "texture.hpp"
#include "point_cloud.hpp"
#include "terrain.hpp"

namespace phoenix::resource {

/**
 * @brief Resource handle for type-safe access
 */
template<typename T>
class ResourceHandle {
public:
    using ResourceType = T;
    
    ResourceHandle() = default;
    
    explicit ResourceHandle(uint64_t id) : m_id(id) {}
    
    bool isValid() const { return m_id != UINT64_MAX; }
    uint64_t getId() const { return m_id; }
    
    bool operator==(const ResourceHandle& other) const { return m_id == other.m_id; }
    bool operator!=(const ResourceHandle& other) const { return m_id != other.m_id; }
    bool operator<(const ResourceHandle& other) const { return m_id < other.m_id; }
    
    static ResourceHandle invalid() { return ResourceHandle(UINT64_MAX); }
    
private:
    uint64_t m_id = UINT64_MAX;
};

// Type-specific handles
using MeshHandle = ResourceHandle<Mesh>;
using TextureHandle = ResourceHandle<Texture>;
using PointCloudHandle = ResourceHandle<PointCloud>;
using TerrainHandle = ResourceHandle<Terrain>;

/**
 * @brief Resource manager for async loading and caching
 */
class ResourceManager {
public:
    struct Config {
        size_t maxMemoryBudget = 1024 * 1024 * 1024;  // 1GB
        uint32_t numLoaderThreads = 2;
        bool enableCaching = true;
        size_t maxCachedResources = 1000;
        float cacheExpirySeconds = 300.0f;  // 5 minutes
        bool enableStreaming = true;
        float streamDistance = 500.0f;
        bool validateOnLoad = true;
        std::set<std::string> allowedPaths;  // Whitelist for security
    };
    
    struct LoadRequest {
        std::string path;
        AssetType type = AssetType::Unknown;
        LoadCallback callback;
        uint64_t priority = 0;
        bool highPriority = false;
        
        bool operator<(const LoadRequest& other) const {
            if (highPriority != other.highPriority) return !highPriority;
            return priority < other.priority;
        }
    };
    
    explicit ResourceManager(const Config& config = Config());
    ~ResourceManager();
    
    // Non-copyable
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    
    /**
     * @brief Initialize resource manager
     */
    bool initialize();
    
    /**
     * @brief Shutdown resource manager
     */
    void shutdown();
    
    /**
     * @brief Register a custom loader
     */
    void registerLoader(std::unique_ptr<AssetLoader> loader);
    
    // ============ Mesh Loading ============
    
    /**
     * @brief Load mesh asynchronously
     */
    std::future<MeshHandle> loadMeshAsync(const std::string& path, 
                                          LoadCallback callback = nullptr);
    
    /**
     * @brief Load mesh synchronously
     */
    MeshHandle loadMesh(const std::string& path);
    
    /**
     * @brief Get loaded mesh
     */
    Mesh* getMesh(MeshHandle handle);
    const Mesh* getMesh(MeshHandle handle) const;
    
    /**
     * @brief Unload mesh
     */
    void unloadMesh(MeshHandle handle);
    
    // ============ Texture Loading ============
    
    /**
     * @brief Load texture asynchronously
     */
    std::future<TextureHandle> loadTextureAsync(const std::string& path,
                                                LoadCallback callback = nullptr);
    
    /**
     * @brief Load texture synchronously
     */
    TextureHandle loadTexture(const std::string& path);
    
    /**
     * @brief Get loaded texture
     */
    Texture* getTexture(TextureHandle handle);
    const Texture* getTexture(TextureHandle handle) const;
    
    /**
     * @brief Unload texture
     */
    void unloadTexture(TextureHandle handle);
    
    // ============ Point Cloud Loading ============
    
    /**
     * @brief Load point cloud asynchronously
     */
    std::future<PointCloudHandle> loadPointCloudAsync(const std::string& path,
                                                      LoadCallback callback = nullptr);
    
    /**
     * @brief Load point cloud synchronously
     */
    PointCloudHandle loadPointCloud(const std::string& path);
    
    /**
     * @brief Get loaded point cloud
     */
    PointCloud* getPointCloud(PointCloudHandle handle);
    const PointCloud* getPointCloud(PointCloudHandle handle) const;
    
    /**
     * @brief Unload point cloud
     */
    void unloadPointCloud(PointCloudHandle handle);
    
    // ============ Terrain Loading ============
    
    /**
     * @brief Load terrain asynchronously
     */
    std::future<TerrainHandle> loadTerrainAsync(const std::string& path,
                                                LoadCallback callback = nullptr);
    
    /**
     * @brief Load terrain synchronously
     */
    TerrainHandle loadTerrain(const std::string& path);
    
    /**
     * @brief Get loaded terrain
     */
    Terrain* getTerrain(TerrainHandle handle);
    const Terrain* getTerrain(TerrainHandle handle) const;
    
    /**
     * @brief Unload terrain
     */
    void unloadTerrain(TerrainHandle handle);
    
    // ============ Memory Management ============
    
    /**
     * @brief Get current memory usage
     */
    size_t getMemoryUsage() const;
    
    /**
     * @brief Get memory budget
     */
    size_t getMemoryBudget() const;
    
    /**
     * @brief Set memory budget
     */
    void setMemoryBudget(size_t bytes);
    
    /**
     * @brief Clear all cached resources
     */
    void clearCache();
    
    /**
     * @brief Trim cache to fit budget
     */
    void trimCache();
    
    // ============ Status ============
    
    /**
     * @brief Get number of pending loads
     */
    size_t getPendingLoadCount() const;
    
    /**
     * @brief Get number of loaded resources
     */
    size_t getLoadedResourceCount() const;
    
    /**
     * @brief Check if path is allowed
     */
    bool isPathAllowed(const std::string& path) const;
    
    /**
     * @brief Add allowed path
     */
    void addAllowedPath(const std::string& path);
    
private:
    Config m_config;
    MemoryBudget m_memoryBudget;
    
    // Loaders
    std::unordered_map<AssetType, std::unique_ptr<AssetLoader>> m_loaders;
    
    // Resource storage
    struct MeshEntry {
        std::unique_ptr<Mesh> mesh;
        float lastAccessTime = 0.0f;
        uint32_t accessCount = 0;
    };
    
    struct TextureEntry {
        std::unique_ptr<Texture> texture;
        float lastAccessTime = 0.0f;
        uint32_t accessCount = 0;
    };
    
    struct PointCloudEntry {
        std::unique_ptr<PointCloud> pointCloud;
        float lastAccessTime = 0.0f;
        uint32_t accessCount = 0;
    };
    
    struct TerrainEntry {
        std::unique_ptr<Terrain> terrain;
        float lastAccessTime = 0.0f;
        uint32_t accessCount = 0;
    };
    
    std::unordered_map<uint64_t, MeshEntry> m_meshes;
    std::unordered_map<uint64_t, TextureEntry> m_textures;
    std::unordered_map<uint64_t, PointCloudEntry> m_pointClouds;
    std::unordered_map<uint64_t, TerrainEntry> m_terrains;
    
    uint64_t m_nextId = 1;
    
    // Load queue
    std::priority_queue<LoadRequest> m_loadQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_queueCV;
    
    // Loader threads
    std::vector<std::thread> m_loaderThreads;
    std::atomic<bool> m_running{false};
    std::atomic<size_t> m_pendingLoads{0};
    
    // Path whitelist
    std::set<std::string> m_allowedPaths;
    std::mutex m_pathMutex;
    
    // Timing
    float m_currentTime = 0.0f;
    
    // Internal methods
    void loaderThreadFunc();
    AssetLoader* getLoaderForPath(const std::string& path);
    AssetLoader* getLoaderForType(AssetType type);
    
    template<typename T, typename MapType>
    ResourceHandle<T> loadResourceAsync(const std::string& path, 
                                        MapType& storage,
                                        std::function<std::unique_ptr<T>(const std::string&)> loaderFunc);
    
    template<typename T, typename MapType>
    ResourceHandle<T> loadResourceSync(const std::string& path,
                                       MapType& storage,
                                       std::function<std::unique_ptr<T>(const std::string&)> loaderFunc);
    
    bool validatePath(const std::string& path) const;
    void updateAccessTime(uint64_t id);
};

} // namespace phoenix::resource
