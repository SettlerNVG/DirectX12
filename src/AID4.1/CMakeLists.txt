cmake_minimum_required(VERSION 3.15)
project(AID4.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
    
    # States
    States/StateManager.cpp
    States/LoadingState.cpp
    States/MenuState.cpp
    States/GameplayState.cpp
    States/PhysicsGameplayState.cpp
    
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
    
    # Systems
    Systems/RenderSystem.h
    Systems/CameraSystem.h
    Systems/DebugRenderSystem.h
    
    # Utils
    Utils/Logger.h
)

# Создаём исполняемый файл (WIN32 для Windows приложения)
add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS})

# Линкуем DirectX 12 библиотеки
target_link_libraries(${PROJECT_NAME}
    d3d12.lib
    dxgi.lib
    d3dcompiler.lib
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    _WINDOWS
    UNICODE
    _UNICODE
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
source_group("Source Files\\States" FILES States/StateManager.cpp States/LoadingState.cpp States/MenuState.cpp States/GameplayState.cpp States/PhysicsGameplayState.cpp)
source_group("Source Files\\Systems" FILES Systems/RenderSystem.cpp Systems/CameraSystem.cpp Systems/DebugRenderSystem.cpp)
source_group("Source Files\\Utils" FILES Utils/Logger.cpp)

source_group("Header Files\\Components" FILES ${HEADERS})
