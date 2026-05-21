#include "RenderSystem.h"
#include "../ECS/World.h"
#include "../Components/Transform.h"
#include "../Components/MeshRenderer.h"
#include "../Components/Hierarchy.h"
#include "../Utils/Logger.h"

using namespace DirectX;

RenderSystem::RenderSystem(RenderAdapter* renderer)
    : m_renderer(renderer) {
    LOG_INFO("RenderSystem created (Resource Manager version)");
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
        
        // Проверяем готовность ресурсов
        if (!meshRenderer->IsReady()) {
            if (logCounter % 60 == 0) {
                LOG_WARNING("Entity has incomplete resources, skipping render");
            }
            continue;
        }
        
        // Вычисляем мировую матрицу с учетом иерархии
        XMMATRIX worldMatrix = CalculateWorldMatrix(world, entity);
        
        // Устанавливаем матрицу модели
        m_renderer->SetModelMatrix(worldMatrix);
        
        // Устанавливаем текстуру
        if (meshRenderer->texture && meshRenderer->texture->GetGPUTexture()) {
            m_renderer->SetTexture(meshRenderer->texture->GetGPUTexture());
        }
        
        // Рисуем меш
        if (meshRenderer->mesh && meshRenderer->mesh->GetVertexBuffer() && meshRenderer->mesh->GetIndexBuffer()) {
            m_renderer->DrawMesh(
                meshRenderer->mesh->GetVertexBuffer(),
                meshRenderer->mesh->GetIndexBuffer(),
                meshRenderer->mesh->GetIndices().size(),
                meshRenderer->shader ? meshRenderer->shader->GetPipelineState() : nullptr
            );
        }
        
        renderedCount++;
    }
    
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
        return parentMatrix * localMatrix;
    }
    
    return localMatrix;
}