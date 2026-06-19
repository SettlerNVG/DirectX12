#include "Application.h"
#include "../Rendering/D3D12Adapter.h"
#include "../States/EditorState.h"
#include "../Input/InputManager.h"
#include "../Utils/Logger.h"
#include <sstream>
#include <iostream>
#include <stdio.h>

Application::Application()
    : m_hwnd(nullptr)
    , m_width(800)
    , m_height(600)
    , m_isRunning(false) {
}

Application::~Application() {
    Shutdown();
}

bool Application::Initialize(HINSTANCE hInstance, int width, int height) {
    // Создаем консольное окно для логов
    AllocConsole();
    SetConsoleTitleW(L"Game Engine - Console Logs");
    
    LOG_INFO("=== Initializing AID5.1 WYSIWYG Editor ===");
    
    m_width = width;
    m_height = height;
    
    // Создание окна
    if (!CreateAppWindow(hInstance, width, height)) {
        LOG_ERROR("Failed to create application window");
        return false;
    }
    
    // Создание адаптера рендеринга
    m_renderer = std::make_unique<D3D12Adapter>();
    if (!m_renderer->Initialize(m_hwnd, width, height)) {
        LOG_ERROR("Failed to initialize renderer");
        return false;
    }
    
    // Создание ECS World - теперь управляется EditorState
    // m_world = std::make_unique<World>();
    // LOG_INFO("ECS World created");
    
    // Создание менеджера состояний
    m_stateManager = std::make_unique<StateManager>();
    
    // Создание и регистрация EditorState
    auto editorState = std::make_unique<EditorState>();
    if (!editorState->Initialize(m_hwnd, m_renderer.get())) {
        LOG_ERROR("Failed to initialize EditorState");
        return false;
    }
    
    m_stateManager->RegisterState("EditorState", std::move(editorState));
    
    // Установка начального состояния - редактор
    m_stateManager->ChangeState("EditorState");
    
    // Инициализация InputManager - теперь управляется EditorState
    // InputManager::GetInstance().Initialize(m_hwnd);
    // LOG_INFO("InputManager initialized");
    
    // Запуск таймера
    m_timer.Start();
    
    m_isRunning = true;
    
    LOG_INFO("=== AID5.1 WYSIWYG Editor Initialized Successfully ===");
    return true;
}

void Application::Run() {
    LOG_INFO("Starting main game loop");
    
    while (m_isRunning) {
        ProcessMessages();
        
        m_timer.Tick();
        float deltaTime = m_timer.GetDeltaTime();
        
        Update(deltaTime);
        Render();
        
        // Логирование FPS каждую секунду
        static float fpsLogTimer = 0.0f;
        fpsLogTimer += deltaTime;
        if (fpsLogTimer >= 1.0f) {
            std::ostringstream oss;
            oss << "FPS: " << m_timer.GetFPS() << " | DeltaTime: " << deltaTime * 1000.0f << "ms";
            LOG_DEBUG(oss.str());
            fpsLogTimer = 0.0f;
        }
    }
    
    LOG_INFO("Main game loop ended");
}

void Application::Shutdown() {
    LOG_INFO("Shutting down application");
    
    if (m_world) {
        m_world->Clear();
        m_world.reset();
    }
    
    if (m_renderer) {
        m_renderer->Shutdown();
        m_renderer.reset();
    }
    
    m_stateManager.reset();
    
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
    
    // Освобождаем консоль
    FreeConsole();
}

void Application::ChangeState(const std::string& stateName) {
    m_stateManager->ChangeState(stateName);
}

bool Application::CreateAppWindow(HINSTANCE hInstance, int width, int height) {
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"GameEngineWindowClass";
    
    if (!RegisterClassEx(&wc)) {
        LOG_ERROR("Failed to register window class");
        return false;
    }
    
    RECT windowRect = { 0, 0, width, height };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);
    
    m_hwnd = CreateWindowEx(
        0,
        L"GameEngineWindowClass",
        L"AID5.1 WYSIWYG Editor - Dear ImGui Integration",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        hInstance,
        this
    );
    
    if (!m_hwnd) {
        LOG_ERROR("Failed to create window");
        return false;
    }
    
    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);
    
    LOG_INFO("Application window created successfully");
    return true;
}

void Application::ProcessMessages() {
    MSG msg = {};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            m_isRunning = false;
        }
        
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void Application::Update(float deltaTime) {
    // InputManager теперь управляется EditorState
    // InputManager::GetInstance().Update();
    
    m_stateManager->Update(deltaTime);
}

void Application::Render() {
    m_renderer->BeginFrame();
    
    // ECS системы теперь управляются EditorState
    // m_world->UpdateSystems(m_timer.GetDeltaTime());
    
    m_stateManager->Render();
    
    m_renderer->EndFrame();
}

LRESULT CALLBACK Application::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    Application* app = nullptr;
    
    if (msg == WM_CREATE) {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        app = reinterpret_cast<Application*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    } else {
        app = reinterpret_cast<Application*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    
    // Передаем сообщение в EditorState для обработки ImGui
    if (app && app->m_stateManager) {
        auto currentState = app->m_stateManager->GetCurrentState();
        if (currentState && currentState->GetName() == "EditorState") {
            EditorState* editorState = static_cast<EditorState*>(currentState);
            if (editorState->HandleWindowMessage(hwnd, msg, wParam, lParam)) {
                return true; // ImGui обработал сообщение
            }
        }
    }
    
    switch (msg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
            
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                if (app) {
                    app->Quit();
                }
            }
            return 0;
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
