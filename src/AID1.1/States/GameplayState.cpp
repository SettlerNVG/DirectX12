#include "GameplayState.h"
#include "../Core/Application.h"
#include "../Utils/Logger.h"
#include "../Rendering/D3D12Adapter.h"
#include <Windows.h>
#include <cmath>

using namespace DirectX;

GameplayState::GameplayState(Application* app)
    : m_objectX(0.0f)
    , m_objectY(0.0f)
    , m_objectScale(1.0f)
    , m_objectRotation(0.0f)
    , m_moveSpeed(0.5f)
    , m_rotationSpeed(2.0f)
    , m_scaleSpeed(0.5f)
    , m_currentFPS(0.0f)
    , m_fpsUpdateTimer(0.0f) {
    m_app = app;
}

void GameplayState::OnEnter() {
    LOG_INFO("Entering Gameplay State");
    m_objectX = 0.0f;
    m_objectY = 0.0f;
    m_objectScale = 1.0f;
    m_objectRotation = 0.0f;
    
    SetWindowTextW(m_app->GetWindowHandle(), 
        L"Arrow Keys=move | Left Mouse=scale | Right Mouse=rotate | Middle Mouse=reset | ESC=menu");
}

void GameplayState::OnExit() {
    LOG_INFO("Exiting Gameplay State");
}

void GameplayState::Update(float deltaTime) {
    m_fpsUpdateTimer += deltaTime;
    if (m_fpsUpdateTimer >= 0.1f) {
        m_currentFPS = 1.0f / deltaTime;
        m_fpsUpdateTimer = 0.0f;
    }
    
    // Управление стрелками
    if (GetAsyncKeyState(VK_UP) & 0x8000) {
        m_objectY += m_moveSpeed * deltaTime;
        LOG_DEBUG("Moving UP");
    }
    if (GetAsyncKeyState(VK_DOWN) & 0x8000) {
        m_objectY -= m_moveSpeed * deltaTime;
        LOG_DEBUG("Moving DOWN");
    }
    if (GetAsyncKeyState(VK_LEFT) & 0x8000) {
        m_objectX -= m_moveSpeed * deltaTime;
        LOG_DEBUG("Moving LEFT");
    }
    if (GetAsyncKeyState(VK_RIGHT) & 0x8000) {
        m_objectX += m_moveSpeed * deltaTime;
        LOG_DEBUG("Moving RIGHT");
    }
    
    // Управление мышью
    if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
        m_objectScale += m_scaleSpeed * deltaTime;
        if (m_objectScale > 3.0f) m_objectScale = 3.0f;
    }
    if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) {
        m_objectRotation += m_rotationSpeed * deltaTime;
    }
    if (GetAsyncKeyState(VK_MBUTTON) & 0x8000) {
        m_objectScale = 1.0f;
        m_objectRotation = 0.0f;
        m_objectX = 0.0f;
        m_objectY = 0.0f;
    }
    
    // Ограничение позиции
    if (m_objectX > 0.7f) m_objectX = 0.7f;
    if (m_objectX < -0.7f) m_objectX = -0.7f;
    if (m_objectY > 0.7f) m_objectY = 0.7f;
    if (m_objectY < -0.7f) m_objectY = -0.7f;
}

void GameplayState::Render() {
    auto renderer = m_app->GetRenderer();
    
    renderer->Clear(0.2f, 0.3f, 0.4f, 1.0f);
    
    renderer->SetObjectTransform(m_objectX, m_objectY, m_objectScale, m_objectRotation);
    renderer->DrawTriangle();
    
    // Рисуем текст через GDI
    HDC hdc = GetDC(m_app->GetWindowHandle());
    if (hdc) {
        SetBkMode(hdc, TRANSPARENT);
        
        HFONT hFont = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
        
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
        SetTextColor(hdc, RGB(255, 255, 255));
        
        TextOutW(hdc, 10, 10, L"CONTROLS:", 9);
        TextOutW(hdc, 10, 35, L"Arrow Keys - Move", 17);
        TextOutW(hdc, 10, 55, L"Left Mouse - Scale", 18);
        TextOutW(hdc, 10, 75, L"Right Mouse - Rotate", 20);
        TextOutW(hdc, 10, 95, L"Middle Mouse - Reset", 20);
        TextOutW(hdc, 10, 115, L"ESC - Menu", 10);
        
        wchar_t info[256];
        swprintf_s(info, L"Position: (%.2f, %.2f)", m_objectX, m_objectY);
        TextOutW(hdc, 600, 10, info, (int)wcslen(info));
        
        swprintf_s(info, L"Scale: %.2f", m_objectScale);
        TextOutW(hdc, 600, 35, info, (int)wcslen(info));
        
        swprintf_s(info, L"Rotation: %.2f", m_objectRotation);
        TextOutW(hdc, 600, 60, info, (int)wcslen(info));
        
        swprintf_s(info, L"FPS: %.0f", m_currentFPS);
        TextOutW(hdc, 10, 560, info, (int)wcslen(info));
        
        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);
        ReleaseDC(m_app->GetWindowHandle(), hdc);
    }
}
