/**
 * Phoenix Engine WASM Player
 * Handles WebGL/WASM demo loading and playback
 */

class PhoenixPlayer {
  constructor(canvasId, options = {}) {
    this.canvas = document.getElementById(canvasId);
    this.ctx = null;
    this.engine = null;
    this.demoData = null;
    this.isRunning = false;
    this.options = {
      width: options.width || 1920,
      height: options.height || 1080,
      antialias: options.antialias !== false,
      alpha: options.alpha || false,
      preserveDrawingBuffer: options.preserveDrawingBuffer || false,
      ...options
    };
    this.stats = {
      fps: 0,
      frameCount: 0,
      lastTime: performance.now(),
      drawCalls: 0,
      vertices: 0,
      triangles: 0
    };
    this.init();
  }

  async init() {
    if (!this.canvas) {
      console.error('Canvas element not found');
      return;
    }

    try {
      this.ctx = this.canvas.getContext('webgl2', {
        antialias: this.options.antialias,
        alpha: this.options.alpha,
        preserveDrawingBuffer: this.options.preserveDrawingBuffer
      }) || this.canvas.getContext('webgl', {
        antialias: this.options.antialias,
        alpha: this.options.alpha
      });

      if (!this.ctx) {
        throw new Error('WebGL not supported');
      }

      this.setupCanvas();
      this.setupResizeListener();
      console.log('🦅 Phoenix Player initialized');
      
    } catch (error) {
      console.error('Failed to initialize player:', error);
      this.showError(error.message);
    }
  }

  setupCanvas() {
    const dpr = window.devicePixelRatio || 1;
    this.canvas.width = this.options.width * dpr;
    this.canvas.height = this.options.height * dpr;
    this.canvas.style.width = `${this.options.width}px`;
    this.canvas.style.height = `${this.options.height}px`;
    
    this.ctx.viewport(0, 0, this.canvas.width, this.canvas.height);
  }

  setupResizeListener() {
    const resizeObserver = new ResizeObserver((entries) => {
      for (const entry of entries) {
        if (entry.target === this.canvas) {
          this.handleResize();
        }
      }
    });
    resizeObserver.observe(this.canvas);
  }

  handleResize() {
    const dpr = window.devicePixelRatio || 1;
    const rect = this.canvas.getBoundingClientRect();
    this.canvas.width = rect.width * dpr;
    this.canvas.height = rect.height * dpr;
    this.ctx.viewport(0, 0, this.canvas.width, this.canvas.height);
    
    if (this.engine && this.engine.onResize) {
      this.engine.onResize(this.canvas.width, this.canvas.height);
    }
  }

  async loadDemo(demoConfig) {
    this.demoData = demoConfig;
    
    try {
      // Simulate WASM/engine loading
      await this.loadEngine();
      await this.loadDemoAssets();
      this.startRenderLoop();
      
      this.updateStatsDisplay();
      console.log(`✅ Demo loaded: ${demoConfig.title}`);
      
    } catch (error) {
      console.error('Failed to load demo:', error);
      this.showError(error.message);
    }
  }

  async loadEngine() {
    // Placeholder for actual WASM engine loading
    // In production, this would load the Phoenix Engine WASM module
    return new Promise((resolve) => {
      // Simulate loading time
      setTimeout(() => {
        this.engine = {
          isReady: true,
          version: '1.0.0'
        };
        resolve();
      }, 500);
    });
  }

  async loadDemoAssets() {
    // Placeholder for demo-specific asset loading
    // Models, textures, shaders, etc.
    return new Promise((resolve) => {
      setTimeout(resolve, 300);
    });
  }

  startRenderLoop() {
    if (this.isRunning) return;
    
    this.isRunning = true;
    this.stats.lastTime = performance.now();
    this.stats.frameCount = 0;
    
    const render = () => {
      if (!this.isRunning) return;
      
      this.render();
      this.updateFPS();
      
      requestAnimationFrame(render);
    };
    
    render();
  }

  render() {
    if (!this.ctx || !this.engine) return;

    // Clear canvas
    this.ctx.clearColor(0.1, 0.1, 0.18, 1.0);
    this.ctx.clear(this.ctx.COLOR_BUFFER_BIT | this.ctx.DEPTH_BUFFER_BIT);

    // Simulate rendering stats
    this.stats.drawCalls = Math.floor(Math.random() * 50) + 20;
    this.stats.vertices = Math.floor(Math.random() * 10000) + 5000;
    this.stats.triangles = this.stats.vertices * 2;

    this.stats.frameCount++;
  }

  updateFPS() {
    const currentTime = performance.now();
    const elapsed = currentTime - this.stats.lastTime;

    if (elapsed >= 1000) {
      this.stats.fps = Math.round((this.stats.frameCount * 1000) / elapsed);
      this.stats.frameCount = 0;
      this.stats.lastTime = currentTime;
      
      this.updateStatsDisplay();
    }
  }

  updateStatsDisplay() {
    const fpsEl = document.getElementById('fpsDisplay');
    const drawCallsEl = document.getElementById('drawCallsDisplay');
    const verticesEl = document.getElementById('verticesDisplay');

    if (fpsEl) fpsEl.textContent = `${this.stats.fps} FPS`;
    if (drawCallsEl) drawCallsEl.textContent = `${this.stats.drawCalls} 绘制调用`;
    if (verticesEl) verticesEl.textContent = `${this.stats.vertices.toLocaleString()} 顶点`;
  }

  pause() {
    this.isRunning = false;
  }

  resume() {
    if (!this.isRunning) {
      this.startRenderLoop();
    }
  }

  stop() {
    this.isRunning = false;
    this.engine = null;
    this.demoData = null;
    
    // Clear canvas
    if (this.ctx) {
      this.ctx.clearColor(0, 0, 0, 1);
      this.ctx.clear(this.ctx.COLOR_BUFFER_BIT);
    }
  }

  showError(message) {
    const canvas = this.canvas;
    if (!canvas) return;

    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    ctx.fillStyle = '#1a1a2e';
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    
    ctx.fillStyle = '#ff6b35';
    ctx.font = 'bold 24px sans-serif';
    ctx.textAlign = 'center';
    ctx.fillText('⚠️ 加载失败', canvas.width / 2, canvas.height / 2 - 20);
    
    ctx.fillStyle = '#b8b8b8';
    ctx.font = '16px sans-serif';
    ctx.fillText(message, canvas.width / 2, canvas.height / 2 + 20);
  }

  getStats() {
    return { ...this.stats };
  }

  captureScreenshot() {
    if (!this.canvas) return null;
    
    try {
      return this.canvas.toDataURL('image/png');
    } catch (error) {
      console.error('Failed to capture screenshot:', error);
      return null;
    }
  }

  setFullscreen() {
    if (!this.canvas) return;

    if (this.canvas.requestFullscreen) {
      this.canvas.requestFullscreen();
    } else if (this.canvas.webkitRequestFullscreen) {
      this.canvas.webkitRequestFullscreen();
    } else if (this.canvas.mozRequestFullScreen) {
      this.canvas.mozRequestFullScreen();
    } else if (this.canvas.msRequestFullscreen) {
      this.canvas.msRequestFullscreen();
    }
  }

  exitFullscreen() {
    if (document.exitFullscreen) {
      document.exitFullscreen();
    } else if (document.webkitExitFullscreen) {
      document.webkitExitFullscreen();
    } else if (document.mozCancelFullScreen) {
      document.mozCancelFullScreen();
    } else if (document.msExitFullscreen) {
      document.msExitFullscreen();
    }
  }
}

// Demo-specific player configurations
const DemoPlayers = {
  'pbr-materials': {
    init: async (player) => {
      // PBR materials demo setup
      console.log('🎨 Loading PBR materials demo...');
    },
    render: (player, delta) => {
      // Custom render logic for PBR demo
    }
  },
  'lighting': {
    init: async (player) => {
      console.log('💡 Loading lighting demo...');
    },
    render: (player, delta) => {
      // Custom render logic for lighting demo
    }
  },
  'animation': {
    init: async (player) => {
      console.log('🎭 Loading animation demo...');
    },
    render: (player, delta) => {
      // Custom render logic for animation demo
    }
  },
  'particles': {
    init: async (player) => {
      console.log('✨ Loading particles demo...');
    },
    render: (player, delta) => {
      // Custom render logic for particles demo
    }
  },
  'performance': {
    init: async (player) => {
      console.log('⚡ Loading performance demo...');
    },
    render: (player, delta) => {
      // Custom render logic for performance demo
    }
  }
};

// Export for module usage
if (typeof module !== 'undefined' && module.exports) {
  module.exports = { PhoenixPlayer, DemoPlayers };
}

// Auto-initialize if canvas exists
document.addEventListener('DOMContentLoaded', () => {
  const canvas = document.getElementById('phoenixCanvas');
  if (canvas) {
    window.player = new PhoenixPlayer('phoenixCanvas');
  }
});
