#pragma once

#include "../ECS/Entity.h"
#include <vector>
#include <algorithm>

// Компонент иерархии для родительско-дочерних отношений
struct Hierarchy {
    Entity parent;
    std::vector<Entity> children;
    
    Hierarchy()
        : parent(NULL_ENTITY) {
    }
    
    Hierarchy(Entity parentEntity)
        : parent(parentEntity) {
    }
    
    void AddChild(Entity child) {
        children.push_back(child);
    }
    
    void RemoveChild(Entity child) {
        auto it = std::find(children.begin(), children.end(), child);
        if (it != children.end()) {
            children.erase(it);
        }
    }
    
    bool HasParent() const {
        return parent != NULL_ENTITY;
    }
    
    bool HasChildren() const {
        return !children.empty();
    }
};
