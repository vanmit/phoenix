#!/bin/bash
# Phoenix Engine WASM 构建脚本 - 简化版本

set -e

echo "🔧 Phoenix Engine WASM 构建工具"
echo "================================"

export PATH="/home/admin/python3.10/bin:/home/admin/emsdk:/home/admin/emsdk/upstream/emscripten:$PATH"

if ! command -v emcc &> /dev/null; then
    echo "❌ Emscripten 未找到"
    exit 1
fi

echo "✅ Emscripten 已找到：$(which emcc)"
emcc --version | head -1

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build_wasm"
DEMO_DIR="${SCRIPT_DIR}/demo-showcase"

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

echo ""
echo "🔨 开始编译..."
echo ""

emcc \
    -O3 \
    -s WASM=1 \
    -s USE_WEBGL2=1 \
    -s FULL_ES3=1 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s INITIAL_MEMORY=134217728 \
    -s EXPORTED_RUNTIME_METHODS=ccall,cwrap \
    --bind \
    -flto \
    -I"${SCRIPT_DIR}/include" \
    "${SCRIPT_DIR}/demo-showcase/demo-app.cpp" \
    -o "${BUILD_DIR}/phoenix-engine.js"

if [ $? -eq 0 ]; then
    echo ""
    echo "✅ 编译成功！"
    echo ""
    ls -lh "${BUILD_DIR}/phoenix-engine."*
    
    cp "${BUILD_DIR}/phoenix-engine.js" "${DEMO_DIR}/"
    cp "${BUILD_DIR}/phoenix-engine.wasm" "${DEMO_DIR}/" 2>/dev/null || true
    
    echo ""
    echo "📁 文件已复制到 Demo 目录"
else
    echo ""
    echo "❌ 编译失败！"
    exit 1
fi
