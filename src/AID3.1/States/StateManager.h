#pragma once

#include "GameState.h"
#include <memory>
#include <unordered_map>
#include <string>

class StateManager {
public:
    StateManager();
    ~StateManager();
    
    void RegisterState(const std::string& name, std::unique_ptr<GameState> state);
    void ChangeState(const std::string& name);
    
    void Update(float deltaTime);
    void Render();
    
    GameState* GetCurrentState() const { return m_currentState; }

private:
    std::unordered_map<std::string, std::unique_ptr<GameState>> m_states;
    GameState* m_currentState;
    std::string m_currentStateName;
};