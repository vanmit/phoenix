#include "../../include/phoenix/scene/ecs.hpp"

namespace phoenix {
namespace scene {

// ============================================================================
// EntityManager Implementation
// ============================================================================

EntityManager::EntityManager()
    : available_()
    , generations_{}
    , signatures_{}
    , alive_{}
    , aliveCount_(0)
    , totalCreated_(0) {
    
    // Pre-populate available entities
    available_.reserve(MaxEntities);
    for (Index i = 0; i < MaxEntities; ++i) {
        available_.push_back(Entity(i));
        generations_[i] = 0;
    }
}

Entity EntityManager::create() {
    if (available_.empty()) {
        return Entity::invalid(); // No more entities
    }
    
    Entity entity = available_.back();
    available_.pop_back();
    
    const Index index = entity.index();
    
    // Increment generation (wrap around)
    generations_[index] = (generations_[index] + 1) % MaxGenerations;
    
    // Create new entity ID with updated generation
    entity = Entity((static_cast<uint32_t>(generations_[index]) << 16) | index);
    
    alive_[index] = true;
    signatures_[index].reset();
    ++aliveCount_;
    ++totalCreated_;
    
    return entity;
}

void EntityManager::destroy(Entity entity) {
    if (!isValid(entity)) return;
    
    const Index index = entity.index();
    
    if (!alive_[index]) return; // Already destroyed
    
    alive_[index] = false;
    --aliveCount_;
    
    available_.push_back(Entity(index)); // Recycle index (generation preserved in ID)
}

bool EntityManager::isAlive(Entity entity) const {
    if (!isValid(entity)) return false;
    
    const Index index = entity.index();
    const Generation gen = entity.generation();
    
    return alive_[index] && generations_[index] == gen;
}

bool EntityManager::isValid(Entity entity) const {
    return entity.index() < MaxEntities;
}

const ComponentSignature& EntityManager::getSignature(Entity entity) const {
    return signatures_[entity.index()];
}

void EntityManager::setSignature(Entity entity, const ComponentSignature& sig) {
    signatures_[entity.index()] = sig;
}

// ============================================================================
// ComponentManager Implementation
// ============================================================================

void ComponentManager::entityDestroyed(Entity entity) {
    for (auto& [type, array] : componentArrays_) {
        array->entityDestroyed(entity);
    }
}

// ============================================================================
// ECSWorld Implementation
// ============================================================================

ECSWorld::ECSWorld() = default;
ECSWorld::~ECSWorld() = default;

Entity ECSWorld::createEntity() {
    Entity entity = entityManager_.create();
    
    if (entity.isValid()) {
        EventBus::Event event(EventType::EntityCreated, entity);
        eventBus_.publish(event);
    }
    
    return entity;
}

void ECSWorld::destroyEntity(Entity entity) {
    if (!entityManager_.isAlive(entity)) return;
    
    // Clean up components
    componentManager_.entityDestroyed(entity);
    
    // Destroy entity
    entityManager_.destroy(entity);
    
    EventBus::Event event(EventType::EntityDestroyed, entity);
    eventBus_.publish(event);
}

void ECSWorld::update(float deltaTime) {
    for (auto& system : systems_) {
        system->update(deltaTime);
    }
}

std::vector<Entity> ECSWorld::getEntitiesWithSignature(const ComponentSignature& sig) const {
    std::vector<Entity> result;
    
    // Iterate through all possible entities
    for (uint16_t i = 0; i < EntityManager::MaxEntities; ++i) {
        Entity entity(i);
        if (entityManager_.isAlive(entity)) {
            const auto& entitySig = entityManager_.getSignature(entity);
            
            // Check if entity has all required components
            if ((entitySig & sig) == sig) {
                result.push_back(entity);
            }
        }
    }
    
    return result;
}

} // namespace scene
} // namespace phoenix
