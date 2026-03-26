/**
 * Phoenix Engine WebAssembly JavaScript API
 * 
 * High-level wrapper for the Phoenix Engine WASM module
 */

class PhoenixEngine {
    constructor() {
        this.module = null;
        this.canvas = null;
        this.context = null;
        this.isInitialized = false;
        this.isRunning = false;
        this.animationFrameId = null;
        this.callbacks = {
            onProgress: null,
            onLoad: null,
            onError: null,
            onFrame: null
        };
        this.resources = new Map();
        this.scenes = new Map();
    }

    /**
     * Initialize the engine
     * @param {Object} config - Configuration options
     * @returns {Promise<PhoenixEngine>}
     */
    async init(config = {}) {
        const defaultConfig = {
            width: 1024,
            height: 768,
            enableWebGPU: true,
            enableWebGL2: true,
            enableAsyncify: true,
            canvasId: 'canvas',
            wasmPath: './phoenix-wasm.js'
        };

        const finalConfig = { ...defaultConfig, ...config };

        try {
            // Load WASM module
            this.module = await this._loadWasmModule(finalConfig.wasmPath);

            // Get canvas
            this.canvas = typeof finalConfig.canvasId === 'string'
                ? document.getElementById(finalConfig.canvasId)
                : finalConfig.canvasId;

            if (!this.canvas) {
                throw new Error('Canvas element not found');
            }

            // Initialize engine
            const engineConfig = this._createEngineConfig(finalConfig);
            const result = this.module.ccall(
                'phoenix_init',
                'number',
                ['number'],
                [engineConfig]
            );

            if (result !== 0) {
                throw new Error(`Engine initialization failed with code: ${result}`);
            }

            this.isInitialized = true;

            if (this.callbacks.onLoad) {
                this.callbacks.onLoad(this);
            }

            return this;
        } catch (error) {
            if (this.callbacks.onError) {
                this.callbacks.onError(error);
            }
            throw error;
        }
    }

    /**
     * Load WASM module
     * @private
     */
    async _loadWasmModule(path) {
        if (typeof createPhoenixEngine !== 'undefined') {
            return await createPhoenixEngine({
                locateFile: (file) => {
                    if (file.endsWith('.wasm')) {
                        return path.replace('.js', '.wasm');
                    }
                    return path.replace('phoenix-wasm.js', file);
                }
            });
        }

        // Dynamic import fallback
        const moduleExports = await import(path);
        return await moduleExports.default();
    }

    /**
     * Create engine config struct in memory
     * @private
     */
    _createEngineConfig(config) {
        const structSize = 40; // Size of EngineConfig struct
        const ptr = this.module._malloc(structSize);

        // Write config values (matching C++ struct layout)
        this.module.setValue(ptr, config.width, 'i32');
        this.module.setValue(ptr + 4, config.height, 'i32');
        this.module.setValue(ptr + 8, config.enableWebGPU ? 1 : 0, 'i32');
        this.module.setValue(ptr + 12, config.enableWebGL2 ? 1 : 0, 'i32');
        this.module.setValue(ptr + 16, config.enableAsyncify ? 1 : 0, 'i32');
        this.module.setValue(ptr + 20, config.initialMemoryMB || 128, 'i32');
        this.module.setValue(ptr + 24, config.maxMemoryMB || 2048, 'i32');
        this.module.setValue(ptr + 28, config.enableThreading ? 1 : 0, 'i32');
        this.module.setValue(ptr + 32, config.threadPoolSize || 4, 'i32');

        return ptr;
    }

    /**
     * Start the render loop
     */
    start() {
        if (!this.isInitialized) {
            throw new Error('Engine not initialized');
        }

        if (this.isRunning) {
            return;
        }

        this.isRunning = true;
        this._renderLoop();
    }

    /**
     * Stop the render loop
     */
    stop() {
        this.isRunning = false;
        if (this.animationFrameId) {
            cancelAnimationFrame(this.animationFrameId);
            this.animationFrameId = null;
        }
    }

    /**
     * Internal render loop
     * @private
     */
    _renderLoop() {
        if (!this.isRunning) {
            return;
        }

        // Update frame
        this.module.ccall('phoenix_frame', null, []);

        // Submit frame
        this.module.ccall('phoenix_submit_frame', null, []);

        // Get frame info
        const frameInfoPtr = this.module.ccall(
            'phoenix_get_frame_info',
            'number',
            []
        );

        const frameInfo = {
            deltaTime: this.module.getValue(frameInfoPtr, 'double'),
            frameTime: this.module.getValue(frameInfoPtr + 8, 'double'),
            frameNumber: this.module.getValue(frameInfoPtr + 16, 'i64'),
            fps: this.module.getValue(frameInfoPtr + 24, 'float')
        };

        // Callback
        if (this.callbacks.onFrame) {
            this.callbacks.onFrame(frameInfo);
        }

        // Next frame
        this.animationFrameId = requestAnimationFrame(() => this._renderLoop());
    }

    /**
     * Resize the canvas
     * @param {number} width 
     * @param {number} height 
     */
    resize(width, height) {
        if (!this.isInitialized) return;

        this.module.ccall('phoenix_resize', null, ['number', 'number'], [width, height]);
        
        if (this.canvas) {
            this.canvas.width = width;
            this.canvas.height = height;
        }
    }

    /**
     * Load a resource
     * @param {string} url - Resource URL
     * @param {string} type - Resource type (mesh, texture, etc.)
     * @returns {Promise<Object>} Resource handle
     */
    async loadResource(url, type = 'mesh') {
        if (!this.isInitialized) {
            throw new Error('Engine not initialized');
        }

        const urlPtr = this.module.stringToUTF8OnStack(url);
        const typePtr = this.module.stringToUTF8OnStack(type);

        const handlePtr = this.module.ccall(
            'phoenix_load_resource',
            'number',
            ['number', 'number'],
            [urlPtr, typePtr]
        );

        const handle = {
            id: this.module.getValue(handlePtr, 'i32'),
            valid: this.module.getValue(handlePtr + 4, 'i32') !== 0,
            type: this.module.UTF8ToString(this.module.getValue(handlePtr + 8, 'i32')),
            error: this.module.getValue(handlePtr + 12, 'i32') !== 0
                ? this.module.UTF8ToString(this.module.getValue(handlePtr + 12, 'i32'))
                : null
        };

        if (handle.valid) {
            this.resources.set(handle.id, handle);
        }

        return handle;
    }

    /**
     * Create a scene
     * @returns {Object} Scene handle
     */
    createScene() {
        if (!this.isInitialized) {
            throw new Error('Engine not initialized');
        }

        const handlePtr = this.module.ccall(
            'phoenix_create_scene',
            'number',
            []
        );

        const handle = {
            id: this.module.getValue(handlePtr, 'i32'),
            valid: this.module.getValue(handlePtr + 4, 'i32') !== 0
        };

        if (handle.valid) {
            this.scenes.set(handle.id, handle);
        }

        return handle;
    }

    /**
     * Add resource to scene
     * @param {number} sceneId 
     * @param {number} resourceId 
     */
    addToScene(sceneId, resourceId) {
        if (!this.isInitialized) return;

        this.module.ccall(
            'phoenix_add_to_scene',
            null,
            ['number', 'number'],
            [sceneId, resourceId]
        );
    }

    /**
     * Create a texture from image data
     * @param {ImageData|HTMLImageElement} data 
     * @returns {Object} Texture handle
     */
    createTexture(data) {
        if (!this.isInitialized) {
            throw new Error('Engine not initialized');
        }

        let width, height, ptr;

        if (data instanceof ImageData) {
            width = data.width;
            height = data.height;
            ptr = this.module._malloc(width * height * 4);
            this.module.HEAPU8.set(data.data, ptr);
        } else if (data instanceof HTMLImageElement) {
            // Would need to draw to canvas first
            const canvas = document.createElement('canvas');
            canvas.width = data.width;
            canvas.height = data.height;
            const ctx = canvas.getContext('2d');
            ctx.drawImage(data, 0, 0);
            const imageData = ctx.getImageData(0, 0, data.width, data.height);
            width = data.width;
            height = data.height;
            ptr = this.module._malloc(width * height * 4);
            this.module.HEAPU8.set(imageData.data, ptr);
        } else {
            throw new Error('Unsupported texture data type');
        }

        const handlePtr = this.module.ccall(
            'phoenix_create_texture',
            'number',
            ['number', 'number', 'number'],
            [width, height, ptr]
        );

        this.module._free(ptr);

        const handle = {
            id: this.module.getValue(handlePtr, 'i32'),
            width: this.module.getValue(handlePtr + 4, 'i32'),
            height: this.module.getValue(handlePtr + 8, 'i32'),
            format: this.module.getValue(handlePtr + 12, 'i32'),
            valid: this.module.getValue(handlePtr + 16, 'i32') !== 0
        };

        if (handle.valid) {
            this.resources.set(handle.id, handle);
        }

        return handle;
    }

    /**
     * Get graphics capabilities
     * @returns {Object} Graphics capabilities
     */
    getGraphicsCaps() {
        if (!this.isInitialized) {
            return null;
        }

        const capsPtr = this.module.ccall(
            'phoenix_get_graphics_caps',
            'number',
            []
        );

        // Read capabilities struct
        const caps = {
            hasWebGPU: this.module.getValue(capsPtr, 'i32') !== 0,
            hasWebGL2: this.module.getValue(capsPtr + 4, 'i32') !== 0,
            hasWebGL: this.module.getValue(capsPtr + 8, 'i32') !== 0,
            hasSharedArrayBuffer: this.module.getValue(capsPtr + 12, 'i32') !== 0,
            hasBigInt: this.module.getValue(capsPtr + 16, 'i32') !== 0,
            hasAsyncify: this.module.getValue(capsPtr + 20, 'i32') !== 0,
            maxTextureSize: this.module.getValue(capsPtr + 24, 'i32'),
            maxVertexAttribs: this.module.getValue(capsPtr + 28, 'i32')
        };

        return caps;
    }

    /**
     * Get memory statistics
     * @returns {Object} Memory stats
     */
    getMemoryStats() {
        if (!this.isInitialized) {
            return null;
        }

        const statsPtr = this.module.ccall(
            'phoenix_get_memory_stats',
            'number',
            []
        );

        const stats = {
            totalBytes: this.module.getValue(statsPtr, 'i64'),
            usedBytes: this.module.getValue(statsPtr + 8, 'i64'),
            freeBytes: this.module.getValue(statsPtr + 16, 'i64'),
            usagePercent: this.module.getValue(statsPtr + 24, 'float')
        };

        return stats;
    }

    /**
     * Save data to IndexedDB
     * @param {string} path 
     * @param {Uint8Array} data 
     */
    saveToIDB(path, data) {
        if (!this.isInitialized) {
            throw new Error('Engine not initialized');
        }

        const pathPtr = this.module.stringToUTF8OnStack(path);
        const dataPtr = this.module._malloc(data.length);
        this.module.HEAPU8.set(data, dataPtr);

        const result = this.module.ccall(
            'phoenix_fs_save',
            'number',
            ['number', 'number', 'number'],
            [pathPtr, dataPtr, data.length]
        );

        this.module._free(dataPtr);

        return result === 0;
    }

    /**
     * Load data from IndexedDB
     * @param {string} path 
     * @returns {Uint8Array|null}
     */
    loadFromIDB(path) {
        if (!this.isInitialized) {
            return null;
        }

        const pathPtr = this.module.stringToUTF8OnStack(path);
        const size = this.module.ccall(
            'phoenix_fs_load',
            'number',
            ['number'],
            [pathPtr]
        );

        if (size <= 0) {
            return null;
        }

        // In a real implementation, we'd read the data from FS
        return null;
    }

    /**
     * Set callbacks
     * @param {Object} callbacks 
     */
    setCallbacks(callbacks) {
        this.callbacks = { ...this.callbacks, ...callbacks };
    }

    /**
     * Shutdown the engine
     */
    shutdown() {
        this.stop();

        if (this.isInitialized && this.module) {
            this.module.ccall('phoenix_shutdown', null, []);
            this.isInitialized = false;
        }

        this.resources.clear();
        this.scenes.clear();
    }
}

// Export for different module systems
if (typeof module !== 'undefined' && module.exports) {
    module.exports = PhoenixEngine;
} else if (typeof window !== 'undefined') {
    window.PhoenixEngine = PhoenixEngine;
}
