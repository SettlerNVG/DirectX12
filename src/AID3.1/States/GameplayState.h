#pragma once

#include "GameState.h"
#include "../ECS/Entity.h"
#include <DirectXMath.h>

class GameplayState : public GameState {
public:
    GameplayState(class Application* app);
    ~GameplayState() override = default;
    
    void OnEnter() override;
    void OnExit() override;
    void Update(float deltaTime) override;
    void Render() override;
    
    std::string GetName() const override { return "GameplayState"; }

private:
    void LoadResources();
    void CreateDemoScene();
    void CreateCamera();
    
    // Сущности для демонстрации
    Entity m_camera;
    Entity m_skull1;
    Entity m_skull2;
    Entity m_car;
    
    float m_time;
};