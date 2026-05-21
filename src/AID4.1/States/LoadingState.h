#pragma once

#include "GameState.h"

class LoadingState : public GameState {
public:
    LoadingState(class Application* app);
    ~LoadingState() override = default;
    
    void OnEnter() override;
    void OnExit() override;
    void Update(float deltaTime) override;
    void Render() override;
    
    std::string GetName() const override { return "LoadingState"; }

private:
    float m_timer;
    float m_loadingDuration;
};
