# Phoenix Engine WebAssembly

High-performance 3D rendering engine compiled to WebAssembly with WebGL 2.0 and WebGPU support.

## Features

- 🚀 **Native Performance** - Near-native speed with WebAssembly
- 🎮 **WebGL 2.0 & WebGPU** - Modern graphics backend support
- 🧵 **Multi-threading** - Web Workers with SharedArrayBuffer
- 💾 **Async Filesystem** - IndexedDB persistent storage
- ⚡ **Optimized Loading** - <3s load time on 3G networks
- 📦 **Framework Ready** - React, Vue, and vanilla JS support

## Quick Start

### Installation

```bash
npm install @phoenix-engine/wasm
```

### Basic Usage

```html
<!DOCTYPE html>
<html>
<head>
  <title>Phoenix Engine Demo</title>
</head>
<body>
  <canvas id="canvas"></canvas>
  <script type="module">
    import { PhoenixEngine } from '@phoenix-engine/wasm';
    
    const engine = new PhoenixEngine();
    await engine.init({
      canvasId: 'canvas',
      width: 1024,
      height: 768
    });
    
    engine.start();
  </script>
</body>
</html>
```

### React

```tsx
import { PhoenixEngine } from '@phoenix-engine/wasm/react';

function App() {
  return (
    <PhoenixEngine 
      config={{ enableWebGPU: true }}
      onFrame={(frame) => console.log(frame.fps)}
    />
  );
}
```

### Vue

```vue
<template>
  <PhoenixEngine 
    :config="{ enableWebGPU: true }"
    @frame="onFrame"
  />
</template>

<script setup>
import { PhoenixEngine } from '@phoenix-engine/wasm/vue';

const onFrame = (frame) => {
  console.log('FPS:', frame.fps);
};
</script>
```

## Building from Source

### Prerequisites

1. **Emscripten SDK** (version 3.1.45+)
```bash
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install 3.1.45
./emsdk activate 3.1.45
source ./emsdk_env.sh
```

2. **CMake** (version 3.20+)
```bash
# Ubuntu/Debian
sudo apt-get install cmake

# macOS
brew install cmake
```

3. **Node.js** (version 16+)
```bash
# Download from https://nodejs.org
```

### Build Steps

```bash
# Clone repository
git clone https://github.com/phoenix-engine/phoenix-engine.git
cd phoenix-engine/wasm

# Install dependencies
npm install

# Configure build
mkdir build && cd build
emcmake cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
emmake make -j$(nproc)

# Optimize (optional, reduces size by ~30%)
make phoenix-wasm-opt

# Compress with Brotli (optional)
make phoenix-wasm-br
```

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `WASM_BUILD_MINIMAL` | OFF | Smaller build, fewer features |
| `WASM_BUILD_WITH_WEBGPU` | ON | Include WebGPU support |
| `WASM_BUILD_WITH_WEBGL2` | ON | Include WebGL2 support |
| `ENABLE_WASM_THREADS` | OFF | Enable multi-threading |
| `ENABLE_IDBFS` | ON | Enable IndexedDB filesystem |
| `WASM_ENABLE_PROFILING` | OFF | Enable performance profiling |

Example:
```bash
emcmake cmake .. \
  -DWASM_BUILD_MINIMAL=ON \
  -DENABLE_WASM_THREADS=ON \
  -DCMAKE_BUILD_TYPE=Release
```

## Configuration

### EngineConfig

```typescript
interface EngineConfig {
  width?: number;              // Canvas width (default: 1024)
  height?: number;             // Canvas height (default: 768)
  enableWebGPU?: boolean;      // Enable WebGPU backend (default: true)
  enableWebGL2?: boolean;      // Enable WebGL2 backend (default: true)
  enableAsyncify?: boolean;    // Enable async operations (default: true)
  initialMemoryMB?: number;    // Initial memory (default: 128)
  maxMemoryMB?: number;        // Max memory (default: 2048)
  enableThreading?: boolean;   // Enable Web Workers (default: false)
  threadPoolSize?: number;     // Worker count (default: 4)
  canvasId?: string | HTMLCanvasElement;
  wasmPath?: string;           // Path to WASM file
}
```

## API Reference

### PhoenixEngine Class

#### Methods

- `init(config: EngineConfig): Promise<PhoenixEngine>` - Initialize engine
- `start(): void` - Start render loop
- `stop(): void` - Stop render loop
- `resize(width: number, height: number): void` - Resize canvas
- `loadResource(url: string, type: string): Promise<ResourceHandle>` - Load resource
- `createScene(): SceneHandle` - Create new scene
- `createTexture(data: ImageData | HTMLImageElement): TextureHandle` - Create texture
- `getGraphicsCaps(): GraphicsCaps` - Get graphics capabilities
- `getMemoryStats(): MemoryStats` - Get memory statistics
- `saveToIDB(path: string, data: Uint8Array): boolean` - Save to IndexedDB
- `loadFromIDB(path: string): Uint8Array | null` - Load from IndexedDB
- `setCallbacks(callbacks: EngineCallbacks): void` - Set event callbacks
- `shutdown(): void` - Shutdown engine

#### Callbacks

```typescript
interface EngineCallbacks {
  onProgress?: (progress: number) => void;
  onLoad?: (engine: PhoenixEngine) => void;
  onError?: (error: Error) => void;
  onFrame?: (frameInfo: FrameInfo) => void;
}
```

## Performance

### Benchmarks (Desktop)

| Browser | FPS (1080p) | Load Time | Memory |
|---------|-------------|-----------|--------|
| Chrome 120 | 60 | 1.2s | 85 MB |
| Firefox 121 | 60 | 1.4s | 92 MB |
| Edge 120 | 60 | 1.3s | 88 MB |
| Safari 17 | 58 | 1.6s | 95 MB |

### Mobile

| Device | FPS (720p) | Load Time |
|--------|------------|-----------|
| iPhone 15 | 60 | 2.1s |
| Pixel 8 | 58 | 2.3s |
| iPad Pro | 60 | 1.8s |

### Optimization Tips

1. **Enable Brotli compression** - Reduces WASM size by ~30%
2. **Use minimal build** for simpler scenes
3. **Enable threading** for CPU-intensive workloads
4. **Lazy load resources** using async resource loading
5. **Use texture compression** (ASTC, ETC2, S3TC)

## Browser Compatibility

### Required Features

| Feature | Chrome | Firefox | Safari | Edge |
|---------|--------|---------|--------|------|
| WebAssembly | 57+ | 52+ | 11+ | 16+ |
| WebGL 2.0 | 56+ | 51+ | 15+ | 79+ |
| SharedArrayBuffer | 92+ | 79+ | 15+ | 92+ |
| BigInt | 67+ | 68+ | 14+ | 79+ |

### WebGPU Support (Experimental)

| Browser | Version | Status |
|---------|---------|--------|
| Chrome | 113+ | ✅ Enabled |
| Edge | 113+ | ✅ Enabled |
| Firefox | 118+ | ⚠️ Nightly |
| Safari | 17+ | ⚠️ Technology Preview |

### Feature Detection

```javascript
const engine = new PhoenixEngine();
const caps = await engine.getGraphicsCaps();

if (caps.hasWebGPU) {
  console.log('WebGPU available!');
} else if (caps.hasWebGL2) {
  console.log('Falling back to WebGL2');
} else {
  console.log('WebGL fallback');
}
```

## File System (IDBFS)

Phoenix Engine uses IndexedDB for persistent storage:

```javascript
// Save data
const data = new Uint8Array([1, 2, 3, 4]);
engine.saveToIDB('/saves/game.dat', data);

// Load data
const loaded = engine.loadFromIDB('/saves/game.dat');
```

### Storage Limits

| Browser | Limit |
|---------|-------|
| Chrome | 60% of disk |
| Firefox | 50% of disk |
| Safari | Unlimited* |
| Edge | 60% of disk |

*Subject to user permission

## Threading

Enable multi-threading for better performance:

```bash
emcmake cmake .. -DENABLE_WASM_THREADS=ON
```

```javascript
await engine.init({
  enableThreading: true,
  threadPoolSize: 4
});
```

**Note:** Requires `Cross-Origin-Opener-Policy` and `Cross-Origin-Embedder-Policy` headers:

```
Cross-Origin-Opener-Policy: same-origin
Cross-Origin-Embedder-Policy: require-corp
```

## Debugging

### Debug Build

```bash
emcmake cmake .. -DCMAKE_BUILD_TYPE=Debug
emmake make
```

Debug builds include:
- Source maps
- Runtime assertions
- Safe heap checking
- Stack overflow detection

### Profiling

```bash
emcmake cmake .. -DWASM_ENABLE_PROFILING=ON
```

Use Chrome DevTools Performance tab for analysis.

### Logging

```javascript
engine.setCallbacks({
  onError: (error) => {
    console.error('Engine error:', error);
  },
  onFrame: (frame) => {
    if (frame.fps < 30) {
      console.warn('Low FPS:', frame.fps);
    }
  }
});
```

## Examples

See the `examples/` directory:

- `examples/html/` - Vanilla JavaScript
- `examples/react/` - React component
- `examples/vue/` - Vue 3 component

## Troubleshooting

### Common Issues

**WASM not loading**
- Check server serves `.wasm` with `application/wasm` MIME type
- Ensure CORS headers are set correctly

**WebGPU not available**
- Update browser to latest version
- Enable WebGPU flags in `chrome://flags` or `about:config`

**Threading not working**
- Add COOP/COEP headers to server
- Serve over HTTPS (required for SharedArrayBuffer)

**Memory errors**
- Increase `initialMemoryMB` and `maxMemoryMB`
- Enable memory growth: `ALLOW_MEMORY_GROWTH=1`

## License

MIT License - see LICENSE file for details.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Run tests: `npm test`
5. Submit a pull request

## Links

- [Documentation](https://phoenix-engine.dev/docs)
- [GitHub](https://github.com/phoenix-engine/phoenix-engine)
- [Discord](https://discord.gg/phoenix-engine)
- [NPM](https://www.npmjs.com/package/@phoenix-engine/wasm)
