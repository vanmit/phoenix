#pragma once

#include "animation_types.hpp"
#include "../math/matrix4.hpp"
#include "../math/quaternion.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>

namespace phoenix {
namespace scene {

/**
 * @brief 骨骼层级结构
 * 
 * 管理骨骼的层级关系和变换传播
 */
class Skeleton {
public:
    Skeleton() = default;
    ~Skeleton() = default;
    
    // 禁止拷贝，允许移动
    Skeleton(const Skeleton&) = delete;
    Skeleton& operator=(const Skeleton&) = delete;
    Skeleton(Skeleton&&) noexcept = default;
    Skeleton& operator=(Skeleton&&) noexcept = default;
    
    // ========================================================================
    // 骨骼管理
    // ========================================================================
    
    /**
     * @brief 添加骨骼
     * @param name 骨骼名称
     * @param parentId 父骨骼 ID（UINT32_MAX 表示根骨骼）
     * @param inverseBindMatrix 绑定逆矩阵
     * @return 骨骼 ID
     */
    uint32_t addBone(const std::string& name, 
                     uint32_t parentId = UINT32_MAX,
                     const math::Matrix4& inverseBindMatrix = math::Matrix4::identity());
    
    /**
     * @brief 获取骨骼数量
     */
    size_t boneCount() const noexcept { return bones_.size(); }
    
    /**
     * @brief 获取骨骼
     */
    const Bone* getBone(uint32_t index) const noexcept;
    Bone* getBone(uint32_t index) noexcept;
    
    /**
     * @brief 通过名称获取骨骼索引
     */
    int32_t findBoneByName(const std::string& name) const;
    
    /**
     * @brief 获取所有骨骼
     */
    const std::vector<Bone>& bones() const noexcept { return bones_; }
    std::vector<Bone>& bones() noexcept { return bones_; }
    
    // ========================================================================
    // 层级关系
    // ========================================================================
    
    /**
     * @brief 设置骨骼父子关系
     */
    void setParent(uint32_t boneIndex, uint32_t parentIndex);
    
    /**
     * @brief 获取子骨骼列表
     */
    const std::vector<uint32_t>& getChildren(uint32_t boneIndex) const;
    
    /**
     * @brief 获取根骨骼列表
     */
    const std::vector<uint32_t>& rootBones() const noexcept { return rootBones_; }
    
    // ========================================================================
    // 变换更新
    // ========================================================================
    
    /**
     * @brief 更新骨骼姿态
     * @param boneIndex 骨骼索引
     * @param position 局部位置
     * @param rotation 局部旋转
     * @param scale 局部缩放
     */
    void updateBonePose(uint32_t boneIndex,
                        const math::Vector3& position,
                        const math::Quaternion& rotation,
                        const math::Vector3& scale);
    
    /**
     * @brief 计算所有骨骼的最终矩阵
     * 
     * 从根骨骼开始递归计算，考虑层级变换
     */
    void calculateFinalMatrices();
    
    /**
     * @brief 获取骨骼的最终矩阵
     */
    const math::Matrix4& getFinalMatrix(uint32_t boneIndex) const noexcept;
    
    /**
     * @brief 获取用于 GPU 的骨骼矩阵数组
     */
    const std::vector<math::Matrix4>& finalMatrices() const noexcept { return finalMatrices_; }
    
    /**
     * @brief 准备 GPU 数据
     * @param outData 输出 GPU 数据
     * @param maxBones 最大骨骼数（通常 64）
     */
    void prepareGPUData(GPUSkinData& outData, size_t maxBones = 64) const;
    
    // ========================================================================
    // 工具方法
    // ========================================================================
    
    /**
     * @brief 重置所有骨骼到绑定姿态
     */
    void resetToBindPose();
    
    /**
     * @brief 获取骨骼数量（用于内存估算）
     */
    size_t memoryUsage() const noexcept;
    
    /**
     * @brief 序列化骨骼数据
     */
    void serialize(class BinarySerializer& serializer) const;
    
    /**
     * @brief 反序列化骨骼数据
     */
    void deserialize(class BinaryDeserializer& deserializer);
    
private:
    std::vector<Bone> bones_;                           ///< 骨骼数组
    std::vector<uint32_t> rootBones_;                   ///< 根骨骼索引
    std::vector<std::vector<uint32_t>> children_;       ///< 子骨骼索引
    std::vector<math::Matrix4> finalMatrices_;          ///< 最终变换矩阵
    std::unordered_map<std::string, uint32_t> boneMap_; ///< 名称到索引的映射
    
    /**
     * @brief 递归计算骨骼矩阵
     */
    void calculateBoneMatrix(uint32_t boneIndex, const math::Matrix4& parentMatrix);
};

/**
 * @brief 蒙皮网格数据
 * 
 * 包含顶点骨骼权重信息
 */
struct SkinnedMeshData {
    std::vector<math::Vector3> positions;           ///< 顶点位置
    std::vector<math::Vector3> normals;             ///< 顶点法线
    std::vector<math::Vector3> tangents;            ///< 顶点切线
    std::vector<math::Vector2> texCoords;           ///< 纹理坐标
    std::vector<BoneWeight> boneWeights;            ///< 骨骼权重
    std::vector<uint32_t> indices;                  ///< 索引
    
    size_t vertexCount() const noexcept { return positions.size(); }
    size_t indexCount() const noexcept { return indices.size(); }
    
    /**
     * @brief 内存使用量（字节）
     */
    size_t memoryUsage() const noexcept;
};

/**
 * @brief 骨骼动画组件
 * 
 * 附加到实体上以启用骨骼动画
 */
struct SkeletonComponent {
    std::shared_ptr<Skeleton> skeleton;             ///< 骨骼指针
    uint32_t rootBoneIndex{0};                      ///< 根骨骼索引
    bool visible{true};                             ///< 是否可见
    bool useGPUSkinning{true};                      ///< 是否使用 GPU 蒙皮
    
    SkeletonComponent() = default;
    explicit SkeletonComponent(std::shared_ptr<Skeleton> skel) 
        : skeleton(std::move(skel)) {}
};

} // namespace scene
} // namespace phoenix
