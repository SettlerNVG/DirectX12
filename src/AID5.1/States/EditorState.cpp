#include "EditorState.h"
#include "../ECS/World.h"
#include "../Physics/PhysicsSystem.h"
#include "../Systems/RenderSystem.h"
#include "../Systems/CameraSystem.h"
#include "../Input/InputManager.h"
#include "../Components/Transform.h"
#include "../Components/Tag.h"
#include "../Components/MeshRenderer.h"
#include "../Components/Rigidbody.h"
#include "../Components/Collider.h"
#include "../Components/Camera.h"
#include "../Resources/ResourceCatalog.h"
#include "../Utils/Logger.h"

// Dear ImGui
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>

// Forward declare the Win32 message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

EditorState::EditorState()
    : m_renderer(nullptr)
    , m_initialized(false)
    , m_editorCameraEntity(0) {
}

EditorState::~EditorState() {
    if (m_initialized) {
        OnExit();
    }
}

bool EditorState::Initialize(HWND hwnd, RenderAdapter* renderer) {
    LOG_INFO("Initializing EditorState...");
    
    m_renderer = renderer;
    
    // Создаем ECS мир
    m_world = std::make_unique<World>();
    
    // Создаем системы с правильными конструкторами
    m_physicsSystem = std::make_unique<PhysicsSystem>();
    m_renderSystem = std::make_unique<RenderSystem>(renderer);
    m_cameraSystem = std::make_unique<CameraSystem>(renderer, hwnd);
    m_inputManager = &InputManager::GetInstance(); // Singleton
    
    // Инициализируем InputManager
    m_inputManager->Initialize(hwnd);
    ResourceCatalog::GetInstance().SetProjectRoot(".");
    ResourceCatalog::GetInstance().Refresh();
    
    // Добавляем системы в мир
    // Примечание: системы не имеют Initialize метода, они инициализируются в конструкторе
    
    // Создаем редактор
    m_editorGUI = std::make_unique<EditorGUI>();
    if (!m_editorGUI->Initialize(hwnd, renderer)) {
        LOG_ERROR("Failed to initialize EditorGUI");
        return false;
    }
    
    // Создаем тестовую сцену
    CreateTestScene();
    
    m_initialized = true;
    LOG_INFO("EditorState initialized successfully");
    return true;
}

void EditorState::OnEnter() {
    LOG_INFO("Entering EditorState");
}

void EditorState::OnExit() {
    LOG_INFO("Exiting EditorState");
    
    if (m_editorGUI) {
        m_editorGUI->Shutdown();
        m_editorGUI.reset();
    }
    
    // InputManager - singleton, не удаляем
    m_inputManager = nullptr;
    
    // Системы удаляются автоматически через unique_ptr
    m_cameraSystem.reset();
    m_renderSystem.reset();
    m_physicsSystem.reset();
    
    m_world.reset();
    m_initialized = false;
}

void EditorState::Update(float deltaTime) {
    if (!m_initialized) return;
    
    // Обновляем ввод
    m_inputManager->Update();
    
    // Обновляем редактор
    m_editorGUI->Update(m_world.get(), deltaTime);
    UpdateCameraAspectFromViewport();
    
    // Обновляем системы в зависимости от режима редактора
    UpdateSystems(deltaTime);
}

void EditorState::Render() {
    if (!m_initialized) return;
    
    // Рендер система не имеет отдельного Render метода
    // Рендеринг происходит в Update
    
    // Рендерим интерфейс редактора
    RenderScene();
    m_editorGUI->Render();
}

bool EditorState::HandleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (!m_initialized) return false;
    
    // Передаем сообщение в ImGui
    return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);
}

void EditorState::CreateTestScene() {
    LOG_INFO("Creating test scene for editor...");
    
    // Создаем камеру редактора
    m_editorCameraEntity = m_world->CreateEntity();
    
    Transform cameraTransform;
    cameraTransform.position = { 0.0f, 5.0f, 10.0f };
    cameraTransform.rotation = { DirectX::XMConvertToRadians(-20.0f), 0.0f, 0.0f };
    cameraTransform.scale = { 1.0f, 1.0f, 1.0f };
    m_world->AddComponent<Transform>(m_editorCameraEntity, cameraTransform);
    
    Camera camera;
    camera.fov = DirectX::XM_PIDIV4;
    camera.nearPlane = 0.1f;
    camera.farPlane = 1000.0f;
    camera.yaw = -DirectX::XM_PIDIV2;
    camera.pitch = DirectX::XMConvertToRadians(-20.0f);
    camera.isActive = true;
    m_world->AddComponent<Camera>(m_editorCameraEntity, camera);
    
    Tag cameraTag;
    cameraTag.name = "Editor Camera";
    m_world->AddComponent<Tag>(m_editorCameraEntity, cameraTag);
    
    // Создаем пол
    uint32_t groundEntity = m_world->CreateEntity();
    
    Transform groundTransform;
    groundTransform.position = { 0.0f, -1.0f, 0.0f };
    groundTransform.rotation = { 0.0f, 0.0f, 0.0f };
    groundTransform.scale = { 20.0f, 1.0f, 20.0f };
    m_world->AddComponent<Transform>(groundEntity, groundTransform);
    
    MeshRenderer groundMesh;
    groundMesh.primitiveType = PrimitiveType::Cube;
    groundMesh.color = { 0.5f, 0.5f, 0.5f, 1.0f };
    m_world->AddComponent<MeshRenderer>(groundEntity, groundMesh);
    
    Collider groundCollider;
    groundCollider.type = ColliderType::Box;
    groundCollider.halfExtents = { 20.0f, 1.0f, 20.0f };
    groundCollider.isTrigger = false;
    m_world->AddComponent<Collider>(groundEntity, groundCollider);
    
    Tag groundTag;
    groundTag.name = "Ground";
    m_world->AddComponent<Tag>(groundEntity, groundTag);
    
    // Создаем несколько тестовых кубов
    for (int i = 0; i < 5; ++i) {
        uint32_t cubeEntity = m_world->CreateEntity();
        
        Transform transform;
        transform.position = { static_cast<float>(i * 2 - 4), 2.0f + i * 2, 0.0f };
        transform.rotation = { 0.0f, DirectX::XMConvertToRadians(static_cast<float>(i * 45)), 0.0f };
        transform.scale = { 1.0f, 1.0f, 1.0f };
        m_world->AddComponent<Transform>(cubeEntity, transform);
        
        MeshRenderer mesh;
        mesh.primitiveType = PrimitiveType::Cube;
        mesh.color = { 
            0.2f + (i * 0.2f), 
            0.3f + (i * 0.15f), 
            0.8f - (i * 0.1f),
            1.0f
        };
        m_world->AddComponent<MeshRenderer>(cubeEntity, mesh);
        
        Rigidbody rigidbody;
        rigidbody.mass = 1.0f;
        rigidbody.useGravity = true;
        rigidbody.isKinematic = false;
        rigidbody.velocity = { 0.0f, 0.0f, 0.0f };
        m_world->AddComponent<Rigidbody>(cubeEntity, rigidbody);
        
        Collider collider;
        collider.type = ColliderType::Box;
        collider.halfExtents = { 1.0f, 1.0f, 1.0f };
        collider.isTrigger = false;
        m_world->AddComponent<Collider>(cubeEntity, collider);
        
        Tag tag;
        tag.name = "Test Cube " + std::to_string(i + 1);
        m_world->AddComponent<Tag>(cubeEntity, tag);
    }
    
    // Создаем сферу (используем куб, так как нет сферы в PrimitiveType)
    uint32_t sphereEntity = m_world->CreateEntity();
    
    Transform sphereTransform;
    sphereTransform.position = { 5.0f, 8.0f, 0.0f };
    sphereTransform.rotation = { 0.0f, 0.0f, 0.0f };
    sphereTransform.scale = { 1.5f, 1.5f, 1.5f };
    m_world->AddComponent<Transform>(sphereEntity, sphereTransform);
    
    MeshRenderer sphereMesh;
    sphereMesh.primitiveType = PrimitiveType::Cube; // Используем куб вместо сферы
    sphereMesh.color = { 1.0f, 0.2f, 0.2f, 1.0f };
    m_world->AddComponent<MeshRenderer>(sphereEntity, sphereMesh);
    
    Rigidbody sphereRigidbody;
    sphereRigidbody.mass = 2.0f;
    sphereRigidbody.useGravity = true;
    sphereRigidbody.isKinematic = false;
    sphereRigidbody.velocity = { -2.0f, 0.0f, 0.0f };
    m_world->AddComponent<Rigidbody>(sphereEntity, sphereRigidbody);
    
    Collider sphereCollider;
    sphereCollider.type = ColliderType::Sphere;
    sphereCollider.halfExtents = { 1.5f, 1.5f, 1.5f }; // Для сферы x = радиус
    sphereCollider.isTrigger = false;
    m_world->AddComponent<Collider>(sphereEntity, sphereCollider);
    
    Tag sphereTag;
    sphereTag.name = "Test Sphere";
    m_world->AddComponent<Tag>(sphereEntity, sphereTag);
    
    LOG_INFO("Test scene created successfully");
}

void EditorState::UpdateSystems(float deltaTime) {
    // Обновляем камеру всегда
    m_cameraSystem->Update(m_world.get(), deltaTime);
    
    // Обновляем физику только в Play режиме
    if (m_editorGUI->IsPlayMode()) {
        m_physicsSystem->Update(m_world.get(), deltaTime);
    }
    
    // Рендер система обновляется всегда для отображения изменений
    if (m_editorGUI) {
        m_editorGUI->SetRuntimeStats(
            m_renderSystem ? m_renderSystem->GetLastRenderedCount() : 0,
            m_physicsSystem ? m_physicsSystem->GetActiveCollisionCount() : 0);
    }
}

void EditorState::RenderScene() {
    if (!m_world || !m_renderSystem || !m_renderer) {
        return;
    }

    if (m_editorGUI && m_editorGUI->HasViewportRect()) {
        m_renderer->SetViewportRect(
            m_editorGUI->GetViewportX(),
            m_editorGUI->GetViewportY(),
            m_editorGUI->GetViewportWidth(),
            m_editorGUI->GetViewportHeight());
    }

    m_renderSystem->Update(m_world.get(), 0.0f);
    m_renderer->ResetViewportRect();
}

void EditorState::UpdateCameraAspectFromViewport() {
    if (!m_editorGUI || !m_world || !m_editorGUI->HasViewportRect()) {
        return;
    }

    auto entities = m_world->GetEntitiesWith<Camera>();
    for (Entity entity : entities) {
        auto* camera = m_world->GetComponent<Camera>(entity);
        if (camera && camera->isActive) {
            camera->aspectRatio = m_editorGUI->GetViewportAspectRatio();
            break;
        }
    }
}
