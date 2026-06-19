#pragma once

#include <memory>
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <cstddef>
#include <cstdint>
#include <vector>

// Forward declarations
class World;
class RenderAdapter;
struct ImGui_ImplDX12_InitInfo;

// Главный класс редактора ImGui
class EditorGUI {
public:
    EditorGUI();
    ~EditorGUI();
    
    // Инициализация ImGui
    bool Initialize(HWND hwnd, RenderAdapter* renderer);
    
    // Завершение работы
    void Shutdown();
    
    // Обновление интерфейса (вызывается каждый кадр)
    void Update(World* world, float deltaTime);
    
    // Рендеринг интерфейса
    void Render();
    
    // Проверка, захватывает ли ImGui ввод
    bool WantCaptureMouse() const;
    bool WantCaptureKeyboard() const;
    
    // Режимы редактора
    bool IsPlayMode() const { return m_isPlayMode; }
    void SetPlayMode(bool playMode) { m_isPlayMode = playMode; }
    bool HasViewportRect() const { return m_viewportWidth > 1.0f && m_viewportHeight > 1.0f; }
    float GetViewportX() const { return m_viewportX; }
    float GetViewportY() const { return m_viewportY; }
    float GetViewportWidth() const { return m_viewportWidth; }
    float GetViewportHeight() const { return m_viewportHeight; }
    float GetViewportAspectRatio() const { return HasViewportRect() ? (m_viewportWidth / m_viewportHeight) : (800.0f / 600.0f); }
    void SetRuntimeStats(int renderedObjects, int activeCollisions);
    
    // Gizmo операции
    enum class GizmoOperation {
        Translate,
        Rotate,
        Scale
    };
    
    enum class GizmoMode {
        Local,
        World
    };

private:
    // Создание главного меню
    void CreateMainMenuBar(World* world);
    
    // Создание dockspace
    void CreateDockSpace();
    
    // Создание панелей
    void CreateSceneHierarchy(World* world);
    void CreateInspector(World* world);
    void CreateViewport(World* world);
    void CreateStatistics(World* world, float deltaTime);
    void CreateToolbar();
    void CreateAssetBrowser();
    void CreateConsole();
    
    // Gizmo манипуляторы
    void HandleGizmo(World* world);
    void DrawGizmo(World* world, uint32_t entity);
    void CreatePrimitive(World* world, int primitiveType, const char* name);
    void SaveScene(World* world, const char* path);
    void LoadScene(World* world, const char* path);
    void NewScene(World* world);
    static void AllocateImGuiSrvDescriptor(ImGui_ImplDX12_InitInfo* info,
                                           D3D12_CPU_DESCRIPTOR_HANDLE* outCpuHandle,
                                           D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandle);
    static void FreeImGuiSrvDescriptor(ImGui_ImplDX12_InitInfo* info,
                                       D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
                                       D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle);
    
    // Выбранная сущность
    uint32_t m_selectedEntity;
    bool m_hasSelectedEntity;
    
    // Режим редактора
    bool m_isPlayMode;
    
    // Gizmo настройки
    GizmoOperation m_gizmoOperation;
    GizmoMode m_gizmoMode;
    bool m_useSnap;
    float m_snapValues[3]; // для translate, rotate, scale
    
    // Статистика
    float m_frameTime;
    float m_fps;
    int m_frameCount;
    float m_fpsTimer;
    
    // Показывать ли панели
    bool m_showSceneHierarchy;
    bool m_showInspector;
    bool m_showViewport;
    bool m_showStatistics;
    bool m_showAssetBrowser;
    bool m_showConsole;
    bool m_showDemo;
    
    // Viewport настройки
    bool m_viewportFocused;
    bool m_viewportHovered;
    float m_viewportX;
    float m_viewportY;
    float m_viewportWidth;
    float m_viewportHeight;
    
    int m_renderedObjects;
    int m_activeCollisions;
    size_t m_estimatedResourceBytes;
    int m_resourceCount;
    
    bool m_dx12BackendInitialized;
    D3D12_CPU_DESCRIPTOR_HANDLE m_imguiSrvCpuStart;
    D3D12_GPU_DESCRIPTOR_HANDLE m_imguiSrvGpuStart;
    UINT m_imguiSrvDescriptorSize;
    std::vector<uint8_t> m_imguiSrvDescriptorUsed;
    
    // Инициализирован ли ImGui
    bool m_initialized;
    
    // Ссылка на рендер адаптер
    RenderAdapter* m_renderer;
};
