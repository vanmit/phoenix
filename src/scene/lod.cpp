#include "../../include/phoenix/scene/lod.hpp"
#include <algorithm>
#include <cmath>

namespace phoenix {
namespace scene {

// ============================================================================
// LODComponent Implementation
// ============================================================================

LODComponent::LODComponent()
    : levels_()
    , currentLevel_(LODQuality::High)
    , currentLevelIndex_(2)
    , transition_()
    , morphEnabled_(true)
    , isHLOD_(false)
    , hlodChildren_() {
}

LODComponent::LODComponent(const LevelList& levels)
    : levels_(levels)
    , currentLevel_(LODQuality::High)
    , currentLevelIndex_(2)
    , transition_()
    , morphEnabled_(true)
    , isHLOD_(false)
    , hlodChildren_() {
    sortLevels();
}

void LODComponent::addLevel(const LODLevel& level) {
    levels_.push_back(level);
    sortLevels();
}

void LODComponent::sortLevels() {
    std::sort(levels_.begin(), levels_.end(),
        [](const LODLevel& a, const LODLevel& b) {
            return a.distance < b.distance;
        });
    
    // Update current index if needed
    if (currentLevelIndex_ >= levels_.size()) {
        currentLevelIndex_ = levels_.empty() ? 0 : levels_.size() - 1;
    }
}

size_t LODComponent::selectLOD(const math::Vector3& cameraPos,
                                const math::BoundingSphere& bounds) const {
    if (levels_.empty()) return 0;
    
    const float distance = (bounds.center - cameraPos).length() - bounds.radius;
    
    // Find appropriate level based on distance
    for (size_t i = 0; i < levels_.size(); ++i) {
        if (distance <= levels_[i].distance) {
            return i;
        }
    }
    
    // Use lowest quality (furthest distance)
    return levels_.size() - 1;
}

size_t LODComponent::selectLODByScreenSpace(const math::Vector3& cameraPos,
                                             const math::BoundingSphere& bounds,
                                             float fovY, float viewportHeight) const {
    if (levels_.empty()) return 0;
    
    const float distance = (bounds.center - cameraPos).length() - bounds.radius;
    
    // Find level with acceptable screen-space error
    for (size_t i = 0; i < levels_.size(); ++i) {
        const float error = calculateScreenSpaceError(levels_[i], distance, fovY, viewportHeight);
        
        if (error <= levels_[i].screenSpaceError) {
            return i;
        }
    }
    
    // Use lowest quality
    return levels_.size() - 1;
}

void LODComponent::setLevel(LODQuality level) {
    for (size_t i = 0; i < levels_.size(); ++i) {
        if (levels_[i].quality == level) {
            setLevelByIndex(i);
            return;
        }
    }
}

void LODComponent::setLevelByIndex(size_t index) {
    if (index >= levels_.size()) return;
    
    if (index == currentLevelIndex_) return;
    
    const LODQuality newLevel = levels_[index].quality;
    
    if (morphEnabled_ && currentLevelIndex_ < levels_.size()) {
        // Start transition
        transition_.fromLevel = currentLevel_;
        transition_.toLevel = newLevel;
        transition_.progress = 0.0f;
        transition_.duration = levels_[index].morphDuration;
        transition_.active = true;
    }
    
    currentLevel_ = newLevel;
    currentLevelIndex_ = index;
}

bool LODComponent::updateTransition(float deltaTime) {
    if (!transition_.active) return false;
    
    transition_.progress += deltaTime / transition_.duration;
    
    if (transition_.progress >= 1.0f) {
        transition_.progress = 1.0f;
        transition_.active = false;
        return false;
    }
    
    return true;
}

float LODComponent::getMorphAlpha() const noexcept {
    if (!transition_.active) return 1.0f;
    return transition_.progress;
}

float LODComponent::calculateScreenSpaceError(const LODLevel& level, float distance,
                                               float fovY, float viewportHeight) {
    if (distance <= 0.0f) return 0.0f;
    if (level.vertexCount == 0) return 0.0f;
    
    // Estimate screen-space size of object
    const float objectSize = 1.0f; // Normalized
    const float screenSize = calculateScreenSpaceSize(objectSize, distance, fovY, viewportHeight);
    
    // Estimate error based on vertex density
    // More vertices = lower error
    const float vertexDensity = std::sqrt(static_cast<float>(level.vertexCount));
    const float error = screenSize / vertexDensity;
    
    return error;
}

float LODComponent::calculateScreenSpaceSize(float objectRadius, float distance,
                                              float fovY, float viewportHeight) {
    if (distance <= 0.0f) return viewportHeight;
    
    // Project object size to screen space
    const float fovScale = viewportHeight / (2.0f * std::tan(fovY * 0.5f));
    const float screenSize = (objectRadius / distance) * fovScale;
    
    return screenSize;
}

LODComponent::Stats LODComponent::getStats() const {
    Stats stats;
    stats.currentLevel = currentLevel_;
    stats.isTransitioning = transition_.active;
    stats.transitionProgress = transition_.progress;
    
    if (currentLevelIndex_ < levels_.size()) {
        const auto& level = levels_[currentLevelIndex_];
        stats.vertexCount = level.vertexCount;
        stats.triangleCount = level.triangleCount;
        stats.memoryUsage = level.memoryUsage;
    } else {
        stats.vertexCount = 0;
        stats.triangleCount = 0;
        stats.memoryUsage = 0;
    }
    
    stats.distanceToCamera = 0.0f;
    stats.screenSpaceError = 0.0f;
    
    return stats;
}

// ============================================================================
// LODSystem Implementation
// ============================================================================

LODSystem::LODSystem(const Config& config)
    : config_(config)
    , lods_()
    , timeSinceUpdate_(0.0f)
    , nextLODIndex_(0) {
}

void LODSystem::registerLOD(LODComponent::Ptr lod) {
    lods_.push_back(std::move(lod));
}

void LODSystem::unregisterLOD(LODComponent* lod) {
    lods_.erase(std::remove_if(lods_.begin(), lods_.end(),
        [lod](const LODComponent::Ptr& ptr) { return ptr.get() == lod; }),
        lods_.end());
}

void LODSystem::update(const math::Vector3& cameraPos, float deltaTime) {
    timeSinceUpdate_ += deltaTime;
    
    if (timeSinceUpdate_ < config_.updateInterval) return;
    
    timeSinceUpdate_ = 0.0f;
    
    // Update LODs with limit on updates per frame
    size_t updatesThisFrame = 0;
    size_t startIdx = nextLODIndex_;
    
    while (updatesThisFrame < config_.maxUpdatesPerFrame && 
           updatesThisFrame < lods_.size()) {
        
        const size_t idx = nextLODIndex_ % lods_.size();
        updateLOD(*lods_[idx], cameraPos);
        
        ++nextLODIndex_;
        ++updatesThisFrame;
        
        // Wrap around check
        if (nextLODIndex_ % lods_.size() == startIdx) break;
    }
}

void LODSystem::updateLOD(LODComponent& lod, const math::Vector3& cameraPos) {
    // Get bounds (would need to get from associated node)
    math::BoundingSphere bounds(math::Vector3::zero(), 1.0f);
    
    size_t targetIndex;
    
    if (config_.useScreenSpace) {
        targetIndex = lod.selectLODByScreenSpace(cameraPos, bounds,
                                                  config_.fovY, config_.viewportHeight);
    } else {
        targetIndex = lod.selectLOD(cameraPos, bounds);
    }
    
    // Apply budget constraints
    if (targetIndex < lod.getLevelCount()) {
        const auto& level = lod.getLevels()[targetIndex];
        
        // Check memory budget
        if (level.memoryUsage > config_.memoryBudget) {
            // Find lower quality level within budget
            for (size_t i = targetIndex + 1; i < lod.getLevelCount(); ++i) {
                if (lod.getLevels()[i].memoryUsage <= config_.memoryBudget) {
                    targetIndex = i;
                    break;
                }
            }
        }
    }
    
    lod.setLevelByIndex(targetIndex);
    
    // Update transition
    lod.updateTransition(config_.updateInterval);
}

float LODSystem::getCurrentMemoryUsage() const {
    float total = 0.0f;
    
    for (const auto& lod : lods_) {
        const auto stats = lod->getStats();
        total += stats.memoryUsage;
    }
    
    return total;
}

uint32_t LODSystem::getCurrentTriangleCount() const {
    uint32_t total = 0;
    
    for (const auto& lod : lods_) {
        const auto stats = lod->getStats();
        total += stats.triangleCount;
    }
    
    return total;
}

LODSystem::Stats LODSystem::getStats() const {
    Stats stats;
    stats.totalLODs = lods_.size();
    stats.updatedLODs = 0;
    stats.transitioningLODs = 0;
    stats.memoryUsage = getCurrentMemoryUsage();
    stats.triangleCount = getCurrentTriangleCount();
    
    for (const auto& dist : stats.distribution) {
        const_cast<LODQuality&>(dist) = LODQuality::Count;
    }
    
    for (const auto& lod : lods_) {
        const auto lodStats = lod->getStats();
        
        if (lodStats.isTransitioning) {
            ++stats.transitioningLODs;
        }
        
        const size_t levelIdx = static_cast<size_t>(lodStats.currentLevel);
        if (levelIdx < static_cast<size_t>(LODQuality::Count)) {
            ++stats.distribution[levelIdx];
        }
    }
    
    stats.updatedLODs = stats.totalLODs; // All updated in last batch
    
    return stats;
}

} // namespace scene
} // namespace phoenix
