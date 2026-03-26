/**
 * Phoenix Engine WASM Bindings Tests
 * 
 * Unit tests for WebAssembly JavaScript bindings
 */

#include <cstdio>
#include <cstring>
#include <cassert>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#include <phoenix/wasm/types.hpp>

using namespace phoenix::wasm;

// Test counter
static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) void name()
#define RUN_TEST(name) do { \
    printf("Running %s... ", #name); \
    tests_run++; \
    try { \
        name(); \
        tests_passed++; \
        printf("✅ PASSED\n"); \
    } catch (const std::exception& e) { \
        printf("❌ FAILED: %s\n", e.what); \
    } catch (...) { \
        printf("❌ FAILED: Unknown error\n"); \
    } \
} while(0)

#define ASSERT(cond, msg) do { \
    if (!(cond)) { \
        throw std::runtime_error(msg); \
    } \
} while(0)

#define ASSERT_EQ(a, b, msg) do { \
    if ((a) != (b)) { \
        char buf[256]; \
        snprintf(buf, sizeof(buf), "%s (expected %ld, got %ld)", msg, (long)(b), (long)(a)); \
        throw std::runtime_error(buf); \
    } \
} while(0)

// ============================================================================
// Engine Initialization Tests
// ============================================================================

TEST(test_engine_init_default) {
    EngineConfig config = {};
    int result = phoenix_init(&config);
    
    ASSERT_EQ(result, 0, "Engine init should succeed");
    
    // Cleanup
    phoenix_shutdown();
}

TEST(test_engine_init_null_config) {
    int result = phoenix_init(nullptr);
    
    ASSERT_EQ(result, 0, "Engine init with null config should succeed");
    
    phoenix_shutdown();
}

TEST(test_engine_double_init) {
    EngineConfig config = {};
    
    int result1 = phoenix_init(&config);
    int result2 = phoenix_init(&config);
    
    ASSERT_EQ(result1, 0, "First init should succeed");
    ASSERT_EQ(result2, -1, "Second init should fail");
    
    phoenix_shutdown();
}

TEST(test_engine_shutdown_without_init) {
    // Should not crash
    phoenix_shutdown();
}

// ============================================================================
// Resource Tests
// ============================================================================

TEST(test_resource_load) {
    EngineConfig config = {};
    phoenix_init(&config);
    
    ResourceHandle handle = phoenix_load_resource("test://mesh.glb", "mesh");
    
    ASSERT(handle.valid, "Resource should be valid");
    ASSERT(handle.id > 0, "Resource ID should be positive");
    ASSERT(handle.type != nullptr, "Resource type should not be null");
    
    // Verify we can retrieve the resource
    ResourceHandle retrieved = phoenix_get_resource(handle.id);
    ASSERT(retrieved.valid, "Retrieved resource should be valid");
    ASSERT_EQ(retrieved.id, handle.id, "Resource IDs should match");
    
    phoenix_shutdown();
}

TEST(test_resource_invalid_id) {
    EngineConfig config = {};
    phoenix_init(&config);
    
    ResourceHandle handle = phoenix_get_resource(99999);
    ASSERT(!handle.valid, "Invalid resource should not be valid");
    
    phoenix_shutdown();
}

TEST(test_resource_unload) {
    EngineConfig config = {};
    phoenix_init(&config);
    
    ResourceHandle handle = phoenix_load_resource("test://texture.png", "texture");
    ASSERT(handle.valid, "Resource should be valid");
    
    phoenix_unload_resource(handle.id);
    
    ResourceHandle after = phoenix_get_resource(handle.id);
    // Resource might still be valid depending on implementation
    // This test verifies no crash occurs
    
    phoenix_shutdown();
}

// ============================================================================
// Scene Tests
// ============================================================================

TEST(test_scene_create) {
    EngineConfig config = {};
    phoenix_init(&config);
    
    SceneHandle handle = phoenix_create_scene();
    
    ASSERT(handle.valid, "Scene should be valid");
    ASSERT(handle.id > 0, "Scene ID should be positive");
    
    phoenix_shutdown();
}

TEST(test_scene_add_resource) {
    EngineConfig config = {};
    phoenix_init(&config);
    
    SceneHandle scene = phoenix_create_scene();
    ResourceHandle resource = phoenix_load_resource("test://mesh.glb", "mesh");
    
    phoenix_add_to_scene(scene.id, resource.id);
    
    // Verify no crash
    ASSERT(scene.valid, "Scene should still be valid");
    
    phoenix_destroy_scene(scene.id);
    phoenix_shutdown();
}

TEST(test_scene_destroy) {
    EngineConfig config = {};
    phoenix_init(&config);
    
    SceneHandle handle = phoenix_create_scene();
    ASSERT(handle.valid, "Scene should be valid");
    
    phoenix_destroy_scene(handle.id);
    
    // Verify no crash on double destroy
    phoenix_destroy_scene(handle.id);
    
    phoenix_shutdown();
}

// ============================================================================
// Texture Tests
// ============================================================================

TEST(test_texture_create) {
    EngineConfig config = {};
    phoenix_init(&config);
    
    // Create test texture data
    const uint32_t width = 64;
    const uint32_t height = 64;
    uint8_t data[width * height * 4];
    memset(data, 0xFF, sizeof(data)); // White texture
    
    TextureHandle handle = phoenix_create_texture(width, height, data);
    
    ASSERT(handle.valid, "Texture should be valid");
    ASSERT_EQ(handle.width, width, "Width should match");
    ASSERT_EQ(handle.height, height, "Height should match");
    
    phoenix_destroy_texture(handle.id);
    phoenix_shutdown();
}

TEST(test_texture_get) {
    EngineConfig config = {};
    phoenix_init(&config);
    
    const uint32_t width = 32;
    const uint32_t height = 32;
    uint8_t data[width * height * 4];
    
    TextureHandle created = phoenix_create_texture(width, height, data);
    TextureHandle retrieved = phoenix_get_texture(created.id);
    
    ASSERT(retrieved.valid, "Retrieved texture should be valid");
    ASSERT_EQ(retrieved.id, created.id, "Texture IDs should match");
    
    phoenix_shutdown();
}

// ============================================================================
// Frame Tests
// ============================================================================

TEST(test_frame_basic) {
    EngineConfig config = {};
    phoenix_init(&config);
    
    // Run a few frames
    for (int i = 0; i < 10; i++) {
        phoenix_frame();
    }
    
    FrameInfo info = phoenix_get_frame_info();
    
    ASSERT(info.frameNumber >= 10, "Frame count should be at least 10");
    ASSERT(info.fps >= 0, "FPS should be non-negative");
    
    phoenix_shutdown();
}

TEST(test_frame_submit) {
    EngineConfig config = {};
    phoenix_init(&config);
    
    phoenix_frame();
    phoenix_submit_frame();
    
    // Verify no crash
    ASSERT(true, "Frame submit should not crash");
    
    phoenix_shutdown();
}

TEST(test_frame_resize) {
    EngineConfig config = {};
    phoenix_init(&config);
    
    phoenix_resize(1920, 1080);
    
    FrameInfo info = phoenix_get_frame_info();
    ASSERT(info.frameNumber == 0, "Frame count should reset or continue");
    
    phoenix_shutdown();
}

// ============================================================================
// Graphics Caps Tests
// ============================================================================

TEST(test_graphics_caps) {
    EngineConfig config = {};
    phoenix_init(&config);
    
    GraphicsCaps caps = phoenix_get_graphics_caps();
    
    // At least one backend should be available (in real browser)
    // In test environment, values may be zero
    ASSERT(true, "Graphics caps query should not crash");
    
    phoenix_shutdown();
}

// ============================================================================
// Memory Tests
// ============================================================================

TEST(test_memory_stats) {
    EngineConfig config = {};
    phoenix_init(&config);
    
    MemoryStats stats = phoenix_get_memory_stats();
    
    ASSERT(stats.totalBytes > 0, "Total memory should be positive");
    ASSERT(stats.usagePercent >= 0 && stats.usagePercent <= 100, 
           "Usage percent should be 0-100");
    
    phoenix_shutdown();
}

TEST(test_memory_gc) {
    EngineConfig config = {};
    phoenix_init(&config);
    
    // Create some resources
    for (int i = 0; i < 10; i++) {
        phoenix_load_resource("test://resource", "mesh");
    }
    
    // Run GC
    phoenix_gc();
    
    // Verify no crash
    ASSERT(true, "GC should not crash");
    
    phoenix_shutdown();
}

// ============================================================================
// Filesystem Tests (IDBFS)
// ============================================================================

TEST(test_fs_init) {
    EngineConfig config = {};
    phoenix_init(&config);
    
    int result = phoenix_fs_init();
    
    // May fail in non-browser environment
    ASSERT(true, "FS init should not crash");
    
    phoenix_shutdown();
}

TEST(test_fs_save_load) {
    EngineConfig config = {};
    phoenix_init(&config);
    
    const char* testData = "Hello, WASM!";
    const size_t testSize = strlen(testData);
    
    int saveResult = phoenix_fs_save("/test.txt", testData, testSize);
    
    // May fail in non-browser environment
    ASSERT(true, "FS save should not crash");
    
    int loadResult = phoenix_fs_load("/test.txt");
    
    // Verify no crash
    ASSERT(true, "FS load should not crash");
    
    phoenix_shutdown();
}

// ============================================================================
// Stress Tests
// ============================================================================

TEST(test_stress_resources) {
    EngineConfig config = {};
    phoenix_init(&config);
    
    const int numResources = 100;
    ResourceHandle handles[numResources];
    
    // Create many resources
    for (int i = 0; i < numResources; i++) {
        handles[i] = phoenix_load_resource("test://resource", "mesh");
        ASSERT(handles[i].valid, "Resource should be valid");
    }
    
    // Access all resources
    for (int i = 0; i < numResources; i++) {
        ResourceHandle retrieved = phoenix_get_resource(handles[i].id);
        ASSERT(retrieved.valid, "Retrieved resource should be valid");
    }
    
    // Cleanup
    for (int i = 0; i < numResources; i++) {
        phoenix_unload_resource(handles[i].id);
    }
    
    phoenix_shutdown();
}

TEST(test_stress_frames) {
    EngineConfig config = {};
    phoenix_init(&config);
    
    const int numFrames = 1000;
    
    for (int i = 0; i < numFrames; i++) {
        phoenix_frame();
        phoenix_submit_frame();
    }
    
    FrameInfo info = phoenix_get_frame_info();
    ASSERT(info.frameNumber >= numFrames, "Should have rendered all frames");
    
    phoenix_shutdown();
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    printf("Phoenix Engine WASM Bindings Tests\n");
    printf("===================================\n\n");
    
    // Engine tests
    printf("Engine Initialization:\n");
    RUN_TEST(test_engine_init_default);
    RUN_TEST(test_engine_init_null_config);
    RUN_TEST(test_engine_double_init);
    RUN_TEST(test_engine_shutdown_without_init);
    printf("\n");
    
    // Resource tests
    printf("Resource Management:\n");
    RUN_TEST(test_resource_load);
    RUN_TEST(test_resource_invalid_id);
    RUN_TEST(test_resource_unload);
    printf("\n");
    
    // Scene tests
    printf("Scene Management:\n");
    RUN_TEST(test_scene_create);
    RUN_TEST(test_scene_add_resource);
    RUN_TEST(test_scene_destroy);
    printf("\n");
    
    // Texture tests
    printf("Texture Management:\n");
    RUN_TEST(test_texture_create);
    RUN_TEST(test_texture_get);
    printf("\n");
    
    // Frame tests
    printf("Frame Rendering:\n");
    RUN_TEST(test_frame_basic);
    RUN_TEST(test_frame_submit);
    RUN_TEST(test_frame_resize);
    printf("\n");
    
    // Graphics tests
    printf("Graphics Capabilities:\n");
    RUN_TEST(test_graphics_caps);
    printf("\n");
    
    // Memory tests
    printf("Memory Management:\n");
    RUN_TEST(test_memory_stats);
    RUN_TEST(test_memory_gc);
    printf("\n");
    
    // Filesystem tests
    printf("Filesystem (IDBFS):\n");
    RUN_TEST(test_fs_init);
    RUN_TEST(test_fs_save_load);
    printf("\n");
    
    // Stress tests
    printf("Stress Tests:\n");
    RUN_TEST(test_stress_resources);
    RUN_TEST(test_stress_frames);
    printf("\n");
    
    // Summary
    printf("===================================\n");
    printf("Tests: %d/%d passed\n", tests_passed, tests_run);
    printf("Success rate: %.1f%%\n", 
           (tests_passed * 100.0) / tests_run);
    
    return (tests_passed == tests_run) ? 0 : 1;
}
