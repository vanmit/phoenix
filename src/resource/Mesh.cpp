#include "../../include/phoenix/resource/mesh.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace phoenix::resource {

// ============ AnimationChannel ============

math::Vector3 AnimationChannel::samplePosition(float time) const {
    if (keyframes.empty()) return math::Vector3(0.0f);
    if (keyframes.size() == 1) return keyframes[0].position;
    
    // Find surrounding keyframes
    for (size_t i = 0; i < keyframes.size() - 1; ++i) {
        if (time >= keyframes[i].time && time <= keyframes[i + 1].time) {
            float t = (time - keyframes[i].time) / (keyframes[i + 1].time - keyframes[i].time);
            return math::lerp(keyframes[i].position, keyframes[i + 1].position, t);
        }
    }
    
    // Outside range - clamp
    if (time < keyframes[0].time) return keyframes[0].position;
    return keyframes.back().position;
}

math::Quaternion AnimationChannel::sampleRotation(float time) const {
    if (keyframes.empty()) return math::Quaternion::identity();
    if (keyframes.size() == 1) return keyframes[0].rotation;
    
    for (size_t i = 0; i < keyframes.size() - 1; ++i) {
        if (time >= keyframes[i].time && time <= keyframes[i + 1].time) {
            float t = (time - keyframes[i].time) / (keyframes[i + 1].time - keyframes[i].time);
            return math::slerp(keyframes[i].rotation, keyframes[i + 1].rotation, t);
        }
    }
    
    if (time < keyframes[0].time) return keyframes[0].rotation;
    return keyframes.back().rotation;
}

math::Vector3 AnimationChannel::sampleScale(float time) const {
    if (keyframes.empty()) return math::Vector3(1.0f);
    if (keyframes.size() == 1) return keyframes[0].scale;
    
    for (size_t i = 0; i < keyframes.size() - 1; ++i) {
        if (time >= keyframes[i].time && time <= keyframes[i + 1].time) {
            float t = (time - keyframes[i].time) / (keyframes[i + 1].time - keyframes[i].time);
            return math::lerp(keyframes[i].scale, keyframes[i + 1].scale, t);
        }
    }
    
    if (time < keyframes[0].time) return keyframes[0].scale;
    return keyframes.back().scale;
}

// ============ AnimationClip ============

math::Matrix4 AnimationClip::sampleJointTransform(uint32_t jointIndex, float time) const {
    for (const auto& channel : channels) {
        if (channel.jointIndex == jointIndex) {
            math::Vector3 pos = channel.samplePosition(time);
            math::Quaternion rot = channel.sampleRotation(time);
            math::Vector3 scale = channel.sampleScale(time);
            
            math::Matrix4 transform = math::Matrix4::translation(pos);
            transform = transform * math::Matrix4::rotation(rot);
            transform = transform * math::Matrix4::scale(scale);
            
            return transform;
        }
    }
    
    return math::Matrix4::identity();
}

// ============ Mesh ============

size_t Mesh::calculateMemoryUsage() const {
    size_t total = 0;
    
    for (const auto& prim : primitives) {
        total += prim.vertexData.size();
        total += prim.indices.size() * sizeof(uint32_t);
        
        for (const auto& morph : prim.morphTargets) {
            total += morph.positionDeltas.size() * sizeof(math::Vector3);
            total += morph.normalDeltas.size() * sizeof(math::Vector3);
        }
        
        total += prim.jointIndices.size();
        total += prim.jointWeights.size() * sizeof(float);
    }
    
    total += materials.size() * sizeof(Material);
    total += joints.size() * sizeof(Joint);
    
    for (const auto& anim : animations) {
        for (const auto& channel : anim.channels) {
            total += channel.keyframes.size() * sizeof(Keyframe);
        }
    }
    
    return total;
}

ValidationResult Mesh::validate() const {
    if (primitives.empty()) {
        return ValidationResult::failure("Mesh has no primitives");
    }
    
    for (size_t i = 0; i < primitives.size(); ++i) {
        const auto& prim = primitives[i];
        
        if (prim.vertexCount == 0) {
            return ValidationResult::failure("Primitive " + std::to_string(i) + " has no vertices");
        }
        
        if (prim.vertexData.size() < prim.vertexCount * prim.vertexFormat.stride) {
            return ValidationResult::failure("Primitive " + std::to_string(i) + " vertex data size mismatch");
        }
        
        if (prim.indexCount > 0 && prim.indices.size() < prim.indexCount) {
            return ValidationResult::failure("Primitive " + std::to_string(i) + " index data size mismatch");
        }
        
        // Check for NaN/Inf in vertices
        const float* verts = reinterpret_cast<const float*>(prim.vertexData.data());
        size_t floatCount = prim.vertexData.size() / sizeof(float);
        
        for (size_t j = 0; j < floatCount; ++j) {
            if (std::isnan(verts[j]) || std::isinf(verts[j])) {
                return ValidationResult::failure("Primitive " + std::to_string(i) + " contains NaN/Inf values");
            }
        }
    }
    
    // Validate materials
    for (size_t i = 0; i < primitives.size(); ++i) {
        if (primitives[i].materialIndex >= materials.size()) {
            return ValidationResult::failure("Primitive " + std::to_string(i) + 
                                           " references invalid material " + 
                                           std::to_string(primitives[i].materialIndex));
        }
    }
    
    // Validate joints
    for (size_t i = 0; i < joints.size(); ++i) {
        if (joints[i].parentIndex >= static_cast<int32_t>(joints.size())) {
            return ValidationResult::failure("Joint " + std::to_string(i) + 
                                           " has invalid parent index");
        }
    }
    
    return ValidationResult::success();
}

const AnimationClip* Mesh::getAnimation(const std::string& name) const {
    for (const auto& anim : animations) {
        if (anim.name == name) {
            return &anim;
        }
    }
    return nullptr;
}

int32_t Mesh::getJointIndex(const std::string& name) const {
    for (size_t i = 0; i < joints.size(); ++i) {
        if (joints[i].name == name) {
            return static_cast<int32_t>(i);
        }
    }
    return -1;
}

void Mesh::buildLODLevels(float maxDistance, uint32_t targetLevels) {
    lodLevels.clear();
    lodLevels.reserve(targetLevels);
    
    float totalVerts = 0;
    for (const auto& prim : primitives) {
        totalVerts += prim.vertexCount;
    }
    
    for (uint32_t i = 0; i < targetLevels; ++i) {
        float t = static_cast<float>(i) / (targetLevels - 1);
        float distance = maxDistance * t;
        float reduction = 1.0f - (t * 0.9f);  // Max 90% reduction
        
        uint32_t targetVerts = static_cast<uint32_t>(totalVerts * reduction);
        
        lodLevels.emplace_back(
            distance,
            t * 10.0f,  // Screen space error
            targetVerts,
            targetVerts  // Simplified: same as verts
        );
    }
}

} // namespace phoenix::resource
