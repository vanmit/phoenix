# Memory Leak Detection Guide
# Using ASAN, UBSAN, and Valgrind for Phoenix Engine

## Overview

Memory safety is critical for Phoenix Engine. This guide covers:
- AddressSanitizer (ASAN) for memory error detection
- UndefinedBehaviorSanitizer (UBSAN) for undefined behavior
- LeakSanitizer (LSAN) for memory leak detection
- Valgrind for comprehensive memory analysis

## Quick Start

```bash
# Build with sanitizers
mkdir build && cd build
cmake -DENABLE_SANITIZERS=ON \
      -DSANITIZER_TYPES="address,undefined,leak" \
      -DCMAKE_BUILD_TYPE=Debug \
      ..
cmake --build .

# Run tests
./phoenix_tests

# Run with specific ASAN options
ASAN_OPTIONS=detect_leaks=1:detect_stack_use_after_return=1 ./phoenix_tests
```

## AddressSanitizer (ASAN)

### What ASAN Detects

| Error Type | CWE | Description |
|------------|-----|-------------|
| Buffer overflow | CWE-119, CWE-120 | Read/write beyond allocated memory |
| Use after free | CWE-416 | Accessing freed memory |
| Double free | CWE-415 | Freeing memory twice |
| Memory leak | CWE-401 | Not freeing allocated memory |
| Stack use after return | CWE-562 | Returning pointer to stack variable |
| Initialization order | CWE-459 | Using uninitialized memory |

### ASAN Configuration

```bash
# Environment variables
export ASAN_OPTIONS="\
detect_leaks=1:\
detect_stack_use_after_return=1:\
abort_on_error=1:\
symbolize=1:\
check_initialization_order=1:\
strict_init_order=1:\
halt_on_error=0:\
quarantine_size_mb=256:\
max_redzone=128"

# Run tests
./phoenix_tests
```

### Example ASAN Output

```
==12345==ERROR: AddressSanitizer: heap-use-after-free on address 0x602000000010
READ of size 4 at 0x602000000010 thread T0
    #0 0x4f8a2b in phoenix::MemoryManager::read() src/core/MemoryManager.cpp:42
    #1 0x4f9c3d in test_memory_access() tests/test_security.cpp:156
    #2 0x7ffff7a12345 in void testing::internal::HandleSehExceptionsInMethodIfSupported<testing::Test, void>()

0x602000000010 is located 0 bytes inside of 16-byte region [0x602000000010,0x602000000020)
freed by thread T0 here:
    #0 0x7ffff7f12345 in operator delete(void*) 
    #1 0x4f8b3c in phoenix::MemoryManager::free() src/core/MemoryManager.cpp:58

previously allocated by thread T0 here:
    #0 0x7ffff7f11234 in operator new(unsigned long)
    #1 0x4f8a1a in phoenix::MemoryManager::allocate() src/core/MemoryManager.cpp:32

SUMMARY: AddressSanitizer: heap-use-after-free src/core/MemoryManager.cpp:42
```

### Fixing ASAN Errors

**Example: Use After Free**

```cpp
// ❌ Bug
void process() {
    auto* data = allocator.allocate(1024);
    allocator.free(data);
    // ... later ...
    use(data);  // ASAN will catch this!
}

// ✅ Fix
void process() {
    auto* data = allocator.allocate(1024);
    use(data);
    allocator.free(data);
    data = nullptr;  // Prevent accidental reuse
}
```

## UndefinedBehaviorSanitizer (UBSAN)

### What UBSAN Detects

| Error Type | Description |
|------------|-------------|
| Signed integer overflow | `INT_MAX + 1` |
| Unsigned integer overflow | With `unsigned-integer-overflow` |
| Null pointer dereference | `*nullptr` |
| Misaligned access | Unaligned memory access |
| Invalid type conversion | Invalid downcast |
| Division by zero | `x / 0` |
| Shift errors | `x << 32` on 32-bit int |

### UBSAN Configuration

```bash
export UBSAN_OPTIONS="\
print_stacktrace=1:\
abort_on_error=1:\
halt_on_error=0:\
report_error_type=1"

# Additional checks
CXXFLAGS="-fsanitize=undefined -fno-sanitize-recover=all"
```

### Example UBSAN Output

```
src/math/Vector3.cpp:45:15: runtime error: signed integer overflow: 2147483647 + 1 cannot be represented in type 'int'
SUMMARY: UndefinedBehaviorSanitizer: undefined-behavior src/math/Vector3.cpp:45:15
```

### Fixing UBSAN Errors

```cpp
// ❌ Bug - potential overflow
int multiply(int a, int b) {
    return a * b;  // May overflow
}

// ✅ Fix - check for overflow
#include <limits>

int multiply_safe(int a, int b) {
    if (a > 0 && b > 0 && a > std::numeric_limits<int>::max() / b) {
        throw std::overflow_error("Multiplication overflow");
    }
    // ... additional checks for other sign combinations ...
    return a * b;
}

// Or use built-in overflow checking (GCC/Clang)
int multiply_safe(int a, int b) {
    int result;
    if (__builtin_mul_overflow(a, b, &result)) {
        throw std::overflow_error("Multiplication overflow");
    }
    return result;
}
```

## LeakSanitizer (LSAN)

### LSAN Configuration

```bash
export ASAN_OPTIONS="detect_leaks=1:log_path=lsan.log"
export LSAN_OPTIONS="\
verbosity=1:\
log_threads=1:\
report_objects=1:\
max_leaks=5"

./phoenix_tests
```

### Example Leak Report

```
=================================================================
==12345==ERROR: LeakSanitizer: detected memory leaks

Direct leak of 1024 byte(s) in 1 object(s) allocated from:
    #0 0x7ffff7f11234 in operator new(unsigned long)
    #1 0x4f8a1a in phoenix::MemoryManager::allocate() src/core/MemoryManager.cpp:32
    #2 0x4f9d4e in test_allocation() tests/test_memory.cpp:25

Indirect leak of 2048 byte(s) in 2 object(s)

SUMMARY: LeakSanitizer: 3072 byte(s) leaked in 3 allocation(s)
```

### Fixing Memory Leaks

```cpp
// ❌ Bug - raw pointer leak
void process() {
    auto* data = new uint8_t[1024];
    // ... use data ...
    // Forgot to delete!
}

// ✅ Fix - use smart pointers
void process() {
    auto data = std::make_unique<uint8_t[]>(1024);
    // ... use data ...
    // Automatically freed when going out of scope
}

// Or use RAII wrapper
void process() {
    SecureBuffer<uint8_t> buffer(1024);
    // ... use buffer ...
    // Automatically zeroized and freed
}
```

## Valgrind

### When to Use Valgrind

- When ASAN is too slow (production-like testing)
- For more detailed leak reports
- For thread error detection (Helgrind)
- For cache profiling (Cachegrind)

### Valgrind Commands

```bash
# Basic memory check
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --verbose \
         ./phoenix_tests

# Thread error detection
valgrind --tool=helgrind ./phoenix_tests

# Cache profiling
valgrind --tool=cachegrind ./phoenix_tests

# Generate suppression file (to ignore known leaks)
valgrind --leak-check=full --gen-suppressions=all ./phoenix_tests 2>&1 | tee valgrind.log
```

### Example Valgrind Output

```
==12345== Invalid read of size 4
==12345==    at 0x4F8A2B: phoenix::MemoryManager::read() (MemoryManager.cpp:42)
==12345==    by 0x4F9C3D: test_memory_access() (test_security.cpp:156)
==12345==  Address 0x4e8a010 is 0 bytes inside a block of size 16 free'd
==12345==    at 0x4C2F24B: operator delete(void*) (in /usr/lib/valgrind/vgpreload_memcheck-amd64-linux.so)
==12345==    by 0x4F8B3C: phoenix::MemoryManager::free() (MemoryManager.cpp:58)

==12345== LEAK SUMMARY:
==12345==    definitely lost: 1,024 bytes in 1 blocks
==12345==    indirectly lost: 2,048 bytes in 2 blocks
==12345==    possibly lost: 0 bytes in 0 blocks
==12345==    still reachable: 4,096 bytes in 4 blocks
```

## CI/CD Integration

### GitHub Actions

```yaml
name: Memory Safety Checks

on: [push, pull_request]

jobs:
  asan-test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Build with ASAN
        run: |
          cmake -DENABLE_SANITIZERS=ON \
                -DSANITIZER_TYPES="address,undefined,leak" \
                -DCMAKE_BUILD_TYPE=Debug ..
          cmake --build .
      
      - name: Run ASAN tests
        run: |
          export ASAN_OPTIONS="detect_leaks=1:abort_on_error=1"
          ./phoenix_tests
      
      - name: Upload ASAN logs
        if: failure()
        uses: actions/upload-artifact@v3
        with:
          name: asan-logs
          path: |
            *.log
            *.txt

  valgrind-test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Install Valgrind
        run: sudo apt-get install -y valgrind
      
      - name: Build without sanitizers
        run: |
          cmake -DCMAKE_BUILD_TYPE=Debug ..
          cmake --build .
      
      - name: Run Valgrind
        run: |
          valgrind --leak-check=full \
                   --error-exitcode=1 \
                   ./phoenix_tests
```

## Best Practices

### 1. Use Smart Pointers

```cpp
// ❌ Avoid raw pointers for ownership
int* data = new int[100];
delete[] data;

// ✅ Use smart pointers
auto data = std::make_unique<int[]>(100);
// or
auto data = std::make_shared<std::vector<int>>(100);
```

### 2. Use RAII

```cpp
// ❌ Manual resource management
FILE* f = fopen("file.txt", "r");
// ... use file ...
fclose(f);

// ✅ RAII wrapper
auto f = FileHandle::open("file.txt", "r");
// ... use file ...
// Automatically closed
```

### 3. Zero Sensitive Data

```cpp
// ❌ Sensitive data may remain in memory
std::vector<uint8_t> key = generateKey();
// ... use key ...
// key destructor doesn't zero memory!

// ✅ Use secure buffer
SecureBuffer<uint8_t> key = generateSecureKey();
// ... use key ...
// Automatically zeroized on destruction
```

### 4. Check Return Values

```cpp
// ❌ Ignoring return values
allocate(size);  // What if it fails?

// ✅ Check return values
auto* ptr = allocate(size);
if (!ptr) {
    throw std::bad_alloc();
}
```

### 5. Use Bounds-Checked Containers

```cpp
// ❌ Raw array with manual bounds
int arr[10];
arr[index] = value;  // No bounds check!

// ✅ std::vector with at()
std::vector<int> arr(10);
arr.at(index) = value;  // Throws on out of bounds
```

## Troubleshooting

### ASAN Too Slow

```bash
# Disable some checks for performance
ASAN_OPTIONS="detect_leaks=0:detect_stack_use_after_return=0" ./phoenix_tests

# Or use only leak sanitizer (faster than full ASAN)
ASAN_OPTIONS="detect_leaks=1" LSAN_OPTIONS="fast_unwind_on_malloc=1" ./phoenix_tests
```

### False Positives

```bash
# Create suppression file
echo "leak:my_known_leak_function" > lsan_suppressions.txt
export LSAN_OPTIONS="suppressions=lsan_suppressions.txt"
```

### Symbolization Issues

```bash
# Ensure debug symbols are present
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Use llvm-symbolizer
export ASAN_SYMBOLIZER_PATH=/usr/bin/llvm-symbolizer
```

## References

- [AddressSanitizer Documentation](https://github.com/google/sanitizers/wiki/AddressSanitizer)
- [UndefinedBehaviorSanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html)
- [Valgrind User Guide](https://valgrind.org/docs/manual/manual.html)
- [CWE Memory Safety](https://cwe.mitre.org/data/definitions/119.html)
