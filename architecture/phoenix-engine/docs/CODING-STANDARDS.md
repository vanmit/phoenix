# Phoenix Engine - Coding Standards

## 1. Overview

This document defines the coding standards for Phoenix Engine, aligned with **MISRA C++ 2023** guidelines. All code must comply with these standards to ensure safety, security, and maintainability.

## 2. MISRA C++ 2023 Compliance

### 2.1 Compliance Level

Phoenix Engine targets **MISRA C++ 2023 Full Compliance** with the following exceptions (documented with suppressions):

```cpp
// NOLINTNEXTLINE(misra-cpp-2023-x.x.x) - Justification required
```

### 2.2 Mandatory Rules

The following MISRA rules are **mandatory** (no exceptions allowed):

| Rule ID | Description | Phoenix Implementation |
|---------|-------------|----------------------|
| 1.1.1 | All code shall be traceable to requirements | Every function documented |
| 2.1.1 | Required language features only | C++20 subset defined below |
| 3.1.1 | External identifiers unique | Naming conventions enforced |
| 4.1.1 | Run-time failures minimized | Result<T> error handling |
| 5.1.1 | Identifiers distinct | No shadowing allowed |
| 6.1.1 | Arithmetic types defined | Fixed-width types only |
| 7.1.1 | Literal suffixes required | `1.0f`, `100u`, `1000LL` |
| 8.1.1 | Functions single definition | One Definition Rule enforced |
| 9.1.1 | Object initialization | All objects initialized |
| 10.1.1 | Expressions well-formed | Static analysis enforced |
| 11.1.1 | Conversions safe | Explicit casts required |
| 12.1.1 | Operator precedence clear | Parentheses for clarity |
| 13.1.1 | Side effects controlled | No hidden side effects |
| 14.1.1 | Loop termination assured | All loops bounded |
| 15.1.1 | Control flow clear | No goto, limited break/continue |
| 16.1.1 | Functions return values | All paths return |
| 17.1.1 | Memory safety | RAII, no raw new/delete |
| 18.1.1 | Pointer safety | Smart pointers only |
| 19.1.1 | Exception safety | No exceptions in core |
| 20.1.1 | Const correctness | Maximize const usage |
| 21.1.1 | Resource management | RAII throughout |

## 3. C++ Language Subset

### 3.1 Allowed C++20 Features

✅ **Allowed:**
- Concepts and constraints
- Ranges library
- Coroutines (limited, documented)
- Modules (when stable)
- `std::span`, `std::string_view`
- `std::optional`, `std::variant`, `std::any`
- Structured bindings
- Designated initializers
- `consteval`, `constinit`
- Three-way comparison (`<=>`)

❌ **Prohibited:**
- Exceptions in core engine code (allowed in tools/examples)
- RTTI (`dynamic_cast`, `typeid`)
- `goto` statements
- Variable-length arrays (VLAs)
- Unions with non-trivial types
- Implicit conversions (except numeric promotions)

### 3.2 Required Language Features

```cpp
// All functions must specify noexcept when applicable
void process() noexcept;

// All pointers must be smart pointers
std::unique_ptr<T> m_resource;
std::shared_ptr<T> m_sharedResource;

// Raw pointers only for non-owning references
T* getPtr() const;  // Non-owning, do not delete

// All numeric types must be fixed-width
u32 count = 0u;
f32 value = 1.0f;
```

## 4. File Organization

### 4.1 Header Files

```cpp
// include/phoenix/core/engine.hpp
#ifndef PHOENIX_CORE_ENGINE_HPP
#define PHOENIX_CORE_ENGINE_HPP

#pragma once  // Secondary, for compilers that support it

// 1. Self-contained check (include own header first in .cpp)
// 2. Standard library includes
#include <memory>
#include <string>
#include <vector>

// 3. Phoenix includes (sorted)
#include "phoenix/core/types.hpp"
#include "phoenix/core/result.hpp"

// 4. Forward declarations
namespace phoenix::core {
    class EngineConfig;
}

// 5. Code
namespace phoenix::core {

class Engine {
    // ...
};

} // namespace phoenix::core

#endif // PHOENIX_CORE_ENGINE_HPP
```

### 4.2 Source Files

```cpp
// src/core/engine.cpp

// 1. Include own header first
#include "phoenix/core/engine.hpp"

// 2. Standard library
#include <algorithm>
#include <chrono>

// 3. Phoenix includes
#include "phoenix/core/logger.hpp"
#include "phoenix/security/manager.hpp"

// 4. Implementation
namespace phoenix::core {

// ...

} // namespace phoenix::core
```

### 4.3 Include Order

1. Own header (in .cpp files)
2. C standard library (`<cstdio>`, `<cstdlib>`)
3. C++ standard library (`<vector>`, `<string>`)
4. Third-party libraries (`<bgfx/bgfx.h>`)
5. Phoenix headers (alphabetically)

## 5. Naming Conventions

### 5.1 General Rules

```cpp
// Classes, structs, enums: PascalCase
class EntityManager;
struct RenderConfig;
enum class RenderMode;

// Functions, methods: camelCase
void initialize();
f32 calculateDistance();

// Variables: camelCase with m_ prefix for members
u32 m_frameCount;
std::string m_name;

// Local variables: camelCase, no prefix
u32 index = 0;
f32 deltaTime = 0.0f;

// Constants: UPPER_SNAKE_CASE
constexpr u32 MAX_ENTITIES = 10000u;
constexpr f32 PI = 3.14159265358979f;

// Namespaces: lowercase, nested
namespace phoenix::core::memory { }

// Template parameters: PascalCase with T prefix
template<typename TValue>
template<u32 TSize>
```

### 5.2 Special Cases

```cpp
// Type aliases: PascalCase
using EntityId = u64;
using ResourceHandle = std::shared_ptr<Resource>;

// Enum values: PascalCase
enum class LogLevel : u8 {
    Debug,
    Info,
    Warning,
    Error
};

// Macro constants: UPPER_SNAKE_CASE
#define PHOENIX_VERSION "1.0.0"

// Function parameters: camelCase, descriptive
void submit(const RenderBatch& batch, ViewId view);
```

## 6. Memory Management

### 6.1 RAII Principle

**Rule**: All resources must be managed through RAII.

```cpp
// ✅ Correct: RAII
class Texture {
    std::unique_ptr<Impl> m_impl;  // Ownership
};

// ❌ Incorrect: Raw pointers
class Texture {
    Impl* m_impl;  // Who owns this?
};
```

### 6.2 Smart Pointer Guidelines

```cpp
// Exclusive ownership
std::unique_ptr<T> m_resource;

// Shared ownership (use sparingly)
std::shared_ptr<T> m_sharedResource;
std::weak_ptr<T> m_weakResource;  // Break cycles

// Non-owning references (raw pointers OK)
T* getResource() const { return m_resource.get(); }
void process(T* resource);  // Document: non-owning

// Factory functions
std::unique_ptr<T> createT();
std::shared_ptr<T> makeSharedT();
```

### 6.3 Allocation Rules

```cpp
// ✅ Allowed
std::vector<T> items;  // Stack or container-managed
auto ptr = std::make_unique<T>();  // Smart pointer
T object;  // Stack allocation

// ❌ Prohibited
T* ptr = new T();  // Raw new
delete ptr;  // Raw delete
malloc/free  // C-style allocation

// ✅ Custom allocators (documented)
template<typename T>
class PoolAllocator {
    // Custom allocation, but RAII-managed
};
```

## 7. Error Handling

### 7.1 Result Type Usage

```cpp
// ✅ Correct: Result for fallible operations
Result<void> initialize();
Result<Resource*> load(const std::string& path);

// Check and propagate
Result<void> process() {
    PHOENIX_CHECK(initialize());
    PHOENIX_CHECK(loadResources());
    return Result<void>::success();
}

// Handle errors
auto result = load("texture.png");
if (result.isError()) {
    Logger::error("Failed to load: {}", result.errorMessage());
    return;
}
auto texture = result.value();
```

### 7.2 Error Codes

```cpp
enum class ErrorCode : u32 {
    Success = 0,
    InvalidParameter = 1,
    NotFound = 2,
    OutOfMemory = 4,
    SecurityViolation = 6,
    // ...
};

// Always include error context
Result<void> validate(const std::string& path) {
    if (path.empty()) {
        return Result<void>::error(
            ErrorCode::InvalidParameter,
            "Path cannot be empty"
        );
    }
    // ...
}
```

### 7.3 Assertions

```cpp
// Debug-only checks
PHOENIX_DEBUG_ASSERT(ptr != nullptr);
PHOENIX_DEBUG_ASSERT(index < m_size);

// Runtime validation (always checked)
PHOENIX_VERIFY(initialized, "Must call initialize() first");

// Security-critical (always checked, cannot be disabled)
PHOENIX_SECURITY_ASSERT(validationPassed, "Security violation");
```

## 8. Const Correctness

### 8.1 Maximize Const

```cpp
// Member functions that don't modify state
f32 getValue() const;
const std::string& getName() const;

// Parameters passed by const reference
void process(const Data& data);

// Return const for internal state
const std::vector<Entity>& getEntities() const;

// Mutable for cached values (documented)
mutable std::optional<Matrix4> m_cachedMatrix;
```

### 8.2 Constexpr Where Possible

```cpp
// Compile-time computation
constexpr f32 degToRad(f32 degrees) {
    return degrees * 0.017453292519943295f;
}

constexpr Vector3 operator+(const Vector3& a, const Vector3& b) {
    return Vector3(a.x + b.x, a.y + b.y, a.z + b.z);
}

// Compile-time constants
constexpr u32 MAX_TEXTURES = 16u;
constexpr f64 EPSILON = 1e-10;
```

## 9. Documentation

### 9.1 Doxygen Format

```cpp
/// @brief Brief description of the class
/// @tparam T The type parameter
/// @note Important note about usage
/// @warning Security warning if applicable
/// @see RelatedClass
template<typename T>
class Container {
public:
    /// @brief Constructs a new Container
    /// @param initialSize Initial capacity
    /// @param allocator Custom allocator (optional)
    /// @throws Nothing (noexcept)
    explicit Container(u32 initialSize = 16u) noexcept;
    
    /// @brief Adds an element
    /// @param value The value to add
    /// @return Result indicating success or error
    /// @retval Success Element added
    /// @retval Error_OutOfMemory Allocation failed
    Result<void> add(const T& value);
    
    /// @brief Gets element count
    /// @return Current number of elements
    [[nodiscard]] u32 size() const noexcept;
    
private:
    u32 m_count;  ///< Current element count
    u32 m_capacity;  ///< Allocated capacity
};
```

### 9.2 Required Documentation

Every public API must have:
1. Brief description (`@brief`)
2. Parameter documentation (`@param`)
3. Return value documentation (`@return`)
4. Error conditions (`@retval`)
5. Thread safety note if not obvious
6. Ownership semantics for pointers

### 9.3 Internal Documentation

```cpp
// Implementation notes for complex algorithms
// Algorithm: Spatial hashing with grid size = cellSize
// Complexity: O(n) average, O(n²) worst case
// Reference: "Real-Time Collision Detection" Chapter 5
void buildSpatialHash();
```

## 10. Thread Safety

### 10.1 Annotations

```cpp
// Thread-safe for reading
class Config {
    std::atomic<u32> m_version;  // Atomic for lock-free reads
};

// Requires external synchronization
class Renderer {
    // @thread_safe: No (must be called from main thread)
    void submit();
};

// Thread-safe with internal locking
class ResourceManager {
    // @thread_safe: Yes (internal mutex)
    Resource* get(const std::string& path);
};
```

### 10.2 Synchronization Rules

```cpp
// Use standard library primitives
std::mutex m_mutex;
std::atomic<u32> m_counter;
std::condition_variable m_cv;

// Lock guards, never raw lock/unlock
{
    std::lock_guard<std::mutex> lock(m_mutex);
    // Critical section
}

// Avoid deadlocks: consistent lock ordering
// Always lock: mutex_A -> mutex_B -> mutex_C
```

## 11. Security Requirements

### 11.1 Input Validation

```cpp
// All external input must be validated
Result<void> loadFile(const std::string& path) {
    // Validate path
    PHOENIX_CHECK(security::validatePath(path));
    
    // Validate file size
    auto size = getFileSize(path);
    PHOENIX_VERIFY(size < MAX_FILE_SIZE, "File too large");
    
    // Validate content through Rust security core
    PHOENIX_CHECK(security::validateFileContent(path));
    
    // ... proceed with loading
}
```

### 11.2 Buffer Safety

```cpp
// Use std::span for buffer views
void process(std::span<const u8> data);

// Bounds checking always enabled
std::vector<T> items;
items.at(index);  // Throws if out of bounds

// No pointer arithmetic
// ❌ ptr + offset
// ✅ std::data(vec) + offset (with bounds check)
```

### 11.3 Integer Safety

```cpp
// Check for overflow
u32 safeAdd(u32 a, u32 b) {
    PHOENIX_VERIFY(a <= UINT32_MAX - b, "Integer overflow");
    return a + b;
}

// Use safe math utilities
u32 result = phoenix::math::checkedAdd(a, b);
PHOENIX_CHECK(result.has_value());

// Casts must be explicit and validated
u16 toU16(u32 value) {
    PHOENIX_VERIFY(value <= UINT16_MAX, "Value too large");
    return static_cast<u16>(value);
}
```

## 12. Testing Requirements

### 12.1 Unit Test Coverage

```cpp
// Every public function must have tests
TEST(EngineTest, Initialize) {
    Engine engine;
    EngineConfig config;
    
    Result<void> result = engine.initialize(config);
    ASSERT_TRUE(result.isSuccess());
    ASSERT_TRUE(engine.isRunning());
}

// Edge cases
TEST(VectorTest, NormalizeZeroVector) {
    Vector3 zero(0.0f, 0.0f, 0.0f);
    // Should handle gracefully (document behavior)
    Vector3 normalized = zero.normalized();
    // ...
}

// Security tests
TEST(SecurityTest, PathTraversal) {
    Result<void> result = loadFile("../../../etc/passwd");
    ASSERT_TRUE(result.isError());
    ASSERT_EQ(result.errorCode(), ErrorCode::SecurityViolation);
}
```

### 12.2 Test Categories

1. **Unit Tests**: Individual function testing
2. **Integration Tests**: Component interaction
3. **Security Tests**: Validation, injection, overflow
4. **Performance Tests**: Benchmarks (optional in CI)
5. **Fuzz Tests**: Random input testing (security-critical)

## 13. Static Analysis

### 13.1 Required Tools

```bash
# Clang-Tidy (MISRA checks)
clang-tidy -checks='readability-*,misc-*,performance-*' src/*.cpp

# Cppcheck
cppcheck --enable=all --std=c++20 src/

# Custom MISRA checker
phoenix-misra-check src/
```

### 13.2 CI Integration

All code must pass:
- ✅ Clang-Tidy (zero warnings)
- ✅ Cppcheck (zero warnings)
- ✅ MISRA compliance check
- ✅ Unit tests (100% pass)
- ✅ Memory sanitizer (no leaks)

## 14. Code Review Checklist

Before submitting code, verify:

- [ ] MISRA C++ 2023 compliance
- [ ] All functions documented
- [ ] Error handling complete
- [ ] Memory management RAII
- [ ] Thread safety annotated
- [ ] Input validation present
- [ ] Unit tests written
- [ ] No compiler warnings
- [ ] Static analysis clean
- [ ] Security review (if applicable)

## 15. Examples

### 15.1 Good Example

```cpp
/// @brief Manages entity lifecycle in a scene
/// @thread_safe: Partially (entity creation requires main thread)
class EntityManager {
public:
    /// @brief Creates a new entity
    /// @param name Entity name (must be unique)
    /// @return Entity ID or error
    /// @retval Success Entity created
    /// @retval Error_InvalidParameter Name empty or duplicate
    /// @retval Error_OutOfMemory Maximum entities reached
    [[nodiscard]] Result<EntityId> createEntity(const std::string& name) {
        // Validate input
        if (name.empty()) {
            return Result<EntityId>::error(
                ErrorCode::InvalidParameter,
                "Entity name cannot be empty"
            );
        }
        
        // Check capacity
        if (m_entities.size() >= MAX_ENTITIES) {
            return Result<EntityId>::error(
                ErrorCode::OutOfMemory,
                "Maximum entity count reached"
            );
        }
        
        // Check uniqueness
        if (m_nameToId.find(name) != m_nameToId.end()) {
            return Result<EntityId>::error(
                ErrorCode::AlreadyExists,
                "Entity name already exists: " + name
            );
        }
        
        // Create entity
        const EntityId id = m_nextId++;
        m_entities.emplace_back(id, name);
        m_nameToId[name] = id;
        
        Logger::debug("Created entity: {} (ID: {})", name, id);
        return Result<EntityId>(id);
    }
    
    /// @brief Gets entity count
    /// @return Current number of entities
    [[nodiscard]] u32 count() const noexcept {
        return static_cast<u32>(m_entities.size());
    }
    
private:
    struct Entity {
        EntityId id;
        std::string name;
    };
    
    std::vector<Entity> m_entities;
    std::unordered_map<std::string, EntityId> m_nameToId;
    EntityId m_nextId = 0u;
    
    static constexpr u32 MAX_ENTITIES = 100000u;
};
```

### 15.2 Bad Example (Violations Marked)

```cpp
// ❌ No documentation
// ❌ No error handling
// ❌ Raw pointers
// ❌ No bounds checking
// ❌ No const correctness
// ❌ Magic numbers
class BadEntity {
public:
    BadEntity() {
        data = new int[100];  // ❌ Raw new
    }
    
    // ❌ No const, no noexcept
    int get(int index) {
        return data[index];  // ❌ No bounds check
    }
    
    void process() {
        for (int i = 0; i < 100; i++) {  // ❌ Magic number
            data[i] = i * 2;
        }
    }
    
    ~BadEntity() {
        delete[] data;  // ❌ Raw delete
    }
    
private:
    int* data;  // ❌ Raw pointer
};
```

---

## Appendix A: Compiler Flags

```cmake
# GCC/Clang
-Wall -Wextra -Wpedantic -Werror
-Wconversion -Wsign-conversion
-Wshadow -Wcast-qual
-Wformat=2 -Wwrite-strings
-fstrict-aliasing -fstrict-enums

# MSVC
/W4 /permissive- /Zc:__cplusplus
/wd4100  # Unused parameters (documented)
```

## Appendix B: Tool Configuration

### .clang-tidy

```yaml
---
Checks: >
  -*,
  bugprone-*,
  cert-*,
  clang-analyzer-*,
  cppcoreguidelines-*,
  misc-*,
  modernize-*,
  performance-*,
  portability-*,
  readability-*
WarningsAsErrors: '*'
HeaderFilterRegex: '.*phoenix.*'
```

### .editorconfig

```ini
root = true

[*]
charset = utf-8
end_of_line = lf
insert_final_newline = true
trim_trailing_whitespace = true
indent_style = space
indent_size = 4

[*.hpp]
max_line_length = 100

[*.cpp]
max_line_length = 100
```

---

*Last Updated: 2026-03-26*
*Version: 1.0.0*
*Compliance: MISRA C++ 2023 Full*
