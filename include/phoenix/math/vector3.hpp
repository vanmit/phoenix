#pragma once

#include <cmath>
#include <algorithm>
#include <array>

namespace phoenix {
namespace math {

/**
 * @brief 3D Vector with SIMD-friendly layout
 * 
 * Cache-friendly structure for 3D mathematical operations.
 * Used throughout the scene system for positions, directions, and scales.
 */
struct alignas(16) Vector3 {
    float x, y, z;
    
    // ========================================================================
    // Constructors
    // ========================================================================
    
    constexpr Vector3() noexcept : x(0.0f), y(0.0f), z(0.0f) {}
    constexpr Vector3(float x, float y, float z) noexcept : x(x), y(y), z(z) {}
    explicit constexpr Vector3(float s) noexcept : x(s), y(s), z(s) {}
    
    // ========================================================================
    // Factory Methods
    // ========================================================================
    
    static constexpr Vector3 zero() noexcept { return Vector3(0.0f, 0.0f, 0.0f); }
    static constexpr Vector3 one() noexcept { return Vector3(1.0f, 1.0f, 1.0f); }
    static constexpr Vector3 unitX() noexcept { return Vector3(1.0f, 0.0f, 0.0f); }
    static constexpr Vector3 unitY() noexcept { return Vector3(0.0f, 1.0f, 0.0f); }
    static constexpr Vector3 unitZ() noexcept { return Vector3(0.0f, 0.0f, 1.0f); }
    
    // ========================================================================
    // Operators
    // ========================================================================
    
    constexpr Vector3 operator+(const Vector3& other) const noexcept {
        return Vector3(x + other.x, y + other.y, z + other.z);
    }
    
    constexpr Vector3 operator-(const Vector3& other) const noexcept {
        return Vector3(x - other.x, y - other.y, z - other.z);
    }
    
    constexpr Vector3 operator*(float scalar) const noexcept {
        return Vector3(x * scalar, y * scalar, z * scalar);
    }
    
    constexpr Vector3 operator*(const Vector3& other) const noexcept {
        return Vector3(x * other.x, y * other.y, z * other.z);
    }
    
    constexpr Vector3 operator/(float scalar) const noexcept {
        const float inv = 1.0f / scalar;
        return Vector3(x * inv, y * inv, z * inv);
    }
    
    constexpr Vector3 operator/(const Vector3& other) const noexcept {
        return Vector3(x / other.x, y / other.y, z / other.z);
    }
    
    constexpr Vector3& operator+=(const Vector3& other) noexcept {
        x += other.x; y += other.y; z += other.z;
        return *this;
    }
    
    constexpr Vector3& operator-=(const Vector3& other) noexcept {
        x -= other.x; y -= other.y; z -= other.z;
        return *this;
    }
    
    constexpr Vector3& operator*=(float scalar) noexcept {
        x *= scalar; y *= scalar; z *= scalar;
        return *this;
    }
    
    constexpr Vector3& operator/=(float scalar) noexcept {
        const float inv = 1.0f / scalar;
        x *= inv; y *= inv; z *= inv;
        return *this;
    }
    
    constexpr Vector3 operator-() const noexcept {
        return Vector3(-x, -y, -z);
    }
    
    constexpr bool operator==(const Vector3& other) const noexcept {
        return x == other.x && y == other.y && z == other.z;
    }
    
    constexpr bool operator!=(const Vector3& other) const noexcept {
        return !(*this == other);
    }
    
    constexpr float& operator[](size_t i) noexcept {
        return (&x)[i];
    }
    
    constexpr const float& operator[](size_t i) const noexcept {
        return (&x)[i];
    }
    
    // ========================================================================
    // Vector Operations
    // ========================================================================
    
    constexpr float dot(const Vector3& other) const noexcept {
        return x * other.x + y * other.y + z * other.z;
    }
    
    constexpr Vector3 cross(const Vector3& other) const noexcept {
        return Vector3(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        );
    }
    
    float length() const noexcept {
        return std::sqrt(x * x + y * y + z * z);
    }
    
    float lengthSquared() const noexcept {
        return x * x + y * y + z * z;
    }
    
    Vector3 normalized() const noexcept {
        const float len = length();
        if (len > 1e-8f) {
            const float invLen = 1.0f / len;
            return Vector3(x * invLen, y * invLen, z * invLen);
        }
        return Vector3::zero();
    }
    
    void normalize() noexcept {
        const float len = length();
        if (len > 1e-8f) {
            const float invLen = 1.0f / len;
            x *= invLen; y *= invLen; z *= invLen;
        }
    }
    
    // Fast normalize using inverse sqrt (less accurate but faster)
    Vector3 fastNormalized() const noexcept {
        const float lenSq = lengthSquared();
        if (lenSq > 1e-8f) {
            const float invLen = 1.0f / std::sqrt(lenSq);
            return Vector3(x * invLen, y * invLen, z * invLen);
        }
        return Vector3::zero();
    }
    
    constexpr Vector3 abs() const noexcept {
        return Vector3(std::abs(x), std::abs(y), std::abs(z));
    }
    
    constexpr Vector3 min(const Vector3& other) const noexcept {
        return Vector3(
            std::min(x, other.x),
            std::min(y, other.y),
            std::min(z, other.z)
        );
    }
    
    constexpr Vector3 max(const Vector3& other) const noexcept {
        return Vector3(
            std::max(x, other.x),
            std::max(y, other.y),
            std::max(z, other.z)
        );
    }
    
    constexpr float minComponent() const noexcept {
        return std::min({x, y, z});
    }
    
    constexpr float maxComponent() const noexcept {
        return std::max({x, y, z});
    }
    
    // ========================================================================
    // Utility
    // ========================================================================
    
    constexpr bool isZero() const noexcept {
        return x == 0.0f && y == 0.0f && z == 0.0f;
    }
    
    constexpr bool isNormalized() const noexcept {
        constexpr float epsilon = 1e-4f;
        return std::abs(lengthSquared() - 1.0f) < epsilon;
    }
    
    constexpr float* data() noexcept { return &x; }
    constexpr const float* data() const noexcept { return &x; }
};

// ============================================================================
// Non-member operators
// ============================================================================

inline Vector3 operator*(float scalar, const Vector3& v) noexcept {
    return v * scalar;
}

} // namespace math
} // namespace phoenix
