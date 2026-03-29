#!/bin/bash
# Phoenix Engine WASM Demo Build Script
# Builds the demo-app.cpp into phoenix-engine.wasm

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "=========================================="
echo "Phoenix Engine WASM Demo Build"
echo "=========================================="
echo ""

# Check for Emscripten
if ! command -v emcc &> /dev/null; then
    echo "❌ Emscripten not found. Please install and activate emsdk:"
    echo "   git clone https://github.com/emscripten-core/emsdk.git"
    echo "   cd emsdk && ./emsdk install latest"
    echo "   ./emsdk activate latest"
    echo "   source ./emsdk_env.sh"
    exit 1
fi

echo "✅ Emscripten found: $(emcc --version | head -1)"
echo ""

# Source files
DEMO_APP="demo-app.cpp"
OUTPUT_WASM="phoenix-engine.wasm"
OUTPUT_JS="phoenix-engine.js"

# Include paths
INCLUDE_PATHS="-I../include -I../third-party/bgfx/include"

# Compile flags
CXX_FLAGS="-std=c++17 -O3 -flto"

# Link flags
LINK_FLAGS="\
-s WASM=1 \
-s USE_WEBGL2=1 \
-s FULL_ES3=1 \
-s ALLOW_MEMORY_GROWTH=1 \
-s MAX_WEBGL_VERSION=2 \
-s INITIAL_MEMORY=134217728 \
-s EXPORTED_FUNCTIONS='[\"_main\",\"_demo_init\",\"_demo_update\",\"_demo_render\",\"_demo_shutdown\",\"_demo_resize\",\"_demo_touch_rotate\",\"_demo_touch_zoom\",\"_demo_touch_pan\",\"_demo_double_tap\",\"_demo_set_camera_mode\",\"_demo_set_material_param\",\"_demo_set_effect\",\"_demo_set_animation\"]' \
-s EXPORTED_RUNTIME_METHODS='[\"ccall\",\"cwrap\"]' \
-s ENVIRONMENT=web \
-s MODULARIZE=0 \
-s SINGLE_FILE=0 \
"

echo "📦 Building WASM module..."
echo "   Source: $DEMO_APP"
echo "   Output: $OUTPUT_WASM"
echo ""

# Compile
emcc $DEMO_APP \
    $INCLUDE_PATHS \
    $CXX_FLAGS \
    $LINK_FLAGS \
    -o $OUTPUT_JS \
    -s WASM_BINARY_FILE=$OUTPUT_WASM

# Check output
if [ -f "$OUTPUT_WASM" ]; then
    WASM_SIZE=$(ls -lh "$OUTPUT_WASM" | awk '{print $5}')
    echo "✅ Build successful!"
    echo "   WASM size: $WASM_SIZE"
    echo ""
    
    # Show file info
    echo "📊 Output files:"
    ls -lh $OUTPUT_WASM $OUTPUT_JS 2>/dev/null || true
    echo ""
    
    # Optional: optimize
    if command -v wasm-opt &> /dev/null; then
        echo "🔧 Optimizing WASM..."
        wasm-opt $OUTPUT_WASM -O3 -o ${OUTPUT_WASM}.opt
        OPT_SIZE=$(ls -lh ${OUTPUT_WASM}.opt | awk '{print $5}')
        echo "   Optimized size: $OPT_SIZE"
        mv ${OUTPUT_WASM}.opt $OUTPUT_WASM
        echo ""
    fi
else
    echo "❌ Build failed!"
    exit 1
fi

echo "=========================================="
echo "Build complete! 🎉"
echo "=========================================="
echo ""
echo "To test the demo:"
echo "  1. Start a local server: python3 -m http.server 8080"
echo "  2. Open: http://localhost:8080/index.html"
echo ""
