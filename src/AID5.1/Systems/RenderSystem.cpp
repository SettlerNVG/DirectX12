#include "RenderSystem.h"
#include "../ECS/World.h"
#include "../Components/Transform.h"
#include "../Components/MeshRenderer.h"
#include "../Components/Hierarchy.h"
#include "../Utils/Logger.h"

using namespace DirectX;

RenderSystem::RenderSystem(RenderAdapter* renderer)
    : m_renderer(renderer)
    , m_lastRenderedCount(0) {
    LOG_INFO("RenderSystem created");
}

void RenderSystem::Update(World* world, float deltaTime) {
    // Получаем все сущности с Transform и MeshRenderer
    auto entities = world->GetEntitiesWith<Transform, MeshRenderer>();
    
    static int logCounter = 0;
    if (logCounter++ % 60 == 0) {
        LOG_DEBUG("RenderSystem: Found " + std::to_string(entities.size()) + " entities to render");
    }
    
    int renderedCount = 0;
    for (Entity entity : entities) {
        auto* transform = world->GetComponent<Transform>(entity);
        auto* meshRenderer = world->GetComponent<MeshRenderer>(entity);
        
        if (!meshRenderer->visible) {
            continue;
        }
        
        // Вычисляем мировую матрицу с учетом иерархии
        XMMATRIX worldMatrix = CalculateWorldMatrix(world, entity);
        
        // Устанавливаем матрицу модели и цвет
        m_renderer->SetModelMatrix(worldMatrix);
        m_renderer->SetColor(
            meshRenderer->color.x,
            meshRenderer->color.y,
            meshRenderer->color.z,
            meshRenderer->color.w
        );
        
        // Рисуем примитив в зависимости от типа
        switch (meshRenderer->primitiveType) {
            case PrimitiveType::Triangle:
                m_renderer->DrawTriangle();
                break;
            case PrimitiveType::Quad:
                m_renderer->DrawQuad();
                break;
            case PrimitiveType::Cube:
                m_renderer->DrawCube();
                break;
        }
        
        renderedCount++;
    }
    
    m_lastRenderedCount = renderedCount;

    if (logCounter % 60 == 0) {
        LOG_DEBUG("RenderSystem: Actually rendered " + std::to_string(renderedCount) + " objects");
    }
}

XMMATRIX RenderSystem::CalculateWorldMatrix(World* world, Entity entity) {
    auto* transform = world->GetComponent<Transform>(entity);
    if (!transform) {
        return XMMatrixIdentity();
    }
    
    // Получаем локальную матрицу
    XMMATRIX localMatrix = transform->GetMatrix();
    
    // Проверяем наличие компонента иерархии
    auto* hierarchy = world->GetComponent<Hierarchy>(entity);
    if (hierarchy && hierarchy->HasParent()) {
        // Рекурсивно вычисляем матрицу родителя
        XMMATRIX parentMatrix = CalculateWorldMatrix(world, hierarchy->parent);
        // ИСПРАВЛЕНО: правильный порядок - сначала локальная, потом родительская
        return parentMatrix * localMatrix;
    }
    
    return localMatrix;
}
