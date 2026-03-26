#include "phoenix/mobile/memory/MemoryManager.hpp"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>

namespace phoenix::mobile {

// Platform detection helpers
#ifdef __APPLE__
    #include <TargetConditionals.h>
    #define PHOENIX_IOS TARGET_OS_IPHONE
    #define PHOENIX_ANDROID 0
#else
    #define PHOENIX_IOS 0
    #ifdef __ANDROID__
        #define PHOENIX_ANDROID 1
    #else
        #define PHOENIX_ANDROID 0
    #endif
#endif

MemoryManager& MemoryManager::getInstance() {
    static MemoryManager instance;
    return instance;
}

bool MemoryManager::initialize(const MemoryConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (isInitialized_) {
        return true;
    }

    config_ = config;
    
    // Initialize stats
    stats_.totalMemory = config.maxMemoryMB * 1024 * 1024;
    stats_.availableMemory = stats_.totalMemory;
    stats_.usedMemory = 0;
    stats_.textureMemory = 0;
    stats_.meshMemory = 0;
    stats_.memoryPressure = 0.0f;
    
    isInitialized_ = true;
    
    std::cout << "[MemoryManager] Initialized with " << config.maxMemoryMB 
              << "MB budget" << std::endl;
    std::cout << "[MemoryManager] Texture budget: " << config.textureBudgetMB 
              << "MB, Mesh budget: " << config.meshBudgetMB << "MB" << std::endl;
    
    return true;
}

void MemoryManager::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    textures_.clear();
    meshes_.clear();
    streamingQueue_ = std::priority_queue<StreamRequest>();
    activeStreams_.clear();
    warningCallbacks_.clear();
    unloadCallbacks_.clear();
    
    isInitialized_ = false;
    
    std::cout << "[MemoryManager] Shutdown complete" << std::endl;
}

void MemoryManager::update() {
    if (!isInitialized_) return;
    
    frameCount_++;
    
    // Update memory statistics
    updateMemoryStats();
    
    // Check memory pressure
    checkMemoryPressure();
    
    // Process streaming queue
    if (config_.enableStreaming) {
        processStreamingQueue();
    }
    
    // Auto-unload old resources
    if (config_.enableAutoUnload) {
        autoUnloadResources();
    }
}

MemoryStats MemoryManager::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
}

float MemoryManager::getMemoryPressure() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_.memoryPressure;
}

bool MemoryManager::isMemoryCritical() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_.memoryPressure >= config_.memoryCriticalThreshold;
}

void MemoryManager::registerTexture(const std::string& id, size_t size,
                                    TextureCompression compression) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    ResourceInfo info;
    info.id = id;
    info.size = size;
    info.isLoaded = true;
    info.lastAccessTime = frameCount_;
    info.accessCount = 1;
    
    textures_[id] = info;
    stats_.textureMemory += size;
    stats_.usedMemory += size;
    stats_.activeTextures++;
    
    updateMemoryStats();
}

void MemoryManager::unregisterTexture(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = textures_.find(id);
    if (it != textures_.end()) {
        stats_.textureMemory -= it->second.size;
        stats_.usedMemory -= it->second.size;
        stats_.activeTextures--;
        textures_.erase(it);
    }
    
    updateMemoryStats();
}

void MemoryManager::registerMesh(const std::string& id, size_t size,
                                 MeshCompression compression) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    ResourceInfo info;
    info.id = id;
    info.size = size;
    info.isLoaded = true;
    info.lastAccessTime = frameCount_;
    info.accessCount = 1;
    
    meshes_[id] = info;
    stats_.meshMemory += size;
    stats_.usedMemory += size;
    stats_.activeMeshes++;
    
    updateMemoryStats();
}

void MemoryManager::unregisterMesh(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = meshes_.find(id);
    if (it != meshes_.end()) {
        stats_.meshMemory -= it->second.size;
        stats_.usedMemory -= it->second.size;
        stats_.activeMeshes--;
        meshes_.erase(it);
    }
    
    updateMemoryStats();
}

void MemoryManager::markResourceAccessed(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto texIt = textures_.find(id);
    if (texIt != textures_.end()) {
        texIt->second.lastAccessTime = frameCount_;
        texIt->second.accessCount++;
        return;
    }
    
    auto meshIt = meshes_.find(id);
    if (meshIt != meshes_.end()) {
        meshIt->second.lastAccessTime = frameCount_;
        meshIt->second.accessCount++;
    }
}

void MemoryManager::requestStreamLoad(const StreamRequest& request) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!config_.enableStreaming) {
        if (request.onComplete) {
            request.onComplete(false);
        }
        return;
    }
    
    streamingQueue_.push(request);
}

void MemoryManager::cancelStreamLoad(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Remove from queue (note: this is simplified, real implementation
    // would need a different data structure for efficient removal)
    activeStreams_.erase(
        std::remove_if(activeStreams_.begin(), activeStreams_.end(),
            [&path](const StreamRequest& req) { return req.path == path; }),
        activeStreams_.end()
    );
}

TextureCompression MemoryManager::getRecommendedTextureCompression() {
#if PHOENIX_IOS
    return TextureCompression::ASTC_4x4;  // iOS prefers ASTC
#elif PHOENIX_ANDROID
    return TextureCompression::ASTC_4x4;  // Modern Android supports ASTC
#else
    return TextureCompression::ASTC_4x4;  // Default
#endif
}

MeshCompression MemoryManager::getRecommendedMeshCompression() {
    return MeshCompression::MeshOptimizer;  // Good balance of speed/compression
}

size_t MemoryManager::calculateCompressedTextureSize(
    uint32_t width, uint32_t height, uint32_t mipLevels,
    TextureCompression compression) {
    
    size_t totalSize = 0;
    uint32_t w = width, h = height;
    
    for (uint32_t i = 0; i < mipLevels; ++i) {
        size_t mipSize = 0;
        float bitsPerPixel = 32.0f;  // Default uncompressed
        
        switch (compression) {
            case TextureCompression::ASTC_4x4:
                bitsPerPixel = 8.0f;
                break;
            case TextureCompression::ASTC_5x5:
                bitsPerPixel = 5.12f;
                break;
            case TextureCompression::ASTC_6x6:
                bitsPerPixel = 3.56f;
                break;
            case TextureCompression::ASTC_8x8:
                bitsPerPixel = 2.0f;
                break;
            case TextureCompression::ETC2_RGBA8:
                bitsPerPixel = 8.0f;
                break;
            case TextureCompression::ETC2_RGB8:
                bitsPerPixel = 4.0f;
                break;
            case TextureCompression::PVRTC_4bpp:
                bitsPerPixel = 4.0f;
                break;
            case TextureCompression::PVRTC_2bpp:
                bitsPerPixel = 2.0f;
                break;
            case TextureCompression::S3TC_DXT5:
                bitsPerPixel = 8.0f;
                break;
            case TextureCompression::S3TC_DXT1:
                bitsPerPixel = 4.0f;
                break;
            default:
                bitsPerPixel = 32.0f;
        }
        
        // ASTC and block compression work on blocks
        if (compression == TextureCompression::ASTC_4x4 ||
            compression == TextureCompression::ASTC_5x5 ||
            compression == TextureCompression::ASTC_6x6 ||
            compression == TextureCompression::ASTC_8x8) {
            uint32_t blockW = (w + 3) / 4;
            uint32_t blockH = (h + 3) / 4;
            mipSize = blockW * blockH * 16;  // 16 bytes per ASTC block
        } else if (compression == TextureCompression::ETC2_RGBA8 ||
                   compression == TextureCompression::S3TC_DXT5) {
            uint32_t blockW = (w + 3) / 4;
            uint32_t blockH = (h + 3) / 4;
            mipSize = blockW * blockH * 16;  // 16 bytes per block
        } else if (compression == TextureCompression::ETC2_RGB8 ||
                   compression == TextureCompression::S3TC_DXT1) {
            uint32_t blockW = (w + 3) / 4;
            uint32_t blockH = (h + 3) / 4;
            mipSize = blockW * blockH * 8;  // 8 bytes per block
        } else if (compression == TextureCompression::PVRTC_4bpp) {
            mipSize = (std::max(w, 8u) * std::max(h, 8u) * 4) / 8;
        } else if (compression == TextureCompression::PVRTC_2bpp) {
            mipSize = (std::max(w, 16u) * std::max(h, 8u) * 2) / 8;
        } else {
            // Uncompressed
            mipSize = w * h * 4;  // RGBA8888
        }
        
        totalSize += mipSize;
        
        w = std::max(1u, w / 2);
        h = std::max(1u, h / 2);
    }
    
    return totalSize;
}

void MemoryManager::forceGarbageCollection() {
    std::cout << "[MemoryManager] Forcing garbage collection" << std::endl;
    // Platform-specific GC would be called here
}

size_t MemoryManager::unloadLRUResources(size_t targetFreeMB) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t targetBytes = targetFreeMB * 1024 * 1024;
    size_t freedBytes = 0;
    
    // Collect all resources with their access times
    std::vector<std::pair<std::string, ResourceInfo*>> allResources;
    
    for (auto& [id, info] : textures_) {
        allResources.emplace_back(id, &info);
    }
    for (auto& [id, info] : meshes_) {
        allResources.emplace_back(id, &info);
    }
    
    // Sort by last access time (oldest first)
    std::sort(allResources.begin(), allResources.end(),
        [](const auto& a, const auto& b) {
            return a.second->lastAccessTime < b.second->lastAccessTime;
        });
    
    // Unload until we've freed enough
    for (auto& [id, info] : allResources) {
        if (freedBytes >= targetBytes) break;
        if (info->priority == ResourcePriority::Critical) continue;
        
        freedBytes += info->size;
        stats_.unloadedResources++;
        
        notifyResourceUnloaded(id);
        
        // Remove from tracking
        if (textures_.count(id)) {
            stats_.textureMemory -= info->size;
            stats_.activeTextures--;
            textures_.erase(id);
        } else if (meshes_.count(id)) {
            stats_.meshMemory -= info->size;
            stats_.activeMeshes--;
            meshes_.erase(id);
        }
    }
    
    stats_.usedMemory -= freedBytes;
    updateMemoryStats();
    
    std::cout << "[MemoryManager] Unloaded " << stats_.unloadedResources 
              << " resources, freed " << (freedBytes / 1024 / 1024) << "MB" << std::endl;
    
    return freedBytes;
}

void MemoryManager::handleMemoryWarning(int level) {
    std::cout << "[MemoryManager] Memory warning level: " << level << std::endl;
    
    float pressure = 0.0f;
    switch (level) {
        case 1:
            pressure = 0.7f;
            forceGarbageCollection();
            break;
        case 2:
            pressure = 0.85f;
            unloadLRUResources(32);  // Free 32MB
            break;
        case 3:
            pressure = 0.95f;
            unloadLRUResources(64);  // Free 64MB aggressively
            break;
    }
    
    notifyMemoryWarning(pressure);
}

void MemoryManager::onMemoryWarning(MemoryWarningCallback callback) {
    warningCallbacks_.push_back(callback);
}

void MemoryManager::onResourceUnloaded(ResourceUnloadCallback callback) {
    unloadCallbacks_.push_back(callback);
}

void MemoryManager::updateMemoryStats() {
    if (stats_.totalMemory > 0) {
        stats_.memoryPressure = static_cast<float>(stats_.usedMemory) / 
                               static_cast<float>(stats_.totalMemory);
    }
    
    stats_.availableMemory = stats_.totalMemory - stats_.usedMemory;
    stats_.peakMemory = std::max(stats_.peakMemory, stats_.usedMemory);
}

void MemoryManager::processStreamingQueue() {
    // Process up to batchSize requests per frame
    uint32_t processed = 0;
    
    while (!streamingQueue_.empty() && 
           processed < config_.streamingBatchSize &&
           activeStreams_.size() < config_.streamingBatchSize * 2) {
        
        StreamRequest request = const_cast<StreamRequest&>(streamingQueue_.top());
        streamingQueue_.pop();
        
        if (request.frameDelay > 0) {
            request.frameDelay--;
            streamingQueue_.push(request);
        } else {
            activeStreams_.push_back(request);
            processed++;
        }
    }
    
    // Complete active streams (simplified - real implementation would async load)
    for (auto it = activeStreams_.begin(); it != activeStreams_.end();) {
        // Simulate completion
        if (it->onComplete) {
            it->onComplete(true);
        }
        it = activeStreams_.erase(it);
    }
}

void MemoryManager::checkMemoryPressure() {
    if (stats_.memoryPressure >= config_.memoryCriticalThreshold) {
        notifyMemoryWarning(stats_.memoryPressure);
        std::cout << "[MemoryManager] CRITICAL: Memory pressure at " 
                  << (stats_.memoryPressure * 100) << "%" << std::endl;
    } else if (stats_.memoryPressure >= config_.memoryWarningThreshold) {
        notifyMemoryWarning(stats_.memoryPressure);
        std::cout << "[MemoryManager] WARNING: Memory pressure at " 
                  << (stats_.memoryPressure * 100) << "%" << std::endl;
    }
}

void MemoryManager::autoUnloadResources() {
    uint32_t threshold = config_.unloadAfterFrames;
    auto now = frameCount_.load();
    
    std::vector<std::string> toUnload;
    
    for (const auto& [id, info] : textures_) {
        if (info.priority == ResourcePriority::Critical) continue;
        if (now - info.lastAccessTime > threshold) {
            toUnload.push_back(id);
        }
    }
    
    for (const auto& [id, info] : meshes_) {
        if (info.priority == ResourcePriority::Critical) continue;
        if (now - info.lastAccessTime > threshold) {
            toUnload.push_back(id);
        }
    }
    
    for (const auto& id : toUnload) {
        notifyResourceUnloaded(id);
    }
}

void MemoryManager::notifyMemoryWarning(float pressure) {
    for (auto& callback : warningCallbacks_) {
        try {
            callback(pressure);
        } catch (...) {
            std::cerr << "[MemoryManager] Warning callback exception" << std::endl;
        }
    }
}

void MemoryManager::notifyResourceUnloaded(const std::string& id) {
    for (auto& callback : unloadCallbacks_) {
        try {
            callback(id);
        } catch (...) {
            std::cerr << "[MemoryManager] Unload callback exception" << std::endl;
        }
    }
}

} // namespace phoenix::mobile
