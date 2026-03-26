#include "phoenix/scene/skeleton.hpp"
#include "phoenix/scene/scene_serializer.hpp"
#include <algorithm>
#include <cstring>

namespace phoenix {
namespace scene {

// ============================================================================
// Skeleton 实现
// ============================================================================

uint32_t Skeleton::addBone(const std::string& name, 
                            uint32_t parentId,
                            const math::Matrix4& inverseBindMatrix) {
    // 创建新骨骼
    Bone bone;
    bone.id = static_cast<uint32_t>(bones_.size());
    bone.parentId = parentId;
    bone.name = name;
    bone.inverseBindMatrix = inverseBindMatrix;
    
    bones_.push_back(bone);
    finalMatrices_.resize(bones_.size());
    children_.resize(bones_.size());
    
    // 添加到名称映射
    boneMap_[name] = bone.id;
    
    // 如果是根骨骼
    if (parentId == UINT32_MAX) {
        rootBones_.push_back(bone.id);
    } else {
        // 添加到父骨骼的子节点列表
        if (parentId < children_.size()) {
            children_[parentId].push_back(bone.id);
        }
    }
    
    return bone.id;
}

const Bone* Skeleton::getBone(uint32_t index) const noexcept {
    if (index < bones_.size()) {
        return &bones_[index];
    }
    return nullptr;
}

Bone* Skeleton::getBone(uint32_t index) noexcept {
    if (index < bones_.size()) {
        return &bones_[index];
    }
    return nullptr;
}

int32_t Skeleton::findBoneByName(const std::string& name) const {
    auto it = boneMap_.find(name);
    if (it != boneMap_.end()) {
        return static_cast<int32_t>(it->second);
    }
    return -1;
}

void Skeleton::setParent(uint32_t boneIndex, uint32_t parentIndex) {
    if (boneIndex >= bones_.size()) {
        return;
    }
    
    Bone& bone = bones_[boneIndex];
    
    // 从旧的父节点移除
    if (bone.parentId != UINT32_MAX && bone.parentId < children_.size()) {
        auto& oldChildren = children_[bone.parentId];
        oldChildren.erase(std::remove(oldChildren.begin(), oldChildren.end(), boneIndex),
                          oldChildren.end());
    } else {
        // 从根骨骼列表移除
        auto it = std::find(rootBones_.begin(), rootBones_.end(), boneIndex);
        if (it != rootBones_.end()) {
            rootBones_.erase(it);
        }
    }
    
    // 设置新的父节点
    bone.parentId = parentIndex;
    
    // 添加到新的父节点
    if (parentIndex == UINT32_MAX) {
        rootBones_.push_back(boneIndex);
    } else {
        if (parentIndex < children_.size()) {
            children_[parentIndex].push_back(boneIndex);
        }
    }
}

const std::vector<uint32_t>& Skeleton::getChildren(uint32_t boneIndex) const {
    static const std::vector<uint32_t> empty;
    if (boneIndex < children_.size()) {
        return children_[boneIndex];
    }
    return empty;
}

void Skeleton::updateBonePose(uint32_t boneIndex,
                               const math::Vector3& position,
                               const math::Quaternion& rotation,
                               const math::Vector3& scale) {
    if (boneIndex >= bones_.size()) {
        return;
    }
    
    Bone& bone = bones_[boneIndex];
    bone.localPosition = position;
    bone.localRotation = rotation;
    bone.localScale = scale;
}

void Skeleton::calculateFinalMatrices() {
    // 计算所有根骨骼的矩阵
    for (uint32_t rootIndex : rootBones_) {
        calculateBoneMatrix(rootIndex, math::Matrix4::identity());
    }
}

void Skeleton::calculateBoneMatrix(uint32_t boneIndex, const math::Matrix4& parentMatrix) {
    if (boneIndex >= bones_.size()) {
        return;
    }
    
    Bone& bone = bones_[boneIndex];
    
    // 构建局部变换矩阵
    math::Matrix4 localMatrix;
    
    // 缩放
    math::Matrix4 scaleMatrix = math::Matrix4::scale(bone.localScale);
    
    // 旋转
    math::Matrix4 rotationMatrix = bone.localRotation.toMatrix();
    
    // 平移
    math::Matrix4 translationMatrix = math::Matrix4::translate(bone.localPosition);
    
    // 组合：SRT
    localMatrix = translationMatrix * rotationMatrix * scaleMatrix;
    
    // 计算世界矩阵
    math::Matrix4 worldMatrix = parentMatrix * localMatrix;
    finalMatrices_[boneIndex] = worldMatrix;
    
    // 递归处理子骨骼
    if (boneIndex < children_.size()) {
        for (uint32_t childIndex : children_[boneIndex]) {
            calculateBoneMatrix(childIndex, worldMatrix);
        }
    }
}

const math::Matrix4& Skeleton::getFinalMatrix(uint32_t boneIndex) const noexcept {
    static const math::Matrix4 identity;
    if (boneIndex < finalMatrices_.size()) {
        return finalMatrices_[boneIndex];
    }
    return identity;
}

void Skeleton::prepareGPUData(GPUSkinData& outData, size_t maxBones) const {
    outData.boneCount = static_cast<uint32_t>(std::min(bones_.size(), maxBones));
    
    for (size_t i = 0; i < outData.boneCount; ++i) {
        // 计算蒙皮矩阵：最终矩阵 * 绑定逆矩阵
        outData.boneMatrices[i] = finalMatrices_[i] * bones_[i].inverseBindMatrix;
    }
    
    // 如果骨骼数量不足，用单位矩阵填充
    for (size_t i = outData.boneCount; i < maxBones; ++i) {
        outData.boneMatrices[i] = math::Matrix4::identity();
    }
}

void Skeleton::resetToBindPose() {
    for (Bone& bone : bones_) {
        // 从绑定逆矩阵恢复绑定姿态
        math::Matrix4 bindMatrix = bone.inverseBindMatrix.inverted();
        
        // 分解矩阵获取位置、旋转、缩放
        bone.localPosition = math::Vector3(bindMatrix.data[3], bindMatrix.data[7], bindMatrix.data[11]);
        bone.localRotation = math::Quaternion::fromMatrix(bindMatrix);
        bone.localScale = math::Vector3(1.0f, 1.0f, 1.0f);
    }
    
    calculateFinalMatrices();
}

size_t Skeleton::memoryUsage() const noexcept {
    size_t total = 0;
    
    // 骨骼数据
    total += bones_.size() * sizeof(Bone);
    
    // 层级数据
    total += children_.size() * sizeof(std::vector<uint32_t>);
    for (const auto& children : children_) {
        total += children.size() * sizeof(uint32_t);
    }
    
    // 最终矩阵
    total += finalMatrices_.size() * sizeof(math::Matrix4);
    
    // 名称映射
    for (const auto& pair : boneMap_) {
        total += pair.first.size() + sizeof(uint32_t);
    }
    
    return total;
}

void Skeleton::serialize(BinarySerializer& serializer) const {
    // TODO: 实现序列化
}

void Skeleton::deserialize(BinaryDeserializer& deserializer) {
    // TODO: 实现反序列化
}

// ============================================================================
// SkinnedMeshData 实现
// ============================================================================

size_t SkinnedMeshData::memoryUsage() const noexcept {
    size_t total = 0;
    total += positions.size() * sizeof(math::Vector3);
    total += normals.size() * sizeof(math::Vector3);
    total += tangents.size() * sizeof(math::Vector3);
    total += texCoords.size() * sizeof(math::Vector2);
    total += boneWeights.size() * sizeof(BoneWeight);
    total += indices.size() * sizeof(uint32_t);
    return total;
}

} // namespace scene
} // namespace phoenix
