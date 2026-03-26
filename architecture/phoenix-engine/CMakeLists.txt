# Phoenix Engine - CMake Build Configuration
# Cross-platform game engine with security-first architecture

cmake_minimum_required(VERSION 3.20)
project(PhoenixEngine 
    VERSION 1.0.0 
    DESCRIPTION "Phoenix Game Engine - Security-First Architecture"
    LANGUAGES CXX C
)

# ============================================================================
# C++ Standard Configuration
# ============================================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================================
# Cross-Platform Support
# ============================================================================
if(WIN32)
    add_definitions(-DPHOENIX_PLATFORM_WINDOWS)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
elseif(UNIX AND NOT APPLE)
    add_definitions(-DPHOENIX_PLATFORM_LINUX)
elseif(APPLE)
    add_definitions(-DPHOENIX_PLATFORM_MACOS)
endif()

# Build type configuration
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DPHOENIX_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")

# ============================================================================
# Compiler Warnings (MISRA C++ 2023 Compatible)
# ============================================================================
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Werror=return-type
        -Wno-unused-parameter
        -Wshadow
        -Wformat=2
        -Wcast-qual
        -Wcast-align
        -Wwrite-strings
        -Wconversion
        -Wsign-conversion
    )
elseif(MSVC)
    add_compile_options(
        /W4
        /WX-
        /permissive-
        /Zc:__cplusplus
    )
endif()

# ============================================================================
# Dependencies
# ============================================================================

# bgfx Graphics Library
find_package(bgfx CONFIG QUIET)
if(NOT bgfx_FOUND)
    # Fetch bgfx if not found
    include(FetchContent)
    FetchContent_Declare(
        bgfx
        GIT_REPOSITORY https://github.com/bkaradzic/bgfx.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(bgfx)
endif()

# bx - Base library for bgfx
find_package(bx CONFIG QUIET)
if(NOT bx_FOUND)
    FetchContent_Declare(
        bx
        GIT_REPOSITORY https://github.com/bkaradzic/bx.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(bx)
endif()

# bimg - Image library for bgfx
find_package(bimg CONFIG QUIET)
if(NOT bimg_FOUND)
    FetchContent_Declare(
        bimg
        GIT_REPOSITORY https://github.com/bkaradzic/bimg.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(bimg)
endif()

# ============================================================================
# Rust Security Core Integration
# ============================================================================
find_package(Rust REQUIRED)

# Build Rust security core library
execute_process(
    COMMAND cargo build --release
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/rust-security-core
    RESULT_VARIABLE RUST_BUILD_RESULT
)

if(NOT RUST_BUILD_RESULT EQUAL 0)
    message(FATAL_ERROR "Rust security core build failed")
endif()

# Link Rust library
if(WIN32)
    set(RUST_SECURITY_LIB ${CMAKE_SOURCE_DIR}/rust-security-core/target/release/phoenix_security.lib)
else()
    set(RUST_SECURITY_LIB ${CMAKE_SOURCE_DIR}/rust-security-core/target/release/libphoenix_security.a)
endif()

# ============================================================================
# Phoenix Engine Library
# ============================================================================
add_library(phoenix_engine STATIC
    # Core
    src/core/Engine.cpp
    src/core/Logger.cpp
    src/core/Timer.cpp
    src/core/MemoryManager.cpp
    
    # Math
    src/math/Vector3.cpp
    src/math/Vector4.cpp
    src/math/Matrix4.cpp
    src/math/Quaternion.cpp
    
    # Render
    src/render/Renderer.cpp
    src/render/Shader.cpp
    src/render/Mesh.cpp
    src/render/Texture.cpp
    src/render/Camera.cpp
    
    # Scene
    src/scene/Scene.cpp
    src/scene/Entity.cpp
    src/scene/Component.cpp
    src/scene/Transform.cpp
    
    # Resource
    src/resource/ResourceManager.cpp
    src/resource/AssetLoader.cpp
    src/resource/AssetCache.cpp
    
    # Security
    src/security/SecurityManager.cpp
    src/security/Validator.cpp
    src/security/Encryption.cpp
)

target_include_directories(phoenix_engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(phoenix_engine
    PUBLIC
        bgfx::bgfx
        bx::bx
        bimg::bimg
    PRIVATE
        ${RUST_SECURITY_LIB}
)

target_compile_definitions(phoenix_engine
    PUBLIC
        PHOENIX_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        PHOENIX_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        PHOENIX_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

# ============================================================================
# Examples
# ============================================================================
add_executable(phoenix_example_basic examples/basic.cpp)
target_link_libraries(phoenix_example_basic PRIVATE phoenix_engine)

add_executable(phoenix_example_rendering examples/rendering.cpp)
target_link_libraries(phoenix_example_rendering PRIVATE phoenix_engine)

# ============================================================================
# Testing Configuration
# ============================================================================
enable_testing()

# Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

# Test executables
add_executable(phoenix_tests
    tests/core/test_engine.cpp
    tests/core/test_logger.cpp
    tests/math/test_vector.cpp
    tests/math/test_matrix.cpp
    tests/render/test_renderer.cpp
    tests/scene/test_entity.cpp
    tests/resource/test_resource_manager.cpp
    tests/security/test_security.cpp
)

target_link_libraries(phoenix_tests
    PRIVATE
        phoenix_engine
        GTest::gtest_main
        GTest::gmock_main
)

include(GoogleTest)
gtest_discover_tests(phoenix_tests)

# ============================================================================
# Installation
# ============================================================================
include(GNUInstallDirs)

install(TARGETS phoenix_engine
    EXPORT phoenix_engine_targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/phoenix
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT phoenix_engine_targets
    FILE phoenix_engine-config.cmake
    NAMESPACE phoenix::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/phoenix_engine
)

# ============================================================================
# Documentation
# ============================================================================
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/docs)
    set(DOXYGEN_GENERATE_HTML YES)
    set(DOXYGEN_GENERATE_MAN NO)
    
    doxygen_add_docs(phoenix_docs
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/docs
        COMMENT "Generating API documentation with Doxygen"
    )
endif()

# ============================================================================
# CPack Configuration
# ============================================================================
set(CPACK_PACKAGE_NAME "PhoenixEngine")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VENDOR "Phoenix Engine Project")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Security-first cross-platform game engine")
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/LICENSE)
set(CPACK_RESOURCE_FILE_README ${CMAKE_SOURCE_DIR}/README.md)

include(CPack)
