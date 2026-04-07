#include "Core/Application.h"
#include "Utils/Logger.h"
#include <Windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Настройка логгера
    Logger::GetInstance().SetLogFile("engine_ecs.log");
    Logger::GetInstance().EnableConsoleOutput(true);
    
    LOG_INFO("========================================");
    LOG_INFO("  Game Engine AID2.1 (ECS) Starting...  ");
    LOG_INFO("========================================");
    
    try {
        Application app;
        
        if (!app.Initialize(hInstance, 800, 600)) {
            LOG_ERROR("Failed to initialize application");
            return -1;
        }
        
        app.Run();
        app.Shutdown();
        
        LOG_INFO("========================================");
        LOG_INFO("  Game Engine Shut Down Successfully  ");
        LOG_INFO("========================================");
        
        return 0;
    }
    catch (const std::exception& e) {
        LOG_ERROR(std::string("Exception caught: ") + e.what());
        MessageBoxA(nullptr, e.what(), "Fatal Error", MB_OK | MB_ICONERROR);
        return -1;
    }
}
