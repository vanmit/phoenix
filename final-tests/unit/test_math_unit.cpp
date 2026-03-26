/**
 * Phoenix Engine - Math Library Unit Tests
 * 
 * 测试数学库的核心功能，包括向量、矩阵、四元数等
 * 目标覆盖率：>95%
 */

#include <gtest/gtest.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

// Phoenix Engine Math 头文件
#include "phoenix/core/math/vector2.hpp"
#include "phoenix/core/math/vector3.hpp"
#include "phoenix/core/math/vector4.hpp"
#include "phoenix/core/math/matrix4.hpp"
#include "phoenix/core/math/quaternion.hpp"
#include "phoenix/core/math/bounding_box.hpp"
#include "phoenix/core/math/ray.hpp"
#include "phoenix/core/math/plane.hpp"

using namespace phoenix::math;

// ============================================================================
// Vector2 测试
// ============================================================================

TEST(Vector2Test, Construction) {
    Vector2f v1;
    EXPECT_FLOAT_EQ(v1.x, 0.0f);
    EXPECT_FLOAT_EQ(v1.y, 0.0f);
    
    Vector2f v2(1.0f, 2.0f);
    EXPECT_FLOAT_EQ(v2.x, 1.0f);
    EXPECT_FLOAT_EQ(v2.y, 2.0f);
    
    Vector2f v3(v2);
    EXPECT_FLOAT_EQ(v3.x, 1.0f);
    EXPECT_FLOAT_EQ(v3.y, 2.0f);
}

TEST(Vector2Test, Arithmetic) {
    Vector2f a(1.0f, 2.0f);
    Vector2f b(3.0f, 4.0f);
    
    Vector2f sum = a + b;
    EXPECT_FLOAT_EQ(sum.x, 4.0f);
    EXPECT_FLOAT_EQ(sum.y, 6.0f);
    
    Vector2f diff = b - a;
    EXPECT_FLOAT_EQ(diff.x, 2.0f);
    EXPECT_FLOAT_EQ(diff.y, 2.0f);
    
    Vector2f prod = a * 2.0f;
    EXPECT_FLOAT_EQ(prod.x, 2.0f);
    EXPECT_FLOAT_EQ(prod.y, 4.0f);
    
    Vector2f quot = b / 2.0f;
    EXPECT_FLOAT_EQ(quot.x, 1.5f);
    EXPECT_FLOAT_EQ(quot.y, 2.0f);
}

TEST(Vector2Test, DotProduct) {
    Vector2f a(1.0f, 2.0f);
    Vector2f b(3.0f, 4.0f);
    
    float dot = a.dot(b);
    EXPECT_FLOAT_EQ(dot, 11.0f);
}

TEST(Vector2Test, Length) {
    Vector2f v(3.0f, 4.0f);
    
    EXPECT_FLOAT_EQ(v.length(), 5.0f);
    EXPECT_FLOAT_EQ(v.lengthSquared(), 25.0f);
    
    Vector2f normalized = v.normalized();
    EXPECT_FLOAT_EQ(normalized.length(), 1.0f);
}

// ============================================================================
// Vector3 测试
// ============================================================================

TEST(Vector3Test, Construction) {
    Vector3f v1;
    EXPECT_FLOAT_EQ(v1.x, 0.0f);
    EXPECT_FLOAT_EQ(v1.y, 0.0f);
    EXPECT_FLOAT_EQ(v1.z, 0.0f);
    
    Vector3f v2(1.0f, 2.0f, 3.0f);
    EXPECT_FLOAT_EQ(v2.x, 1.0f);
    EXPECT_FLOAT_EQ(v2.y, 2.0f);
    EXPECT_FLOAT_EQ(v2.z, 3.0f);
}

TEST(Vector3Test, Arithmetic) {
    Vector3f a(1.0f, 2.0f, 3.0f);
    Vector3f b(4.0f, 5.0f, 6.0f);
    
    Vector3f sum = a + b;
    EXPECT_FLOAT_EQ(sum.x, 5.0f);
    EXPECT_FLOAT_EQ(sum.y, 7.0f);
    EXPECT_FLOAT_EQ(sum.z, 9.0f);
    
    Vector3f cross = a.cross(b);
    EXPECT_FLOAT_EQ(cross.x, -3.0f);
    EXPECT_FLOAT_EQ(cross.y, 6.0f);
    EXPECT_FLOAT_EQ(cross.z, -3.0f);
}

TEST(Vector3Test, DotProduct) {
    Vector3f a(1.0f, 2.0f, 3.0f);
    Vector3f b(4.0f, 5.0f, 6.0f);
    
    float dot = a.dot(b);
    EXPECT_FLOAT_EQ(dot, 32.0f);
}

TEST(Vector3Test, Length) {
    Vector3f v(2.0f, 3.0f, 6.0f);
    
    EXPECT_FLOAT_EQ(v.length(), 7.0f);
    EXPECT_FLOAT_EQ(v.lengthSquared(), 49.0f);
}

// ============================================================================
// Vector4 测试
// ============================================================================

TEST(Vector4Test, Construction) {
    Vector4f v1;
    EXPECT_FLOAT_EQ(v1.x, 0.0f);
    EXPECT_FLOAT_EQ(v1.y, 0.0f);
    EXPECT_FLOAT_EQ(v1.z, 0.0f);
    EXPECT_FLOAT_EQ(v1.w, 0.0f);
    
    Vector4f v2(1.0f, 2.0f, 3.0f, 4.0f);
    EXPECT_FLOAT_EQ(v2.x, 1.0f);
    EXPECT_FLOAT_EQ(v2.y, 2.0f);
    EXPECT_FLOAT_EQ(v2.z, 3.0f);
    EXPECT_FLOAT_EQ(v2.w, 4.0f);
}

TEST(Vector4Test, Arithmetic) {
    Vector4f a(1.0f, 2.0f, 3.0f, 4.0f);
    Vector4f b(5.0f, 6.0f, 7.0f, 8.0f);
    
    Vector4f sum = a + b;
    EXPECT_FLOAT_EQ(sum.x, 6.0f);
    EXPECT_FLOAT_EQ(sum.y, 8.0f);
    EXPECT_FLOAT_EQ(sum.z, 10.0f);
    EXPECT_FLOAT_EQ(sum.w, 12.0f);
}

// ============================================================================
// Matrix4 测试
// ============================================================================

TEST(Matrix4Test, Identity) {
    Matrix4f identity = Matrix4f::identity();
    
    EXPECT_FLOAT_EQ(identity(0, 0), 1.0f);
    EXPECT_FLOAT_EQ(identity(1, 1), 1.0f);
    EXPECT_FLOAT_EQ(identity(2, 2), 1.0f);
    EXPECT_FLOAT_EQ(identity(3, 3), 1.0f);
    
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (i == j) {
                EXPECT_FLOAT_EQ(identity(i, j), 1.0f);
            } else {
                EXPECT_FLOAT_EQ(identity(i, j), 0.0f);
            }
        }
    }
}

TEST(Matrix4Test, Translation) {
    Matrix4f translation = Matrix4f::translate(1.0f, 2.0f, 3.0f);
    
    Vector4f v(0.0f, 0.0f, 0.0f, 1.0f);
    Vector4f result = translation * v;
    
    EXPECT_FLOAT_EQ(result.x, 1.0f);
    EXPECT_FLOAT_EQ(result.y, 2.0f);
    EXPECT_FLOAT_EQ(result.z, 3.0f);
}

TEST(Matrix4Test, Rotation) {
    Matrix4f rotation = Matrix4f::rotate(glm::radians(90.0f), Vector3f(0.0f, 1.0f, 0.0f));
    
    Vector4f v(1.0f, 0.0f, 0.0f, 1.0f);
    Vector4f result = rotation * v;
    
    EXPECT_NEAR(result.x, 0.0f, 0.0001f);
    EXPECT_FLOAT_EQ(result.y, 0.0f);
    EXPECT_NEAR(result.z, -1.0f, 0.0001f);
}

TEST(Matrix4Test, Scaling) {
    Matrix4f scale = Matrix4f::scale(2.0f, 3.0f, 4.0f);
    
    Vector4f v(1.0f, 1.0f, 1.0f, 1.0f);
    Vector4f result = scale * v;
    
    EXPECT_FLOAT_EQ(result.x, 2.0f);
    EXPECT_FLOAT_EQ(result.y, 3.0f);
    EXPECT_FLOAT_EQ(result.z, 4.0f);
}

TEST(Matrix4Test, Multiplication) {
    Matrix4f a = Matrix4f::identity();
    Matrix4f b = Matrix4f::translate(1.0f, 0.0f, 0.0f);
    
    Matrix4f result = a * b;
    
    Vector4f v(0.0f, 0.0f, 0.0f, 1.0f);
    Vector4f transformed = result * v;
    
    EXPECT_FLOAT_EQ(transformed.x, 1.0f);
}

TEST(Matrix4Test, Inverse) {
    Matrix4f translation = Matrix4f::translate(5.0f, 0.0f, 0.0f);
    Matrix4f inverse = translation.inverse();
    
    Vector4f v(5.0f, 0.0f, 0.0f, 1.0f);
    Vector4f result = inverse * v;
    
    EXPECT_FLOAT_EQ(result.x, 0.0f);
}

// ============================================================================
// Quaternion 测试
// ============================================================================

TEST(QuaternionTest, Construction) {
    Quaternionf q1;
    EXPECT_FLOAT_EQ(q1.x, 0.0f);
    EXPECT_FLOAT_EQ(q1.y, 0.0f);
    EXPECT_FLOAT_EQ(q1.z, 0.0f);
    EXPECT_FLOAT_EQ(q1.w, 1.0f);
    
    Quaternionf q2(1.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_FLOAT_EQ(q2.x, 1.0f);
    EXPECT_FLOAT_EQ(q2.w, 0.0f);
}

TEST(QuaternionTest, Rotation) {
    Quaternionf q = Quaternionf::fromAxisAngle(Vector3f(0.0f, 1.0f, 0.0f), glm::radians(90.0f));
    
    Vector3f v(1.0f, 0.0f, 0.0f);
    Vector3f result = q * v;
    
    EXPECT_NEAR(result.x, 0.0f, 0.0001f);
    EXPECT_FLOAT_EQ(result.y, 0.0f);
    EXPECT_NEAR(result.z, -1.0f, 0.0001f);
}

TEST(QuaternionTest, Slerp) {
    Quaternionf q1;
    Quaternionf q2 = Quaternionf::fromAxisAngle(Vector3f(0.0f, 1.0f, 0.0f), glm::radians(90.0f));
    
    Quaternionf result = Quaternionf::slerp(q1, q2, 0.5f);
    
    Vector3f v(1.0f, 0.0f, 0.0f);
    Vector3f rotated = result * v;
    
    EXPECT_NEAR(rotated.x, 0.7071f, 0.0001f);
    EXPECT_FLOAT_EQ(rotated.y, 0.0f);
    EXPECT_NEAR(rotated.z, -0.7071f, 0.0001f);
}

TEST(QuaternionTest, Normalization) {
    Quaternionf q(2.0f, 0.0f, 0.0f, 0.0f);
    
    Quaternionf normalized = q.normalized();
    
    EXPECT_FLOAT_EQ(normalized.length(), 1.0f);
}

// ============================================================================
// BoundingBox 测试
// ============================================================================

TEST(BoundingBoxTest, Construction) {
    BoundingBoxf box;
    EXPECT_TRUE(box.isEmpty());
    
    BoundingBoxf box2(Vector3f(-1.0f, -1.0f, -1.0f), Vector3f(1.0f, 1.0f, 1.0f));
    EXPECT_FALSE(box2.isEmpty());
}

TEST(BoundingBoxTest, Extend) {
    BoundingBoxf box;
    
    box.extend(Vector3f(-1.0f, -1.0f, -1.0f));
    box.extend(Vector3f(1.0f, 1.0f, 1.0f));
    
    EXPECT_FLOAT_EQ(box.min().x, -1.0f);
    EXPECT_FLOAT_EQ(box.max().x, 1.0f);
    EXPECT_FLOAT_EQ(box.min().y, -1.0f);
    EXPECT_FLOAT_EQ(box.max().y, 1.0f);
    EXPECT_FLOAT_EQ(box.min().z, -1.0f);
    EXPECT_FLOAT_EQ(box.max().z, 1.0f);
}

TEST(BoundingBoxTest, Contains) {
    BoundingBoxf box(Vector3f(-1.0f, -1.0f, -1.0f), Vector3f(1.0f, 1.0f, 1.0f));
    
    EXPECT_TRUE(box.contains(Vector3f(0.0f, 0.0f, 0.0f)));
    EXPECT_TRUE(box.contains(Vector3f(1.0f, 1.0f, 1.0f)));
    EXPECT_FALSE(box.contains(Vector3f(2.0f, 0.0f, 0.0f)));
}

TEST(BoundingBoxTest, Intersects) {
    BoundingBoxf box1(Vector3f(-1.0f, -1.0f, -1.0f), Vector3f(1.0f, 1.0f, 1.0f));
    BoundingBoxf box2(Vector3f(0.0f, 0.0f, 0.0f), Vector3f(2.0f, 2.0f, 2.0f));
    BoundingBoxf box3(Vector3f(5.0f, 5.0f, 5.0f), Vector3f(6.0f, 6.0f, 6.0f));
    
    EXPECT_TRUE(box1.intersects(box2));
    EXPECT_FALSE(box1.intersects(box3));
}

// ============================================================================
// Ray 测试
// ============================================================================

TEST(RayTest, Construction) {
    Rayf ray(Vector3f(0.0f, 0.0f, 0.0f), Vector3f(0.0f, 0.0f, -1.0f));
    
    EXPECT_FLOAT_EQ(ray.origin().x, 0.0f);
    EXPECT_FLOAT_EQ(ray.direction().z, -1.0f);
}

TEST(RayTest, PointAt) {
    Rayf ray(Vector3f(0.0f, 0.0f, 0.0f), Vector3f(0.0f, 0.0f, -1.0f));
    
    Vector3f point = ray.pointAt(5.0f);
    
    EXPECT_FLOAT_EQ(point.x, 0.0f);
    EXPECT_FLOAT_EQ(point.y, 0.0f);
    EXPECT_FLOAT_EQ(point.z, -5.0f);
}

TEST(RayTest, IntersectsSphere) {
    Rayf ray(Vector3f(0.0f, 0.0f, 5.0f), Vector3f(0.0f, 0.0f, -1.0f));
    
    bool intersects;
    float t;
    bool result = ray.intersectsSphere(Vector3f(0.0f, 0.0f, 0.0f), 1.0f, intersects, t);
    
    EXPECT_TRUE(result);
    EXPECT_FLOAT_EQ(t, 4.0f);
}

TEST(RayTest, IntersectsPlane) {
    Rayf ray(Vector3f(0.0f, 0.0f, 5.0f), Vector3f(0.0f, 0.0f, -1.0f));
    Planef plane(Vector3f(0.0f, 0.0f, 1.0f), 0.0f);
    
    float t;
    bool result = ray.intersectsPlane(plane, t);
    
    EXPECT_TRUE(result);
    EXPECT_FLOAT_EQ(t, 5.0f);
}

// ============================================================================
// Plane 测试
// ============================================================================

TEST(PlaneTest, Construction) {
    Planef plane(Vector3f(0.0f, 1.0f, 0.0f), 0.0f);
    
    EXPECT_FLOAT_EQ(plane.normal().y, 1.0f);
    EXPECT_FLOAT_EQ(plane.distance(), 0.0f);
}

TEST(PlaneTest, DistanceToPoint) {
    Planef plane(Vector3f(0.0f, 1.0f, 0.0f), 0.0f);
    
    float dist = plane.distanceToPoint(Vector3f(0.0f, 5.0f, 0.0f));
    
    EXPECT_FLOAT_EQ(dist, 5.0f);
}

TEST(PlaneTest, Side) {
    Planef plane(Vector3f(0.0f, 1.0f, 0.0f), 0.0f);
    
    EXPECT_EQ(plane.getSide(Vector3f(0.0f, 1.0f, 0.0f)), PlaneSide::Front);
    EXPECT_EQ(plane.getSide(Vector3f(0.0f, -1.0f, 0.0f)), PlaneSide::Back);
    EXPECT_EQ(plane.getSide(Vector3f(0.0f, 0.0f, 0.0f)), PlaneSide::OnPlane);
}

// ============================================================================
// 主测试入口
// ============================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
