#pragma once

#include "vector3.hpp"
#include "matrix4.hpp"
#include <cmath>

namespace phoenix {
namespace math {

/**
 * @brief Quaternion for 3D rotation representation
 * 
 * Provides efficient rotation operations without gimbal lock.
 * Used for smooth interpolation and composition of rotations.
 */
struct alignas(16) Quaternion {
    float x, y, z, w;
    
    // ========================================================================
    // Constructors
    // ========================================================================
    
    constexpr Quaternion() noexcept : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {}
    constexpr Quaternion(float x, float y, float z, float w) noexcept 
        : x(x), y(y), z(z), w(w) {}
    
    // ========================================================================
    // Factory Methods
    // ========================================================================
    
    static constexpr Quaternion identity() noexcept {
        return Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
    }
    
    static Quaternion fromAxisAngle(const Vector3& axis, float radians) noexcept {
        const float halfAngle = radians * 0.5f;
        const float s = std::sin(halfAngle);
        const Vector3 n = axis.normalized();
        return Quaternion(n.x * s, n.y * s, n.z * s, std::cos(halfAngle));
    }
    
    static Quaternion fromEuler(float pitch, float yaw, float roll) noexcept {
        const float cp = std::cos(pitch * 0.5f);
        const float sp = std::sin(pitch * 0.5f);
        const float cy = std::cos(yaw * 0.5f);
        const float sy = std::sin(yaw * 0.5f);
        const float cr = std::cos(roll * 0.5f);
        const float sr = std::sin(roll * 0.5f);
        
        return Quaternion(
            sr * cp * cy - cr * sp * sy,
            cr * sp * cy + sr * cp * sy,
            cr * cp * sy - sr * sp * cy,
            cr * cp * cy + sr * sp * sy
        );
    }
    
    static Quaternion fromVectors(const Vector3& from, const Vector3& to) noexcept {
        const Vector3 f = from.normalized();
        const Vector3 t = to.normalized();
        
        const float dot = f.dot(t);
        if (dot >= 1.0f) {
            return identity();
        }
        if (dot <= -1.0f) {
            // 180 degree rotation - need arbitrary perpendicular axis
            Vector3 axis;
            if (std::abs(f.x) < 0.9f) {
                axis = f.cross(Vector3::unitX()).normalized();
            } else {
                axis = f.cross(Vector3::unitY()).normalized();
            }
            return fromAxisAngle(axis, 3.14159265359f);
        }
        
        const Vector3 c = f.cross(t);
        const float s = std::sqrt((1.0f + dot) * 2.0f);
        const float invS = 1.0f / s;
        
        return Quaternion(c.x * invS, c.y * invS, c.z * invS, s * 0.5f);
    }
    
    static Quaternion fromMatrix(const Matrix4& m) noexcept {
        const float trace = m.data[0] + m.data[5] + m.data[10];
        
        if (trace > 0.0f) {
            const float s = std::sqrt(trace + 1.0f) * 2.0f;
            const float invS = 1.0f / s;
            return Quaternion(
                (m.data[9] - m.data[6]) * invS,
                (m.data[2] - m.data[8]) * invS,
                (m.data[4] - m.data[1]) * invS,
                s * 0.25f
            );
        } else if (m.data[0] > m.data[5] && m.data[0] > m.data[10]) {
            const float s = std::sqrt(1.0f + m.data[0] - m.data[5] - m.data[10]) * 2.0f;
            const float invS = 1.0f / s;
            return Quaternion(
                s * 0.25f,
                (m.data[4] + m.data[1]) * invS,
                (m.data[2] + m.data[8]) * invS,
                (m.data[9] - m.data[6]) * invS
            );
        } else if (m.data[5] > m.data[10]) {
            const float s = std::sqrt(1.0f + m.data[5] - m.data[0] - m.data[10]) * 2.0f;
            const float invS = 1.0f / s;
            return Quaternion(
                (m.data[4] + m.data[1]) * invS,
                s * 0.25f,
                (m.data[9] + m.data[6]) * invS,
                (m.data[2] - m.data[8]) * invS
            );
        } else {
            const float s = std::sqrt(1.0f + m.data[10] - m.data[0] - m.data[5]) * 2.0f;
            const float invS = 1.0f / s;
            return Quaternion(
                (m.data[2] + m.data[8]) * invS,
                (m.data[9] + m.data[6]) * invS,
                s * 0.25f,
                (m.data[4] - m.data[1]) * invS
            );
        }
    }
    
    // ========================================================================
    // Operators
    // ========================================================================
    
    constexpr Quaternion operator+(const Quaternion& other) const noexcept {
        return Quaternion(x + other.x, y + other.y, z + other.z, w + other.w);
    }
    
    constexpr Quaternion operator-(const Quaternion& other) const noexcept {
        return Quaternion(x - other.x, y - other.y, z - other.z, w - other.w);
    }
    
    Quaternion operator*(const Quaternion& other) const noexcept {
        return Quaternion(
            w * other.x + x * other.w + y * other.z - z * other.y,
            w * other.y - x * other.z + y * other.w + z * other.x,
            w * other.z + x * other.y - y * other.x + z * other.w,
            w * other.w - x * other.x - y * other.y - z * other.z
        );
    }
    
    constexpr Quaternion operator*(float scalar) const noexcept {
        return Quaternion(x * scalar, y * scalar, z * scalar, w * scalar);
    }
    
    constexpr Quaternion& operator*=(const Quaternion& other) noexcept {
        *this = *this * other;
        return *this;
    }
    
    constexpr bool operator==(const Quaternion& other) const noexcept {
        return x == other.x && y == other.y && z == other.z && w == other.w;
    }
    
    constexpr bool operator!=(const Quaternion& other) const noexcept {
        return !(*this == other);
    }
    
    // ========================================================================
    // Quaternion Operations
    // ========================================================================
    
    float length() const noexcept {
        return std::sqrt(x * x + y * y + z * z + w * w);
    }
    
    float lengthSquared() const noexcept {
        return x * x + y * y + z * z + w * w;
    }
    
    Quaternion normalized() const noexcept {
        const float len = length();
        if (len > 1e-8f) {
            const float invLen = 1.0f / len;
            return Quaternion(x * invLen, y * invLen, z * invLen, w * invLen);
        }
        return *this;
    }
    
    void normalize() noexcept {
        const float len = length();
        if (len > 1e-8f) {
            const float invLen = 1.0f / len;
            x *= invLen; y *= invLen; z *= invLen; w *= invLen;
        }
    }
    
    Quaternion conjugated() const noexcept {
        return Quaternion(-x, -y, -z, w);
    }
    
    void conjugate() noexcept {
        x = -x; y = -y; z = -z;
    }
    
    Quaternion inverted() const noexcept {
        const float lenSq = lengthSquared();
        if (lenSq > 1e-8f) {
            const float invLenSq = 1.0f / lenSq;
            return Quaternion(-x * invLenSq, -y * invLenSq, -z * invLenSq, w * invLenSq);
        }
        return *this;
    }
    
    void invert() noexcept {
        *this = inverted();
    }
    
    // ========================================================================
    // Rotation Operations
    // ========================================================================
    
    Vector3 rotate(const Vector3& v) const noexcept {
        const Vector3 qv(x, y, z);
        const Vector3 uv = qv.cross(v);
        const Vector3 uuv = qv.cross(uv);
        return v + ((uv * w) + uuv) * 2.0f;
    }
    
    Vector3 inverseRotate(const Vector3& v) const noexcept {
        return conjugated().rotate(v);
    }
    
    Matrix4 toMatrix() const noexcept {
        Matrix4 m;
        const float xx = x * x, yy = y * y, zz = z * z;
        const float xy = x * y, xz = x * z, yz = y * z;
        const float wx = w * x, wy = w * y, wz = w * z;
        
        m.data[0] = 1.0f - 2.0f * (yy + zz);
        m.data[1] = 2.0f * (xy + wz);
        m.data[2] = 2.0f * (xz - wy);
        
        m.data[4] = 2.0f * (xy - wz);
        m.data[5] = 1.0f - 2.0f * (xx + zz);
        m.data[6] = 2.0f * (yz + wx);
        
        m.data[8] = 2.0f * (xz + wy);
        m.data[9] = 2.0f * (yz - wx);
        m.data[10] = 1.0f - 2.0f * (xx + yy);
        
        return m;
    }
    
    // ========================================================================
    // Interpolation
    // ========================================================================
    
    static Quaternion lerp(const Quaternion& a, const Quaternion& b, float t) noexcept {
        return Quaternion(
            a.x + (b.x - a.x) * t,
            a.y + (b.y - a.y) * t,
            a.z + (b.z - a.z) * t,
            a.w + (b.w - a.w) * t
        ).normalized();
    }
    
    static Quaternion slerp(const Quaternion& a, const Quaternion& b, float t) noexcept {
        float cosTheta = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
        
        // Take shortest path
        Quaternion qb = b;
        if (cosTheta < 0.0f) {
            qb = Quaternion(-b.x, -b.y, -b.z, -b.w);
            cosTheta = -cosTheta;
        }
        
        if (cosTheta > 0.9995f) {
            // Nearly parallel - use linear interpolation
            return lerp(a, qb, t).normalized();
        }
        
        const float theta = std::acos(cosTheta);
        const float sinTheta = std::sin(theta);
        const float invSinTheta = 1.0f / sinTheta;
        
        const float ra = std::sin((1.0f - t) * theta) * invSinTheta;
        const float rb = std::sin(t * theta) * invSinTheta;
        
        return Quaternion(
            a.x * ra + qb.x * rb,
            a.y * ra + qb.y * rb,
            a.z * ra + qb.z * rb,
            a.w * ra + qb.w * rb
        );
    }
    
    // ========================================================================
    // Utility
    // ========================================================================
    
    constexpr bool isIdentity() const noexcept {
        constexpr float epsilon = 1e-6f;
        return std::abs(x) < epsilon && std::abs(y) < epsilon && 
               std::abs(z) < epsilon && std::abs(w - 1.0f) < epsilon;
    }
    
    constexpr float* data() noexcept { return &x; }
    constexpr const float* data() const noexcept { return &x; }
};

// ============================================================================
// Non-member operators
// ============================================================================

inline Quaternion operator*(float scalar, const Quaternion& q) noexcept {
    return q * scalar;
}

} // namespace math
} // namespace phoenix
