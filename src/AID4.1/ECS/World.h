#pragma once

#include "Entity.h"
#include "System.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <typeindex>
#include <any>

// Центральный реестр сущностей и компонентов
class World {
public:
    World();
    ~World();
    
    // Управление сущностями
    Entity CreateEntity();
    void DestroyEntity(Entity entity);
    bool IsEntityValid(Entity entity) const;
    
    // Управление компонентами
    template<typename T>
    T* AddComponent(Entity entity, const T& component);
    
    template<typename T>
    T* GetComponent(Entity entity);
    
    template<typename T>
    const T* GetComponent(Entity entity) const;
    
    template<typename T>
    bool HasComponent(Entity entity) const;
    
    template<typename T>
    void RemoveComponent(Entity entity);
    
    // Получение всех сущностей с определенными компонентами
    template<typename... Components>
    std::vector<Entity> GetEntitiesWith();
    
    // Управление системами
    void AddSystem(std::unique_ptr<System> system);
    void UpdateSystems(float deltaTime);
    
    template<typename T>
    T* GetSystem();
    
    // Очистка всего мира
    void Clear();

private:
    Entity m_nextEntityId;
    std::vector<Entity> m_entities;
    
    // Хранилище компонентов: тип компонента -> (entity -> компонент)
    std::unordered_map<std::type_index, std::unordered_map<Entity, std::any>> m_components;
    
    // Список систем
    std::vector<std::unique_ptr<System>> m_systems;
    
    // Вспомогательные методы
    template<typename T>
    std::unordered_map<Entity, std::any>& GetComponentMap();
    
    template<typename T>
    const std::unordered_map<Entity, std::any>& GetComponentMap() const;
};

// Реализация шаблонных методов
template<typename T>
T* World::AddComponent(Entity entity, const T& component) {
    auto& componentMap = GetComponentMap<T>();
    componentMap[entity] = component;
    return std::any_cast<T>(&componentMap[entity]);
}

template<typename T>
T* World::GetComponent(Entity entity) {
    auto& componentMap = GetComponentMap<T>();
    auto it = componentMap.find(entity);
    if (it != componentMap.end()) {
        return std::any_cast<T>(&it->second);
    }
    return nullptr;
}

template<typename T>
const T* World::GetComponent(Entity entity) const {
    const auto& componentMap = GetComponentMap<T>();
    auto it = componentMap.find(entity);
    if (it != componentMap.end()) {
        return std::any_cast<T>(&it->second);
    }
    return nullptr;
}

template<typename T>
bool World::HasComponent(Entity entity) const {
    const auto& componentMap = GetComponentMap<T>();
    return componentMap.find(entity) != componentMap.end();
}

template<typename T>
void World::RemoveComponent(Entity entity) {
    auto& componentMap = GetComponentMap<T>();
    componentMap.erase(entity);
}

template<typename... Components>
std::vector<Entity> World::GetEntitiesWith() {
    std::vector<Entity> result;
    
    for (Entity entity : m_entities) {
        bool hasAll = (HasComponent<Components>(entity) && ...);
        if (hasAll) {
            result.push_back(entity);
        }
    }
    
    return result;
}

template<typename T>
std::unordered_map<Entity, std::any>& World::GetComponentMap() {
    std::type_index typeIndex(typeid(T));
    return m_components[typeIndex];
}

template<typename T>
const std::unordered_map<Entity, std::any>& World::GetComponentMap() const {
    std::type_index typeIndex(typeid(T));
    static std::unordered_map<Entity, std::any> emptyMap;
    auto it = m_components.find(typeIndex);
    if (it != m_components.end()) {
        return it->second;
    }
    return emptyMap;
}

template<typename T>
T* World::GetSystem() {
    for (auto& system : m_systems) {
        T* ptr = dynamic_cast<T*>(system.get());
        if (ptr) return ptr;
    }
    return nullptr;
}
