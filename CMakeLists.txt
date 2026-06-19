cmake_minimum_required(VERSION 3.15)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg.json")
    set(VCPKG_MANIFEST_MODE ON CACHE BOOL "" FORCE)
    set(VCPKG_MANIFEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE PATH "" FORCE)
endif()

project(GameEngineProjects)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build the final editor-engine by default. Historical lab snapshots remain
# available for comparison without becoming part of the main deliverable.
option(BUILD_LEGACY_LABS "Build AID1.1-AID4.1 historical lab snapshots" OFF)

if(BUILD_LEGACY_LABS)
    set(PROJECT_DIRS AID1.1 AID2.1 AID3.1 AID4.1 AID5.1)
else()
    set(PROJECT_DIRS AID5.1)
endif()

foreach(LAB_DIR IN LISTS PROJECT_DIRS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/${LAB_DIR}/CMakeLists.txt")
        add_subdirectory("src/${LAB_DIR}")
    endif()
endforeach()
