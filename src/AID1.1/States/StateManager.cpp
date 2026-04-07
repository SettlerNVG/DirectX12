#include "StateManager.h"
#include "../Utils/Logger.h"

StateManager::StateManager() 
    : m_currentState(nullptr) {
}

StateManager::~StateManager() {
    if (m_currentState) {
        m_currentState->OnExit();
    }
}

void StateManager::RegisterState(const std::string& name, std::unique_ptr<GameState> state) {
    m_states[name] = std::move(state);
    LOG_INFO("State registered: " + name);
}

void StateManager::ChangeState(const std::string& name) {
    auto it = m_states.find(name);
    if (it == m_states.end()) {
        LOG_ERROR("State not found: " + name);
        return;
    }
    
    if (m_currentState) {
        LOG_INFO("Exiting state: " + m_currentStateName);
        m_currentState->OnExit();
    }
    
    m_currentState = it->second.get();
    m_currentStateName = name;
    
    LOG_INFO("Entering state: " + name);
    m_currentState->OnEnter();
}

void StateManager::Update(float deltaTime) {
    if (m_currentState) {
        m_currentState->Update(deltaTime);
    }
}

void StateManager::Render() {
    if (m_currentState) {
        m_currentState->Render();
    }
}
