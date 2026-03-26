#pragma once

#include "vector3.hpp"
#include "matrix4.hpp"
#include "bounding.hpp"
#include <array>

namespace phoenix {
namespace math {

/**
 * @brief View Frustum for visibility culling
 * 
 * Represents the camera's viewable volume as 6 planes.
 * Used for frustum culling in the scene graph.
 */
struct alignas(16) Frustum {
    // Plane indices
    enum Plane : size_t {
        Left = 0,
        Right = 1,
        Bottom = 2,
        Top = 3,
        Near = 4,
        Far = 5,
        Count = 6
    };
    
    // Plane: ax + by + cz + d = 0, normal points inward
    struct alignas(16) PlaneData {
        Vector3 normal;  // (a, b, c)
        float d;         // Distance from origin
        
        constexpr PlaneData() noexcept : normal(), d(0.0f) {}
        constexpr PlaneData(const Vector3& n, float dist) noexcept : normal(n), d(dist) {}
        
        constexpr float distanceToPoint(const Vector3& p) const noexcept {
            return normal.dot(p) + d;
        }
        
        constexpr bool isInside(const Vector3& p) const noexcept {
            return distanceToPoint(p) >= 0.0f;
        }
    };
    
    std::array<PlaneData, Count> planes;
    
    // ========================================================================
    // Constructors
    // ========================================================================
    
    constexpr Frustum() noexcept = default;
    
    // ========================================================================
    // Factory Methods
    // ========================================================================
    
    static Frustum fromViewProjection(const Matrix4& viewProj) noexcept {
        Frustum frustum;
        
        // Extract planes from view-projection matrix
        // Column-major order
        
        // Left plane
        frustum.planes[Left].normal.x = viewProj[3] + viewProj[0];
        frustum.planes[Left].normal.y = viewProj[7] + viewProj[4];
        frustum.planes[Left].normal.z = viewProj[11] + viewProj[8];
        frustum.planes[Left].d = viewProj[15] + viewProj[12];
        
        // Right plane
        frustum.planes[Right].normal.x = viewProj[3] - viewProj[0];
        frustum.planes[Right].normal.y = viewProj[7] - viewProj[4];
        frustum.planes[Right].normal.z = viewProj[11] - viewProj[8];
        frustum.planes[Right].d = viewProj[15] - viewProj[12];
        
        // Bottom plane
        frustum.planes[Bottom].normal.x = viewProj[3] + viewProj[1];
        frustum.planes[Bottom].normal.y = viewProj[7] + viewProj[5];
        frustum.planes[Bottom].normal.z = viewProj[11] + viewProj[9];
        frustum.planes[Bottom].d = viewProj[15] + viewProj[13];
        
        // Top plane
        frustum.planes[Top].normal.x = viewProj[3] - viewProj[1];
        frustum.planes[Top].normal.y = viewProj[7] - viewProj[5];
        frustum.planes[Top].normal.z = viewProj[11] - viewProj[9];
        frustum.planes[Top].d = viewProj[15] - viewProj[13];
        
        // Near plane
        frustum.planes[Near].normal.x = viewProj[3] + viewProj[2];
        frustum.planes[Near].normal.y = viewProj[7] + viewProj[6];
        frustum.planes[Near].normal.z = viewProj[11] + viewProj[10];
        frustum.planes[Near].d = viewProj[15] + viewProj[14];
        
        // Far plane
        frustum.planes[Far].normal.x = viewProj[3] - viewProj[2];
        frustum.planes[Far].normal.y = viewProj[7] - viewProj[6];
        frustum.planes[Far].normal.z = viewProj[11] - viewProj[10];
        frustum.planes[Far].d = viewProj[15] - viewProj[14];
        
        // Normalize planes
        for (auto& plane : frustum.planes) {
            const float len = plane.normal.length();
            if (len > 1e-8f) {
                const float invLen = 1.0f / len;
                plane.normal *= invLen;
                plane.d *= invLen;
            }
        }
        
        return frustum;
    }
    
    static Frustum fromPerspective(const Vector3& position, const Vector3& forward, 
                                   const Vector3& up, float fovY, float aspect,
                                   float nearDist, float farDist) noexcept {
        const Vector3 right = forward.cross(up).normalized();
        const Vector3 actualUp = right.cross(forward).normalized();
        
        const float tanHalfFov = std::tan(fovY * 0.5f);
        const float nearHeight = nearDist * tanHalfFov;
        const float nearWidth = nearHeight * aspect;
        const float farHeight = farDist * tanHalfFov;
        const float farWidth = farHeight * aspect;
        
        // Calculate frustum corners
        const Vector3 nearCenter = position + forward * nearDist;
        const Vector3 farCenter = position + forward * farDist;
        
        const Vector3 nearTopLeft = nearCenter + actualUp * nearHeight - right * nearWidth;
        const Vector3 nearTopRight = nearCenter + actualUp * nearHeight + right * nearWidth;
        const Vector3 nearBottomLeft = nearCenter - actualUp * nearHeight - right * nearWidth;
        const Vector3 nearBottomRight = nearCenter - actualUp * nearHeight + right * nearWidth;
        
        const Vector3 farTopLeft = farCenter + actualUp * farHeight - right * farWidth;
        const Vector3 farTopRight = farCenter + actualUp * farHeight + right * farWidth;
        const Vector3 farBottomLeft = farCenter - actualUp * farHeight - right * farWidth;
        const Vector3 farBottomRight = farCenter - actualUp * farHeight + right * farWidth;
        
        Frustum frustum;
        
        // Create planes from corners
        frustum.planes[Left] = planeFromPoints(nearBottomLeft, farBottomLeft, farTopLeft);
        frustum.planes[Right] = planeFromPoints(nearBottomRight, farTopRight, farBottomRight);
        frustum.planes[Bottom] = planeFromPoints(nearBottomLeft, nearBottomRight, farBottomRight);
        frustum.planes[Top] = planeFromPoints(nearTopLeft, farTopLeft, farTopRight);
        frustum.planes[Near] = planeFromPoints(nearTopLeft, nearTopRight, nearBottomRight);
        frustum.planes[Far] = planeFromPoints(farTopLeft, farBottomLeft, farBottomRight);
        
        return frustum;
    }
    
    // ========================================================================
    // Plane Creation
    // ========================================================================
    
    static constexpr PlaneData planeFromPoints(const Vector3& a, const Vector3& b, 
                                                const Vector3& c) noexcept {
        const Vector3 edge1 = b - a;
        const Vector3 edge2 = c - a;
        Vector3 normal = edge1.cross(edge2).normalized();
        const float d = -normal.dot(a);
        return PlaneData(normal, d);
    }
    
    // ========================================================================
    // Intersection Tests
    // ========================================================================
    
    /**
     * @brief Test if point is inside frustum
     * @return true if inside or on boundary
     */
    constexpr bool contains(const Vector3& point) const noexcept {
        for (const auto& plane : planes) {
            if (!plane.isInside(point)) {
                return false;
            }
        }
        return true;
    }
    
    /**
     * @brief Test if sphere is inside frustum
     * @return true if inside or intersects
     */
    constexpr bool intersectsSphere(const Vector3& center, float radius) const noexcept {
        for (const auto& plane : planes) {
            const float dist = plane.distanceToPoint(center);
            if (dist < -radius) {
                return false; // Completely outside
            }
        }
        return true; // Inside or intersects
    }
    
    constexpr bool intersectsSphere(const BoundingSphere& sphere) const noexcept {
        return intersectsSphere(sphere.center, sphere.radius);
    }
    
    /**
     * @brief Test if AABB is inside frustum
     * 
     * Uses optimized test that selects the appropriate corner
     * for each plane based on plane normal direction.
     */
    constexpr bool intersectsBox(const BoundingBox& box) const noexcept {
        if (box.isEmpty()) return false;
        
        const Vector3 center = box.center();
        const Vector3 extents = box.extents();
        
        for (const auto& plane : planes) {
            // Compute the projection interval radius of b onto L(t) = c + t * p.n
            const float r = extents.x * std::abs(plane.normal.x) +
                           extents.y * std::abs(plane.normal.y) +
                           extents.z * std::abs(plane.normal.z);
            
            // Compute distance of box's center from origin
            const float dist = plane.normal.dot(center) + plane.d;
            
            // Check if box is outside plane
            if (dist < -r) {
                return false;
            }
        }
        
        return true;
    }
    
    /**
     * @brief Test if frustum completely contains box
     */
    constexpr bool containsBox(const BoundingBox& box) const noexcept {
        const auto corners = box.corners();
        for (const auto& corner : corners) {
            if (!contains(corner)) {
                return false;
            }
        }
        return true;
    }
    
    /**
     * @brief Get frustum classification result
     */
    enum class Intersection {
        Outside,    // Completely outside
        Intersects, // Partially inside
        Inside      // Completely inside
    };
    
    constexpr Intersection classifySphere(const Vector3& center, float radius) const noexcept {
        bool insideAll = true;
        
        for (const auto& plane : planes) {
            const float dist = plane.distanceToPoint(center);
            if (dist < -radius) {
                return Intersection::Outside;
            }
            if (dist < radius) {
                insideAll = false;
            }
        }
        
        return insideAll ? Intersection::Inside : Intersection::Intersects;
    }
    
    constexpr Intersection classifySphere(const BoundingSphere& sphere) const noexcept {
        return classifySphere(sphere.center, sphere.radius);
    }
    
    constexpr Intersection classifyBox(const BoundingBox& box) const noexcept {
        if (box.isEmpty()) return Intersection::Outside;
        
        const Vector3 center = box.center();
        const Vector3 extents = box.extents();
        
        bool insideAll = true;
        
        for (const auto& plane : planes) {
            const float r = extents.x * std::abs(plane.normal.x) +
                           extents.y * std::abs(plane.normal.y) +
                           extents.z * std::abs(plane.normal.z);
            
            const float dist = plane.normal.dot(center) + plane.d;
            
            if (dist < -r) {
                return Intersection::Outside;
            }
            if (dist < r) {
                insideAll = false;
            }
        }
        
        return insideAll ? Intersection::Inside : Intersection::Intersects;
    }
    
    // ========================================================================
    // Corners
    // ========================================================================
    
    std::array<Vector3, 8> corners() const noexcept {
        // Compute intersection points of 3 planes at a time
        std::array<Vector3, 8> result;
        
        // Near plane corners
        result[0] = intersectPlanes(Left, Bottom, Near);
        result[1] = intersectPlanes(Right, Bottom, Near);
        result[2] = intersectPlanes(Right, Top, Near);
        result[3] = intersectPlanes(Left, Top, Near);
        
        // Far plane corners
        result[4] = intersectPlanes(Left, Bottom, Far);
        result[5] = intersectPlanes(Right, Bottom, Far);
        result[6] = intersectPlanes(Right, Top, Far);
        result[7] = intersectPlanes(Left, Top, Far);
        
        return result;
    }
    
    Vector3 intersectPlanes(size_t i, size_t j, size_t k) const noexcept {
        const auto& p1 = planes[i];
        const auto& p2 = planes[j];
        const auto& p3 = planes[k];
        
        // Solve system of 3 plane equations
        const Vector3 cross12 = p1.normal.cross(p2.normal);
        const Vector3 cross23 = p2.normal.cross(p3.normal);
        const Vector3 cross31 = p3.normal.cross(p1.normal);
        
        const float det = p1.normal.dot(cross23);
        if (std::abs(det) < 1e-8f) {
            return Vector3::zero(); // Planes don't intersect at single point
        }
        
        const float invDet = 1.0f / det;
        
        return (cross23 * p1.d + cross31 * p2.d + cross12 * p3.d) * invDet;
    }
    
    // ========================================================================
    // Utility
    // ========================================================================
    
    constexpr const PlaneData& getPlane(Plane plane) const noexcept {
        return planes[static_cast<size_t>(plane)];
    }
    
    constexpr PlaneData& getPlane(Plane plane) noexcept {
        return planes[static_cast<size_t>(plane)];
    }
};

} // namespace math
} // namespace phoenix
