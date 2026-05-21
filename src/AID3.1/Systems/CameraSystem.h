#pragma once

#include "../ECS/Entity.h"
#include "../ECS/System.h"
#include "../Rendering/RenderAdapter.h"
#include <Windows.h>

class World;

// Система управления камерой
class CameraSystem : public System {
public:
    CameraSystem(RenderAdapter* renderer, HWND hwnd);
    ~CameraSystem() override = default;
    
    void Update(World* world, float deltaTime) override;

private:
    RenderAdapter* m_renderer;
    HWND m_hwnd;
    
    // Для управления мышью
    bool m_firstMouse;
    int m_lastMouseX;
    int m_lastMouseY;
    bool m_mouseCaptured;
    
    void HandleInput(World* world, Entity cameraEntity, float deltaTime);
    void UpdateViewProjection(World* world, Entity cameraEntity);
};