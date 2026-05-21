cmake_minimum_required(VERSION 3.15)
project(AID1.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(SOURCES
    main.cpp
    Core/Application.cpp
    Core/Timer.cpp
    Rendering/D3D12Adapter.cpp
    Rendering/ImGuiHelper.cpp
    Rendering/TextRenderer.cpp
    States/LoadingState.cpp
    States/GameplayState.cpp
    States/MenuState.cpp
    States/StateManager.cpp
    Utils/Logger.cpp
)

# Header files
set(HEADERS
    Core/Application.h
    Core/Timer.h
    Rendering/D3D12Adapter.h
    Rendering/ImGuiHelper.h
    Rendering/RenderAdapter.h
    Rendering/TextRenderer.h
    States/LoadingState.h
    States/GameplayState.h
    States/GameState.h
    States/MenuState.h
    States/StateManager.h
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
    d3d12.lib
    dxgi.lib
    d3dcompiler.lib
)

# Set output directories
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
)
