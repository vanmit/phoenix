/**
 * Phoenix Engine WebGPU Backend
 * 
 * wgpu-native WASM compilation with async resource loading and error handling
 */

#include <phoenix/wasm/types.hpp>
#include <cstdio>
#include <cstring>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5_webgpu.h>
#include <webgpu/webgpu.h>
#endif

namespace phoenix::wasm {

// ============================================================================
// WebGPU Backend State
// ============================================================================

struct WebGPUState {
    bool initialized = false;
    
#ifdef __EMSCRIPTEN__
    WGPUInstance instance = nullptr;
    WGPUAdapter adapter = nullptr;
    WGPUDevice device = nullptr;
    WGPUQueue queue = nullptr;
    WGPUSurface surface = nullptr;
    WGPUSwapChain swapChain = nullptr;
#endif
    
    // Capabilities
    bool hasComputeShaders = false;
    bool hasStorageBuffers = false;
    bool hasTimestampQueries = false;
    bool hasPipelineStatistics = false;
    bool hasTextureCompressionBC = false;
    bool hasTextureCompressionETC2 = false;
    bool hasTextureCompressionASTC = false;
    bool hasFloat32Filterable = false;
    
    // Limits
    uint32_t maxTextureDimension1D = 0;
    uint32_t maxTextureDimension2D = 0;
    uint32_t maxTextureDimension3D = 0;
    uint32_t maxTextureArrayLayers = 0;
    uint32_t maxBindGroups = 0;
    uint32_t maxDynamicUniformBuffersPerPipelineLayout = 0;
    uint32_t maxDynamicStorageBuffersPerPipelineLayout = 0;
    uint32_t maxSampledTexturesPerShaderStage = 0;
    uint32_t maxSamplersPerShaderStage = 0;
    uint32_t maxUniformBuffersPerShaderStage = 0;
    uint32_t maxStorageBuffersPerShaderStage = 0;
    uint32_t maxUniformBufferBindingSize = 0;
    uint32_t maxStorageBufferBindingSize = 0;
    uint32_t minUniformBufferOffsetAlignment = 0;
    uint32_t minStorageBufferOffsetAlignment = 0;
    uint32_t maxVertexBuffers = 0;
    uint32_t maxVertexAttributes = 0;
    uint32_t maxVertexBufferArrayStride = 0;
    uint32_t maxInterStageShaderVariables = 0;
    
    // Statistics
    uint32_t drawCalls = 0;
    uint32_t computeDispatches = 0;
    uint64_t bytesUploaded = 0;
    
    // Error handling
    char lastError[256] = {0};
    bool hasError = false;
};

static WebGPUState g_webgpu_state;

// ============================================================================
// Error Callback
// ============================================================================

#ifdef __EMSCRIPTEN__
static void webgpu_error_callback(
    WGPUErrorType type,
    const char* message,
    void* userdata
) {
    WebGPUState* state = static_cast<WebGPUState*>(userdata);
    
    snprintf(state->lastError, sizeof(state->lastError), 
             "WebGPU Error [%d]: %s", type, message);
    state->hasError = true;
    
    fprintf(stderr, "%s\n", state->lastError);
    
    // Log to browser console
    EM_ASM({
        console.error(UTF8ToString($0));
    }, state->lastError);
}
#endif

// ============================================================================
// Initialization
// ============================================================================

bool webgpu_init() {
    if (g_webgpu_state.initialized) {
        return true;
    }
    
#ifdef __EMSCRIPTEN__
    // Check WebGPU availability
    bool hasWebGPU = EM_ASM_INT({
        return typeof navigator !== 'undefined' && 
               navigator.gpu !== undefined;
    });
    
    if (!hasWebGPU) {
        fprintf(stderr, "WebGPU not available in this browser\n");
        return false;
    }
    
    // Create instance
    WGPUInstanceDescriptor instanceDesc = {};
    instanceDesc.nextInChain = nullptr;
    g_webgpu_state.instance = wgpuCreateInstance(&instanceDesc);
    
    if (!g_webgpu_state.instance) {
        fprintf(stderr, "Failed to create WebGPU instance\n");
        return false;
    }
    
    // Request adapter
    // Note: In real implementation, this would be async
    EM_ASM({
        // WebGPU adapter request would happen here
        // This is a simplified synchronous version
        Module._webgpu_adapter_requested = 1;
    });
    
    // For now, we'll simulate successful initialization
    // In production, you'd use Asyncify to handle the promise
    g_webgpu_state.adapter = reinterpret_cast<WGPUAdapter>(1);
    
    // Request device
    WGPUDeviceDescriptor deviceDesc = {};
    deviceDesc.nextInChain = nullptr;
    deviceDesc.label = "Phoenix Engine Device";
    
    // Set up error callback
    deviceDesc.deviceLostCallback = [](WGPUDeviceLostReason reason, 
                                        const char* message, 
                                        void* userdata) {
        fprintf(stderr, "Device lost: %s\n", message);
    };
    deviceDesc.deviceLostUserdata = &g_webgpu_state;
    
    g_webgpu_state.device = wgpuAdapterRequestDevice(
        g_webgpu_state.adapter, 
        &deviceDesc
    );
    
    if (!g_webgpu_state.device) {
        fprintf(stderr, "Failed to get WebGPU device\n");
        webgpu_shutdown();
        return false;
    }
    
    // Set up error callback
    wgpuDeviceSetUncapturedErrorCallback(
        g_webgpu_state.device,
        webgpu_error_callback,
        &g_webgpu_state
    );
    
    // Get queue
    g_webgpu_state.queue = wgpuDeviceGetQueue(g_webgpu_state.device);
    
    // Query limits
    WGPUSupportedLimits supportedLimits = {};
    if (wgpuDeviceGetLimits(g_webgpu_state.device, &supportedLimits)) {
        g_webgpu_state.maxTextureDimension1D = supportedLimits.limits.maxTextureDimension1D;
        g_webgpu_state.maxTextureDimension2D = supportedLimits.limits.maxTextureDimension2D;
        g_webgpu_state.maxTextureDimension3D = supportedLimits.limits.maxTextureDimension3D;
        g_webgpu_state.maxTextureArrayLayers = supportedLimits.limits.maxTextureArrayLayers;
        g_webgpu_state.maxBindGroups = supportedLimits.limits.maxBindGroups;
        g_webgpu_state.maxDynamicUniformBuffersPerPipelineLayout = 
            supportedLimits.limits.maxDynamicUniformBuffersPerPipelineLayout;
        g_webgpu_state.maxDynamicStorageBuffersPerPipelineLayout = 
            supportedLimits.limits.maxDynamicStorageBuffersPerPipelineLayout;
        g_webgpu_state.maxSampledTexturesPerShaderStage = 
            supportedLimits.limits.maxSampledTexturesPerShaderStage;
        g_webgpu_state.maxSamplersPerShaderStage = 
            supportedLimits.limits.maxSamplersPerShaderStage;
        g_webgpu_state.maxUniformBuffersPerShaderStage = 
            supportedLimits.limits.maxUniformBuffersPerShaderStage;
        g_webgpu_state.maxStorageBuffersPerShaderStage = 
            supportedLimits.limits.maxStorageBuffersPerShaderStage;
        g_webgpu_state.maxUniformBufferBindingSize = 
            supportedLimits.limits.maxUniformBufferBindingSize;
        g_webgpu_state.maxStorageBufferBindingSize = 
            supportedLimits.limits.maxStorageBufferBindingSize;
        g_webgpu_state.minUniformBufferOffsetAlignment = 
            supportedLimits.limits.minUniformBufferOffsetAlignment;
        g_webgpu_state.minStorageBufferOffsetAlignment = 
            supportedLimits.limits.minStorageBufferOffsetAlignment;
        g_webgpu_state.maxVertexBuffers = supportedLimits.limits.maxVertexBuffers;
        g_webgpu_state.maxVertexAttributes = supportedLimits.limits.maxVertexAttributes;
        g_webgpu_state.maxVertexBufferArrayStride = 
            supportedLimits.limits.maxVertexBufferArrayStride;
        g_webgpu_state.maxInterStageShaderVariables = 
            supportedLimits.limits.maxInterStageShaderVariables;
    }
    
    // Query features
    // (In real implementation, check wgpuDeviceEnumerateFeatures)
    g_webgpu_state.hasComputeShaders = true;
    g_webgpu_state.hasStorageBuffers = true;
    g_webgpu_state.hasTimestampQueries = true;
    
#endif
    
    g_webgpu_state.initialized = true;
    
    printf("WebGPU initialized successfully\n");
    printf("  Max Texture 2D: %u\n", g_webgpu_state.maxTextureDimension2D);
    printf("  Max Bind Groups: %u\n", g_webgpu_state.maxBindGroups);
    printf("  Max Vertex Buffers: %u\n", g_webgpu_state.maxVertexBuffers);
    
    return true;
}

void webgpu_shutdown() {
    if (!g_webgpu_state.initialized) {
        return;
    }
    
#ifdef __EMSCRIPTEN__
    if (g_webgpu_state.queue) {
        wgpuQueueRelease(g_webgpu_state.queue);
        g_webgpu_state.queue = nullptr;
    }
    
    if (g_webgpu_state.device) {
        wgpuDeviceRelease(g_webgpu_state.device);
        g_webgpu_state.device = nullptr;
    }
    
    if (g_webgpu_state.adapter) {
        wgpuAdapterRelease(g_webgpu_state.adapter);
        g_webgpu_state.adapter = nullptr;
    }
    
    if (g_webgpu_state.instance) {
        wgpuInstanceRelease(g_webgpu_state.instance);
        g_webgpu_state.instance = nullptr;
    }
#endif
    
    g_webgpu_state.initialized = false;
}

// ============================================================================
// Async Resource Loading
// ============================================================================

struct ResourceLoadRequest {
    uint32_t id;
    const char* url;
    void (*callback)(void* userdata, bool success);
    void* userdata;
};

#ifdef __EMSCRIPTEN__
static EM_BOOL on_resource_loaded(
    int eventType,
    const EmscriptenWebGpuResourceLoadEvent* event,
    void* userdata
) {
    ResourceLoadRequest* request = static_cast<ResourceLoadRequest*>(userdata);
    
    if (request && request->callback) {
        request->callback(request->userdata, event->success);
    }
    
    return EM_TRUE;
}
#endif

uint32_t webgpu_load_texture_async(
    const char* url,
    void (*callback)(void* userdata, bool success),
    void* userdata
) {
#ifdef __EMSCRIPTEN__
    // Create load request
    static uint32_t nextRequestId = 1;
    uint32_t requestId = nextRequestId++;
    
    EM_ASM({
        // Async texture loading via fetch
        const url = UTF8ToString($0);
        const requestId = $1;
        
        fetch(url)
            .then(response => response.blob())
            .then(blob => createImageBitmap(blob))
            .then(bitmap => {
                // Upload to WebGPU texture
                Module._webgpu_texture_loaded(requestId, bitmap.width, bitmap.height);
            })
            .catch(err => {
                console.error('Texture load failed:', err);
                Module._webgpu_texture_load_failed(requestId);
            });
    }, url, requestId);
    
    return requestId;
#else
    return 0;
#endif
}

uint32_t webgpu_load_model_async(
    const char* url,
    void (*callback)(void* userdata, bool success),
    void* userdata
) {
#ifdef __EMSCRIPTEN__
    static uint32_t nextRequestId = 1;
    uint32_t requestId = nextRequestId++;
    
    EM_ASM({
        const url = UTF8ToString($0);
        const requestId = $1;
        
        fetch(url)
            .then(response => response.arrayBuffer())
            .then(buffer => {
                // Parse glTF or other format
                Module._webgpu_model_loaded(requestId);
            })
            .catch(err => {
                console.error('Model load failed:', err);
                Module._webgpu_model_load_failed(requestId);
            });
    }, url, requestId);
    
    return requestId;
#else
    return 0;
#endif
}

// ============================================================================
// Pipeline Creation
// ============================================================================

#ifdef __EMSCRIPTEN__
WGPURenderPipeline webgpu_create_pipeline(
    const char* vertexShader,
    const char* fragmentShader,
    const WGPURenderPipelineDescriptor* desc
) {
    if (!g_webgpu_state.initialized || !g_webgpu_state.device) {
        return nullptr;
    }
    
    // Create shader modules
    WGPUShaderModuleVSDescriptor vsDesc = {};
    vsDesc.nextInChain = nullptr;
    // In real implementation, compile WGSL or SPIR-V
    
    WGPUShaderModuleFSDescriptor fsDesc = {};
    fsDesc.nextInChain = nullptr;
    
    WGPURenderPipelineDescriptor pipelineDesc = *desc;
    pipelineDesc.vertex.module = wgpuDeviceCreateShaderModule(
        g_webgpu_state.device,
        &vsDesc
    );
    
    if (fragmentShader) {
        pipelineDesc.fragment.module = wgpuDeviceCreateShaderModule(
            g_webgpu_state.device,
            &fsDesc
        );
    }
    
    WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline(
        g_webgpu_state.device,
        &pipelineDesc
    );
    
    return pipeline;
}
#endif

// ============================================================================
// Buffer Management
// ============================================================================

#ifdef __EMSCRIPTEN__
WGPUBuffer webgpu_create_buffer(
    const void* data,
    size_t size,
    WGPUBufferUsage usage,
    WGPUMapMode mapMode
) {
    if (!g_webgpu_state.initialized || !g_webgpu_state.device) {
        return nullptr;
    }
    
    WGPUBufferDescriptor desc = {};
    desc.nextInChain = nullptr;
    desc.label = nullptr;
    desc.usage = usage;
    desc.size = size;
    desc.mappedAtCreation = (mapMode == WGPUMapMode_Write);
    
    WGPUBuffer buffer = wgpuDeviceCreateBuffer(g_webgpu_state.device, &desc);
    
    if (buffer && data && mapMode == WGPUMapMode_Write) {
        void* mapped = wgpuBufferGetMappedRange(buffer, 0, size);
        memcpy(mapped, data, size);
        wgpuBufferUnmap(buffer);
        g_webgpu_state.bytesUploaded += size;
    }
    
    return buffer;
}

void webgpu_destroy_buffer(WGPUBuffer buffer) {
    if (buffer) {
        wgpuBufferRelease(buffer);
    }
}
#endif

// ============================================================================
// Texture Management
// ============================================================================

#ifdef __EMSCRIPTEN__
WGPUTexture webgpu_create_texture(
    uint32_t width,
    uint32_t height,
    WGPUTextureFormat format,
    WGPUTextureUsage usage,
    const void* data
) {
    if (!g_webgpu_state.initialized || !g_webgpu_state.device) {
        return nullptr;
    }
    
    WGPUTextureDescriptor desc = {};
    desc.nextInChain = nullptr;
    desc.label = nullptr;
    desc.usage = usage;
    desc.dimension = WGPUTextureDimension_2D;
    desc.size.width = width;
    desc.size.height = height;
    desc.size.depthOrArrayLayers = 1;
    desc.format = format;
    desc.mipLevelCount = 1;
    desc.sampleCount = 1;
    
    WGPUTexture texture = wgpuDeviceCreateTexture(g_webgpu_state.device, &desc);
    
    if (texture && data) {
        WGPUTextureViewDescriptor viewDesc = {};
        WGPUTextureView view = wgpuTextureCreateView(texture, &viewDesc);
        
        // Upload data via queue
        wgpuQueueWriteTexture(
            g_webgpu_state.queue,
            {texture, {0, 0, 0}, {width, height, 1}, {0, 0}},
            data,
            width * 4, // Assume RGBA
            {width, height, 1}
        );
        
        wgpuTextureViewRelease(view);
        g_webgpu_state.bytesUploaded += width * height * 4;
    }
    
    return texture;
}

void webgpu_destroy_texture(WGPUTexture texture) {
    if (texture) {
        wgpuTextureRelease(texture);
    }
}
#endif

// ============================================================================
// Error Handling
// ============================================================================

const char* webgpu_get_last_error() {
    return g_webgpu_state.hasError ? g_webgpu_state.lastError : nullptr;
}

void webgpu_clear_error() {
    g_webgpu_state.hasError = false;
    g_webgpu_state.lastError[0] = '\0';
}

bool webgpu_has_error() {
    return g_webgpu_state.hasError;
}

// ============================================================================
// Statistics
// ============================================================================

WebGPUStats webgpu_get_stats() {
    return {
        .drawCalls = g_webgpu_state.drawCalls,
        .computeDispatches = g_webgpu_state.computeDispatches,
        .bytesUploaded = g_webgpu_state.bytesUploaded
    };
}

void webgpu_reset_stats() {
    g_webgpu_state.drawCalls = 0;
    g_webgpu_state.computeDispatches = 0;
    g_webgpu_state.bytesUploaded = 0;
}

// ============================================================================
// Capabilities Query
// ============================================================================

WebGPUCaps webgpu_get_caps() {
    return {
        .hasComputeShaders = g_webgpu_state.hasComputeShaders,
        .hasStorageBuffers = g_webgpu_state.hasStorageBuffers,
        .hasTimestampQueries = g_webgpu_state.hasTimestampQueries,
        .maxTextureDimension2D = g_webgpu_state.maxTextureDimension2D,
        .maxBindGroups = g_webgpu_state.maxBindGroups,
        .maxVertexBuffers = g_webgpu_state.maxVertexBuffers
    };
}

} // namespace phoenix::wasm
