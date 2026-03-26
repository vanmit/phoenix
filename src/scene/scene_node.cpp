#include "../../include/phoenix/scene/scene_node.hpp"
#include "../../include/phoenix/scene/scene_serializer.hpp"
#include <algorithm>
#include <atomic>

namespace phoenix {
namespace scene {

// ============================================================================
// Static ID Generator
// ============================================================================

static std::atomic<uint32_t> s_nextId{1};

// ============================================================================
// SceneNode Implementation
// ============================================================================

SceneNode::SceneNode(const std::string& name, NodeType type)
    : id_(s_nextId++)
    , name_(name)
    , type_(type)
    , flags_(NodeFlags::Visible)
    , transform_()
    , localBounds_(math::BoundingBox::empty())
    , localBoundsSphere_(math::BoundingSphere::empty())
    , parent_(nullptr)
    , children_()
    , components_()
    , userData_()
    , cachedWorldMatrix_(math::Matrix4::identity())
    , cachedWorldBounds_(math::BoundingBox::empty())
    , worldMatrixValid_(false)
    , worldBoundsValid_(false) {
}

SceneNode::~SceneNode() {
    // Remove from parent if attached
    if (parent_) {
        parent_->removeChild(shared_from_this());
    }
    
    // Clear children (they'll be destroyed if no other references)
    for (auto& child : children_) {
        child->parent_ = nullptr;
    }
    children_.clear();
}

SceneNode::SceneNode(SceneNode&& other) noexcept
    : id_(other.id_)
    , name_(std::move(other.name_))
    , type_(other.type_)
    , flags_(other.flags_)
    , transform_(std::move(other.transform_))
    , localBounds_(other.localBounds_)
    , localBoundsSphere_(other.localBoundsSphere_)
    , parent_(other.parent_)
    , children_(std::move(other.children_))
    , components_(std::move(other.components_))
    , userData_(std::move(other.userData_))
    , cachedWorldMatrix_(other.cachedWorldMatrix_)
    , cachedWorldBounds_(other.cachedWorldBounds_)
    , worldMatrixValid_(other.worldMatrixValid_)
    , worldBoundsValid_(other.worldBoundsValid_) {
    
    // Update parent and children references
    if (parent_) {
        for (auto& child : parent_->children_) {
            if (child.get() == &other) {
                child.reset(this);
                break;
            }
        }
    }
    
    for (auto& child : children_) {
        if (child->parent_ == &other) {
            child->parent_ = this;
        }
    }
    
    other.parent_ = nullptr;
    other.children_.clear();
}

SceneNode& SceneNode::operator=(SceneNode&& other) noexcept {
    if (this != &other) {
        id_ = other.id_;
        name_ = std::move(other.name_);
        type_ = other.type_;
        flags_ = other.flags_;
        transform_ = std::move(other.transform_);
        localBounds_ = other.localBounds_;
        localBoundsSphere_ = other.localBoundsSphere_;
        parent_ = other.parent_;
        children_ = std::move(other.children_);
        components_ = std::move(other.components_);
        userData_ = std::move(other.userData_);
        cachedWorldMatrix_ = other.cachedWorldMatrix_;
        cachedWorldBounds_ = other.cachedWorldBounds_;
        worldMatrixValid_ = other.worldMatrixValid_;
        worldBoundsValid_ = other.worldBoundsValid_;
        
        // Update references
        if (parent_) {
            for (auto& child : parent_->children_) {
                if (child.get() == &other) {
                    child.reset(this);
                    break;
                }
            }
        }
        
        for (auto& child : children_) {
            if (child->parent_ == &other) {
                child->parent_ = this;
            }
        }
        
        other.parent_ = nullptr;
        other.children_.clear();
    }
    
    return *this;
}

// ============================================================================
// Hierarchy Operations
// ============================================================================

SceneNode* SceneNode::findChild(const std::string& name) noexcept {
    auto it = std::find_if(children_.begin(), children_.end(),
        [&name](const Ptr& child) { return child->getName() == name; });
    
    return (it != children_.end()) ? it->get() : nullptr;
}

const SceneNode* SceneNode::findChild(const std::string& name) const noexcept {
    auto it = std::find_if(children_.begin(), children_.end(),
        [&name](const Ptr& child) { return child->getName() == name; });
    
    return (it != children_.end()) ? it->get() : nullptr;
}

void SceneNode::addChild(Ptr child) {
    if (!child || child.get() == this) {
        return;
    }
    
    // Remove from current parent
    if (child->parent_) {
        child->parent_->removeChild(child);
    }
    
    child->parent_ = this;
    children_.push_back(std::move(child));
    
    onChildAdded(children_.back());
}

void SceneNode::removeChild(Ptr child) {
    if (!child) return;
    
    auto it = std::find_if(children_.begin(), children_.end(),
        [&child](const Ptr& c) { return c.get() == child.get(); });
    
    if (it != children_.end()) {
        onChildRemoved(*it);
        (*it)->parent_ = nullptr;
        children_.erase(it);
    }
}

void SceneNode::removeChild(size_t index) {
    if (index < children_.size()) {
        onChildRemoved(children_[index]);
        children_[index]->parent_ = nullptr;
        children_.erase(children_.begin() + index);
    }
}

void SceneNode::removeChild(const std::string& name) {
    auto it = std::find_if(children_.begin(), children_.end(),
        [&name](const Ptr& child) { return child->getName() == name; });
    
    if (it != children_.end()) {
        onChildRemoved(*it);
        (*it)->parent_ = nullptr;
        children_.erase(it);
    }
}

void SceneNode::removeFromParent() {
    if (parent_) {
        parent_->removeChild(shared_from_this());
    }
}

bool SceneNode::isDescendantOf(const SceneNode& other) const noexcept {
    const SceneNode* current = parent_;
    while (current) {
        if (current == &other) return true;
        current = current->parent_;
    }
    return false;
}

bool SceneNode::isAncestorOf(const SceneNode& other) const noexcept {
    return other.isDescendantOf(*this);
}

void SceneNode::onChildAdded(Ptr child) {
    // Mark world transforms as dirty
    markTransformDirty();
    child->markTransformDirty();
}

void SceneNode::onChildRemoved(Ptr child) {
    (void)child; // Suppress unused warning
    markTransformDirty();
}

// ============================================================================
// Transform Operations
// ============================================================================

const math::Matrix4& SceneNode::getWorldMatrix() {
    updateCachedWorldMatrix();
    return cachedWorldMatrix_;
}

void SceneNode::updateWorldTransform() {
    // Update from root to this node
    std::vector<SceneNode*> path;
    
    SceneNode* current = this;
    while (current) {
        path.push_back(current);
        current = current->parent_;
    }
    
    // Update from root down
    math::Matrix4 parentMatrix = math::Matrix4::identity();
    for (auto it = path.rbegin(); it != path.rend(); ++it) {
        (*it)->transform_.updateWorldMatrix(parentMatrix);
        parentMatrix = (*it)->transform_.getWorldMatrix();
        (*it)->cachedWorldMatrix_ = parentMatrix;
        (*it)->worldMatrixValid_ = true;
    }
}

void SceneNode::markTransformDirty() {
    transform_.markWorldDirty();
    worldMatrixValid_ = false;
    worldBoundsValid_ = false;
    
    // Mark all children as dirty too
    for (auto& child : children_) {
        child->markTransformDirty();
    }
}

void SceneNode::updateCachedWorldMatrix() {
    if (worldMatrixValid_) return;
    
    if (parent_) {
        parent_->updateCachedWorldMatrix();
        transform_.updateWorldMatrix(parent_->cachedWorldMatrix_);
    } else {
        transform_.updateWorldMatrix();
    }
    
    cachedWorldMatrix_ = transform_.getWorldMatrix();
    worldMatrixValid_ = true;
}

// ============================================================================
// Bounding Volume Operations
// ============================================================================

math::BoundingBox SceneNode::getWorldBoundingBox() {
    updateCachedWorldBounds();
    return cachedWorldBounds_;
}

math::BoundingSphere SceneNode::getWorldBoundingSphere() {
    updateCachedWorldMatrix();
    return localBoundsSphere_.transform(cachedWorldMatrix_);
}

void SceneNode::updateCachedWorldBounds() {
    if (worldBoundsValid_) return;
    
    updateCachedWorldMatrix();
    
    // Transform local bounds by world matrix
    cachedWorldBounds_ = localBounds_;
    cachedWorldBounds_.transform(cachedWorldMatrix_);
    
    // Extend with children bounds
    for (auto& child : children_) {
        child->updateCachedWorldBounds();
        cachedWorldBounds_.extend(child->cachedWorldBounds_);
    }
    
    worldBoundsValid_ = true;
}

// ============================================================================
// Traversal
// ============================================================================

void SceneNode::accept(SceneVisitor& visitor) {
    visitor.visit(*this);
}

void SceneNode::traverse(SceneVisitor& visitor) {
    if (visitor.enter(*this)) {
        for (auto& child : children_) {
            child->traverse(visitor);
        }
    }
    visitor.leave(*this);
}

void SceneNode::traverse(const TraverseCallback& callback) {
    callback(*this);
    for (auto& child : children_) {
        child->traverse(callback);
    }
}

void SceneNode::traverse(const TraverseCallback& callback, int maxDepth) {
    if (maxDepth < 0) return;
    
    callback(*this);
    
    if (maxDepth > 0) {
        for (auto& child : children_) {
            child->traverse(callback, maxDepth - 1);
        }
    }
}

int SceneNode::getDepth() const noexcept {
    int depth = 0;
    const SceneNode* current = parent_;
    while (current) {
        ++depth;
        current = current->parent_;
    }
    return depth;
}

size_t SceneNode::getNodeCount() const noexcept {
    size_t count = 1; // This node
    for (const auto& child : children_) {
        count += child->getNodeCount();
    }
    return count;
}

// ============================================================================
// Clone and Reset
// ============================================================================

SceneNode::Ptr SceneNode::clone() const {
    auto clone = std::make_shared<SceneNode>(name_, type_);
    clone->flags_ = flags_;
    clone->transform_ = transform_;
    clone->localBounds_ = localBounds_;
    clone->localBoundsSphere_ = localBoundsSphere_;
    
    // Clone children
    for (const auto& child : children_) {
        clone->addChild(child->clone());
    }
    
    // Note: Components and user data are not cloned by default
    // Subclasses should override to handle custom components
    
    return clone;
}

void SceneNode::reset() {
    transform_ = Transform();
    localBounds_ = math::BoundingBox::empty();
    localBoundsSphere_ = math::BoundingSphere::empty();
    flags_ = NodeFlags::Visible;
    worldMatrixValid_ = false;
    worldBoundsValid_ = false;
    
    components_.clear();
    userData_.clear();
    
    for (auto& child : children_) {
        child->parent_ = nullptr;
    }
    children_.clear();
}

// ============================================================================
// Serialization (stubs - implemented in scene_serializer.cpp)
// ============================================================================

void SceneNode::serialize(SceneSerializer& serializer) const {
    serializer.beginNode(*this);
    serializer.serialize("name", name_);
    serializer.serialize("type", static_cast<uint8_t>(type_));
    serializer.serialize("flags", static_cast<uint32_t>(flags_));
    serializer.serialize("transform", transform_);
    serializer.serialize("localBounds", localBounds_);
    serializer.endNode();
}

void SceneNode::deserialize(SceneDeserializer& deserializer) {
    deserializer.beginNode(*this);
    deserializer.deserialize("name", name_);
    
    uint8_t typeInt;
    deserializer.deserialize("type", typeInt);
    type_ = static_cast<NodeType>(typeInt);
    
    uint32_t flagsInt;
    deserializer.deserialize("flags", flagsInt);
    flags_ = static_cast<NodeFlags>(flagsInt);
    
    deserializer.deserialize("transform", transform_);
    deserializer.deserialize("localBounds", localBounds_);
    
    deserializer.endNode();
}

} // namespace scene
} // namespace phoenix
