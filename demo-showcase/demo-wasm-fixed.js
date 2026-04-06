/**
 * Phoenix Engine - WASM Loader (Fixed Version)
 * 
 * Applied fixes from Wasm technical analysis:
 * - Robust loading with fallback to non-streaming
 * - Proper MIME type detection
 * - Memory management improvements
 * - Comprehensive error handling
 * - Debug mode support
 */

class PhoenixWasmLoader {
    constructor() {
        this.module = null;
        this.instance = null;
        this.canvas = null;
        this.isLoaded = false;
        this.isWasmSupported = true;
        this.debugMode = true;
        
        // Performance monitoring
        this.fps = 0;
        this.frameTime = 0;
        this.drawCalls = 0;
        this.triangleCount = 0;
        this.memoryUsage = 0;
        this.lastMemoryCheck = 0;
        
        // UI Elements
        this.ui = {};
        
        // Control state
        this.controlState = {
            camera: 'orbit',
            metallic: 0.5,
            roughness: 0.5,
            clearcoat: 0.0,
            directionalLight: true,
            pointLight: true,
            spotLight: true,
            shadows: true,
            bloom: true,
            toneMapping: true,
            ssao: false,
            animation: 'idle',
            blendSpeed: 0.3
        };
        
        // Memory views cache (to avoid use-after-free)
        this.memoryViews = new Map();
    }

    log(...args) {
        if (this.debugMode) {
            console.log('[PhoenixWasmLoader]', ...args);
        }
    }

    error(...args) {
        console.error('[PhoenixWasmLoader]', ...args);
    }

    async init() {
        this.log('Initializing...');
        const startTime = performance.now();
        
        try {
            this.cacheUIElements();
            this.setupEventListeners();
            
            // Step 1: Check WASM support
            this.updateLoadingStatus('检查浏览器支持...');
            this.updateLoadingProgress(5);
            
            if (!this.checkWasmSupport()) {
                throw new Error('您的浏览器不支持 WebAssembly。请使用现代浏览器 (Chrome 90+, Firefox 88+, Safari 15+, Edge 90+)');
            }
            this.log('✓ WASM support verified');
            this.updateLoadingProgress(10);
            
            // Step 2: Load WASM module
            await this.loadWasmModule();
            this.log('✓ WASM module loaded');
            
            // Step 3: Initialize GL/bgfx
            this.updateLoadingStatus('初始化渲染引擎...');
            this.updateLoadingProgress(85);
            await this.initializeGL();
            this.log('✓ GL initialized');
            
            // Step 4: Start render loop
            this.startRenderLoop();
            this.log('✓ Render loop started');
            
            // Step 5: Hide loading overlay
            this.updateLoadingProgress(100);
            setTimeout(() => this.hideLoading(), 300);
            
            this.isLoaded = true;
            const loadTime = performance.now() - startTime;
            this.log(`✓ Initialization complete in ${loadTime.toFixed(2)}ms`);
            
            // Start memory monitoring
            this.startMemoryMonitoring();
            
        } catch (error) {
            this.error('Initialization failed:', error);
            this.showError(this.getFriendlyErrorMessage(error));
        }
    }

    checkWasmSupport() {
        if (typeof WebAssembly !== 'object') {
            this.isWasmSupported = false;
            return false;
        }
        
        try {
            const bin = new Uint8Array([
                0x00, 0x61, 0x73, 0x6d,
                0x01, 0x00, 0x00, 0x00
            ]);
            new WebAssembly.Module(bin);
            return true;
        } catch (e) {
            this.isWasmSupported = false;
            return false;
        }
    }

    getFriendlyErrorMessage(error) {
        const msg = error.message?.toLowerCase() || '';
        const stack = error.stack || '';
        
        if (msg.includes('webassembly') || msg.includes('wasm')) {
            return 'WebAssembly 不支持：您的浏览器可能过旧或不支持 WASM。请升级到最新版本的 Chrome、Firefox、Safari 或 Edge。';
        }
        
        if (msg.includes('fetch') || msg.includes('network') || msg.includes('404') || msg.includes('failed to fetch')) {
            return 'WASM 文件加载失败：无法找到 phoenix-engine.wasm 文件。\n\n可能原因：\n• 文件路径错误\n• 服务器未正确配置 MIME 类型\n• CORS 限制\n\n请确保使用 HTTP 服务器访问（不能直接打开 file://）';
        }
        
        if (msg.includes('webgl') || msg.includes('context') || msg.includes('gl')) {
            return 'WebGL 初始化失败：您的浏览器或显卡驱动可能不支持 WebGL 2.0。\n\n建议：\n• 更新显卡驱动\n• 尝试 Chrome/Firefox 浏览器\n• 检查浏览器设置中 WebGL 是否启用';
        }
        
        if (msg.includes('memory') || msg.includes('heap') || msg.includes('out of memory')) {
            return '内存不足：WASM 模块需要更多内存。\n\n建议：\n• 关闭其他标签页\n• 重启浏览器\n• 检查是否有内存泄漏';
        }
        
        if (msg.includes('instantiate') || msg.includes('compile')) {
            return 'WASM 编译失败：WASM 文件可能已损坏或不兼容。\n\n建议：\n• 刷新页面重试\n• 清除浏览器缓存\n• 检查 WASM 文件是否完整';
        }
        
        if (msg.includes('mime') || msg.includes('content-type')) {
            return 'MIME 类型错误：服务器未正确配置 WASM 的 MIME 类型。\n\n服务器应配置：application/wasm for .wasm 文件';
        }
        
        return `初始化失败：${error.message}\n\n请打开浏览器控制台 (F12) 查看详细信息，或尝试刷新页面。`;
    }

    cacheUIElements() {
        this.ui = {
            loadingOverlay: document.getElementById('loading-overlay'),
            loadingProgress: document.getElementById('loading-progress'),
            loadingStatus: document.getElementById('loading-status'),
            errorOverlay: document.getElementById('error-overlay'),
            errorMessage: document.getElementById('error-message'),
            fpsValue: document.getElementById('fps-value'),
            frameTimeValue: document.getElementById('frame-time-value'),
            drawCallsValue: document.getElementById('drawCalls-value'),
            trianglesValue: document.getElementById('triangles-value'),
            memoryValue: document.getElementById('memory-value'),
            hudToggle: document.getElementById('hud-toggle'),
            hudContent: document.getElementById('hud-content'),
            controlToggle: document.getElementById('control-toggle'),
            controlContent: document.getElementById('control-content')
        };
    }

    setupEventListeners() {
        // HUD toggles
        this.ui.hudToggle?.addEventListener('click', () => {
            this.ui.hudContent.classList.toggle('collapsed');
            this.ui.hudToggle.textContent = this.ui.hudContent.classList.contains('collapsed') ? '▶' : '▼';
        });

        this.ui.controlToggle?.addEventListener('click', () => {
            this.ui.controlContent.classList.toggle('collapsed');
            this.ui.controlToggle.textContent = this.ui.controlContent.classList.contains('collapsed') ? '▶' : '▼';
        });

        // Camera buttons
        document.querySelectorAll('[data-camera]').forEach(btn => {
            btn.addEventListener('click', (e) => {
                document.querySelectorAll('[data-camera]').forEach(b => b.classList.remove('active'));
                e.target.classList.add('active');
                this.controlState.camera = e.target.dataset.camera;
                this.updateCameraMode(this.controlState.camera);
            });
        });

        // Animation buttons
        document.querySelectorAll('[data-anim]').forEach(btn => {
            btn.addEventListener('click', (e) => {
                document.querySelectorAll('[data-anim]').forEach(b => b.classList.remove('active'));
                e.target.classList.add('active');
                this.controlState.animation = e.target.dataset.anim;
                this.updateAnimation(this.controlState.animation);
            });
        });

        // Sliders
        const sliders = [
            { id: 'metallic-slider', valueId: 'metallic-value', key: 'metallic' },
            { id: 'roughness-slider', valueId: 'roughness-value', key: 'roughness' },
            { id: 'clearcoat-slider', valueId: 'clearcoat-value', key: 'clearcoat' },
            { id: 'blend-slider', valueId: 'blend-value', key: 'blendSpeed' }
        ];

        sliders.forEach(slider => {
            const el = document.getElementById(slider.id);
            const valueEl = document.getElementById(slider.valueId);
            if (el && valueEl) {
                el.addEventListener('input', (e) => {
                    const value = parseFloat(e.target.value);
                    valueEl.textContent = value.toFixed(2);
                    this.controlState[slider.key] = value;
                    this.updateMaterialParam(slider.key, value);
                });
            }
        });

        // Toggles
        const toggles = [
            { id: 'directional-light', key: 'directionalLight' },
            { id: 'point-light', key: 'pointLight' },
            { id: 'spot-light', key: 'spotLight' },
            { id: 'shadows', key: 'shadows' },
            { id: 'bloom', key: 'bloom' },
            { id: 'tone-mapping', key: 'toneMapping' },
            { id: 'ssao', key: 'ssao' }
        ];

        toggles.forEach(toggle => {
            const el = document.getElementById(toggle.id);
            if (el) {
                el.addEventListener('change', (e) => {
                    this.controlState[toggle.key] = e.target.checked;
                    this.updateEffect(toggle.key, e.target.checked);
                });
            }
        });

        // Retry button
        document.getElementById('retry-btn')?.addEventListener('click', () => {
            this.ui.errorOverlay.classList.add('hidden');
            this.isLoaded = false;
            this.memoryViews.clear();
            this.init();
        });

        // Gesture hints fade out
        setTimeout(() => {
            document.querySelectorAll('.hint').forEach(hint => {
                hint.classList.add('fade-out');
            });
        }, 8000);

        this.setupTouchGestures();
        this.setupKeyboardControls();
    }
    
    setupKeyboardControls() {
        document.addEventListener('keydown', (e) => {
            if (!this.isLoaded || !this.instance) return;
            
            switch(e.key) {
                case 'w': case 'W':
                    this.safeCall('demo_touch_rotate', 0, -1);
                    break;
                case 's': case 'S':
                    this.safeCall('demo_touch_rotate', 0, 1);
                    break;
                case 'a': case 'A':
                    this.safeCall('demo_touch_rotate', -1, 0);
                    break;
                case 'd': case 'D':
                    this.safeCall('demo_touch_rotate', 1, 0);
                    break;
                case '=': case '+':
                    this.safeCall('demo_touch_zoom', -0.5);
                    break;
                case '-': case '_':
                    this.safeCall('demo_touch_zoom', 0.5);
                    break;
                case 'r': case 'R':
                    this.safeCall('demo_double_tap');
                    break;
                case 'p': case 'P':
                    this.togglePerformanceHUD();
                    break;
                case 'h': case 'H':
                    this.toggleHelp();
                    break;
            }
        });
    }

    setupTouchGestures() {
        const canvas = document.getElementById('glcanvas');
        if (!canvas) return;

        let lastTouchDistance = 0;
        let lastTouchCenter = null;
        let lastTouchX = 0;
        let lastTouchY = 0;

        canvas.addEventListener('touchstart', (e) => {
            e.preventDefault();
            
            if (e.touches.length === 1) {
                lastTouchX = e.touches[0].clientX;
                lastTouchY = e.touches[0].clientY;
            } else if (e.touches.length === 2) {
                lastTouchDistance = this.getTouchDistance(e.touches);
                lastTouchCenter = this.getTouchCenter(e.touches);
            }
        }, { passive: false });

        canvas.addEventListener('touchmove', (e) => {
            e.preventDefault();

            if (e.touches.length === 1) {
                const deltaX = e.touches[0].clientX - lastTouchX;
                const deltaY = e.touches[0].clientY - lastTouchY;
                this.safeCall('demo_touch_rotate', deltaX * 0.5, deltaY * 0.5);
                lastTouchX = e.touches[0].clientX;
                lastTouchY = e.touches[0].clientY;
            } else if (e.touches.length === 2) {
                const currentDistance = this.getTouchDistance(e.touches);
                const currentCenter = this.getTouchCenter(e.touches);

                if (lastTouchDistance > 0) {
                    const delta = currentDistance - lastTouchDistance;
                    this.safeCall('demo_touch_zoom', delta * 0.1);
                }

                if (lastTouchCenter) {
                    const deltaX = currentCenter.x - lastTouchCenter.x;
                    const deltaY = currentCenter.y - lastTouchCenter.y;
                    this.safeCall('demo_touch_pan', deltaX * 0.5, deltaY * 0.5);
                }

                lastTouchDistance = currentDistance;
                lastTouchCenter = currentCenter;
            }
        }, { passive: false });

        canvas.addEventListener('touchend', (e) => {
            if (e.touches.length === 0) {
                lastTouchDistance = 0;
                lastTouchCenter = null;
            }
        });

        let lastTapTime = 0;
        canvas.addEventListener('touchend', (e) => {
            const currentTime = new Date().getTime();
            const tapLength = currentTime - lastTapTime;
            
            if (tapLength < 300 && tapLength > 0) {
                e.preventDefault();
                this.safeCall('demo_double_tap');
            }
            
            lastTapTime = currentTime;
        });
    }

    getTouchDistance(touches) {
        const dx = touches[0].clientX - touches[1].clientX;
        const dy = touches[0].clientY - touches[1].clientY;
        return Math.sqrt(dx * dx + dy * dy);
    }

    getTouchCenter(touches) {
        return {
            x: (touches[0].clientX + touches[1].clientX) / 2,
            y: (touches[0].clientY + touches[1].clientY) / 2
        };
    }

    /**
     * Load WASM module with robust error handling
     * Implements streaming fallback pattern from Wasm best practices
     */
    async loadWasmModule() {
        this.updateLoadingStatus('加载 Phoenix Engine WASM 模块...');
        this.updateLoadingProgress(10);
        
        const wasmPath = 'phoenix-engine.wasm';
        
        try {
            // Step 1: Check if file exists and get MIME type
            this.updateLoadingStatus('检查 WASM 文件...');
            this.updateLoadingProgress(20);
            
            const headResponse = await fetch(wasmPath, { 
                method: 'HEAD',
                credentials: 'same-origin'
            });
            
            if (!headResponse.ok) {
                throw new Error(`WASM 文件不存在 (HTTP ${headResponse.status})`);
            }
            
            const contentType = headResponse.headers.get('content-type');
            this.log(`WASM Content-Type: ${contentType}`);
            
            if (contentType && !contentType.includes('wasm') && !contentType.includes('octet-stream')) {
                this.error(`Warning: Unexpected MIME type: ${contentType}`);
            }
            
            // Step 2: Try streaming instantiation first (best performance)
            this.updateLoadingStatus('流式加载 WASM...');
            this.updateLoadingProgress(30);
            
            try {
                await this.loadWithStreaming(wasmPath);
                this.log('✓ Loaded with streaming');
                return;
            } catch (streamError) {
                this.log('Streaming failed, falling back to non-streaming:', streamError.message);
            }
            
            // Step 3: Fallback to non-streaming
            this.updateLoadingStatus('备用加载方式...');
            this.updateLoadingProgress(40);
            await this.loadWithFetch(wasmPath);
            this.log('✓ Loaded with fetch fallback');
            
        } catch (error) {
            this.error('WASM load failed:', error);
            throw error;
        }
    }

    /**
     * Load using instantiateStreaming (preferred method)
     */
    async loadWithStreaming(wasmPath) {
        const response = await fetch(wasmPath, {
            credentials: 'same-origin',
            headers: {
                'Accept': 'application/wasm'
            }
        });

        if (!response.ok) {
            throw new Error(`Streaming fetch failed (HTTP ${response.status})`);
        }

        // Create import object
        const imports = this.createImportObject();
        
        // Use instantiateStreaming
        const { instance, module } = await WebAssembly.instantiateStreaming(
            response,
            imports
        );
        
        this.instance = instance;
        this.module = module;
    }

    /**
     * Load using fetch + instantiate (fallback method)
     */
    async loadWithFetch(wasmPath) {
        const response = await fetch(wasmPath, {
            credentials: 'same-origin',
            headers: {
                'Accept': 'application/wasm'
            }
        });

        if (!response.ok) {
            throw new Error(`Fetch failed (HTTP ${response.status})`);
        }

        // Read with progress
        const contentLength = response.headers.get('content-length');
        const total = parseInt(contentLength, 10) || 0;
        
        const reader = response.body.getReader();
        const chunks = [];
        let loaded = 0;
        
        while (true) {
            const { done, value } = await reader.read();
            if (done) break;
            
            chunks.push(value);
            loaded += value.length;
            
            if (total > 0) {
                const progress = 40 + (loaded / total) * 40;
                this.updateLoadingProgress(progress);
                this.updateLoadingStatus(`下载中：${this.formatBytes(loaded)} / ${this.formatBytes(total)}`);
            }
        }
        
        // Concatenate chunks
        const wasmBytes = new Uint8Array(loaded);
        let position = 0;
        for (const chunk of chunks) {
            wasmBytes.set(chunk, position);
            position += chunk.length;
        }
        
        this.updateLoadingStatus('编译 WASM 模块...');
        this.updateLoadingProgress(85);
        
        // Compile
        const wasmModule = await WebAssembly.compile(wasmBytes);
        
        this.updateLoadingStatus('实例化 WASM...');
        this.updateLoadingProgress(95);
        
        // Instantiate
        const imports = this.createImportObject();
        const instance = await WebAssembly.instantiate(wasmModule, imports);
        
        this.instance = instance;
        this.module = wasmModule;
    }

    /**
     * Create import object with all required functions
     * Follows Wasm import/export best practices
     */
    createImportObject() {
        const memory = new WebAssembly.Memory({ 
            initial: 256, 
            maximum: 512,
            shared: false 
        });
        
        const table = new WebAssembly.Table({ 
            initial: 0, 
            element: 'anyfunc' 
        });
        
        return {
            env: {
                memory: memory,
                table: table,
                
                // Time functions
                _emscripten_get_now: () => performance.now(),
                _clock_gettime: (clk_id, tp) => {
                    const now = performance.now();
                    const sec = Math.floor(now / 1000);
                    const nsec = (now % 1000) * 1000000;
                    
                    try {
                        const view = new DataView(this.instance.exports.memory.buffer);
                        view.setBigInt64(0, BigInt(sec) * 1000000000n + BigInt(nsec), true);
                        return 0;
                    } catch (e) {
                        return -1;
                    }
                },
                
                // Console logging
                _console_log: (ptr, len) => {
                    const msg = this.readWasmString(ptr, len);
                    this.log('[WASM]', msg);
                },
                _console_error: (ptr, len) => {
                    const msg = this.readWasmString(ptr, len);
                    this.error('[WASM]', msg);
                },
                
                // Abort handler
                _abort: (msg) => {
                    const errorMsg = msg ? this.readWasmString(msg) : 'Unknown abort';
                    this.error('WASM abort:', errorMsg);
                    throw new Error(`WASM abort: ${errorMsg}`);
                },
                
                // Exit handler
                _exit: (code) => {
                    this.log('WASM exit:', code);
                },
                
                // File system (if needed)
                _emscripten_date_now: () => Date.now(),
                
                // Memory management
                _emscripten_memcpy_big: (dest, src, num) => {
                    const heap = new Uint8Array(memory.buffer);
                    heap.set(heap.subarray(src, src + num), dest);
                },
            },
            
            // Additional modules if needed
            wasi_snapshot_preview1: {
                proc_exit: (code) => {
                    this.log('WASI exit:', code);
                }
            }
        };
    }

    /**
     * Safely read string from WASM memory
     * Includes bounds checking to prevent crashes
     */
    readWasmString(ptr, len) {
        if (!this.instance?.exports?.memory) return '';
        
        try {
            const memory = this.instance.exports.memory.buffer;
            const bytes = new Uint8Array(memory, ptr, len);
            let str = '';
            for (let i = 0; i < len && i < bytes.length; i++) {
                const char = bytes[i];
                if (char === 0) break;
                str += String.fromCharCode(char);
            }
            return str;
        } catch (e) {
            this.error('Failed to read WASM string:', e);
            return '';
        }
    }

    /**
     * Safely call WASM function with error handling
     */
    safeCall(funcName, ...args) {
        if (!this.instance?.exports?.[funcName]) {
            this.log(`Function not exported: ${funcName}`);
            return null;
        }
        
        try {
            return this.instance.exports[funcName](...args);
        } catch (e) {
            this.error(`Error calling ${funcName}:`, e);
            return null;
        }
    }

    /**
     * Get memory view with cache invalidation on growth
     * Prevents use-after-free bugs
     */
    getMemoryView(type = 'Uint8Array', offset = 0, length) {
        if (!this.instance?.exports?.memory) return null;
        
        const memory = this.instance.exports.memory;
        const key = `${type}:${offset}:${length || 'all'}`;
        
        // Check if memory has grown since last access
        const currentBuffer = memory.buffer;
        const cached = this.memoryViews.get(key);
        
        if (cached && cached.buffer === currentBuffer) {
            return cached.view;
        }
        
        // Create new view
        let view;
        switch(type) {
            case 'Uint8Array': view = new Uint8Array(currentBuffer, offset, length || currentBuffer.byteLength - offset); break;
            case 'Int8Array': view = new Int8Array(currentBuffer, offset, length || currentBuffer.byteLength - offset); break;
            case 'Uint16Array': view = new Uint16Array(currentBuffer, offset, length || (currentBuffer.byteLength - offset) / 2); break;
            case 'Int16Array': view = new Int16Array(currentBuffer, offset, length || (currentBuffer.byteLength - offset) / 2); break;
            case 'Uint32Array': view = new Uint32Array(currentBuffer, offset, length || (currentBuffer.byteLength - offset) / 4); break;
            case 'Int32Array': view = new Int32Array(currentBuffer, offset, length || (currentBuffer.byteLength - offset) / 4); break;
            case 'Float32Array': view = new Float32Array(currentBuffer, offset, length || (currentBuffer.byteLength - offset) / 4); break;
            case 'Float64Array': view = new Float64Array(currentBuffer, offset, length || (currentBuffer.byteLength - offset) / 8); break;
            default: view = new Uint8Array(currentBuffer, offset, length || currentBuffer.byteLength - offset);
        }
        
        this.memoryViews.set(key, { buffer: currentBuffer, view });
        return view;
    }

    async initializeGL() {
        this.canvas = document.getElementById('glcanvas');
        if (!this.canvas) {
            throw new Error('Canvas 元素未找到');
        }

        // Handle high DPI displays
        const dpr = window.devicePixelRatio || 1;
        const rect = this.canvas.getBoundingClientRect();
        
        this.canvas.width = Math.floor(rect.width * dpr);
        this.canvas.height = Math.floor(rect.height * dpr);
        
        this.log(`Canvas size: ${this.canvas.width}x${this.canvas.height} (DPR: ${dpr})`);
        
        // Initialize WASM demo
        const result = this.safeCall('demo_init', this.canvas.width, this.canvas.height);
        
        if (result !== 0 && result !== undefined) {
            let errorMsg = `初始化失败 (错误码：${result})`;
            if (result === -1) errorMsg = '引擎已初始化';
            if (result === -2) errorMsg = '渲染后端初始化失败';
            if (result === -3) errorMsg = 'WebGL 上下文创建失败';
            throw new Error(errorMsg);
        }

        window.addEventListener('resize', () => this.handleResize());
    }

    handleResize() {
        const dpr = window.devicePixelRatio || 1;
        const rect = this.canvas.getBoundingClientRect();
        
        this.canvas.width = Math.floor(rect.width * dpr);
        this.canvas.height = Math.floor(rect.height * dpr);
        
        this.safeCall('demo_resize', this.canvas.width, this.canvas.height);
        this.log('Canvas resized:', this.canvas.width, 'x', this.canvas.height);
    }

    startRenderLoop() {
        let lastTime = performance.now();
        let frameCount = 0;
        let fpsUpdateTime = lastTime;

        const render = (currentTime) => {
            if (!this.isLoaded) {
                requestAnimationFrame(render);
                return;
            }
            
            const deltaTime = (currentTime - lastTime) / 1000;
            lastTime = currentTime;

            this.frameTime = deltaTime * 1000;

            frameCount++;
            if (currentTime - fpsUpdateTime >= 1000) {
                this.fps = frameCount;
                frameCount = 0;
                fpsUpdateTime = currentTime;
                this.updatePerformanceHUD();
            }

            this.safeCall('demo_update', deltaTime);
            this.safeCall('demo_render');

            requestAnimationFrame(render);
        };

        requestAnimationFrame(render);
    }

    startMemoryMonitoring() {
        setInterval(() => {
            if (performance.memory) {
                this.memoryUsage = performance.memory.usedJSHeapSize / (1024 * 1024);
            }
            
            // Check WASM memory
            if (this.instance?.exports?.memory) {
                const wasmMemory = this.instance.exports.memory.buffer.byteLength / (1024 * 1024);
                this.log(`Memory - JS: ${this.memoryUsage.toFixed(1)}MB, WASM: ${wasmMemory.toFixed(1)}MB`);
            }
        }, 5000);
    }

    updateLoadingProgress(progress) {
        const clamped = Math.min(100, Math.max(0, progress));
        if (this.ui.loadingProgress) {
            this.ui.loadingProgress.style.width = `${clamped}%`;
        }
    }

    updateLoadingStatus(status) {
        if (this.ui.loadingStatus) {
            this.ui.loadingStatus.textContent = status;
        }
    }

    hideLoading() {
        if (this.ui.loadingOverlay) {
            this.ui.loadingOverlay.classList.add('hidden');
            setTimeout(() => {
                this.ui.loadingOverlay.style.display = 'none';
            }, 500);
        }
    }

    showError(message) {
        this.error('Showing error:', message);
        
        if (this.ui.errorMessage) {
            this.ui.errorMessage.textContent = message;
            this.ui.errorMessage.style.whiteSpace = 'pre-line';
        }
        if (this.ui.errorOverlay) {
            this.ui.errorOverlay.classList.remove('hidden');
        }
        
        if (this.ui.loadingStatus) {
            this.ui.loadingStatus.textContent = '❌ 加载失败';
            this.ui.loadingStatus.style.color = '#ef4444';
        }
    }

    updatePerformanceHUD() {
        if (this.ui.fpsValue) {
            this.ui.fpsValue.textContent = this.fps;
            this.ui.fpsValue.style.color = this.fps >= 50 ? 'var(--success-color)' : 
                                            this.fps >= 30 ? 'var(--warning-color)' : 'var(--error-color)';
        }
        if (this.ui.frameTimeValue) {
            this.ui.frameTimeValue.textContent = this.frameTime.toFixed(1);
        }
        if (this.ui.drawCallsValue) {
            this.ui.drawCallsValue.textContent = this.drawCalls;
        }
        if (this.ui.trianglesValue) {
            this.ui.trianglesValue.textContent = (this.triangleCount / 1000).toFixed(1);
        }
        if (this.ui.memoryValue) {
            this.ui.memoryValue.textContent = this.memoryUsage.toFixed(0);
        }
    }

    togglePerformanceHUD() {
        const hud = document.getElementById('performance-hud');
        if (hud) {
            hud.classList.toggle('collapsed');
        }
    }

    toggleHelp() {
        alert(`Phoenix Engine Controls:

WASD - 旋转相机
+/- - 缩放
R - 重置视角
P - 切换性能面板
H - 显示帮助

触摸:
单指滑动 - 旋转
双指捏合 - 缩放
双指拖动 - 平移
双击 - 重置`);
    }

    formatBytes(bytes) {
        if (bytes === 0) return '0 B';
        const k = 1024;
        const sizes = ['B', 'KB', 'MB', 'GB'];
        const i = Math.floor(Math.log(bytes) / Math.log(k));
        return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
    }

    // Control update methods
    updateCameraMode(mode) {
        this.log('Camera mode:', mode);
        const modes = { orbit: 0, fps: 1, third: 2 };
        this.safeCall('demo_set_camera_mode', modes[mode] || 0);
    }

    updateMaterialParam(param, value) {
        this.log('Material param:', param, value);
        const params = { metallic: 0, roughness: 1, clearcoat: 2 };
        this.safeCall('demo_set_material_param', params[param], value);
    }

    updateEffect(effect, enabled) {
        this.log('Effect:', effect, enabled);
        const effects = {
            directionalLight: 0, pointLight: 1, spotLight: 2, shadows: 3,
            bloom: 4, toneMapping: 5, ssao: 6
        };
        this.safeCall('demo_set_effect', effects[effect], enabled ? 1 : 0);
    }

    updateAnimation(anim) {
        this.log('Animation:', anim);
        const anims = { idle: 0, walk: 1, run: 2, jump: 3 };
        this.safeCall('demo_set_animation', anims[anim] || 0);
    }
}

// Initialize on DOM ready
document.addEventListener('DOMContentLoaded', () => {
    console.log('[PhoenixWasmLoader] DOM ready');
    const loader = new PhoenixWasmLoader();
    loader.init();
    
    // Expose for debugging
    window.phoenixLoader = loader;
    
    // Debug commands
    console.log(`
🔧 Phoenix Engine Debug Commands:
  phoenixLoader.log() - Toggle debug logging
  phoenixLoader.instance - Access WASM instance
  phoenixLoader.module - Access WASM module
  phoenixLoader.safeCall('function', args) - Safe WASM call
  performance.memory - Check JS heap (Chrome only)
`);
});

// Service Worker registration
if ('serviceWorker' in navigator) {
    window.addEventListener('load', () => {
        navigator.serviceWorker.register('sw.js')
            .then(reg => console.log('[SW] Registered:', reg.scope))
            .catch(err => console.log('[SW] Registration failed:', err));
    });
}
