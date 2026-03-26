#pragma once

#include "animation_types.hpp"
#include "../math/vector3.hpp"
#include <vector>
#include <string>
#include <memory>

namespace phoenix {
namespace scene {

/**
 * @brief 形变动画控制器
 * 
 * 管理顶点形变和混合
 */
class MorphAnimationController {
public:
    MorphAnimationController() = default;
    ~MorphAnimationController() = default;
    
    // ========================================================================
    // 形变目标管理
    // ========================================================================
    
    /**
     * @brief 添加形变目标
     */
    uint32_t addMorphTarget(MorphTarget target);
    
    /**
     * @brief 获取形变目标数量
     */
    size_t morphTargetCount() const noexcept { return morphTargets_.size(); }
    
    /**
     * @brief 获取形变目标
     */
    const MorphTarget* getMorphTarget(uint32_t index) const noexcept;
    MorphTarget* getMorphTarget(uint32_t index) noexcept;
    
    /**
     * @brief 通过名称查找形变目标
     */
    int32_t findMorphTargetByName(const std::string& name) const;
    
    // ========================================================================
    // 权重控制
    // ========================================================================
    
    /**
     * @brief 设置形变权重
     * @param index 形变目标索引
     * @param weight 权重 (0-1)
     */
    void setWeight(uint32_t index, float weight);
    
    /**
     * @brief 获取形变权重
     */
    float weight(uint32_t index) const;
    
    /**
     * @brief 设置所有权重
     */
    void setWeights(const std::vector<float>& weights);
    
    /**
     * @brief 获取所有权重
     */
    const std::vector<float>& weights() const noexcept { return weights_; }
    
    /**
     * @brief 重置所有权重
     */
    void resetWeights();
    
    // ========================================================================
    // 形变计算
    // ========================================================================
    
    /**
     * @brief 应用形变到顶点
     * @param positions 位置数组（输入/输出）
     * @param normals 法线数组（输入/输出）
     * @param tangents 切线数组（输入/输出，可选）
     */
    void apply(std::vector<math::Vector3>& positions,
               std::vector<math::Vector3>& normals,
               std::vector<math::Vector3>* tangents = nullptr) const;
    
    /**
     * @brief 应用形变到单个顶点
     */
    void applyToVertex(math::Vector3& position, math::Vector3& normal, uint32_t vertexIndex) const;
    
    /**
     * @brief 计算混合后的形变数据（用于 GPU 上传）
     * @param outPositions 输出的位置增量
     * @param outNormals 输出的法线增量
     */
    void computeBlendedDeltas(std::vector<math::Vector3>& outPositions,
                               std::vector<math::Vector3>& outNormals) const;
    
    // ========================================================================
    // 表情动画
    // ========================================================================
    
    /**
     * @brief 添加预定义表情
     * @param name 表情名称
     * @param morphIndices 涉及的形变目标索引
     * @param weights 对应的权重
     */
    void addExpression(const std::string& name,
                       const std::vector<uint32_t>& morphIndices,
                       const std::vector<float>& weights);
    
    /**
     * @brief 播放表情
     * @param name 表情名称
     * @param fadeDuration 淡入时间
     */
    void playExpression(const std::string& name, float fadeDuration = 0.1f);
    
    /**
     * @brief 停止表情
     */
    void stopExpression(const std::string& name);
    
    /**
     * @brief 停止所有表情
     */
    void stopAllExpressions();
    
    // ========================================================================
    // 工具方法
    // ========================================================================
    
    /**
     * @brief 获取内存使用量
     */
    size_t memoryUsage() const noexcept;
    
    /**
     * @brief 设置顶点数量（用于初始化）
     */
    void setVertexCount(size_t count);
    
    /**
     * @brief 获取顶点数量
     */
    size_t vertexCount() const noexcept { return vertexCount_; }
    
private:
    std::vector<MorphTarget> morphTargets_;       ///< 形变目标列表
    std::vector<float> weights_;                   ///< 当前权重
    size_t vertexCount_{0};                        ///< 顶点数量
    
    struct Expression {
        std::string name;
        std::vector<uint32_t> morphIndices;
        std::vector<float> weights;
        float currentWeight{0.0f};
        float targetWeight{0.0f};
        float fadeSpeed{0.0f};
    };
    
    std::vector<Expression> expressions_;          ///< 表情列表
    
    /**
     * @brief 更新表情权重
     * @param deltaTime 时间增量
     */
    void updateExpressions(float deltaTime);
};

/**
 * @brief 形变动画组件
 * 
 * 附加到实体以启用形变动画
 */
struct MorphComponent {
    std::shared_ptr<MorphAnimationController> controller;  ///< 控制器
    bool enabled{true};                                    ///< 是否启用
    bool updateNormals{true};                              ///< 是否更新法线
    bool updateTangents{false};                            ///< 是否更新切线
    
    MorphComponent() = default;
    explicit MorphComponent(std::shared_ptr<MorphAnimationController> ctrl)
        : controller(std::move(ctrl)) {}
};

} // namespace scene
} // namespace phoenix
