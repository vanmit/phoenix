#pragma once

#include "transform.hpp"
#include "../math/bounding.hpp"
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <nlohmann/json.hpp>

namespace phoenix {
namespace scene {

// Forward declarations
class Scene;
class SceneNode;

/**
 * @brief JSON-based scene serializer (glTF compatible)
 * 
 * Supports:
 * - Scene graph hierarchy
 * - Transforms and components
 * - glTF 2.0 export/import
 * - Binary and text formats
 */
class SceneSerializer {
public:
    using json = nlohmann::json;
    
    explicit SceneSerializer(const std::string& filename);
    virtual ~SceneSerializer();
    
    // ========================================================================
    // Scene Serialization
    // ========================================================================
    
    bool saveScene(const Scene& scene);
    bool loadScene(Scene& scene);
    
    // ========================================================================
    // Node Serialization
    // ========================================================================
    
    void beginNode(const SceneNode& node);
    void endNode();
    
    template<typename T>
    void serialize(const std::string& name, const T& value) {
        currentNode_[name] = toJson(value);
    }
    
    // ========================================================================
    // glTF Export
    // ========================================================================
    
    bool exportToGlTF(const std::string& filename, bool binary = false);
    bool importFromGlTF(const std::string& filename);
    
protected:
    std::string filename_;
    json root_;
    json currentNode_;
    std::vector<json> nodes_;
    
    virtual json toJson(const Transform& t) const;
    virtual json toJson(const math::Vector3& v) const;
    virtual json toJson(const math::Quaternion& q) const;
    virtual json toJson(const math::BoundingBox& b) const;
    
    void writeToFile();
    bool readFromFile();
};

/**
 * @brief Scene deserializer
 */
class SceneDeserializer {
public:
    using json = nlohmann::json;
    
    explicit SceneDeserializer(const std::string& filename);
    virtual ~SceneDeserializer();
    
    bool loadScene(Scene& scene);
    
    void beginNode(SceneNode& node);
    void endNode();
    
    template<typename T>
    void deserialize(const std::string& name, T& value) {
        if (currentNode_.contains(name)) {
            value = fromJson<T>(currentNode_[name]);
        }
    }
    
protected:
    std::string filename_;
    json root_;
    json currentNode_;
    std::vector<SceneNode*> nodeStack_;
    
    template<typename T>
    T fromJson(const json& j) const;
    
    Transform fromJsonTransform(const json& j) const;
    math::Vector3 fromJsonVector3(const json& j) const;
    math::Quaternion fromJsonQuaternion(const json& j) const;
    math::BoundingBox fromJsonBoundingBox(const json& j) const;
    
    bool readFromFile();
};

// ============================================================================
// Template Implementations
// ============================================================================

template<>
inline Transform SceneDeserializer::fromJson<Transform>(const json& j) const {
    return fromJsonTransform(j);
}

template<>
inline math::Vector3 SceneDeserializer::fromJson<math::Vector3>(const json& j) const {
    return fromJsonVector3(j);
}

template<>
inline math::Quaternion SceneDeserializer::fromJson<math::Quaternion>(const json& j) const {
    return fromJsonQuaternion(j);
}

template<>
inline math::BoundingBox SceneDeserializer::fromJson<math::BoundingBox>(const json& j) const {
    return fromJsonBoundingBox(j);
}

} // namespace scene
} // namespace phoenix
