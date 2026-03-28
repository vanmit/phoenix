#!/bin/bash
# Phoenix Engine WASM Demo - Build Script

set -e

echo "🦅 Phoenix Engine - WASM Build"
echo "=============================="

# Check for Emscripten
if ! command -v emcc &> /dev/null; then
    echo "❌ Emscripten not found. Please install emsdk first."
    echo "   git clone https://github.com/emscripten-core/emsdk.git"
    echo "   cd emsdk && ./emsdk install latest && ./emsdk activate latest"
    echo "   source ./emsdk_env.sh"
    exit 1
fi

echo "✅ Emscripten found: $(emcc --version | head -1)"

# Create output directory
mkdir -p dist

# Compile WASM module
echo "🔨 Compiling C++ to WASM..."
emcc demo-app.cpp -o dist/demo-app.wasm \
  -s WASM=1 \
  -s USE_WEBGL2=1 \
  -s FULL_ES3=1 \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s MAX_WEBGL_VERSION=2 \
  -s INITIAL_MEMORY=134217728 \
  -s EXPORTED_FUNCTIONS='["_init_gl","_update","_render","_on_resize","_on_touch_rotate","_on_touch_zoom","_on_touch_pan","_on_double_tap","_set_camera_mode","_set_material_param","_set_effect","_set_animation"]' \
  -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
  -s ENVIRONMENT=web \
  -s MODULARIZE=0 \
  -O3 -flto --closure 1

echo "✅ WASM compiled: dist/demo-app.wasm"

# Show file size
WASM_SIZE=$(ls -lh dist/demo-app.wasm | awk '{print $5}')
echo "📦 WASM size: $WASM_SIZE"

# Compress with Brotli if available
if command -v brotli &> /dev/null; then
    echo "🗜️  Compressing with Brotli..."
    brotli -9 -f dist/demo-app.wasm
    BROTLI_SIZE=$(ls -lh dist/demo-app.wasm.br | awk '{print $5}')
    echo "📦 Brotli size: $BROTLI_SIZE"
fi

# Copy other files
echo "📋 Copying assets..."
cp index.html dist/
cp demo-wasm.js dist/
cp -r styles dist/
cp -r assets dist/

echo ""
echo "✅ Build complete!"
echo "📁 Output: dist/"
echo ""
echo "To test locally:"
echo "  cd dist && python3 -m http.server 8080"
echo "  Open http://localhost:8080"
