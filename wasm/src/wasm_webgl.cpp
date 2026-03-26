/**
 * Phoenix Engine WebGL 2.0 Backend
 * 
 * bgfx WebGL2 integration with extension detection and optimizations
 */

#include <phoenix/wasm/types.hpp>
#include <cstdio>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5_webgl.h>
#endif

namespace phoenix::wasm {

// ============================================================================
// WebGL2 Backend State
// ============================================================================

struct WebGL2State {
    bool initialized = false;
    uint32_t glContext = 0;
    
    // Extensions
    bool hasEXT_color_buffer_float = false;
    bool hasEXT_color_buffer_half_float = false;
    bool hasOES_texture_float_linear = false;
    bool hasWEBGL_multi_draw = false;
    bool hasKHR_parallel_shader_compile = false;
    bool hasEXT_disjoint_timer_query = false;
    bool hasWEBGL_compressed_texture_s3tc = false;
    bool hasWEBGL_compressed_texture_etc1 = false;
    bool hasWEBGL_compressed_texture_astc = false;
    
    // Limits
    uint32_t maxTextureSize = 0;
    uint32_t maxCubeMapSize = 0;
    uint32_t maxVertexAttribs = 0;
    uint32_t maxFragmentUniforms = 0;
    uint32_t maxVertexUniforms = 0;
    uint32_t maxDrawBuffers = 0;
    uint32_t maxSamples = 0;
    
    // Performance features
    bool hasVertexArrayObject = false;
    bool hasInstancedArrays = false;
    bool hasTransformFeedback = false;
    
    // Statistics
    uint32_t drawCalls = 0;
    uint32_t triangles = 0;
    uint32_t vertices = 0;
};

static WebGL2State g_webgl2_state;

// ============================================================================
// Extension Detection
// ============================================================================

#ifdef __EMSCRIPTEN__
static void detect_extensions() {
    EM_ASM({
        const gl = GLctx;
        if (!gl) return;
        
        // Check for extensions
        const exts = {
            colorBufferFloat: gl.getExtension('EXT_color_buffer_float'),
            colorBufferHalfFloat: gl.getExtension('EXT_color_buffer_half_float'),
            textureFloatLinear: gl.getExtension('OES_texture_float_linear'),
            multiDraw: gl.getExtension('WEBGL_multi_draw'),
            parallelShaderCompile: gl.getExtension('KHR_parallel_shader_compile'),
            disjointTimerQuery: gl.getExtension('EXT_disjoint_timer_query_webgl2'),
            compressedS3TC: gl.getExtension('WEBGL_compressed_texture_s3tc'),
            compressedETC1: gl.getExtension('WEBGL_compressed_texture_etc1'),
            compressedASTC: gl.getExtension('WEBGL_compressed_texture_astc')
        };
        
        // Store extension availability
        Module._webgl2_ext_color_buffer_float = exts.colorBufferFloat ? 1 : 0;
        Module._webgl2_ext_color_buffer_half_float = exts.colorBufferHalfFloat ? 1 : 0;
        Module._webgl2_ext_texture_float_linear = exts.textureFloatLinear ? 1 : 0;
        Module._webgl2_ext_multi_draw = exts.multiDraw ? 1 : 0;
        Module._webgl2_ext_parallel_shader_compile = exts.parallelShaderCompile ? 1 : 0;
        Module._webgl2_ext_disjoint_timer_query = exts.disjointTimerQuery ? 1 : 0;
        Module._webgl2_ext_compressed_s3tc = exts.compressedS3TC ? 1 : 0;
        Module._webgl2_ext_compressed_etc1 = exts.compressedETC1 ? 1 : 0;
        Module._webgl2_ext_compressed_astc = exts.compressedASTC ? 1 : 0;
    });
    
    g_webgl2_state.hasEXT_color_buffer_float = EM_ASM_INT({
        return Module._webgl2_ext_color_buffer_float || 0;
    });
    
    g_webgl2_state.hasEXT_color_buffer_half_float = EM_ASM_INT({
        return Module._webgl2_ext_color_buffer_half_float || 0;
    });
    
    g_webgl2_state.hasOES_texture_float_linear = EM_ASM_INT({
        return Module._webgl2_ext_texture_float_linear || 0;
    });
    
    g_webgl2_state.hasWEBGL_multi_draw = EM_ASM_INT({
        return Module._webgl2_ext_multi_draw || 0;
    });
    
    g_webgl2_state.hasKHR_parallel_shader_compile = EM_ASM_INT({
        return Module._webgl2_ext_parallel_shader_compile || 0;
    });
    
    g_webgl2_state.hasEXT_disjoint_timer_query = EM_ASM_INT({
        return Module._webgl2_ext_disjoint_timer_query || 0;
    });
    
    g_webgl2_state.hasWEBGL_compressed_texture_s3tc = EM_ASM_INT({
        return Module._webgl2_ext_compressed_s3tc || 0;
    });
    
    g_webgl2_state.hasWEBGL_compressed_texture_etc1 = EM_ASM_INT({
        return Module._webgl2_ext_compressed_etc1 || 0;
    });
    
    g_webgl2_state.hasWEBGL_compressed_texture_astc = EM_ASM_INT({
        return Module._webgl2_ext_compressed_astc || 0;
    });
}

static void query_limits() {
    g_webgl2_state.maxTextureSize = EM_ASM_INT({
        const gl = GLctx;
        return gl ? gl.getParameter(gl.MAX_TEXTURE_SIZE) : 0;
    });
    
    g_webgl2_state.maxCubeMapSize = EM_ASM_INT({
        const gl = GLctx;
        return gl ? gl.getParameter(gl.MAX_CUBE_MAP_TEXTURE_SIZE) : 0;
    });
    
    g_webgl2_state.maxVertexAttribs = EM_ASM_INT({
        const gl = GLctx;
        return gl ? gl.getParameter(gl.MAX_VERTEX_ATTRIBS) : 0;
    });
    
    g_webgl2_state.maxFragmentUniforms = EM_ASM_INT({
        const gl = GLctx;
        return gl ? gl.getParameter(gl.MAX_FRAGMENT_UNIFORM_VECTORS) : 0;
    });
    
    g_webgl2_state.maxVertexUniforms = EM_ASM_INT({
        const gl = GLctx;
        return gl ? gl.getParameter(gl.MAX_VERTEX_UNIFORM_VECTORS) : 0;
    });
    
    g_webgl2_state.maxDrawBuffers = EM_ASM_INT({
        const gl = GLctx;
        return gl ? gl.getParameter(gl.MAX_DRAW_BUFFERS) : 0;
    });
    
    g_webgl2_state.maxSamples = EM_ASM_INT({
        const gl = GLctx;
        return gl ? gl.getParameter(gl.MAX_SAMPLES) : 0;
    });
}
#endif

// ============================================================================
// Initialization
// ============================================================================

bool webgl2_init() {
    if (g_webgl2_state.initialized) {
        return true;
    }
    
#ifdef __EMSCRIPTEN__
    // Get WebGL2 context
    g_webgl2_state.glContext = emscripten_webgl_get_current_context();
    
    if (!g_webgl2_state.glContext) {
        g_webgl2_state.glContext = emscripten_webgl_create_context(
            "#canvas",
            {
                .majorVersion = 2,
                .minorVersion = 0,
                .alpha = true,
                .depth = true,
                .stencil = false,
                .antialias = true,
                .premultipliedAlpha = true,
                .preserveDrawingBuffer = false,
                .powerPreference = EM_WEBGL_POWER_PREFERENCE_HIGH_PERFORMANCE,
                .failIfMajorPerformanceCaveat = false,
                .enableExtensionsByDefault = true,
                .explicitSwapControl = 0,
                .proxyContextToMainThread = 1
            }
        );
        
        if (!g_webgl2_state.glContext) {
            fprintf(stderr, "Failed to create WebGL2 context\n");
            return false;
        }
        
        emscripten_webgl_make_context_current(g_webgl2_state.glContext);
    }
    
    // Detect extensions
    detect_extensions();
    
    // Query limits
    query_limits();
    
    // Enable features
    EM_ASM({
        const gl = GLctx;
        if (!gl) return;
        
        // Enable VAO extension (core in WebGL2)
        Module._webgl2_has_vao = 1;
        
        // Enable instanced arrays (core in WebGL2)
        Module._webgl2_has_instanced_arrays = 1;
        
        // Enable transform feedback (core in WebGL2)
        Module._webgl2_has_transform_feedback = 1;
    });
    
    g_webgl2_state.hasVertexArrayObject = true;
    g_webgl2_state.hasInstancedArrays = true;
    g_webgl2_state.hasTransformFeedback = true;
#endif
    
    g_webgl2_state.initialized = true;
    
    printf("WebGL2 initialized successfully\n");
    printf("  Max Texture Size: %u\n", g_webgl2_state.maxTextureSize);
    printf("  Max Cube Map Size: %u\n", g_webgl2_state.maxCubeMapSize);
    printf("  Max Vertex Attribs: %u\n", g_webgl2_state.maxVertexAttribs);
    
    return true;
}

void webgl2_shutdown() {
    if (!g_webgl2_state.initialized) {
        return;
    }
    
#ifdef __EMSCRIPTEN__
    if (g_webgl2_state.glContext) {
        emscripten_webgl_destroy_context(g_webgl2_state.glContext);
        g_webgl2_state.glContext = 0;
    }
#endif
    
    g_webgl2_state.initialized = false;
}

// ============================================================================
// Extension Queries
// ============================================================================

bool webgl2_has_extension(const char* name) {
#ifdef __EMSCRIPTEN__
    return EM_ASM_INT({
        const gl = GLctx;
        if (!gl) return 0;
        const extName = UTF8ToString($0);
        return gl.getExtension(extName) ? 1 : 0;
    }, name);
#else
    return false;
#endif
}

WebGL2Extensions webgl2_get_extensions() {
    return {
        .colorBufferFloat = g_webgl2_state.hasEXT_color_buffer_float,
        .colorBufferHalfFloat = g_webgl2_state.hasEXT_color_buffer_half_float,
        .textureFloatLinear = g_webgl2_state.hasOES_texture_float_linear,
        .multiDraw = g_webgl2_state.hasWEBGL_multi_draw,
        .parallelShaderCompile = g_webgl2_state.hasKHR_parallel_shader_compile
    };
}

// ============================================================================
// Performance Optimizations
// ============================================================================

void webgl2_enable_vertex_cache() {
#ifdef __EMSCRIPTEN__
    EM_ASM({
        const gl = GLctx;
        if (!gl) return;
        
        // Enable primitive restart for better vertex cache utilization
        gl.enable(gl.PRIMITIVE_RESTART_FIXED_INDEX);
        
        // Hint for high performance
        gl.pixelStorei(gl.PACK_ALIGNMENT, 1);
        gl.pixelStorei(gl.UNPACK_ALIGNMENT, 1);
    });
#endif
}

void webgl2_enable_batching() {
#ifdef __EMSCRIPTEN__
    EM_ASM({
        const gl = GLctx;
        if (!gl) return;
        
        // Enable multi-draw if available
        if (Module._webgl2_ext_multi_draw) {
            console.log('WEBGL_multi_draw enabled for batching');
        }
    });
#endif
}

void webgl2_optimize_draw_calls(uint32_t count) {
    g_webgl2_state.drawCalls += count;
    
    // Statistics tracking for performance monitoring
#ifdef __EMSCRIPTEN__
    if (g_webgl2_state.drawCalls % 100 == 0) {
        EM_ASM({
            console.log('Draw calls:', $0);
        }, g_webgl2_state.drawCalls);
    }
#endif
}

// ============================================================================
// Resource Management
// ============================================================================

uint32_t webgl2_create_vertex_array() {
#ifdef __EMSCRIPTEN__
    return EM_ASM_INT({
        const gl = GLctx;
        if (!gl) return 0;
        const vao = gl.createVertexArray();
        return vao ? GL.vaoMap.get(vao) : 0;
    });
#else
    return 0;
#endif
}

void webgl2_delete_vertex_array(uint32_t vao) {
#ifdef __EMSCRIPTEN__
    EM_ASM({
        const gl = GLctx;
        if (!gl) return;
        const vao = GL.vaoMap.get($0);
        if (vao) {
            gl.deleteVertexArray(vao);
            GL.vaoMap.delete($0);
        }
    }, vao);
#endif
}

uint32_t webgl2_create_buffer() {
#ifdef __EMSCRIPTEN__
    return EM_ASM_INT({
        const gl = GLctx;
        if (!gl) return 0;
        const buffer = gl.createBuffer();
        return GL.bufferMap.get(buffer);
    });
#else
    return 0;
#endif
}

void webgl2_upload_buffer(
    uint32_t buffer,
    uint32_t target,
    const void* data,
    size_t size,
    uint32_t usage
) {
#ifdef __EMSCRIPTEN__
    EM_ASM({
        const gl = GLctx;
        if (!gl) return;
        const glBuffer = GL.bufferMap.get($0);
        if (!glBuffer) return;
        gl.bindBuffer($1, glBuffer);
        gl.bufferData($1, HEAPU8.slice($2, $2 + $3), $4);
    }, buffer, target, reinterpret_cast<uintptr_t>(data), size, usage);
#endif
}

// ============================================================================
// Texture Management
// ============================================================================

uint32_t webgl2_create_texture(
    uint32_t width,
    uint32_t height,
    uint32_t format,
    const void* data
) {
#ifdef __EMSCRIPTEN__
    return EM_ASM_INT({
        const gl = GLctx;
        if (!gl) return 0;
        
        const texture = gl.createTexture();
        gl.bindTexture(gl.TEXTURE_2D, texture);
        
        // Set parameters
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR_MIPMAP_LINEAR);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
        
        // Upload data
        const ptr = $4;
        const size = $0 * $1 * 4; // Assume RGBA
        const pixels = new Uint8Array(HEAPU8.buffer, ptr, size);
        gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, $0, $1, 0, gl.RGBA, gl.UNSIGNED_BYTE, pixels);
        gl.generateMipmap(gl.TEXTURE_2D);
        
        return GL.textureMap.get(texture);
    }, width, height, format, 0, reinterpret_cast<uintptr_t>(data));
#else
    return 0;
#endif
}

void webgl2_destroy_texture(uint32_t texture) {
#ifdef __EMSCRIPTEN__
    EM_ASM({
        const gl = GLctx;
        if (!gl) return;
        const glTexture = GL.textureMap.get($0);
        if (glTexture) {
            gl.deleteTexture(glTexture);
            GL.textureMap.delete($0);
        }
    }, texture);
#endif
}

// ============================================================================
// Statistics
// ============================================================================

WebGL2Stats webgl2_get_stats() {
    return {
        .drawCalls = g_webgl2_state.drawCalls,
        .triangles = g_webgl2_state.triangles,
        .vertices = g_webgl2_state.vertices
    };
}

void webgl2_reset_stats() {
    g_webgl2_state.drawCalls = 0;
    g_webgl2_state.triangles = 0;
    g_webgl2_state.vertices = 0;
}

} // namespace phoenix::wasm
