#pragma once

#include "vector3.hpp"
#include "matrix4.hpp"
#include <algorithm>
#include <array>

namespace phoenix {
namespace math {

/**
 * @brief Axis-Aligned Bounding Box
 * 
 * Efficient bounding volume for culling and collision detection.
 * Stored as min/max corners for cache-friendly access.
 */
struct alignas(16) BoundingBox {
    Vector3 min;
    Vector3 max;
    
    // ========================================================================
    // Constructors
    // ========================================================================
    
    constexpr BoundingBox() noexcept 
        : min(FLT_MAX, FLT_MAX, FLT_MAX), max(-FLT_MAX, -FLT_MAX, -FLT_MAX) {}
    
    constexpr BoundingBox(const Vector3& min, const Vector3& max) noexcept 
        : min(min), max(max) {}
    
    explicit constexpr BoundingBox(const Vector3& point) noexcept 
        : min(point), max(point) {}
    
    // ========================================================================
    // Factory Methods
    // ========================================================================
    
    static constexpr BoundingBox empty() noexcept {
        return BoundingBox(
            Vector3(FLT_MAX, FLT_MAX, FLT_MAX),
            Vector3(-FLT_MAX, -FLT_MAX, -FLT_MAX)
        );
    }
    
    static constexpr BoundingBox unit() noexcept {
        return BoundingBox(Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f, 1.0f, 1.0f));
    }
    
    static constexpr BoundingBox fromCenterAndSize(const Vector3& center, const Vector3& size) noexcept {
        const Vector3 halfSize = size * 0.5f;
        return BoundingBox(center - halfSize, center + halfSize);
    }
    
    static BoundingBox fromPoints(const Vector3* points, size_t count) noexcept {
        BoundingBox result;
        for (size_t i = 0; i < count; ++i) {
            result.extend(points[i]);
        }
        return result;
    }
    
    // ========================================================================
    // Properties
    // ========================================================================
    
    constexpr Vector3 center() const noexcept {
        return (min + max) * 0.5f;
    }
    
    constexpr Vector3 size() const noexcept {
        return max - min;
    }
    
    constexpr Vector3 extents() const noexcept {
        return size() * 0.5f;
    }
    
    constexpr float volume() const noexcept {
        const Vector3 s = size();
        return s.x * s.y * s.z;
    }
    
    constexpr float surfaceArea() const noexcept {
        const Vector3 d = size();
        return 2.0f * (d.x * d.y + d.y * d.z + d.z * d.x);
    }
    
    constexpr bool isValid() const noexcept {
        return min.x <= max.x && min.y <= max.y && min.z <= max.z;
    }
    
    constexpr bool isEmpty() const noexcept {
        return min.x > max.x || min.y > max.y || min.z > max.z;
    }
    
    // ========================================================================
    // Modification
    // ========================================================================
    
    void extend(const Vector3& point) noexcept {
        min = min.min(point);
        max = max.max(point);
    }
    
    void extend(const BoundingBox& other) noexcept {
        min = min.min(other.min);
        max = max.max(other.max);
    }
    
    void expand(float amount) noexcept {
        min -= Vector3(amount, amount, amount);
        max += Vector3(amount, amount, amount);
    }
    
    void transform(const Matrix4& matrix) noexcept {
        const Vector3 corners[8] = {
            {min.x, min.y, min.z}, {max.x, min.y, min.z},
            {min.x, max.y, min.z}, {max.x, max.y, min.z},
            {min.x, min.y, max.z}, {max.x, min.y, max.z},
            {min.x, max.y, max.z}, {max.x, max.y, max.z}
        };
        
        *this = BoundingBox::empty();
        for (const auto& corner : corners) {
            extend(matrix.transformPoint(corner));
        }
    }
    
    // ========================================================================
    // Intersection Tests
    // ========================================================================
    
    constexpr bool contains(const Vector3& point) const noexcept {
        return point.x >= min.x && point.x <= max.x &&
               point.y >= min.y && point.y <= max.y &&
               point.z >= min.z && point.z <= max.z;
    }
    
    constexpr bool contains(const BoundingBox& other) const noexcept {
        return other.min.x >= min.x && other.max.x <= max.x &&
               other.min.y >= min.y && other.max.y <= max.y &&
               other.min.z >= min.z && other.max.z <= max.z;
    }
    
    constexpr bool intersects(const BoundingBox& other) const noexcept {
        return min.x <= other.max.x && max.x >= other.min.x &&
               min.y <= other.max.y && max.y >= other.min.y &&
               min.z <= other.max.z && max.z >= other.min.z;
    }
    
    bool intersectsSphere(const Vector3& center, float radius) const noexcept {
        const Vector3 closest(
            std::max(min.x, std::min(center.x, max.x)),
            std::max(min.y, std::min(center.y, max.y)),
            std::max(min.z, std::min(center.z, max.z))
        );
        
        const Vector3 diff = closest - center;
        return diff.lengthSquared() <= radius * radius;
    }
    
    // ========================================================================
    // Distance
    // ========================================================================
    
    float distanceToPoint(const Vector3& point) const noexcept {
        if (contains(point)) return 0.0f;
        
        const Vector3 closest(
            std::max(min.x, std::min(point.x, max.x)),
            std::max(min.y, std::min(point.y, max.y)),
            std::max(min.z, std::min(point.z, max.z))
        );
        
        return (closest - point).length();
    }
    
    float distanceToBox(const BoundingBox& other) const noexcept {
        if (intersects(other)) return 0.0f;
        
        const Vector3 c1 = center();
        const Vector3 c2 = other.center();
        const Vector3 e1 = extents();
        const Vector3 e2 = other.extents();
        
        const Vector3 d = (c2 - c1).abs() - e1 - e2;
        const Vector3 clamped(d.x > 0 ? d.x : 0, d.y > 0 ? d.y : 0, d.z > 0 ? d.z : 0);
        
        return clamped.length();
    }
    
    // ========================================================================
    // Corners
    // ========================================================================
    
    std::array<Vector3, 8> corners() const noexcept {
        return {{
            {min.x, min.y, min.z}, {max.x, min.y, min.z},
            {min.x, max.y, min.z}, {max.x, max.y, min.z},
            {min.x, min.y, max.z}, {max.x, min.y, max.z},
            {min.x, max.y, max.z}, {max.x, max.y, max.z}
        }};
    }
    
    // ========================================================================
    // Operators
    // ========================================================================
    
    constexpr bool operator==(const BoundingBox& other) const noexcept {
        return min == other.min && max == other.max;
    }
    
    constexpr bool operator!=(const BoundingBox& other) const noexcept {
        return !(*this == other);
    }
};

/**
 * @brief Bounding Sphere
 * 
 * Simpler bounding volume, faster intersection tests.
 * Useful for coarse culling before AABB tests.
 */
struct alignas(16) BoundingSphere {
    Vector3 center;
    float radius;
    
    // ========================================================================
    // Constructors
    // ========================================================================
    
    constexpr BoundingSphere() noexcept : center(), radius(0.0f) {}
    constexpr BoundingSphere(const Vector3& center, float radius) noexcept 
        : center(center), radius(radius) {}
    
    // ========================================================================
    // Factory Methods
    // ========================================================================
    
    static constexpr BoundingSphere empty() noexcept {
        return BoundingSphere(Vector3::zero(), 0.0f);
    }
    
    static BoundingSphere fromPoints(const Vector3* points, size_t count) noexcept {
        if (count == 0) return empty();
        
        // Ritter's algorithm for bounding sphere
        BoundingSphere sphere(points[0], 0.0f);
        
        for (size_t i = 0; i < count; ++i) {
            if (!sphere.contains(points[i])) {
                const float dist = (points[i] - sphere.center).length();
                sphere.radius = (sphere.radius + dist) * 0.5f;
                sphere.center = sphere.center + (points[i] - sphere.center) * 
                               ((sphere.radius - dist) / dist);
            }
        }
        
        return sphere;
    }
    
    static BoundingSphere fromBox(const BoundingBox& box) noexcept {
        return BoundingSphere(box.center(), box.extents().length());
    }
    
    // ========================================================================
    // Properties
    // ========================================================================
    
    constexpr float diameter() const noexcept {
        return radius * 2.0f;
    }
    
    constexpr float volume() const noexcept {
        constexpr float pi = 3.14159265359f;
        return (4.0f / 3.0f) * pi * radius * radius * radius;
    }
    
    constexpr float surfaceArea() const noexcept {
        constexpr float pi = 3.14159265359f;
        return 4.0f * pi * radius * radius;
    }
    
    // ========================================================================
    // Modification
    // ========================================================================
    
    void extend(const Vector3& point) noexcept {
        if (!contains(point)) {
            const float dist = (point - center).length();
            radius = (radius + dist) * 0.5f;
            center = center + (point - center) * ((radius - dist) / dist);
        }
    }
    
    void extend(const BoundingSphere& other) noexcept {
        if (other.radius <= 0.0f) return;
        if (radius <= 0.0f) {
            *this = other;
            return;
        }
        
        const float dist = (other.center - center).length();
        if (dist + other.radius <= radius) return; // Already contains other
        
        if (dist + radius <= other.radius) {
            *this = other;
            return;
        }
        
        const float newRadius = (dist + radius + other.radius) * 0.5f;
        const float t = (newRadius - radius) / dist;
        center = center + (other.center - center) * t;
        radius = newRadius;
    }
    
    void transform(const Matrix4& matrix) noexcept {
        center = matrix.transformPoint(center);
        
        // Transform radius by maximum scale of matrix
        const Vector3 scaleX = matrix.transformVector(Vector3::unitX());
        const Vector3 scaleY = matrix.transformVector(Vector3::unitY());
        const Vector3 scaleZ = matrix.transformVector(Vector3::unitZ());
        
        const float maxScale = std::max({scaleX.length(), scaleY.length(), scaleZ.length()});
        radius *= maxScale;
    }
    
    // ========================================================================
    // Intersection Tests
    // ========================================================================
    
    constexpr bool contains(const Vector3& point) const noexcept {
        return (point - center).lengthSquared() <= radius * radius;
    }
    
    constexpr bool contains(const BoundingSphere& other) const noexcept {
        return (other.center - center).length() + other.radius <= radius;
    }
    
    constexpr bool intersects(const BoundingSphere& other) const noexcept {
        const float distSq = (other.center - center).lengthSquared();
        const float radiusSum = radius + other.radius;
        return distSq <= radiusSum * radiusSum;
    }
    
    bool intersects(const BoundingBox& box) const noexcept {
        return box.intersectsSphere(center, radius);
    }
    
    // ========================================================================
    // Distance
    // ========================================================================
    
    float distanceToPoint(const Vector3& point) const noexcept {
        return std::max(0.0f, (point - center).length() - radius);
    }
    
    float distanceToSphere(const BoundingSphere& other) const noexcept {
        return std::max(0.0f, (other.center - center).length() - radius - other.radius);
    }
    
    // ========================================================================
    // Operators
    // ========================================================================
    
    constexpr bool operator==(const BoundingSphere& other) const noexcept {
        return center == other.center && radius == other.radius;
    }
    
    constexpr bool operator!=(const BoundingSphere& other) const noexcept {
        return !(*this == other);
    }
};

} // namespace math
} // namespace phoenix
