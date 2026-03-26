# Emscripten CMake Toolchain File
# Phoenix Engine WebAssembly Build Configuration
# 
# Usage: cmake -DCMAKE_TOOLCHAIN_FILE=../emscripten/Toolchain.cmake ..

set(CMAKE_SYSTEM_NAME Emscripten)
set(CMAKE_SYSTEM_VERSION 1)

# Emscripten compiler
set(CMAKE_C_COMPILER "emcc")
set(CMAKE_CXX_COMPILER "em++")

# Emscripten linker
set(CMAKE_LINKER "emcc")
set(CMAKE_AR "emar")
set(CMAKE_RANLIB "emranlib")

# Force static linking for WASM
set(BUILD_SHARED_LIBS OFF)

# WASM-specific flags
set(EMSCRIPTEN_FLAGS 
    "-s WASM=1"
    "-s ALLOW_MEMORY_GROWTH=1"
    "-s INITIAL_MEMORY=134217728"  # 128MB initial
    "-s MAXIMUM_MEMORY=4294967296"  # 4GB max
    "-s SUPPORT_LONGJMP=0"
    "-s DISABLE_EXCEPTION_CATCHING=0"
    "-s ENVIRONMENT=web"
    "-s MODULARIZE=1"
    "-s EXPORT_NAME='PhoenixEngine'"
    "-s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','FS','IDBFS','preRun','postRun']"
    "-s EXPORTED_FUNCTIONS=['_malloc','_free','_main']"
    "-s USE_WEBGPU=1"
    "-s USE_WEBGL2=1"
    "-s LEGACY_GL_EMULATION=0"
    "-s FULL_ES3=1"
    "-s ALLOW_TABLE_GROWTH=1"
    "-s STACK_SIZE=5242880"  # 5MB stack
    "-s ASYNCIFY=1"
    "-s ASYNCIFY_STACK_SIZE=131072"  # 128KB async stack
)

# Optimization flags for production
set(EMSCRIPTEN_RELEASE_FLAGS
    "-O3"
    "-s LTO=1"
    "-s FLAT_NAMESPACE=1"
    "--closure 1"  # Google Closure Compiler simple
    "-g0"
)

# Debug flags
set(EMSCRIPTEN_DEBUG_FLAGS
    "-O0"
    "-g4"  # Full debug info with source maps
    "-s ASSERTIONS=1"
    "-s SAFE_HEAP=1"
    "-s STACK_OVERFLOW_CHECK=2"
)

# SIMD optimization flags
set(EMSCRIPTEN_SIMD_FLAGS
    "-msimd128"
    "-mllvm -force-vector-width=4"
)

# Multi-threading flags (requires SharedArrayBuffer)
set(EMSCRIPTEN_THREAD_FLAGS
    "-s USE_PTHREADS=1"
    "-s PTHREAD_POOL_SIZE=4"
    "-s SHARED_MEMORY=1"
    "-s LOCKS_SUPPORTING_ASYNC=1"
    "-s PROXY_TO_PTHREAD=1"
)

# Combine flags based on build type
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EMSCRIPTEN_FLAGS} ${EMSCRIPTEN_RELEASE_FLAGS} ${EMSCRIPTEN_SIMD_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EMSCRIPTEN_FLAGS} ${EMSCRIPTEN_RELEASE_FLAGS} ${EMSCRIPTEN_SIMD_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${EMSCRIPTEN_FLAGS} ${EMSCRIPTEN_RELEASE_FLAGS}")
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EMSCRIPTEN_FLAGS} ${EMSCRIPTEN_DEBUG_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EMSCRIPTEN_FLAGS} ${EMSCRIPTEN_DEBUG_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${EMSCRIPTEN_FLAGS} ${EMSCRIPTEN_DEBUG_FLAGS}")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EMSCRIPTEN_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EMSCRIPTEN_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${EMSCRIPTEN_FLAGS}")
endif()

# Enable threading if requested
option(ENABLE_WASM_THREADS "Enable WebAssembly threading support" OFF)
if(ENABLE_WASM_THREADS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EMSCRIPTEN_THREAD_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EMSCRIPTEN_THREAD_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${EMSCRIPTEN_THREAD_FLAGS}")
    add_compile_definitions(PHOENIX_WASM_THREADS=1)
endif()

# IDBFS (IndexedDB File System) for async file storage
option(ENABLE_IDBFS "Enable IndexedDB file system" ON)
if(ENABLE_IDBFS)
    list(APPEND CMAKE_EXE_LINKER_FLAGS "-s FORCE_FILESYSTEM=1")
    list(APPEND CMAKE_EXE_LINKER_FLAGS "-s EXPORTED_RUNTIME_METHODS=['FS','IDBFS']")
    add_compile_definitions(PHOENIX_WASM_IDBFS=1)
endif()

# Output file settings
set(CMAKE_EXECUTABLE_SUFFIX ".js")
set(CMAKE_C_OUTPUT_EXTENSION ".o")
set(CMAKE_CXX_OUTPUT_EXTENSION ".o")

# Find Emscripten sysroot
execute_process(
    COMMAND em-config EMSCRIPTEN
    OUTPUT_VARIABLE EMSCRIPTEN_SYSROOT
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(CMAKE_FIND_ROOT_PATH "${EMSCRIPTEN_SYSROOT}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Platform definition
set(UNIX ON)
set(WASM ON)
set(EMSCRIPTEN ON)

message(STATUS "Phoenix Engine WASM Build Configuration:")
message(STATUS "  Emscripten Sysroot: ${EMSCRIPTEN_SYSROOT}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Threading: ${ENABLE_WASM_THREADS}")
message(STATUS "  IDBFS: ${ENABLE_IDBFS}")
