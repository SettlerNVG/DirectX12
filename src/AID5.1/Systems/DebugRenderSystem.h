#pragma once

#include "../ECS/System.h"
#include "../Rendering/RenderAdapter.h"

class World;

// Система отладочной визуализации
class DebugRenderSystem : public System {
public:
    DebugRenderSystem(RenderAdapter* renderer);
    ~DebugRenderSystem() override = default;
    
    void Update(World* world, float deltaTime) override;
    
    // Включить/выключить отрисовку коллайдеров
    void SetDrawColliders(bool enabled) { m_drawColliders = enabled; }
    bool IsDrawingColliders() const { return m_drawColliders; }
    
    // Включить/выключить отрисовку осей координат
    void SetDrawAxes(bool enabled) { m_drawAxes = enabled; }
    bool IsDrawingAxes() const { return m_drawAxes; }

private:
    void DrawColliders(World* world);
    void DrawAxes(World* world);
    void DrawAABB(const DirectX::XMFLOAT3& center, const DirectX::XMFLOAT3& extents, const DirectX::XMFLOAT4& color);
    void DrawSphere(const DirectX::XMFLOAT3& center, float radius, const DirectX::XMFLOAT4& color);
    void DrawWireCube(const DirectX::XMMATRIX& transform, const DirectX::XMFLOAT4& color);
    
    RenderAdapter* m_renderer;
    bool m_drawColliders;
    bool m_drawAxes;
};
