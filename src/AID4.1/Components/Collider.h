#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <DirectXMath.h>
#include <algorithm>

// Типы коллайдеров
enum class ColliderType {
    Box,      // AABB (Axis-Aligned Bounding Box)
    Sphere    // Сферический коллайдер
};

// Компонент коллайдера
struct Collider {
    ColliderType type;
    DirectX::XMFLOAT3 halfExtents;  // Для Box: половина размеров; Для Sphere: x = радиус
    DirectX::XMFLOAT3 offset;       // Смещение относительно Transform
    bool isTrigger;                 // Триггер (не создаёт физического отталкивания)
    
    Collider()
        : type(ColliderType::Box)
        , halfExtents(0.5f, 0.5f, 0.5f)
        , offset(0.0f, 0.0f, 0.0f)
        , isTrigger(false) {
    }
    
    Collider(ColliderType t, const DirectX::XMFLOAT3& extents)
        : type(t)
        , halfExtents(extents)
        , offset(0.0f, 0.0f, 0.0f)
        , isTrigger(false) {
    }
    
    Collider(ColliderType t, const DirectX::XMFLOAT3& extents, const DirectX::XMFLOAT3& off)
        : type(t)
        , halfExtents(extents)
        , offset(off)
        , isTrigger(false) {
    }
    
    // Создать Box коллайдер
    static Collider CreateBox(float halfWidth, float halfHeight, float halfDepth) {
        return Collider(ColliderType::Box, DirectX::XMFLOAT3(halfWidth, halfHeight, halfDepth));
    }
    
    // Создать Sphere коллайдер
    static Collider CreateSphere(float radius) {
        return Collider(ColliderType::Sphere, DirectX::XMFLOAT3(radius, radius, radius));
    }
    
    // Получить мировую позицию коллайдера
    DirectX::XMFLOAT3 GetWorldCenter(const DirectX::XMFLOAT3& transformPosition) const {
        using namespace DirectX;
        XMVECTOR pos = XMLoadFloat3(&transformPosition);
        XMVECTOR off = XMLoadFloat3(&offset);
        XMVECTOR worldCenter = XMVectorAdd(pos, off);
        
        XMFLOAT3 result;
        XMStoreFloat3(&result, worldCenter);
        return result;
    }
};

// AABB структура для проверки коллизий
struct AABB {
    DirectX::XMFLOAT3 min;
    DirectX::XMFLOAT3 max;
    
    AABB() {
        min = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
        max = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
    }
    
    AABB(const DirectX::XMFLOAT3& center, const DirectX::XMFLOAT3& halfExtents) {
        min.x = center.x - halfExtents.x;
        min.y = center.y - halfExtents.y;
        min.z = center.z - halfExtents.z;
        
        max.x = center.x + halfExtents.x;
        max.y = center.y + halfExtents.y;
        max.z = center.z + halfExtents.z;
    }
    
    DirectX::XMFLOAT3 GetCenter() const {
        return DirectX::XMFLOAT3(
            (min.x + max.x) * 0.5f,
            (min.y + max.y) * 0.5f,
            (min.z + max.z) * 0.5f
        );
    }
    
    DirectX::XMFLOAT3 GetExtents() const {
        return DirectX::XMFLOAT3(
            (max.x - min.x) * 0.5f,
            (max.y - min.y) * 0.5f,
            (max.z - min.z) * 0.5f
        );
    }
    
    // Проверка пересечения с другим AABB
    bool Intersects(const AABB& other) const {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
               (min.y <= other.max.y && max.y >= other.min.y) &&
               (min.z <= other.max.z && max.z >= other.min.z);
    }
    
    // Вычислить глубину проникновения
    DirectX::XMFLOAT3 GetPenetrationDepth(const AABB& other) const {
        DirectX::XMFLOAT3 depth;
        
        float overlapX1 = max.x - other.min.x;
        float overlapX2 = other.max.x - min.x;
        depth.x = ((std::min)(overlapX1, overlapX2) == overlapX1) ? overlapX1 : -overlapX2;
        
        float overlapY1 = max.y - other.min.y;
        float overlapY2 = other.max.y - min.y;
        depth.y = ((std::min)(overlapY1, overlapY2) == overlapY1) ? overlapY1 : -overlapY2;
        
        float overlapZ1 = max.z - other.min.z;
        float overlapZ2 = other.max.z - min.z;
        depth.z = ((std::min)(overlapZ1, overlapZ2) == overlapZ1) ? overlapZ1 : -overlapZ2;
        
        return depth;
    }
};
