#pragma once

#include "animation_types.hpp"
#include "skeleton.hpp"
#include <vector>
#include <string>
#include <functional>
#include <memory>

namespace phoenix {
namespace scene {

// 前向声明
class AnimationClip;
class Skeleton;

/**
 * @brief 动画混合状态
 */
struct BlendState {
    uint32_t sourceState{0};                        ///< 源状态索引
    uint32_t targetState{1};                        ///< 目标状态索引
    float blendFactor{0.0f};                        ///< 混合因子 0-1
    float blendDuration{0.25f};                     ///< 混合持续时间
    BlendMode blendMode{BlendMode::Override};       ///< 混合模式
    
    BlendState() = default;
    BlendState(uint32_t src, uint32_t tgt, float duration, BlendMode mode = BlendMode::Override)
        : sourceState(src), targetState(tgt), blendFactor(0.0f), 
          blendDuration(duration), blendMode(mode) {}
};

/**
 * @brief 动画状态机状态
 */
struct StateMachineState {
    std::string name;                               ///< 状态名称
    uint32_t clipIndex{UINT32_MAX};                 ///< 关联的动画剪辑
    std::vector<uint32_t> transitions;              ///< 可转移到的状态
    std::function<bool()> exitCondition;            ///< 退出条件
    
    StateMachineState() = default;
    StateMachineState(const std::string& n, uint32_t clip) 
        : name(n), clipIndex(clip) {}
};

/**
 * @brief 动画状态机转换
 */
struct StateMachineTransition {
    uint32_t fromState{0};                          ///< 源状态
    uint32_t toState{0};                            ///< 目标状态
    float blendDuration{0.25f};                     ///< 混合时间
    std::function<bool()> condition;                ///< 触发条件
    BlendMode blendMode{BlendMode::Override};       ///< 混合模式
    
    StateMachineTransition() = default;
    StateMachineTransition(uint32_t f, uint32_t t, float duration)
        : fromState(f), toState(t), blendDuration(duration) {}
};

/**
 * @brief 动画状态机
 * 
 * 管理动画状态和状态转换
 */
class AnimationStateMachine {
public:
    AnimationStateMachine() = default;
    ~AnimationStateMachine() = default;
    
    // ========================================================================
    // 状态管理
    // ========================================================================
    
    /**
     * @brief 添加状态
     */
    uint32_t addState(const std::string& name, uint32_t clipIndex);
    
    /**
     * @brief 添加状态转换
     */
    void addTransition(uint32_t fromState, uint32_t toState, 
                       float blendDuration,
                       std::function<bool()> condition,
                       BlendMode blendMode = BlendMode::Override);
    
    /**
     * @brief 设置当前状态
     */
    void setCurrentState(uint32_t stateIndex);
    
    /**
     * @brief 获取当前状态
     */
    uint32_t currentState() const noexcept { return currentState_; }
    
    /**
     * @brief 获取状态数量
     */
    size_t stateCount() const noexcept { return states_.size(); }
    
    /**
     * @brief 获取状态
     */
    const StateMachineState* getState(uint32_t index) const noexcept;
    
    // ========================================================================
    // 更新
    // ========================================================================
    
    /**
     * @brief 更新状态机
     * @param deltaTime 时间增量
     * @return 当前激活的动画状态
     */
    std::vector<AnimationState> update(float deltaTime);
    
    /**
     * @brief 重置状态机
     */
    void reset();
    
private:
    std::vector<StateMachineState> states_;         ///< 状态列表
    std::vector<StateMachineTransition> transitions_; ///< 转换列表
    uint32_t currentState_{0};                      ///< 当前状态
    std::vector<BlendState> activeBlends_;          ///< 激活的混合
};

/**
 * @brief 动画混合器
 * 
 * 处理多个动画的混合计算
 */
class AnimationBlender {
public:
    AnimationBlender() = default;
    ~AnimationBlender() = default;
    
    /**
     * @brief 添加动画状态进行混合
     */
    void addState(const AnimationState& state);
    
    /**
     * @brief 清除所有状态
     */
    void clear();
    
    /**
     * @brief 计算混合后的骨骼变换
     * @param skeleton 目标骨骼
     * @param deltaTime 时间增量
     */
    void apply(Skeleton& skeleton, float deltaTime);
    
    /**
     * @brief 计算混合后的形变权重
     * @param morphWeights 输出形变权重
     */
    void calculateMorphWeights(std::vector<float>& morphWeights) const;
    
    /**
     * @brief 获取激活的动画状态
     */
    const std::vector<AnimationState>& activeStates() const noexcept { return states_; }
    
private:
    std::vector<AnimationState> states_;            ///< 激活的动画状态
    std::vector<BlendState> blends_;                ///< 混合状态
    
    /**
     * @brief 对两个变换进行混合
     */
    static void blendTransforms(math::Vector3& outPos, math::Quaternion& outRot, math::Vector3& outScale,
                                const math::Vector3& posA, const math::Quaternion& rotA, const math::Vector3& scaleA, float weightA,
                                const math::Vector3& posB, const math::Quaternion& rotB, const math::Vector3& scaleB, float weightB,
                                BlendMode mode);
    
    /**
     * @brief 采样动画通道
     */
    static math::Vector3 sampleChannel(const AnimationClip::Channel& channel, float time);
    static math::Quaternion sampleRotationChannel(const AnimationClip::Channel& channel, float time);
};

/**
 * @brief 动画师组件
 * 
 * 管理实体的动画播放
 */
class Animator {
public:
    Animator() = default;
    ~Animator() = default;
    
    // ========================================================================
    // 初始化
    // ========================================================================
    
    /**
     * @brief 设置骨骼
     */
    void setSkeleton(std::shared_ptr<Skeleton> skeleton);
    
    /**
     * @brief 获取骨骼
     */
    std::shared_ptr<Skeleton> skeleton() const noexcept { return skeleton_; }
    
    /**
     * @brief 添加动画剪辑
     */
    uint32_t addClip(std::shared_ptr<AnimationClip> clip);
    
    /**
     * @brief 获取剪辑数量
     */
    size_t clipCount() const noexcept { return clips_.size(); }
    
    /**
     * @brief 获取剪辑
     */
    const std::shared_ptr<AnimationClip>& getClip(uint32_t index) const noexcept;
    
    // ========================================================================
    // 动画播放控制
    // ========================================================================
    
    /**
     * @brief 播放动画
     * @param clipIndex 剪辑索引
     * @param fadeDuration 淡入时间
     * @param loopMode 循环模式
     */
    void play(uint32_t clipIndex, float fadeDuration = 0.25f, 
              LoopMode loopMode = LoopMode::Loop);
    
    /**
     * @brief 通过名称播放动画
     */
    void playByName(const std::string& clipName, float fadeDuration = 0.25f,
                    LoopMode loopMode = LoopMode::Loop);
    
    /**
     * @brief 停止所有动画
     */
    void stop();
    
    /**
     * @brief 停止指定动画
     */
    void stop(uint32_t clipIndex);
    
    /**
     * @brief 暂停/继续动画
     */
    void setPaused(bool paused);
    bool isPaused() const noexcept { return paused_; }
    
    /**
     * @brief 设置动画速度
     */
    void setSpeed(float speed);
    float speed() const noexcept { return speed_; }
    
    // ========================================================================
    // 动画混合
    // ========================================================================
    
    /**
     * @brief 混合两个动画
     * @param clipIndexA 第一个剪辑
     * @param clipIndexB 第二个剪辑
     * @param blendFactor 混合因子（0-1）
     * @param blendDuration 混合时间
     */
    void blend(uint32_t clipIndexA, uint32_t clipIndexB, 
               float blendFactor, float blendDuration = 0.25f);
    
    /**
     * @brief 添加动画层
     * @param clipIndex 剪辑索引
     * @param weight 权重
     * @param additive 是否加法混合
     */
    void addLayer(uint32_t clipIndex, float weight = 1.0f, bool additive = false);
    
    /**
     * @brief 设置层的权重
     */
    void setLayerWeight(size_t layerIndex, float weight);
    
    // ========================================================================
    // 动画状态机
    // ========================================================================
    
    /**
     * @brief 获取状态机
     */
    AnimationStateMachine& stateMachine() noexcept { return stateMachine_; }
    const AnimationStateMachine& stateMachine() const noexcept { return stateMachine_; }
    
    // ========================================================================
    // 根运动
    // ========================================================================
    
    /**
     * @brief 启用/禁用根运动
     */
    void setRootMotionEnabled(bool enabled);
    bool isRootMotionEnabled() const noexcept { return rootMotion_.enabled; }
    
    /**
     * @brief 获取根运动数据
     */
    const RootMotion& rootMotion() const noexcept { return rootMotion_; }
    
    /**
     * @brief 应用根运动到变换
     * @param position 当前位置
     * @param rotation 当前旋转
     * @param deltaTime 时间增量
     */
    void applyRootMotion(math::Vector3& position, math::Quaternion& rotation, float deltaTime);
    
    // ========================================================================
    // 形变动画
    // ========================================================================
    
    /**
     * @brief 设置形变目标权重
     */
    void setMorphWeight(uint32_t morphIndex, float weight);
    
    /**
     * @brief 获取形变权重
     */
    float morphWeight(uint32_t morphIndex) const;
    
    /**
     * @brief 获取所有形变权重
     */
    const std::vector<float>& morphWeights() const noexcept { return morphWeights_; }
    
    // ========================================================================
    // 更新
    // ========================================================================
    
    /**
     * @brief 更新动画
     * @param deltaTime 时间增量
     */
    void update(float deltaTime);
    
    /**
     * @brief 应用动画到骨骼
     */
    void applyToSkeleton();
    
    /**
     * @brief 获取当前动画时间
     */
    float currentTime() const noexcept { return currentTime_; }
    
    /**
     * @brief 获取当前动画时长
     */
    float currentDuration() const noexcept;
    
    /**
     * @brief 获取当前播放的动画索引
     */
    uint32_t currentClipIndex() const noexcept { return currentClipIndex_; }
    
    /**
     * @brief 是否正在播放
     */
    bool isPlaying() const noexcept { return playing_; }
    
private:
    std::shared_ptr<Skeleton> skeleton_;              ///< 骨骼
    std::vector<std::shared_ptr<AnimationClip>> clips_; ///< 动画剪辑
    AnimationStateMachine stateMachine_;              ///< 状态机
    AnimationBlender blender_;                        ///< 混合器
    
    std::vector<AnimationState> states_;              ///< 动画状态
    std::vector<BlendState> blends_;                  ///< 混合状态
    std::vector<float> morphWeights_;                 ///< 形变权重
    
    uint32_t currentClipIndex_{UINT32_MAX};           ///< 当前剪辑索引
    float currentTime_{0.0f};                         ///< 当前时间
    float speed_{1.0f};                               ///< 播放速度
    bool playing_{false};                             ///< 是否播放中
    bool paused_{false};                              ///< 是否暂停
    
    RootMotion rootMotion_;                           ///< 根运动
    
    /**
     * @brief 采样动画通道
     */
    math::Vector3 samplePosition(const AnimationClip& clip, uint32_t channelIndex, float time) const;
    math::Quaternion sampleRotation(const AnimationClip& clip, uint32_t channelIndex, float time) const;
    math::Vector3 sampleScale(const AnimationClip& clip, uint32_t channelIndex, float time) const;
    
    /**
     * @brief 更新动画状态
     */
    void updateAnimationStates(float deltaTime);
    
    /**
     * @brief 处理状态机转换
     */
    void updateStateMachine(float deltaTime);
};

} // namespace scene
} // namespace phoenix
