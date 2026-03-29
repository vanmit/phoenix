# CMake Sanitizer 配置

**准备时间**: 2026-03-29 16:10  
**任务**: 1.4 - ASAN/UBSAN 集成  

---

## 📋 CMakeLists.txt 修改

### 添加 sanitizer 选项

```cmake
# Sanitizer support (AddressSanitizer + UndefinedBehaviorSanitizer)
option(ENABLE_SANITIZERS "Enable ASAN and UBSAN for debugging" OFF)
option(SANITIZE_THREAD "Use ThreadSanitizer instead of ASAN" OFF)

if(ENABLE_SANITIZERS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        message(STATUS "Enabling sanitizers...")
        
        # Base sanitizer flags
        set(SANITIZER_FLAGS "-fsanitize=address,undefined")
        set(SANITIZER_LINK_FLAGS "-fsanitize=address,undefined")
        
        # ThreadSanitizer (alternative to ASAN)
        if(SANITIZE_THREAD)
            set(SANITIZER_FLAGS "-fsanitize=thread,undefined")
            set(SANITIZER_LINK_FLAGS "-fsanitize=thread,undefined")
        endif()
        
        # Add to compiler flags
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SANITIZER_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS}")
        set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} ${SANITIZER_LINK_FLAGS}")
        
        # ASAN options
        if(NOT SANITIZE_THREAD)
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize-address-use-after-scope")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize-address-use-after-scope")
        endif()
        
        # Disable for Release builds
        if(CMAKE_BUILD_TYPE STREQUAL "Release")
            message(WARNING "Sanitizers are disabled in Release builds")
        endif()
        
    elseif(MSVC)
        # MSVC uses different sanitizer options
        message(STATUS "MSVC: Using /fsanitize=address")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /fsanitize=address")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fsanitize=address")
    else()
        message(WARNING "Sanitizers not supported for this compiler")
    endif()
endif()

# Print sanitizer status
if(ENABLE_SANITIZERS)
    message(STATUS "Sanitizers enabled: ${SANITIZER_FLAGS}")
else()
    message(STATUS "Sanitizers disabled")
endif()
```

---

## 📋 GitHub Actions CI 配置

### `.github/workflows/ci-sanitizer.yml`

```yaml
name: CI - Sanitizer Tests

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main ]

jobs:
  sanitizer-test:
    runs-on: ubuntu-latest
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y clang cmake ninja-build
    
    - name: Configure with ASAN
      run: |
        cmake -B build -S . \
          -DCMAKE_BUILD_TYPE=Debug \
          -DCMAKE_C_COMPILER=clang \
          -DCMAKE_CXX_COMPILER=clang++ \
          -DENABLE_SANITIZERS=ON \
          -G Ninja
    
    - name: Build
      run: cmake --build build
    
    - name: Run tests
      run: |
        cd build
        ctest --output-on-failure
      env:
        ASAN_OPTIONS: detect_leaks=1:abort_on_error=1:halt_on_error=1
        UBSAN_OPTIONS: halt_on_error=1:abort_on_error=1
    
    - name: Upload sanitizer report
      if: failure()
      uses: actions/upload-artifact@v3
      with:
        name: sanitizer-report
        path: build/Testing/
```

---

## 📋 测试脚本

### `scripts/asan_test.sh`

```bash
#!/bin/bash
set -e

echo "=== Phoenix Engine ASAN/UBSAN Test ==="

# Configuration
BUILD_DIR="build-sanitizer"
ASAN_OPTIONS="detect_leaks=1:abort_on_error=1:halt_on_error=1"
UBSAN_OPTIONS="halt_on_error=1:abort_on_error=1"

# Clean build
rm -rf $BUILD_DIR
mkdir -p $BUILD_DIR
cd $BUILD_DIR

# Configure
cmake .. \
  -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_SANITIZERS=ON \
  -G Ninja

# Build
echo "Building with sanitizers..."
cmake --build .

# Run tests
echo "Running tests with ASAN/UBSAN..."
export ASAN_OPTIONS=$ASAN_OPTIONS
export UBSAN_OPTIONS=$UBSAN_OPTIONS

ctest --output-on-failure

echo "=== All tests passed ==="
```

---

## 🎯 验收标准

### 配置正确性
- [ ] CMake 配置无警告
- [ ] 编译成功
- [ ] 测试可执行

### CI/CD 集成
- [ ] GitHub Actions 配置正确
- [ ] 自动触发
- [ ] 错误报告生成

### 测试覆盖
- [ ] 所有测试通过
- [ ] 零 sanitizer 错误
- [ ] 性能开销 <20%

---

## 🔍 审查要点

### Security Audit Agent
1. Sanitizer 配置正确性
2. 测试覆盖率
3. 错误检测能力
4. CI/CD 集成完整性

---

*准备完成，等待任务 1.3 审查通过后执行*
