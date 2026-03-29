#!/bin/bash
set -e

echo "=== Phoenix Engine ASAN/UBSAN Test ==="

# Configuration
BUILD_DIR="build-sanitizer"
export ASAN_OPTIONS="detect_leaks=1:abort_on_error=1:halt_on_error=1"
export UBSAN_OPTIONS="halt_on_error=1:abort_on_error=1"

# Clean build
rm -rf $BUILD_DIR
mkdir -p $BUILD_DIR
cd $BUILD_DIR

# Configure
echo "Configuring with sanitizers..."
cmake .. \
  -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_SANITIZERS=ON \
  -G Ninja

# Build
echo "Building with sanitizers..."
cmake --build . -j$(nproc)

# Run tests
echo "Running tests with ASAN/UBSAN..."
ctest --output-on-failure

echo ""
echo "=== All tests passed ==="
