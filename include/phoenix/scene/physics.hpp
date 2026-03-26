#pragma once

#include "../math/vector3.hpp"
#include "../math/quaternion.hpp"
#include "../math/matrix4.hpp"
#include <memory>
#include <vector>
#include <functional>
#include <cstdint>

namespace phoenix {
namespace scene {

// 前向声明
struct CollisionShape;
struct PhysicsMaterial;
class PhysicsWorld;

// ============================================================================
// 物理基础类型
// ============================================================================

/**
 * @brief 刚体类型
 */
enum class RigidBodyType : uint8_t {
    Static,      ///< 静态物体（无限质量，不移动）
    Dynamic,     ///< 动态物体（受力和碰撞影响）
    Kinematic    ///< 运动学物体（手动控制，影响其他物体）
};

/**
 * @brief 碰撞形状类型
 */
enum class CollisionShapeType : uint8_t {
    Sphere,
    Box,
    Capsule,
    Cylinder,
    ConvexHull,
    Mesh,
    Heightfield,
    Compound
};

/**
 * @brief 射线命中结果
 */
struct RayHit {
    math::Vector3 position;                     ///< 命中点
    math::Vector3 normal;                       ///< 命中点法线
    float distance{0.0f};                       ///< 距离
    uint32_t collisionObject{UINT32_MAX};       ///< 命中的碰撞对象
    uint32_t triangleIndex{UINT32_MAX};         ///< 三角形索引（如果是网格）
    void* userData{nullptr};                    ///< 用户数据
    
    RayHit() = default;
    RayHit(const math::Vector3& pos, const math::Vector3& norm, float dist)
        : position(pos), normal(norm), distance(dist) {}
};

/**
 * @brief 形状扫描命中结果
 */
struct ShapeHit {
    math::Vector3 position;                     ///< 命中点
    math::Vector3 normal;                       ///< 命中点法线
    float fraction{0.0f};                       ///< 扫描进度 (0-1)
    uint32_t collisionObject{UINT32_MAX};       ///< 命中的对象
    void* userData{nullptr};                    ///< 用户数据
};

/**
 * @brief 物理材质
 */
struct PhysicsMaterial {
    float friction{0.5f};                       ///< 摩擦系数
    float restitution{0.0f};                    ///< 弹性系数 (0-1)
    float density{1.0f};                        ///< 密度
    float linearDamping{0.0f};                  ///< 线性阻尼
    float angularDamping{0.0f};                 ///< 角阻尼
    
    PhysicsMaterial() = default;
    PhysicsMaterial(float fric, float rest, float dens = 1.0f)
        : friction(fric), restitution(rest), density(dens) {}
    
    // 预设材质
    static PhysicsMaterial metal() { return PhysicsMaterial(0.3f, 0.1f, 7.8f); }
    static PhysicsMaterial wood() { return PhysicsMaterial(0.5f, 0.2f, 0.7f); }
    static PhysicsMaterial rubber() { return PhysicsMaterial(0.8f, 0.7f, 1.1f); }
    static PhysicsMaterial ice() { return PhysicsMaterial(0.05f, 0.05f, 0.9f); }
};

/**
 * @brief 碰撞形状
 */
struct CollisionShape {
    CollisionShapeType type{CollisionShapeType::Box};
    math::Vector3 scale{1.0f};                  ///< 缩放
    float radius{1.0f};                         ///< 半径（球体/圆柱/胶囊）
    float height{1.0f};                         ///< 高度（圆柱/胶囊）
    math::Vector3 halfExtents{0.5f};            ///< 半 extents（盒子）
    std::vector<math::Vector3> vertices;        ///< 顶点（凸包/网格）
    std::vector<uint32_t> indices;              ///< 索引（网格）
    std::vector<std::pair<std::shared_ptr<CollisionShape>, math::Matrix4>> children;  ///< 子形状（复合）
    
    CollisionShape() = default;
    explicit CollisionShape(CollisionShapeType t) : type(t) {}
    
    // 工厂方法
    static std::shared_ptr<CollisionShape> createSphere(float radius);
    static std::shared_ptr<CollisionShape> createBox(const math::Vector3& halfExtents);
    static std::shared_ptr<CollisionShape> createCapsule(float radius, float height);
    static std::shared_ptr<CollisionShape> createCylinder(float radius, float height);
    static std::shared_ptr<CollisionShape> createConvexHull(const std::vector<math::Vector3>& vertices);
    static std::shared_ptr<CollisionShape> createMesh(const std::vector<math::Vector3>& vertices,
                                                       const std::vector<uint32_t>& indices);
};

/**
 * @brief 刚体组件
 */
struct RigidBodyComponent {
    RigidBodyType bodyType{RigidBodyType::Dynamic};
    PhysicsMaterial material;
    std::shared_ptr<CollisionShape> collisionShape;
    
    math::Vector3 linearVelocity{0.0f};         ///< 线速度
    math::Vector3 angularVelocity{0.0f};        ///< 角速度
    math::Vector3 linearFactor{1.0f};           ///< 线性因子
    math::Vector3 angularFactor{1.0f};          ///< 角因子
    
    float mass{1.0f};                           ///< 质量
    float maxLinearVelocity{100.0f};            ///< 最大线速度
    float maxAngularVelocity{100.0f};           ///< 最大角速度
    
    bool enableCCD{false};                      ///< 启用连续碰撞检测
    float ccdMotionThreshold{0.0f};             ///< CCD 运动阈值
    float ccdSweptSphereRadius{0.0f};           ///< CCD 扫描球半径
    
    bool isTrigger{false};                      ///< 是否为触发器
    uint32_t collisionGroup{1};                 ///< 碰撞组
    uint32_t collisionMask{0xFFFFFFFF};         ///< 碰撞掩码
    
    void* userData{nullptr};                    ///< 用户数据
    
    RigidBodyComponent() = default;
    RigidBodyComponent(RigidBodyType type, std::shared_ptr<CollisionShape> shape, float m = 1.0f)
        : bodyType(type), collisionShape(std::move(shape)), mass(m) {}
};

/**
 * @brief 碰撞事件
 */
struct CollisionEvent {
    uint32_t objectA{UINT32_MAX};               ///< 对象 A
    uint32_t objectB{UINT32_MAX};               ///< 对象 B
    math::Vector3 contactPoint;                 ///< 接触点
    math::Vector3 contactNormal;                ///< 接触法线
    float impulse{0.0f};                        ///< 冲量
    bool isEnter{true};                         ///< 是否进入碰撞
    
    CollisionEvent() = default;
    CollisionEvent(uint32_t a, uint32_t b, const math::Vector3& point, 
                   const math::Vector3& normal, float imp, bool enter)
        : objectA(a), objectB(b), contactPoint(point), contactNormal(normal),
          impulse(imp), isEnter(enter) {}
};

// ============================================================================
// 物理世界
// ============================================================================

/**
 * @brief 物理世界配置
 */
struct PhysicsWorldConfig {
    math::Vector3 gravity{0.0f, -9.81f, 0.0f};  ///< 重力
    int maxObjects{10000};                      ///< 最大对象数
    int maxProxies{10000};                      ///< 最大代理数
    bool enableSPU{true};                       ///< 启用 SPU 优化
    bool enableSat{true};                       ///< 启用 SAT 碰撞
    int threadCount{1};                         ///< 线程数
    
    PhysicsWorldConfig() = default;
};

/**
 * @brief 物理世界
 * 
 * 使用 Bullet Physics 引擎
 */
class PhysicsWorld {
public:
    PhysicsWorld();
    ~PhysicsWorld();
    
    // 禁止拷贝
    PhysicsWorld(const PhysicsWorld&) = delete;
    PhysicsWorld& operator=(const PhysicsWorld&) = delete;
    
    // ========================================================================
    // 初始化
    // ========================================================================
    
    /**
     * @brief 初始化物理世界
     */
    bool initialize(const PhysicsWorldConfig& config = PhysicsWorldConfig());
    
    /**
     * @brief 关闭物理世界
     */
    void shutdown();
    
    /**
     * @brief 是否已初始化
     */
    bool isInitialized() const noexcept { return initialized_; }
    
    // ========================================================================
    // 对象管理
    // ========================================================================
    
    /**
     * @brief 添加刚体
     * @param rigidBody 刚体组件
     * @param position 位置
     * @param rotation 旋转
     * @return 对象 ID
     */
    uint32_t addRigidBody(const RigidBodyComponent& rigidBody,
                          const math::Vector3& position = math::Vector3(0.0f),
                          const math::Quaternion& rotation = math::Quaternion::identity());
    
    /**
     * @brief 移除刚体
     */
    void removeRigidBody(uint32_t objectId);
    
    /**
     * @brief 获取刚体数量
     */
    size_t rigidBodyCount() const noexcept;
    
    // ========================================================================
    // 更新
    // ========================================================================
    
    /**
     * @brief 更新物理世界
     * @param deltaTime 时间增量
     */
    void update(float deltaTime);
    
    /**
     * @brief 设置重力
     */
    void setGravity(const math::Vector3& gravity);
    
    /**
     * @brief 获取重力
     */
    math::Vector3 gravity() const;
    
    // ========================================================================
    // 变换操作
    // ========================================================================
    
    /**
     * @brief 设置对象变换
     */
    void setTransform(uint32_t objectId,
                      const math::Vector3& position,
                      const math::Quaternion& rotation);
    
    /**
     * @brief 获取对象变换
     */
    math::Matrix4 getTransform(uint32_t objectId) const;
    
    /**
     * @brief 获取对象位置
     */
    math::Vector3 getPosition(uint32_t objectId) const;
    
    /**
     * @brief 获取对象旋转
     */
    math::Quaternion getRotation(uint32_t objectId) const;
    
    // ========================================================================
    // 力与速度
    // ========================================================================
    
    /**
     * @brief 施加力
     */
    void applyForce(uint32_t objectId, const math::Vector3& force);
    
    /**
     * @brief 施加冲量
     */
    void applyImpulse(uint32_t objectId, const math::Vector3& impulse);
    
    /**
     * @brief 施加扭矩
     */
    void applyTorque(uint32_t objectId, const math::Vector3& torque);
    
    /**
     * @brief 设置线速度
     */
    void setLinearVelocity(uint32_t objectId, const math::Vector3& velocity);
    
    /**
     * @brief 获取线速度
     */
    math::Vector3 getLinearVelocity(uint32_t objectId) const;
    
    /**
     * @brief 设置角速度
     */
    void setAngularVelocity(uint32_t objectId, const math::Vector3& velocity);
    
    /**
     * @brief 获取角速度
     */
    math::Vector3 getAngularVelocity(uint32_t objectId) const;
    
    // ========================================================================
    // 碰撞检测
    // ========================================================================
    
    /**
     * @brief 射线检测
     * @param from 起点
     * @param to 终点
     * @param hits 命中结果列表
     * @param collisionMask 碰撞掩码
     * @return 是否命中
     */
    bool raycast(const math::Vector3& from, const math::Vector3& to,
                 std::vector<RayHit>& hits,
                 uint32_t collisionMask = 0xFFFFFFFF) const;
    
    /**
     * @brief 射线检测（最近命中）
     */
    bool raycastClosest(const math::Vector3& from, const math::Vector3& to,
                        RayHit& hit,
                        uint32_t collisionMask = 0xFFFFFFFF) const;
    
    /**
     * @brief 形状扫描
     */
    bool sweepTest(uint32_t objectId,
                   const math::Vector3& from, const math::Vector3& to,
                   ShapeHit& hit,
                   uint32_t collisionMask = 0xFFFFFFFF) const;
    
    /**
     * @brief 区域测试
     */
    bool testOverlap(const math::Vector3& center, float radius,
                     uint32_t collisionMask = 0xFFFFFFFF) const;
    
    // ========================================================================
    // 碰撞事件
    // ========================================================================
    
    /**
     * @brief 设置碰撞事件回调
     */
    using CollisionCallback = std::function<void(const CollisionEvent&)>;
    void setCollisionCallback(CollisionCallback callback);
    
    /**
     * @brief 获取碰撞事件
     */
    const std::vector<CollisionEvent>& collisionEvents() const noexcept { return collisionEvents_; }
    
    /**
     * @brief 清除碰撞事件
     */
    void clearCollisionEvents();
    
    // ========================================================================
    // 调试
    // ========================================================================
    
    /**
     * @brief 获取调试绘制数据
     */
    struct DebugLine {
        math::Vector3 from;
        math::Vector3 to;
        math::Vector3 color;
    };
    
    const std::vector<DebugLine>& debugLines() const noexcept { return debugLines_; }
    
private:
    bool initialized_{false};
    
    // Bullet Physics 内部指针
    void* dispatcher_{nullptr};
    void* overlappingPairCache_{nullptr};
    void* solver_{nullptr};
    void* dynamicsWorld_{nullptr};
    void* broadphase_{nullptr};
    
    std::vector<CollisionEvent> collisionEvents_;
    std::vector<DebugLine> debugLines_;
    CollisionCallback collisionCallback_;
    
    // 对象映射
    struct ObjectData {
        RigidBodyComponent component;
        void* bulletRigidBody{nullptr};
        void* bulletCollisionShape{nullptr};
        bool active{false};
    };
    
    std::vector<ObjectData> objects_;
    uint32_t nextObjectId_{1};
    
    /**
     * @brief 创建 Bullet 碰撞形状
     */
    void* createBulletShape(const CollisionShape& shape);
    
    /**
     * @brief 销毁 Bullet 碰撞形状
     */
    void destroyBulletShape(void* shape, CollisionShapeType type);
    
    /**
     * @brief 更新调试绘制
     */
    void updateDebugDraw();
};

} // namespace scene
} // namespace phoenix
