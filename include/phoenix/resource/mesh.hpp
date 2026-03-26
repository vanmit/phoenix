#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

#include "../math/vector.hpp"
#include "../math/matrix.hpp"
#include "types.hpp"

namespace phoenix::resource {

/**
 * @brief Vertex attribute layout
 */
enum class VertexAttribute : uint8_t {
    Position = 0,
    Normal = 1,
    Tangent = 2,
    Bitangent = 3,
    Color = 4,
    UV0 = 5,
    UV1 = 6,
    JointIndices = 7,
    JointWeights = 8,
    Custom0 = 9,
    Custom1 = 10
};

/**
 * @brief Vertex format descriptor
 */
struct VertexFormat {
    uint32_t stride = 0;
    std::unordered_map<VertexAttribute, struct {
        uint8_t offset;
        uint8_t size;      // Number of components
        uint8_t typeSize;  // Size per component (1, 2, 4)
        bool normalized;
    }> attributes;
    
    void addAttribute(VertexAttribute attr, uint8_t components, uint8_t typeSize, bool normalized = false) {
        uint8_t offset = static_cast<uint8_t>(stride);
        attributes[attr] = {offset, components, typeSize, normalized};
        stride += components * typeSize;
    }
};

/**
 * @brief Skinned mesh joint
 */
struct Joint {
    math::Matrix4 inverseBindMatrix;
    math::Matrix4 transform;
    std::string name;
    int32_t parentIndex = -1;
};

/**
 * @brief Animation keyframe
 */
struct Keyframe {
    float time = 0.0f;
    math::Vector3 position;
    math::Quaternion rotation;
    math::Vector3 scale;
};

/**
 * @brief Animation channel for a single joint
 */
struct AnimationChannel {
    uint32_t jointIndex = 0;
    std::vector<Keyframe> keyframes;
    
    math::Vector3 samplePosition(float time) const;
    math::Quaternion sampleRotation(float time) const;
    math::Vector3 sampleScale(float time) const;
};

/**
 * @brief Animation clip
 */
struct AnimationClip {
    std::string name;
    float duration = 0.0f;
    float fps = 30.0f;
    std::vector<AnimationChannel> channels;
    bool isLooping = true;
    
    math::Matrix4 sampleJointTransform(uint32_t jointIndex, float time) const;
};

/**
 * @brief Morph target for shape deformation
 */
struct MorphTarget {
    std::string name;
    std::vector<math::Vector3> positionDeltas;
    std::vector<math::Vector3> normalDeltas;
    float weight = 0.0f;
};

/**
 * @brief Material PBR properties
 */
struct Material {
    std::string name;
    
    // Base color
    math::Color4 baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f};
    int32_t baseColorTexture = -1;
    
    // Metallic roughness
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
    int32_t metallicRoughnessTexture = -1;
    
    // Normal map
    int32_t normalTexture = -1;
    float normalScale = 1.0f;
    
    // Occlusion
    int32_t occlusionTexture = -1;
    float occlusionStrength = 1.0f;
    
    // Emissive
    math::Color3 emissiveFactor{0.0f, 0.0f, 0.0f};
    int32_t emissiveTexture = -1;
    
    // Alpha mode
    enum class AlphaMode { Opaque, Mask, Blend } alphaMode = AlphaMode::Opaque;
    float alphaCutoff = 0.5f;
    
    // Double sided
    bool doubleSided = false;
    
    // Texture coordinates
    uint8_t baseColorTexCoord = 0;
    uint8_t metallicRoughnessTexCoord = 0;
    uint8_t normalTexCoord = 0;
    uint8_t occlusionTexCoord = 0;
    uint8_t emissiveTexCoord = 0;
};

/**
 * @brief Mesh primitive
 */
struct MeshPrimitive {
    // Vertex data (interleaved)
    std::vector<uint8_t> vertexData;
    VertexFormat vertexFormat;
    uint32_t vertexCount = 0;
    
    // Index data
    std::vector<uint32_t> indices;
    uint32_t indexCount = 0;
    
    // Material reference
    uint32_t materialIndex = 0;
    
    // Morph targets
    std::vector<MorphTarget> morphTargets;
    
    // Bounding volume
    BoundingVolume bounds;
    
    // LOD levels
    std::vector<LODLevel> lodLevels;
    
    bool hasSkinning = false;
    std::vector<uint8_t> jointIndices;  // 4 bytes per vertex
    std::vector<float> jointWeights;    // 4 floats per vertex
};

/**
 * @brief Complete mesh resource
 */
class Mesh {
public:
    std::string name;
    std::string sourcePath;
    AssetType assetType = AssetType::Unknown;
    
    std::vector<MeshPrimitive> primitives;
    std::vector<Material> materials;
    std::vector<Joint> joints;
    std::vector<AnimationClip> animations;
    
    BoundingVolume bounds;
    MemoryBudget memoryUsage;
    
    LoadState loadState = LoadState::Pending;
    std::string loadError;
    
    // Skeleton root
    int32_t skeletonRoot = -1;
    
    Mesh() = default;
    ~Mesh() = default;
    
    // Move semantics
    Mesh(Mesh&&) noexcept = default;
    Mesh& operator=(Mesh&&) noexcept = default;
    
    // Copy semantics
    Mesh(const Mesh&) = default;
    Mesh& operator=(const Mesh&) = default;
    
    /**
     * @brief Calculate total memory usage
     */
    size_t calculateMemoryUsage() const;
    
    /**
     * @brief Validate mesh data
     */
    ValidationResult validate() const;
    
    /**
     * @brief Get animation clip by name
     */
    const AnimationClip* getAnimation(const std::string& name) const;
    
    /**
     * @brief Get joint by name
     */
    int32_t getJointIndex(const std::string& name) const;
    
    /**
     * @brief Build LOD levels
     */
    void buildLODLevels(float maxDistance = 100.0f, uint32_t targetLevels = 4);
};

} // namespace phoenix::resource
