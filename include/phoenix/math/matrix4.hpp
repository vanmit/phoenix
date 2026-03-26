#pragma once

#include "vector3.hpp"
#include <array>
#include <cmath>

namespace phoenix {
namespace math {

/**
 * @brief 4x4 Column-major Matrix for 3D transformations
 * 
 * Optimized for graphics operations with SIMD-friendly layout.
 * Column-major order matches OpenGL/Vulkan conventions.
 */
struct alignas(16) Matrix4 {
    std::array<float, 16> data;
    
    // ========================================================================
    // Constructors
    // ========================================================================
    
    constexpr Matrix4() noexcept : data{} {
        // Identity matrix
        data[0] = 1.0f; data[5] = 1.0f; data[10] = 1.0f; data[15] = 1.0f;
    }
    
    explicit constexpr Matrix4(float diagonal) noexcept : data{} {
        data[0] = diagonal; data[5] = diagonal; 
        data[10] = diagonal; data[15] = diagonal;
    }
    
    // ========================================================================
    // Factory Methods
    // ========================================================================
    
    static constexpr Matrix4 identity() noexcept {
        return Matrix4();
    }
    
    static constexpr Matrix4 translation(const Vector3& t) noexcept {
        Matrix4 m;
        m.data[12] = t.x;
        m.data[13] = t.y;
        m.data[14] = t.z;
        return m;
    }
    
    static constexpr Matrix4 translation(float x, float y, float z) noexcept {
        Matrix4 m;
        m.data[12] = x;
        m.data[13] = y;
        m.data[14] = z;
        return m;
    }
    
    static constexpr Matrix4 scale(const Vector3& s) noexcept {
        Matrix4 m;
        m.data[0] = s.x;
        m.data[5] = s.y;
        m.data[10] = s.z;
        return m;
    }
    
    static constexpr Matrix4 scale(float s) noexcept {
        Matrix4 m;
        m.data[0] = s;
        m.data[5] = s;
        m.data[10] = s;
        return m;
    }
    
    static Matrix4 rotationX(float radians) noexcept {
        Matrix4 m;
        const float c = std::cos(radians);
        const float s = std::sin(radians);
        m.data[5] = c;  m.data[9] = -s;
        m.data[6] = s;  m.data[10] = c;
        return m;
    }
    
    static Matrix4 rotationY(float radians) noexcept {
        Matrix4 m;
        const float c = std::cos(radians);
        const float s = std::sin(radians);
        m.data[0] = c;  m.data[8] = s;
        m.data[2] = -s; m.data[10] = c;
        return m;
    }
    
    static Matrix4 rotationZ(float radians) noexcept {
        Matrix4 m;
        const float c = std::cos(radians);
        const float s = std::sin(radians);
        m.data[0] = c;  m.data[4] = -s;
        m.data[1] = s;  m.data[5] = c;
        return m;
    }
    
    static Matrix4 rotation(float radians, const Vector3& axis) noexcept {
        Matrix4 m;
        const float c = std::cos(radians);
        const float s = std::sin(radians);
        const float t = 1.0f - c;
        
        Vector3 n = axis.normalized();
        float x = n.x, y = n.y, z = n.z;
        
        m.data[0] = t * x * x + c;
        m.data[1] = t * x * y + s * z;
        m.data[2] = t * x * z - s * y;
        
        m.data[4] = t * x * y - s * z;
        m.data[5] = t * y * y + c;
        m.data[6] = t * y * z + s * x;
        
        m.data[8] = t * x * z + s * y;
        m.data[9] = t * y * z - s * x;
        m.data[10] = t * z * z + c;
        
        return m;
    }
    
    static Matrix4 perspective(float fovY, float aspect, float near, float far) noexcept {
        Matrix4 m;
        const float tanHalfFov = std::tan(fovY / 2.0f);
        
        m.data[0] = 1.0f / (aspect * tanHalfFov);
        m.data[5] = 1.0f / tanHalfFov;
        m.data[10] = -(far + near) / (far - near);
        m.data[11] = -1.0f;
        m.data[14] = -(2.0f * far * near) / (far - near);
        m.data[15] = 0.0f;
        
        return m;
    }
    
    static Matrix4 orthographic(float left, float right, float bottom, float top, float near, float far) noexcept {
        Matrix4 m;
        m.data[0] = 2.0f / (right - left);
        m.data[5] = 2.0f / (top - bottom);
        m.data[10] = -2.0f / (far - near);
        m.data[12] = -(right + left) / (right - left);
        m.data[13] = -(top + bottom) / (top - bottom);
        m.data[14] = -(far + near) / (far - near);
        return m;
    }
    
    static Matrix4 lookAt(const Vector3& eye, const Vector3& target, const Vector3& up) noexcept {
        const Vector3 f = (target - eye).normalized();
        const Vector3 s = f.cross(up).normalized();
        const Vector3 u = s.cross(f);
        
        Matrix4 m;
        m.data[0] = s.x;  m.data[4] = s.y;  m.data[8] = s.z;
        m.data[1] = u.x;  m.data[5] = u.y;  m.data[9] = u.z;
        m.data[2] = -f.x; m.data[6] = -f.y; m.data[10] = -f.z;
        m.data[12] = -s.dot(eye);
        m.data[13] = -u.dot(eye);
        m.data[14] = f.dot(eye);
        
        return m;
    }
    
    // ========================================================================
    // Element Access
    // ========================================================================
    
    constexpr float& operator()(size_t row, size_t col) noexcept {
        return data[col * 4 + row];
    }
    
    constexpr const float& operator()(size_t row, size_t col) const noexcept {
        return data[col * 4 + row];
    }
    
    constexpr float& operator[](size_t index) noexcept {
        return data[index];
    }
    
    constexpr const float& operator[](size_t index) const noexcept {
        return data[index];
    }
    
    // ========================================================================
    // Matrix Operations
    // ========================================================================
    
    Matrix4 operator+(const Matrix4& other) const noexcept {
        Matrix4 result;
        for (size_t i = 0; i < 16; ++i) {
            result.data[i] = data[i] + other.data[i];
        }
        return result;
    }
    
    Matrix4 operator-(const Matrix4& other) const noexcept {
        Matrix4 result;
        for (size_t i = 0; i < 16; ++i) {
            result.data[i] = data[i] - other.data[i];
        }
        return result;
    }
    
    Matrix4 operator*(const Matrix4& other) const noexcept {
        Matrix4 result;
        for (size_t col = 0; col < 4; ++col) {
            for (size_t row = 0; row < 4; ++row) {
                result(row, col) = 
                    (*this)(row, 0) * other(0, col) +
                    (*this)(row, 1) * other(1, col) +
                    (*this)(row, 2) * other(2, col) +
                    (*this)(row, 3) * other(3, col);
            }
        }
        return result;
    }
    
    Matrix4& operator*=(const Matrix4& other) noexcept {
        *this = *this * other;
        return *this;
    }
    
    Vector3 transformPoint(const Vector3& p) const noexcept {
        const float w = data[3] * p.x + data[7] * p.y + data[11] * p.z + data[15];
        const float invW = 1.0f / w;
        return Vector3(
            (data[0] * p.x + data[4] * p.y + data[8] * p.z + data[12]) * invW,
            (data[1] * p.x + data[5] * p.y + data[9] * p.z + data[13]) * invW,
            (data[2] * p.x + data[6] * p.y + data[10] * p.z + data[14]) * invW
        );
    }
    
    Vector3 transformDirection(const Vector3& d) const noexcept {
        return Vector3(
            data[0] * d.x + data[4] * d.y + data[8] * d.z,
            data[1] * d.x + data[5] * d.y + data[9] * d.z,
            data[2] * d.x + data[6] * d.y + data[10] * d.z
        ).normalized();
    }
    
    Vector3 transformVector(const Vector3& v) const noexcept {
        return Vector3(
            data[0] * v.x + data[4] * v.y + data[8] * v.z,
            data[1] * v.x + data[5] * v.y + data[9] * v.z,
            data[2] * v.x + data[6] * v.y + data[10] * v.z
        );
    }
    
    Matrix4 transposed() const noexcept {
        Matrix4 result;
        for (size_t row = 0; row < 4; ++row) {
            for (size_t col = 0; col < 4; ++col) {
                result(row, col) = (*this)(col, row);
            }
        }
        return result;
    }
    
    void transpose() noexcept {
        *this = transposed();
    }
    
    Matrix4 inverted() const noexcept;
    
    float determinant() const noexcept {
        const float a = data[0], b = data[1], c = data[2], d = data[3];
        const float e = data[4], f = data[5], g = data[6], h = data[7];
        const float i = data[8], j = data[9], k = data[10], l = data[11];
        const float m = data[12], n = data[13], o = data[14], p = data[15];
        
        return a * (f * (k * p - l * o) - g * (j * p - l * n) + h * (j * o - k * n))
             - b * (e * (k * p - l * o) - g * (i * p - l * m) + h * (i * o - k * m))
             + c * (e * (j * p - l * n) - f * (i * p - l * m) + h * (i * n - j * m))
             - d * (e * (j * o - k * n) - f * (i * o - k * m) + g * (i * n - j * m));
    }
    
    // ========================================================================
    // Utility
    // ========================================================================
    
    constexpr bool isIdentity() const noexcept {
        constexpr float epsilon = 1e-6f;
        for (size_t i = 0; i < 16; ++i) {
            const float expected = (i % 5 == 0) ? 1.0f : 0.0f;
            if (std::abs(data[i] - expected) > epsilon) {
                return false;
            }
        }
        return true;
    }
    
    constexpr float* data() noexcept { return data.data(); }
    constexpr const float* data() const noexcept { return data.data(); }
};

// ============================================================================
// Matrix4 Inverse Implementation
// ============================================================================

inline Matrix4 Matrix4::inverted() const noexcept {
    Matrix4 result;
    
    const float a = data[0], b = data[1], c = data[2], d = data[3];
    const float e = data[4], f = data[5], g = data[6], h = data[7];
    const float i = data[8], j = data[9], k = data[10], l = data[11];
    const float m = data[12], n = data[13], o = data[14], p = data[15];
    
    const float det = determinant();
    if (std::abs(det) < 1e-8f) {
        return Matrix4::identity(); // Singular matrix
    }
    
    const float invDet = 1.0f / det;
    
    result.data[0] = invDet * (f * (k * p - l * o) - g * (j * p - l * n) + h * (j * o - k * n));
    result.data[1] = invDet * -(b * (k * p - l * o) - c * (j * p - l * n) + d * (j * o - k * n));
    result.data[2] = invDet * (b * (g * p - h * o) - c * (f * p - h * n) + d * (f * o - g * n));
    result.data[3] = invDet * -(b * (g * k - h * j) - c * (f * k - h * i) + d * (f * j - g * i));
    
    result.data[4] = invDet * -(e * (k * p - l * o) - g * (i * p - l * m) + h * (i * o - k * m));
    result.data[5] = invDet * (a * (k * p - l * o) - c * (i * p - l * m) + d * (i * o - k * m));
    result.data[6] = invDet * -(a * (g * p - h * o) - c * (e * p - h * m) + d * (e * o - g * m));
    result.data[7] = invDet * (a * (g * k - h * j) - c * (e * k - h * i) + d * (e * j - g * i));
    
    result.data[8] = invDet * (e * (j * p - l * n) - f * (i * p - l * m) + h * (i * n - j * m));
    result.data[9] = invDet * -(a * (j * p - l * n) - b * (i * p - l * m) + d * (i * n - j * m));
    result.data[10] = invDet * (a * (f * p - h * n) - b * (e * p - h * m) + d * (e * n - f * m));
    result.data[11] = invDet * -(a * (f * k - h * j) - b * (e * k - h * i) + d * (e * j - f * i));
    
    result.data[12] = invDet * -(e * (j * o - k * n) - f * (i * o - k * m) + g * (i * n - j * m));
    result.data[13] = invDet * (a * (j * o - k * n) - b * (i * o - k * m) + c * (i * n - j * m));
    result.data[14] = invDet * -(a * (f * o - g * n) - b * (e * o - g * m) + c * (e * n - f * m));
    result.data[15] = invDet * (a * (f * k - g * j) - b * (e * k - g * i) + c * (e * j - f * i));
    
    return result;
}

} // namespace math
} // namespace phoenix
