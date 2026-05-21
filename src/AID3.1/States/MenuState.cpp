#include "MenuState.h"
#include "../Core/Application.h"
#include "../Utils/Logger.h"
#include <Windows.h>

MenuState::MenuState(Application* app) {
    m_app = app;
}

void MenuState::OnEnter() {
    LOG_INFO("Entering Menu State");
    SetWindowTextW(m_app->GetWindowHandle(), L"Resource Manager Demo | Press SPACE to start");
}

void MenuState::OnExit() {
    LOG_INFO("Exiting Menu State");
}

void MenuState::Update(float deltaTime) {
    static bool spaceWasPressed = false;
    
    bool spacePressed = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
    
    if (spacePressed && !spaceWasPressed) {
        LOG_INFO("SPACE pressed - starting game!");
        m_app->ChangeState("GameplayState");
    }
    
    spaceWasPressed = spacePressed;
}

void MenuState::Render() {
    auto renderer = m_app->GetRenderer();
    renderer->Clear(0.1f, 0.1f, 0.2f, 1.0f);
    
    // Рисуем текст через GDI
    HDC hdc = GetDC(m_app->GetWindowHandle());
    if (hdc) {
        SetBkMode(hdc, TRANSPARENT);
        
        // Большой заголовок
        HFONT hBigFont = CreateFontW(60, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
        
        HFONT hOldFont = (HFONT)SelectObject(hdc, hBigFont);
        SetTextColor(hdc, RGB(255, 255, 255));
        TextOutW(hdc, 120, 200, L"Resource Manager Demo", 21);
        
        // Подсказки
        HFONT hSmallFont = CreateFontW(24, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
        
        SelectObject(hdc, hSmallFont);
        SetTextColor(hdc, RGB(200, 200, 200));
        TextOutW(hdc, 240, 350, L"Press SPACE to start", 20);
        TextOutW(hdc, 320, 400, L"ESC to exit", 11);
        
        SelectObject(hdc, hOldFont);
        DeleteObject(hBigFont);
        DeleteObject(hSmallFont);
        ReleaseDC(m_app->GetWindowHandle(), hdc);
    }
}