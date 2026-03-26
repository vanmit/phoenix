#include "phoenix/scene/animator.hpp"
#include "phoenix/scene/animation_types.hpp"
#include <algorithm>
#include <cmath>

namespace phoenix {
namespace scene {

// ============================================================================
// AnimationStateMachine 实现
// ============================================================================

uint32_t AnimationStateMachine::addState(const std::string& name, uint32_t clipIndex) {
    StateMachineState state(name, clipIndex);
    states_.push_back(state);
    return static_cast<uint32_t>(states_.size() - 1);
}

void AnimationStateMachine::addTransition(uint32_t fromState, uint32_t toState,
                                           float blendDuration,
                                           std::function<bool()> condition,
                                           BlendMode blendMode) {
    StateMachineTransition transition;
    transition.fromState = fromState;
    transition.toState = toState;
    transition.blendDuration = blendDuration;
    transition.condition = std::move(condition);
    transition.blendMode = blendMode;
    transitions_.push_back(transition);
}

void AnimationStateMachine::setCurrentState(uint32_t stateIndex) {
    if (stateIndex < states_.size()) {
        currentState_ = stateIndex;
    }
}

const StateMachineState* AnimationStateMachine::getState(uint32_t index) const noexcept {
    if (index < states_.size()) {
        return &states_[index];
    }
    return nullptr;
}

std::vector<AnimationState> AnimationStateMachine::update(float deltaTime) {
    std::vector<AnimationState> result;
    
    if (states_.empty()) {
        return result;
    }
    
    // 检查状态转换
    for (const auto& transition : transitions_) {
        if (transition.fromState == currentState_ && transition.condition) {
            if (transition.condition()) {
                // 触发转换
                BlendState blend;
                blend.sourceState = currentState_;
                blend.targetState = transition.toState;
                blend.blendFactor = 0.0f;
                blend.blendDuration = transition.blendDuration;
                blend.blendMode = transition.blendMode;
                activeBlends_.push_back(blend);
                
                currentState_ = transition.toState;
                break;
            }
        }
    }
    
    // 更新混合
    for (auto it = activeBlends_.begin(); it != activeBlends_.end();) {
        it->blendFactor += deltaTime / it->blendDuration;
        
        if (it->blendFactor >= 1.0f) {
            it = activeBlends_.erase(it);
        } else {
            ++it;
        }
    }
    
    // 构建激活的动画状态
    AnimationState mainState;
    mainState.clipIndex = states_[currentState_].clipIndex;
    mainState.weight = 1.0f;
    mainState.playing = true;
    result.push_back(mainState);
    
    // 添加混合状态
    for (const auto& blend : activeBlends_) {
        AnimationState blendState;
        blendState.clipIndex = states_[blend.targetState].clipIndex;
        blendState.weight = blend.blendFactor;
        blendState.playing = true;
        result.push_back(blendState);
    }
    
    return result;
}

void AnimationStateMachine::reset() {
    currentState_ = 0;
    activeBlends_.clear();
}

// ============================================================================
// AnimationBlender 实现
// ============================================================================

void AnimationBlender::addState(const AnimationState& state) {
    states_.push_back(state);
}

void AnimationBlender::clear() {
    states_.clear();
    blends_.clear();
}

void AnimationBlender::blendTransforms(math::Vector3& outPos, math::Quaternion& outRot, math::Vector3& outScale,
                                        const math::Vector3& posA, const math::Quaternion& rotA, const math::Vector3& scaleA, float weightA,
                                        const math::Vector3& posB, const math::Quaternion& rotB, const math::Vector3& scaleB, float weightB,
                                        BlendMode mode) {
    switch (mode) {
        case BlendMode::Override: {
            // 线性插值
            float t = weightB / (weightA + weightB + 1e-6f);
            outPos = math::Vector3::lerp(posA, posB, t);
            outRot = math::Quaternion::slerp(rotA, rotB, t);
            outScale = math::Vector3::lerp(scaleA, scaleB, t);
            break;
        }
        
        case BlendMode::Additive: {
            // 加法混合
            outPos = posA + (posB - math::Vector3(0.0f)) * weightB;
            
            // 旋转的加法混合：使用相对旋转
            math::Quaternion deltaRot = rotB * rotA.conjugated();
            outRot = deltaRot * rotA;
            
            outScale = math::Vector3(
                scaleA.x * (1.0f + (scaleB.x - 1.0f) * weightB),
                scaleA.y * (1.0f + (scaleB.y - 1.0f) * weightB),
                scaleA.z * (1.0f + (scaleB.z - 1.0f) * weightB)
            );
            break;
        }
        
        case BlendMode::Multiply: {
            // 乘法混合
            outPos = math::Vector3(posA.x * posB.x, posA.y * posB.y, posA.z * posB.z);
            
            // 旋转相乘
            outRot = rotA * rotB;
            
            outScale = math::Vector3(
                scaleA.x * scaleB.x,
                scaleA.y * scaleB.y,
                scaleA.z * scaleB.z
            );
            break;
        }
        
        case BlendMode::Average: {
            // 平均混合
            float totalWeight = weightA + weightB;
            float invTotal = 1.0f / (totalWeight + 1e-6f);
            
            outPos = math::Vector3::lerp(posA, posB, weightB * invTotal);
            outRot = math::Quaternion::slerp(rotA, rotB, weightB * invTotal);
            outScale = math::Vector3::lerp(scaleA, scaleB, weightB * invTotal);
            break;
        }
    }
}

math::Vector3 AnimationBlender::sampleChannel(const AnimationClip::Channel& channel, float time) {
    if (channel.times.empty()) {
        return math::Vector3(0.0f);
    }
    
    if (time <= channel.times.front()) {
        return channel.values[0];
    }
    
    if (time >= channel.times.back()) {
        return channel.values.back();
    }
    
    // 查找关键帧区间
    size_t index = 0;
    for (size_t i = 0; i < channel.times.size() - 1; ++i) {
        if (channel.times[i] <= time && channel.times[i + 1] > time) {
            index = i;
            break;
        }
    }
    
    // 插值
    float t0 = channel.times[index];
    float t1 = channel.times[index + 1];
    float t = (time - t0) / (t1 - t0 + 1e-6f);
    
    switch (channel.interpolation) {
        case InterpolationType::Constant:
            return channel.values[index];
            
        case InterpolationType::Linear: {
            return math::Vector3::lerp(channel.values[index], channel.values[index + 1], t);
        }
        
        case InterpolationType::CubicSpline: {
            // 三次样条插值（简化版）
            const math::Vector3& p0 = channel.values[index];
            const math::Vector3& p1 = channel.values[index + 1];
            const math::Vector3& m0 = channel.outTangents[index];
            const math::Vector3& m1 = channel.inTangents[index + 1];
            
            float t2 = t * t;
            float t3 = t2 * t;
            
            float h00 = 2 * t3 - 3 * t2 + 1;
            float h10 = t3 - 2 * t2 + t;
            float h01 = -2 * t3 + 3 * t2;
            float h11 = t3 - t2;
            
            return p0 * h00 + m0 * h10 + p1 * h01 + m1 * h11;
        }
    }
    
    return math::Vector3(0.0f);
}

math::Quaternion AnimationBlender::sampleRotationChannel(const AnimationClip::Channel& channel, float time) {
    if (channel.times.empty()) {
        return math::Quaternion::identity();
    }
    
    if (time <= channel.times.front()) {
        return channel.rotations[0];
    }
    
    if (time >= channel.times.back()) {
        return channel.rotations.back();
    }
    
    // 查找关键帧区间
    size_t index = 0;
    for (size_t i = 0; i < channel.times.size() - 1; ++i) {
        if (channel.times[i] <= time && channel.times[i + 1] > time) {
            index = i;
            break;
        }
    }
    
    // 球面线性插值
    float t0 = channel.times[index];
    float t1 = channel.times[index + 1];
    float t = (time - t0) / (t1 - t0 + 1e-6f);
    
    if (channel.interpolation == InterpolationType::Constant) {
        return channel.rotations[index];
    }
    
    return math::Quaternion::slerp(channel.rotations[index], channel.rotations[index + 1], t);
}

void AnimationBlender::apply(Skeleton& skeleton, float deltaTime) {
    if (states_.empty() || !skeleton.boneCount()) {
        return;
    }
    
    // 对每个骨骼进行混合
    for (size_t boneIndex = 0; boneIndex < skeleton.boneCount(); ++boneIndex) {
        math::Vector3 blendedPos(0.0f);
        math::Quaternion blendedRot = math::Quaternion::identity();
        math::Vector3 blendedScale(1.0f);
        float totalWeight = 0.0f;
        
        // 收集所有状态的贡献
        for (const auto& state : states_) {
            if (!state.playing || state.clipIndex == UINT32_MAX) {
                continue;
            }
            
            // 获取骨骼的局部变换
            math::Vector3 pos(0.0f);
            math::Quaternion rot = math::Quaternion::identity();
            math::Vector3 scale(1.0f);
            
            // TODO: 从动画剪辑采样
            
            blendedPos = blendedPos * totalWeight + pos * state.weight;
            // 旋转混合需要特殊处理
            blendedRot = math::Quaternion::slerp(blendedRot, rot, state.weight / (totalWeight + state.weight));
            blendedScale = math::Vector3::lerp(blendedScale, scale, state.weight / (totalWeight + state.weight));
            
            totalWeight += state.weight;
        }
        
        if (totalWeight > 0.0f) {
            skeleton.updateBonePose(static_cast<uint32_t>(boneIndex), blendedPos, blendedRot, blendedScale);
        }
    }
    
    // 计算最终矩阵
    skeleton.calculateFinalMatrices();
}

void AnimationBlender::calculateMorphWeights(std::vector<float>& morphWeights) const {
    // 收集所有状态的形变权重
    for (const auto& state : states_) {
        if (!state.playing) {
            continue;
        }
        
        // TODO: 从动画剪辑获取形变权重并混合
    }
}

// ============================================================================
// Animator 实现
// ============================================================================

void Animator::setSkeleton(std::shared_ptr<Skeleton> skeleton) {
    skeleton_ = std::move(skeleton);
}

uint32_t Animator::addClip(std::shared_ptr<AnimationClip> clip) {
    clips_.push_back(std::move(clip));
    return static_cast<uint32_t>(clips_.size() - 1);
}

const std::shared_ptr<AnimationClip>& Animator::getClip(uint32_t index) const noexcept {
    static const std::shared_ptr<AnimationClip> empty;
    if (index < clips_.size()) {
        return clips_[index];
    }
    return empty;
}

void Animator::play(uint32_t clipIndex, float fadeDuration, LoopMode loopMode) {
    if (clipIndex >= clips_.size()) {
        return;
    }
    
    currentClipIndex_ = clipIndex;
    currentTime_ = 0.0f;
    playing_ = true;
    paused_ = false;
    
    // 创建动画状态
    AnimationState state;
    state.clipIndex = clipIndex;
    state.time = 0.0f;
    state.weight = 0.0f;
    state.speed = speed_;
    state.loopMode = loopMode;
    state.playing = true;
    state.fadeTime = 0.0f;
    state.fadeDuration = fadeDuration;
    
    states_.clear();
    states_.push_back(state);
}

void Animator::playByName(const std::string& clipName, float fadeDuration, LoopMode loopMode) {
    for (size_t i = 0; i < clips_.size(); ++i) {
        if (clips_[i]->name == clipName) {
            play(static_cast<uint32_t>(i), fadeDuration, loopMode);
            return;
        }
    }
}

void Animator::stop() {
    playing_ = false;
    states_.clear();
    blends_.clear();
}

void Animator::stop(uint32_t clipIndex) {
    auto it = std::find_if(states_.begin(), states_.end(),
                           [clipIndex](const AnimationState& s) { return s.clipIndex == clipIndex; });
    if (it != states_.end()) {
        states_.erase(it);
        if (states_.empty()) {
            playing_ = false;
        }
    }
}

void Animator::setPaused(bool paused) {
    paused_ = paused;
}

void Animator::setSpeed(float speed) {
    speed_ = std::max(0.0f, speed);
    for (auto& state : states_) {
        state.speed = speed_;
    }
}

void Animator::blend(uint32_t clipIndexA, uint32_t clipIndexB, float blendFactor, float blendDuration) {
    if (clipIndexA >= clips_.size() || clipIndexB >= clips_.size()) {
        return;
    }
    
    BlendState blend;
    blend.sourceState = clipIndexA;
    blend.targetState = clipIndexB;
    blend.blendFactor = blendFactor;
    blend.blendDuration = blendDuration;
    blend.blendMode = BlendMode::Override;
    
    blends_.push_back(blend);
}

void Animator::addLayer(uint32_t clipIndex, float weight, bool additive) {
    if (clipIndex >= clips_.size()) {
        return;
    }
    
    AnimationState state;
    state.clipIndex = clipIndex;
    state.weight = weight;
    state.speed = speed_;
    state.loopMode = LoopMode::Loop;
    state.playing = true;
    
    states_.push_back(state);
}

void Animator::setLayerWeight(size_t layerIndex, float weight) {
    if (layerIndex < states_.size()) {
        states_[layerIndex].weight = weight;
    }
}

void Animator::setRootMotionEnabled(bool enabled) {
    rootMotion_.enabled = enabled;
}

void Animator::applyRootMotion(math::Vector3& position, math::Quaternion& rotation, float deltaTime) {
    if (!rootMotion_.enabled || states_.empty()) {
        return;
    }
    
    // 从当前动画提取根运动
    if (currentClipIndex_ < clips_.size()) {
        const auto& clip = clips_[currentClipIndex_];
        
        // 采样根运动曲线
        // TODO: 实现根运动采样
        
        // 应用位置增量
        position = position + rootMotion_.positionDelta * deltaTime;
        
        // 应用旋转增量
        rotation = rootMotion_.rotationDelta * rotation;
    }
}

void Animator::setMorphWeight(uint32_t morphIndex, float weight) {
    if (morphIndex >= morphWeights_.size()) {
        morphWeights_.resize(morphIndex + 1, 0.0f);
    }
    morphWeights_[morphIndex] = std::clamp(weight, 0.0f, 1.0f);
}

float Animator::morphWeight(uint32_t morphIndex) const {
    if (morphIndex < morphWeights_.size()) {
        return morphWeights_[morphIndex];
    }
    return 0.0f;
}

void Animator::update(float deltaTime) {
    if (!playing_ || paused_) {
        return;
    }
    
    // 更新状态机
    updateStateMachine(deltaTime);
    
    // 更新动画状态
    updateAnimationStates(deltaTime);
    
    // 应用动画到骨骼
    applyToSkeleton();
}

void Animator::updateAnimationStates(float deltaTime) {
    for (auto& state : states_) {
        if (!state.playing) {
            continue;
        }
        
        // 更新淡入淡出
        if (state.fadeTime < state.fadeDuration) {
            state.fadeTime += deltaTime;
            state.weight = std::min(1.0f, state.fadeTime / state.fadeDuration);
        }
        
        // 更新时间
        state.time += deltaTime * state.speed;
        
        // 处理循环
        if (state.clipIndex < clips_.size()) {
            float duration = clips_[state.clipIndex]->duration;
            
            switch (state.loopMode) {
                case LoopMode::Once:
                    if (state.time >= duration) {
                        state.time = duration;
                        state.playing = false;
                    }
                    break;
                    
                case LoopMode::Loop:
                    while (state.time >= duration) {
                        state.time -= duration;
                    }
                    break;
                    
                case LoopMode::PingPong:
                    // TODO: 实现来回播放
                    break;
                    
                case LoopMode::Clamp:
                    state.time = std::min(state.time, duration);
                    break;
            }
        }
    }
    
    // 移除完成的混合
    blends_.erase(std::remove_if(blends_.begin(), blends_.end(),
                                  [](const BlendState& b) { return b.blendFactor >= 1.0f; }),
                   blends_.end());
}

void Animator::updateStateMachine(float deltaTime) {
    if (stateMachine_.stateCount() > 0) {
        auto smStates = stateMachine_.update(deltaTime);
        // 合并状态机状态和手动状态
    }
}

void Animator::applyToSkeleton() {
    if (!skeleton_) {
        return;
    }
    
    // 使用混合器应用动画
    blender_.clear();
    for (const auto& state : states_) {
        blender_.addState(state);
    }
    
    blender_.apply(*skeleton_, 0.0f);
}

float Animator::currentDuration() const {
    if (currentClipIndex_ < clips_.size()) {
        return clips_[currentClipIndex_]->duration;
    }
    return 0.0f;
}

math::Vector3 Animator::samplePosition(const AnimationClip& clip, uint32_t channelIndex, float time) const {
    if (channelIndex >= clip.channels.size()) {
        return math::Vector3(0.0f);
    }
    
    const auto& channel = clip.channels[channelIndex];
    return AnimationBlender::sampleChannel(channel, time);
}

math::Quaternion Animator::sampleRotation(const AnimationClip& clip, uint32_t channelIndex, float time) const {
    if (channelIndex >= clip.channels.size()) {
        return math::Quaternion::identity();
    }
    
    const auto& channel = clip.channels[channelIndex];
    return AnimationBlender::sampleRotationChannel(channel, time);
}

math::Vector3 Animator::sampleScale(const AnimationClip& clip, uint32_t channelIndex, float time) const {
    return samplePosition(clip, channelIndex, time);
}

} // namespace scene
} // namespace phoenix
