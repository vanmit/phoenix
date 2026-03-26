cmake_minimum_required(VERSION 3.16)
project(phoenix_performance_benchmarks CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Phoenix Engine 根目录
set(PHOENIX_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")

# 查找性能分析库
find_package(nlohmann_json QUIET)

# 性能基准可执行文件
add_executable(benchmark_fps
  benchmark_fps.cpp
)

add_executable(benchmark_memory
  benchmark_memory.cpp
)

add_executable(benchmark_loading
  benchmark_loading.cpp
)

add_executable(benchmark_gpu_cpu
  benchmark_gpu_cpu.cpp
)

# 包含目录
target_include_directories(benchmark_fps PRIVATE
  ${PHOENIX_ROOT}/include
  ${PHOENIX_ROOT}/src
)

target_include_directories(benchmark_memory PRIVATE
  ${PHOENIX_ROOT}/include
  ${PHOENIX_ROOT}/src
)

target_include_directories(benchmark_loading PRIVATE
  ${PHOENIX_ROOT}/include
  ${PHOENIX_ROOT}/src
)

target_include_directories(benchmark_gpu_cpu PRIVATE
  ${PHOENIX_ROOT}/include
  ${PHOENIX_ROOT}/src
)

# 链接库
target_link_libraries(benchmark_fps PRIVATE phoenix_core phoenix_render)
target_link_libraries(benchmark_memory PRIVATE phoenix_core phoenix_render)
target_link_libraries(benchmark_loading PRIVATE phoenix_core phoenix_resource)
target_link_libraries(benchmark_gpu_cpu PRIVATE phoenix_core phoenix_render)

# JSON 支持
if(nlohmann_json_FOUND)
  target_link_libraries(benchmark_fps PRIVATE nlohmann_json::nlohmann_json)
  target_link_libraries(benchmark_memory PRIVATE nlohmann_json::nlohmann_json)
  target_link_libraries(benchmark_loading PRIVATE nlohmann_json::nlohmann_json)
  target_link_libraries(benchmark_gpu_cpu PRIVATE nlohmann_json::nlohmann_json)
endif()

# 安装目标
install(TARGETS benchmark_fps benchmark_memory benchmark_loading benchmark_gpu_cpu
  DESTINATION bin
)
