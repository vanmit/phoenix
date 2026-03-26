#pragma once

#include "../math/vector3.hpp"
#include "../math/vector4.hpp"
#include "../math/color.hpp"
#include <vector>
#include <memory>
#include <functional>
#include <cstdint>

namespace phoenix {
namespace scene {

// ============================================================================
// 粒子基础类型
// ============================================================================

/**
 * @brief 粒子渲染模式
 */
enum class ParticleRenderMode : uint8_t {
    Billboard,          ///< 始终面向相机
    StretchedBillboard, ///< 沿速度方向拉伸
    Mesh,               ///< 使用网格
    HorizontalBillboard,///< 水平面向上
    VerticalBillboard   ///< 垂直面向上
};

/**
 * @brief 粒子发射形状
 */
enum class EmitterShape : uint8_t {
    Point,              ///< 点发射
    Sphere,             ///< 球体发射
    Box,                ///< 盒子发射
    Cone,               ///< 圆锥发射
    Circle,             ///< 圆形发射
    Mesh                ///< 网格表面发射
};

/**
 * @brief 粒子数据结构（GPU 友好）
 */
struct alignas(16) Particle {
    math::Vector3 position;         ///< 位置
    float lifetime;                 ///< 当前寿命
    math::Vector3 velocity;         ///< 速度
    float invLifetime;              ///< 1/总寿命
    math::Vector3 size;             ///< 尺寸
    uint32_t flags;                 ///< 标志位
    math::Vector4 color;            ///< 颜色 (RGBA)
    math::Vector3 acceleration;     ///< 加速度
    float rotation;                 ///< 旋转角度
    float angularVelocity;          ///< 角速度
    uint32_t seed;                  ///< 随机种子
    uint32_t padding[2];            ///< 对齐填充
    
    Particle() : lifetime(0), invLifetime(0), flags(0), 
                 color(1.0f), rotation(0), angularVelocity(0), seed(0) {
        position = math::Vector3(0.0f);
        velocity = math::Vector3(0.0f);
        size = math::Vector3(1.0f);
        acceleration = math::Vector3(0.0f);
    }
};

/**
 * @brief 粒子发射器配置
 */
struct ParticleEmitterConfig {
    // 发射参数
    EmitterShape shape{EmitterShape::Point};    ///< 发射形状
    math::Vector3 position{0.0f};               ///< 发射器位置
    math::Vector3 direction{0.0f, 1.0f, 0.0f};  ///< 发射方向
    float spreadAngle{0.0f};                    ///< 扩散角度（弧度）
    float tangentVelocity{0.0f};                ///< 切向速度
    
    // 形状参数
    float radius{1.0f};                         ///< 半径（球体/圆形/圆锥）
    math::Vector3 halfExtents{1.0f};            ///< 半范围（盒子）
    float coneAngle{0.5f};                      ///< 圆锥角度
    float coneHeight{1.0f};                     ///< 圆锥高度
    
    // 发射率
    float emissionRate{10.0f};                  ///< 每秒发射数量
    float burstCount{0.0f};                     ///< 爆发数量
    float burstInterval{0.0f};                  ///< 爆发间隔
    
    // 初始速度
    float minSpeed{1.0f};                       ///< 最小速度
    float maxSpeed{10.0f};                      ///< 最大速度
    math::Vector3 velocityOverLifetime{0.0f};   ///< 速度随寿命变化
    
    // 初始尺寸
    float minSize{0.1f};                        ///< 最小尺寸
    float maxSize{1.0f};                        ///< 最大尺寸
    math::Vector3 sizeOverLifetime{1.0f};       ///< 尺寸随寿命变化
    
    // 初始颜色
    math::Color minColor{1.0f};                 ///< 最小颜色
    math::Color maxColor{1.0f};                 ///< 最大颜色
    math::Color colorOverLifetime{1.0f};        ///< 颜色随寿命变化
    
    // 寿命
    float minLifetime{1.0f};                    ///< 最小寿命
    float maxLifetime{3.0f};                    ///< 最大寿命
    
    // 旋转
    float minRotation{0.0f};                    ///< 最小初始旋转
    float maxRotation{6.28f};                   ///< 最大初始旋转
    float minAngularVelocity{-1.0f};            ///< 最小角速度
    float maxAngularVelocity{1.0f};             ///< 最大角速度
    
    // 随机种子
    uint32_t randomSeed{0};                     ///< 随机种子
    
    ParticleEmitterConfig() = default;
};

/**
 * @brief 粒子力场
 */
struct ParticleForceField {
    enum class Type {
        Gravity,            ///< 重力
        Wind,               ///< 风力
        Vortex,             ///< 漩涡
        Attractor,          ///< 吸引点
        Repulsor,           ///< 排斥点
        Drag,               ///< 阻力
        Noise               ///< 噪声
    };
    
    Type type{Type::Gravity};
    math::Vector3 position{0.0f};           ///< 力场位置
    math::Vector3 direction{0.0f, 1.0f, 0.0f}; ///< 方向
    float strength{1.0f};                   ///< 强度
    float range{10.0f};                     ///< 影响范围
    float falloff{1.0f};                    ///< 衰减指数
    
    // 噪声参数
    float noiseScale{1.0f};                 ///< 噪声缩放
    float noiseSpeed{1.0f};                 ///< 噪声速度
    
    ParticleForceField() = default;
    ParticleForceField(Type t, const math::Vector3& dir, float s)
        : type(t), direction(dir), strength(s) {}
    
    // 预设力场
    static ParticleForceField gravity(float g = 9.81f) {
        return ParticleForceField(Type::Gravity, math::Vector3(0, -1, 0), g);
    }
    static ParticleForceField wind(const math::Vector3& dir, float s = 1.0f) {
        return ParticleForceField(Type::Wind, dir, s);
    }
    static ParticleForceField vortex(const math::Vector3& pos, float s = 1.0f) {
        ParticleForceField field(Type::Vortex, math::Vector3(0, 1, 0), s);
        field.position = pos;
        return field;
    }
};

/**
 * @brief 粒子碰撞配置
 */
struct ParticleCollisionConfig {
    bool enabled{false};                    ///< 是否启用碰撞
    float radius{0.1f};                     ///< 碰撞半径
    float damping{0.5f};                    ///< 碰撞阻尼
    float bounce{0.3f};                     ///< 反弹系数
    bool collideWithWorld{true};            ///< 与世界碰撞
    uint32_t collisionLayers{0xFFFFFFFF};   ///< 碰撞层
    
    // 碰撞响应
    enum class Response {
        Bounce,         ///< 反弹
        Stick,          ///< 粘附
        Kill            ///< 销毁
    };
    
    Response response{Response::Bounce};
};

/**
 * @brief 粒子子系统组件
 */
struct ParticleSystemComponent {
    std::shared_ptr<class ParticleSystem> system;   ///< 粒子系统
    bool playing{true};                             ///< 是否播放
    bool looping{true};                             ///< 是否循环
    float timeScale{1.0f};                          ///< 时间缩放
    uint32_t maxParticles{10000};                   ///< 最大粒子数
    ParticleRenderMode renderMode{ParticleRenderMode::Billboard};
    ParticleCollisionConfig collision;              ///< 碰撞配置
    
    ParticleSystemComponent() = default;
    explicit ParticleSystemComponent(std::shared_ptr<class ParticleSystem> sys)
        : system(std::move(sys)) {}
};

// ============================================================================
// 粒子系统
// ============================================================================

/**
 * @brief GPU 粒子计算着色器数据
 */
struct GPUParticleData {
    uint32_t particleCount;             ///< 粒子数量
    uint32_t maxParticles;              ///< 最大粒子数
    float deltaTime;                    ///< 时间增量
    float simulationTime;               ///< 模拟时间
    math::Vector3 gravity;              ///< 重力
    uint32_t emitterCount;              ///< 发射器数量
    uint32_t forceFieldCount;           ///< 力场数量
    uint32_t padding[2];                ///< 对齐
};

/**
 * @brief 粒子系统
 * 
 * 支持 GPU 加速的粒子模拟
 */
class ParticleSystem {
public:
    ParticleSystem();
    ~ParticleSystem();
    
    // ========================================================================
    // 初始化
    // ========================================================================
    
    /**
     * @brief 初始化粒子系统
     * @param maxParticles 最大粒子数
     * @param useGPU 是否使用 GPU 计算
     */
    bool initialize(size_t maxParticles = 10000, bool useGPU = true);
    
    /**
     * @brief 关闭粒子系统
     */
    void shutdown();
    
    /**
     * @brief 是否已初始化
     */
    bool isInitialized() const noexcept { return initialized_; }
    
    /**
     * @brief 是否使用 GPU
     */
    bool isUsingGPU() const noexcept { return useGPU_; }
    
    // ========================================================================
    // 发射器管理
    // ========================================================================
    
    /**
     * @brief 添加发射器
     */
    uint32_t addEmitter(const ParticleEmitterConfig& config);
    
    /**
     * @brief 移除发射器
     */
    void removeEmitter(uint32_t emitterIndex);
    
    /**
     * @brief 获取发射器
     */
    ParticleEmitterConfig* getEmitter(uint32_t emitterIndex);
    const ParticleEmitterConfig* getEmitter(uint32_t emitterIndex) const;
    
    /**
     * @brief 获取发射器数量
     */
    size_t emitterCount() const noexcept { return emitters_.size(); }
    
    // ========================================================================
    // 力场管理
    // ========================================================================
    
    /**
     * @brief 添加力场
     */
    uint32_t addForceField(const ParticleForceField& field);
    
    /**
     * @brief 移除力场
     */
    void removeForceField(uint32_t fieldIndex);
    
    /**
     * @brief 获取力场
     */
    ParticleForceField* getForceField(uint32_t fieldIndex);
    const ParticleForceField* getForceField(uint32_t fieldIndex) const;
    
    /**
     * @brief 获取力场数量
     */
    size_t forceFieldCount() const noexcept { return forceFields_.size(); }
    
    // ========================================================================
    // 播放控制
    // ========================================================================
    
    /**
     * @brief 播放
     */
    void play();
    
    /**
     * @brief 暂停
     */
    void pause();
    
    /**
     * @brief 停止
     */
    void stop();
    
    /**
     * @brief 触发爆发
     */
    void burst(uint32_t emitterIndex, uint32_t count);
    
    /**
     * @brief 清除所有粒子
     */
    void clear();
    
    /**
     * @brief 是否正在播放
     */
    bool isPlaying() const noexcept { return playing_; }
    
    // ========================================================================
    // 更新
    // ========================================================================
    
    /**
     * @brief 更新粒子系统
     * @param deltaTime 时间增量
     */
    void update(float deltaTime);
    
    /**
     * @brief 获取活跃粒子数量
     */
    size_t activeParticleCount() const noexcept { return activeParticleCount_; }
    
    /**
     * @brief 获取所有粒子
     */
    const std::vector<Particle>& particles() const noexcept { return particles_; }
    
    // ========================================================================
    // 渲染数据
    // ========================================================================
    
    /**
     * @brief 获取用于渲染的粒子数据
     */
    const std::vector<Particle>& renderParticles() const noexcept { return particles_; }
    
    /**
     * @brief 获取 GPU 数据
     */
    const GPUParticleData& gpuData() const noexcept { return gpuData_; }
    
    /**
     * @brief 获取粒子位置缓冲区（GPU）
     */
    const void* gpuPositionBuffer() const { return gpuPositionBuffer_; }
    
    /**
     * @brief 获取粒子速度缓冲区（GPU）
     */
    const void* gpuVelocityBuffer() const { return gpuVelocityBuffer_; }
    
    // ========================================================================
    // 碰撞
    // ========================================================================
    
    /**
     * @brief 设置碰撞配置
     */
    void setCollisionConfig(const ParticleCollisionConfig& config);
    
    /**
     * @brief 获取碰撞配置
     */
    const ParticleCollisionConfig& collisionConfig() const noexcept { return collisionConfig_; }
    
    // ========================================================================
    // 工具方法
    // ========================================================================
    
    /**
     * @brief 获取内存使用量
     */
    size_t memoryUsage() const noexcept;
    
    /**
     * @brief 设置渲染模式
     */
    void setRenderMode(ParticleRenderMode mode);
    
    /**
     * @brief 获取渲染模式
     */
    ParticleRenderMode renderMode() const noexcept { return renderMode_; }
    
private:
    bool initialized_{false};
    bool useGPU_{false};
    bool playing_{true};
    
    std::vector<Particle> particles_;           ///< 粒子数组
    std::vector<ParticleEmitterConfig> emitters_; ///< 发射器
    std::vector<ParticleForceField> forceFields_; ///< 力场
    
    size_t activeParticleCount_{0};             ///< 活跃粒子数
    size_t maxParticles_{10000};                ///< 最大粒子数
    float simulationTime_{0.0f};                ///< 模拟时间
    
    ParticleCollisionConfig collisionConfig_;   ///< 碰撞配置
    ParticleRenderMode renderMode_{ParticleRenderMode::Billboard};
    
    // GPU 资源
    void* gpuPositionBuffer_{nullptr};          ///< GPU 位置缓冲
    void* gpuVelocityBuffer_{nullptr};          ///< GPU 速度缓冲
    void* gpuParticleBuffer_{nullptr};          ///< GPU 粒子缓冲
    void* computePipeline_{nullptr};            ///< 计算管线
    GPUParticleData gpuData_;                   ///< GPU 数据
    
    /**
     * @brief CPU 更新粒子
     */
    void updateCPU(float deltaTime);
    
    /**
     * @brief GPU 更新粒子
     */
    void updateGPU(float deltaTime);
    
    /**
     * @brief 发射新粒子
     */
    void emitParticles(uint32_t emitterIndex, float deltaTime);
    
    /**
     * @brief 应用力场
     */
    void applyForceFields(Particle& particle, float deltaTime);
    
    /**
     * @brief 处理碰撞
     */
    void handleCollision(Particle& particle, float deltaTime);
    
    /**
     * @brief 初始化 GPU 资源
     */
    bool initializeGPU();
    
    /**
     * @brief 销毁 GPU 资源
     */
    void destroyGPU();
};

} // namespace scene
} // namespace phoenix
