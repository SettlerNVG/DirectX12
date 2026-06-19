#pragma once

#include "../ECS/Entity.h"
#include "../ECS/System.h"
#include "../Rendering/RenderAdapter.h"
#include <DirectXMath.h>

class World;

// Система рендеринга для отрисовки всех сущностей с Transform и MeshRenderer
class RenderSystem : public System {
public:
    RenderSystem(RenderAdapter* renderer);
    ~RenderSystem() override = default;
    
    void Update(World* world, float deltaTime) override;
    int GetLastRenderedCount() const { return m_lastRenderedCount; }

private:
    RenderAdapter* m_renderer;
    int m_lastRenderedCount;
    
    // Вычисление мировой матрицы с учетом иерархии
    DirectX::XMMATRIX CalculateWorldMatrix(World* world, Entity entity);
};
