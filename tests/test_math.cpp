/**
 * @file test_math.cpp
 * @brief Phoenix Engine Math Library Tests
 * 
 * Comprehensive test suite for Phoenix Engine mathematical operations,
 * including vector, matrix, and utility functions.
 */

#include <gtest/gtest.h>
#include <cmath>
#include <limits>
#include <array>

// ============================================================================
// Math Utility Functions (Example implementations for testing)
// ============================================================================

namespace phoenix {
namespace math {

/**
 * @brief Clamp a value between min and max
 */
template<typename T>
T clamp(T value, T min_val, T max_val) {
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

/**
 * @brief Linear interpolation between two values
 */
template<typename T>
T lerp(T a, T b, T t) {
    return a + t * (b - a);
}

/**
 * @brief Check if two floating point values are approximately equal
 */
bool approxEqual(float a, float b, float epsilon = 1e-6f) {
    return std::abs(a - b) < epsilon;
}

bool approxEqual(double a, double b, double epsilon = 1e-9) {
    return std::abs(a - b) < epsilon;
}

/**
 * @brief 3D Vector structure
 */
struct Vector3 {
    float x, y, z;
    
    Vector3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
    
    Vector3 operator+(const Vector3& other) const {
        return Vector3(x + other.x, y + other.y, z + other.z);
    }
    
    Vector3 operator-(const Vector3& other) const {
        return Vector3(x - other.x, y - other.y, z - other.z);
    }
    
    Vector3 operator*(float scalar) const {
        return Vector3(x * scalar, y * scalar, z * scalar);
    }
    
    float dot(const Vector3& other) const {
        return x * other.x + y * other.y + z * other.z;
    }
    
    Vector3 cross(const Vector3& other) const {
        return Vector3(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        );
    }
    
    float length() const {
        return std::sqrt(x * x + y * y + z * z);
    }
    
    Vector3 normalize() const {
        float len = length();
        if (len > 0) {
            return Vector3(x / len, y / len, z / len);
        }
        return *this;
    }
};

/**
 * @brief 4x4 Matrix structure (column-major)
 */
struct Matrix4 {
    float data[16];
    
    Matrix4() {
        // Initialize as identity matrix
        for (int i = 0; i < 16; ++i) {
            data[i] = (i % 5 == 0) ? 1.0f : 0.0f;
        }
    }
    
    static Matrix4 identity() {
        return Matrix4();
    }
    
    static Matrix4 translation(float x, float y, float z) {
        Matrix4 m;
        m.data[12] = x;
        m.data[13] = y;
        m.data[14] = z;
        return m;
    }
    
    static Matrix4 scale(float x, float y, float z) {
        Matrix4 m;
        m.data[0] = x;
        m.data[5] = y;
        m.data[10] = z;
        return m;
    }
};

} // namespace math
} // namespace phoenix

// ============================================================================
// Test Cases
// ============================================================================

/**
 * @test Test clamp function
 */
TEST(MathUtilsTest, Clamp) {
    using namespace phoenix::math;
    
    // Test within range
    EXPECT_EQ(clamp(5, 0, 10), 5);
    
    // Test below range
    EXPECT_EQ(clamp(-5, 0, 10), 0);
    
    // Test above range
    EXPECT_EQ(clamp(15, 0, 10), 10);
    
    // Test boundary values
    EXPECT_EQ(clamp(0, 0, 10), 0);
    EXPECT_EQ(clamp(10, 0, 10), 10);
    
    // Test float values
    EXPECT_FLOAT_EQ(clamp(5.5f, 0.0f, 10.0f), 5.5f);
    EXPECT_FLOAT_EQ(clamp(-1.5f, 0.0f, 10.0f), 0.0f);
}

/**
 * @test Test linear interpolation
 */
TEST(MathUtilsTest, Lerp) {
    using namespace phoenix::math;
    
    // Test endpoints
    EXPECT_FLOAT_EQ(lerp(0.0f, 10.0f, 0.0f), 0.0f);
    EXPECT_FLOAT_EQ(lerp(0.0f, 10.0f, 1.0f), 10.0f);
    
    // Test midpoint
    EXPECT_FLOAT_EQ(lerp(0.0f, 10.0f, 0.5f), 5.0f);
    
    // Test quarter point
    EXPECT_FLOAT_EQ(lerp(0.0f, 100.0f, 0.25f), 25.0f);
    
    // Test negative values
    EXPECT_FLOAT_EQ(lerp(-10.0f, 10.0f, 0.5f), 0.0f);
}

/**
 * @test Test floating point approximation
 */
TEST(MathUtilsTest, ApproxEqual) {
    using namespace phoenix::math;
    
    // Test exact equality
    EXPECT_TRUE(approxEqual(1.0f, 1.0f));
    
    // Test within epsilon
    EXPECT_TRUE(approxEqual(1.0f, 1.0f + 1e-7f));
    
    // Test outside epsilon
    EXPECT_FALSE(approxEqual(1.0f, 1.1f));
    
    // Test zero
    EXPECT_TRUE(approxEqual(0.0f, 1e-7f));
    EXPECT_FALSE(approxEqual(0.0f, 1e-5f));
}

/**
 * @test Test Vector3 addition
 */
TEST(Vector3Test, Addition) {
    using namespace phoenix::math;
    
    Vector3 a(1.0f, 2.0f, 3.0f);
    Vector3 b(4.0f, 5.0f, 6.0f);
    Vector3 c = a + b;
    
    EXPECT_FLOAT_EQ(c.x, 5.0f);
    EXPECT_FLOAT_EQ(c.y, 7.0f);
    EXPECT_FLOAT_EQ(c.z, 9.0f);
}

/**
 * @test Test Vector3 subtraction
 */
TEST(Vector3Test, Subtraction) {
    using namespace phoenix::math;
    
    Vector3 a(10.0f, 20.0f, 30.0f);
    Vector3 b(1.0f, 2.0f, 3.0f);
    Vector3 c = a - b;
    
    EXPECT_FLOAT_EQ(c.x, 9.0f);
    EXPECT_FLOAT_EQ(c.y, 18.0f);
    EXPECT_FLOAT_EQ(c.z, 27.0f);
}

/**
 * @test Test Vector3 scalar multiplication
 */
TEST(Vector3Test, ScalarMultiplication) {
    using namespace phoenix::math;
    
    Vector3 v(1.0f, 2.0f, 3.0f);
    Vector3 scaled = v * 2.0f;
    
    EXPECT_FLOAT_EQ(scaled.x, 2.0f);
    EXPECT_FLOAT_EQ(scaled.y, 4.0f);
    EXPECT_FLOAT_EQ(scaled.z, 6.0f);
}

/**
 * @test Test Vector3 dot product
 */
TEST(Vector3Test, DotProduct) {
    using namespace phoenix::math;
    
    Vector3 a(1.0f, 0.0f, 0.0f);
    Vector3 b(0.0f, 1.0f, 0.0f);
    Vector3 c(1.0f, 1.0f, 0.0f);
    
    // Orthogonal vectors
    EXPECT_FLOAT_EQ(a.dot(b), 0.0f);
    
    // Same vector
    EXPECT_FLOAT_EQ(a.dot(a), 1.0f);
    
    // 45 degree angle
    EXPECT_FLOAT_EQ(a.dot(c), 1.0f);
}

/**
 * @test Test Vector3 cross product
 */
TEST(Vector3Test, CrossProduct) {
    using namespace phoenix::math;
    
    Vector3 i(1.0f, 0.0f, 0.0f);
    Vector3 j(0.0f, 1.0f, 0.0f);
    Vector3 k(0.0f, 0.0f, 1.0f);
    
    // i × j = k
    Vector3 result = i.cross(j);
    EXPECT_FLOAT_EQ(result.x, k.x);
    EXPECT_FLOAT_EQ(result.y, k.y);
    EXPECT_FLOAT_EQ(result.z, k.z);
    
    // j × i = -k
    result = j.cross(i);
    EXPECT_FLOAT_EQ(result.x, -k.x);
    EXPECT_FLOAT_EQ(result.y, -k.y);
    EXPECT_FLOAT_EQ(result.z, -k.z);
}

/**
 * @test Test Vector3 length
 */
TEST(Vector3Test, Length) {
    using namespace phoenix::math;
    
    Vector3 v(3.0f, 4.0f, 0.0f);
    EXPECT_FLOAT_EQ(v.length(), 5.0f);
    
    Vector3 unit(1.0f, 0.0f, 0.0f);
    EXPECT_FLOAT_EQ(unit.length(), 1.0f);
    
    Vector3 zero(0.0f, 0.0f, 0.0f);
    EXPECT_FLOAT_EQ(zero.length(), 0.0f);
}

/**
 * @test Test Vector3 normalization
 */
TEST(Vector3Test, Normalize) {
    using namespace phoenix::math;
    
    Vector3 v(3.0f, 4.0f, 0.0f);
    Vector3 normalized = v.normalize();
    
    EXPECT_FLOAT_EQ(normalized.length(), 1.0f);
    EXPECT_FLOAT_EQ(normalized.x, 0.6f);
    EXPECT_FLOAT_EQ(normalized.y, 0.8f);
    EXPECT_FLOAT_EQ(normalized.z, 0.0f);
    
    // Zero vector should remain unchanged
    Vector3 zero(0.0f, 0.0f, 0.0f);
    Vector3 zero_normalized = zero.normalize();
    EXPECT_FLOAT_EQ(zero_normalized.x, 0.0f);
    EXPECT_FLOAT_EQ(zero_normalized.y, 0.0f);
    EXPECT_FLOAT_EQ(zero_normalized.z, 0.0f);
}

/**
 * @test Test Matrix4 identity
 */
TEST(Matrix4Test, Identity) {
    using namespace phoenix::math;
    
    Matrix4 m = Matrix4::identity();
    
    // Check diagonal elements
    EXPECT_FLOAT_EQ(m.data[0], 1.0f);
    EXPECT_FLOAT_EQ(m.data[5], 1.0f);
    EXPECT_FLOAT_EQ(m.data[10], 1.0f);
    EXPECT_FLOAT_EQ(m.data[15], 1.0f);
    
    // Check off-diagonal elements
    for (int i = 0; i < 16; ++i) {
        if (i % 5 != 0) {
            EXPECT_FLOAT_EQ(m.data[i], 0.0f);
        }
    }
}

/**
 * @test Test Matrix4 translation
 */
TEST(Matrix4Test, Translation) {
    using namespace phoenix::math;
    
    Matrix4 m = Matrix4::translation(5.0f, 10.0f, 15.0f);
    
    EXPECT_FLOAT_EQ(m.data[12], 5.0f);
    EXPECT_FLOAT_EQ(m.data[13], 10.0f);
    EXPECT_FLOAT_EQ(m.data[14], 15.0f);
    
    // Identity part should be preserved
    EXPECT_FLOAT_EQ(m.data[0], 1.0f);
    EXPECT_FLOAT_EQ(m.data[5], 1.0f);
    EXPECT_FLOAT_EQ(m.data[10], 1.0f);
}

/**
 * @test Test Matrix4 scale
 */
TEST(Matrix4Test, Scale) {
    using namespace phoenix::math;
    
    Matrix4 m = Matrix4::scale(2.0f, 3.0f, 4.0f);
    
    EXPECT_FLOAT_EQ(m.data[0], 2.0f);
    EXPECT_FLOAT_EQ(m.data[5], 3.0f);
    EXPECT_FLOAT_EQ(m.data[10], 4.0f);
    
    // Other diagonal should be 1
    EXPECT_FLOAT_EQ(m.data[15], 1.0f);
}

// ============================================================================
// Performance Tests (Optional)
// ============================================================================

/**
 * @test Performance test for Vector3 operations
 */
TEST(Vector3PerformanceTest, Operations) {
    using namespace phoenix::math;
    
    const int iterations = 1000000;
    Vector3 a(1.0f, 2.0f, 3.0f);
    Vector3 b(4.0f, 5.0f, 6.0f);
    Vector3 result(0, 0, 0);
    
    // Addition performance
    for (int i = 0; i < iterations; ++i) {
        result = a + b;
    }
    
    // Dot product performance
    float dot = 0;
    for (int i = 0; i < iterations; ++i) {
        dot = a.dot(b);
    }
    
    // Ensure results are used (prevent optimization)
    EXPECT_GT(result.length(), 0);
    EXPECT_GT(dot, 0);
}
