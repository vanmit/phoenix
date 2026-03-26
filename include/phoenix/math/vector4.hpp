#pragma once

#include <cmath>
#include <algorithm>

namespace phoenix {
namespace math {

/**
 * @brief 4 维向量
 */
struct alignas(16) Vector4 {
    float x, y, z, w;
    
    constexpr Vector4() noexcept : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
    constexpr Vector4(float x, float y, float z, float w) noexcept 
        : x(x), y(y), z(z), w(w) {}
    
    constexpr Vector4(float v) noexcept : x(v), y(v), z(v), w(v) {}
    
    // 单位向量
    static constexpr Vector4 unitX() noexcept { return Vector4(1.0f, 0.0f, 0.0f, 0.0f); }
    static constexpr Vector4 unitY() noexcept { return Vector4(0.0f, 1.0f, 0.0f, 0.0f); }
    static constexpr Vector4 unitZ() noexcept { return Vector4(0.0f, 0.0f, 1.0f, 0.0f); }
    static constexpr Vector4 unitW() noexcept { return Vector4(0.0f, 0.0f, 0.0f, 1.0f); }
    static constexpr Vector4 one() noexcept { return Vector4(1.0f, 1.0f, 1.0f, 1.0f); }
    static constexpr Vector4 zero() noexcept { return Vector4(0.0f, 0.0f, 0.0f, 0.0f); }
    
    // 运算符
    constexpr Vector4 operator+(const Vector4& other) const noexcept {
        return Vector4(x + other.x, y + other.y, z + other.z, w + other.w);
    }
    
    constexpr Vector4 operator-(const Vector4& other) const noexcept {
        return Vector4(x - other.x, y - other.y, z - other.z, w - other.w);
    }
    
    constexpr Vector4 operator*(float scalar) const noexcept {
        return Vector4(x * scalar, y * scalar, z * scalar, w * scalar);
    }
    
    constexpr Vector4 operator/(float scalar) const noexcept {
        float inv = 1.0f / scalar;
        return Vector4(x * inv, y * inv, z * inv, w * inv);
    }
    
    constexpr Vector4 operator-() const noexcept {
        return Vector4(-x, -y, -z, -w);
    }
    
    constexpr Vector4& operator+=(const Vector4& other) noexcept {
        x += other.x; y += other.y; z += other.z; w += other.w;
        return *this;
    }
    
    constexpr Vector4& operator-=(const Vector4& other) noexcept {
        x -= other.x; y -= other.y; z -= other.z; w -= other.w;
        return *this;
    }
    
    constexpr Vector4& operator*=(float scalar) noexcept {
        x *= scalar; y *= scalar; z *= scalar; w *= scalar;
        return *this;
    }
    
    constexpr Vector4& operator/=(float scalar) noexcept {
        float inv = 1.0f / scalar;
        x *= inv; y *= inv; z *= inv; w *= inv;
        return *this;
    }
    
    constexpr bool operator==(const Vector4& other) const noexcept {
        return x == other.x && y == other.y && z == other.z && w == other.w;
    }
    
    constexpr bool operator!=(const Vector4& other) const noexcept {
        return !(*this == other);
    }
    
    // 点积
    constexpr float dot(const Vector4& other) const noexcept {
        return x * other.x + y * other.y + z * other.z + w * other.w;
    }
    
    // 长度
    float length() const noexcept {
        return std::sqrt(lengthSquared());
    }
    
    constexpr float lengthSquared() const noexcept {
        return x * x + y * y + z * z + w * w;
    }
    
    // 归一化
    Vector4 normalized() const noexcept {
        float len = length();
        if (len > 1e-8f) {
            float invLen = 1.0f / len;
            return Vector4(x * invLen, y * invLen, z * invLen, w * invLen);
        }
        return *this;
    }
    
    void normalize() noexcept {
        *this = normalized();
    }
    
    // 线性插值
    static Vector4 lerp(const Vector4& a, const Vector4& b, float t) noexcept {
        return Vector4(
            a.x + (b.x - a.x) * t,
            a.y + (b.y - a.y) * t,
            a.z + (b.z - a.z) * t,
            a.w + (b.w - a.w) * t
        );
    }
    
    // 工具方法
    constexpr bool isZero() const noexcept {
        constexpr float epsilon = 1e-6f;
        return std::abs(x) < epsilon && std::abs(y) < epsilon && 
               std::abs(z) < epsilon && std::abs(w) < epsilon;
    }
    
    constexpr float* data() noexcept { return &x; }
    constexpr const float* data() const noexcept { return &x; }
};

inline Vector4 operator*(float scalar, const Vector4& v) noexcept {
    return v * scalar;
}

} // namespace math
} // namespace phoenix
