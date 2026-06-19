cmake_minimum_required(VERSION 3.15)
project(AID3.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json CONFIG REQUIRED)

# Source files
set(SOURCES
    main.cpp
    Core/Application.cpp
    Core/Timer.cpp
    ECS/World.cpp
    Rendering/D3D12Adapter.cpp
    Resources/HotReloadWatcher.cpp
    Resources/Mesh.cpp
    Resources/MeshLoader.cpp
    Resources/ResourceManager.cpp
    Resources/ResourceManifest.cpp
    Resources/Shader.cpp
    Resources/ShaderLoader.cpp
    Resources/Texture.cpp
    Resources/TextureLoader.cpp
    States/LoadingState.cpp
    States/GameplayState.cpp
    States/MenuState.cpp
    States/StateManager.cpp
    Systems/RenderSystem.cpp
    Systems/CameraSystem.cpp
    Utils/Logger.cpp
)

# Header files
set(HEADERS
    Components/Camera.h
    Components/Hierarchy.h
    Components/MeshRenderer.h
    Components/Tag.h
    Components/Transform.h
    Core/Application.h
    Core/Timer.h
    ECS/Component.h
    ECS/Entity.h
    ECS/System.h
    ECS/World.h
    Rendering/D3D12Adapter.h
    Rendering/RenderAdapter.h
    Resources/HotReloadWatcher.h
    Resources/Mesh.h
    Resources/MeshLoader.h
    Resources/Resource.h
    Resources/ResourceManager.h
    Resources/ResourceManifest.h
    Resources/Shader.h
    Resources/ShaderLoader.h
    Resources/Texture.h
    Resources/TextureLoader.h
    Resources/stb_image.h
    Resources/json.hpp
    States/LoadingState.h
    States/GameplayState.h
    States/GameState.h
    States/MenuState.h
    States/StateManager.h
    Systems/RenderSystem.h
    Systems/CameraSystem.h
    Utils/Logger.h
)

# Create executable
add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Preprocessor definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
    _WINDOWS
    UNICODE
    _UNICODE
)

# Compiler options
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W3
        /sdl
        $<$<CONFIG:Release>:/O2 /Oi>
    )
endif()

# Link libraries (DirectX 12)
target_link_libraries(${PROJECT_NAME} PRIVATE
    nlohmann_json::nlohmann_json
    d3d12.lib
    dxgi.lib
    d3dcompiler.lib
)

# Set output directories
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
)
