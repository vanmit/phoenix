# Phoenix Engine Scene System CMake Configuration

cmake_minimum_required(VERSION 3.16)

# =============================================================================
# Scene Library
# =============================================================================

add_library(phoenix_scene STATIC
    # Core
    scene_node.cpp
    scene.cpp
    
    # Spatial
    octree.cpp
    bvh.cpp
    
    # LOD
    lod.cpp
    
    # Serialization
    scene_serializer.cpp
    
    # ECS
    ecs.cpp
    
    # Animation System (Phase 3)
    skeleton.cpp
    animator.cpp
    morph_animation.cpp
    gltf_loader.cpp
    
    # Physics Integration (Phase 3)
    physics.cpp
    
    # Particle System (Phase 3)
    particle_system.cpp
)

target_include_directories(phoenix_scene PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

target_link_libraries(phoenix_scene PUBLIC
    phoenix_math
)

target_compile_features(phoenix_scene PUBLIC cxx_std_17)

# Enable optimizations
target_compile_options(phoenix_scene PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-O3 -march=native>
    $<$<CXX_COMPILER_ID:MSVC>:/O2 /arch:AVX2>
)

# =============================================================================
# Tests
# =============================================================================

if(PHOENIX_BUILD_TESTS)
    enable_testing()
    
    # Scene Graph Tests
    add_executable(test_scene_graph ../tests/scene/test_scene_graph.cpp)
    target_link_libraries(test_scene_graph PRIVATE
        phoenix_scene
        GTest::gtest_main
    )
    add_test(NAME SceneGraph COMMAND test_scene_graph)
    
    # Spatial Structure Tests
    add_executable(test_spatial ../tests/scene/test_spatial.cpp)
    target_link_libraries(test_spatial PRIVATE
        phoenix_scene
        GTest::gtest_main
    )
    add_test(NAME Spatial COMMAND test_spatial)
    
    # ECS Tests
    add_executable(test_ecs ../tests/scene/test_ecs.cpp)
    target_link_libraries(test_ecs PRIVATE
        phoenix_scene
        GTest::gtest_main
    )
    add_test(NAME ECS COMMAND test_ecs)
    
    # LOD Tests
    add_executable(test_lod ../tests/scene/test_lod.cpp)
    target_link_libraries(test_lod PRIVATE
        phoenix_scene
        GTest::gtest_main
    )
    add_test(NAME LOD COMMAND test_lod)
endif()

# =============================================================================
# Benchmarks
# =============================================================================

if(PHOENIX_BUILD_BENCHMARKS)
    add_executable(benchmark_scene ../benchmarks/scene/benchmark_scene.cpp)
    target_link_libraries(benchmark_scene PRIVATE
        phoenix_scene
    )
    target_compile_options(benchmark_scene PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-O3 -march=native>
    )
endif()
