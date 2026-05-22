#pragma once

#include "../ECS/System.h"
#include "../ECS/Entity.h"
#include "../Components/Collider.h"
#include <DirectXMath.h>
#include <vector>

class World;

// Информация о коллизии для разрешения
struct CollisionInfo {
    Entity entityA;
    Entity entityB;
    DirectX::XMFLOAT3 normal;
    float penetrationDepth;
    bool isTrigger;
};

// Система физики
class PhysicsSystem : public System {
public:
    PhysicsSystem();
    ~PhysicsSystem() override = default;
    
    void Update(World* world, float deltaTime) override;
    
    // Настройки физики
    void SetGravity(const DirectX::XMFLOAT3& gravity) { m_gravity = gravity; }
    DirectX::XMFLOAT3 GetGravity() const { return m_gravity; }
    
    void SetFixedTimeStep(float step) { m_fixedTimeStep = step; }
    float GetFixedTimeStep() const { return m_fixedTimeStep; }
    
    // Включить/выключить debug визуализацию
    void SetDebugDraw(bool enabled) { m_debugDraw = enabled; }
    bool IsDebugDrawEnabled() const { return m_debugDraw; }
    
    // Настройки sleeping объектов
    void SetSleepThreshold(float threshold) { m_sleepThreshold = threshold; }
    float GetSleepThreshold() const { return m_sleepThreshold; }
    
    void SetSleepTime(float time) { m_sleepTime = time; }
    float GetSleepTime() const { return m_sleepTime; }

private:
    // Сброс состояний "на земле" в начале кадра
    void ResetGroundedStates(World* world);
    
    // Обновление sleeping состояний объектов
    void UpdateSleepingObjects(World* world, float deltaTime);
    // Применение гравитации
    void ApplyGravity(World* world, float deltaTime);
    
    // Интегрирование физики (обновление позиций)
    void IntegratePhysics(World* world, float deltaTime);
    
    // Обнаружение коллизий
    void DetectCollisions(World* world);
    
    // Разрешение коллизий
    void ResolveCollisions(World* world);
    
    // Проверка коллизии между двумя AABB
    bool CheckAABBCollision(const AABB& a, const AABB& b, CollisionInfo& info);
    
    // Проверка коллизии между двумя сферами
    bool CheckSphereCollision(
        const DirectX::XMFLOAT3& centerA, float radiusA,
        const DirectX::XMFLOAT3& centerB, float radiusB,
        CollisionInfo& info
    );
    
    // Проверка коллизии между AABB и сферой
    bool CheckAABBSphereCollision(
        const AABB& aabb,
        const DirectX::XMFLOAT3& sphereCenter, float sphereRadius,
        CollisionInfo& info
    );
    
    // Получить AABB для сущности
    AABB GetAABB(World* world, Entity entity);
    
    // Разрешить коллизию между двумя объектами
    void ResolveCollision(World* world, const CollisionInfo& collision);
    
    DirectX::XMFLOAT3 m_gravity;
    float m_fixedTimeStep;
    float m_accumulator;
    bool m_debugDraw;
    
    // Sleeping objects optimization
    float m_sleepThreshold;  // Минимальная скорость для засыпания
    float m_sleepTime;       // Время неподвижности до засыпания
    int m_collisionCounter;  // Счетчик коллизий для отладки
    
    // Список обнаруженных коллизий
    std::vector<CollisionInfo> m_collisions;
};
