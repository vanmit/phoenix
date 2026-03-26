#pragma once

#include "vector3.hpp"
#include "vector4.hpp"

namespace phoenix {
namespace math {

/**
 * @brief RGBA 颜色表示
 */
struct Color {
    float r, g, b, a;
    
    constexpr Color() noexcept : r(1.0f), g(1.0f), b(1.0f), a(1.0f) {}
    constexpr Color(float r, float g, float b, float a = 1.0f) noexcept 
        : r(r), g(g), b(b), a(a) {}
    
    constexpr Color(const Vector3& rgb, float a = 1.0f) noexcept
        : r(rgb.x), g(rgb.y), b(rgb.z), a(a) {}
    
    constexpr Color(const Vector4& rgba) noexcept
        : r(rgba.x), g(rgba.y), b(rgba.z), a(rgba.w) {}
    
    // 预设颜色
    static constexpr Color red() { return Color(1.0f, 0.0f, 0.0f, 1.0f); }
    static constexpr Color green() { return Color(0.0f, 1.0f, 0.0f, 1.0f); }
    static constexpr Color blue() { return Color(0.0f, 0.0f, 1.0f, 1.0f); }
    static constexpr Color white() { return Color(1.0f, 1.0f, 1.0f, 1.0f); }
    static constexpr Color black() { return Color(0.0f, 0.0f, 0.0f, 1.0f); }
    static constexpr Color transparent() { return Color(0.0f, 0.0f, 0.0f, 0.0f); }
    
    // 转换为 Vector4
    constexpr Vector4 toVector4() const noexcept {
        return Vector4(r, g, b, a);
    }
    
    // 转换为 Vector3 (忽略 alpha)
    constexpr Vector3 toVector3() const noexcept {
        return Vector3(r, g, b);
    }
    
    // 运算符
    constexpr Color operator+(const Color& other) const noexcept {
        return Color(r + other.r, g + other.g, b + other.b, a + other.a);
    }
    
    constexpr Color operator-(const Color& other) const noexcept {
        return Color(r - other.r, g - other.g, b - other.b, a - other.a);
    }
    
    constexpr Color operator*(float scalar) const noexcept {
        return Color(r * scalar, g * scalar, b * scalar, a * scalar);
    }
    
    constexpr Color operator*(const Color& other) const noexcept {
        return Color(r * other.r, g * other.g, b * other.b, a * other.a);
    }
    
    constexpr Color& operator+=(const Color& other) noexcept {
        r += other.r; g += other.g; b += other.b; a += other.a;
        return *this;
    }
    
    constexpr Color& operator-=(const Color& other) noexcept {
        r -= other.r; g -= other.g; b -= other.b; a -= other.a;
        return *this;
    }
    
    constexpr Color& operator*=(float scalar) noexcept {
        r *= scalar; g *= scalar; b *= scalar; a *= scalar;
        return *this;
    }
    
    constexpr bool operator==(const Color& other) const noexcept {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }
    
    constexpr bool operator!=(const Color& other) const noexcept {
        return !(*this == other);
    }
};

inline Color operator*(float scalar, const Color& color) noexcept {
    return color * scalar;
}

} // namespace math
} // namespace phoenix
