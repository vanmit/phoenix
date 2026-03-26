cmake_minimum_required(VERSION 3.16)
project(phoenix_unit_tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 Phoenix Engine
set(PHOENIX_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")

# GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
)
FetchContent_MakeAvailable(googletest)

# 覆盖率选项
option(ENABLE_COVERAGE "Enable code coverage" ON)

if(ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(--coverage -O0 -g)
  add_link_options(--coverage)
endif()

# 测试可执行文件
add_executable(unit_tests
  test_math_unit.cpp
  test_security_unit.cpp
  test_render_unit.cpp
  test_resource_unit.cpp
)

target_include_directories(unit_tests PRIVATE
  ${PHOENIX_ROOT}/include
  ${PHOENIX_ROOT}/src
)

target_link_libraries(unit_tests PRIVATE
  GTest::gtest_main
  GTest::gmock_main
  phoenix_core
)

# 注册测试
include(GoogleTest)
gtest_discover_tests(unit_tests)

# 覆盖率目标
if(ENABLE_COVERAGE)
  add_custom_target(coverage
    COMMAND ${CMAKE_COMMAND} -E echo "Generating coverage report..."
    COMMAND gcovr -r ${PHOENIX_ROOT} --html-details coverage.html
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Generating HTML coverage report"
  )
  add_dependencies coverage unit_tests
endif()
