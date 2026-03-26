#pragma once

#include <cstdint>
#include <cstddef>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/val.h>
#endif

namespace phoenix::wasm {

// ============================================================================
// WASM Module Types
// ============================================================================

/**
 * Engine initialization configuration
 */
struct EngineConfig {
    uint32_t width = 1024;
    uint32_t height = 768;
    bool enableWebGPU = true;
    bool enableWebGL2 = true;
    bool enableAsyncify = true;
    uint32_t initialMemoryMB = 128;
    uint32_t maxMemoryMB = 2048;
    bool enableThreading = false;
    uint32_t threadPoolSize = 4;
};

/**
 * Resource loading result
 */
struct ResourceHandle {
    uint32_t id;
    bool valid;
    const char* type;
    const char* error;
};

/**
 * Scene handle
 */
struct SceneHandle {
    uint32_t id;
    bool valid;
};

/**
 * Texture handle
 */
struct TextureHandle {
    uint32_t id;
    uint32_t width;
    uint32_t height;
    uint32_t format;
    bool valid;
};

/**
 * Frame timing information
 */
struct FrameInfo {
    double deltaTime;
    double frameTime;
    uint64_t frameNumber;
    float fps;
};

/**
 * Graphics capabilities
 */
struct GraphicsCaps {
    bool hasWebGPU;
    bool hasWebGL2;
    bool hasWebGL;
    bool hasSharedArrayBuffer;
    bool hasBigInt;
    bool hasAsyncify;
    
    // Extensions
    bool hasEXT_color_buffer_float;
    bool hasEXT_color_buffer_half_float;
    bool hasOES_texture_float_linear;
    bool hasWEBGL_multi_draw;
    bool hasKHR_parallel_shader_compile;
    
    // Limits
    uint32_t maxTextureSize;
    uint32_t maxCubeMapTextureSize;
    uint32_t maxVertexAttribs;
    uint32_t maxFragmentUniformVectors;
    uint32_t maxVertexUniformVectors;
    uint32_t maxDrawBuffers;
    
    // Performance
    bool hasVertexArrayObject;
    bool hasInstancedArrays;
    bool hasDrawBuffers;
    bool hasTransformFeedback;
};

/**
 * Memory statistics
 */
struct MemoryStats {
    uint64_t totalBytes;
    uint64_t usedBytes;
    uint64_t freeBytes;
    float usagePercent;
};

// ============================================================================
// Callback Types
// ============================================================================

#ifdef __EMSCRIPTEN__
using ProgressCallback = emscripten::val;
using LoadCompleteCallback = emscripten::val;
using ErrorCallback = emscripten::val;
using FrameCallback = emscripten::val;
#else
// Native fallback types
using ProgressCallback = void*;
using LoadCompleteCallback = void*;
using ErrorCallback = void*;
using FrameCallback = void*;
#endif

// ============================================================================
// Export Macros
// ============================================================================

#ifdef __EMSCRIPTEN__
#define WASM_EXPORT EMSCRIPTEN_KEEPALIVE
#define WASM_EXPORT_JS(name) EMSCRIPTEN_NAME(name)
#else
#define WASM_EXPORT
#define WASM_EXPORT_JS(name)
#endif

// ============================================================================
// C Interface (for JS interop)
// ============================================================================

extern "C" {

// Engine lifecycle
WASM_EXPORT int phoenix_init(const EngineConfig* config);
WASM_EXPORT void phoenix_shutdown();
WASM_EXPORT void phoenix_frame();
WASM_EXPORT void phoenix_resize(uint32_t width, uint32_t height);

// Resource management
WASM_EXPORT ResourceHandle phoenix_load_resource(const char* url, const char* type);
WASM_EXPORT void phoenix_unload_resource(uint32_t id);
WASM_EXPORT ResourceHandle phoenix_get_resource(uint32_t id);

// Scene management
WASM_EXPORT SceneHandle phoenix_create_scene();
WASM_EXPORT void phoenix_destroy_scene(uint32_t id);
WASM_EXPORT void phoenix_add_to_scene(uint32_t sceneId, uint32_t resourceId);

// Texture management
WASM_EXPORT TextureHandle phoenix_create_texture(uint32_t width, uint32_t height, const void* data);
WASM_EXPORT void phoenix_destroy_texture(uint32_t id);
WASM_EXPORT TextureHandle phoenix_get_texture(uint32_t id);

// Graphics
WASM_EXPORT void phoenix_submit_frame();
WASM_EXPORT FrameInfo phoenix_get_frame_info();
WASM_EXPORT GraphicsCaps phoenix_get_graphics_caps();

// Memory
WASM_EXPORT MemoryStats phoenix_get_memory_stats();
WASM_EXPORT void phoenix_gc();

// Filesystem
WASM_EXPORT int phoenix_fs_init();
WASM_EXPORT int phoenix_fs_load(const char* path);
WASM_EXPORT int phoenix_fs_save(const char* path, const void* data, size_t size);

} // extern "C"

} // namespace phoenix::wasm
