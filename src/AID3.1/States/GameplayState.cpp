#include "GameplayState.h"
#include "../Core/Application.h"
#include "../ECS/World.h"
#include "../Components/Transform.h"
#include "../Components/MeshRenderer.h"
#include "../Components/Tag.h"
#include "../Components/Camera.h"
#include "../Systems/RenderSystem.h"
#include "../Systems/CameraSystem.h"
#include "../Resources/ResourceManager.h"
#include "../Utils/Logger.h"
#include <Windows.h>

using namespace DirectX;

GameplayState::GameplayState(Application* app)
    : m_time(0.0f) {
    m_app = app;
}

void GameplayState::OnEnter() {
    LOG_INFO("Entering Gameplay State - Resource Manager Demo");
    SetWindowTextW(m_app->GetWindowHandle(), L"Resource Manager Demo | RMB+WASD to fly, ESC to menu");
    
    auto* world = m_app->GetWorld();
    
    // Очищаем мир
    world->Clear();
    
    // Добавляем системы
    world->AddSystem(std::make_unique<CameraSystem>(m_app->GetRenderer(), m_app->GetWindowHandle()));
    world->AddSystem(std::make_unique<RenderSystem>(m_app->GetRenderer()));
    
    // Загружаем ресурсы
    LoadResources();
    
    // Создаем камеру
    CreateCamera();
    
    // Создаем демо-сцену с загруженными моделями
    CreateDemoScene();
    
    m_time = 0.0f;
    
    LOG_INFO("Demo scene created with loaded 3D models");
}

void GameplayState::OnExit() {
    LOG_INFO("Exiting Gameplay State");
    
    // Очищаем мир
    auto* world = m_app->GetWorld();
    world->Clear();
}

void GameplayState::LoadResources() {
    LOG_INFO("=== Loading Resources via ResourceManager ===");
    
    auto& resourceManager = ResourceManager::GetInstance();
    auto* hotReloadWatcher = m_app->GetHotReloadWatcher();
    
    // Загружаем модели (используем модели из Chapter 11)
    auto skullMesh = resourceManager.LoadMesh("../Chapter 11 Stenciling/StencilDemo/Models/skull.txt");
    if (skullMesh) {
        LOG_INFO("Skull mesh loaded successfully!");
    } else {
        LOG_ERROR("Failed to load skull mesh");
    }
    
    auto carMesh = resourceManager.LoadMesh("../Chapter 11 Stenciling/StencilDemo/Models/car.txt");
    if (carMesh) {
        LOG_INFO("Car mesh loaded successfully!");
    } else {
        LOG_ERROR("Failed to load car mesh");
    }
    
    // Загружаем текстуры (пока заглушки)
    auto defaultTexture = resourceManager.LoadTexture("assets/default.png");
    if (defaultTexture) {
        LOG_INFO("Default texture loaded");
        // Добавляем в Hot Reload Watcher
        if (hotReloadWatcher) {
            hotReloadWatcher->WatchTexture("assets/default.png");
        }
    }
    
    // Загружаем шейдер (пока заглушка)
    auto basicShader = resourceManager.LoadShader("assets/basic.hlsl");
    if (basicShader) {
        LOG_INFO("Basic shader loaded");
        // Добавляем в Hot Reload Watcher (БОНУСНОЕ ЗАДАНИЕ)
        if (hotReloadWatcher) {
            hotReloadWatcher->WatchShader("assets/basic.hlsl");
            LOG_INFO("Shader added to hot reload watcher - changes will be detected automatically!");
        }
    }
    
    LOG_INFO("=== Resource Loading Complete ===");
    LOG_INFO("=== Hot Reload enabled for shaders and textures ===");
}

void GameplayState::CreateDemoScene() {
    auto* world = m_app->GetWorld();
    auto& resourceManager = ResourceManager::GetInstance();
    
    LOG_INFO("Creating demo scene with loaded models...");
    
    // Получаем загруженные ресурсы из кэша
    auto skullMesh = resourceManager.LoadMesh("../Chapter 11 Stenciling/StencilDemo/Models/skull.txt");
    auto carMesh = resourceManager.LoadMesh("../Chapter 11 Stenciling/StencilDemo/Models/car.txt");
    auto defaultTexture = resourceManager.LoadTexture("assets/default.png");
    auto basicShader = resourceManager.LoadShader("assets/basic.hlsl");
    
    // 1. Череп слева (вращающийся)
    m_skull1 = world->CreateEntity();
    world->AddComponent(m_skull1, Tag("Skull 1"));
    world->AddComponent(m_skull1, Transform(
        XMFLOAT3(-3.0f, 0.0f, 5.0f),
        XMFLOAT3(0.0f, 0.0f, 0.0f),
        XMFLOAT3(0.05f, 0.05f, 0.05f)  // Масштаб для skull модели
    ));
    world->AddComponent(m_skull1, MeshRenderer(skullMesh, defaultTexture, basicShader));
    LOG_INFO("Created Skull 1 at (-3, 0, 5)");
    
    // 2. Череп справа (статичный)
    m_skull2 = world->CreateEntity();
    world->AddComponent(m_skull2, Tag("Skull 2"));
    world->AddComponent(m_skull2, Transform(
        XMFLOAT3(3.0f, 0.0f, 5.0f),
        XMFLOAT3(0.0f, XM_PI, 0.0f),  // Повернут на 180 градусов
        XMFLOAT3(0.05f, 0.05f, 0.05f)
    ));
    world->AddComponent(m_skull2, MeshRenderer(skullMesh, defaultTexture, basicShader));
    LOG_INFO("Created Skull 2 at (3, 0, 5)");
    
    // 3. Машина в центре
    m_car = world->CreateEntity();
    world->AddComponent(m_car, Tag("Car"));
    world->AddComponent(m_car, Transform(
        XMFLOAT3(0.0f, -1.0f, 8.0f),
        XMFLOAT3(0.0f, 0.0f, 0.0f),
        XMFLOAT3(0.02f, 0.02f, 0.02f)  // Масштаб для car модели
    ));
    world->AddComponent(m_car, MeshRenderer(carMesh, defaultTexture, basicShader));
    LOG_INFO("Created Car at (0, -1, 8)");
    
    LOG_INFO("=== Demo scene created: 2 skulls + 1 car ===");
    LOG_INFO("=== Demonstrating resource caching - same meshes reused ===");
}

void GameplayState::CreateCamera() {
    auto* world = m_app->GetWorld();
    
    // Создаем камеру
    m_camera = world->CreateEntity();
    world->AddComponent(m_camera, Tag("Main Camera"));
    
    // Позиция камеры - сзади и выше
    world->AddComponent(m_camera, Transform(
        XMFLOAT3(0.0f, 2.0f, -5.0f),
        XMFLOAT3(0.0f, 0.0f, 0.0f),
        XMFLOAT3(1.0f, 1.0f, 1.0f)
    ));
    
    Camera cameraComponent;
    cameraComponent.aspectRatio = 800.0f / 600.0f;
    cameraComponent.moveSpeed = 5.0f;
    cameraComponent.lookSpeed = 0.002f;
    cameraComponent.yaw = XM_PIDIV2;  // Смотрит вперед
    cameraComponent.pitch = 0.0f;
    world->AddComponent(m_camera, cameraComponent);
    
    LOG_INFO("Created camera at (0, 2, -5) looking forward");
}

void GameplayState::Update(float deltaTime) {
    m_time += deltaTime;
    
    auto* world = m_app->GetWorld();
    
    // Анимация первого черепа (вращение)
    if (world->IsEntityValid(m_skull1)) {
        auto* transform = world->GetComponent<Transform>(m_skull1);
        if (transform) {
            transform->rotation.y = m_time * 0.5f;
        }
    }
    
    // Анимация машины (движение вверх-вниз)
    if (world->IsEntityValid(m_car)) {
        auto* transform = world->GetComponent<Transform>(m_car);
        if (transform) {
            transform->position.y = -1.0f + sinf(m_time) * 0.5f;
            transform->rotation.y = m_time * 0.3f;
        }
    }
}

void GameplayState::Render() {
    auto renderer = m_app->GetRenderer();
    renderer->Clear(0.1f, 0.1f, 0.15f, 1.0f);
    
    // ECS системы рендерят объекты автоматически
    
    // Рисуем UI текст
    HDC hdc = GetDC(m_app->GetWindowHandle());
    if (hdc) {
        SetBkMode(hdc, TRANSPARENT);
        
        HFONT hFont = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
        
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
        SetTextColor(hdc, RGB(255, 255, 255));
        
        TextOutW(hdc, 10, 10, L"Resource Manager Demo", 21);
        TextOutW(hdc, 10, 35, L"Loaded Models: Skull (x2), Car (x1)", 36);
        TextOutW(hdc, 10, 60, L"Hold RMB + Move Mouse - Look around", 36);
        TextOutW(hdc, 10, 85, L"WASD - Move camera", 18);
        TextOutW(hdc, 10, 110, L"Q/E - Move Up/Down", 18);
        TextOutW(hdc, 10, 135, L"ESC - Back to menu", 18);
        
        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);
        ReleaseDC(m_app->GetWindowHandle(), hdc);
    }
}