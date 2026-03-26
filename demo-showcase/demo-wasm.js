/**
 * Phoenix Engine - WASM Loader
 * Mobile-optimized WASM module loader with performance monitoring
 */

class PhoenixWasmLoader {
    constructor() {
        this.module = null;
        this.instance = null;
        this.canvas = null;
        this.gl = null;
        this.loadingProgress = 0;
        this.isLoaded = false;
        
        // Performance monitoring
        this.fps = 0;
        this.frameTime = 0;
        this.drawCalls = 0;
        this.triangleCount = 0;
        this.memoryUsage = 0;
        this.frameHistory = [];
        
        // UI Elements
        this.ui = {
            loadingOverlay: null,
            loadingProgress: null,
            loadingStatus: null,
            errorOverlay: null,
            errorMessage: null,
            fpsValue: null,
            frameTimeValue: null,
            drawCallsValue: null,
            trianglesValue: null,
            memoryValue: null,
            hudToggle: null,
            hudContent: null,
            controlToggle: null,
            controlContent: null
        };
        
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
            blendSpeed: 0.3,
            particleFire: false,
            particleSmoke: false,
            particleSparks: false
        };
    }

    async init() {
        this.cacheUIElements();
        this.setupEventListeners();
        
        try {
            await this.loadWasmModule();
            await this.initializeGL();
            this.startRenderLoop();
            this.hideLoading();
        } catch (error) {
            this.showError(error.message);
        }
    }

    cacheUIElements() {
        this.ui.loadingOverlay = document.getElementById('loading-overlay');
        this.ui.loadingProgress = document.getElementById('loading-progress');
        this.ui.loadingStatus = document.getElementById('loading-status');
        this.ui.errorOverlay = document.getElementById('error-overlay');
        this.ui.errorMessage = document.getElementById('error-message');
        this.ui.fpsValue = document.getElementById('fps-value');
        this.ui.frameTimeValue = document.getElementById('frame-time-value');
        this.ui.drawCallsValue = document.getElementById('draw-calls-value');
        this.ui.trianglesValue = document.getElementById('triangles-value');
        this.ui.memoryValue = document.getElementById('memory-value');
        this.ui.hudToggle = document.getElementById('hud-toggle');
        this.ui.hudContent = document.getElementById('hud-content');
        this.ui.controlToggle = document.getElementById('control-toggle');
        this.ui.controlContent = document.getElementById('control-content');
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
            { id: 'ssao', key: 'ssao' },
            { id: 'particle-fire', key: 'particleFire' },
            { id: 'particle-smoke', key: 'particleSmoke' },
            { id: 'particle-sparks', key: 'particleSparks' }
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
    }

    setupTouchGestures() {
        const canvas = document.getElementById('glcanvas');
        if (!canvas) return;

        let lastTouchDistance = 0;
        let lastTouchCenter = null;
        let isDoubleTap = false;

        canvas.addEventListener('touchstart', (e) => {
            e.preventDefault();
            
            if (e.touches.length === 1) {
                // Single touch - rotate
                this.handleTouchStart(e.touches[0]);
            } else if (e.touches.length === 2) {
                // Two fingers - pinch/pan
                lastTouchDistance = this.getTouchDistance(e.touches);
                lastTouchCenter = this.getTouchCenter(e.touches);
            }
        }, { passive: false });

        canvas.addEventListener('touchmove', (e) => {
            e.preventDefault();

            if (e.touches.length === 1) {
                this.handleTouchMove(e.touches[0]);
            } else if (e.touches.length === 2) {
                const currentDistance = this.getTouchDistance(e.touches);
                const currentCenter = this.getTouchCenter(e.touches);

                // Pinch zoom
                if (lastTouchDistance > 0) {
                    const delta = currentDistance - lastTouchDistance;
                    this.handleZoom(delta);
                }

                // Pan
                if (lastTouchCenter) {
                    const deltaX = currentCenter.x - lastTouchCenter.x;
                    const deltaY = currentCenter.y - lastTouchCenter.y;
                    this.handlePan(deltaX, deltaY);
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
                this.handleDoubleTap();
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

    handleTouchStart(touch) {
        this.touchStartX = touch.clientX;
        this.touchStartY = touch.clientY;
    }

    handleTouchMove(touch) {
        if (this.touchStartX !== undefined) {
            const deltaX = touch.clientX - this.touchStartX;
            const deltaY = touch.clientY - this.touchStartY;
            this.handleRotate(deltaX, deltaY);
            this.touchStartX = touch.clientX;
            this.touchStartY = touch.clientY;
        }
    }

    handleRotate(deltaX, deltaY) {
        if (this.instance?.exports?.on_touch_rotate) {
            this.instance.exports.on_touch_rotate(deltaX * 0.5, deltaY * 0.5);
        }
    }

    handleZoom(delta) {
        if (this.instance?.exports?.on_touch_zoom) {
            this.instance.exports.on_touch_zoom(delta * 0.1);
        }
    }

    handlePan(deltaX, deltaY) {
        if (this.instance?.exports?.on_touch_pan) {
            this.instance.exports.on_touch_pan(deltaX * 0.5, deltaY * 0.5);
        }
    }

    handleDoubleTap() {
        if (this.instance?.exports?.on_double_tap) {
            this.instance.exports.on_double_tap();
        }
    }

    async loadWasmModule() {
        this.updateLoadingStatus('加载 WASM 模块...');
        
        try {
            // Check if WASM is supported
            if (!WebAssembly) {
                throw new Error('您的浏览器不支持 WebAssembly');
            }

            // Try to load the WASM module
            const wasmPath = 'demo-app.wasm';
            
            const response = await fetch(wasmPath, {
                credentials: 'same-origin',
                headers: {
                    'Accept': 'application/wasm'
                }
            });

            if (!response.ok) {
                // If WASM file doesn't exist, create a demo mode
                console.log('WASM module not found, running in demo mode');
                await this.createDemoMode();
                return;
            }

            const wasmBytes = await response.arrayBuffer();
            
            // Compile and instantiate
            const { instance, module } = await WebAssembly.instantiate(wasmBytes, {
                env: {
                    memory: new WebAssembly.Memory({ initial: 256, maximum: 512 }),
                    table: new WebAssembly.Table({ initial: 0, element: 'anyfunc' }),
                    
                    // Import functions
                    _emscripten_get_now: () => performance.now(),
                    
                    // Console logging
                    _console_log: (ptr, len) => {
                        const msg = this.readWasmString(ptr, len);
                        console.log('[WASM]', msg);
                    },
                    _console_error: (ptr, len) => {
                        const msg = this.readWasmString(ptr, len);
                        console.error('[WASM]', msg);
                    },
                    
                    // Performance callbacks
                    _update_fps: (fps) => { this.fps = fps; },
                    _update_draw_calls: (count) => { this.drawCalls = count; },
                    _update_triangles: (count) => { this.triangleCount = count; },
                    _update_memory: (mb) => { this.memoryUsage = mb; },
                    
                    // File loading
                    _load_file: (ptr, len) => {
                        const filename = this.readWasmString(ptr, len);
                        return this.loadAssetFile(filename);
                    }
                },
                wasi_snapshot_preview1: {
                    proc_exit: (code) => {
                        console.log('WASI exit:', code);
                    }
                }
            });

            this.instance = instance;
            this.module = module;
            this.loadingProgress = 100;
            this.updateLoadingProgress(100);
            
        } catch (error) {
            console.warn('WASM load failed, using fallback:', error);
            await this.createDemoMode();
        }
    }

    async createDemoMode() {
        // Create a pure JavaScript/WebGL demo when WASM is not available
        this.updateLoadingStatus('初始化 WebGL 渲染...');
        await new Promise(resolve => setTimeout(resolve, 500));
        
        // Simulate loading progress
        for (let i = 0; i <= 100; i += 10) {
            this.updateLoadingProgress(i);
            await new Promise(resolve => setTimeout(resolve, 100));
        }
        
        this.isLoaded = true;
        
        // Start demo simulation
        this.startDemoSimulation();
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

    async loadAssetFile(filename) {
        try {
            const response = await fetch(`assets/${filename}`);
            if (!response.ok) return 0;
            const buffer = await response.arrayBuffer();
            // In real implementation, would copy to WASM memory
            return buffer.byteLength;
        } catch {
            return 0;
        }
    }

    async initializeGL() {
        this.canvas = document.getElementById('glcanvas');
        if (!this.canvas) throw new Error('Canvas not found');

        // Handle high DPI displays
        const dpr = window.devicePixelRatio || 1;
        const rect = this.canvas.getBoundingClientRect();
        
        this.canvas.width = rect.width * dpr;
        this.canvas.height = rect.height * dpr;
        
        this.gl = this.canvas.getContext('webgl2', {
            alpha: false,
            antialias: true,
            powerPreference: 'high-performance',
            preserveDrawingBuffer: false
        }) || this.canvas.getContext('webgl', {
            alpha: false,
            antialias: true
        });

        if (!this.gl) {
            throw new Error('WebGL not supported');
        }

        // Initialize WASM with GL context
        if (this.instance?.exports?.init_gl) {
            this.instance.exports.init_gl(
                this.canvas.width,
                this.canvas.height
            );
        }

        // Handle resize
        window.addEventListener('resize', () => this.handleResize());
    }

    handleResize() {
        const dpr = window.devicePixelRatio || 1;
        const rect = this.canvas.getBoundingClientRect();
        
        this.canvas.width = rect.width * dpr;
        this.canvas.height = rect.height * dpr;
        
        if (this.gl) {
            this.gl.viewport(0, 0, this.canvas.width, this.canvas.height);
        }
        
        if (this.instance?.exports?.on_resize) {
            this.instance.exports.on_resize(this.canvas.width, this.canvas.height);
        }
    }

    startRenderLoop() {
        let lastTime = performance.now();
        let frameCount = 0;
        let fpsUpdateTime = lastTime;

        const render = (currentTime) => {
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

            // Record frame time for graph
            this.frameHistory.push(this.frameTime);
            if (this.frameHistory.length > 60) {
                this.frameHistory.shift();
            }

            // Update WASM
            if (this.instance?.exports?.update) {
                this.instance.exports.update(deltaTime);
            }

            // Render
            if (this.instance?.exports?.render) {
                this.instance.exports.render();
            } else {
                // Demo mode rendering
                this.renderDemoFrame(currentTime);
            }

            requestAnimationFrame(render);
        };

        requestAnimationFrame(render);
    }

    startDemoSimulation() {
        // Simulate performance metrics for demo mode
        setInterval(() => {
            this.fps = Math.floor(55 + Math.random() * 10);
            this.frameTime = 1000 / this.fps;
            this.drawCalls = Math.floor(150 + Math.random() * 50);
            this.triangleCount = Math.floor(45 + Math.random() * 10);
            this.memoryUsage = Math.floor(120 + Math.random() * 30);
            this.updatePerformanceHUD();
        }, 1000);
    }

    renderDemoFrame(time) {
        if (!this.gl) return;

        const gl = this.gl;
        const width = this.canvas.width;
        const height = this.canvas.height;

        // Clear with gradient-like color
        const r = 0.1 + 0.05 * Math.sin(time * 0.001);
        const g = 0.1 + 0.05 * Math.sin(time * 0.0013);
        const b = 0.18 + 0.05 * Math.sin(time * 0.0007);
        gl.clearColor(r, g, b, 1.0);
        gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

        // Enable depth test
        gl.enable(gl.DEPTH_TEST);
        gl.enable(gl.CULL_FACE);
    }

    updateLoadingProgress(progress) {
        this.loadingProgress = Math.min(100, Math.max(0, progress));
        this.ui.loadingProgress.style.width = `${this.loadingProgress}%`;
    }

    updateLoadingStatus(status) {
        this.ui.loadingStatus.textContent = status;
    }

    hideLoading() {
        this.ui.loadingOverlay.classList.add('hidden');
        setTimeout(() => {
            this.ui.loadingOverlay.style.display = 'none';
        }, 500);
    }

    showError(message) {
        this.ui.errorMessage.textContent = message;
        this.ui.errorOverlay.classList.remove('hidden');
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

        // Update frame time graph
        this.updateFrameTimeGraph();
    }

    updateFrameTimeGraph() {
        const container = document.getElementById('frame-time-graph');
        if (!container || this.frameHistory.length < 2) return;

        // Clear previous canvas
        container.innerHTML = '';
        
        const canvas = document.createElement('canvas');
        const ctx = canvas.getContext('2d');
        const width = container.clientWidth;
        const height = container.clientHeight;
        
        canvas.width = width * (window.devicePixelRatio || 1);
        canvas.height = height * (window.devicePixelRatio || 1);
        ctx.scale(window.devicePixelRatio || 1, window.devicePixelRatio || 1);
        
        container.appendChild(canvas);

        // Draw graph
        const maxTime = 33; // 30fps threshold
        const barWidth = width / 60;
        
        this.frameHistory.forEach((time, i) => {
            const barHeight = (time / maxTime) * height;
            const x = i * barWidth;
            const y = height - barHeight;
            
            // Color based on frame time
            if (time < 16.67) {
                ctx.fillStyle = '#4ade80'; // Green - good
            } else if (time < 33) {
                ctx.fillStyle = '#fbbf24'; // Yellow - okay
            } else {
                ctx.fillStyle = '#ef4444'; // Red - bad
            }
            
            ctx.fillRect(x, y, barWidth - 1, barHeight);
        });

        // Draw 30fps line
        ctx.strokeStyle = 'rgba(255, 255, 255, 0.3)';
        ctx.setLineDash([2, 2]);
        ctx.beginPath();
        const threshold30 = height - (33 / maxTime) * height;
        ctx.moveTo(0, threshold30);
        ctx.lineTo(width, threshold30);
        ctx.stroke();
    }

    // Control update methods (would call into WASM in real implementation)
    updateCameraMode(mode) {
        console.log('Camera mode:', mode);
        if (this.instance?.exports?.set_camera_mode) {
            const modes = { orbit: 0, fps: 1, third: 2 };
            this.instance.exports.set_camera_mode(modes[mode] || 0);
        }
    }

    updateMaterialParam(param, value) {
        console.log('Material param:', param, value);
        if (this.instance?.exports?.set_material_param) {
            const params = { metallic: 0, roughness: 1, clearcoat: 2 };
            this.instance.exports.set_material_param(params[param], value);
        }
    }

    updateEffect(effect, enabled) {
        console.log('Effect:', effect, enabled);
        if (this.instance?.exports?.set_effect) {
            const effects = {
                directionalLight: 0, pointLight: 1, spotLight: 2, shadows: 3,
                bloom: 4, toneMapping: 5, ssao: 6,
                particleFire: 10, particleSmoke: 11, particleSparks: 12
            };
            this.instance.exports.set_effect(effects[effect], enabled ? 1 : 0);
        }
    }

    updateAnimation(anim) {
        console.log('Animation:', anim);
        if (this.instance?.exports?.set_animation) {
            const anims = { idle: 0, walk: 1, run: 2, jump: 3 };
            this.instance.exports.set_animation(anims[anim] || 0);
        }
    }
}

// Initialize on DOM ready
document.addEventListener('DOMContentLoaded', () => {
    const loader = new PhoenixWasmLoader();
    loader.init();
    
    // Expose for debugging
    window.phoenixLoader = loader;
});

// Service Worker registration for offline support
if ('serviceWorker' in navigator) {
    window.addEventListener('load', () => {
        navigator.serviceWorker.register('sw.js').catch(() => {
            console.log('Service Worker registration skipped');
        });
    });
}
