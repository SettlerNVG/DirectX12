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
    void CreateTestScene();
    void CreateHierarchyDemo();
    void CreateCamera();
    
    // Сущности для демонстрации
    Entity m_camera;
    Entity m_rotatingCube;
    Entity m_movingTriangle;
    Entity m_parentEntity;
    Entity m_childEntity1;
    Entity m_childEntity2;
    
    float m_time;
};
