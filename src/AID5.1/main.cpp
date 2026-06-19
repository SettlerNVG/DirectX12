#include "Core/Application.h"
#include "Utils/Logger.h"
#include <Windows.h>

int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd)
{
    // Включаем консоль для логов
    AllocConsole();
    FILE* pFile;
    freopen_s(&pFile, "CONOUT$", "w", stdout);
    freopen_s(&pFile, "CONOUT$", "w", stderr);
    
    LOG_INFO("==============================================");
    LOG_INFO("  AID5.1 - WYSIWYG Editor with Dear ImGui");
    LOG_INFO("  DirectX 12 + ECS + Physics + ImGui Editor");
    LOG_INFO("==============================================");
    
    Application app;
    
    if (!app.Initialize(hInstance, 1600, 900)) {
        LOG_ERROR("Failed to initialize application!");
        return -1;
    }
    
    LOG_INFO("Application initialized successfully");
    LOG_INFO("Starting main loop...");
    
    app.Run();
    
    LOG_INFO("Shutting down application...");
    app.Shutdown();
    
    LOG_INFO("Application closed successfully");
    
    // Закрываем консоль
    if (pFile) {
        fclose(pFile);
    }
    FreeConsole();
    
    return 0;
}
