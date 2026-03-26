#pragma once

#include "../math/vector3.hpp"
#include "../math/quaternion.hpp"
#include "../math/matrix4.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include <memory>

namespace phoenix {
namespace scene {

// ============================================================================
// 动画基础类型定义
// ============================================================================

/**
 * @brief 动画插值类型
 */
enum class InterpolationType : uint8_t {
    Constant,    ///< 恒定值（无插值）
    Linear,      ///< 线性插值
    CubicSpline  ///< 三次样条插值
};

/**
 * @brief 动画通道类型
 */
enum class AnimationChannelType : uint8_t {
    Translation,  ///< 位置通道
    Rotation,     ///< 旋转通道
    Scale,        ///< 缩放通道
    MorphWeight,  ///< 形变权重通道
    Color         ///< 颜色通道
};

/**
 * @brief 动画混合模式
 */
enum class BlendMode : uint8_t {
    Override,     ///< 覆盖（完全替换）
    Additive,     ///< 加法混合
    Multiply,     ///< 乘法混合
    Average       ///< 平均混合
};

/**
 * @brief 动画循环模式
 */
enum class LoopMode : uint8_t {
    Once,         ///< 播放一次
    Loop,         ///< 循环播放
    PingPong,     ///< 来回播放
    Clamp         ///< 钳制到最后一帧
};

/**
 * @brief 关键帧数据结构
 */
struct Keyframe {
    float time;                          ///< 时间点（秒）
    math::Vector3 position{0.0f};        ///< 位置
    math::Quaternion rotation{1.0f, 0.0f, 0.0f, 0.0f};  ///< 旋转
    math::Vector3 scale{1.0f};           ///< 缩放
    math::Vector3 inTangent{0.0f};       ///< 输入切线（样条插值）
    math::Vector3 outTangent{0.0f};      ///< 输出切线（样条插值）
    
    Keyframe() = default;
    Keyframe(float t, const math::Vector3& pos, const math::Quaternion& rot, const math::Vector3& scl)
        : time(t), position(pos), rotation(rot), scale(scl) {}
};

/**
 * @brief 骨骼数据
 */
struct alignas(16) Bone {
    uint32_t id;                         ///< 骨骼 ID
    uint32_t parentId;                   ///< 父骨骼 ID（UINT32_MAX 表示无父节点）
    math::Matrix4 inverseBindMatrix;     ///< 绑定逆矩阵
    math::Matrix4 poseMatrix;            ///< 当前姿态矩阵
    math::Vector3 localPosition{0.0f};   ///< 局部位置
    math::Quaternion localRotation{1.0f, 0.0f, 0.0f, 0.0f};  ///< 局部旋转
    math::Vector3 localScale{1.0f};      ///< 局部缩放
    std::string name;                    ///< 骨骼名称
    
    Bone() : id(0), parentId(UINT32_MAX) {}
};

/**
 * @brief 骨骼权重（用于蒙皮）
 */
struct alignas(8) BoneWeight {
    uint8_t boneIndex[4];                ///< 骨骼索引
    float weight[4];                     ///< 权重值
    
    BoneWeight() {
        boneIndex[0] = boneIndex[1] = boneIndex[2] = boneIndex[3] = 0;
        weight[0] = weight[1] = weight[2] = weight[3] = 0.0f;
    }
};

/**
 * @brief 形变目标（Morph Target）
 */
struct MorphTarget {
    std::string name;                               ///< 形变名称
    std::vector<math::Vector3> positionDeltas;      ///< 位置增量
    std::vector<math::Vector3> normalDeltas;        ///< 法线增量
    std::vector<math::Vector3> tangentDeltas;       ///< 切线增量
    float weight{0.0f};                             ///< 当前权重
    
    MorphTarget() = default;
    explicit MorphTarget(const std::string& n) : name(n) {}
};

/**
 * @brief 动画剪辑数据
 */
struct AnimationClip {
    std::string name;                               ///< 动画名称
    float duration{0.0f};                           ///< 动画时长（秒）
    float fps{30.0f};                               ///< 帧率
    LoopMode loopMode{LoopMode::Loop};              ///< 循环模式
    
    // 每个骨骼的关键帧序列
    struct Channel {
        uint32_t boneIndex;                         ///< 骨骼索引
        AnimationChannelType type;                  ///< 通道类型
        InterpolationType interpolation{InterpolationType::Linear};
        std::vector<float> times;                   ///< 时间数组
        std::vector<math::Vector3> values;          ///< 值数组（位置/缩放）
        std::vector<math::Quaternion> rotations;    ///< 旋转数组（仅旋转通道）
        std::vector<math::Vector3> inTangents;      ///< 输入切线
        std::vector<math::Vector3> outTangents;     ///< 输出切线
    };
    
    std::vector<Channel> channels;                  ///< 所有通道
    
    // 形变目标关键帧
    struct MorphChannel {
        uint32_t morphIndex;                        ///< 形变目标索引
        std::vector<float> times;
        std::vector<float> weights;
    };
    
    std::vector<MorphChannel> morphChannels;        ///< 形变通道
    
    AnimationClip() = default;
    explicit AnimationClip(const std::string& n) : name(n) {}
};

/**
 * @brief 动画状态
 */
struct AnimationState {
    uint32_t clipIndex{UINT32_MAX};                 ///< 剪辑索引
    float time{0.0f};                               ///< 当前时间
    float weight{1.0f};                             ///< 混合权重
    float speed{1.0f};                              ///< 播放速度
    LoopMode loopMode{LoopMode::Loop};              ///< 循环模式
    bool playing{false};                            ///< 是否正在播放
    bool faded{false};                              ///< 是否已淡入
    
    float fadeTime{0.0f};                           ///< 淡入淡出时间
    float fadeDuration{0.25f};                      ///< 淡入淡出持续时间
};

/**
 * @brief 根运动数据
 */
struct RootMotion {
    math::Vector3 positionDelta{0.0f};              ///< 位置增量
    math::Quaternion rotationDelta{1.0f, 0.0f, 0.0f, 0.0f};  ///< 旋转增量
    bool enabled{false};                            ///< 是否启用
    bool extractFromAnimation{true};                ///< 是否从动画提取
    
    // 提取的根运动曲线
    std::vector<float> times;
    std::vector<math::Vector3> positions;
    std::vector<math::Quaternion> rotations;
};

/**
 * @brief GPU 蒙皮数据（用于着色器）
 */
struct alignas(16) GPUSkinData {
    math::Matrix4 boneMatrices[64];                 ///< 骨骼矩阵（最多 64 根）
    uint32_t boneCount{0};                          ///< 骨骼数量
};

} // namespace scene
} // namespace phoenix
