/**
 * Phoenix Engine WebAssembly TypeScript Definitions
 */

export interface EngineConfig {
    width?: number;
    height?: number;
    enableWebGPU?: boolean;
    enableWebGL2?: boolean;
    enableAsyncify?: boolean;
    initialMemoryMB?: number;
    maxMemoryMB?: number;
    enableThreading?: boolean;
    threadPoolSize?: number;
    canvasId?: string | HTMLCanvasElement;
    wasmPath?: string;
}

export interface ResourceHandle {
    id: number;
    valid: boolean;
    type: string;
    error: string | null;
}

export interface SceneHandle {
    id: number;
    valid: boolean;
}

export interface TextureHandle {
    id: number;
    width: number;
    height: number;
    format: number;
    valid: boolean;
}

export interface FrameInfo {
    deltaTime: number;
    frameTime: number;
    frameNumber: bigint;
    fps: number;
}

export interface GraphicsCaps {
    hasWebGPU: boolean;
    hasWebGL2: boolean;
    hasWebGL: boolean;
    hasSharedArrayBuffer: boolean;
    hasBigInt: boolean;
    hasAsyncify: boolean;
    hasEXT_color_buffer_float: boolean;
    hasEXT_color_buffer_half_float: boolean;
    hasOES_texture_float_linear: boolean;
    hasWEBGL_multi_draw: boolean;
    hasKHR_parallel_shader_compile: boolean;
    maxTextureSize: number;
    maxCubeMapTextureSize: number;
    maxVertexAttribs: number;
    maxFragmentUniformVectors: number;
    maxVertexUniformVectors: number;
    maxDrawBuffers: number;
    hasVertexArrayObject: boolean;
    hasInstancedArrays: boolean;
    hasDrawBuffers: boolean;
    hasTransformFeedback: boolean;
}

export interface MemoryStats {
    totalBytes: bigint;
    usedBytes: bigint;
    freeBytes: bigint;
    usagePercent: number;
}

export interface EngineCallbacks {
    onProgress?: (progress: number) => void;
    onLoad?: (engine: PhoenixEngine) => void;
    onError?: (error: Error) => void;
    onFrame?: (frameInfo: FrameInfo) => void;
}

export interface PhoenixEngineOptions {
    config?: EngineConfig;
    callbacks?: EngineCallbacks;
}

/**
 * Phoenix Engine WebAssembly Class
 */
export declare class PhoenixEngine {
    module: any;
    canvas: HTMLCanvasElement | null;
    context: WebGL2RenderingContext | GPUCanvasContext | null;
    isInitialized: boolean;
    isRunning: boolean;
    callbacks: EngineCallbacks;
    resources: Map<number, ResourceHandle>;
    scenes: Map<number, SceneHandle>;

    constructor();

    /**
     * Initialize the engine
     */
    init(config?: EngineConfig): Promise<PhoenixEngine>;

    /**
     * Start the render loop
     */
    start(): void;

    /**
     * Stop the render loop
     */
    stop(): void;

    /**
     * Resize the canvas
     */
    resize(width: number, height: number): void;

    /**
     * Load a resource
     */
    loadResource(url: string, type?: string): Promise<ResourceHandle>;

    /**
     * Create a scene
     */
    createScene(): SceneHandle;

    /**
     * Add resource to scene
     */
    addToScene(sceneId: number, resourceId: number): void;

    /**
     * Create a texture from image data
     */
    createTexture(data: ImageData | HTMLImageElement): TextureHandle;

    /**
     * Get graphics capabilities
     */
    getGraphicsCaps(): GraphicsCaps | null;

    /**
     * Get memory statistics
     */
    getMemoryStats(): MemoryStats | null;

    /**
     * Save data to IndexedDB
     */
    saveToIDB(path: string, data: Uint8Array): boolean;

    /**
     * Load data from IndexedDB
     */
    loadFromIDB(path: string): Uint8Array | null;

    /**
     * Set callbacks
     */
    setCallbacks(callbacks: EngineCallbacks): void;

    /**
     * Shutdown the engine
     */
    shutdown(): void;
}

/**
 * WebGPU-specific types
 */
export interface WebGPUDevice {
    adapter: GPUAdapter;
    device: GPUDevice;
    context: GPUCanvasContext;
    queue: GPUQueue;
}

export interface WebGPUPipeline {
    pipeline: GPURenderPipeline;
    bindGroup: GPUBindGroup;
    shaderModule: GPUShaderModule;
}

export interface WebGPUTexture {
    texture: GPUTexture;
    view: GPUTextureView;
    sampler: GPUSampler;
}

/**
 * WebGL2-specific types
 */
export interface WebGL2Extensions {
    colorBufferFloat?: EXT_color_buffer_float;
    colorBufferHalfFloat?: EXT_color_buffer_half_float;
    textureFloatLinear?: OES_texture_float_linear;
    multiDraw?: WEBGL_multi_draw;
    parallelShaderCompile?: KHR_parallel_shader_compile;
}

export interface WebGL2Context {
    gl: WebGL2RenderingContext;
    extensions: WebGL2Extensions;
    vao: WebGLVertexArrayObject;
}

/**
 * Resource types
 */
export type ResourceType = 'mesh' | 'texture' | 'shader' | 'scene' | 'animation';

export interface MeshData {
    vertices: Float32Array;
    normals: Float32Array;
    uvs: Float32Array;
    indices: Uint16Array | Uint32Array;
    materialIndex: number;
}

export interface TextureData {
    width: number;
    height: number;
    channels: number;
    data: Uint8Array | Uint16Array | Float32Array;
    format: string;
}

export interface ShaderData {
    vertex: string;
    fragment: string;
    uniforms: UniformInfo[];
    attributes: AttributeInfo[];
}

export interface UniformInfo {
    name: string;
    type: string;
    location: number;
}

export interface AttributeInfo {
    name: string;
    type: string;
    location: number;
}

/**
 * Event types
 */
export interface EngineEvent {
    type: 'init' | 'frame' | 'resize' | 'load' | 'error' | 'shutdown';
    timestamp: number;
    data?: any;
}

export interface FrameEvent extends EngineEvent {
    type: 'frame';
    data: FrameInfo;
}

export interface LoadEvent extends EngineEvent {
    type: 'load';
    data: ResourceHandle;
}

export interface ErrorEvent extends EngineEvent {
    type: 'error';
    data: Error;
}

/**
 * Performance metrics
 */
export interface PerformanceMetrics {
    fps: number;
    frameTime: number;
    drawCalls: number;
    triangles: number;
    vertices: number;
    textures: number;
    memoryUsage: number;
    gpuMemoryUsage: number;
}

/**
 * WASM Module exports
 */
export interface PhoenixWasmModule {
    _malloc: (size: number) => number;
    _free: (ptr: number) => void;
    ccall: (
        name: string,
        returnType: string | null,
        argTypes: string[],
        args: any[]
    ) => any;
    cwrap: (
        name: string,
        returnType: string | null,
        argTypes: string[]
    ) => (...args: any[]) => any;
    FS: any;
    IDBFS: any;
    preRun: any[];
    postRun: any[];
    setValue: (ptr: number, value: any, type: string) => void;
    getValue: (ptr: number, type: string) => any;
    stringToUTF8: (str: string, outPtr: number, maxBytes: number) => void;
    stringToUTF8OnStack: (str: string) => number;
    UTF8ToString: (ptr: number) => string;
    writeArrayToMemory: (array: ArrayLike<number>, ptr: number) => void;
    HEAP8: Int8Array;
    HEAPU8: Uint8Array;
    HEAP16: Int16Array;
    HEAPU16: Uint16Array;
    HEAP32: Int32Array;
    HEAPU32: Uint32Array;
    HEAPF32: Float32Array;
    HEAPF64: Float64Array;
}

/**
 * Module factory function
 */
export interface PhoenixModuleFactory {
    (config?: {
        locateFile?: (path: string) => string;
    }): Promise<PhoenixWasmModule>;
}

declare global {
    interface Window {
        createPhoenixEngine?: PhoenixModuleFactory;
        PhoenixEngine?: typeof PhoenixEngine;
    }
}

export default PhoenixEngine;
