#pragma once

#include "../ECS/Entity.h"
#include "../ECS/System.h"
#include "../Rendering/RenderAdapter.h"
#include <Windows.h>

class World;

// Система управления камерой с использованием InputManager
class CameraSystem : public System {
public:
    CameraSystem(RenderAdapter* renderer, HWND hwnd);
    ~CameraSystem() override = default;
    
    void Update(World* world, float deltaTime) override;

private:
    void HandleInput(World* world, Entity cameraEntity, float deltaTime);
    void UpdateViewProjection(World* world, Entity cameraEntity);
    
    RenderAdapter* m_renderer;
    HWND m_hwnd;
    
    // Для управления мышью
    bool m_mouseCaptured;
    bool m_firstCapture;
};
