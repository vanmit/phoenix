#include "../../include/phoenix/resource/resource_manager.hpp"
#include <chrono>
#include <algorithm>
#include <filesystem>

namespace phoenix::resource {

// ============ ResourceManager ============

ResourceManager::ResourceManager(const Config& config) 
    : m_config(config)
    , m_memoryBudget{config.maxMemoryBudget, 0, 0}
{
    // Initialize default loaders
    m_loaders[AssetType::Model_GLTF] = std::make_unique<GLTFLoader>();
    m_loaders[AssetType::Model_GLTF_BINARY] = std::make_unique<GLTFLoader>();
    m_loaders[AssetType::Model_FBX] = std::make_unique<FBXLoader>();
    m_loaders[AssetType::Model_OBJ] = std::make_unique<OBJLoader>();
    m_loaders[AssetType::Model_STL] = std::make_unique<STLLoader>();
    m_loaders[AssetType::Texture_PNG] = std::make_unique<TextureLoader>();
    m_loaders[AssetType::Texture_JPEG] = std::make_unique<TextureLoader>();
    m_loaders[AssetType::Texture_KTX2] = std::make_unique<TextureLoader>();
    m_loaders[AssetType::Texture_DDS] = std::make_unique<TextureLoader>();
    m_loaders[AssetType::Texture_BASIS] = std::make_unique<TextureLoader>();
    m_loaders[AssetType::PointCloud_LAS] = std::make_unique<PointCloudLoader>();
    m_loaders[AssetType::PointCloud_LAZ] = std::make_unique<PointCloudLoader>();
    m_loaders[AssetType::PointCloud_PCD] = std::make_unique<PointCloudLoader>();
    m_loaders[AssetType::Terrain_GeoTIFF] = std::make_unique<TerrainLoader>();
    m_loaders[AssetType::Terrain_Heightmap] = std::make_unique<TerrainLoader>();
}

ResourceManager::~ResourceManager() {
    shutdown();
}

bool ResourceManager::initialize() {
    m_running = true;
    
    // Start loader threads
    for (uint32_t i = 0; i < m_config.numLoaderThreads; ++i) {
        m_loaderThreads.emplace_back(&ResourceManager::loaderThreadFunc, this);
    }
    
    return true;
}

void ResourceManager::shutdown() {
    m_running = false;
    m_queueCV.notify_all();
    
    for (auto& thread : m_loaderThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    m_loaderThreads.clear();
    clearCache();
}

void ResourceManager::registerLoader(std::unique_ptr<AssetLoader> loader) {
    if (loader) {
        m_loaders[loader->getAssetType()] = std::move(loader);
    }
}

void ResourceManager::loaderThreadFunc() {
    while (m_running) {
        LoadRequest request;
        
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            
            m_queueCV.wait(lock, [this]() {
                return !m_running || !m_loadQueue.empty();
            });
            
            if (!m_running && m_loadQueue.empty()) {
                break;
            }
            
            if (m_loadQueue.empty()) {
                continue;
            }
            
            request = m_loadQueue.top();
            m_loadQueue.pop();
        }
        
        // Process request
        ++m_pendingLoads;
        
        // Determine loader
        AssetLoader* loader = nullptr;
        
        if (request.type != AssetType::Unknown) {
            loader = getLoaderForType(request.type);
        } else {
            loader = getLoaderForPath(request.path);
        }
        
        if (!loader) {
            if (request.callback) {
                request.callback(AssetType::Unknown, request.path, LoadState::Failed);
            }
            --m_pendingLoads;
            continue;
        }
        
        // Validate path
        if (!validatePath(request.path)) {
            if (request.callback) {
                request.callback(loader->getAssetType(), request.path, LoadState::Failed);
            }
            --m_pendingLoads;
            continue;
        }
        
        // Check memory budget
        size_t estimatedSize = loader->estimateMemoryUsage(request.path);
        
        if (!m_memoryBudget.canAllocate(estimatedSize)) {
            trimCache();
            
            if (!m_memoryBudget.canAllocate(estimatedSize)) {
                if (request.callback) {
                    request.callback(loader->getAssetType(), request.path, LoadState::Failed);
                }
                --m_pendingLoads;
                continue;
            }
        }
        
        // Load based on type
        // This is simplified - actual implementation would use std::variant or similar
        
        --m_pendingLoads;
        
        if (request.callback) {
            request.callback(loader->getAssetType(), request.path, LoadState::Loaded);
        }
    }
}

AssetLoader* ResourceManager::getLoaderForPath(const std::string& path) {
    for (auto& [type, loader] : m_loaders) {
        if (loader && loader->canLoad(path)) {
            return loader.get();
        }
    }
    return nullptr;
}

AssetLoader* ResourceManager::getLoaderForType(AssetType type) {
    auto it = m_loaders.find(type);
    if (it != m_loaders.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool ResourceManager::validatePath(const std::string& path) const {
    // Check for path traversal
    if (path.find("..") != std::string::npos) {
        return false;
    }
    
    // Check against whitelist if configured
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_pathMutex));
    
    if (m_allowedPaths.empty()) {
        return true;  // No restrictions
    }
    
    for (const auto& allowed : m_allowedPaths) {
        if (path.find(allowed) == 0) {
            return true;
        }
    }
    
    return false;
}

void ResourceManager::addAllowedPath(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_pathMutex);
    m_allowedPaths.insert(path);
}

bool ResourceManager::isPathAllowed(const std::string& path) const {
    return validatePath(path);
}

// ============ Mesh Loading ============

std::future<MeshHandle> ResourceManager::loadMeshAsync(const std::string& path,
                                                        LoadCallback callback) {
    return std::async(std::launch::async, [this, path, callback]() {
        return loadMesh(path);
    });
}

MeshHandle ResourceManager::loadMesh(const std::string& path) {
    auto loader = getLoaderForPath(path);
    
    if (!loader) {
        return MeshHandle::invalid();
    }
    
    // Check cache
    // Simplified - actual implementation would use path hashing
    
    // Create GLTF loader and load
    GLTFLoader gltfLoader;
    auto mesh = gltfLoader.load(path);
    
    if (!mesh || mesh->loadState != LoadState::Loaded) {
        return MeshHandle::invalid();
    }
    
    // Store in manager
    uint64_t id = m_nextId++;
    
    MeshEntry entry;
    entry.mesh = std::move(mesh);
    entry.lastAccessTime = m_currentTime;
    entry.accessCount = 1;
    
    m_meshes[id] = std::move(entry);
    m_memoryBudget.allocate(entry.mesh->calculateMemoryUsage());
    
    return MeshHandle(id);
}

Mesh* ResourceManager::getMesh(MeshHandle handle) {
    if (!handle.isValid()) return nullptr;
    
    auto it = m_meshes.find(handle.getId());
    if (it == m_meshes.end()) return nullptr;
    
    it->second.lastAccessTime = m_currentTime;
    ++it->second.accessCount;
    
    return it->second.mesh.get();
}

const Mesh* ResourceManager::getMesh(MeshHandle handle) const {
    if (!handle.isValid()) return nullptr;
    
    auto it = m_meshes.find(handle.getId());
    if (it == m_meshes.end()) return nullptr;
    
    return it->second.mesh.get();
}

void ResourceManager::unloadMesh(MeshHandle handle) {
    if (!handle.isValid()) return;
    
    auto it = m_meshes.find(handle.getId());
    if (it == m_meshes.end()) return;
    
    m_memoryBudget.deallocate(it->second.mesh->calculateMemoryUsage());
    m_meshes.erase(it);
}

// ============ Texture Loading ============

std::future<TextureHandle> ResourceManager::loadTextureAsync(const std::string& path,
                                                              LoadCallback callback) {
    return std::async(std::launch::async, [this, path, callback]() {
        return loadTexture(path);
    });
}

TextureHandle ResourceManager::loadTexture(const std::string& path) {
    TextureLoader loader;
    auto texture = loader.load(path);
    
    if (!texture || texture->loadState != LoadState::Loaded) {
        return TextureHandle::invalid();
    }
    
    uint64_t id = m_nextId++;
    
    TextureEntry entry;
    entry.texture = std::move(texture);
    entry.lastAccessTime = m_currentTime;
    entry.accessCount = 1;
    
    m_textures[id] = std::move(entry);
    m_memoryBudget.allocate(entry.texture->calculateMemoryUsage());
    
    return TextureHandle(id);
}

Texture* ResourceManager::getTexture(TextureHandle handle) {
    if (!handle.isValid()) return nullptr;
    
    auto it = m_textures.find(handle.getId());
    if (it == m_textures.end()) return nullptr;
    
    it->second.lastAccessTime = m_currentTime;
    ++it->second.accessCount;
    
    return it->second.texture.get();
}

const Texture* ResourceManager::getTexture(TextureHandle handle) const {
    if (!handle.isValid()) return nullptr;
    
    auto it = m_textures.find(handle.getId());
    if (it == m_textures.end()) return nullptr;
    
    return it->second.texture.get();
}

void ResourceManager::unloadTexture(TextureHandle handle) {
    if (!handle.isValid()) return;
    
    auto it = m_textures.find(handle.getId());
    if (it == m_textures.end()) return;
    
    m_memoryBudget.deallocate(it->second.texture->calculateMemoryUsage());
    m_textures.erase(it);
}

// ============ Point Cloud Loading ============

std::future<PointCloudHandle> ResourceManager::loadPointCloudAsync(const std::string& path,
                                                                    LoadCallback callback) {
    return std::async(std::launch::async, [this, path, callback]() {
        return loadPointCloud(path);
    });
}

PointCloudHandle ResourceManager::loadPointCloud(const std::string& path) {
    PointCloudLoader loader;
    auto cloud = loader.load(path);
    
    if (!cloud || cloud->loadState != LoadState::Loaded) {
        return PointCloudHandle::invalid();
    }
    
    uint64_t id = m_nextId++;
    
    PointCloudEntry entry;
    entry.pointCloud = std::move(cloud);
    entry.lastAccessTime = m_currentTime;
    entry.accessCount = 1;
    
    m_pointClouds[id] = std::move(entry);
    m_memoryBudget.allocate(entry.pointCloud->calculateMemoryUsage());
    
    return PointCloudHandle(id);
}

PointCloud* ResourceManager::getPointCloud(PointCloudHandle handle) {
    if (!handle.isValid()) return nullptr;
    
    auto it = m_pointClouds.find(handle.getId());
    if (it == m_pointClouds.end()) return nullptr;
    
    it->second.lastAccessTime = m_currentTime;
    ++it->second.accessCount;
    
    return it->second.pointCloud.get();
}

const PointCloud* ResourceManager::getPointCloud(PointCloudHandle handle) const {
    if (!handle.isValid()) return nullptr;
    
    auto it = m_pointClouds.find(handle.getId());
    if (it == m_pointClouds.end()) return nullptr;
    
    return it->second.pointCloud.get();
}

void ResourceManager::unloadPointCloud(PointCloudHandle handle) {
    if (!handle.isValid()) return;
    
    auto it = m_pointClouds.find(handle.getId());
    if (it == m_pointClouds.end()) return;
    
    m_memoryBudget.deallocate(it->second.pointCloud->calculateMemoryUsage());
    m_pointClouds.erase(it);
}

// ============ Terrain Loading ============

std::future<TerrainHandle> ResourceManager::loadTerrainAsync(const std::string& path,
                                                              LoadCallback callback) {
    return std::async(std::launch::async, [this, path, callback]() {
        return loadTerrain(path);
    });
}

TerrainHandle ResourceManager::loadTerrain(const std::string& path) {
    TerrainLoader loader;
    auto terrain = loader.load(path);
    
    if (!terrain || terrain->loadState != LoadState::Loaded) {
        return TerrainHandle::invalid();
    }
    
    uint64_t id = m_nextId++;
    
    TerrainEntry entry;
    entry.terrain = std::move(terrain);
    entry.lastAccessTime = m_currentTime;
    entry.accessCount = 1;
    
    m_terrains[id] = std::move(entry);
    m_memoryBudget.allocate(entry.terrain->calculateMemoryUsage());
    
    return TerrainHandle(id);
}

Terrain* ResourceManager::getTerrain(TerrainHandle handle) {
    if (!handle.isValid()) return nullptr;
    
    auto it = m_terrains.find(handle.getId());
    if (it == m_terrains.end()) return nullptr;
    
    it->second.lastAccessTime = m_currentTime;
    ++it->second.accessCount;
    
    return it->second.terrain.get();
}

const Terrain* ResourceManager::getTerrain(TerrainHandle handle) const {
    if (!handle.isValid()) return nullptr;
    
    auto it = m_terrains.find(handle.getId());
    if (it == m_terrains.end()) return nullptr;
    
    return it->second.terrain.get();
}

void ResourceManager::unloadTerrain(TerrainHandle handle) {
    if (!handle.isValid()) return;
    
    auto it = m_terrains.find(handle.getId());
    if (it == m_terrains.end()) return;
    
    m_memoryBudget.deallocate(it->second.terrain->calculateMemoryUsage());
    m_terrains.erase(it);
}

// ============ Memory Management ============

size_t ResourceManager::getMemoryUsage() const {
    return m_memoryBudget.currentUsage;
}

size_t ResourceManager::getMemoryBudget() const {
    return m_memoryBudget.maxMemory;
}

void ResourceManager::setMemoryBudget(size_t bytes) {
    m_memoryBudget.maxMemory = bytes;
    trimCache();
}

void ResourceManager::clearCache() {
    m_meshes.clear();
    m_textures.clear();
    m_pointClouds.clear();
    m_terrains.clear();
    m_memoryBudget.currentUsage = 0;
}

void ResourceManager::trimCache() {
    // Simple LRU-based cache trimming
    // Would sort by lastAccessTime and remove oldest entries
    
    while (m_memoryBudget.currentUsage > m_memoryBudget.maxMemory * 0.9f) {
        // Find least recently used resource
        uint64_t oldestId = UINT64_MAX;
        float oldestTime = FLT_MAX;
        bool isMesh = false;
        
        for (const auto& [id, entry] : m_meshes) {
            if (entry.lastAccessTime < oldestTime) {
                oldestTime = entry.lastAccessTime;
                oldestId = id;
                isMesh = true;
            }
        }
        
        for (const auto& [id, entry] : m_textures) {
            if (entry.lastAccessTime < oldestTime) {
                oldestTime = entry.lastAccessTime;
                oldestId = id;
                isMesh = false;
            }
        }
        
        if (oldestId == UINT64_MAX) break;
        
        if (isMesh) {
            unloadMesh(MeshHandle(oldestId));
        } else {
            unloadTexture(TextureHandle(oldestId));
        }
    }
}

// ============ Status ============

size_t ResourceManager::getPendingLoadCount() const {
    return m_pendingLoads;
}

size_t ResourceManager::getLoadedResourceCount() const {
    return m_meshes.size() + m_textures.size() + 
           m_pointClouds.size() + m_terrains.size();
}

} // namespace phoenix::resource
