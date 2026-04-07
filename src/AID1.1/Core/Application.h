#pragma once

#include "../Rendering/RenderAdapter.h"
#include "../States/StateManager.h"
#include "Timer.h"
#include <Windows.h>
#include <memory>
#include <string>

class Application {
public:
    Application();
    ~Application();
    
    bool Initialize(HINSTANCE hInstance, int width = 800, int height = 600);
    void Run();
    void Shutdown();
    
    void ChangeState(const std::string& stateName);
    
    RenderAdapter* GetRenderer() const { return m_renderer.get(); }
    HWND GetWindowHandle() const { return m_hwnd; }
    
    bool IsRunning() const { return m_isRunning; }
    void Quit() { m_isRunning = false; }

private:
    bool CreateAppWindow(HINSTANCE hInstance, int width, int height);
    void ProcessMessages();
    void Update(float deltaTime);
    void Render();
    
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    HWND m_hwnd;
    int m_width;
    int m_height;
    bool m_isRunning;
    
    std::unique_ptr<RenderAdapter> m_renderer;
    std::unique_ptr<StateManager> m_stateManager;
    Timer m_timer;
};
