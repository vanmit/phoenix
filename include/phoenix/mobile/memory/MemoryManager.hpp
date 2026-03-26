#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

namespace phoenix::mobile {

/**
 * @brief Texture compression formats for mobile
 */
enum class TextureCompression {
    None,       // No compression (RGBA8888)
    ASTC_4x4,   // ASTC 4x4 (8 bits/pixel)
    ASTC_5x5,   // ASTC 5x5 (5.12 bits/pixel)
    ASTC_6x6,   // ASTC 6x6 (3.56 bits/pixel)
    ASTC_8x8,   // ASTC 8x8 (2 bits/pixel)
    ETC2_RGBA8, // ETC2 with alpha (8 bits/pixel)
    ETC2_RGB8,  // ETC2 RGB (4 bits/pixel)
    PVRTC_4bpp, // PVRTC 4bpp (iOS)
    PVRTC_2bpp, // PVRTC 2bpp (iOS)
    S3TC_DXT5,  // DXT5 with alpha
    S3TC_DXT1   // DXT1 without alpha
};

/**
 * @brief Mesh compression methods
 */
enum class MeshCompression {
    None,           // No compression
    Draco,          // Google Draco compression
    MeshOptimizer,  // meshoptimizer library
    Quantized16,    // 16-bit quantization
    Quantized8      // 8-bit quantization (aggressive)
};

/**
 * @brief Resource priority levels for streaming
 */
enum class ResourcePriority {
    Critical,   // Must be loaded immediately
    High,       // Load as soon as possible
    Normal,     // Standard loading
    Low,        // Load when resources available
    Background  // Load in background only
};

/**
 * @brief Memory usage statistics
 */
struct MemoryStats {
    size_t totalMemory = 0;           // Total system memory
    size_t usedMemory = 0;            // Currently used
    size_t availableMemory = 0;       // Available for allocation
    size_t textureMemory = 0;         // Memory used by textures
    size_t meshMemory = 0;            // Memory used by meshes
    size_t shaderMemory = 0;          // Memory used by shaders
    size_t audioMemory = 0;           // Memory used by audio
    size_t otherMemory = 0;           // Other memory usage
    size_t peakMemory = 0;            // Peak memory usage
    float memoryPressure = 0.0f;      // 0.0 - 1.0 pressure level
    int unloadedResources = 0;        // Number of resources unloaded
    int activeTextures = 0;           // Number of active textures
    int activeMeshes = 0;             // Number of active meshes
};

/**
 * @brief Streaming load request
 */
struct StreamRequest {
    std::string path;
    ResourcePriority priority = ResourcePriority::Normal;
    bool isUrgent = false;
    uint32_t frameDelay = 0;
    std::function<void(bool)> onComplete;
};

/**
 * @brief Resource tracking information
 */
struct ResourceInfo {
    std::string id;
    std::string path;
    size_t size = 0;
    uint64_t lastAccessTime = 0;
    uint32_t accessCount = 0;
    ResourcePriority priority = ResourcePriority::Normal;
    bool isLoaded = false;
    bool isStreaming = false;
};

/**
 * @brief Memory manager configuration
 */
struct MemoryConfig {
    size_t maxMemoryMB = 256;           // Maximum memory budget
    size_t textureBudgetMB = 128;       // Texture memory budget
    size_t meshBudgetMB = 64;           // Mesh memory budget
    float memoryWarningThreshold = 0.8f; // Warning at 80%
    float memoryCriticalThreshold = 0.9f; // Critical at 90%
    size_t maxActiveTextures = 256;     // Max active textures
    size_t maxActiveMeshes = 512;       // Max active meshes
    bool enableAutoUnload = true;       // Auto-unload unused resources
    uint32_t unloadAfterFrames = 300;   // Unload after 5 seconds at 60fps
    bool enableStreaming = true;        // Enable streaming loads
    uint32_t streamingBatchSize = 4;    // Resources per frame
    MeshCompression defaultMeshCompression = MeshCompression::MeshOptimizer;
    TextureCompression defaultTextureCompression = TextureCompression::ASTC_4x4;
};

/**
 * @brief Main memory management class for mobile devices
 * 
 * Handles:
 * - Texture compression (ASTC/ETC2/PVRTC)
 * - Mesh compression (Draco/meshoptimizer)
 * - Streaming resource loading
 * - Memory warning handling
 * - Automatic resource unloading
 */
class MemoryManager {
public:
    using MemoryWarningCallback = std::function<void(float pressure)>;
    using ResourceUnloadCallback = std::function<void(const std::string&)>;
    using StreamCompleteCallback = std::function<void(const std::string&, bool)>;

    static MemoryManager& getInstance();

    /**
     * @brief Initialize memory manager
     */
    bool initialize(const MemoryConfig& config = MemoryConfig());

    /**
     * @brief Shutdown memory manager
     */
    void shutdown();

    /**
     * @brief Update memory manager (call once per frame)
     */
    void update();

    /**
     * @brief Get current memory statistics
     */
    MemoryStats getStats() const;

    /**
     * @brief Get memory pressure (0.0 - 1.0)
     */
    float getMemoryPressure() const;

    /**
     * @brief Check if memory is critical
     */
    bool isMemoryCritical() const;

    /**
     * @brief Register texture for tracking
     */
    void registerTexture(const std::string& id, size_t size, 
                        TextureCompression compression = TextureCompression::None);

    /**
     * @brief Unregister texture
     */
    void unregisterTexture(const std::string& id);

    /**
     * @brief Register mesh for tracking
     */
    void registerMesh(const std::string& id, size_t size,
                     MeshCompression compression = MeshCompression::None);

    /**
     * @brief Unregister mesh
     */
    void unregisterMesh(const std::string& id);

    /**
     * @brief Mark resource as accessed
     */
    void markResourceAccessed(const std::string& id);

    /**
     * @brief Request streaming load
     */
    void requestStreamLoad(const StreamRequest& request);

    /**
     * @brief Cancel streaming load
     */
    void cancelStreamLoad(const std::string& path);

    /**
     * @brief Get recommended texture compression for platform
     */
    static TextureCompression getRecommendedTextureCompression();

    /**
     * @brief Get recommended mesh compression
     */
    static MeshCompression getRecommendedMeshCompression();

    /**
     * @brief Calculate compressed texture size
     */
    static size_t calculateCompressedTextureSize(
        uint32_t width, uint32_t height, uint32_t mipLevels,
        TextureCompression compression);

    /**
     * @brief Force garbage collection
     */
    void forceGarbageCollection();

    /**
     * @brief Unload least recently used resources
     * @param targetFreeMB Target memory to free in MB
     * @return Actual memory freed in bytes
     */
    size_t unloadLRUResources(size_t targetFreeMB);

    /**
     * @brief Handle platform memory warning
     * @param level Warning level (0-3, 3 is critical)
     */
    void handleMemoryWarning(int level);

    /**
     * @brief Register memory warning callback
     */
    void onMemoryWarning(MemoryWarningCallback callback);

    /**
     * @brief Register resource unload callback
     */
    void onResourceUnloaded(ResourceUnloadCallback callback);

private:
    MemoryManager() = default;
    ~MemoryManager() = default;
    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;

    void updateMemoryStats();
    void processStreamingQueue();
    void checkMemoryPressure();
    void autoUnloadResources();
    void notifyMemoryWarning(float pressure);
    void notifyResourceUnloaded(const std::string& id);

    MemoryConfig config_;
    MemoryStats stats_;
    
    std::unordered_map<std::string, ResourceInfo> textures_;
    std::unordered_map<std::string, ResourceInfo> meshes_;
    
    std::priority_queue<StreamRequest> streamingQueue_;
    std::vector<StreamRequest> activeStreams_;
    
    std::vector<MemoryWarningCallback> warningCallbacks_;
    std::vector<ResourceUnloadCallback> unloadCallbacks_;
    
    mutable std::mutex mutex_;
    std::atomic<uint64_t> frameCount_{0};
    std::atomic<bool> isInitialized_{false};
};

} // namespace phoenix::mobile
