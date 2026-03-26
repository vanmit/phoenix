#pragma once

#include "transform.hpp"
#include "../math/bounding.hpp"
#include <vector>
#include <array>
#include <unordered_map>
#include <cstdint>
#include <memory>
#include <functional>
#include <typeindex>
#include <bitset>

namespace phoenix {
namespace scene {

/**
 * @brief Entity handle (32-bit with generation counter)
 * 
 * Upper 16 bits: generation counter (for detecting stale handles)
 * Lower 16 bits: entity index
 */
class Entity {
public:
    using Index = uint16_t;
    using Generation = uint16_t;
    
    constexpr Entity() noexcept : id_(0) {}
    constexpr explicit Entity(uint32_t id) noexcept : id_(id) {}
    
    constexpr Index index() const noexcept { return id_ & 0xFFFF; }
    constexpr Generation generation() const noexcept { return id_ >> 16; }
    constexpr uint32_t id() const noexcept { return id_; }
    
    constexpr bool isValid() const noexcept { return id_ != 0; }
    constexpr bool operator==(const Entity& other) const noexcept { return id_ == other.id_; }
    constexpr bool operator!=(const Entity& other) const noexcept { return id_ != other.id_; }
    constexpr bool operator<(const Entity& other) const noexcept { return id_ < other.id_; }
    
    static constexpr Entity invalid() noexcept { return Entity(0); }
    
private:
    uint32_t id_;
};

// Hash support for unordered_map
template<>
struct std::hash<Entity> {
    size_t operator()(const Entity& e) const noexcept {
        return std::hash<uint32_t>{}(e.id());
    }
};

/**
 * @brief Component signature (up to 256 component types)
 */
using ComponentSignature = std::bitset<256>;

/**
 * @brief Entity Manager - handles entity creation/destruction
 * 
 * Uses generation counter to detect stale entity handles.
 * Supports up to 65535 concurrent entities.
 */
class EntityManager {
public:
    static constexpr size_t MaxEntities = 65535;
    static constexpr size_t MaxGenerations = 65535;
    
    EntityManager();
    
    /**
     * @brief Create new entity
     * @return Valid entity handle
     */
    Entity create();
    
    /**
     * @brief Destroy entity
     */
    void destroy(Entity entity);
    
    /**
     * @brief Check if entity is alive
     */
    bool isAlive(Entity entity) const;
    
    /**
     * @brief Check if entity handle is valid (not necessarily alive)
     */
    bool isValid(Entity entity) const;
    
    /**
     * @brief Get entity signature
     */
    const ComponentSignature& getSignature(Entity entity) const;
    
    /**
     * @brief Set entity signature
     */
    void setSignature(Entity entity, const ComponentSignature& sig);
    
    /**
     * @brief Get alive entity count
     */
    size_t getAliveCount() const noexcept { return aliveCount_; }
    
    /**
     * @brief Get total created count
     */
    size_t getTotalCreated() const noexcept { return totalCreated_; }
    
private:
    std::vector<Entity> available_;  // Recycled entity slots
    std::array<Generation, MaxEntities> generations_;
    std::array<ComponentSignature, MaxEntities> signatures_;
    std::bitset<MaxEntities> alive_;
    size_t aliveCount_;
    size_t totalCreated_;
};

/**
 * @brief Component Array base class
 */
class ComponentArray {
public:
    virtual ~ComponentArray() = default;
    virtual void entityDestroyed(Entity entity) = 0;
    virtual ComponentSignature getSignature() const = 0;
};

/**
 * @brief Component Array for specific component type (SoA layout)
 * 
 * Structure of Arrays for cache-friendly access.
 * Supports dense iteration over components.
 */
template<typename T>
class ComponentArrayImpl : public ComponentArray {
public:
    void insert(Entity entity, const T& component) {
        const size_t index = entity.index();
        
        if (index >= data_.size()) {
            data_.resize(index + 1);
            alive_.resize(index + 1);
        }
        
        data_[index] = component;
        alive_[index] = true;
        ++size_;
    }
    
    void remove(Entity entity) {
        const size_t index = entity.index();
        if (index < alive_.size() && alive_[index]) {
            alive_[index] = false;
            --size_;
        }
    }
    
    T& get(Entity entity) {
        return data_[entity.index()];
    }
    
    const T& get(Entity entity) const {
        return data_[entity.index()];
    }
    
    bool has(Entity entity) const {
        const size_t index = entity.index();
        return index < alive_.size() && alive_[index];
    }
    
    void entityDestroyed(Entity entity) override {
        remove(entity);
    }
    
    ComponentSignature getSignature() const override {
        return signature_;
    }
    
    void setSignature(const ComponentSignature& sig) {
        signature_ = sig;
    }
    
    // Dense iteration
    struct Iterator {
        T* ptr;
        size_t index;
        const std::vector<bool>* alive;
        
        T& operator*() { return *ptr; }
        T* operator->() { return ptr; }
        
        Iterator& operator++() {
            do {
                ++ptr;
                ++index;
            } while (index < alive->size() && !(*alive)[index]);
            return *this;
        }
        
        bool operator!=(const Iterator& other) const {
            return ptr != other.ptr;
        }
    };
    
    Iterator begin() {
        // Find first alive
        size_t i = 0;
        while (i < alive_.size() && !alive_[i]) ++i;
        return Iterator{data_.data() + i, i, &alive_};
    }
    
    Iterator end() {
        return Iterator{data_.data() + data_.size(), data_.size(), &alive_};
    }
    
    size_t size() const noexcept { return size_; }
    
private:
    std::vector<T> data_;
    std::vector<bool> alive_;
    ComponentSignature signature_;
    size_t size_ = 0;
};

/**
 * @brief Component Manager - handles component storage
 * 
 * Type-erased interface for component operations.
 * Uses SoA layout for cache efficiency.
 */
class ComponentManager {
public:
    /**
     * @brief Register component type
     * @return Type index for the component
     */
    template<typename T>
    void registerComponent() {
        const std::type_index type = std::type_index(typeid(T));
        
        if (componentTypes_.find(type) == componentTypes_.end()) {
            componentTypes_[type] = nextTypeIndex_++;
            
            auto array = std::make_shared<ComponentArrayImpl<T>>();
            ComponentSignature sig;
            sig.set(componentTypes_[type]);
            array->setSignature(sig);
            
            componentArrays_[type] = array;
        }
    }
    
    /**
     * @brief Add component to entity
     */
    template<typename T>
    void addComponent(Entity entity, const T& component) {
        const std::type_index type = std::type_index(typeid(T));
        
        auto array = std::static_pointer_cast<ComponentArrayImpl<T>>(componentArrays_[type]);
        array->insert(entity, component);
    }
    
    /**
     * @brief Remove component from entity
     */
    template<typename T>
    void removeComponent(Entity entity) {
        const std::type_index type = std::type_index(typeid(T));
        
        auto array = std::static_pointer_cast<ComponentArrayImpl<T>>(componentArrays_[type]);
        array->remove(entity);
    }
    
    /**
     * @brief Get component for entity
     */
    template<typename T>
    T& getComponent(Entity entity) {
        const std::type_index type = std::type_index(typeid(T));
        
        auto array = std::static_pointer_cast<ComponentArrayImpl<T>>(componentArrays_[type]);
        return array->get(entity);
    }
    
    template<typename T>
    const T& getComponent(Entity entity) const {
        const std::type_index type = std::type_index(typeid(T));
        
        auto array = std::static_pointer_cast<const ComponentArrayImpl<T>>(componentArrays_.at(type));
        return array->get(entity);
    }
    
    /**
     * @brief Check if entity has component
     */
    template<typename T>
    bool hasComponent(Entity entity) const {
        const std::type_index type = std::type_index(typeid(T));
        
        auto array = std::static_pointer_cast<const ComponentArrayImpl<T>>(componentArrays_.at(type));
        return array->has(entity);
    }
    
    /**
     * @brief Get component type index
     */
    template<typename T>
    uint8_t getComponentType() const {
        const std::type_index type = std::type_index(typeid(T));
        return componentTypes_.at(type);
    }
    
    /**
     * @brief Called when entity is destroyed
     */
    void entityDestroyed(Entity entity);
    
private:
    std::unordered_map<std::type_index, std::shared_ptr<ComponentArray>> componentArrays_;
    std::unordered_map<std::type_index, uint8_t> componentTypes_;
    uint8_t nextTypeIndex_ = 0;
};

/**
 * @brief System base class
 * 
 * Systems operate on entities with specific component signatures.
 */
class System {
public:
    virtual ~System() = default;
    
    /**
     * @brief Set required component signature
     */
    void setSignature(const ComponentSignature& signature) {
        signature_ = signature;
    }
    
    const ComponentSignature& getSignature() const {
        return signature_;
    }
    
    /**
     * @brief Update system
     * 
     * Override in derived classes.
     */
    virtual void update(float deltaTime) = 0;
    
protected:
    ComponentSignature signature_;
};

/**
 * @brief Event types for event bus
 */
enum class EventType : uint8_t {
    EntityCreated,
    EntityDestroyed,
    ComponentAdded,
    ComponentRemoved,
    Custom
};

/**
 * @brief Event base class
 */
struct Event {
    EventType type;
    Entity entity;
    
    Event(EventType t, Entity e) : type(t), entity(e) {}
    virtual ~Event() = default;
};

/**
 * @brief Event Bus for inter-system communication
 */
class EventBus {
public:
    using Listener = std::function<void(const Event&)>;
    using ListenerId = size_t;
    
    /**
     * @brief Subscribe to event type
     */
    ListenerId subscribe(EventType type, Listener listener) {
        const ListenerId id = nextListenerId_++;
        listeners_[type].emplace_back(id, std::move(listener));
        return id;
    }
    
    /**
     * @brief Unsubscribe
     */
    void unsubscribe(ListenerId id) {
        for (auto& [type, list] : listeners_) {
            list.erase(std::remove_if(list.begin(), list.end(),
                [id](const auto& l) { return l.first == id; }), list.end());
        }
    }
    
    /**
     * @brief Publish event
     */
    void publish(const Event& event) {
        auto it = listeners_.find(event.type);
        if (it != listeners_.end()) {
            for (const auto& [id, listener] : it->second) {
                listener(event);
            }
        }
    }
    
private:
    std::unordered_map<EventType, std::vector<std::pair<ListenerId, Listener>>> listeners_;
    ListenerId nextListenerId_ = 0;
};

/**
 * @brief ECS World - main coordinator
 * 
 * Manages entities, components, and systems.
 * Provides cache-friendly iteration over matching entities.
 */
class ECSWorld {
public:
    ECSWorld();
    ~ECSWorld();
    
    // ========================================================================
    // Entity Operations
    // ========================================================================
    
    Entity createEntity();
    void destroyEntity(Entity entity);
    
    bool isEntityAlive(Entity entity) const {
        return entityManager_.isAlive(entity);
    }
    
    // ========================================================================
    // Component Operations
    // ========================================================================
    
    template<typename T>
    void registerComponent() {
        componentManager_.registerComponent<T>();
    }
    
    template<typename T>
    void addComponent(Entity entity, const T& component) {
        componentManager_.addComponent<T>(entity, component);
        
        // Update signature
        auto sig = entityManager_.getSignature(entity);
        sig.set(componentManager_.getComponentType<T>());
        entityManager_.setSignature(entity, sig);
        
        // Publish event
        EventBus::Event event(EventType::ComponentAdded, entity);
        eventBus_.publish(event);
    }
    
    template<typename T>
    void removeComponent(Entity entity) {
        componentManager_.removeComponent<T>(entity);
        
        auto sig = entityManager_.getSignature(entity);
        sig.reset(componentManager_.getComponentType<T>());
        entityManager_.setSignature(entity, sig);
        
        EventBus::Event event(EventType::ComponentRemoved, entity);
        eventBus_.publish(event);
    }
    
    template<typename T>
    T& getComponent(Entity entity) {
        return componentManager_.getComponent<T>(entity);
    }
    
    template<typename T>
    bool hasComponent(Entity entity) const {
        return componentManager_.hasComponent<T>(entity);
    }
    
    // ========================================================================
    // System Operations
    // ========================================================================
    
    template<typename T, typename... Args>
    T& registerSystem(Args&&... args) {
        auto system = std::make_shared<T>(std::forward<Args>(args)...);
        systems_.push_back(system);
        return *system;
    }
    
    /**
     * @brief Update all systems
     */
    void update(float deltaTime);
    
    // ========================================================================
    // Event Operations
    // ========================================================================
    
    EventBus::ListenerId subscribe(EventType type, EventBus::Listener listener) {
        return eventBus_.subscribe(type, std::move(listener));
    }
    
    void unsubscribe(EventBus::ListenerId id) {
        eventBus_.unsubscribe(id);
    }
    
    // ========================================================================
    // Query
    // ========================================================================
    
    /**
     * @brief Get entities matching signature
     */
    std::vector<Entity> getEntitiesWithSignature(const ComponentSignature& sig) const;
    
    /**
     * @brief Get entity count
     */
    size_t getEntityCount() const noexcept {
        return entityManager_.getAliveCount();
    }
    
private:
    EntityManager entityManager_;
    ComponentManager componentManager_;
    std::vector<std::shared_ptr<System>> systems_;
    EventBus eventBus_;
};

} // namespace scene
} // namespace phoenix
