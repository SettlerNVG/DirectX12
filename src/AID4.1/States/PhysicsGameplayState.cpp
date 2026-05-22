#include "PhysicsGameplayState.h"
#include "../Core/Application.h"
#include "../ECS/World.h"
#include "../Components/Transform.h"
#include "../Components/MeshRenderer.h"
#include "../Components/Tag.h"
#include "../Components/Camera.h"
#include "../Components/Rigidbody.h"
#include "../Components/Collider.h"
#include "../Systems/RenderSystem.h"
#include "../Systems/CameraSystem.h"
#include "../Systems/DebugRenderSystem.h"
#include "../Physics/PhysicsSystem.h"
#include "../Input/InputManager.h"
#include "../Events/EventBus.h"
#include "../Events/CollisionEvent.h"
#include "../Utils/Logger.h"
#include <Windows.h>

using namespace DirectX;

PhysicsGameplayState::PhysicsGameplayState(Application* app)
    : m_time(0.0f)
    , m_debugDrawEnabled(true)
    , m_collisionCount(0) {
    m_app = app;
}

void PhysicsGameplayState::OnEnter() {
    LOG_INFO("=== Entering Physics Gameplay State ===");
    SetWindowTextW(m_app->GetWindowHandle(), L"AID4.1 - Physics Demo | RMB+WASD to fly, F3 debug, ESC menu");
    
    auto* world = m_app->GetWorld();
    world->Clear();
    
    // Инициализируем InputManager
    InputManager::GetInstance().Initialize(m_app->GetWindowHandle());
    
    // Добавляем системы
    auto* physicsSystem = new PhysicsSystem();
    physicsSystem->SetDebugDraw(m_debugDrawEnabled);
    world->AddSystem(std::unique_ptr<System>(physicsSystem));
    
    world->AddSystem(std::make_unique<CameraSystem>(m_app->GetRenderer(), m_app->GetWindowHandle()));
    world->AddSystem(std::make_unique<RenderSystem>(m_app->GetRenderer()));
    
    auto* debugSystem = new DebugRenderSystem(m_app->GetRenderer());
    debugSystem->SetDrawColliders(m_debugDrawEnabled);
    world->AddSystem(std::unique_ptr<System>(debugSystem));
    
    // Настраиваем привязки клавиш
    SetupInputBindings();
    
    // Настраиваем обработчики коллизий
    SetupCollisionHandlers();
    
    // Создаем сцену
    CreateCamera();
    CreateGround();
    CreateFallingObjects();
    CreatePlayerControlledObject();
    
    m_time = 0.0f;
    m_collisionCount = 0;
    
    LOG_INFO("=== Physics scene created successfully ===");
}

void PhysicsGameplayState::OnExit() {
    LOG_INFO("Exiting Physics Gameplay State");
    
    // Очищаем привязки
    InputManager::GetInstance().ClearBindings();
    
    // Очищаем события
    EventBus::GetInstance().Clear();
    
    // Очищаем мир
    auto* world = m_app->GetWorld();
    world->Clear();
}

void PhysicsGameplayState::CreateCamera() {
    auto* world = m_app->GetWorld();
    
    m_camera = world->CreateEntity();
    world->AddComponent(m_camera, Tag("Main Camera"));
    world->AddComponent(m_camera, Transform(
        XMFLOAT3(0.0f, 5.0f, -15.0f),
        XMFLOAT3(0.0f, 0.0f, 0.0f),
        XMFLOAT3(1.0f, 1.0f, 1.0f)
    ));
    
    Camera cameraComponent;
    cameraComponent.aspectRatio = 800.0f / 600.0f;
    cameraComponent.moveSpeed = 5.0f;
    cameraComponent.lookSpeed = 0.002f;
    cameraComponent.yaw = XM_PIDIV2;
    cameraComponent.pitch = -0.3f;
    world->AddComponent(m_camera, cameraComponent);
    
    LOG_INFO("Created camera at (0, 5, -15)");
}

void PhysicsGameplayState::CreateGround() {
    auto* world = m_app->GetWorld();
    
    // Большая платформа-земля
    m_ground = world->CreateEntity();
    world->AddComponent(m_ground, Tag("Ground"));
    world->AddComponent(m_ground, Transform(
        XMFLOAT3(0.0f, -2.0f, 0.0f),
        XMFLOAT3(0.0f, 0.0f, 0.0f),
        XMFLOAT3(15.0f, 0.5f, 15.0f)  // Большая плоская платформа
    ));
    world->AddComponent(m_ground, MeshRenderer(
        PrimitiveType::Cube,
        XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f)  // Серый
    ));
    
    // Статичный коллайдер (без Rigidbody)
    world->AddComponent(m_ground, Collider::CreateBox(0.5f, 0.5f, 0.5f));
    
    LOG_INFO("Created ground platform");
}

void PhysicsGameplayState::CreateFallingObjects() {
    auto* world = m_app->GetWorld();
    
    // Создаём несколько падающих объектов
    struct ObjectConfig {
        XMFLOAT3 position;
        XMFLOAT3 scale;
        XMFLOAT4 color;
        PrimitiveType type;
        ColliderType colliderType;
        float mass;
        std::string name;
    };
    
    std::vector<ObjectConfig> configs = {
        // Кубы с ХОРОШИМ отскоком
        { XMFLOAT3(-5.0f, 10.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), PrimitiveType::Cube, ColliderType::Box, 1.0f, "Red Cube" },
        { XMFLOAT3(-2.0f, 12.0f, 0.0f), XMFLOAT3(1.2f, 1.2f, 1.2f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f), PrimitiveType::Cube, ColliderType::Box, 1.5f, "Green Cube" },
        { XMFLOAT3(2.0f, 8.0f, 0.0f), XMFLOAT3(0.8f, 0.8f, 0.8f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f), PrimitiveType::Cube, ColliderType::Box, 0.8f, "Blue Cube" },
        { XMFLOAT3(5.0f, 15.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f), PrimitiveType::Cube, ColliderType::Box, 1.0f, "Yellow Cube" },
        
        // Сферы (визуально кубы, но с sphere collider)
        { XMFLOAT3(0.0f, 20.0f, -3.0f), XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f), PrimitiveType::Cube, ColliderType::Sphere, 1.0f, "Magenta Sphere" },
        { XMFLOAT3(3.0f, 18.0f, 3.0f), XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f), PrimitiveType::Cube, ColliderType::Sphere, 1.0f, "Cyan Sphere" },
    };
    
    for (const auto& config : configs) {
        Entity entity = world->CreateEntity();
        world->AddComponent(entity, Tag(config.name));
        world->AddComponent(entity, Transform(config.position, XMFLOAT3(0, 0, 0), config.scale));
        world->AddComponent(entity, MeshRenderer(config.type, config.color));
        
        // Rigidbody с УЛУЧШЕННЫМИ параметрами для стабильности
        Rigidbody rb(config.mass, true);
        rb.restitution = 0.3f;  // Уменьшил отскок для стабильности
        rb.drag = 0.05f;        // Увеличил сопротивление для быстрого затухания
        world->AddComponent(entity, rb);
        
        // Коллайдер
        if (config.colliderType == ColliderType::Box) {
            world->AddComponent(entity, Collider::CreateBox(0.5f, 0.5f, 0.5f));
        } else {
            world->AddComponent(entity, Collider::CreateSphere(0.7f));
        }
        
        m_fallingObjects.push_back(entity);
        
        LOG_INFO("Created falling object: " + config.name);
    }
}

void PhysicsGameplayState::CreatePlayerControlledObject() {
    auto* world = m_app->GetWorld();
    
    // Управляемый игроком куб
    m_player = world->CreateEntity();
    world->AddComponent(m_player, Tag("Player"));
    world->AddComponent(m_player, Transform(
        XMFLOAT3(0.0f, 2.0f, 5.0f),
        XMFLOAT3(0.0f, 0.0f, 0.0f),
        XMFLOAT3(1.5f, 1.5f, 1.5f)
    ));
    world->AddComponent(m_player, MeshRenderer(
        PrimitiveType::Cube,
        XMFLOAT4(1.0f, 0.5f, 0.0f, 1.0f)  // Оранжевый
    ));
    
    // Rigidbody с ХОРОШИМ управлением
    Rigidbody rb(2.0f, true);
    rb.restitution = 0.1f;  // Минимальный отскок для игрока
    rb.drag = 0.8f;         // БОЛЬШОЕ сопротивление - быстро останавливается
    world->AddComponent(m_player, rb);
    
    world->AddComponent(m_player, Collider::CreateBox(0.75f, 0.75f, 0.75f));
    
    LOG_INFO("Created player-controlled object");
}

void PhysicsGameplayState::SetupInputBindings() {
    auto& input = InputManager::GetInstance();
    
    // Привязываем действия
    input.BindAction("MoveForward", KeyCode::Up);
    input.BindAction("MoveBackward", KeyCode::Down);
    input.BindAction("MoveLeft", KeyCode::Left);
    input.BindAction("MoveRight", KeyCode::Right);
    input.BindAction("Jump", KeyCode::Space);
    input.BindAction("ToggleDebug", KeyCode::F3);
    
    // Явно регистрируем клавиши для отслеживания
    input.BindAction("SpaceKey", KeyCode::Space);
    input.BindAction("CameraW", KeyCode::W);
    input.BindAction("CameraA", KeyCode::A);
    input.BindAction("CameraS", KeyCode::S);
    input.BindAction("CameraD", KeyCode::D);
    input.BindAction("CameraQ", KeyCode::Q);
    input.BindAction("CameraE", KeyCode::E);
    
    LOG_INFO("Input bindings configured");
}

void PhysicsGameplayState::SetupCollisionHandlers() {
    // Подписываемся на события коллизий с улучшенной обработкой
    SUBSCRIBE_TO_EVENT(CollisionEvent, [this](const CollisionEvent& event) {
        m_collisionCount++;
        
        auto* world = m_app->GetWorld();
        if (!world) return;
        
        auto* tagA = world->GetComponent<Tag>(event.entityA);
        auto* tagB = world->GetComponent<Tag>(event.entityB);
        
        std::string nameA = tagA ? tagA->name : "Unknown";
        std::string nameB = tagB ? tagB->name : "Unknown";
        
        // Логируем только важные коллизии или каждую 20-ю для уменьшения спама
        bool shouldLog = false;
        
        // Всегда логируем коллизии с игроком
        if (nameA == "Player" || nameB == "Player") {
            shouldLog = true;
        }
        // Или каждую 20-ю коллизию для общей статистики
        else if (m_collisionCount % 20 == 0) {
            shouldLog = true;
        }
        
        if (shouldLog) {
            std::string triggerInfo = event.isTrigger ? " (TRIGGER)" : "";
            LOG_INFO("Collision #" + std::to_string(m_collisionCount) + ": " + nameA + " <-> " + nameB + triggerInfo);
        }
        
        // Специальная обработка коллизий игрока с землей
        if ((nameA == "Player" && nameB == "Ground") || (nameA == "Ground" && nameB == "Player")) {
            // Можно добавить звуковые эффекты или другую логику
        }
    });
    
    LOG_INFO("Enhanced collision handlers configured");
}

void PhysicsGameplayState::Update(float deltaTime) {
    m_time += deltaTime;
    
    auto& input = InputManager::GetInstance();
    auto* world = m_app->GetWorld();
    
    if (!world) {
        LOG_INFO("ERROR: World is null in PhysicsGameplayState::Update");
        return;
    }
    
    // Переключение debug-отрисовки по F3
    static bool f3WasPressed = false;
    bool f3IsPressed = input.IsKeyPressed(KeyCode::F3);
    
    if (f3IsPressed && !f3WasPressed) {
        m_debugDrawEnabled = !m_debugDrawEnabled;
        
        if (auto* debugSys = world->GetSystem<DebugRenderSystem>()) {
            debugSys->SetDrawColliders(m_debugDrawEnabled);
        }
        if (auto* physicsSys = world->GetSystem<PhysicsSystem>()) {
            physicsSys->SetDebugDraw(m_debugDrawEnabled);
        }
        
        LOG_INFO(m_debugDrawEnabled ? "Debug draw ENABLED" : "Debug draw DISABLED");
    }
    f3WasPressed = f3IsPressed;
    
    // ЗАЩИТА: Проверяем, что объекты не упали под пол
    for (Entity entity : m_fallingObjects) {
        if (world->IsEntityValid(entity)) {
            auto* transform = world->GetComponent<Transform>(entity);
            auto* rb = world->GetComponent<Rigidbody>(entity);
            
            if (transform && rb && transform->position.y < -10.0f) {
                // Объект упал слишком низко - возвращаем наверх
                transform->position.y = 15.0f + (rand() % 10); // Случайная высота 15-25
                transform->position.x = -5.0f + (rand() % 11); // Случайная позиция X от -5 до 5
                transform->position.z = -5.0f + (rand() % 11); // Случайная позиция Z от -5 до 5
                rb->velocity = XMFLOAT3(0, 0, 0); // Сбрасываем скорость
                rb->isSleeping = false; // Пробуждаем объект
                rb->sleepTimer = 0.0f;
                
                auto* tag = world->GetComponent<Tag>(entity);
                std::string name = tag ? tag->name : "Unknown";
                LOG_INFO("Respawned fallen object: " + name);
            }
            
            // ДОПОЛНИТЕЛЬНАЯ ЗАЩИТА: Проверяем на NaN значения
            if (transform && (isnan(transform->position.x) || isnan(transform->position.y) || isnan(transform->position.z))) {
                LOG_INFO("ERROR: NaN position detected, resetting object");
                transform->position = XMFLOAT3(0.0f, 10.0f, 0.0f);
                if (rb) {
                    rb->velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
                    rb->acceleration = XMFLOAT3(0.0f, 0.0f, 0.0f);
                    rb->isSleeping = false;
                    rb->sleepTimer = 0.0f;
                }
            }
            
            if (rb && (isnan(rb->velocity.x) || isnan(rb->velocity.y) || isnan(rb->velocity.z))) {
                LOG_INFO("ERROR: NaN velocity detected, resetting velocity");
                rb->velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
                rb->acceleration = XMFLOAT3(0.0f, 0.0f, 0.0f);
            }
        }
    }
    
    // Управление игроком через стрелки
    if (world->IsEntityValid(m_player)) {
        auto* rb = world->GetComponent<Rigidbody>(m_player);
        auto* transform = world->GetComponent<Transform>(m_player);
        
        if (rb && transform) {
            // ЗАЩИТА: Проверяем на NaN значения у игрока
            if (isnan(transform->position.x) || isnan(transform->position.y) || isnan(transform->position.z)) {
                LOG_INFO("ERROR: Player NaN position detected, resetting");
                transform->position = XMFLOAT3(0.0f, 2.0f, 5.0f);
                rb->velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
                rb->acceleration = XMFLOAT3(0.0f, 0.0f, 0.0f);
            }
            
            if (isnan(rb->velocity.x) || isnan(rb->velocity.y) || isnan(rb->velocity.z)) {
                LOG_INFO("ERROR: Player NaN velocity detected, resetting");
                rb->velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
                rb->acceleration = XMFLOAT3(0.0f, 0.0f, 0.0f);
            }
            
            XMFLOAT3 force(0, 0, 0);
            float forceMagnitude = 25.0f;  // Увеличил силу движения
            
            if (input.IsActionActive("MoveForward")) {
                force.z += forceMagnitude;
            }
            if (input.IsActionActive("MoveBackward")) {
                force.z -= forceMagnitude;
            }
            if (input.IsActionActive("MoveLeft")) {
                force.x -= forceMagnitude;
            }
            if (input.IsActionActive("MoveRight")) {
                force.x += forceMagnitude;
            }
            
            // Прыжок - УЛУЧШЕННАЯ проверка с определением "на земле"
            static bool spaceWasPressed = false;
            bool spaceIsPressed = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
            
            if (spaceIsPressed && !spaceWasPressed) {
                // УЛУЧШЕННАЯ проверка: проверяем есть ли коллизия с землей снизу
                bool canJump = false;
                
                // Метод 1: Проверяем вертикальную скорость (должна быть близка к 0)
                if (fabsf(rb->velocity.y) < 2.0f) { // Увеличил порог
                    canJump = true;
                }
                
                // Метод 2: Дополнительная проверка - не слишком высоко над землей
                if (transform->position.y > 10.0f) { // Если слишком высоко - точно не на земле
                    canJump = false;
                }
                
                // Метод 3: Проверяем коллизии с землей (raycast вниз)
                auto* world = m_app->GetWorld();
                auto entities = world->GetEntitiesWith<Transform, Collider>();
                
                bool groundNearby = false;
                for (Entity entity : entities) {
                    if (entity == m_player) continue; // Пропускаем себя
                    
                    auto* otherTransform = world->GetComponent<Transform>(entity);
                    auto* tag = world->GetComponent<Tag>(entity);
                    
                    if (otherTransform && tag && tag->name == "Ground") {
                        // Проверяем расстояние до земли
                        float distanceToGround = transform->position.y - otherTransform->position.y;
                        if (distanceToGround < 3.0f && distanceToGround > -1.0f) { // В пределах разумного
                            groundNearby = true;
                            break;
                        }
                    }
                }
                
                if (canJump && groundNearby) {
                    XMFLOAT3 jumpImpulse(0, 12.0f, 0); // Увеличил силу прыжка
                    rb->AddImpulse(jumpImpulse);
                    rb->isSleeping = false; // Пробуждаем при прыжке
                    rb->sleepTimer = 0.0f;
                    LOG_INFO("Player jumped! Y velocity was: " + std::to_string(rb->velocity.y) + ", Ground nearby: " + (groundNearby ? "YES" : "NO"));
                } else {
                    LOG_INFO("Player can't jump - Y velocity: " + std::to_string(rb->velocity.y) + 
                            ", Y pos: " + std::to_string(transform->position.y) + 
                            ", Ground nearby: " + (groundNearby ? "YES" : "NO"));
                }
            }
            spaceWasPressed = spaceIsPressed;
            
            if (force.x != 0 || force.z != 0) {
                rb->AddForce(force);
                rb->isSleeping = false; // Пробуждаем при движении
                rb->sleepTimer = 0.0f;
            }
            
            // ЗАЩИТА: Если игрок упал слишком низко
            if (transform->position.y < -10.0f) {
                LOG_INFO("Player fell too low, respawning");
                transform->position = XMFLOAT3(0.0f, 2.0f, 5.0f);
                rb->velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
                rb->acceleration = XMFLOAT3(0.0f, 0.0f, 0.0f);
                rb->isSleeping = false;
                rb->sleepTimer = 0.0f;
            }
        }
    }
}

void PhysicsGameplayState::Render() {
    auto renderer = m_app->GetRenderer();
    renderer->Clear(0.1f, 0.15f, 0.2f, 1.0f);
    
    // ECS системы рендерят объекты автоматически
    
    // Рисуем UI
    HDC hdc = GetDC(m_app->GetWindowHandle());
    if (hdc) {
        SetBkMode(hdc, TRANSPARENT);
        
        HFONT hFont = CreateFontW(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
        
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
        SetTextColor(hdc, RGB(255, 255, 0));
        
        TextOutW(hdc, 10, 10, L"AID4.1 - Physics & Input Demo", 29);
        
        SetTextColor(hdc, RGB(255, 255, 255));
        HFONT hSmallFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
        SelectObject(hdc, hSmallFont);
        
        TextOutW(hdc, 10, 40, L"Camera: RMB + Mouse + WASD/QE", 30);
        TextOutW(hdc, 10, 60, L"Player: Arrow Keys + Space (Jump)", 34);
        TextOutW(hdc, 10, 80, L"F3 - Toggle Collider Wireframes", 32);
        TextOutW(hdc, 10, 100, L"ESC - Back to Menu", 18);
        
        std::wstring collisionText = L"Collisions: " + std::to_wstring(m_collisionCount);
        TextOutW(hdc, 10, 130, collisionText.c_str(), static_cast<int>(collisionText.length()));
        
        std::wstring debugText = m_debugDrawEnabled ? 
            L"Debug Wireframes: ON (Yellow=Box, Cyan=Sphere)" : 
            L"Debug Wireframes: OFF";
        SetTextColor(hdc, m_debugDrawEnabled ? RGB(0, 255, 0) : RGB(255, 0, 0));
        TextOutW(hdc, 10, 150, debugText.c_str(), static_cast<int>(debugText.length()));
        
        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);
        DeleteObject(hSmallFont);
        ReleaseDC(m_app->GetWindowHandle(), hdc);
    }
}
