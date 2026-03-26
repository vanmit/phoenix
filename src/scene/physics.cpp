#include "phoenix/scene/physics.hpp"
#include <algorithm>
#include <cmath>

// Bullet Physics 头文件（条件编译）
#ifdef PHOENIX_USE_BULLET
#include <btBulletDynamicsCommon.h>
#endif

namespace phoenix {
namespace scene {

// ============================================================================
// CollisionShape 工厂方法
// ============================================================================

std::shared_ptr<CollisionShape> CollisionShape::createSphere(float radius) {
    auto shape = std::make_shared<CollisionShape>(CollisionShapeType::Sphere);
    shape->radius = radius;
    return shape;
}

std::shared_ptr<CollisionShape> CollisionShape::createBox(const math::Vector3& halfExtents) {
    auto shape = std::make_shared<CollisionShape>(CollisionShapeType::Box);
    shape->halfExtents = halfExtents;
    return shape;
}

std::shared_ptr<CollisionShape> CollisionShape::createCapsule(float radius, float height) {
    auto shape = std::make_shared<CollisionShape>(CollisionShapeType::Capsule);
    shape->radius = radius;
    shape->height = height;
    return shape;
}

std::shared_ptr<CollisionShape> CollisionShape::createCylinder(float radius, float height) {
    auto shape = std::make_shared<CollisionShape>(CollisionShapeType::Cylinder);
    shape->radius = radius;
    shape->height = height;
    return shape;
}

std::shared_ptr<CollisionShape> CollisionShape::createConvexHull(const std::vector<math::Vector3>& vertices) {
    auto shape = std::make_shared<CollisionShape>(CollisionShapeType::ConvexHull);
    shape->vertices = vertices;
    return shape;
}

std::shared_ptr<CollisionShape> CollisionShape::createMesh(const std::vector<math::Vector3>& vertices,
                                                            const std::vector<uint32_t>& indices) {
    auto shape = std::make_shared<CollisionShape>(CollisionShapeType::Mesh);
    shape->vertices = vertices;
    shape->indices = indices;
    return shape;
}

// ============================================================================
// PhysicsWorld 实现
// ============================================================================

PhysicsWorld::PhysicsWorld() = default;

PhysicsWorld::~PhysicsWorld() {
    shutdown();
}

bool PhysicsWorld::initialize(const PhysicsWorldConfig& config) {
#ifdef PHOENIX_USE_BULLET
    // 创建碰撞配置
    auto* collisionConfiguration = new btDefaultCollisionConfiguration();
    dispatcher_ = new btCollisionDispatcher(collisionConfiguration);
    
    // 创建重叠对缓存
    overlappingPairCache_ = new btDbvtBroadphase();
    
    // 创建约束求解器
    solver_ = new btSequentialImpulseConstraintSolver();
    
    // 创建动力学世界
    dynamicsWorld_ = new btDiscreteDynamicsWorld(
        static_cast<btCollisionDispatcher*>(dispatcher_),
        static_cast<btBroadphaseInterface*>(overlappingPairCache_),
        static_cast<btSequentialImpulseConstraintSolver*>(solver_),
        collisionConfiguration
    );
    
    // 设置重力
    btVector3 gravity(config.gravity.x, config.gravity.y, config.gravity.z);
    static_cast<btDiscreteDynamicsWorld*>(dynamicsWorld_)->setGravity(gravity);
    
    // 设置碰撞回调
    static_cast<btDiscreteDynamicsWorld*>(dynamicsWorld_)->setInternalTickCallback(
        [](btDynamicsWorld* world, btScalar timeStep) {
            // 碰撞回调处理
        },
        this,
        true
    );
    
    initialized_ = true;
    return true;
#else
    // 无 Bullet 时的简化实现
    initialized_ = true;
    return true;
#endif
}

void PhysicsWorld::shutdown() {
#ifdef PHOENIX_USE_BULLET
    if (dynamicsWorld_) {
        // 移除所有刚体
        for (int i = dynamicsWorld_->getNumCollisionObjects() - 1; i >= 0; --i) {
            btCollisionObject* obj = dynamicsWorld_->getCollisionObjectArray()[i];
            btRigidBody* body = btRigidBody::upcast(obj);
            if (body && body->getMotionState()) {
                delete body->getMotionState();
            }
            dynamicsWorld_->removeCollisionObject(obj);
            delete obj;
        }
        
        delete dynamicsWorld_;
        delete solver_;
        delete overlappingPairCache_;
        delete dispatcher_;
    }
#endif
    
    // 清理碰撞形状
    for (auto& obj : objects_) {
        if (obj.bulletCollisionShape) {
            // 销毁形状
        }
    }
    
    objects_.clear();
    initialized_ = false;
}

size_t PhysicsWorld::rigidBodyCount() const noexcept {
    size_t count = 0;
    for (const auto& obj : objects_) {
        if (obj.active) ++count;
    }
    return count;
}

uint32_t PhysicsWorld::addRigidBody(const RigidBodyComponent& rigidBody,
                                     const math::Vector3& position,
                                     const math::Quaternion& rotation) {
    if (!initialized_) {
        return UINT32_MAX;
    }
    
    // 分配对象 ID
    uint32_t objectId = nextObjectId_++;
    if (objectId >= objects_.size()) {
        objects_.resize(objectId + 1);
    }
    
    ObjectData& obj = objects_[objectId];
    obj.component = rigidBody;
    obj.active = true;
    
#ifdef PHOENIX_USE_BULLET
    // 创建碰撞形状
    void* bulletShape = createBulletShape(*rigidBody.collisionShape);
    obj.bulletCollisionShape = bulletShape;
    
    // 创建刚体
    btCollisionShape* shape = static_cast<btCollisionShape*>(bulletShape);
    
    // 计算质量
    btScalar mass = rigidBody.mass;
    btVector3 localInertia(0, 0, 0);
    if (mass > 0.0f) {
        shape->calculateLocalInertia(mass, localInertia);
    }
    
    // 创建运动状态
    btTransform startTransform;
    startTransform.setOrigin(btVector3(position.x, position.y, position.z));
    startTransform.setRotation(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w));
    
    btDefaultMotionState* motionState = new btDefaultMotionState(startTransform);
    
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, shape, localInertia);
    rbInfo.m_friction = rigidBody.material.friction;
    rbInfo.m_restitution = rigidBody.material.restitution;
    rbInfo.m_linearDamping = rigidBody.material.linearDamping;
    rbInfo.m_angularDamping = rigidBody.material.angularDamping;
    
    btRigidBody* body = new btRigidBody(rbInfo);
    body->setUserPointer(reinterpret_cast<void*>(static_cast<uintptr_t>(objectId)));
    
    // 设置刚体类型
    if (rigidBody.bodyType == RigidBodyType::Static) {
        body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT);
    } else if (rigidBody.bodyType == RigidBodyType::Kinematic) {
        body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
    }
    
    // 设置 CCD
    if (rigidBody.enableCCD) {
        body->setCcdMotionThreshold(rigidBody.ccdMotionThreshold);
        body->setCcdSweptSphereRadius(rigidBody.ccdSweptSphereRadius);
    }
    
    // 添加到世界
    static_cast<btDiscreteDynamicsWorld*>(dynamicsWorld_)->addRigidBody(
        body,
        rigidBody.collisionGroup,
        rigidBody.collisionMask
    );
    
    obj.bulletRigidBody = body;
#endif
    
    return objectId;
}

void PhysicsWorld::removeRigidBody(uint32_t objectId) {
    if (objectId >= objects_.size() || !objects_[objectId].active) {
        return;
    }
    
#ifdef PHOENIX_USE_BULLET
    ObjectData& obj = objects_[objectId];
    
    if (obj.bulletRigidBody && dynamicsWorld_) {
        btRigidBody* body = static_cast<btRigidBody*>(obj.bulletRigidBody);
        static_cast<btDiscreteDynamicsWorld*>(dynamicsWorld_)->removeRigidBody(body);
        
        if (body->getMotionState()) {
            delete body->getMotionState();
        }
        delete body;
    }
    
    if (obj.bulletCollisionShape) {
        destroyBulletShape(obj.bulletCollisionShape, obj.component.collisionShape->type);
    }
#endif
    
    obj.active = false;
}

void PhysicsWorld::update(float deltaTime) {
    if (!initialized_) {
        return;
    }
    
#ifdef PHOENIX_USE_BULLET
    // 步进模拟
    static_cast<btDiscreteDynamicsWorld*>(dynamicsWorld_)->stepSimulation(deltaTime, 10);
    
    // 同步变换
    for (auto& obj : objects_) {
        if (!obj.active || !obj.bulletRigidBody) continue;
        
        btRigidBody* body = static_cast<btRigidBody*>(obj.bulletRigidBody);
        btTransform trans;
        body->getMotionState()->getWorldTransform(trans);
        
        btVector3 origin = trans.getOrigin();
        btQuaternion rot = trans.getRotation();
        
        // 更新组件
        // TODO: 同步到场景图
    }
    
    // 更新调试绘制
    updateDebugDraw();
#else
    // 简化更新（无 Bullet）
    for (auto& obj : objects_) {
        if (!obj.active) continue;
        
        // 简化物理
        if (obj.component.bodyType == RigidBodyType::Dynamic) {
            // 应用重力
            obj.component.linearVelocity = obj.component.linearVelocity + 
                                           math::Vector3(0.0f, -9.81f, 0.0f) * deltaTime;
            
            // 应用阻尼
            obj.component.linearVelocity = obj.component.linearVelocity * 
                                           (1.0f - obj.component.material.linearDamping);
        }
    }
#endif
    
    clearCollisionEvents();
}

void PhysicsWorld::setGravity(const math::Vector3& gravity) {
#ifdef PHOENIX_USE_BULLET
    if (dynamicsWorld_) {
        static_cast<btDiscreteDynamicsWorld*>(dynamicsWorld_)->setGravity(
            btVector3(gravity.x, gravity.y, gravity.z)
        );
    }
#endif
}

math::Vector3 PhysicsWorld::gravity() const {
#ifdef PHOENIX_USE_BULLET
    if (dynamicsWorld_) {
        btVector3 g = static_cast<btDiscreteDynamicsWorld*>(dynamicsWorld_)->getGravity();
        return math::Vector3(g.x(), g.y(), g.z());
    }
#endif
    return math::Vector3(0.0f, -9.81f, 0.0f);
}

void PhysicsWorld::setTransform(uint32_t objectId,
                                 const math::Vector3& position,
                                 const math::Quaternion& rotation) {
    if (objectId >= objects_.size() || !objects_[objectId].active) {
        return;
    }
    
#ifdef PHOENIX_USE_BULLET
    ObjectData& obj = objects_[objectId];
    if (obj.bulletRigidBody) {
        btRigidBody* body = static_cast<btRigidBody*>(obj.bulletRigidBody);
        
        btTransform trans;
        trans.setOrigin(btVector3(position.x, position.y, position.z));
        trans.setRotation(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w));
        
        body->setWorldTransform(trans);
        if (body->getMotionState()) {
            body->getMotionState()->setWorldTransform(trans);
        }
    }
#endif
}

math::Matrix4 PhysicsWorld::getTransform(uint32_t objectId) const {
    math::Matrix4 result = math::Matrix4::identity();
    
    if (objectId >= objects_.size() || !objects_[objectId].active) {
        return result;
    }
    
#ifdef PHOENIX_USE_BULLET
    const ObjectData& obj = objects_[objectId];
    if (obj.bulletRigidBody) {
        const btRigidBody* body = static_cast<const btRigidBody*>(obj.bulletRigidBody);
        btTransform trans;
        body->getMotionState()->getWorldTransform(trans);
        
        btVector3 origin = trans.getOrigin();
        btQuaternion rot = trans.getRotation();
        
        math::Quaternion q(rot.x(), rot.y(), rot.z(), rot.w());
        math::Vector3 p(origin.x(), origin.y(), origin.z());
        
        result = math::Matrix4::translate(p) * q.toMatrix();
    }
#endif
    
    return result;
}

math::Vector3 PhysicsWorld::getPosition(uint32_t objectId) const {
    if (objectId >= objects_.size() || !objects_[objectId].active) {
        return math::Vector3(0.0f);
    }
    
#ifdef PHOENIX_USE_BULLET
    const ObjectData& obj = objects_[objectId];
    if (obj.bulletRigidBody) {
        const btRigidBody* body = static_cast<const btRigidBody*>(obj.bulletRigidBody);
        btTransform trans;
        body->getMotionState()->getWorldTransform(trans);
        btVector3 origin = trans.getOrigin();
        return math::Vector3(origin.x(), origin.y(), origin.z());
    }
#endif
    
    return math::Vector3(0.0f);
}

math::Quaternion PhysicsWorld::getRotation(uint32_t objectId) const {
    if (objectId >= objects_.size() || !objects_[objectId].active) {
        return math::Quaternion::identity();
    }
    
#ifdef PHOENIX_USE_BULLET
    const ObjectData& obj = objects_[objectId];
    if (obj.bulletRigidBody) {
        const btRigidBody* body = static_cast<const btRigidBody*>(obj.bulletRigidBody);
        btTransform trans;
        body->getMotionState()->getWorldTransform(trans);
        btQuaternion rot = trans.getRotation();
        return math::Quaternion(rot.x(), rot.y(), rot.z(), rot.w());
    }
#endif
    
    return math::Quaternion::identity();
}

void PhysicsWorld::applyForce(uint32_t objectId, const math::Vector3& force) {
#ifdef PHOENIX_USE_BULLET
    if (objectId >= objects_.size() || !objects_[objectId].active) return;
    
    ObjectData& obj = objects_[objectId];
    if (obj.bulletRigidBody) {
        btRigidBody* body = static_cast<btRigidBody*>(obj.bulletRigidBody);
        body->applyForce(btVector3(force.x, force.y, force.z), btVector3(0, 0, 0));
    }
#else
    if (objectId < objects_.size() && objects_[objectId].active) {
        // 简化：直接改变速度
        float dt = 0.016f;  // 假设 60fps
        objects_[objectId].component.linearVelocity = 
            objects_[objectId].component.linearVelocity + force * dt;
    }
#endif
}

void PhysicsWorld::applyImpulse(uint32_t objectId, const math::Vector3& impulse) {
#ifdef PHOENIX_USE_BULLET
    if (objectId >= objects_.size() || !objects_[objectId].active) return;
    
    ObjectData& obj = objects_[objectId];
    if (obj.bulletRigidBody) {
        btRigidBody* body = static_cast<btRigidBody*>(obj.bulletRigidBody);
        body->applyImpulse(btVector3(impulse.x, impulse.y, impulse.z), btVector3(0, 0, 0));
    }
#else
    if (objectId < objects_.size() && objects_[objectId].active) {
        float invMass = 1.0f / objects_[objectId].component.mass;
        objects_[objectId].component.linearVelocity = 
            objects_[objectId].component.linearVelocity + impulse * invMass;
    }
#endif
}

void PhysicsWorld::applyTorque(uint32_t objectId, const math::Vector3& torque) {
#ifdef PHOENIX_USE_BULLET
    if (objectId >= objects_.size() || !objects_[objectId].active) return;
    
    ObjectData& obj = objects_[objectId];
    if (obj.bulletRigidBody) {
        btRigidBody* body = static_cast<btRigidBody*>(obj.bulletRigidBody);
        body->applyTorque(btVector3(torque.x, torque.y, torque.z));
    }
#endif
}

void PhysicsWorld::setLinearVelocity(uint32_t objectId, const math::Vector3& velocity) {
#ifdef PHOENIX_USE_BULLET
    if (objectId >= objects_.size() || !objects_[objectId].active) return;
    
    ObjectData& obj = objects_[objectId];
    if (obj.bulletRigidBody) {
        btRigidBody* body = static_cast<btRigidBody*>(obj.bulletRigidBody);
        body->setLinearVelocity(btVector3(velocity.x, velocity.y, velocity.z));
    }
#else
    if (objectId < objects_.size() && objects_[objectId].active) {
        objects_[objectId].component.linearVelocity = velocity;
    }
#endif
}

math::Vector3 PhysicsWorld::getLinearVelocity(uint32_t objectId) const {
#ifdef PHOENIX_USE_BULLET
    if (objectId >= objects_.size() || !objects_[objectId].active) {
        return math::Vector3(0.0f);
    }
    
    const ObjectData& obj = objects_[objectId];
    if (obj.bulletRigidBody) {
        const btRigidBody* body = static_cast<const btRigidBody*>(obj.bulletRigidBody);
        btVector3 vel = body->getLinearVelocity();
        return math::Vector3(vel.x(), vel.y(), vel.z());
    }
#endif
    
    return math::Vector3(0.0f);
}

void PhysicsWorld::setAngularVelocity(uint32_t objectId, const math::Vector3& velocity) {
#ifdef PHOENIX_USE_BULLET
    if (objectId >= objects_.size() || !objects_[objectId].active) return;
    
    ObjectData& obj = objects_[objectId];
    if (obj.bulletRigidBody) {
        btRigidBody* body = static_cast<btRigidBody*>(obj.bulletRigidBody);
        body->setAngularVelocity(btVector3(velocity.x, velocity.y, velocity.z));
    }
#endif
}

math::Vector3 PhysicsWorld::getAngularVelocity(uint32_t objectId) const {
#ifdef PHOENIX_USE_BULLET
    if (objectId >= objects_.size() || !objects_[objectId].active) {
        return math::Vector3(0.0f);
    }
    
    const ObjectData& obj = objects_[objectId];
    if (obj.bulletRigidBody) {
        const btRigidBody* body = static_cast<const btRigidBody*>(obj.bulletRigidBody);
        btVector3 vel = body->getAngularVelocity();
        return math::Vector3(vel.x(), vel.y(), vel.z());
    }
#endif
    
    return math::Vector3(0.0f);
}

bool PhysicsWorld::raycast(const math::Vector3& from, const math::Vector3& to,
                            std::vector<RayHit>& hits,
                            uint32_t collisionMask) const {
#ifdef PHOENIX_USE_BULLET
    if (!dynamicsWorld_) return false;
    
    btVector3 btFrom(from.x, from.y, from.z);
    btVector3 btTo(to.x, to.y, to.z);
    
    struct RayCallback : public btCollisionWorld::ClosestRayResultCallback {
        std::vector<RayHit>& hits;
        
        RayCallback(const btVector3& f, const btVector3& t, std::vector<RayHit>& h)
            : ClosestRayResultCallback(f, t), hits(h) {}
        
        btScalar addSingleResult(btCollisionWorld::LocalRayResult& result, bool normalInWorldSpace) override {
            RayHit hit;
            hit.position = math::Vector3(result.m_hitPointWorld.getX(),
                                          result.m_hitPointWorld.getY(),
                                          result.m_hitPointWorld.getZ());
            hit.normal = math::Vector3(result.m_hitNormalWorld.getX(),
                                        result.m_hitNormalWorld.getY(),
                                        result.m_hitNormalWorld.getZ());
            hit.distance = static_cast<float>(m_closestHitFraction);
            hit.collisionObject = reinterpret_cast<uintptr_t>(result.m_collisionObject->getUserPointer());
            hits.push_back(hit);
            return result.m_closestHitFraction;
        }
    };
    
    RayCallback callback(btFrom, btTo, hits);
    callback.m_collisionFilterMask = collisionMask;
    
    static_cast<btCollisionWorld*>(dynamicsWorld_)->rayTest(btFrom, btTo, callback);
    
    return !hits.empty();
#else
    // 简化实现：无碰撞检测
    return false;
#endif
}

bool PhysicsWorld::raycastClosest(const math::Vector3& from, const math::Vector3& to,
                                   RayHit& hit,
                                   uint32_t collisionMask) const {
    std::vector<RayHit> hits;
    if (raycast(from, to, hits, collisionMask)) {
        hit = hits[0];
        return true;
    }
    return false;
}

bool PhysicsWorld::sweepTest(uint32_t objectId,
                              const math::Vector3& from, const math::Vector3& to,
                              ShapeHit& hit,
                              uint32_t collisionMask) const {
    // TODO: 实现形状扫描
    return false;
}

bool PhysicsWorld::testOverlap(const math::Vector3& center, float radius,
                                uint32_t collisionMask) const {
    // TODO: 实现区域测试
    return false;
}

void PhysicsWorld::setCollisionCallback(CollisionCallback callback) {
    collisionCallback_ = std::move(callback);
}

void PhysicsWorld::clearCollisionEvents() {
    collisionEvents_.clear();
}

void* PhysicsWorld::createBulletShape(const CollisionShape& shape) {
#ifdef PHOENIX_USE_BULLET
    switch (shape.type) {
        case CollisionShapeType::Sphere:
            return new btSphereShape(shape.radius);
            
        case CollisionShapeType::Box:
            return new btBoxShape(btVector3(shape.halfExtents.x, 
                                             shape.halfExtents.y, 
                                             shape.halfExtents.z));
            
        case CollisionShapeType::Capsule:
            return new btCapsuleShape(shape.radius, shape.height);
            
        case CollisionShapeType::Cylinder:
            return new btCylinderShape(btVector3(shape.radius, shape.height * 0.5f, shape.radius));
            
        case CollisionShapeType::ConvexHull: {
            auto* hull = new btConvexHullShape();
            for (const auto& v : shape.vertices) {
                hull->addPoint(btVector3(v.x, v.y, v.z));
            }
            return hull;
        }
            
        case CollisionShapeType::Mesh: {
            auto* mesh = new btTriangleMesh();
            for (size_t i = 0; i < shape.indices.size(); i += 3) {
                const auto& v0 = shape.vertices[shape.indices[i]];
                const auto& v1 = shape.vertices[shape.indices[i + 1]];
                const auto& v2 = shape.vertices[shape.indices[i + 2]];
                mesh->addTriangle(btVector3(v0.x, v0.y, v0.z),
                                  btVector3(v1.x, v1.y, v1.z),
                                  btVector3(v2.x, v2.y, v2.z));
            }
            return new btBvhTriangleMeshShape(mesh, true);
        }
            
        default:
            return new btBoxShape(btVector3(1, 1, 1));
    }
#else
    return nullptr;
#endif
}

void PhysicsWorld::destroyBulletShape(void* shape, CollisionShapeType type) {
#ifdef PHOENIX_USE_BULLET
    if (!shape) return;
    
    switch (type) {
        case CollisionShapeType::Mesh: {
            auto* meshShape = static_cast<btBvhTriangleMeshShape*>(shape);
            delete meshShape->getMeshInterface();
            delete meshShape;
            break;
        }
        default:
            delete static_cast<btCollisionShape*>(shape);
            break;
    }
#endif
}

void PhysicsWorld::updateDebugDraw() {
    debugLines_.clear();
    
#ifdef PHOENIX_USE_BULLET
    // TODO: 实现调试绘制
#endif
}

} // namespace scene
} // namespace phoenix
