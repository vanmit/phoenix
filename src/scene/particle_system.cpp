#include "phoenix/scene/particle_system.hpp"
#include <random>
#include <algorithm>
#include <cmath>

namespace phoenix {
namespace scene {

// ============================================================================
// ParticleSystem 实现
// ============================================================================

ParticleSystem::ParticleSystem() = default;

ParticleSystem::~ParticleSystem() {
    shutdown();
}

bool ParticleSystem::initialize(size_t maxParticles, bool useGPU) {
    if (initialized_) {
        return true;
    }
    
    maxParticles_ = maxParticles;
    useGPU_ = useGPU && false;  // 暂时禁用 GPU，待实现完整渲染后端
    
    particles_.reserve(maxParticles_);
    particles_.resize(maxParticles_);
    
    // 初始化 GPU 数据
    gpuData_.maxParticles = static_cast<uint32_t>(maxParticles_);
    gpuData_.particleCount = 0;
    
    if (useGPU_ && !initializeGPU()) {
        useGPU_ = false;
    }
    
    initialized_ = true;
    return true;
}

void ParticleSystem::shutdown() {
    if (useGPU_) {
        destroyGPU();
    }
    
    particles_.clear();
    emitters_.clear();
    forceFields_.clear();
    activeParticleCount_ = 0;
    initialized_ = false;
}

bool ParticleSystem::initializeGPU() {
    // TODO: 实现 GPU 初始化（需要渲染后端支持）
    // 创建计算着色器、缓冲区等
    return false;
}

void ParticleSystem::destroyGPU() {
    // TODO: 销毁 GPU 资源
    gpuPositionBuffer_ = nullptr;
    gpuVelocityBuffer_ = nullptr;
    gpuParticleBuffer_ = nullptr;
    computePipeline_ = nullptr;
}

uint32_t ParticleSystem::addEmitter(const ParticleEmitterConfig& config) {
    emitters_.push_back(config);
    return static_cast<uint32_t>(emitters_.size() - 1);
}

void ParticleSystem::removeEmitter(uint32_t emitterIndex) {
    if (emitterIndex < emitters_.size()) {
        emitters_.erase(emitters_.begin() + emitterIndex);
    }
}

ParticleEmitterConfig* ParticleSystem::getEmitter(uint32_t emitterIndex) {
    if (emitterIndex < emitters_.size()) {
        return &emitters_[emitterIndex];
    }
    return nullptr;
}

const ParticleEmitterConfig* ParticleSystem::getEmitter(uint32_t emitterIndex) const {
    if (emitterIndex < emitters_.size()) {
        return &emitters_[emitterIndex];
    }
    return nullptr;
}

uint32_t ParticleSystem::addForceField(const ParticleForceField& field) {
    forceFields_.push_back(field);
    return static_cast<uint32_t>(forceFields_.size() - 1);
}

void ParticleSystem::removeForceField(uint32_t fieldIndex) {
    if (fieldIndex < forceFields_.size()) {
        forceFields_.erase(forceFields_.begin() + fieldIndex);
    }
}

ParticleForceField* ParticleSystem::getForceField(uint32_t fieldIndex) {
    if (fieldIndex < forceFields_.size()) {
        return &forceFields_[fieldIndex];
    }
    return nullptr;
}

const ParticleForceField* ParticleSystem::getForceField(uint32_t fieldIndex) const {
    if (fieldIndex < forceFields_.size()) {
        return &forceFields_[fieldIndex];
    }
    return nullptr;
}

void ParticleSystem::play() {
    playing_ = true;
}

void ParticleSystem::pause() {
    playing_ = false;
}

void ParticleSystem::stop() {
    playing_ = false;
    clear();
}

void ParticleSystem::burst(uint32_t emitterIndex, uint32_t count) {
    if (emitterIndex >= emitters_.size()) {
        return;
    }
    
    // 立即发射指定数量的粒子
    const auto& emitter = emitters_[emitterIndex];
    
    static std::mt19937 rng(emitter.randomSeed);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    for (uint32_t i = 0; i < count && activeParticleCount_ < maxParticles_; ++i) {
        // 找到空闲粒子槽
        size_t particleIndex = activeParticleCount_;
        
        Particle& p = particles_[particleIndex];
        p.lifetime = 0.0f;
        p.invLifetime = 1.0f / (emitter.minLifetime + dist(rng) * (emitter.maxLifetime - emitter.minLifetime));
        p.position = emitter.position;
        
        // 根据发射形状生成位置
        switch (emitter.shape) {
            case EmitterShape::Sphere: {
                math::Vector3 dir(dist(rng) * 2 - 1, dist(rng) * 2 - 1, dist(rng) * 2 - 1);
                dir = dir.normalized() * dist(rng) * emitter.radius;
                p.position = emitter.position + dir;
                break;
            }
            case EmitterShape::Box: {
                p.position.x += (dist(rng) * 2 - 1) * emitter.halfExtents.x;
                p.position.y += (dist(rng) * 2 - 1) * emitter.halfExtents.y;
                p.position.z += (dist(rng) * 2 - 1) * emitter.halfExtents.z;
                break;
            }
            case EmitterShape::Cone: {
                float angle = dist(rng) * emitter.coneAngle;
                float height = dist(rng) * emitter.coneHeight;
                float radius = height * std::tan(angle) * dist(rng);
                float theta = dist(rng) * 6.28318f;
                p.position.x += radius * std::cos(theta);
                p.position.z += radius * std::sin(theta);
                p.position.y += height;
                break;
            }
            default:
                break;
        }
        
        // 生成速度
        math::Vector3 dir = emitter.direction;
        if (emitter.spreadAngle > 0) {
            // 添加扩散
            float spread = emitter.spreadAngle * (dist(rng) * 2 - 1);
            // TODO: 正确的扩散计算
        }
        
        float speed = emitter.minSpeed + dist(rng) * (emitter.maxSpeed - emitter.minSpeed);
        p.velocity = dir.normalized() * speed;
        
        // 添加切向速度
        if (emitter.tangentVelocity > 0) {
            math::Vector3 tangent = math::Vector3(-dir.z, 0, dir.x).normalized();
            p.velocity = p.velocity + tangent * emitter.tangentVelocity * (dist(rng) * 2 - 1);
        }
        
        // 尺寸
        float size = emitter.minSize + dist(rng) * (emitter.maxSize - emitter.minSize);
        p.size = math::Vector3(size);
        
        // 颜色
        float r = emitter.minColor.r + dist(rng) * (emitter.maxColor.r - emitter.minColor.r);
        float g = emitter.minColor.g + dist(rng) * (emitter.maxColor.g - emitter.minColor.g);
        float b = emitter.minColor.b + dist(rng) * (emitter.maxColor.b - emitter.minColor.b);
        float a = emitter.minColor.a + dist(rng) * (emitter.maxColor.a - emitter.minColor.a);
        p.color = math::Vector4(r, g, b, a);
        
        // 旋转
        p.rotation = emitter.minRotation + dist(rng) * (emitter.maxRotation - emitter.minRotation);
        p.angularVelocity = emitter.minAngularVelocity + dist(rng) * (emitter.maxAngularVelocity - emitter.minAngularVelocity);
        
        // 随机种子
        p.seed = static_cast<uint32_t>(dist(rng) * UINT32_MAX);
        
        ++activeParticleCount_;
    }
}

void ParticleSystem::clear() {
    activeParticleCount_ = 0;
    gpuData_.particleCount = 0;
}

void ParticleSystem::update(float deltaTime) {
    if (!initialized_ || !playing_) {
        return;
    }
    
    simulationTime_ += deltaTime;
    
    if (useGPU_) {
        updateGPU(deltaTime);
    } else {
        updateCPU(deltaTime);
    }
    
    // 更新 GPU 数据
    gpuData_.particleCount = static_cast<uint32_t>(activeParticleCount_);
    gpuData_.deltaTime = deltaTime;
    gpuData_.simulationTime = simulationTime_;
}

void ParticleSystem::updateCPU(float deltaTime) {
    static std::mt19937 rng(12345);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    // 发射新粒子
    for (size_t i = 0; i < emitters_.size(); ++i) {
        emitParticles(static_cast<uint32_t>(i), deltaTime);
    }
    
    // 更新现有粒子
    size_t writeIndex = 0;
    
    for (size_t i = 0; i < activeParticleCount_; ++i) {
        Particle& p = particles_[i];
        
        // 更新寿命
        p.lifetime += deltaTime;
        
        // 检查是否死亡
        if (p.lifetime >= 1.0f / p.invLifetime) {
            continue;  // 跳过死亡的粒子
        }
        
        // 应用力场
        applyForceFields(p, deltaTime);
        
        // 更新位置
        p.velocity = p.velocity + p.acceleration * deltaTime;
        p.position = p.position + p.velocity * deltaTime;
        
        // 更新旋转
        p.rotation += p.angularVelocity * deltaTime;
        
        // 处理碰撞
        if (collisionConfig_.enabled) {
            handleCollision(p, deltaTime);
        }
        
        // 应用尺寸变化
        float t = p.lifetime * p.invLifetime;
        p.size = p.size * math::Vector3(
            1.0f + (p.size.x - 1.0f) * t,
            1.0f + (p.size.y - 1.0f) * t,
            1.0f + (p.size.z - 1.0f) * t
        );
        
        // 应用颜色变化
        p.color.w = p.color.w * (1.0f - t);  // 简单的淡出
        
        // 复制到写入位置
        if (writeIndex != i) {
            particles_[writeIndex] = p;
        }
        ++writeIndex;
    }
    
    activeParticleCount_ = writeIndex;
}

void ParticleSystem::updateGPU(float deltaTime) {
    // TODO: 实现 GPU 更新（需要渲染后端）
    // 绑定计算着色器、设置参数、调度
}

void ParticleSystem::emitParticles(uint32_t emitterIndex, float deltaTime) {
    if (emitterIndex >= emitters_.size()) {
        return;
    }
    
    const auto& emitter = emitters_[emitterIndex];
    
    // 计算发射数量
    float emissionCount = emitter.emissionRate * deltaTime;
    uint32_t count = static_cast<uint32_t>(emissionCount);
    
    // 处理小数部分（累积）
    static std::vector<float> carry(emitters_.size(), 0.0f);
    carry[emitterIndex] += emissionCount - count;
    if (carry[emitterIndex] >= 1.0f) {
        ++count;
        carry[emitterIndex] -= 1.0f;
    }
    
    // 爆发发射
    if (emitter.burstCount > 0 && emitter.burstInterval > 0) {
        static std::vector<float> burstTimer(emitters_.size(), 0.0f);
        burstTimer[emitterIndex] += deltaTime;
        
        if (burstTimer[emitterIndex] >= emitter.burstInterval) {
            count += static_cast<uint32_t>(emitter.burstCount);
            burstTimer[emitterIndex] = 0.0f;
        }
    }
    
    // 发射粒子
    for (uint32_t i = 0; i < count && activeParticleCount_ < maxParticles_; ++i) {
        size_t particleIndex = activeParticleCount_++;
        Particle& p = particles_[particleIndex];
        
        p.lifetime = 0.0f;
        p.invLifetime = 1.0f / (emitter.minLifetime + dist(rng) * (emitter.maxLifetime - emitter.minLifetime));
        p.position = emitter.position;
        
        // 根据形状生成位置偏移
        switch (emitter.shape) {
            case EmitterShape::Point:
                break;
                
            case EmitterShape::Sphere: {
                math::Vector3 dir(dist(rng) * 2 - 1, dist(rng) * 2 - 1, dist(rng) * 2 - 1);
                float r = std::cbrt(dist(rng)) * emitter.radius;
                dir = dir.normalized() * r;
                p.position = emitter.position + dir;
                break;
            }
            
            case EmitterShape::Box: {
                p.position.x += (dist(rng) * 2 - 1) * emitter.halfExtents.x;
                p.position.y += (dist(rng) * 2 - 1) * emitter.halfExtents.y;
                p.position.z += (dist(rng) * 2 - 1) * emitter.halfExtents.z;
                break;
            }
            
            case EmitterShape::Circle: {
                float theta = dist(rng) * 6.28318f;
                float r = std::sqrt(dist(rng)) * emitter.radius;
                p.position.x += r * std::cos(theta);
                p.position.z += r * std::sin(theta);
                break;
            }
            
            case EmitterShape::Cone: {
                float height = dist(rng) * emitter.coneHeight;
                float radius = height * std::tan(emitter.coneAngle) * std::sqrt(dist(rng));
                float theta = dist(rng) * 6.28318f;
                p.position.x += radius * std::cos(theta);
                p.position.z += radius * std::sin(theta);
                p.position.y += height;
                break;
            }
            
            default:
                break;
        }
        
        // 生成速度方向
        math::Vector3 dir = emitter.direction.normalized();
        
        if (emitter.spreadAngle > 0) {
            // 在圆锥内随机方向
            float cosAngle = std::cos(emitter.spreadAngle * 0.5f);
            float z = dist(rng) * (1.0f - cosAngle) + cosAngle;
            float t = dist(rng) * 6.28318f;
            float r = std::sqrt(1.0f - z * z);
            
            math::Vector3 randDir(r * std::cos(t), r * std::sin(t), z);
            
            // 从方向向量构建坐标系
            math::Vector3 up = std::abs(dir.y) < 0.9f ? math::Vector3(0, 1, 0) : math::Vector3(1, 0, 0);
            math::Vector3 right = dir.cross(up).normalized();
            math::Vector3 forward = right.cross(dir).normalized();
            
            dir = right * randDir.x + forward * randDir.y + dir * randDir.z;
        }
        
        // 设置速度大小
        float speed = emitter.minSpeed + dist(rng) * (emitter.maxSpeed - emitter.minSpeed);
        p.velocity = dir * speed;
        
        // 切向速度
        if (emitter.tangentVelocity > 0) {
            math::Vector3 up = std::abs(dir.y) < 0.9f ? math::Vector3(0, 1, 0) : math::Vector3(1, 0, 0);
            math::Vector3 tangent = dir.cross(up).normalized();
            p.velocity = p.velocity + tangent * emitter.tangentVelocity * (dist(rng) * 2 - 1);
        }
        
        // 尺寸
        float size = emitter.minSize + dist(rng) * (emitter.maxSize - emitter.minSize);
        p.size = math::Vector3(size);
        
        // 颜色
        p.color = math::Vector4(
            emitter.minColor.r + dist(rng) * (emitter.maxColor.r - emitter.minColor.r),
            emitter.minColor.g + dist(rng) * (emitter.maxColor.g - emitter.minColor.g),
            emitter.minColor.b + dist(rng) * (emitter.maxColor.b - emitter.minColor.b),
            emitter.minColor.a + dist(rng) * (emitter.maxColor.a - emitter.minColor.a)
        );
        
        // 旋转
        p.rotation = emitter.minRotation + dist(rng) * (emitter.maxRotation - emitter.minRotation);
        p.angularVelocity = emitter.minAngularVelocity + dist(rng) * (emitter.maxAngularVelocity - emitter.minAngularVelocity);
        
        // 种子
        p.seed = static_cast<uint32_t>(dist(rng) * UINT32_MAX);
        
        // 加速度
        p.acceleration = math::Vector3(0.0f);
    }
}

void ParticleSystem::applyForceFields(Particle& particle, float deltaTime) {
    for (const auto& field : forceFields_) {
        math::Vector3 force(0.0f);
        
        switch (field.type) {
            case ParticleForceField::Type::Gravity:
                force = field.direction * field.strength;
                break;
                
            case ParticleForceField::Type::Wind:
                force = field.direction * field.strength;
                break;
                
            case ParticleForceField::Type::Vortex: {
                math::Vector3 toParticle = particle.position - field.position;
                toParticle.y = 0;
                toParticle = toParticle.normalized();
                
                math::Vector3 axis = field.direction.normalized();
                math::Vector3 tangent = axis.cross(toParticle).normalized();
                
                force = tangent * field.strength;
                break;
            }
            
            case ParticleForceField::Type::Attractor: {
                math::Vector3 toField = field.position - particle.position;
                float dist = toField.length();
                if (dist > 0.001f && dist < field.range) {
                    float falloff = std::pow(dist / field.range, field.falloff);
                    force = toField.normalized() * field.strength * (1.0f - falloff);
                }
                break;
            }
            
            case ParticleForceField::Type::Repulsor: {
                math::Vector3 fromField = particle.position - field.position;
                float dist = fromField.length();
                if (dist > 0.001f && dist < field.range) {
                    float falloff = std::pow(dist / field.range, field.falloff);
                    force = fromField.normalized() * field.strength * (1.0f - falloff);
                }
                break;
            }
            
            case ParticleForceField::Type::Drag:
                force = -particle.velocity * field.strength;
                break;
                
            case ParticleForceField::Type::Noise: {
                float t = simulationTime_ * field.noiseSpeed + particle.seed * 0.001f;
                force = math::Vector3(
                    std::sin(t) * field.strength,
                    std::cos(t * 0.7f) * field.strength,
                    std::sin(t * 1.3f) * field.strength
                );
                break;
            }
        }
        
        particle.acceleration = particle.acceleration + force;
    }
}

void ParticleSystem::handleCollision(Particle& particle, float deltaTime) {
    // 简化碰撞处理
    // 实际项目需要与物理世界集成
    
    // 地面碰撞
    if (particle.position.y < collisionConfig_.radius) {
        if (collisionConfig_.response == ParticleCollisionConfig::Response::Bounce) {
            particle.position.y = collisionConfig_.radius;
            particle.velocity.y = -particle.velocity.y * collisionConfig_.bounce;
            particle.velocity.x *= collisionConfig_.damping;
            particle.velocity.z *= collisionConfig_.damping;
        } else if (collisionConfig_.response == ParticleCollisionConfig::Response::Kill) {
            particle.lifetime = 1.0f / particle.invLifetime;  // 立即死亡
        }
    }
}

void ParticleSystem::setCollisionConfig(const ParticleCollisionConfig& config) {
    collisionConfig_ = config;
}

size_t ParticleSystem::memoryUsage() const noexcept {
    size_t total = 0;
    total += particles_.capacity() * sizeof(Particle);
    total += emitters_.size() * sizeof(ParticleEmitterConfig);
    total += forceFields_.size() * sizeof(ParticleForceField);
    return total;
}

void ParticleSystem::setRenderMode(ParticleRenderMode mode) {
    renderMode_ = mode;
}

} // namespace scene
} // namespace phoenix
