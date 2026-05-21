#pragma once

#include "../ECS/Entity.h"
#include <DirectXMath.h>

// Событие коллизии
struct CollisionEvent {
    Entity entityA;
    Entity entityB;
    DirectX::XMFLOAT3 normal;           // Нормаль столкновения
    DirectX::XMFLOAT3 contactPoint;     // Точка контакта
    float penetrationDepth;              // Глубина проникновения
    bool isTrigger;                      // Это триггер или физическая коллизия
    
    CollisionEvent()
        : entityA(NULL_ENTITY)
        , entityB(NULL_ENTITY)
        , normal(0, 0, 0)
        , contactPoint(0, 0, 0)
        , penetrationDepth(0.0f)
        , isTrigger(false) {
    }
    
    CollisionEvent(Entity a, Entity b, const DirectX::XMFLOAT3& n, float depth, bool trigger = false)
        : entityA(a)
        , entityB(b)
        , normal(n)
        , contactPoint(0, 0, 0)
        , penetrationDepth(depth)
        , isTrigger(trigger) {
    }
};
