#include "DebugRenderSystem.h"
#include "../ECS/World.h"
#include "../Components/Transform.h"
#include "../Components/Collider.h"
#include "../Utils/Logger.h"

using namespace DirectX;

DebugRenderSystem::DebugRenderSystem(RenderAdapter* renderer)
    : m_renderer(renderer)
    , m_drawColliders(false)
    , m_drawAxes(false) {
    LOG_INFO("DebugRenderSystem created");
}

void DebugRenderSystem::Update(World* world, float deltaTime) {
    if (m_drawColliders) {
        DrawColliders(world);
    }
    
    if (m_drawAxes) {
        DrawAxes(world);
    }
}

void DebugRenderSystem::DrawColliders(World* world) {
    auto entities = world->GetEntitiesWith<Transform, Collider>();
    
    for (Entity entity : entities) {
        auto* transform = world->GetComponent<Transform>(entity);
        auto* collider = world->GetComponent<Collider>(entity);
        
        if (!transform || !collider) {
            continue;
        }
        
        XMFLOAT3 center = collider->GetWorldCenter(transform->position);
        
        // Цвет в зависимости от типа коллайдера
        XMFLOAT4 color = collider->isTrigger ? 
            XMFLOAT4(0.0f, 1.0f, 0.0f, 0.5f) :  // Зелёный для триггеров
            XMFLOAT4(1.0f, 1.0f, 0.0f, 0.5f);   // Жёлтый для обычных
        
        if (collider->type == ColliderType::Box) {
            // Учитываем масштаб
            XMFLOAT3 scaledExtents;
            scaledExtents.x = collider->halfExtents.x * transform->scale.x;
            scaledExtents.y = collider->halfExtents.y * transform->scale.y;
            scaledExtents.z = collider->halfExtents.z * transform->scale.z;
            
            DrawAABB(center, scaledExtents, color);
        }
        else if (collider->type == ColliderType::Sphere) {
            float radius = collider->halfExtents.x * std::max({transform->scale.x, transform->scale.y, transform->scale.z});
            DrawSphere(center, radius, color);
        }
    }
}

void DebugRenderSystem::DrawAxes(World* world) {
    // X - красная ось
    XMMATRIX sx = XMMatrixScaling(2.0f, 0.05f, 0.05f);
    XMMATRIX tx = XMMatrixTranslation(1.0f, 0.0f, 0.0f);
    m_renderer->SetColor(1.0f, 0.0f, 0.0f, 1.0f);
    m_renderer->SetModelMatrix(sx * tx);
    m_renderer->DrawCube();

    // Y - зелёная ось
    XMMATRIX sy = XMMatrixScaling(0.05f, 2.0f, 0.05f);
    XMMATRIX ty = XMMatrixTranslation(0.0f, 1.0f, 0.0f);
    m_renderer->SetColor(0.0f, 1.0f, 0.0f, 1.0f);
    m_renderer->SetModelMatrix(sy * ty);
    m_renderer->DrawCube();

    // Z - синяя ось
    XMMATRIX sz = XMMatrixScaling(0.05f, 0.05f, 2.0f);
    XMMATRIX tz = XMMatrixTranslation(0.0f, 0.0f, 1.0f);
    m_renderer->SetColor(0.0f, 0.0f, 1.0f, 1.0f);
    m_renderer->SetModelMatrix(sz * tz);
    m_renderer->DrawCube();
}

void DebugRenderSystem::DrawAABB(const XMFLOAT3& center, const XMFLOAT3& extents, const XMFLOAT4& color) {
    // Создаём матрицу трансформации для куба
    XMMATRIX scaleMatrix = XMMatrixScaling(extents.x * 2.0f, extents.y * 2.0f, extents.z * 2.0f);
    XMMATRIX translationMatrix = XMMatrixTranslation(center.x, center.y, center.z);
    XMMATRIX transform = scaleMatrix * translationMatrix;
    
    DrawWireCube(transform, color);
}

void DebugRenderSystem::DrawSphere(const XMFLOAT3& center, float radius, const XMFLOAT4& color) {
    // Для простоты рисуем сферу как куб с соответствующим радиусом
    XMFLOAT3 extents(radius, radius, radius);
    DrawAABB(center, extents, color);
}

void DebugRenderSystem::DrawWireCube(const XMMATRIX& transform, const XMFLOAT4& color) {
    // Устанавливаем цвет и матрицу
    m_renderer->SetColor(color.x, color.y, color.z, color.w);
    m_renderer->SetModelMatrix(transform);
    
    // Рисуем куб (в будущем можно добавить wireframe режим)
    m_renderer->DrawCube();
}
