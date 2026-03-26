# Emscripten 编译配置 for Phoenix Engine WebGPU/WebGL 后端
# 使用方法：emcmake cmake .. && cmake --build .

cmake_minimum_required(VERSION 3.20)

# Emscripten 工具链检测
if(NOT EMSCRIPTEN)
    message(FATAL_ERROR "This toolchain file must be used with Emscripten (emcmake cmake)")
endif()

project(PhoenixEngineWeb 
    VERSION 1.0.0
    DESCRIPTION "Phoenix Engine - Web Platform (WebGPU/WebGL)"
    LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================================
# Emscripten 编译选项
# ============================================================================

set(EMSCRIPTEN_FLAGS "--bind -s WASM=1 -s ALLOW_MEMORY_GROWTH=1")
set(EMSCRIPTEN_FLAGS "${EMSCRIPTEN_FLAGS} -s MAX_WEBGL_VERSION=2")
set(EMSCRIPTEN_FLAGS "${EMSCRIPTEN_FLAGS} -s MIN_WEBGL_VERSION=2")
set(EMSCRIPTEN_FLAGS "${EMSCRIPTEN_FLAGS} -s USE_WEBGPU=1")

# 优化选项
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(EMSCRIPTEN_FLAGS "${EMSCRIPTEN_FLAGS} -O3")
    set(EMSCRIPTEN_FLAGS "${EMSCRIPTEN_FLAGS} -s LTO=1")
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(EMSCRIPTEN_FLAGS "${EMSCRIPTEN_FLAGS} -g4")
    set(EMSCRIPTEN_FLAGS "${EMSCRIPTEN_FLAGS} -s ASSERTIONS=1")
    set(EMSCRIPTEN_FLAGS "${EMSCRIPTEN_FLAGS} -s SAFE_HEAP=1")
endif()

# 内存配置
set(EMSCRIPTEN_FLAGS "${EMSCRIPTEN_FLAGS} -s INITIAL_MEMORY=268435456")  # 256MB
set(EMSCRIPTEN_FLAGS "${EMSCRIPTEN_FLAGS} -s MAXIMUM_MEMORY=536870912")  # 512MB

# 线程支持 (可选)
# set(EMSCRIPTEN_FLAGS "${EMSCRIPTEN_FLAGS} -s USE_PTHREADS=1")
# set(EMSCRIPTEN_FLAGS "${EMSCRIPTEN_FLAGS} -s PTHREAD_POOL_SIZE=4")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EMSCRIPTEN_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EMSCRIPTEN_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${EMSCRIPTEN_FLAGS}")

# ============================================================================
# 编译定义
# ============================================================================

add_compile_definitions(
    PHOENIX_PLATFORM_WEB=1
    PHOENIX_WEBGPU_ENABLED=1
    PHOENIX_WEBGL_ENABLED=1
    __EMSCRIPTEN__
)

# ============================================================================
# 包含目录
# ============================================================================

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/platform/webgpu
)

# ============================================================================
# Phoenix Engine Web 库
# ============================================================================

add_library(phoenix_web STATIC
    # WebGPU 后端
    src/platform/webgpu/webgpu_device.cpp
    
    # WebGL 后备
    src/platform/webgpu/webgl_fallback.cpp
    
    # 平台抽象层
    src/platform/common/platform_common.cpp
    src/platform/common/window_web.cpp
    src/platform/common/input_web.cpp
    src/platform/common/timer_web.cpp
)

target_include_directories(phoenix_web PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# ============================================================================
# 示例程序
# ============================================================================

add_executable(cross-platform-demo
    examples/cross-platform-demo/main.cpp
)

target_link_libraries(cross-platform-demo PRIVATE
    phoenix_web
)

# 生成 HTML 包装器
set_target_properties(cross-platform-demo PROPERTIES
    SUFFIX ".html"
    LINK_FLAGS "--shell-file ${CMAKE_SOURCE_DIR}/examples/cross-platform-demo/shell.html"
)

# ============================================================================
# 资源文件
# ============================================================================

# 复制资源到构建目录
file(COPY ${CMAKE_SOURCE_DIR}/examples/cross-platform-demo/resources
     DESTINATION ${CMAKE_BINARY_DIR}/resources)

# ============================================================================
# 安装
# ============================================================================

install(TARGETS phoenix_web cross-platform-demo
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/phoenix
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp"
)

# ============================================================================
# 构建摘要
# ============================================================================

message(STATUS "")
message(STATUS "Phoenix Engine Web Configuration:")
message(STATUS "  Emscripten: ${EMSCRIPTEN}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  WASM: Yes")
message(STATUS "  WebGPU: Enabled")
message(STATUS "  WebGL 2: Enabled (fallback)")
message(STATUS "  Initial Memory: 256MB")
message(STATUS "  Max Memory: 512MB")
message(STATUS "")
