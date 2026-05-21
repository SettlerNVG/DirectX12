#pragma once

#include <string>

class Application;

// Базовый класс для всех состояний игры
class GameState {
public:
    virtual ~GameState() = default;
    
    virtual void OnEnter() = 0;
    virtual void OnExit() = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Render() = 0;
    
    virtual std::string GetName() const = 0;
    
protected:
    Application* m_app = nullptr;
};