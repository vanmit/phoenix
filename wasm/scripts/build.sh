#!/bin/bash
# Phoenix Engine WASM Build Script
# 
# Usage: ./build.sh [debug|release|minimal]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"

# Default build type
BUILD_TYPE="${1:-release}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Phoenix Engine WASM Build${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Check for Emscripten
if ! command -v emcc &> /dev/null; then
    echo -e "${RED}Error: Emscripten not found${NC}"
    echo "Please install and activate Emscripten SDK:"
    echo "  git clone https://github.com/emscripten-core/emsdk.git"
    echo "  cd emsdk && ./emsdk install latest"
    echo "  ./emsdk activate latest && source ./emsdk_env.sh"
    exit 1
fi

# Check Emscripten version
EMCC_VERSION=$(emcc --version | head -n1)
echo -e "${GREEN}Emscripten:${NC} $EMCC_VERSION"

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure based on build type
case "$BUILD_TYPE" in
    debug)
        echo -e "${YELLOW}Build Type: Debug${NC}"
        emcmake cmake .. \
            -DCMAKE_BUILD_TYPE=Debug \
            -DWASM_ENABLE_ASSERTIONS=ON \
            -DWASM_ENABLE_PROFILING=ON
        ;;
    release)
        echo -e "${YELLOW}Build Type: Release${NC}"
        emcmake cmake .. \
            -DCMAKE_BUILD_TYPE=Release \
            -DWASM_BUILD_WITH_WEBGPU=ON \
            -DWASM_BUILD_WITH_WEBGL2=ON
        ;;
    minimal)
        echo -e "${YELLOW}Build Type: Minimal${NC}"
        emcmake cmake .. \
            -DCMAKE_BUILD_TYPE=Release \
            -DWASM_BUILD_MINIMAL=ON
        ;;
    threaded)
        echo -e "${YELLOW}Build Type: Threaded${NC}"
        emcmake cmake .. \
            -DCMAKE_BUILD_TYPE=Release \
            -DENABLE_WASM_THREADS=ON
        ;;
    *)
        echo -e "${RED}Unknown build type: $BUILD_TYPE${NC}"
        echo "Valid options: debug, release, minimal, threaded"
        exit 1
        ;;
esac

# Build
echo ""
echo -e "${BLUE}Building...${NC}"
emmake make -j$(nproc)

# Check output
if [ -f "$BUILD_DIR/dist/phoenix-wasm.js" ]; then
    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}Build Successful!${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo ""
    
    # Show file sizes
    echo "Output files:"
    ls -lh "$BUILD_DIR/dist/" | grep -E "phoenix-wasm\.(js|wasm)"
    
    # Calculate compressed size
    if command -v brotli &> /dev/null; then
        echo ""
        echo "Compressed sizes (Brotli):"
        brotli --quality=11 -z -k "$BUILD_DIR/dist/phoenix-wasm.wasm" 2>/dev/null || true
        ls -lh "$BUILD_DIR/dist/phoenix-wasm.wasm.br" 2>/dev/null || true
    fi
    
    echo ""
    echo -e "${BLUE}To run the examples:${NC}"
    echo "  cd $BUILD_DIR/dist"
    echo "  npx http-server -p 8080"
    echo ""
    echo "Then open: http://localhost:8080/examples/index.html"
else
    echo -e "${RED}Build failed! Output not found.${NC}"
    exit 1
fi
