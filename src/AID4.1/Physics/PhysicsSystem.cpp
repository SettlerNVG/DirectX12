#include "PhysicsSystem.h"
#include "../ECS/World.h"
#include "../Components/Transform.h"
#include "../Components/Rigidbody.h"
#include "../Components/Collider.h"
#include "../Components/Tag.h"
#include "../Events/EventBus.h"
#include "../Events/CollisionEvent.h"
#include "../Utils/Logger.h"
#include <algorithm>

using namespace DirectX;

PhysicsSystem::PhysicsSystem()
    : m_gravity(0.0f, -9.81f, 0.0f)
    , m_fixedTimeStep(1.0f / 60.0f)
    , m_accumulator(0.0f)
    , m_debugDraw(false)
    , m_collisionCounter(0)
    , m_sleepThreshold(0.2f)  // Увеличил порог для более быстрого засыпания
    , m_sleepTime(0.5f) {     // Уменьшил время до засыпания
    LOG_INFO("PhysicsSystem created with gravity: (0, -9.81, 0)");
}

void PhysicsSystem::Update(World* world, float deltaTime) {
    // Используем фиксированный шаг для стабильности физики
    m_accumulator += deltaTime;
    
    int iterations = 0;
    const int maxIterations = 5; // Предотвращаем spiral of death
    
    while (m_accumulator >= m_fixedTimeStep && iterations < maxIterations) {
        // 0. НОВОЕ: Сбрасываем состояние "на земле" в начале каждого кадра
        ResetGroundedStates(world);
        
        // 1. Обновляем sleeping состояния
        UpdateSleepingObjects(world, m_fixedTimeStep);
        
        // 2. Применяем гравитацию
        ApplyGravity(world, m_fixedTimeStep);
        
        // 3. Интегрируем физику (обновляем позиции)
        IntegratePhysics(world, m_fixedTimeStep);
        
        // 4. Обнаруживаем коллизии
        DetectCollisions(world);
        
        // 5. Разрешаем коллизии (несколько итераций для стабильности)
        for (int i = 0; i < 3; ++i) {
            ResolveCollisions(world);
        }
        
        m_accumulator -= m_fixedTimeStep;
        iterations++;
    }
    
    // Если накопилось слишком много времени, сбрасываем
    if (m_accumulator > m_fixedTimeStep * 2.0f) {
        m_accumulator = 0.0f;
    }
}

void PhysicsSystem::UpdateSleepingObjects(World* world, float deltaTime) {
    auto entities = world->GetEntitiesWith<Rigidbody>();
    
    for (Entity entity : entities) {
        auto* rb = world->GetComponent<Rigidbody>(entity);
        
        if (!rb || rb->isKinematic) {
            continue;
        }
        
        // Вычисляем скорость
        XMVECTOR velocity = XMLoadFloat3(&rb->velocity);
        float speed = XMVectorGetX(XMVector3Length(velocity));
        
        // Если объект медленно движется
        if (speed < m_sleepThreshold) {
            rb->sleepTimer += deltaTime;
            
            // Если объект неподвижен достаточно долго - усыпляем
            if (rb->sleepTimer > m_sleepTime) {
                rb->isSleeping = true;
                rb->velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
                rb->acceleration = XMFLOAT3(0.0f, 0.0f, 0.0f);
            }
        } else {
            // Объект движется - просыпаем
            rb->sleepTimer = 0.0f;
            rb->isSleeping = false;
        }
    }
}

void PhysicsSystem::ApplyGravity(World* world, float deltaTime) {
    auto entities = world->GetEntitiesWith<Rigidbody>();
    
    for (Entity entity : entities) {
        auto* rb = world->GetComponent<Rigidbody>(entity);
        
        if (!rb || !rb->useGravity || rb->isKinematic || rb->isSleeping) {
            continue;
        }
        
        // Применяем гравитацию: v += g * dt
        XMVECTOR velocity = XMLoadFloat3(&rb->velocity);
        XMVECTOR gravity = XMLoadFloat3(&m_gravity);
        velocity = XMVectorAdd(velocity, XMVectorScale(gravity, deltaTime));
        XMStoreFloat3(&rb->velocity, velocity);
    }
}

void PhysicsSystem::IntegratePhysics(World* world, float deltaTime) {
    auto entities = world->GetEntitiesWith<Transform, Rigidbody>();
    
    for (Entity entity : entities) {
        auto* transform = world->GetComponent<Transform>(entity);
        auto* rb = world->GetComponent<Rigidbody>(entity);
        
        if (!transform || !rb || rb->isKinematic || rb->isSleeping) {
            continue;
        }
        
        // Применяем ускорение к скорости
        XMVECTOR velocity = XMLoadFloat3(&rb->velocity);
        XMVECTOR acceleration = XMLoadFloat3(&rb->acceleration);
        velocity = XMVectorAdd(velocity, XMVectorScale(acceleration, deltaTime));
        
        // Применяем drag (сопротивление воздуха)
        float dragFactor = 1.0f - (rb->drag * deltaTime);
        if (dragFactor < 0.0f) dragFactor = 0.0f;
        velocity = XMVectorScale(velocity, dragFactor);
        
        XMStoreFloat3(&rb->velocity, velocity);
        
        // Обновляем позицию: p += v * dt
        XMVECTOR position = XMLoadFloat3(&transform->position);
        position = XMVectorAdd(position, XMVectorScale(velocity, deltaTime));
        XMStoreFloat3(&transform->position, position);
        
        // Сбрасываем ускорение
        rb->acceleration = XMFLOAT3(0.0f, 0.0f, 0.0f);
        
        // Проверяем границы мира (защита от падения в бесконечность)
        if (transform->position.y < -50.0f) {
            transform->position.y = 20.0f;
            rb->velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
            rb->isSleeping = false;
            rb->sleepTimer = 0.0f;
        }
    }
}

void PhysicsSystem::DetectCollisions(World* world) {
    m_collisions.clear();
    
    auto entities = world->GetEntitiesWith<Transform, Collider>();
    
    // Проверяем все пары объектов (O(n²) - для оптимизации можно использовать spatial partitioning)
    for (size_t i = 0; i < entities.size(); ++i) {
        for (size_t j = i + 1; j < entities.size(); ++j) {
            Entity entityA = entities[i];
            Entity entityB = entities[j];
            
            auto* colliderA = world->GetComponent<Collider>(entityA);
            auto* colliderB = world->GetComponent<Collider>(entityB);
            auto* transformA = world->GetComponent<Transform>(entityA);
            auto* transformB = world->GetComponent<Transform>(entityB);
            
            if (!colliderA || !colliderB || !transformA || !transformB) {
                continue;
            }
            
            // ОПТИМИЗАЦИЯ: Пропускаем коллизии со спящими объектами
            auto* rbA = world->GetComponent<Rigidbody>(entityA);
            auto* rbB = world->GetComponent<Rigidbody>(entityB);
            
            bool aIsSleeping = (rbA && rbA->isSleeping && !rbA->isKinematic);
            bool bIsSleeping = (rbB && rbB->isSleeping && !rbB->isKinematic);
            
            // Если оба объекта спят - пропускаем
            if (aIsSleeping && bIsSleeping) {
                continue;
            }
            
            CollisionInfo info;
            info.entityA = entityA;
            info.entityB = entityB;
            info.isTrigger = colliderA->isTrigger || colliderB->isTrigger;
            
            bool collided = false;
            
            // Проверяем коллизию в зависимости от типов коллайдеров
            if (colliderA->type == ColliderType::Box && colliderB->type == ColliderType::Box) {
                AABB aabbA = GetAABB(world, entityA);
                AABB aabbB = GetAABB(world, entityB);
                collided = CheckAABBCollision(aabbA, aabbB, info);
            }
            else if (colliderA->type == ColliderType::Sphere && colliderB->type == ColliderType::Sphere) {
                XMFLOAT3 centerA = colliderA->GetWorldCenter(transformA->position);
                XMFLOAT3 centerB = colliderB->GetWorldCenter(transformB->position);
                // ВАЖНО: Учитываем масштаб для радиуса!
                float radiusA = colliderA->halfExtents.x * (std::max)({transformA->scale.x, transformA->scale.y, transformA->scale.z});
                float radiusB = colliderB->halfExtents.x * (std::max)({transformB->scale.x, transformB->scale.y, transformB->scale.z});
                collided = CheckSphereCollision(centerA, radiusA, centerB, radiusB, info);
            }
            else {
                // AABB vs Sphere
                AABB aabb;
                XMFLOAT3 sphereCenter;
                float sphereRadius;
                
                if (colliderA->type == ColliderType::Box) {
                    aabb = GetAABB(world, entityA);
                    sphereCenter = colliderB->GetWorldCenter(transformB->position);
                    // ВАЖНО: Учитываем масштаб!
                    sphereRadius = colliderB->halfExtents.x * (std::max)({transformB->scale.x, transformB->scale.y, transformB->scale.z});
                } else {
                    aabb = GetAABB(world, entityB);
                    sphereCenter = colliderA->GetWorldCenter(transformA->position);
                    // ВАЖНО: Учитываем масштаб!
                    sphereRadius = colliderA->halfExtents.x * (std::max)({transformA->scale.x, transformA->scale.y, transformA->scale.z});
                }
                
                collided = CheckAABBSphereCollision(aabb, sphereCenter, sphereRadius, info);
            }
            
            if (collided) {
                m_collisions.push_back(info);
                
                // ОПТИМИЗАЦИЯ: Пробуждаем спящие объекты при коллизии
                if (aIsSleeping && rbA) {
                    rbA->isSleeping = false;
                    rbA->sleepTimer = 0.0f;
                }
                if (bIsSleeping && rbB) {
                    rbB->isSleeping = false;
                    rbB->sleepTimer = 0.0f;
                }
                
                // ОГРАНИЧЕНИЕ ЧАСТОТЫ: Публикуем событие коллизии только каждую 5-ю коллизию
                m_collisionCounter++;
                if (m_collisionCounter % 5 == 0) {
                    CollisionEvent event(entityA, entityB, info.normal, info.penetrationDepth, info.isTrigger);
                    PUBLISH_EVENT(event);
                }
            }
        }
    }
}

void PhysicsSystem::ResolveCollisions(World* world) {
    for (const auto& collision : m_collisions) {
        if (!collision.isTrigger) {
            ResolveCollision(world, collision);
        }
    }
}

bool PhysicsSystem::CheckAABBCollision(const AABB& a, const AABB& b, CollisionInfo& info) {
    if (!a.Intersects(b)) {
        return false;
    }
    
    // Вычисляем глубину проникновения по каждой оси
    XMFLOAT3 depth = a.GetPenetrationDepth(b);
    
    // Находим ось с минимальной глубиной проникновения
    float absX = fabsf(depth.x);
    float absY = fabsf(depth.y);
    float absZ = fabsf(depth.z);
    
    if (absX < absY && absX < absZ) {
        info.normal = XMFLOAT3(depth.x > 0 ? 1.0f : -1.0f, 0.0f, 0.0f);
        info.penetrationDepth = absX;
    }
    else if (absY < absZ) {
        info.normal = XMFLOAT3(0.0f, depth.y > 0 ? 1.0f : -1.0f, 0.0f);
        info.penetrationDepth = absY;
    }
    else {
        info.normal = XMFLOAT3(0.0f, 0.0f, depth.z > 0 ? 1.0f : -1.0f);
        info.penetrationDepth = absZ;
    }
    
    return true;
}

bool PhysicsSystem::CheckSphereCollision(
    const XMFLOAT3& centerA, float radiusA,
    const XMFLOAT3& centerB, float radiusB,
    CollisionInfo& info) {
    
    XMVECTOR posA = XMLoadFloat3(&centerA);
    XMVECTOR posB = XMLoadFloat3(&centerB);
    XMVECTOR diff = XMVectorSubtract(posB, posA);
    
    float distance = XMVectorGetX(XMVector3Length(diff));
    float radiusSum = radiusA + radiusB;
    
    if (distance >= radiusSum) {
        return false;
    }
    
    info.penetrationDepth = radiusSum - distance;
    
    if (distance > 0.0001f) {
        XMVECTOR normal = XMVector3Normalize(diff);
        XMStoreFloat3(&info.normal, normal);
    } else {
        info.normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
    }
    
    return true;
}

bool PhysicsSystem::CheckAABBSphereCollision(
    const AABB& aabb,
    const XMFLOAT3& sphereCenter, float sphereRadius,
    CollisionInfo& info) {
    
    // Находим ближайшую точку на AABB к центру сферы
    XMFLOAT3 closestPoint;
    closestPoint.x = (std::max)(aabb.min.x, (std::min)(sphereCenter.x, aabb.max.x));
    closestPoint.y = (std::max)(aabb.min.y, (std::min)(sphereCenter.y, aabb.max.y));
    closestPoint.z = (std::max)(aabb.min.z, (std::min)(sphereCenter.z, aabb.max.z));
    
    XMVECTOR closest = XMLoadFloat3(&closestPoint);
    XMVECTOR center = XMLoadFloat3(&sphereCenter);
    XMVECTOR diff = XMVectorSubtract(center, closest);
    
    float distance = XMVectorGetX(XMVector3Length(diff));
    
    if (distance >= sphereRadius) {
        return false;
    }
    
    info.penetrationDepth = sphereRadius - distance;
    
    if (distance > 0.0001f) {
        XMVECTOR normal = XMVector3Normalize(diff);
        XMStoreFloat3(&info.normal, normal);
    } else {
        info.normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
    }
    
    return true;
}

AABB PhysicsSystem::GetAABB(World* world, Entity entity) {
    auto* transform = world->GetComponent<Transform>(entity);
    auto* collider = world->GetComponent<Collider>(entity);
    
    if (!transform || !collider) {
        return AABB();
    }
    
    XMFLOAT3 center = collider->GetWorldCenter(transform->position);
    
    // Учитываем масштаб трансформа
    XMFLOAT3 scaledExtents;
    scaledExtents.x = collider->halfExtents.x * transform->scale.x;
    scaledExtents.y = collider->halfExtents.y * transform->scale.y;
    scaledExtents.z = collider->halfExtents.z * transform->scale.z;
    
    return AABB(center, scaledExtents);
}

void PhysicsSystem::ResolveCollision(World* world, const CollisionInfo& collision) {
    auto* rbA = world->GetComponent<Rigidbody>(collision.entityA);
    auto* rbB = world->GetComponent<Rigidbody>(collision.entityB);
    auto* transformA = world->GetComponent<Transform>(collision.entityA);
    auto* transformB = world->GetComponent<Transform>(collision.entityB);
    
    if (!transformA || !transformB) {
        return;
    }
    
    bool hasRbA = (rbA != nullptr && !rbA->isKinematic);
    bool hasRbB = (rbB != nullptr && !rbB->isKinematic);
    
    if (!hasRbA && !hasRbB) {
        return; // Оба статичные
    }
    
    XMVECTOR normal = XMLoadFloat3(&collision.normal);
    float depth = collision.penetrationDepth;
    
    // ВАЖНО: Если проникновение слишком большое - это ошибка, пропускаем
    if (depth > 2.0f) {
        return;
    }
    
    // Минимальная глубина для обработки
    if (depth < 0.0001f) {
        return;
    }
    
    // Определяем, какой объект касается земли
    auto* tagA = world->GetComponent<Tag>(collision.entityA);
    auto* tagB = world->GetComponent<Tag>(collision.entityB);
    
    bool aIsGround = (tagA && tagA->name == "Ground");
    bool bIsGround = (tagB && tagB->name == "Ground");
    
    // НОВАЯ ЛОГИКА: Обрабатываем коллизии с землей особым образом
    if (hasRbA && bIsGround) {
        // A динамический, B земля
        XMVECTOR posA = XMLoadFloat3(&transformA->position);
        
        // КРИТИЧНО: Точно позиционируем объект НА поверхности земли
        float groundY = transformB->position.y + (transformB->scale.y * 0.5f); // Верх земли
        float objectHalfHeight = transformA->scale.y * 0.5f; // Половина высоты объекта
        float targetY = groundY + objectHalfHeight; // Где должен быть центр объекта
        
        // Если объект проваливается в землю - ставим точно на поверхность
        if (transformA->position.y < targetY + 0.01f) {
            transformA->position.y = targetY;
            
            // Отмечаем контакт с землей
            rbA->isGrounded = true;
            rbA->groundContactTime += 0.016f; // ~60 FPS
            
            // СТАБИЛИЗАЦИЯ: Если объект на земле и движется медленно
            if (rbA->groundContactTime > 0.1f && fabsf(rbA->velocity.y) < 1.0f) {
                rbA->velocity.y = 0.0f; // Полностью останавливаем вертикальное движение
                
                // Если горизонтальная скорость тоже маленькая - сильно затухаем
                if (sqrtf(rbA->velocity.x * rbA->velocity.x + rbA->velocity.z * rbA->velocity.z) < 0.5f) {
                    rbA->velocity.x *= 0.5f;
                    rbA->velocity.z *= 0.5f;
                }
            }
            
            // Применяем отскок только если скорость значительная
            if (fabsf(rbA->velocity.y) > 0.5f) {
                rbA->velocity.y = -rbA->velocity.y * rbA->restitution;
            }
        }
    }
    else if (hasRbB && aIsGround) {
        // B динамический, A земля
        XMVECTOR posB = XMLoadFloat3(&transformB->position);
        
        float groundY = transformA->position.y + (transformA->scale.y * 0.5f);
        float objectHalfHeight = transformB->scale.y * 0.5f;
        float targetY = groundY + objectHalfHeight;
        
        if (transformB->position.y < targetY + 0.01f) {
            transformB->position.y = targetY;
            
            rbB->isGrounded = true;
            rbB->groundContactTime += 0.016f;
            
            if (rbB->groundContactTime > 0.1f && fabsf(rbB->velocity.y) < 1.0f) {
                rbB->velocity.y = 0.0f;
                
                if (sqrtf(rbB->velocity.x * rbB->velocity.x + rbB->velocity.z * rbB->velocity.z) < 0.5f) {
                    rbB->velocity.x *= 0.5f;
                    rbB->velocity.z *= 0.5f;
                }
            }
            
            if (fabsf(rbB->velocity.y) > 0.5f) {
                rbB->velocity.y = -rbB->velocity.y * rbB->restitution;
            }
        }
    }
    else if (hasRbA && hasRbB) {
        // Оба динамические - стандартная обработка
        float massA = rbA->mass;
        float massB = rbB->mass;
        float totalMass = massA + massB;
        
        float ratioA = massB / totalMass;
        float ratioB = massA / totalMass;
        
        XMVECTOR posA = XMLoadFloat3(&transformA->position);
        XMVECTOR posB = XMLoadFloat3(&transformB->position);
        
        posA = XMVectorSubtract(posA, XMVectorScale(normal, depth * ratioA * 0.5f));
        posB = XMVectorAdd(posB, XMVectorScale(normal, depth * ratioB * 0.5f));
        
        XMStoreFloat3(&transformA->position, posA);
        XMStoreFloat3(&transformB->position, posB);
        
        // Применяем импульс
        XMVECTOR velA = XMLoadFloat3(&rbA->velocity);
        XMVECTOR velB = XMLoadFloat3(&rbB->velocity);
        XMVECTOR relativeVel = XMVectorSubtract(velA, velB);
        
        float velAlongNormal = XMVectorGetX(XMVector3Dot(relativeVel, normal));
        
        if (velAlongNormal < 0) {
            float restitution = (std::min)(rbA->restitution, rbB->restitution);
            float j = -(1.0f + restitution) * velAlongNormal;
            j /= (1.0f / massA + 1.0f / massB);
            
            XMVECTOR impulse = XMVectorScale(normal, j);
            
            velA = XMVectorAdd(velA, XMVectorScale(impulse, 1.0f / massA));
            velB = XMVectorSubtract(velB, XMVectorScale(impulse, 1.0f / massB));
            
            XMStoreFloat3(&rbA->velocity, velA);
            XMStoreFloat3(&rbB->velocity, velB);
        }
    }
}
void PhysicsSystem::ResetGroundedStates(World* world) {
    auto entities = world->GetEntitiesWith<Rigidbody>();
    
    for (Entity entity : entities) {
        auto* rb = world->GetComponent<Rigidbody>(entity);
        
        if (!rb || rb->isKinematic) {
            continue;
        }
        
        // Сбрасываем состояние "на земле" - будет установлено заново при коллизии
        if (!rb->isGrounded) {
            rb->groundContactTime = 0.0f;
        } else {
            // Если объект был на земле, но теперь движется быстро - сбрасываем
            if (fabsf(rb->velocity.y) > 1.0f) {
                rb->isGrounded = false;
                rb->groundContactTime = 0.0f;
            }
        }
    }
}