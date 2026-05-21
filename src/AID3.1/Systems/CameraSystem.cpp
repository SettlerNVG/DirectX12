#include "CameraSystem.h"
#include "../ECS/World.h"
#include "../Components/Transform.h"
#include "../Components/Camera.h"
#include "../Utils/Logger.h"

using namespace DirectX;

CameraSystem::CameraSystem(RenderAdapter* renderer, HWND hwnd)
    : m_renderer(renderer)
    , m_hwnd(hwnd)
    , m_firstMouse(true)
    , m_lastMouseX(0)
    , m_lastMouseY(0)
    , m_mouseCaptured(false) {
    LOG_INFO("CameraSystem created");
}

void CameraSystem::Update(World* world, float deltaTime) {
    // Получаем все сущности с Transform и Camera
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
    
    // Захват/освобождение мыши по правой кнопке
    if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) {
        if (!m_mouseCaptured) {
            m_mouseCaptured = true;
            m_firstMouse = true;
            ShowCursor(FALSE);
            
            // Центрируем курсор
            RECT rect;
            GetClientRect(m_hwnd, &rect);
            POINT center = { (rect.right - rect.left) / 2, (rect.bottom - rect.top) / 2 };
            ClientToScreen(m_hwnd, &center);
            SetCursorPos(center.x, center.y);
        }
    } else {
        if (m_mouseCaptured) {
            m_mouseCaptured = false;
            ShowCursor(TRUE);
        }
    }
    
    // Управление мышью (только если захвачена)
    if (m_mouseCaptured) {
        POINT mousePos;
        GetCursorPos(&mousePos);
        ScreenToClient(m_hwnd, &mousePos);
        
        if (m_firstMouse) {
            m_lastMouseX = mousePos.x;
            m_lastMouseY = mousePos.y;
            m_firstMouse = false;
        }
        
        int deltaX = mousePos.x - m_lastMouseX;
        int deltaY = mousePos.y - m_lastMouseY;
        
        m_lastMouseX = mousePos.x;
        m_lastMouseY = mousePos.y;
        
        camera->yaw += deltaX * camera->lookSpeed;
        camera->pitch -= deltaY * camera->lookSpeed;
        
        // Ограничение pitch
        const float maxPitch = XM_PIDIV2 - 0.01f;
        if (camera->pitch > maxPitch) camera->pitch = maxPitch;
        if (camera->pitch < -maxPitch) camera->pitch = -maxPitch;
        
        // Возвращаем курсор в центр
        RECT rect;
        GetClientRect(m_hwnd, &rect);
        POINT center = { (rect.right - rect.left) / 2, (rect.bottom - rect.top) / 2 };
        ClientToScreen(m_hwnd, &center);
        SetCursorPos(center.x, center.y);
        m_lastMouseX = (rect.right - rect.left) / 2;
        m_lastMouseY = (rect.bottom - rect.top) / 2;
    }
    
    // Управление WASD
    XMVECTOR forward = camera->GetForward();
    XMVECTOR right = camera->GetRight();
    XMVECTOR up = camera->GetUp();
    
    XMVECTOR movement = XMVectorZero();
    
    if (GetAsyncKeyState('W') & 0x8000) {
        movement = XMVectorAdd(movement, forward);
    }
    if (GetAsyncKeyState('S') & 0x8000) {
        movement = XMVectorSubtract(movement, forward);
    }
    if (GetAsyncKeyState('D') & 0x8000) {
        movement = XMVectorAdd(movement, right);
    }
    if (GetAsyncKeyState('A') & 0x8000) {
        movement = XMVectorSubtract(movement, right);
    }
    if (GetAsyncKeyState('E') & 0x8000) {
        movement = XMVectorAdd(movement, up);
    }
    if (GetAsyncKeyState('Q') & 0x8000) {
        movement = XMVectorSubtract(movement, up);
    }
    
    // Ускорение на Shift
    float speed = camera->moveSpeed;
    if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
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