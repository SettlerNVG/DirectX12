#pragma once

#include "GameState.h"
#include "../Editor/EditorGUI.h"
#include <memory>

// Forward declarations
class World;
class RenderAdapter;
class PhysicsSystem;
class RenderSystem;
class CameraSystem;
class InputManager;

// Состояние редактора - основное состояние для AID5.1
class EditorState : public GameState {
public:
    EditorState();
    virtual ~EditorState();
    
    // GameState interface
    void OnEnter() override;
    void OnExit() override;
    void Update(float deltaTime) override;
    void Render() override;
    std::string GetName() const override { return "EditorState"; }
    
    // Инициализация с параметрами приложения
    bool Initialize(HWND hwnd, RenderAdapter* renderer);
    
    // Обработка сообщений Windows для ImGui
    bool HandleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    // Создание тестовой сцены
    void CreateTestScene();
    
    // Обновление систем в зависимости от режима
    void UpdateSystems(float deltaTime);
    void RenderScene();
    void UpdateCameraAspectFromViewport();
    
    // ECS компоненты
    std::unique_ptr<World> m_world;
    
    // Системы
    std::unique_ptr<PhysicsSystem> m_physicsSystem;
    std::unique_ptr<RenderSystem> m_renderSystem;
    std::unique_ptr<CameraSystem> m_cameraSystem;
    
    // Редактор
    std::unique_ptr<EditorGUI> m_editorGUI;
    
    // Менеджер ввода (singleton)
    InputManager* m_inputManager;
    
    // Рендерер
    RenderAdapter* m_renderer;
    
    // Инициализирован ли редактор
    bool m_initialized;
    
    // Камера для редактора
    uint32_t m_editorCameraEntity;
};
