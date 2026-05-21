#include "LoadingState.h"
#include "../Core/Application.h"
#include "../Utils/Logger.h"
#include <Windows.h>

LoadingState::LoadingState(Application* app)
    : m_timer(0.0f)
    , m_loadingDuration(2.0f) {
    m_app = app;
}

void LoadingState::OnEnter() {
    LOG_INFO("Entering Loading State");
    m_timer = 0.0f;
    SetWindowTextW(m_app->GetWindowHandle(), L"Loading...");
}

void LoadingState::OnExit() {
    LOG_INFO("Exiting Loading State");
}

void LoadingState::Update(float deltaTime) {
    m_timer += deltaTime;
    
    if (m_timer >= m_loadingDuration) {
        m_app->ChangeState("MenuState");
    }
}

void LoadingState::Render() {
    auto renderer = m_app->GetRenderer();
    renderer->Clear(0.0f, 0.2f, 0.0f, 1.0f);
    
    // Рисуем текст "LOADING..." через GDI
    HDC hdc = GetDC(m_app->GetWindowHandle());
    if (hdc) {
        SetBkMode(hdc, TRANSPARENT);
        
        HFONT hFont = CreateFontW(48, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
        
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
        SetTextColor(hdc, RGB(255, 255, 255));
        TextOutW(hdc, 280, 250, L"LOADING...", 10);
        
        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);
        ReleaseDC(m_app->GetWindowHandle(), hdc);
    }
    
    // Прогресс-бар через DirectX
    float progress = m_timer / m_loadingDuration;
    renderer->DrawUIRect(-0.6f, -0.3f, 1.2f * progress, 0.05f, 0.2f, 1.0f, 0.2f, 1.0f);
}
