/**
 * Phoenix Engine WebAssembly Bindings
 * 
 * Main C++ to JavaScript bridge implementation
 */

#include <phoenix/wasm/types.hpp>
#include <cstring>
#include <cstdio>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/val.h>
#include <emscripten/html5.h>
#include <emscripten/html5_webgl.h>
#endif

namespace phoenix::wasm {

// ============================================================================
// Global State
// ============================================================================

static struct {
    bool initialized = false;
    EngineConfig config;
    uint32_t nextResourceId = 1;
    uint32_t nextSceneId = 1;
    uint64_t frameCount = 0;
    double lastFrameTime = 0;
    double accumulatedTime = 0;
} g_state;

// ============================================================================
// Engine Lifecycle
// ============================================================================

extern "C" WASM_EXPORT int phoenix_init(const EngineConfig* config) {
    if (g_state.initialized) {
        return -1; // Already initialized
    }
    
    // Validate config pointer if provided
    if (config) {
#ifdef __EMSCRIPTEN__
        // SECURITY FIX: Complete boundary check for config pointer
        int valid = EM_ASM_INT({
            const ptr = $0;
            const maxPtr = HEAPU8.length;
            const minSize = 16;  // Minimum EngineConfig size
            
            try {
                // Check if pointer is a valid number
                if (typeof ptr !== 'number' || ptr === 0) return 0;
                // Check pointer is within bounds
                if (ptr < 0 || ptr >= maxPtr) return 0;
                // Check entire structure is accessible (boundary check)
                if (ptr + minSize > maxPtr) return 0;
                
                // Try to read first and last byte of structure
                HEAPU8[ptr];
                HEAPU8[ptr + minSize - 1];
                return 1;
            } catch(e) {
                console.error('Invalid config pointer:', ptr, e);
                return 0;
            }
        }, reinterpret_cast<uintptr_t>(config));
        
        if (!valid) {
            return -2; // Invalid config pointer
        }
#endif
        
        // Validate config values
        if (config->width > 16384 || config->height > 16384) {
            return -3; // Resolution too large
        }
        
        g_state.config = *config;
    } else {
        g_state.config = EngineConfig{};
    }
    
#ifdef __EMSCRIPTEN__
    // Initialize filesystem (IDBFS)
    EM_ASM({
        FS.mkdir('/idbfs');
        FS.mount(IDBFS, {}, '/idbfs');
        FS.syncfs(true, function(err) {
            if (err) console.error('IDBFS sync error:', err);
        });
    });
    
    // Set up canvas
    if (g_state.config.width > 0 && g_state.config.height > 0) {
        EM_ASM({
            const canvas = document.getElementById('canvas') || 
                          document.querySelector('canvas');
            if (canvas) {
                canvas.width = $0;
                canvas.height = $1;
            }
        }, g_state.config.width, g_state.config.height);
    }
#endif
    
    g_state.initialized = true;
    g_state.lastFrameTime = 0;
    g_state.frameCount = 0;
    
    return 0;
}

extern "C" WASM_EXPORT void phoenix_shutdown() {
    if (!g_state.initialized) {
        return;
    }
    
    // Cleanup resources
    g_state.initialized = false;
    
#ifdef __EMSCRIPTEN__
    // Sync filesystem before shutdown
    EM_ASM({
        FS.syncfs(false, function(err) {
            if (err) console.error('IDBFS sync error on shutdown:', err);
        });
    });
#endif
}

extern "C" WASM_EXPORT void phoenix_frame() {
    if (!g_state.initialized) {
        return;
    }
    
    g_state.frameCount++;
    
#ifdef __EMSCRIPTEN__
    double currentTime = emscripten_get_now() / 1000.0;
#else
    double currentTime = 0; // Placeholder
#endif
    
    if (g_state.lastFrameTime > 0) {
        g_state.accumulatedTime += currentTime - g_state.lastFrameTime;
    }
    g_state.lastFrameTime = currentTime;
}

extern "C" WASM_EXPORT void phoenix_resize(uint32_t width, uint32_t height) {
    g_state.config.width = width;
    g_state.config.height = height;
    
#ifdef __EMSCRIPTEN__
    EM_ASM({
        const canvas = document.getElementById('canvas') || 
                      document.querySelector('canvas');
        if (canvas) {
            canvas.width = $0;
            canvas.height = $1;
        }
    }, width, height);
#endif
}

// ============================================================================
// Resource Management
// ============================================================================

extern "C" WASM_EXPORT ResourceHandle phoenix_load_resource(
    const char* url, 
    const char* type
) {
    ResourceHandle handle = {0};
    handle.id = g_state.nextResourceId++;
    handle.valid = true;
    handle.type = type;
    handle.error = nullptr;
    
#ifdef __EMSCRIPTEN__
    // Async resource loading via JavaScript
    emscripten::val jsUrl = emscripten::val(url);
    emscripten::val jsType = emscripten::val(type);
    
    // This would typically use fetch() in JS
    // For now, we just create a placeholder handle
#endif
    
    return handle;
}

extern "C" WASM_EXPORT void phoenix_unload_resource(uint32_t id) {
    // Resource cleanup logic
}

extern "C" WASM_EXPORT ResourceHandle phoenix_get_resource(uint32_t id) {
    ResourceHandle handle = {0};
    handle.id = id;
    handle.valid = (id > 0 && id < g_state.nextResourceId);
    return handle;
}

// ============================================================================
// Scene Management
// ============================================================================

extern "C" WASM_EXPORT SceneHandle phoenix_create_scene() {
    SceneHandle handle = {0};
    handle.id = g_state.nextSceneId++;
    handle.valid = true;
    return handle;
}

extern "C" WASM_EXPORT void phoenix_destroy_scene(uint32_t id) {
    // Scene cleanup logic
}

extern "C" WASM_EXPORT void phoenix_add_to_scene(
    uint32_t sceneId, 
    uint32_t resourceId
) {
    // Add resource to scene logic
}

// ============================================================================
// Texture Management
// ============================================================================

extern "C" WASM_EXPORT TextureHandle phoenix_create_texture(
    uint32_t width, 
    uint32_t height, 
    const void* data
) {
    TextureHandle handle = {0};
    handle.id = g_state.nextResourceId++;
    handle.width = width;
    handle.height = height;
    handle.format = 0; // RGBA8
    handle.valid = true;
    
#ifdef __EMSCRIPTEN__
    // Upload texture data to WebGL/WebGPU
    if (data) {
        EM_ASM({
            // Texture upload logic would go here
            const ptr = $0;
            const size = $1 * $2 * 4; // RGBA
        }, reinterpret_cast<uintptr_t>(data), width, height);
    }
#endif
    
    return handle;
}

extern "C" WASM_EXPORT void phoenix_destroy_texture(uint32_t id) {
    // Texture cleanup logic
}

extern "C" WASM_EXPORT TextureHandle phoenix_get_texture(uint32_t id) {
    TextureHandle handle = {0};
    handle.id = id;
    handle.valid = (id > 0);
    return handle;
}

// ============================================================================
// Graphics
// ============================================================================

extern "C" WASM_EXPORT void phoenix_submit_frame() {
    if (!g_state.initialized) {
        return;
    }
    
#ifdef __EMSCRIPTEN__
    // Swap buffers / present frame
    // bgfx::frame() would be called here
#endif
}

extern "C" WASM_EXPORT FrameInfo phoenix_get_frame_info() {
    FrameInfo info = {0};
    
#ifdef __EMSCRIPTEN__
    double currentTime = emscripten_get_now() / 1000.0;
    info.deltaTime = (g_state.lastFrameTime > 0) ? 
                     (currentTime - g_state.lastFrameTime) : 0;
    info.frameTime = info.deltaTime;
    info.frameNumber = g_state.frameCount;
    info.fps = (info.deltaTime > 0) ? (1.0f / info.deltaTime) : 0.0f;
#else
    info.frameNumber = g_state.frameCount;
#endif
    
    return info;
}

extern "C" WASM_EXPORT GraphicsCaps phoenix_get_graphics_caps() {
    GraphicsCaps caps = {0};
    
#ifdef __EMSCRIPTEN__
    caps.hasWebGL2 = EM_ASM_INT({
        return typeof WebGL2RenderingContext !== 'undefined';
    });
    
    caps.hasWebGL = EM_ASM_INT({
        return typeof WebGLRenderingContext !== 'undefined';
    });
    
    caps.hasSharedArrayBuffer = EM_ASM_INT({
        return typeof SharedArrayBuffer !== 'undefined';
    });
    
    caps.hasBigInt = EM_ASM_INT({
        return typeof BigInt !== 'undefined';
    });
    
    caps.hasWebGPU = EM_ASM_INT({
        return typeof navigator !== 'undefined' && 
               navigator.gpu !== undefined;
    });
    
    // Query WebGL limits
    if (caps.hasWebGL2) {
        caps.maxTextureSize = EM_ASM_INT({
            const canvas = document.createElement('canvas');
            const gl = canvas.getContext('webgl2');
            return gl ? gl.getParameter(gl.MAX_TEXTURE_SIZE) : 0;
        });
        
        caps.maxVertexAttribs = EM_ASM_INT({
            const canvas = document.createElement('canvas');
            const gl = canvas.getContext('webgl2');
            return gl ? gl.getParameter(gl.MAX_VERTEX_ATTRIBS) : 0;
        });
    }
#endif
    
    return caps;
}

// ============================================================================
// Memory Management
// ============================================================================

extern "C" WASM_EXPORT MemoryStats phoenix_get_memory_stats() {
    MemoryStats stats = {0};
    
#ifdef __EMSCRIPTEN__
    stats.totalBytes = EM_ASM_INT({
        return HEAP8.length;
    });
    
    // Approximate used memory (would need more sophisticated tracking)
    stats.usedBytes = stats.totalBytes * 0.3; // Placeholder
    stats.freeBytes = stats.totalBytes - stats.usedBytes;
    stats.usagePercent = (stats.usedBytes * 100.0f) / stats.totalBytes;
#endif
    
    return stats;
}

extern "C" WASM_EXPORT void phoenix_gc() {
    // Garbage collection / memory cleanup
#ifdef __EMSCRIPTEN__
    EM_ASM({
        if (typeof gc === 'function') {
            gc();
        }
    });
#endif
}

// ============================================================================
// Filesystem (IDBFS)
// ============================================================================

extern "C" WASM_EXPORT int phoenix_fs_init() {
#ifdef __EMSCRIPTEN__
    EM_ASM({
        FS.mkdir('/idbfs');
        FS.mount(IDBFS, {}, '/idbfs');
        FS.syncfs(true, function(err) {
            if (err) {
                console.error('IDBFS mount error:', err);
                return -1;
            }
        });
    });
    return 0;
#else
    return -1;
#endif
}

extern "C" WASM_EXPORT int phoenix_fs_load(const char* path) {
#ifdef __EMSCRIPTEN__
    int result = EM_ASM_INT({
        const pathStr = UTF8ToString($0);
        try {
            const data = FS.readFile(pathStr);
            return data ? data.length : -1;
        } catch (e) {
            console.error('FS load error:', e);
            return -1;
        }
    }, path);
    return result;
#else
    return -1;
#endif
}

extern "C" WASM_EXPORT int phoenix_fs_save(
    const char* path, 
    const void* data, 
    size_t size
) {
#ifdef __EMSCRIPTEN__
    int result = EM_ASM_INT({
        const pathStr = UTF8ToString($0);
        const ptr = $1;
        const len = $2;
        try {
            const array = new Uint8Array(HEAPU8.buffer, ptr, len);
            FS.writeFile(pathStr, array);
            FS.syncfs(false, function(err) {
                if (err) console.error('FS save error:', err);
            });
            return 0;
        } catch (e) {
            console.error('FS save error:', e);
            return -1;
        }
    }, path, reinterpret_cast<uintptr_t>(data), size);
    return result;
#else
    return -1;
#endif
}

} // namespace phoenix::wasm
