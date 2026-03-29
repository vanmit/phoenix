# Phoenix Engine Test Configuration
# Requires: CMake 3.14+, C++17, Google Test

cmake_minimum_required(VERSION 3.14)
project(phoenix_engine_tests LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type (default to Release for tests)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
elseif(MSVC)
    add_compile_options(/W4)
endif()

# Find Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# Find Phoenix Engine source (assuming parent directory structure)
set(PHOENIX_ENGINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")

# Code coverage configuration (GCC/Clang only)
option(ENABLE_COVERAGE "Enable code coverage" OFF)
if(ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)
endif()

# ============================================================================
# Test Executables
# ============================================================================

# Test main entry point
add_executable(test_main test_main.cpp)
target_link_libraries(test_main PRIVATE GTest::gtest GTest::gtest_main)

# Math library tests
add_executable(test_math test_math.cpp)
target_link_libraries(test_math PRIVATE GTest::gtest GTest::gtest_main)
# If Phoenix Engine has a math library, link it here:
# target_link_libraries(test_math PRIVATE phoenix_math)

# Security module tests
add_executable(test_security test_security.cpp)
target_link_libraries(test_security PRIVATE GTest::gtest GTest::gtest_main)
# If Phoenix Engine has a security library, link it here:
# target_link_libraries(test_security PRIVATE phoenix_security)

# Resource loader tests
add_executable(test_resource_loaders resource/test_resource_loaders.cpp)
target_link_libraries(test_resource_loaders PRIVATE GTest::gtest GTest::gtest_main)
target_include_directories(test_resource_loaders PRIVATE 
    ${PHOENIX_ENGINE_DIR}/include
    ${PHOENIX_ENGINE_DIR}/build/_deps/tinygltf-src
)

# glTF loader specific tests
add_executable(test_gltf_loader resource/test_gltf_loader.cpp)
target_link_libraries(test_gltf_loader PRIVATE GTest::gtest GTest::gtest_main)
target_include_directories(test_gltf_loader PRIVATE 
    ${PHOENIX_ENGINE_DIR}/include
    ${PHOENIX_ENGINE_DIR}/build/_deps/tinygltf-src
)
# Link against the GLTFLoader implementation
target_sources(test_gltf_loader PRIVATE 
    ${PHOENIX_ENGINE_DIR}/src/resource/GLTFLoader.cpp
)

# ============================================================================
# Test Registration with CTest
# ============================================================================

include(GoogleTest)

gtest_discover_tests(test_main
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    TEST_PREFIX "[MAIN] "
    DISCOVERY_TIMEOUT 30
)

gtest_discover_tests(test_math
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    TEST_PREFIX "[MATH] "
    DISCOVERY_TIMEOUT 30
)

gtest_discover_tests(test_security
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    TEST_PREFIX "[SECURITY] "
    DISCOVERY_TIMEOUT 30
)

gtest_discover_tests(test_resource_loaders
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    TEST_PREFIX "[RESOURCE] "
    DISCOVERY_TIMEOUT 30
)

gtest_discover_tests(test_gltf_loader
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    TEST_PREFIX "[GLTF] "
    DISCOVERY_TIMEOUT 30
)

# ============================================================================
# Coverage Target (when coverage is enabled)
# ============================================================================

if(ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    find_program(LCOV_PATH lcov)
    find_program(GENHTML_PATH genhtml)
    
    if(LCOV_PATH AND GENHTML_PATH)
        add_custom_target(coverage
            COMMAND ${LCOV_PATH} --directory . --zerocounters
            COMMAND ${CMAKE_COMMAND} --build . --target test
            COMMAND ${LCOV_PATH} --directory . --capture --output-file coverage.info
            COMMAND ${LCOV_PATH} --remove coverage.info '/usr/*' '*/tests/*' --output-file coverage.info
            COMMAND ${GENHTML_PATH} coverage.info --output-directory coverage_report
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating code coverage report..."
        )
    endif()
endif()

# ============================================================================
# Installation
# ============================================================================

install(TARGETS test_main test_math test_security
    RUNTIME DESTINATION bin/tests
)
