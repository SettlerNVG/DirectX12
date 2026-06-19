cmake_minimum_required(VERSION 3.20)

# Устанавливаем vcpkg toolchain
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../../vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

project(AID5.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(AID51_VENDORED_VCPKG_PREFIX "${CMAKE_CURRENT_LIST_DIR}/vcpkg_installed/x64-windows/x64-windows")
if(EXISTS "${AID51_VENDORED_VCPKG_PREFIX}/share/imgui/imgui-config.cmake")
    list(PREPEND CMAKE_PREFIX_PATH "${AID51_VENDORED_VCPKG_PREFIX}")
endif()

# Поиск пакетов vcpkg
find_package(imgui CONFIG REQUIRED)
find_package(imguizmo CONFIG REQUIRED)

# Исходные файлы
set(SOURCES
    main.cpp
    
    # Core
    Core/Application.cpp
    Core/Timer.cpp
    
    # ECS
    ECS/World.cpp
    
    # Input
    Input/InputManager.cpp
    
    # Physics
    Physics/PhysicsSystem.cpp
    
    # Rendering
    Rendering/D3D12Adapter.cpp
    
    # Editor (новое для AID5.1)
    Editor/EditorGUI.cpp
    Resources/ResourceCatalog.cpp
    
    # States
    States/StateManager.cpp
    States/LoadingState.cpp
    States/MenuState.cpp
    States/GameplayState.cpp
    States/PhysicsGameplayState.cpp
    States/EditorState.cpp
    
    # Systems
    Systems/RenderSystem.cpp
    Systems/CameraSystem.cpp
    Systems/DebugRenderSystem.cpp
    
    # Utils
    Utils/Logger.cpp
)

# Заголовочные файлы
set(HEADERS
    # Components
    Components/Transform.h
    Components/Tag.h
    Components/MeshRenderer.h
    Components/Camera.h
    Components/Hierarchy.h
    Components/Rigidbody.h
    Components/Collider.h
    
    # Core
    Core/Application.h
    Core/Timer.h
    
    # ECS
    ECS/Entity.h
    ECS/Component.h
    ECS/System.h
    ECS/World.h
    
    # Editor (новое для AID5.1)
    Editor/EditorGUI.h
    Resources/ResourceCatalog.h
    
    # Events
    Events/EventBus.h
    Events/CollisionEvent.h
    
    # Input
    Input/InputManager.h
    
    # Physics
    Physics/PhysicsSystem.h
    
    # Rendering
    Rendering/RenderAdapter.h
    Rendering/D3D12Adapter.h
    
    # States
    States/GameState.h
    States/StateManager.h
    States/LoadingState.h
    States/MenuState.h
    States/GameplayState.h
    States/PhysicsGameplayState.h
    States/EditorState.h
    
    # Systems
    Systems/RenderSystem.h
    Systems/CameraSystem.h
    Systems/DebugRenderSystem.h
    
    # Utils
    Utils/Logger.h
)

# Создаём исполняемый файл (WIN32 для Windows приложения)
add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS})

if(MSVC)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        VS_GLOBAL_VcpkgEnabled false
        VS_GLOBAL_VcpkgEnableManifest false
    )
endif()

# Линкуем DirectX 12 библиотеки и vcpkg пакеты
target_link_libraries(${PROJECT_NAME}
    imgui::imgui
    imguizmo::imguizmo
    d3d12.lib
    dxgi.lib
    d3dcompiler.lib
)

# Настройки компилятора
target_compile_definitions(${PROJECT_NAME} PRIVATE
    _WINDOWS
    UNICODE
    _UNICODE
    IMGUI_IMPL_WIN32_DISABLE_GAMEPAD
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

# Устанавливаем рабочую директорию для Visual Studio
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

# Группировка файлов в Visual Studio
source_group("Source Files\\Core" FILES Core/Application.cpp Core/Timer.cpp)
source_group("Source Files\\ECS" FILES ECS/World.cpp)
source_group("Source Files\\Input" FILES Input/InputManager.cpp)
source_group("Source Files\\Physics" FILES Physics/PhysicsSystem.cpp)
source_group("Source Files\\Rendering" FILES Rendering/D3D12Adapter.cpp)
source_group("Source Files\\Resources" FILES Resources/ResourceCatalog.cpp)
source_group("Source Files\\States" FILES States/StateManager.cpp States/LoadingState.cpp States/MenuState.cpp States/GameplayState.cpp States/PhysicsGameplayState.cpp States/EditorState.cpp)
source_group("Source Files\\Editor" FILES Editor/EditorGUI.cpp)
source_group("Source Files\\Systems" FILES Systems/RenderSystem.cpp Systems/CameraSystem.cpp Systems/DebugRenderSystem.cpp)
source_group("Source Files\\Utils" FILES Utils/Logger.cpp)

source_group("Header Files\\Components" FILES ${HEADERS})
# Информационные сообщения
message(STATUS "=== AID5.1 WYSIWYG Editor Configuration ===")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Using Dear ImGui with DirectX 12 backend")
message(STATUS "Using ImGuizmo for 3D manipulators")
message(STATUS "vcpkg integration: ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "============================================")
