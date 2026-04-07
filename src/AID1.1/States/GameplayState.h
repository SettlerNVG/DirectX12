#pragma once

#include "GameState.h"

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
    float m_objectX;
    float m_objectY;
    float m_objectScale;
    float m_objectRotation;
    
    float m_moveSpeed;
    float m_rotationSpeed;
    float m_scaleSpeed;
    
    float m_currentFPS;
    float m_fpsUpdateTimer;
};
