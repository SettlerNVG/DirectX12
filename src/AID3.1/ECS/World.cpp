#include "World.h"
#include "../Utils/Logger.h"
#include <algorithm>

World::World() 
    : m_nextEntityId(1) {
    LOG_INFO("World created");
}

World::~World() {
    Clear();
    LOG_INFO("World destroyed");
}

Entity World::CreateEntity() {
    Entity entity = m_nextEntityId++;
    m_entities.push_back(entity);
    return entity;
}

void World::DestroyEntity(Entity entity) {
    // Удаляем все компоненты этой сущности
    for (auto& [typeIndex, componentMap] : m_components) {
        componentMap.erase(entity);
    }
    
    // Удаляем сущность из списка
    auto it = std::find(m_entities.begin(), m_entities.end(), entity);
    if (it != m_entities.end()) {
        m_entities.erase(it);
    }
}

bool World::IsEntityValid(Entity entity) const {
    return std::find(m_entities.begin(), m_entities.end(), entity) != m_entities.end();
}

void World::AddSystem(std::unique_ptr<System> system) {
    m_systems.push_back(std::move(system));
}

void World::UpdateSystems(float deltaTime) {
    for (auto& system : m_systems) {
        system->Update(this, deltaTime);
    }
}

void World::Clear() {
    m_systems.clear();
    m_components.clear();
    m_entities.clear();
    m_nextEntityId = 1;
}