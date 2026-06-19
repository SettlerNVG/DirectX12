#pragma once

#include "GameState.h"

class MenuState : public GameState {
public:
    MenuState(class Application* app);
    ~MenuState() override = default;
    
    void OnEnter() override;
    void OnExit() override;
    void Update(float deltaTime) override;
    void Render() override;
    
    std::string GetName() const override { return "MenuState"; }
};
