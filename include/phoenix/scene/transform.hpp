#pragma once

#include "../math/vector3.hpp"
#include "../math/quaternion.hpp"
#include "../math/matrix4.hpp"
#include "../math/bounding.hpp"
#include <cstdint>
#include <atomic>

namespace phoenix {
namespace scene {

/**
 * @brief Transform component for scene graph nodes
 * 
 * Supports hierarchical transforms with dirty flag optimization.
 * Uses quaternion rotation for smooth interpolation and no gimbal lock.
 * 
 * Memory layout optimized for cache coherence.
 */
struct alignas(16) Transform {
    // ========================================================================
    // Data (SoA-friendly layout)
    // ========================================================================
    
    math::Vector3 position;      // Local position
    math::Quaternion rotation;   // Local rotation (quaternion)
    math::Vector3 scale;         // Local scale
    
    // Cached world transform
    math::Matrix4 worldMatrix;
    
    // Dirty flags for incremental updates
    mutable uint32_t dirtyFlags;
    
    // Parent link (index or null)
    int32_t parentIndex;
    
    // ========================================================================
    // Dirty Flags
    // ========================================================================
    
    enum DirtyFlag : uint32_t {
        Clean = 0,
        PositionDirty = 1 << 0,
        RotationDirty = 1 << 1,
        ScaleDirty = 1 << 2,
        LocalDirty = PositionDirty | RotationDirty | ScaleDirty,
        WorldDirty = 1 << 3,
        AllDirty = LocalDirty | WorldDirty
    };
    
    // ========================================================================
    // Constructors
    // ========================================================================
    
    constexpr Transform() noexcept 
        : position(math::Vector3::zero())
        , rotation(math::Quaternion::identity())
        , scale(math::Vector3::one())
        , worldMatrix(math::Matrix4::identity())
        , dirtyFlags(AllDirty)
        , parentIndex(-1) {}
    
    constexpr Transform(const math::Vector3& pos) noexcept
        : position(pos)
        , rotation(math::Quaternion::identity())
        , scale(math::Vector3::one())
        , worldMatrix(math::Matrix4::identity())
        , dirtyFlags(AllDirty)
        , parentIndex(-1) {}
    
    constexpr Transform(const math::Vector3& pos, const math::Quaternion& rot, 
                        const math::Vector3& scl) noexcept
        : position(pos), rotation(rot), scale(scl)
        , worldMatrix(math::Matrix4::identity())
        , dirtyFlags(AllDirty)
        , parentIndex(-1) {}
    
    // ========================================================================
    // Local Transform Setters
    // ========================================================================
    
    void setPosition(const math::Vector3& pos) noexcept {
        position = pos;
        dirtyFlags |= PositionDirty | WorldDirty;
    }
    
    void setRotation(const math::Quaternion& rot) noexcept {
        rotation = rot.normalized();
        dirtyFlags |= RotationDirty | WorldDirty;
    }
    
    void setRotationFromEuler(float pitch, float yaw, float roll) noexcept {
        rotation = math::Quaternion::fromEuler(pitch, yaw, roll);
        dirtyFlags |= RotationDirty | WorldDirty;
    }
    
    void setRotationFromAxisAngle(const math::Vector3& axis, float radians) noexcept {
        rotation = math::Quaternion::fromAxisAngle(axis, radians);
        dirtyFlags |= RotationDirty | WorldDirty;
    }
    
    void setScale(const math::Vector3& scl) noexcept {
        scale = scl;
        dirtyFlags |= ScaleDirty | WorldDirty;
    }
    
    void setScale(float uniformScale) noexcept {
        scale = math::Vector3(uniformScale, uniformScale, uniformScale);
        dirtyFlags |= ScaleDirty | WorldDirty;
    }
    
    // ========================================================================
    // Local Transform Getters
    // ========================================================================
    
    constexpr const math::Vector3& getPosition() const noexcept {
        return position;
    }
    
    constexpr const math::Quaternion& getRotation() const noexcept {
        return rotation;
    }
    
    constexpr const math::Vector3& getScale() const noexcept {
        return scale;
    }
    
    math::Vector3 getEulerAngles() const noexcept {
        // Convert quaternion to euler angles
        const float x = rotation.x, y = rotation.y, z = rotation.z, w = rotation.w;
        
        // Roll (x-axis rotation)
        const float sinr_cosp = 2.0f * (w * x + y * z);
        const float cosr_cosp = 1.0f - 2.0f * (x * x + y * y);
        const float roll = std::atan2(sinr_cosp, cosr_cosp);
        
        // Pitch (y-axis rotation)
        const float sinp = 2.0f * (w * y - z * x);
        float pitch;
        if (std::abs(sinp) >= 1.0f) {
            pitch = std::copysign(3.14159265359f / 2.0f, sinp);
        } else {
            pitch = std::asin(sinp);
        }
        
        // Yaw (z-axis rotation)
        const float siny_cosp = 2.0f * (w * z + x * y);
        const float cosy_cosp = 1.0f - 2.0f * (y * y + z * z);
        const float yaw = std::atan2(siny_cosp, cosy_cosp);
        
        return math::Vector3(pitch, yaw, roll);
    }
    
    // ========================================================================
    // Local Matrix
    // ========================================================================
    
    math::Matrix4 getLocalMatrix() const noexcept {
        const math::Matrix4 T = math::Matrix4::translation(position);
        const math::Matrix4 R = rotation.toMatrix();
        const math::Matrix4 S = math::Matrix4::scale(scale);
        
        return T * R * S;
    }
    
    // ========================================================================
    // World Transform
    // ========================================================================
    
    /**
     * @brief Check if world matrix needs recalculation
     */
    constexpr bool isWorldDirty() const noexcept {
        return (dirtyFlags & WorldDirty) != 0;
    }
    
    /**
     * @brief Check if local transform has changed
     */
    constexpr bool isLocalDirty() const noexcept {
        return (dirtyFlags & LocalDirty) != 0;
    }
    
    /**
     * @brief Mark transform as clean
     */
    void markClean() noexcept {
        dirtyFlags = Clean;
    }
    
    /**
     * @brief Mark world matrix as dirty (for parent updates)
     */
    void markWorldDirty() noexcept {
        dirtyFlags |= WorldDirty;
    }
    
    /**
     * @brief Get the cached world matrix
     * 
     * Note: Call updateWorldMatrix() first if parent may have changed.
     */
    constexpr const math::Matrix4& getWorldMatrix() const noexcept {
        return worldMatrix;
    }
    
    /**
     * @brief Update world matrix based on parent
     * 
     * @param parentWorld Parent's world matrix (identity if no parent)
     * @return true if matrix was updated
     */
    bool updateWorldMatrix(const math::Matrix4& parentWorld) noexcept {
        if (!isWorldDirty() && parentWorld.isIdentity()) {
            return false;
        }
        
        const math::Matrix4 local = getLocalMatrix();
        worldMatrix = parentWorld * local;
        
        dirtyFlags = Clean;
        return true;
    }
    
    /**
     * @brief Update world matrix (no parent)
     */
    bool updateWorldMatrix() noexcept {
        if (!isWorldDirty()) {
            return false;
        }
        
        worldMatrix = getLocalMatrix();
        dirtyFlags = Clean;
        return true;
    }
    
    // ========================================================================
    // World Space Operations
    // ========================================================================
    
    math::Vector3 getWorldPosition() const noexcept {
        return math::Vector3(worldMatrix[12], worldMatrix[13], worldMatrix[14]);
    }
    
    math::Quaternion getWorldRotation() const noexcept {
        return math::Quaternion::fromMatrix(worldMatrix);
    }
    
    math::Vector3 getWorldScale() const noexcept {
        const math::Vector3 scaleX(worldMatrix[0], worldMatrix[1], worldMatrix[2]);
        const math::Vector3 scaleY(worldMatrix[4], worldMatrix[5], worldMatrix[6]);
        const math::Vector3 scaleZ(worldMatrix[8], worldMatrix[9], worldMatrix[10]);
        
        return math::Vector3(scaleX.length(), scaleY.length(), scaleZ.length());
    }
    
    // ========================================================================
    // Transform Operations
    // ========================================================================
    
    /**
     * @brief Transform point from local to world space
     */
    math::Vector3 transformPoint(const math::Vector3& localPoint) const noexcept {
        return worldMatrix.transformPoint(localPoint);
    }
    
    /**
     * @brief Transform direction from local to world space
     */
    math::Vector3 transformDirection(const math::Vector3& localDir) const noexcept {
        return worldMatrix.transformDirection(localDir);
    }
    
    /**
     * @brief Transform point from world to local space
     */
    math::Vector3 inverseTransformPoint(const math::Vector3& worldPoint) const noexcept {
        return worldMatrix.inverted().transformPoint(worldPoint);
    }
    
    /**
     * @brief Get forward direction in world space
     */
    math::Vector3 getForward() const noexcept {
        return transformDirection(math::Vector3(0.0f, 0.0f, -1.0f));
    }
    
    /**
     * @brief Get right direction in world space
     */
    math::Vector3 getRight() const noexcept {
        return transformDirection(math::Vector3(1.0f, 0.0f, 0.0f));
    }
    
    /**
     * @brief Get up direction in world space
     */
    math::Vector3 getUp() const noexcept {
        return transformDirection(math::Vector3(0.0f, 1.0f, 0.0f));
    }
    
    // ========================================================================
    // Look At
    // ========================================================================
    
    void lookAt(const math::Vector3& target, const math::Vector3& up = math::Vector3::unitY()) noexcept {
        const math::Vector3 forward = (target - position).normalized();
        const math::Vector3 right = forward.cross(up).normalized();
        const math::Vector3 actualUp = right.cross(forward);
        
        const math::Matrix4 lookAt = math::Matrix4::lookAt(math::Vector3::zero(), forward, actualUp);
        rotation = math::Quaternion::fromMatrix(lookAt.inverted());
        dirtyFlags |= RotationDirty | WorldDirty;
    }
    
    // ========================================================================
    // Bounding Volume
    // ========================================================================
    
    math::BoundingBox getWorldBoundingBox(const math::BoundingBox& localBox) const noexcept {
        math::BoundingBox worldBox;
        worldBox.transform(worldMatrix);
        
        // Transform local box corners
        const auto corners = localBox.corners();
        worldBox = math::BoundingBox::empty();
        for (const auto& corner : corners) {
            worldBox.extend(transformPoint(corner));
        }
        
        return worldBox;
    }
    
    math::BoundingSphere getWorldBoundingSphere(const math::BoundingSphere& localSphere) const noexcept {
        math::BoundingSphere worldSphere = localSphere;
        worldSphere.transform(worldMatrix);
        return worldSphere;
    }
    
    // ========================================================================
    // Interpolation
    // ========================================================================
    
    static Transform lerp(const Transform& a, const Transform& b, float t) noexcept {
        Transform result;
        result.position = math::Vector3::lerp(a.position, b.position, t);
        result.rotation = math::Quaternion::slerp(a.rotation, b.rotation, t);
        result.scale = math::Vector3::lerp(a.scale, b.scale, t);
        result.dirtyFlags = AllDirty;
        return result;
    }
    
    // ========================================================================
    // Operators
    // ========================================================================
    
    constexpr bool operator==(const Transform& other) const noexcept {
        return position == other.position && 
               rotation == other.rotation && 
               scale == other.scale;
    }
    
    constexpr bool operator!=(const Transform& other) const noexcept {
        return !(*this == other);
    }
};

} // namespace scene
} // namespace phoenix
