<template>
  <div class="phoenix-engine-container" :class="className" :style="containerStyle">
    <canvas 
      ref="canvasRef"
      id="phoenix-canvas"
      class="phoenix-canvas"
    />
    
    <!-- Overlay UI -->
    <div v-if="isInitialized" class="phoenix-overlay">
      <div class="phoenix-stats">
        <span class="phoenix-stat">FPS: {{ fps.toFixed(1) }}</span>
        <span class="phoenix-stat">Backend: {{ backend }}</span>
        <span class="phoenix-stat">Memory: {{ memoryMB.toFixed(1) }} MB</span>
      </div>
    </div>
    
    <!-- Loading State -->
    <div v-if="!isInitialized && !error" class="phoenix-loading">
      <div class="phoenix-spinner" />
      <p>Loading Phoenix Engine...</p>
      <p v-if="loadProgress > 0" class="phoenix-progress">
        {{ Math.round(loadProgress * 100) }}%
      </p>
    </div>
    
    <!-- Error State -->
    <div v-if="error" class="phoenix-error">
      <h3>Engine Error</h3>
      <p>{{ error.message }}</p>
      <button @click="retry">Retry</button>
    </div>
    
    <!-- Custom UI Slot -->
    <slot />
  </div>
</template>

<script lang="ts">
import { 
  defineComponent, 
  ref, 
  onMounted, 
  onBeforeUnmount,
  computed,
  watch
} from 'vue';

import type { PropType } from 'vue';

interface PhoenixEngineConfig {
  width?: number;
  height?: number;
  enableWebGPU?: boolean;
  enableWebGL2?: boolean;
  enableAsyncify?: boolean;
  wasmPath?: string;
}

interface FrameInfo {
  deltaTime: number;
  frameTime: number;
  frameNumber: bigint;
  fps: number;
}

interface GraphicsCaps {
  hasWebGPU: boolean;
  hasWebGL2: boolean;
  hasWebGL: boolean;
  maxTextureSize: number;
}

export default defineComponent({
  name: 'PhoenixEngine',
  
  props: {
    config: {
      type: Object as PropType<PhoenixEngineConfig>,
      default: () => ({})
    },
    className: {
      type: String,
      default: ''
    },
    autoStart: {
      type: Boolean,
      default: true
    }
  },
  
  emits: ['init', 'frame', 'error', 'load', 'ready'],
  
  setup(props, { emit }) {
    const canvasRef = ref<HTMLCanvasElement | null>(null);
    const engineRef = ref<any>(null);
    const animationFrameRef = ref<number>(0);
    
    const isInitialized = ref(false);
    const isRunning = ref(false);
    const fps = ref(0);
    const memoryMB = ref(0);
    const backend = ref('');
    const loadProgress = ref(0);
    const error = ref<Error | null>(null);
    
    const containerStyle = computed(() => ({
      position: 'relative' as const,
      width: '100%',
      height: '100%'
    }));
    
    // Initialize engine
    const initEngine = async () => {
      try {
        // Dynamically import the engine
        const module = await import('../../js/phoenix-wasm-api');
        const EngineClass = module.PhoenixEngine || module.default;
        
        const engine = new EngineClass();
        engineRef.value = engine;
        
        // Set callbacks
        engine.setCallbacks({
          onProgress: (progress: number) => {
            loadProgress.value = progress;
          },
          onLoad: () => {
            isInitialized.value = true;
            emit('load');
            emit('init', engine);
            
            // Detect backend
            const caps = engine.getGraphicsCaps();
            if (caps) {
              backend.value = caps.hasWebGPU ? 'WebGPU' : 
                             caps.hasWebGL2 ? 'WebGL2' : 'WebGL';
            }
            
            if (props.autoStart) {
              start();
            }
            
            emit('ready');
          },
          onError: (err: Error) => {
            error.value = err;
            emit('error', err);
          },
          onFrame: (frameInfo: FrameInfo) => {
            fps.value = frameInfo.fps;
            
            // Update memory stats periodically
            if (frameInfo.frameNumber % 60n === 0n) {
              const stats = engine.getMemoryStats();
              if (stats) {
                memoryMB.value = Number(stats.usedBytes) / 1024 / 1024;
              }
            }
            
            emit('frame', frameInfo);
          }
        });
        
        // Initialize with config
        await engine.init({
          ...props.config,
          canvasId: canvasRef.value
        });
        
      } catch (err) {
        const errObj = err instanceof Error ? err : new Error(String(err));
        error.value = errObj;
        emit('error', errObj);
      }
    };
    
    const start = () => {
      if (engineRef.value && !isRunning.value) {
        engineRef.value.start();
        isRunning.value = true;
      }
    };
    
    const stop = () => {
      if (engineRef.value && isRunning.value) {
        engineRef.value.stop();
        isRunning.value = false;
      }
    };
    
    const resize = (width: number, height: number) => {
      if (engineRef.value) {
        engineRef.value.resize(width, height);
      }
    };
    
    const handleWindowResize = () => {
      if (canvasRef.value && engineRef.value) {
        resize(canvasRef.value.clientWidth, canvasRef.value.clientHeight);
      }
    };
    
    const retry = async () => {
      error.value = null;
      loadProgress.value = 0;
      isInitialized.value = false;
      await initEngine();
    };
    
    // Expose methods to parent
    const expose = {
      start,
      stop,
      resize,
      loadResource: async (url: string, type: string) => {
        if (engineRef.value) {
          return await engineRef.value.loadResource(url, type);
        }
        throw new Error('Engine not initialized');
      },
      createScene: () => {
        if (engineRef.value) {
          return engineRef.value.createScene();
        }
        throw new Error('Engine not initialized');
      },
      getGraphicsCaps: (): GraphicsCaps | null => {
        if (engineRef.value) {
          return engineRef.value.getGraphicsCaps();
        }
        return null;
      },
      isRunning,
      isInitialized
    };
    
    defineExpose(expose);
    
    // Lifecycle
    onMounted(() => {
      initEngine();
      window.addEventListener('resize', handleWindowResize);
    });
    
    onBeforeUnmount(() => {
      if (animationFrameRef.value) {
        cancelAnimationFrame(animationFrameRef.value);
      }
      if (engineRef.value) {
        engineRef.value.shutdown();
      }
      window.removeEventListener('resize', handleWindowResize);
    });
    
    return {
      canvasRef,
      isInitialized,
      isRunning,
      fps,
      memoryMB,
      backend,
      loadProgress,
      error,
      containerStyle,
      Math: Math,
      retry
    };
  }
});
</script>

<style scoped>
.phoenix-engine-container {
  width: 100%;
  height: 100%;
  background: #1a1a2e;
  overflow: hidden;
}

.phoenix-canvas {
  display: block;
  width: 100%;
  height: 100%;
}

.phoenix-overlay {
  position: absolute;
  top: 20px;
  left: 20px;
  background: rgba(0, 0, 0, 0.7);
  padding: 15px 20px;
  border-radius: 8px;
  backdrop-filter: blur(10px);
  color: #fff;
  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
}

.phoenix-stats {
  display: flex;
  gap: 20px;
  font-size: 14px;
}

.phoenix-stat {
  font-weight: 600;
}

.phoenix-stat:first-child {
  color: #00ff88;
}

.phoenix-loading {
  position: absolute;
  top: 50%;
  left: 50%;
  transform: translate(-50%, -50%);
  text-align: center;
  color: #fff;
  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
}

.phoenix-spinner {
  width: 50px;
  height: 50px;
  border: 4px solid rgba(255, 255, 255, 0.1);
  border-top-color: #00d4ff;
  border-radius: 50%;
  animation: spin 1s linear infinite;
  margin: 0 auto 20px;
}

@keyframes spin {
  to {
    transform: rotate(360deg);
  }
}

.phoenix-progress {
  margin-top: 10px;
  color: #00d4ff;
  font-weight: 600;
}

.phoenix-error {
  position: absolute;
  top: 50%;
  left: 50%;
  transform: translate(-50%, -50%);
  background: rgba(255, 68, 68, 0.9);
  padding: 30px;
  border-radius: 12px;
  text-align: center;
  color: #fff;
  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
  max-width: 400px;
}

.phoenix-error h3 {
  margin: 0 0 15px 0;
  font-size: 20px;
}

.phoenix-error p {
  margin: 0 0 20px 0;
  opacity: 0.9;
}

.phoenix-error button {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  border: none;
  color: white;
  padding: 12px 24px;
  border-radius: 6px;
  cursor: pointer;
  font-size: 14px;
  font-weight: 600;
  transition: all 0.3s ease;
}

.phoenix-error button:hover {
  transform: translateY(-2px);
  box-shadow: 0 6px 20px rgba(102, 126, 234, 0.6);
}
</style>
