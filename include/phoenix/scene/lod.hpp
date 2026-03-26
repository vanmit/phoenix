#pragma once

#include "scene_node.hpp"
#include "../math/vector3.hpp"
#include "../math/bounding.hpp"
#include <vector>
#include <memory>
#include <string>

namespace phoenix {
namespace scene {

/**
 * @brief LOD quality levels
 */
enum class LODQuality : uint8_t {
    VeryLow = 0,
    Low = 1,
    Medium = 2,
    High = 3,
    VeryHigh = 4,
    Ultra = 5,
    Count
};

/**
 * @brief LOD level configuration
 */
struct alignas(16) LODLevel {
    LODQuality quality;
    float distance;           // Switch distance (world units)
    float screenSpaceError;   // Max screen-space error (pixels)
    float morphDuration;      // Transition duration (seconds)
    
    // LOD data references
    std::string meshId;       // Mesh asset ID for this level
    uint32_t vertexCount;     // Approximate vertex count
    uint32_t triangleCount;   // Approximate triangle count
    float memoryUsage;        // Memory in bytes
    
    LODLevel() noexcept 
        : quality(LODQuality::Medium)
        , distance(100.0f)
        , screenSpaceError(4.0f)
        , morphDuration(0.5f)
        , meshId()
        , vertexCount(0)
        , triangleCount(0)
        , memoryUsage(0) {}
    
    LODLevel(LODQuality q, float dist, const std::string& mesh = "") noexcept
        : quality(q)
        , distance(dist)
        , screenSpaceError(4.0f)
        , morphDuration(0.5f)
        , meshId(mesh)
        , vertexCount(0)
        , triangleCount(0)
        , memoryUsage(0) {}
};

/**
 * @brief LOD transition state
 */
struct LODTransition {
    LODQuality fromLevel;
    LODQuality toLevel;
    float progress;        // 0.0 to 1.0
    float duration;
    bool active;
    
    LODTransition() noexcept 
        : fromLevel(LODQuality::Medium)
        , toLevel(LODQuality::Medium)
        , progress(0.0f)
        , duration(0.5f)
        , active(false) {}
};

/**
 * @brief LOD Component for scene nodes
 * 
 * Manages multiple LOD levels and automatic switching based on:
 * - Distance from camera
 * - Screen-space error
 * - View frustum
 * 
 * Supports smooth morph transitions between levels.
 */
class LODComponent {
public:
    using Ptr = std::shared_ptr<LODComponent>;
    using LevelList = std::vector<LODLevel>;
    
    // ========================================================================
    // Construction
    // ========================================================================
    
    LODComponent();
    explicit LODComponent(const LevelList& levels);
    
    // ========================================================================
    // LOD Levels
    // ========================================================================
    
    /**
     * @brief Add LOD level
     */
    void addLevel(const LODLevel& level);
    
    /**
     * @brief Get LOD levels
     */
    const LevelList& getLevels() const noexcept { return levels_; }
    LevelList& getLevels() noexcept { return levels_; }
    
    /**
     * @brief Get number of levels
     */
    size_t getLevelCount() const noexcept { return levels_.size(); }
    
    /**
     * @brief Sort levels by distance
     */
    void sortLevels();
    
    // ========================================================================
    // LOD Selection
    // ========================================================================
    
    /**
     * @brief Select LOD based on distance
     * 
     * @param cameraPos Camera position in world space
     * @param bounds Object's world bounding sphere
     * @return Selected LOD level index
     */
    size_t selectLOD(const math::Vector3& cameraPos, 
                     const math::BoundingSphere& bounds) const;
    
    /**
     * @brief Select LOD based on screen-space error
     * 
     * @param cameraPos Camera position
     * @param bounds Object bounds
     * @param fovY Vertical field of view (radians)
     * @param viewportHeight Viewport height in pixels
     * @return Selected LOD level index
     */
    size_t selectLODByScreenSpace(const math::Vector3& cameraPos,
                                   const math::BoundingSphere& bounds,
                                   float fovY, float viewportHeight) const;
    
    /**
     * @brief Get current LOD level
     */
    LODQuality getCurrentLevel() const noexcept { return currentLevel_; }
    
    /**
     * @brief Get current level index
     */
    size_t getCurrentLevelIndex() const noexcept { return currentLevelIndex_; }
    
    /**
     * @brief Set LOD level directly (no transition)
     */
    void setLevel(LODQuality level);
    void setLevelByIndex(size_t index);
    
    // ========================================================================
    // Transitions
    // ========================================================================
    
    /**
     * @brief Enable/disable morph transitions
     */
    void setMorphEnabled(bool enabled) noexcept { morphEnabled_ = enabled; }
    bool isMorphEnabled() const noexcept { return morphEnabled_; }
    
    /**
     * @brief Update transition
     * 
     * @param deltaTime Time since last update
     * @return true if transition is still in progress
     */
    bool updateTransition(float deltaTime);
    
    /**
     * @brief Get current transition state
     */
    const LODTransition& getTransition() const noexcept { return transition_; }
    
    /**
     * @brief Get morph alpha (0.0 to 1.0)
     */
    float getMorphAlpha() const noexcept;
    
    // ========================================================================
    // Screen Space Error
    // ========================================================================
    
    /**
     * @brief Calculate screen-space error for a LOD level
     * 
     * @param level LOD level
     * @param distance Distance to camera
     * @param fovY Vertical FOV
     * @param viewportHeight Viewport height
     * @return Error in pixels
     */
    static float calculateScreenSpaceError(const LODLevel& level, float distance,
                                           float fovY, float viewportHeight);
    
    /**
     * @brief Estimate screen-space size of object
     */
    static float calculateScreenSpaceSize(float objectRadius, float distance,
                                          float fovY, float viewportHeight);
    
    // ========================================================================
    // HLOD Support
    // ========================================================================
    
    /**
     * @brief Check if this is an HLOD (hierarchical LOD)
     */
    bool isHLOD() const noexcept { return isHLOD_; }
    
    /**
     * @brief Set as HLOD
     */
    void setHLOD(bool isHLOD) noexcept { isHLOD_ = isHLOD; }
    
    /**
     * @brief Get child nodes for HLOD
     */
    const std::vector<SceneNode::Ptr>& getHLODChildren() const noexcept {
        return hlodChildren_;
    }
    
    void addHLODChild(SceneNode::Ptr child) {
        hlodChildren_.push_back(std::move(child));
    }
    
    // ========================================================================
    // Statistics
    // ========================================================================
    
    struct Stats {
        LODQuality currentLevel;
        float distanceToCamera;
        float screenSpaceError;
        float memoryUsage;
        uint32_t vertexCount;
        uint32_t triangleCount;
        bool isTransitioning;
        float transitionProgress;
    };
    
    Stats getStats() const;
    
private:
    LevelList levels_;
    LODQuality currentLevel_;
    size_t currentLevelIndex_;
    LODTransition transition_;
    bool morphEnabled_;
    bool isHLOD_;
    std::vector<SceneNode::Ptr> hlodChildren_;
};

/**
 * @brief LOD System - manages LOD for entire scene
 * 
 * Coordinates LOD updates across all LOD components.
 * Supports budget-based LOD selection.
 */
class LODSystem {
public:
    using LODList = std::vector<LODComponent::Ptr>;
    
    // ========================================================================
    // Configuration
    // ========================================================================
    
    struct Config {
        float updateInterval;      // Time between LOD updates
        size_t maxUpdatesPerFrame; // Max LOD updates per frame
        float memoryBudget;        // Target memory usage (bytes)
        float triangleBudget;      // Target triangle count
        bool useScreenSpace;       // Use screen-space error
        float fovY;                // Camera FOV
        float viewportHeight;      // Viewport height
        
        Config() noexcept
            : updateInterval(0.1f)
            , maxUpdatesPerFrame(100)
            , memoryBudget(512.0f * 1024.0f * 1024.0f) // 512 MB
            , triangleBudget(1000000) // 1M triangles
            , useScreenSpace(true)
            , fovY(1.047f) // 60 degrees
            , viewportHeight(1080.0f) {}
    };
    
    explicit LODSystem(const Config& config = Config());
    
    // ========================================================================
    // LOD Registration
    // ========================================================================
    
    void registerLOD(LODComponent::Ptr lod);
    void unregisterLOD(LODComponent* lod);
    
    // ========================================================================
    // Updates
    // ========================================================================
    
    /**
     * @brief Update all LODs
     * 
     * @param cameraPos Camera position
     * @param deltaTime Time since last update
     */
    void update(const math::Vector3& cameraPos, float deltaTime);
    
    /**
     * @brief Update single LOD
     */
    void updateLOD(LODComponent& lod, const math::Vector3& cameraPos);
    
    // ========================================================================
    // Budget Management
    // ========================================================================
    
    /**
     * @brief Set memory budget
     */
    void setMemoryBudget(float bytes) noexcept {
        config_.memoryBudget = bytes;
    }
    
    /**
     * @brief Set triangle budget
     */
    void setTriangleBudget(uint32_t count) noexcept {
        config_.triangleBudget = count;
    }
    
    /**
     * @brief Get current memory usage
     */
    float getCurrentMemoryUsage() const;
    
    /**
     * @brief Get current triangle count
     */
    uint32_t getCurrentTriangleCount() const;
    
    // ========================================================================
    // Statistics
    // ========================================================================
    
    struct Stats {
        size_t totalLODs;
        size_t updatedLODs;
        size_t transitioningLODs;
        float memoryUsage;
        uint32_t triangleCount;
        LODQuality distribution[static_cast<size_t>(LODQuality::Count)];
    };
    
    Stats getStats() const;
    
private:
    Config config_;
    LODList lods_;
    float timeSinceUpdate_;
    
    // LOD index for round-robin updates
    size_t nextLODIndex_;
};

} // namespace scene
} // namespace phoenix
