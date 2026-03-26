#pragma once

#include "transform.hpp"
#include "../math/bounding.hpp"
#include <vector>
#include <memory>
#include <string>
#include <functional>
#include <typeindex>
#include <unordered_map>
#include <any>

namespace phoenix {
namespace scene {

// Forward declarations
class SceneNode;
class SceneVisitor;

/**
 * @brief Node types for scene graph classification
 */
enum class NodeType : uint8_t {
    Unknown = 0,
    Root,
    Group,
    Mesh,
    Light,
    Camera,
    Skeleton,
    LOD,
    Instance,
    Terrain,
    Volume,
    Custom
};

/**
 * @brief Scene graph node flags
 */
enum class NodeFlags : uint32_t {
    None = 0,
    Visible = 1 << 0,
    CastsShadow = 1 << 1,
    ReceivesShadow = 1 << 2,
    IsStatic = 1 << 3,        // Transform never changes
    IsDynamic = 1 << 4,       // Transform changes frequently
    IsTrigger = 1 << 5,       // Collision trigger only
    Selectable = 1 << 6,      // Can be selected
    Culled = 1 << 7,          // Currently culled
    Dirty = 1 << 8,           // Needs update
    Custom1 = 1 << 16,
    Custom2 = 1 << 17,
    Custom3 = 1 << 18,
    Custom4 = 1 << 19
};

inline NodeFlags operator|(NodeFlags a, NodeFlags b) {
    return static_cast<NodeFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline NodeFlags operator&(NodeFlags a, NodeFlags b) {
    return static_cast<NodeFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline NodeFlags operator~(NodeFlags a) {
    return static_cast<NodeFlags>(~static_cast<uint32_t>(a));
}

inline NodeFlags& operator|=(NodeFlags& a, NodeFlags b) {
    a = a | b;
    return a;
}

inline NodeFlags& operator&=(NodeFlags& a, NodeFlags b) {
    a = a & b;
    return a;
}

/**
 * @brief Visitor pattern base class for scene graph traversal
 * 
 * Implement custom visitors for rendering, culling, serialization, etc.
 */
class SceneVisitor {
public:
    virtual ~SceneVisitor() = default;
    
    /**
     * @brief Called before visiting a node
     * @return true to continue visiting children
     */
    virtual bool enter(SceneNode& node) = 0;
    
    /**
     * @brief Called after visiting a node's children
     */
    virtual void leave(SceneNode& node) = 0;
    
    /**
     * @brief Called for each node during traversal
     */
    virtual void visit(SceneNode& node) = 0;
};

/**
 * @brief Base class for all scene graph nodes
 * 
 * Implements hierarchical transform system with dirty flag optimization.
 * Supports arbitrary child nodes and component attachment.
 * 
 * Thread-safe for read operations; writes require external synchronization.
 */
class SceneNode {
public:
    // ========================================================================
    // Type Definitions
    // ========================================================================
    
    using Ptr = std::shared_ptr<SceneNode>;
    using ConstPtr = std::shared_ptr<const SceneNode>;
    using WeakPtr = std::weak_ptr<SceneNode>;
    using ChildrenList = std::vector<Ptr>;
    using ComponentMap = std::unordered_map<std::type_index, std::any>;
    
    // ========================================================================
    // Construction
    // ========================================================================
    
    explicit SceneNode(const std::string& name = "Node", NodeType type = NodeType::Unknown);
    virtual ~SceneNode();
    
    // Non-copyable, movable
    SceneNode(const SceneNode&) = delete;
    SceneNode& operator=(const SceneNode&) = delete;
    SceneNode(SceneNode&&) noexcept;
    SceneNode& operator=(SceneNode&&) noexcept;
    
    // ========================================================================
    // Identity
    // ========================================================================
    
    uint32_t getId() const noexcept { return id_; }
    const std::string& getName() const noexcept { return name_; }
    void setName(const std::string& name) noexcept { name_ = name; }
    
    NodeType getType() const noexcept { return type_; }
    void setType(NodeType type) noexcept { type_ = type; }
    
    NodeFlags getFlags() const noexcept { return flags_; }
    void setFlags(NodeFlags flags) noexcept { flags_ = flags; }
    
    bool hasFlag(NodeFlags flag) const noexcept { return (flags_ & flag) != NodeFlags::None; }
    void setFlag(NodeFlags flag, bool enabled) noexcept {
        if (enabled) {
            flags_ |= flag;
        } else {
            flags_ &= ~flag;
        }
    }
    
    // ========================================================================
    // Hierarchy
    // ========================================================================
    
    SceneNode* getParent() noexcept { return parent_; }
    const SceneNode* getParent() const noexcept { return parent_; }
    
    const ChildrenList& getChildren() const noexcept { return children_; }
    size_t getChildCount() const noexcept { return children_.size(); }
    
    SceneNode* getChild(size_t index) noexcept { return children_[index].get(); }
    const SceneNode* getChild(size_t index) const noexcept { return children_[index].get(); }
    
    SceneNode* findChild(const std::string& name) noexcept;
    const SceneNode* findChild(const std::string& name) const noexcept;
    
    /**
     * @brief Add child node
     * 
     * Sets this node as the child's parent and marks transforms as dirty.
     */
    void addChild(Ptr child);
    
    /**
     * @brief Remove child node
     * 
     * Does not delete the child, just removes from hierarchy.
     */
    void removeChild(Ptr child);
    void removeChild(size_t index);
    void removeChild(const std::string& name);
    
    /**
     * @brief Remove from parent
     * 
     * Detaches this node from its parent.
     */
    void removeFromParent();
    
    /**
     * @brief Check if node is descendant of another
     */
    bool isDescendantOf(const SceneNode& other) const noexcept;
    
    /**
     * @brief Check if node is ancestor of another
     */
    bool isAncestorOf(const SceneNode& other) const noexcept;
    
    // ========================================================================
    // Transform
    // ========================================================================
    
    Transform& getTransform() noexcept { return transform_; }
    const Transform& getTransform() const noexcept { return transform_; }
    
    math::Vector3 getPosition() const noexcept { return transform_.getPosition(); }
    math::Quaternion getRotation() const noexcept { return transform_.getRotation(); }
    math::Vector3 getScale() const noexcept { return transform_.getScale(); }
    
    void setPosition(const math::Vector3& pos) { transform_.setPosition(pos); }
    void setRotation(const math::Quaternion& rot) { transform_.setRotation(rot); }
    void setScale(const math::Vector3& scl) { transform_.setScale(scl); }
    
    /**
     * @brief Get world matrix (updates if dirty)
     */
    const math::Matrix4& getWorldMatrix();
    const math::Matrix4& getWorldMatrix() const noexcept { return transform_.getWorldMatrix(); }
    
    /**
     * @brief Force world matrix update
     * 
     * Recursively updates from root to this node.
     */
    void updateWorldTransform();
    
    /**
     * @brief Mark transform as dirty
     */
    void markTransformDirty();
    
    // ========================================================================
    // Bounding Volume
    // ========================================================================
    
    void setLocalBoundingBox(const math::BoundingBox& box) noexcept { localBounds_ = box; }
    const math::BoundingBox& getLocalBoundingBox() const noexcept { return localBounds_; }
    
    math::BoundingBox getWorldBoundingBox();
    
    void setLocalBoundingSphere(const math::BoundingSphere& sphere) noexcept { localBoundsSphere_ = sphere; }
    const math::BoundingSphere& getLocalBoundingSphere() const noexcept { return localBoundsSphere_; }
    
    math::BoundingSphere getWorldBoundingSphere();
    
    // ========================================================================
    // Components
    // ========================================================================
    
    /**
     * @brief Attach a component to this node
     */
    template<typename T, typename... Args>
    T& addComponent(Args&&... args) {
        auto component = std::make_shared<T>(std::forward<Args>(args)...);
        components_[typeid(T)] = component;
        return *component;
    }
    
    /**
     * @brief Get component by type
     */
    template<typename T>
    T* getComponent() noexcept {
        auto it = components_.find(typeid(T));
        if (it != components_.end()) {
            return std::any_cast<std::shared_ptr<T>>(it->second).get();
        }
        return nullptr;
    }
    
    template<typename T>
    const T* getComponent() const noexcept {
        auto it = components_.find(typeid(T));
        if (it != components_.end()) {
            return std::any_cast<std::shared_ptr<T>>(it->second).get();
        }
        return nullptr;
    }
    
    /**
     * @brief Check if node has component
     */
    template<typename T>
    bool hasComponent() const noexcept {
        return components_.find(typeid(T)) != components_.end();
    }
    
    /**
     * @brief Remove component
     */
    template<typename T>
    void removeComponent() {
        components_.erase(typeid(T));
    }
    
    /**
     * @brief Get all components
     */
    const ComponentMap& getComponents() const noexcept { return components_; }
    
    // ========================================================================
    // User Data
    // ========================================================================
    
    void setUserData(const std::string& key, std::any data);
    
    template<typename T>
    T* getUserData(const std::string& key) noexcept {
        auto it = userData_.find(key);
        if (it != userData_.end()) {
            return std::any_cast<T>(&it->second);
        }
        return nullptr;
    }
    
    template<typename T>
    const T* getUserData(const std::string& key) const noexcept {
        auto it = userData_.find(key);
        if (it != userData_.end()) {
            return std::any_cast<T>(&it->second);
        }
        return nullptr;
    }
    
    void removeUserData(const std::string& key);
    void clearUserData();
    
    // ========================================================================
    // Traversal
    // ========================================================================
    
    /**
     * @brief Accept visitor (visitor pattern)
     */
    virtual void accept(SceneVisitor& visitor);
    
    /**
     * @brief Traverse subtree with visitor
     */
    void traverse(SceneVisitor& visitor);
    
    /**
     * @brief Traverse with custom function
     */
    using TraverseCallback = std::function<void(SceneNode&)>;
    void traverse(const TraverseCallback& callback);
    
    /**
     * @brief Traverse with depth
     */
    void traverse(const TraverseCallback& callback, int maxDepth);
    
    // ========================================================================
    // Utility
    // ========================================================================
    
    /**
     * @brief Get depth in scene graph (root = 0)
     */
    int getDepth() const noexcept;
    
    /**
     * @brief Get total node count in subtree
     */
    size_t getNodeCount() const noexcept;
    
    /**
     * @brief Clone this node (deep copy)
     */
    virtual Ptr clone() const;
    
    /**
     * @brief Reset node to default state
     */
    virtual void reset();
    
    /**
     * @brief Check if node is effectively visible
     */
    bool isVisible() const noexcept {
        return hasFlag(NodeFlags::Visible) && !hasFlag(NodeFlags::Culled);
    }
    
    // ========================================================================
    // Serialization Support
    // ========================================================================
    
    virtual void serialize(class SceneSerializer& serializer) const;
    virtual void deserialize(class SceneDeserializer& deserializer);
    
protected:
    // ========================================================================
    // Protected Members
    // ========================================================================
    
    uint32_t id_;
    std::string name_;
    NodeType type_;
    NodeFlags flags_;
    
    Transform transform_;
    math::BoundingBox localBounds_;
    math::BoundingSphere localBoundsSphere_;
    
    SceneNode* parent_;
    ChildrenList children_;
    ComponentMap components_;
    std::unordered_map<std::string, std::any> userData_;
    
    // Cached values
    math::Matrix4 cachedWorldMatrix_;
    math::BoundingBox cachedWorldBounds_;
    bool worldMatrixValid_;
    bool worldBoundsValid_;
    
private:
    void updateCachedWorldMatrix();
    void updateCachedWorldBounds();
    void onChildAdded(Ptr child);
    void onChildRemoved(Ptr child);
};

// ============================================================================
// Inline Implementations
// ============================================================================

inline void SceneNode::setUserData(const std::string& key, std::any data) {
    userData_[key] = std::move(data);
}

inline void SceneNode::removeUserData(const std::string& key) {
    userData_.erase(key);
}

inline void SceneNode::clearUserData() {
    userData_.clear();
}

} // namespace scene
} // namespace phoenix
