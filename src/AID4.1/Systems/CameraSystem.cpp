#include "CameraSystem.h"
#include "../ECS/World.h"
#include "../Components/Transform.h"
#include "../Components/Camera.h"
#include "../Input/InputManager.h"
#include "../Utils/Logger.h"

using namespace DirectX;

CameraSystem::CameraSystem(RenderAdapter* renderer, HWND hwnd)
    : m_renderer(renderer)
    , m_hwnd(hwnd)
    , m_mouseCaptured(false)
    , m_firstCapture(true) {
    LOG_INFO("CameraSystem created with InputManager integration");
}

void CameraSystem::Update(World* world, float deltaTime) {
    auto entities = world->GetEntitiesWith<Transform, Camera>();
    
    for (Entity entity : entities) {
        auto* camera = world->GetComponent<Camera>(entity);
        
        if (!camera->isActive) {
            continue;
        }
        
        // Обработка ввода
        HandleInput(world, entity, deltaTime);
        
        // Обновление матриц View и Projection
        UpdateViewProjection(world, entity);
        
        // Только одна активная камера
        break;
    }
}

void CameraSystem::HandleInput(World* world, Entity cameraEntity, float deltaTime) {
    auto* transform = world->GetComponent<Transform>(cameraEntity);
    auto* camera = world->GetComponent<Camera>(cameraEntity);
    
    if (!transform || !camera) {
        return;
    }
    
    auto& input = InputManager::GetInstance();
    
    // Захват/освобождение мыши по правой кнопке
    if (input.IsMouseButtonPressed(KeyCode::MouseRight)) {
        if (!m_mouseCaptured) {
            m_mouseCaptured = true;
            m_firstCapture = true;
            input.ShowCursor(false);
            input.CenterCursor();
        }
    } else {
        if (m_mouseCaptured) {
            m_mouseCaptured = false;
            input.ShowCursor(true);
        }
    }
    
    // Управление мышью (только если захвачена)
    if (m_mouseCaptured) {
        if (m_firstCapture) {
            m_firstCapture = false;
            input.CenterCursor();
        }
        
        XMFLOAT2 mouseDelta = input.GetMouseDelta();
        
        camera->yaw += mouseDelta.x * camera->lookSpeed;
        camera->pitch -= mouseDelta.y * camera->lookSpeed;
        
        // Ограничение pitch
        const float maxPitch = XM_PIDIV2 - 0.01f;
        if (camera->pitch > maxPitch) camera->pitch = maxPitch;
        if (camera->pitch < -maxPitch) camera->pitch = -maxPitch;
        
        // Возвращаем курсор в центр
        input.CenterCursor();
    }
    
    // Управление WASD + QE
    XMVECTOR forward = camera->GetForward();
    XMVECTOR right = camera->GetRight();
    XMVECTOR up = camera->GetUp();
    
    XMVECTOR movement = XMVectorZero();
    
    if (input.IsKeyPressed(KeyCode::W)) {
        movement = XMVectorAdd(movement, forward);
    }
    if (input.IsKeyPressed(KeyCode::S)) {
        movement = XMVectorSubtract(movement, forward);
    }
    if (input.IsKeyPressed(KeyCode::D)) {
        movement = XMVectorAdd(movement, right);
    }
    if (input.IsKeyPressed(KeyCode::A)) {
        movement = XMVectorSubtract(movement, right);
    }
    if (input.IsKeyPressed(KeyCode::E)) {
        movement = XMVectorAdd(movement, up);
    }
    if (input.IsKeyPressed(KeyCode::Q)) {
        movement = XMVectorSubtract(movement, up);
    }
    
    // Ускорение на Shift
    float speed = camera->moveSpeed;
    if (input.IsKeyPressed(KeyCode::Shift)) {
        speed *= 2.0f;
    }
    
    // Нормализуем и применяем движение
    if (!XMVector3Equal(movement, XMVectorZero())) {
        movement = XMVector3Normalize(movement);
        movement = XMVectorScale(movement, speed * deltaTime);
        
        XMVECTOR pos = XMLoadFloat3(&transform->position);
        pos = XMVectorAdd(pos, movement);
        XMStoreFloat3(&transform->position, pos);
    }
}

void CameraSystem::UpdateViewProjection(World* world, Entity cameraEntity) {
    auto* transform = world->GetComponent<Transform>(cameraEntity);
    auto* camera = world->GetComponent<Camera>(cameraEntity);
    
    if (!transform || !camera) {
        return;
    }
    
    // Вычисляем матрицу вида
    XMVECTOR eye = XMLoadFloat3(&transform->position);
    XMVECTOR forward = camera->GetForward();
    XMVECTOR at = XMVectorAdd(eye, forward);
    XMVECTOR up = camera->GetUp();
    
    XMMATRIX viewMatrix = XMMatrixLookAtLH(eye, at, up);
    
    // Вычисляем матрицу проекции
    XMMATRIX projectionMatrix = XMMatrixPerspectiveFovLH(
        camera->fov,
        camera->aspectRatio,
        camera->nearPlane,
        camera->farPlane
    );
    
    // Устанавливаем матрицы в рендерер
    m_renderer->SetViewMatrix(viewMatrix);
    m_renderer->SetProjectionMatrix(projectionMatrix);
}
