#pragma once

#include "GameState.h"
#include "../ECS/Entity.h"
#include <DirectXMath.h>
#include <vector>

// Состояние игры с физикой
class PhysicsGameplayState : public GameState {
public:
    PhysicsGameplayState(class Application* app);
    ~PhysicsGameplayState() override = default;
    
    void OnEnter() override;
    void OnExit() override;
    void Update(float deltaTime) override;
    void Render() override;
    
    std::string GetName() const override { return "PhysicsGameplayState"; }

private:
    void CreatePhysicsScene();
    void CreateGround();
    void CreateFallingObjects();
    void CreatePlayerControlledObject();
    void CreateCamera();
    void SetupInputBindings();
    void SetupCollisionHandlers();
    
    // Сущности
    Entity m_camera;
    Entity m_ground;
    Entity m_player;
    
    std::vector<Entity> m_fallingObjects;
    
    float m_time;
    bool m_debugDrawEnabled;
    int m_collisionCount;
};
