#!/bin/bash
# Phoenix Engine WASM 构建脚本
# 使用 Emscripten 编译 Phoenix Engine 为 WebAssembly

set -e

echo "🔧 Phoenix Engine WASM 构建工具"
echo "================================"

# 检查 Emscripten
if ! command -v emcc &> /dev/null; then
    echo "❌ Emscripten 未找到，请先运行："
    echo "   source /home/admin/emsdk/emsdk_env.sh"
    exit 1
fi

echo "✅ Emscripten 已找到：$(which emcc)"
emcc --version | head -1

# 设置目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build_wasm"
DEMO_DIR="${SCRIPT_DIR}/demo-showcase"

echo ""
echo "📁 工作目录："
echo "   源码：${SCRIPT_DIR}"
echo "   构建：${BUILD_DIR}"
echo "   Demo: ${DEMO_DIR}"

# 创建构建目录
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# Emscripten 编译选项
EMCC_OPTS="\
-O3 \
-s WASM=1 \
-s USE_WEBGL2=1 \
-s FULL_ES3=1 \
-s ALLOW_MEMORY_GROWTH=1 \
-s MAX_WEBGL_VERSION=2 \
-s MIN_WEBGL_VERSION=2 \
-s INITIAL_MEMORY=134217728 \
-s MAXIMUM_MEMORY=536870912 \
-s DISABLE_EXCEPTION_CATCHING=0 \
-s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
-s ENVIRONMENT=web \
-s MODULARIZE=1 \
-s EXPORT_NAME='createPhoenixEngine' \
-s EXPORT_ES6=0 \
-s SINGLE_FILE=1 \
--bind \
-flto \
--closure 1"

echo ""
echo "🔨 开始编译 Phoenix Engine 为 WASM..."
echo ""

# 编译主 Demo
emcc ${EMCC_OPTS} \
    -I"${SCRIPT_DIR}/include" \
    -I"${SCRIPT_DIR}/src" \
    "${SCRIPT_DIR}/demo-showcase/demo-app.cpp" \
    -o "${BUILD_DIR}/phoenix-engine.js"

if [ $? -eq 0 ]; then
    echo ""
    echo "✅ 编译成功！"
    echo ""
    echo "📦 生成文件:"
    ls -lh "${BUILD_DIR}/phoenix-engine."*
    echo ""
    
    # 复制到 Demo 目录
    cp "${BUILD_DIR}/phoenix-engine.js" "${DEMO_DIR}/phoenix-engine.js"
    cp "${BUILD_DIR}/phoenix-engine.wasm" "${DEMO_DIR}/phoenix-engine.wasm" 2>/dev/null || true
    
    echo "📁 文件已复制到 Demo 目录"
    echo ""
    echo "🎉 部署到服务器:"
    echo "   scp ${DEMO_DIR}/phoenix-engine.* root@47.245.126.212:/var/www/demo/"
else
    echo ""
    echo "❌ 编译失败！"
    exit 1
fi
