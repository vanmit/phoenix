# Phoenix Engine WebAssembly Build
# CMakeLists.txt for WASM target

cmake_minimum_required(VERSION 3.20)

# Must be set before project()
if(DEFINED ENV{EMSDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake")
else()
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Emscripten.cmake")
endif()

project(PhoenixEngineWASM
    VERSION 1.0.0
    DESCRIPTION "Phoenix Engine WebAssembly Build"
    LANGUAGES CXX C
)

# C++17 standard (required for bgfx)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist)

# ============ WASM-specific Options ============

option(WASM_BUILD_MINIMAL "Build minimal WASM module (smaller size)" OFF)
option(WASM_BUILD_WITH_WEBGPU "Include WebGPU support" ON)
option(WASM_BUILD_WITH_WEBGL2 "Include WebGL2 support" ON)
option(WASM_ENABLE_PROFILING "Enable performance profiling" OFF)
option(WASM_ENABLE_ASSERTIONS "Enable runtime assertions" OFF)

# Size optimization
if(WASM_BUILD_MINIMAL)
    add_compile_definitions(
        PHOENIX_WASM_MINIMAL=1
        BGFX_CONFIG_MAX_VIEWS=64
        BGFX_CONFIG_MAX_DRAW_CALLS=1024
    )
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os")
else()
    add_compile_definitions(
        BGFX_CONFIG_MAX_VIEWS=256
        BGFX_CONFIG_MAX_DRAW_CALLS=4096
    )
endif()

# WebGPU support
if(WASM_BUILD_WITH_WEBGPU)
    add_compile_definitions(PHOENIX_WEBGPU_ENABLED=1)
    list(APPEND WASM_LINK_FLAGS "-s USE_WEBGPU=1")
endif()

# WebGL2 support
if(WASM_BUILD_WITH_WEBGL2)
    add_compile_definitions(PHOENIX_WEBGL2_ENABLED=1)
    list(APPEND WASM_LINK_FLAGS "-s FULL_ES3=1" "-s USE_WEBGL2=1")
endif()

# Profiling
if(WASM_ENABLE_PROFILING)
    add_compile_definitions(PHOENIX_WASM_PROFILING=1)
    list(APPEND WASM_LINK_FLAGS "--profiling-funcs")
endif()

# Assertions
if(WASM_ENABLE_ASSERTIONS)
    add_compile_definitions(PHOENIX_WASM_ASSERTIONS=1 BGFX_CONFIG_DEBUG=1)
    list(APPEND WASM_LINK_FLAGS "-s ASSERTIONS=1")
endif()

# ============ Source Files ============

# Core engine sources
set(PHOENIX_WASM_SOURCES
    # WASM-specific bindings
    src/wasm_bindings.cpp
    src/wasm_main.cpp
    src/wasm_filesystem.cpp
    src/wasm_graphics.cpp
    src/wasm_webgpu.cpp
    src/wasm_webgl.cpp
    
    # Core engine (subset for WASM)
    ../src/core/MemoryManager.cpp
    ../src/core/Logger.cpp
    ../src/core/Timer.cpp
    
    # Render
    ../src/render/RenderDevice.cpp
    ../src/render/Shader.cpp
    ../src/render/Pipeline.cpp
    ../src/render/Resources.cpp
    
    # Resources
    ../src/resource/Mesh.cpp
    ../src/resource/Texture.cpp
    ../src/resource/ResourceManager.cpp
    ../src/resource/GLTFLoader.cpp
    
    # Scene
    ../src/scene/skeleton.cpp
    ../src/scene/animator.cpp
)

# ============ Include Directories ============

set(PHOENIX_WASM_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/../include
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/../third-party
)

# ============ WASM Module ============

add_executable(phoenix-wasm ${PHOENIX_WASM_SOURCES})

target_include_directories(phoenix-wasm PUBLIC
    ${PHOENIX_WASM_INCLUDE_DIRS}
)

# Link libraries from parent project
target_link_libraries(phoenix-wasm PRIVATE
    bgfx::bgfx
)

# WASM-specific link flags
set(WASM_EXPORTED_FUNCTIONS
    "_malloc"
    "_free"
    "_phoenix_init"
    "_phoenix_shutdown"
    "_phoenix_frame"
    "_phoenix_resize"
    "_phoenix_load_resource"
    "_phoenix_create_scene"
    "_phoenix_destroy_scene"
    "_phoenix_get_texture"
    "_phoenix_submit_frame"
)

set(WASM_EXPORTED_RUNTIME_METHODS
    "ccall"
    "cwrap"
    "FS"
    "IDBFS"
    "preRun"
    "postRun"
    "setValue"
    "getValue"
    "stringToUTF8"
    "UTF8ToString"
    "writeArrayToMemory"
)

set(WASM_EXTRA_LINK_FLAGS
    "-s WASM=1"
    "-s ALLOW_MEMORY_GROWTH=1"
    "-s INITIAL_MEMORY=134217728"
    "-s MAXIMUM_MEMORY=2147483648"
    "-s ENVIRONMENT=web"
    "-s MODULARIZE=1"
    "-s EXPORT_NAME='createPhoenixEngine'"
    "-s ASYNCIFY=1"
    "-s ASYNCIFY_STACK_SIZE=131072"
    "-s EXPORTED_FUNCTIONS=[${WASM_EXPORTED_FUNCTIONS}]"
    "-s EXPORTED_RUNTIME_METHODS=[${WASM_EXPORTED_RUNTIME_METHODS}]"
    "-s FORCE_FILESYSTEM=1"
    "-s SINGLE_FILE=0"
    "-s WASM_BIGINT=1"
    "-s SAFE_HEAP=0"
)

target_link_options(phoenix-wasm PRIVATE
    ${WASM_EXTRA_LINK_FLAGS}
    ${WASM_LINK_FLAGS}
)

# Generate TypeScript definitions
add_custom_command(TARGET phoenix-wasm POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Generating TypeScript definitions..."
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/generate_typescript_defs.sh
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/phoenix-wasm.js
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/phoenix-wasm.d.ts
    COMMENT "Generating TypeScript type definitions"
)

# Generate optimized production build
add_custom_target(phoenix-wasm-opt
    DEPENDS phoenix-wasm
    COMMAND wasm-opt ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/phoenix-wasm.wasm
            -O3
            -o ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/phoenix-wasm.opt.wasm
    COMMAND ${CMAKE_COMMAND} -E echo "Optimized WASM size:"
    COMMAND ls -lh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/phoenix-wasm.opt.wasm
    COMMENT "Optimizing WASM binary"
)

# Generate brotli compressed version
add_custom_target(phoenix-wasm-br
    DEPENDS phoenix-wasm
    COMMAND brotli --quality=11 -z ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/phoenix-wasm.wasm
    COMMAND ${CMAKE_COMMAND} -E echo "Brotli compressed WASM size:"
    COMMAND ls -lh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/phoenix-wasm.wasm.br
    COMMENT "Compressing WASM with Brotli"
)

# ============ Test Module ============

add_executable(phoenix-wasm-tests
    tests/test_wasm_bindings.cpp
    tests/test_memory.cpp
    tests/test_graphics.cpp
)

target_include_directories(phoenix-wasm-tests PRIVATE
    ${PHOENIX_WASM_INCLUDE_DIRS}
)

target_link_libraries(phoenix-wasm-tests PRIVATE
    bgfx::bgfx
)

target_link_options(phoenix-wasm-tests PRIVATE
    ${WASM_EXTRA_LINK_FLAGS}
    "-s TEST=1"
)

# ============ Installation ============

install(TARGETS phoenix-wasm
    RUNTIME DESTINATION ${CMAKE_BINARY_DIR}/dist
)

install(FILES
    ${CMAKE_SOURCE_DIR}/js/phoenix-wasm-api.js
    ${CMAKE_SOURCE_DIR}/js/phoenix-wasm-types.ts
    DESTINATION ${CMAKE_BINARY_DIR}/dist/js
)

install(DIRECTORY
    ${CMAKE_SOURCE_DIR}/examples/html/
    DESTINATION ${CMAKE_BINARY_DIR}/dist/examples
)

# ============ Build Summary ============

message(STATUS "")
message(STATUS "Phoenix Engine WASM Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Minimal Build: ${WASM_BUILD_MINIMAL}")
message(STATUS "  WebGPU Support: ${WASM_BUILD_WITH_WEBGPU}")
message(STATUS "  WebGL2 Support: ${WASM_BUILD_WITH_WEBGL2}")
message(STATUS "  Profiling: ${WASM_ENABLE_PROFILING}")
message(STATUS "  Assertions: ${WASM_ENABLE_ASSERTIONS}")
message(STATUS "")
