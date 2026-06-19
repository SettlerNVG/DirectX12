#include "GameplayState.h"
#include "../Core/Application.h"
#include "../ECS/World.h"
#include "../Components/Transform.h"
#include "../Components/MeshRenderer.h"
#include "../Components/Tag.h"
#include "../Components/Hierarchy.h"
#include "../Components/Camera.h"
#include "../Systems/RenderSystem.h"
#include "../Systems/CameraSystem.h"
#include "../Utils/Logger.h"
#include <Windows.h>

using namespace DirectX;

GameplayState::GameplayState(Application* app)
    : m_time(0.0f) {
    m_app = app;
}

void GameplayState::OnEnter() {
    LOG_INFO("Entering Gameplay State");
    SetWindowTextW(m_app->GetWindowHandle(), L"ECS Demo - Gameplay | RMB+WASD to fly, ESC to menu");
    
    auto* world = m_app->GetWorld();
    
    // Очищаем мир
    world->Clear();
    
    // Добавляем системы
    world->AddSystem(std::make_unique<CameraSystem>(m_app->GetRenderer(), m_app->GetWindowHandle()));
    world->AddSystem(std::make_unique<RenderSystem>(m_app->GetRenderer()));
    
    // Создаем камеру
    CreateCamera();
    
    // Создаем тестовую сцену
    CreateTestScene();
    CreateHierarchyDemo();
    
    m_time = 0.0f;
    
    LOG_INFO("Test scene created with multiple entities");
}

void GameplayState::OnExit() {
    LOG_INFO("Exiting Gameplay State");
    
    // Очищаем мир
    auto* world = m_app->GetWorld();
    world->Clear();
}

void GameplayState::CreateTestScene() {
    auto* world = m_app->GetWorld();
    
    LOG_INFO("Creating LARGE visible scene...");
    
    // 1. ОГРОМНЫЙ центральный куб (ОРАНЖЕВЫЙ)
    m_rotatingCube = world->CreateEntity();
    world->AddComponent(m_rotatingCube, Tag("Rotating Cube"));
    world->AddComponent(m_rotatingCube, Transform(
        XMFLOAT3(0.0f, 0.0f, 5.0f),  // ЦЕНТР
        XMFLOAT3(0.0f, 0.0f, 0.0f),
        XMFLOAT3(2.0f, 2.0f, 2.0f)  // БОЛЬШОЙ
    ));
    world->AddComponent(m_rotatingCube, MeshRenderer(
        PrimitiveType::Cube,
        XMFLOAT4(1.0f, 0.5f, 0.0f, 1.0f)  // Оранжевый
    ));
    LOG_INFO("Created LARGE Rotating Cube at (0, 0, 5) scale=2 - ORANGE");
    
    // 2. ОГРОМНЫЙ треугольник СЛЕВА (ЗЕЛЕНЫЙ)
    m_movingTriangle = world->CreateEntity();
    world->AddComponent(m_movingTriangle, Tag("Moving Triangle"));
    world->AddComponent(m_movingTriangle, Transform(
        XMFLOAT3(-4.0f, 0.0f, 5.0f),  // СЛЕВА
        XMFLOAT3(0.0f, 0.0f, 0.0f),
        XMFLOAT3(2.0f, 2.0f, 2.0f)  // БОЛЬШОЙ
    ));
    world->AddComponent(m_movingTriangle, MeshRenderer(
        PrimitiveType::Triangle,
        XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)  // Зеленый
    ));
    LOG_INFO("Created LARGE Moving Triangle at (-4, 0, 5) scale=2 - GREEN");
    
    // 3. ОГРОМНЫЙ квадрат СПРАВА (СИНИЙ)
    Entity rightQuad = world->CreateEntity();
    world->AddComponent(rightQuad, Tag("Right Quad"));
    world->AddComponent(rightQuad, Transform(
        XMFLOAT3(4.0f, 0.0f, 5.0f),  // СПРАВА
        XMFLOAT3(0.0f, 0.0f, 0.0f),
        XMFLOAT3(2.0f, 2.0f, 2.0f)  // БОЛЬШОЙ
    ));
    world->AddComponent(rightQuad, MeshRenderer(
        PrimitiveType::Quad,
        XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f)  // Синий
    ));
    LOG_INFO("Created LARGE Right Quad at (4, 0, 5) scale=2 - BLUE");
    
    // 4. ОГРОМНЫЙ куб СВЕРХУ (РОЗОВЫЙ)
    Entity topCube = world->CreateEntity();
    world->AddComponent(topCube, Tag("Top Cube"));
    world->AddComponent(topCube, Transform(
        XMFLOAT3(0.0f, 3.0f, 5.0f),  // СВЕРХУ
        XMFLOAT3(0.0f, 0.0f, 0.0f),
        XMFLOAT3(1.5f, 1.5f, 1.5f)  // БОЛЬШОЙ
    ));
    world->AddComponent(topCube, MeshRenderer(
        PrimitiveType::Cube,
        XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f)  // Розовый
    ));
    LOG_INFO("Created LARGE Top Cube at (0, 3, 5) scale=1.5 - PINK");
    
    // 5. ОГРОМНЫЙ треугольник СНИЗУ (ЖЕЛТЫЙ)
    Entity bottomTriangle = world->CreateEntity();
    world->AddComponent(bottomTriangle, Tag("Bottom Triangle"));
    world->AddComponent(bottomTriangle, Transform(
        XMFLOAT3(0.0f, -3.0f, 5.0f),  // СНИЗУ
        XMFLOAT3(0.0f, 0.0f, 0.0f),
        XMFLOAT3(2.0f, 2.0f, 2.0f)  // БОЛЬШОЙ
    ));
    world->AddComponent(bottomTriangle, MeshRenderer(
        PrimitiveType::Triangle,
        XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f)  // Желтый
    ));
    LOG_INFO("Created LARGE Bottom Triangle at (0, -3, 5) scale=2 - YELLOW");
    
    LOG_INFO("=== Created 5 LARGE objects in CROSS pattern at Z=5 ===");
}

void GameplayState::CreateHierarchyDemo() {
    auto* world = m_app->GetWorld();
    
    LOG_INFO("Creating LARGE hierarchy demo...");
    
    // Родительский куб (КРАСНЫЙ) слева - БОЛЬШОЙ
    m_parentEntity = world->CreateEntity();
    world->AddComponent(m_parentEntity, Tag("Parent Cube"));
    world->AddComponent(m_parentEntity, Transform(
        XMFLOAT3(-6.0f, 0.0f, 8.0f),  // Слева и дальше
        XMFLOAT3(0.0f, 0.0f, 0.0f),
        XMFLOAT3(1.5f, 1.5f, 1.5f)  // БОЛЬШОЙ
    ));
    world->AddComponent(m_parentEntity, MeshRenderer(
        PrimitiveType::Cube,
        XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)  // КРАСНЫЙ
    ));
    world->AddComponent(m_parentEntity, Hierarchy());
    
    LOG_INFO("Created LARGE Parent Cube at (-6, 0, 8) scale=1.5 - RED");
    
    // Дочерний треугольник (БЕЛЫЙ) - БОЛЬШОЙ
    m_childEntity1 = world->CreateEntity();
    world->AddComponent(m_childEntity1, Tag("Child Triangle"));
    world->AddComponent(m_childEntity1, Transform(
        XMFLOAT3(2.0f, 0.0f, 0.0f),  // Локально справа от родителя
        XMFLOAT3(0.0f, 0.0f, 0.0f),
        XMFLOAT3(0.8f, 0.8f, 0.8f)  // БОЛЬШОЙ
    ));
    world->AddComponent(m_childEntity1, MeshRenderer(
        PrimitiveType::Triangle,
        XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)  // БЕЛЫЙ
    ));
    
    Hierarchy childHierarchy1(m_parentEntity);
    world->AddComponent(m_childEntity1, childHierarchy1);
    
    auto* parentHierarchy = world->GetComponent<Hierarchy>(m_parentEntity);
    parentHierarchy->AddChild(m_childEntity1);
    
    LOG_INFO("Created LARGE Child Triangle scale=0.8 - WHITE");
    
    // Дочерний квадрат (ГОЛУБОЙ) - БОЛЬШОЙ
    m_childEntity2 = world->CreateEntity();
    world->AddComponent(m_childEntity2, Tag("Child Quad"));
    world->AddComponent(m_childEntity2, Transform(
        XMFLOAT3(-2.0f, 0.0f, 0.0f),  // Локально слева от родителя
        XMFLOAT3(0.0f, 0.0f, 0.0f),
        XMFLOAT3(0.8f, 0.8f, 0.8f)  // БОЛЬШОЙ
    ));
    world->AddComponent(m_childEntity2, MeshRenderer(
        PrimitiveType::Quad,
        XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f)  // ГОЛУБОЙ
    ));
    
    Hierarchy childHierarchy2(m_parentEntity);
    world->AddComponent(m_childEntity2, childHierarchy2);
    
    parentHierarchy->AddChild(m_childEntity2);
    
    LOG_INFO("Created LARGE Child Quad scale=0.8 - CYAN");
    LOG_INFO("=== LARGE Hierarchy at Z=8: RED parent + WHITE triangle + CYAN quad ===");
}

void GameplayState::Update(float deltaTime) {
    m_time += deltaTime;
    
    auto* world = m_app->GetWorld();
    
    // Анимация вращающегося куба
    if (world->IsEntityValid(m_rotatingCube)) {
        auto* transform = world->GetComponent<Transform>(m_rotatingCube);
        if (transform) {
            transform->rotation.x = m_time * 0.5f;
            transform->rotation.y = m_time * 0.7f;
            transform->rotation.z = m_time * 0.3f;
        }
    }
    
    // Анимация движущегося треугольника (вверх-вниз)
    if (world->IsEntityValid(m_movingTriangle)) {
        auto* transform = world->GetComponent<Transform>(m_movingTriangle);
        if (transform) {
            transform->position.y = sinf(m_time * 2.0f) * 1.5f;
            transform->rotation.z = m_time;
        }
    }
    
    // Анимация родительского объекта (вращение вокруг Y)
    if (world->IsEntityValid(m_parentEntity)) {
        auto* transform = world->GetComponent<Transform>(m_parentEntity);
        if (transform) {
            transform->rotation.y = m_time * 0.8f;
        }
    }
}

void GameplayState::Render() {
    auto renderer = m_app->GetRenderer();
    renderer->Clear(0.1f, 0.1f, 0.15f, 1.0f);
    
    // ECS системы рендерят объекты автоматически через World::UpdateSystems
    // которая вызывается в Application::Render
    
    // Рисуем UI текст
    HDC hdc = GetDC(m_app->GetWindowHandle());
    if (hdc) {
        SetBkMode(hdc, TRANSPARENT);
        
        HFONT hFont = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
        
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
        SetTextColor(hdc, RGB(255, 255, 255));
        
        TextOutW(hdc, 10, 10, L"ECS Demo - Free Camera", 22);
        TextOutW(hdc, 10, 35, L"Hold RMB + Move Mouse - Look around", 36);
        TextOutW(hdc, 10, 60, L"WASD - Move camera", 18);
        TextOutW(hdc, 10, 85, L"Q/E - Move Up/Down", 18);
        TextOutW(hdc, 10, 110, L"Shift - Move faster", 19);
        TextOutW(hdc, 10, 135, L"ESC - Back to menu", 18);
        
        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);
        ReleaseDC(m_app->GetWindowHandle(), hdc);
    }
}

void GameplayState::CreateCamera() {
    auto* world = m_app->GetWorld();
    
    // Создаем камеру
    m_camera = world->CreateEntity();
    world->AddComponent(m_camera, Tag("Main Camera"));
    
    // Позиция камеры - ФИКСИРОВАННАЯ, смотрит на сцену
    world->AddComponent(m_camera, Transform(
        XMFLOAT3(0.0f, 2.0f, -8.0f),  // Сзади и немного выше
        XMFLOAT3(0.0f, 0.0f, 0.0f),
        XMFLOAT3(1.0f, 1.0f, 1.0f)
    ));
    
    Camera cameraComponent;
    cameraComponent.aspectRatio = 800.0f / 600.0f;
    cameraComponent.moveSpeed = 3.0f;
    cameraComponent.lookSpeed = 0.002f;
    cameraComponent.yaw = XM_PIDIV2;  // 90 градусов - смотрит вперед по Z+
    cameraComponent.pitch = -0.2f;  // Немного вниз
    world->AddComponent(m_camera, cameraComponent);
    
    LOG_INFO("Created camera at (0, 2, -8) looking forward");
}
