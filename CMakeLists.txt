cmake_minimum_required(VERSION 3.20)

project(PhoenixEngine 
    VERSION 1.0.0 
    DESCRIPTION "Phoenix Engine - Cross-platform 3D Rendering Engine"
    LANGUAGES CXX C
)

# C++20 standard (required by bgfx)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================================
# Sanitizer Support (AddressSanitizer + UndefinedBehaviorSanitizer)
# ============================================================================
option(ENABLE_SANITIZERS "Enable ASAN and UBSAN for debugging" OFF)
option(SANITIZE_THREAD "Use ThreadSanitizer instead of ASAN" OFF)

if(ENABLE_SANITIZERS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        message(STATUS "🔍 Enabling sanitizers...")
        
        # Base sanitizer flags
        set(SANITIZER_FLAGS "-fsanitize=address,undefined")
        set(SANITIZER_LINK_FLAGS "-fsanitize=address,undefined")
        
        # ThreadSanitizer (alternative to ASAN)
        if(SANITIZE_THREAD)
            set(SANITIZER_FLAGS "-fsanitize=thread,undefined")
            set(SANITIZER_LINK_FLAGS "-fsanitize=thread,undefined")
            message(STATUS "  Using ThreadSanitizer")
        else()
            message(STATUS "  Using AddressSanitizer + UBSAN")
        endif()
        
        # Add to compiler flags
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SANITIZER_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS}")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZER_LINK_FLAGS}")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${SANITIZER_LINK_FLAGS}")
        
        # ASAN specific options
        if(NOT SANITIZE_THREAD)
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize-address-use-after-scope")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize-address-use-after-scope")
        endif()
        
        # Disable for Release builds
        if(CMAKE_BUILD_TYPE STREQUAL "Release")
            message(WARNING "⚠️ Sanitizers are disabled in Release builds")
        endif()
        
    elseif(MSVC)
        # MSVC uses different sanitizer options
        message(STATUS "🔍 MSVC: Using /fsanitize=address")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /fsanitize=address")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fsanitize=address")
    else()
        message(WARNING "⚠️ Sanitizers not supported for this compiler")
    endif()
endif()

# Print sanitizer status
if(ENABLE_SANITIZERS)
    message(STATUS "  Sanitizer flags: ${SANITIZER_FLAGS}")
else()
    message(STATUS "🔍 Sanitizers disabled (use -DENABLE_SANITIZERS=ON to enable)")
endif()
# ============================================================================

# 编译选项
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall -Wextra -Wpedantic
        -Wshadow -Wcast-qual
        -Wno-unused-parameter
    )
    
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
    else()
        add_compile_options(-O3)
    endif()
    
elseif(MSVC)
    add_compile_options(/W4 /MP)
    add_compile_options(/permissive-)
endif()

# 输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 查找或获取 bgfx (bgfx.cmake includes SPIRV-Headers, SPIRV-Tools, glslang)
find_package(bgfx CONFIG QUIET)
if(NOT bgfx_FOUND)
    message(STATUS "Fetching bgfx from GitHub...")
    include(FetchContent)
    FetchContent_Declare(
        bgfx
        GIT_REPOSITORY https://github.com/bkaradzic/bgfx.cmake.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(bgfx)
endif()

# 获取 shaderc (C++ library for GLSL to SPIR-V compilation)
# Note: shaderc requires SPIRV-Tools which is included with bgfx
# 在 FetchContent 之前设置选项以避免与 bgfx 的 shaderc 目标冲突
set(SHADERC_SKIP_TESTS ON CACHE BOOL "Skip shaderc tests" FORCE)
set(SHADERC_SKIP_EXAMPLES ON CACHE BOOL "Skip shaderc examples" FORCE)
set(SHADERC_SKIP_COPYRIGHT_CHECK ON CACHE BOOL "Skip copyright check" FORCE)
set(SHADERC_SKIP_GLSLC ON CACHE BOOL "Skip glslc tool" FORCE)
set(SHADERC_SKIP_INSTALL ON CACHE BOOL "Skip installation" FORCE)

include(FetchContent)
FetchContent_Declare(
    shaderc
    GIT_REPOSITORY https://github.com/google/shaderc.git
    GIT_TAG v2023.6
    GIT_SHALLOW TRUE
)
FetchContent_GetProperties(shaderc)
if(NOT shaderc_POPULATED)
    FetchContent_Populate(shaderc)
    # 构建 libshaderc 静态库，重命名避免与 bgfx 冲突
    add_library(shaderc_static STATIC
        ${shaderc_SOURCE_DIR}/libshaderc/src/shaderc.cc
    )
    target_include_directories(shaderc_static PUBLIC
        ${shaderc_SOURCE_DIR}/libshaderc/include
        ${SPIRV-Headers_SOURCE_DIR}/include
        ${SPIRV-Tools_SOURCE_DIR}/include
    )
    target_link_libraries(shaderc_static PRIVATE
        SPIRV-Tools-static
        SPIRV-Tools-opt
        glslang
    )
    set_target_properties(shaderc_static PROPERTIES 
        OUTPUT_NAME "shaderc_lib"
        POSITION_INDEPENDENT_CODE ON
    )
endif()

# 查找 OpenGL
find_package(OpenGL QUIET)

# 查找 Vulkan
find_package(Vulkan QUIET)

# ============ Third-party Libraries ============

# tinygltf for glTF loading
option(USE_TINYGLTF "Use tinygltf for glTF loading" ON)
if(USE_TINYGLTF)
    include(FetchContent)
    FetchContent_Declare(
        tinygltf
        GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
        GIT_TAG v2.8.16
    )
    FetchContent_MakeAvailable(tinygltf)
endif()

# stb_image for texture loading
option(USE_STB_IMAGE "Use stb_image for texture loading" ON)
if(USE_STB_IMAGE)
    include(FetchContent)
    FetchContent_Declare(
        stb
        GIT_REPOSITORY https://github.com/nothings/stb.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(stb)
endif()

# Assimp for FBX and other formats
option(USE_ASSIMP "Use Assimp for FBX loading" ON)
if(USE_ASSIMP)
    find_package(assimp QUIET)
    if(NOT assimp_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            assimp
            GIT_REPOSITORY https://github.com/assimp/assimp.git
            GIT_TAG v5.3.1
        )
        FetchContent_MakeAvailable(assimp)
    endif()
endif()

# GDAL for GeoTIFF terrain loading
option(USE_GDAL "Use GDAL for terrain loading" OFF)
if(USE_GDAL)
    find_package(GDAL QUIET)
endif()

# ============ Phoenix Engine Library ============

add_library(phoenix_engine STATIC
    # Core
    src/core/MemoryManager.cpp
    src/core/Logger.cpp
    src/core/Timer.cpp
    
    # Render Device
    src/render/RenderDevice.cpp
    
    # Shader System
    src/render/Shader.cpp
    src/render/BuiltinShaders.cpp
    
    # Render Pipeline
    src/render/Pipeline.cpp
    
    # Resource Management
    src/render/Resources.cpp
    
    # Resource Loaders (Phase 3)
    src/resource/Mesh.cpp
    src/resource/Texture.cpp
    src/resource/PointCloud.cpp
    src/resource/Terrain.cpp
    src/resource/GLTFLoader.cpp
    src/resource/FBXLoader.cpp
    src/resource/OBJLoader.cpp
    src/resource/STLLoader.cpp
    src/resource/TextureLoader.cpp
    src/resource/PointCloudLoader.cpp
    src/resource/TerrainLoader.cpp
    src/resource/ResourceManager.cpp
    
    # Mobile Optimization (Phase 4)
    src/mobile/power/PowerManager.cpp
    src/mobile/memory/MemoryManager.cpp
    src/mobile/input/TouchInput.cpp
    src/mobile/platform/MobilePlatform.cpp
    src/mobile/profiler/MobileProfiler.cpp
)

target_include_directories(phoenix_engine PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(phoenix_engine PUBLIC
    bgfx
    bx
    bimg
    shaderc_static
)

# Third-party libraries
# shaderc is always used now
target_compile_definitions(phoenix_engine PRIVATE PHOENIX_USE_SHADERC)
if(USE_TINYGLTF)
    target_link_libraries(phoenix_engine PRIVATE tinygltf)
    target_compile_definitions(phoenix_engine PRIVATE PHOENIX_USE_TINYGLTF)
endif()

if(USE_STB_IMAGE)
    target_include_directories(phoenix_engine PRIVATE ${stb_SOURCE_DIR})
    target_compile_definitions(phoenix_engine PRIVATE PHOENIX_USE_STB_IMAGE)
endif()

if(USE_ASSIMP)
    target_link_libraries(phoenix_engine PRIVATE assimp::assimp)
    target_compile_definitions(phoenix_engine PRIVATE PHOENIX_USE_ASSIMP)
endif()

if(USE_GDAL AND GDAL_FOUND)
    target_link_libraries(phoenix_engine PRIVATE GDAL::GDAL)
    target_compile_definitions(phoenix_engine PRIVATE PHOENIX_USE_GDAL)
endif()

if(OpenGL_FOUND)
    target_link_libraries(phoenix_engine PRIVATE OpenGL::GL)
endif()

if(Vulkan_FOUND)
    target_link_libraries(phoenix_engine PRIVATE Vulkan::Vulkan)
endif()

# Platform related库
if(WIN32)
    target_link_libraries(phoenix_engine PRIVATE 
        dwmapi.lib psapi.lib dxgi.lib d3d11.lib d3d12.lib
    )
elseif(UNIX AND NOT APPLE)
    target_link_libraries(phoenix_engine PRIVATE 
        dl pthread
    )
elseif(APPLE)
    find_library(COCOA_FRAMEWORK Cocoa)
    find_library(METAL_FRAMEWORK Metal)
    find_library(METALKIT_FRAMEWORK MetalKit)
    target_link_libraries(phoenix_engine PRIVATE
        ${COCOA_FRAMEWORK} ${METAL_FRAMEWORK} ${METALKIT_FRAMEWORK}
    )
endif()

# 编译定义
target_compile_definitions(phoenix_engine PUBLIC
    PHOENIX_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    PHOENIX_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    PHOENIX_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

# ============ Unit Tests ============

option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    enable_testing()
    
    # Google Test
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    FetchContent_MakeAvailable(googletest)
    
    # Main test executable
    add_executable(phoenix_tests
        tests/test_main.cpp
        tests/test_math.cpp
        tests/test_security.cpp
        
        # Render tests
        tests/render/test_render_device.cpp
        tests/render/test_integration.cpp
        
        # Scene tests
        tests/scene/test_ecs.cpp
        tests/scene/test_spatial.cpp
        tests/scene/test_scene_graph.cpp
        tests/scene/test_lod.cpp
        
        # Animation tests (Phase 3)
        tests/scene/test_animation.cpp
        tests/scene/test_physics.cpp
        tests/scene/test_particle_system.cpp
        
        # Resource tests (Phase 3)
        tests/resource/test_resource_loaders.cpp
    )
    
    target_link_libraries(phoenix_tests PRIVATE
        phoenix_engine
        GTest::gtest_main
    )
    
    target_include_directories(phoenix_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
    
    add_test(NAME PhoenixTests COMMAND phoenix_tests)
    
    set_tests_properties(PhoenixTests PROPERTIES
        TIMEOUT 120
        LABELS "unit"
    )
endif()

# ============ Example Programs ============

option(BUILD_EXAMPLES "Build example programs" ON)
if(BUILD_EXAMPLES)
    # Basic render example
    add_executable(basic-render
        examples/basic-render/main.cpp
    )
    
    target_link_libraries(basic-render PRIVATE
        phoenix_engine
    )
    
    target_include_directories(basic-render PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
    
    # Model viewer example (Phase 3)
    add_executable(model-viewer
        examples/model-viewer/main.cpp
    )
    
    target_link_libraries(model-viewer PRIVATE
        phoenix_engine
    )
    
    target_include_directories(model-viewer PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
    
    # Animation demo example (Phase 3)
    add_executable(animation-demo
        examples/animation-demo/main.cpp
        src/scene/skeleton.cpp
        src/scene/animator.cpp
        src/scene/morph_animation.cpp
        src/scene/gltf_loader.cpp
        src/scene/physics.cpp
        src/scene/particle_system.cpp
    )
    
    target_link_libraries(animation-demo PRIVATE
        phoenix_engine
    )
    
    target_include_directories(animation-demo PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
endif()

# ============ Benchmarks ============

option(BUILD_BENCHMARKS "Build benchmarks" ON)
if(BUILD_BENCHMARKS)
    # Benchmark for resource loading
    add_executable(benchmark-resources
        benchmarks/resource_loading.cpp
    )
    
    target_link_libraries(benchmark-resources PRIVATE
        phoenix_engine
    )
    
    # Animation system benchmark (Phase 3)
    add_executable(benchmark-animation
        benchmarks/scene/benchmark_animation.cpp
        src/scene/skeleton.cpp
        src/scene/animator.cpp
        src/scene/morph_animation.cpp
        src/scene/particle_system.cpp
    )
    
    target_link_libraries(benchmark-animation PRIVATE
        phoenix_engine
    )
    
    target_include_directories(benchmark-animation PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
endif()

# ============ Installation ============

include(GNUInstallDirs)

install(TARGETS phoenix_engine
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/phoenix
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp"
)

# ============ CPack Configuration ============

set(CPACK_PACKAGE_NAME "PhoenixEngine")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Cross-platform 3D Rendering Engine")
set(CPACK_PACKAGE_VENDOR "Phoenix Engine Team")
include(CPack)

# ============ Build Summary ============

message(STATUS "")
message(STATUS "Phoenix Engine Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build tests: ${BUILD_TESTS}")
message(STATUS "  Build examples: ${BUILD_EXAMPLES}")
message(STATUS "  Build benchmarks: ${BUILD_BENCHMARKS}")
message(STATUS "")
message(STATUS "Third-party Libraries:")
message(STATUS "  bgfx: ${bgfx_FOUND}")
message(STATUS "  shaderc: enabled (GLSL to SPIR-V)")
message(STATUS "  OpenGL: ${OpenGL_FOUND}")
message(STATUS "  Vulkan: ${Vulkan_FOUND}")
message(STATUS "  tinygltf: ${USE_TINYGLTF}")
message(STATUS "  stb_image: ${USE_STB_IMAGE}")
message(STATUS "  Assimp: ${USE_ASSIMP}")
message(STATUS "  GDAL: ${USE_GDAL}")
message(STATUS "")
