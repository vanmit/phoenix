/**
 * Phoenix Engine - WASM Loader
 * 
 * Refactored to use Phoenix Engine API with bgfx backend
 * Includes improved error handling and clear failure messages
 */

class PhoenixWasmLoader {
    constructor() {
        this.module = null;
        this.instance = null;
        this.canvas = null;
        this.isLoaded = false;
        this.isWasmSupported = true;
        
        // Performance monitoring
        this.fps = 0;
        this.frameTime = 0;
        this.drawCalls = 0;
        this.triangleCount = 0;
        this.memoryUsage = 0;
        
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
    }

    async init() {
        console.log('[PhoenixWasmLoader] Initializing...');
        
        try {
            this.cacheUIElements();
            this.setupEventListeners();
            
            // Check WASM support first
            if (!this.checkWasmSupport()) {
                throw new Error('您的浏览器不支持 WebAssembly。请使用现代浏览器 (Chrome 90+, Firefox 88+, Safari 15+, Edge 90+)');
            }
            
            // Load WASM module
            await this.loadWasmModule();
            
            // Initialize GL/bgfx
            await this.initializeGL();
            
            // Start render loop
            this.startRenderLoop();
            
            // Hide loading overlay
            this.hideLoading();
            
            this.isLoaded = true;
            console.log('[PhoenixWasmLoader] Initialization complete');
            
        } catch (error) {
            console.error('[PhoenixWasmLoader] Initialization failed:', error);
            this.showError(this.getFriendlyErrorMessage(error));
        }
    }

    /**
     * Check if WebAssembly is supported
     */
    checkWasmSupport() {
        if (typeof WebAssembly !== 'object') {
            this.isWasmSupported = false;
            return false;
        }
        
        // Test basic WASM functionality
        try {
            const bin = new Uint8Array([
                0x00, 0x61, 0x73, 0x6d, // WASM magic number
                0x01, 0x00, 0x00, 0x00  // Version 1
            ]);
            new WebAssembly.Module(bin);
            return true;
        } catch (e) {
            this.isWasmSupported = false;
            return false;
        }
    }

    /**
     * Get user-friendly error message
     */
    getFriendlyErrorMessage(error) {
        const msg = error.message.toLowerCase();
        
        if (msg.includes('webassembly') || msg.includes('wasm')) {
            return 'WebAssembly 不支持：您的浏览器可能过旧或不支持 WASM。请升级到最新版本的 Chrome、Firefox、Safari 或 Edge。';
        }
        
        if (msg.includes('fetch') || msg.includes('network') || msg.includes('404')) {
            return 'WASM 文件加载失败：无法找到 phoenix-engine.wasm 文件。请确保文件存在于正确位置，或检查网络连接。';
        }
        
        if (msg.includes('webgl') || msg.includes('context')) {
            return 'WebGL 初始化失败：您的浏览器或显卡驱动可能不支持 WebGL 2.0。请更新显卡驱动或尝试其他浏览器。';
        }
        
        if (msg.includes('memory') || msg.includes('heap')) {
            return '内存不足：WASM 模块需要更多内存。请关闭其他标签页或重启浏览器。';
        }
        
        if (msg.includes('instantiate') || msg.includes('compile')) {
            return 'WASM 编译失败：WASM 文件可能已损坏或不兼容。请刷新页面重试或清除浏览器缓存。';
        }
        
        return `初始化失败：${error.message}。请刷新页面重试，如果问题持续，请检查浏览器控制台获取详细信息。`;
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
            drawCallsValue: document.getElementById('draw-calls-value'),
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
            this.init();
        });

        // Gesture hints fade out
        setTimeout(() => {
            document.querySelectorAll('.hint').forEach(hint => {
                hint.classList.add('fade-out');
            });
        }, 8000);

        // Touch gesture handling
        this.setupTouchGestures();
        
        // Keyboard controls
        this.setupKeyboardControls();
    }
    
    setupKeyboardControls() {
        document.addEventListener('keydown', (e) => {
            if (!this.isLoaded || !this.instance) return;
            
            switch(e.key) {
                case 'w': case 'W':
                    this.instance.exports.demo_touch_rotate?.(0, -1);
                    break;
                case 's': case 'S':
                    this.instance.exports.demo_touch_rotate?.(0, 1);
                    break;
                case 'a': case 'A':
                    this.instance.exports.demo_touch_rotate?.(-1, 0);
                    break;
                case 'd': case 'D':
                    this.instance.exports.demo_touch_rotate?.(1, 0);
                    break;
                case '=': case '+':
                    this.instance.exports.demo_touch_zoom?.(-0.5);
                    break;
                case '-': case '_':
                    this.instance.exports.demo_touch_zoom?.(0.5);
                    break;
                case 'r': case 'R':
                    this.instance.exports.demo_double_tap?.();
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
                
                if (this.instance?.exports?.demo_touch_rotate) {
                    this.instance.exports.demo_touch_rotate(deltaX * 0.5, deltaY * 0.5);
                }
                
                lastTouchX = e.touches[0].clientX;
                lastTouchY = e.touches[0].clientY;
            } else if (e.touches.length === 2) {
                const currentDistance = this.getTouchDistance(e.touches);
                const currentCenter = this.getTouchCenter(e.touches);

                // Pinch zoom
                if (lastTouchDistance > 0) {
                    const delta = currentDistance - lastTouchDistance;
                    if (this.instance?.exports?.demo_touch_zoom) {
                        this.instance.exports.demo_touch_zoom(delta * 0.1);
                    }
                }

                // Pan
                if (lastTouchCenter) {
                    const deltaX = currentCenter.x - lastTouchCenter.x;
                    const deltaY = currentCenter.y - lastTouchCenter.y;
                    if (this.instance?.exports?.demo_touch_pan) {
                        this.instance.exports.demo_touch_pan(deltaX * 0.5, deltaY * 0.5);
                    }
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

        // Double tap detection
        let lastTapTime = 0;
        canvas.addEventListener('touchend', (e) => {
            const currentTime = new Date().getTime();
            const tapLength = currentTime - lastTapTime;
            
            if (tapLength < 300 && tapLength > 0) {
                e.preventDefault();
                if (this.instance?.exports?.demo_double_tap) {
                    this.instance.exports.demo_double_tap();
                }
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

    async loadWasmModule() {
        this.updateLoadingStatus('加载 Phoenix Engine WASM 模块...');
        this.updateLoadingProgress(10);
        
        const wasmPath = 'phoenix-engine.wasm';
        const jsPath = 'phoenix-engine.js';
        
        try {
            // Try to load the WASM module
            this.updateLoadingStatus('正在下载 WASM 文件...');
            this.updateLoadingProgress(30);
            
            const response = await fetch(wasmPath, {
                credentials: 'same-origin',
                headers: {
                    'Accept': 'application/wasm'
                }
            });

            if (!response.ok) {
                throw new Error(`WASM 文件加载失败 (HTTP ${response.status})`);
            }

            const contentLength = response.headers.get('content-length');
            const total = parseInt(contentLength, 10);
            let loaded = 0;

            // Create a streaming reader for progress
            const reader = response.body.getReader();
            const chunks = [];
            
            while (true) {
                const { done, value } = await reader.read();
                if (done) break;
                
                chunks.push(value);
                loaded += value.length;
                
                if (total > 0) {
                    const progress = 30 + (loaded / total) * 40;
                    this.updateLoadingProgress(progress);
                    this.updateLoadingStatus(`下载中：${Math.round(loaded / 1024)}KB / ${Math.round(total / 1024)}KB`);
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
            this.updateLoadingProgress(80);
            
            // Compile and instantiate
            const wasmModule = await WebAssembly.compile(wasmBytes);
            
            this.updateLoadingStatus('初始化渲染引擎...');
            this.updateLoadingProgress(90);
            
            // Get imports
            const imports = {
                env: {
                    memory: new WebAssembly.Memory({ initial: 256, maximum: 512 }),
                    table: new WebAssembly.Table({ initial: 0, element: 'anyfunc' }),
                    
                    // Emscripten imports
                    _emscripten_get_now: () => performance.now(),
                    
                    // Console logging
                    _console_log: (ptr, len) => {
                        const msg = this.readWasmString(ptr, len);
                        console.log('[Phoenix WASM]', msg);
                    },
                    _console_error: (ptr, len) => {
                        const msg = this.readWasmString(ptr, len);
                        console.error('[Phoenix WASM]', msg);
                    },
                    
                    // Time
                    _clock_gettime: () => 0,
                    
                    // Abort
                    _abort: () => {
                        console.error('WASM abort called');
                    }
                }
            };
            
            const instance = await WebAssembly.instantiate(wasmModule, imports);
            this.instance = instance;
            this.module = wasmModule;
            
            this.updateLoadingProgress(100);
            console.log('[PhoenixWasmLoader] WASM module loaded successfully');
            
        } catch (error) {
            console.error('[PhoenixWasmLoader] WASM load failed:', error);
            throw error;
        }
    }

    readWasmString(ptr, len) {
        if (!this.instance?.exports?.memory) return '';
        const memory = new Uint8Array(this.instance.exports.memory.buffer);
        let str = '';
        for (let i = 0; i < len; i++) {
            const char = memory[ptr + i];
            if (char === 0) break;
            str += String.fromCharCode(char);
        }
        return str;
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
        
        // Initialize WASM demo
        if (this.instance?.exports?.demo_init) {
            const result = this.instance.exports.demo_init(this.canvas.width, this.canvas.height);
            
            if (result !== 0) {
                let errorMsg = `初始化失败 (错误码：${result})`;
                if (result === -1) errorMsg = '引擎已初始化';
                if (result === -2) errorMsg = '渲染后端初始化失败';
                throw new Error(errorMsg);
            }
        } else {
            throw new Error('WASM 模块缺少 demo_init 导出函数');
        }

        // Handle resize
        window.addEventListener('resize', () => this.handleResize());
    }

    handleResize() {
        const dpr = window.devicePixelRatio || 1;
        const rect = this.canvas.getBoundingClientRect();
        
        this.canvas.width = Math.floor(rect.width * dpr);
        this.canvas.height = Math.floor(rect.height * dpr);
        
        if (this.instance?.exports?.demo_resize) {
            this.instance.exports.demo_resize(this.canvas.width, this.canvas.height);
        }
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

            // Update frame time
            this.frameTime = deltaTime * 1000;

            // FPS calculation
            frameCount++;
            if (currentTime - fpsUpdateTime >= 1000) {
                this.fps = frameCount;
                frameCount = 0;
                fpsUpdateTime = currentTime;
                this.updatePerformanceHUD();
            }

            // Update WASM
            if (this.instance?.exports?.demo_update) {
                this.instance.exports.demo_update(deltaTime);
            }

            // Render
            if (this.instance?.exports?.demo_render) {
                this.instance.exports.demo_render();
            }

            requestAnimationFrame(render);
        };

        requestAnimationFrame(render);
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
        console.error('[PhoenixWasmLoader] Error:', message);
        
        if (this.ui.errorMessage) {
            this.ui.errorMessage.textContent = message;
        }
        if (this.ui.errorOverlay) {
            this.ui.errorOverlay.classList.remove('hidden');
        }
        
        // Also show in loading status
        if (this.ui.loadingStatus) {
            this.ui.loadingStatus.textContent = '❌ ' + message;
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

    // Control update methods
    updateCameraMode(mode) {
        console.log('[PhoenixWasmLoader] Camera mode:', mode);
        if (this.instance?.exports?.demo_set_camera_mode) {
            const modes = { orbit: 0, fps: 1, third: 2 };
            this.instance.exports.demo_set_camera_mode(modes[mode] || 0);
        }
    }

    updateMaterialParam(param, value) {
        console.log('[PhoenixWasmLoader] Material param:', param, value);
        if (this.instance?.exports?.demo_set_material_param) {
            const params = { metallic: 0, roughness: 1, clearcoat: 2 };
            this.instance.exports.demo_set_material_param(params[param], value);
        }
    }

    updateEffect(effect, enabled) {
        console.log('[PhoenixWasmLoader] Effect:', effect, enabled);
        if (this.instance?.exports?.demo_set_effect) {
            const effects = {
                directionalLight: 0, pointLight: 1, spotLight: 2, shadows: 3,
                bloom: 4, toneMapping: 5, ssao: 6
            };
            this.instance.exports.demo_set_effect(effects[effect], enabled ? 1 : 0);
        }
    }

    updateAnimation(anim) {
        console.log('[PhoenixWasmLoader] Animation:', anim);
        if (this.instance?.exports?.demo_set_animation) {
            const anims = { idle: 0, walk: 1, run: 2, jump: 3 };
            this.instance.exports.demo_set_animation(anims[anim] || 0);
        }
    }
}

// Initialize on DOM ready
document.addEventListener('DOMContentLoaded', () => {
    console.log('[PhoenixWasmLoader] DOM ready, initializing...');
    const loader = new PhoenixWasmLoader();
    loader.init();
    
    // Expose for debugging
    window.phoenixLoader = loader;
});

// Service Worker registration for offline support
if ('serviceWorker' in navigator) {
    window.addEventListener('load', () => {
        navigator.serviceWorker.register('sw.js')
            .then(reg => console.log('[SW] Registered:', reg.scope))
            .catch(err => console.log('[SW] Registration failed:', err));
    });
}
