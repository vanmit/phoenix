#include "../../include/phoenix/scene/scene_serializer.hpp"
#include "../../include/phoenix/scene/scene.hpp"
#include "../../include/phoenix/scene/scene_node.hpp"
#include <fstream>
#include <sstream>

namespace phoenix {
namespace scene {

// ============================================================================
// SceneSerializer Implementation
// ============================================================================

SceneSerializer::SceneSerializer(const std::string& filename)
    : filename_(filename)
    , root_()
    , currentNode_()
    , nodes_() {
}

SceneSerializer::~SceneSerializer() = default;

bool SceneSerializer::saveScene(const Scene& scene) {
    root_ = json::object();
    root_["asset"] = {
        {"version", "2.0"},
        {"generator", "Phoenix Engine"}
    };
    
    nodes_.clear();
    
    // Serialize scene graph
    if (scene.getRoot()) {
        scene.getRoot()->serialize(*this);
    }
    
    root_["nodes"] = nodes_;
    
    writeToFile();
    
    return true;
}

bool SceneSerializer::loadScene(Scene& scene) {
    // TODO: Implement loading
    (void)scene;
    return readFromFile();
}

void SceneSerializer::beginNode(const SceneNode& node) {
    currentNode_ = json::object();
    currentNode_["name"] = node.getName();
    currentNode_["id"] = node.getId();
}

void SceneSerializer::endNode() {
    nodes_.push_back(currentNode_);
    currentNode_.clear();
}

json SceneSerializer::toJson(const Transform& t) const {
    return {
        {"position", toJson(t.getPosition())},
        {"rotation", {t.getRotation().x, t.getRotation().y, t.getRotation().z, t.getRotation().w}},
        {"scale", toJson(t.getScale())}
    };
}

json SceneSerializer::toJson(const math::Vector3& v) const {
    return {v.x, v.y, v.z};
}

json SceneSerializer::toJson(const math::Quaternion& q) const {
    return {q.x, q.y, q.z, q.w};
}

json SceneSerializer::toJson(const math::BoundingBox& b) const {
    return {
        {"min", toJson(b.min)},
        {"max", toJson(b.max)}
    };
}

void SceneSerializer::writeToFile() {
    std::ofstream file(filename_);
    if (file.is_open()) {
        file << root_.dump(2);
        file.close();
    }
}

bool SceneSerializer::readFromFile() {
    std::ifstream file(filename_);
    if (file.is_open()) {
        try {
            file >> root_;
            file.close();
            return true;
        } catch (...) {
            file.close();
            return false;
        }
    }
    return false;
}

bool SceneSerializer::exportToGlTF(const std::string& filename, bool binary) {
    // TODO: Full glTF implementation
    (void)filename;
    (void)binary;
    return false;
}

bool SceneSerializer::importFromGlTF(const std::string& filename) {
    // TODO: Full glTF implementation
    (void)filename;
    return false;
}

// ============================================================================
// SceneDeserializer Implementation
// ============================================================================

SceneDeserializer::SceneDeserializer(const std::string& filename)
    : filename_(filename)
    , root_()
    , currentNode_()
    , nodeStack_() {
}

SceneDeserializer::~SceneDeserializer() = default;

bool SceneDeserializer::loadScene(Scene& scene) {
    if (!readFromFile()) return false;
    
    // TODO: Parse glTF and populate scene
    (void)scene;
    
    return true;
}

void SceneDeserializer::beginNode(SceneNode& node) {
    nodeStack_.push_back(&node);
}

void SceneDeserializer::endNode() {
    if (!nodeStack_.empty()) {
        nodeStack_.pop_back();
    }
}

Transform SceneDeserializer::fromJsonTransform(const json& j) const {
    Transform t;
    
    if (j.contains("position")) {
        t.setPosition(fromJsonVector3(j["position"]));
    }
    
    if (j.contains("rotation")) {
        const auto& rot = j["rotation"];
        t.setRotation(math::Quaternion(rot[0], rot[1], rot[2], rot[3]));
    }
    
    if (j.contains("scale")) {
        t.setScale(fromJsonVector3(j["scale"]));
    }
    
    return t;
}

math::Vector3 SceneDeserializer::fromJsonVector3(const json& j) const {
    return math::Vector3(j[0], j[1], j[2]);
}

math::Quaternion SceneDeserializer::fromJsonQuaternion(const json& j) const {
    return math::Quaternion(j[0], j[1], j[2], j[3]);
}

math::BoundingBox SceneDeserializer::fromJsonBoundingBox(const json& j) const {
    return math::BoundingBox(
        fromJsonVector3(j["min"]),
        fromJsonVector3(j["max"])
    );
}

bool SceneDeserializer::readFromFile() {
    std::ifstream file(filename_);
    if (file.is_open()) {
        try {
            file >> root_;
            file.close();
            return true;
        } catch (...) {
            file.close();
            return false;
        }
    }
    return false;
}

} // namespace scene
} // namespace phoenix
