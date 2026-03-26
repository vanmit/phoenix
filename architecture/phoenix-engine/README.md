# Phoenix Engine

**Security-First Cross-Platform Game Engine**

Phoenix Engine is a modern game engine built with C++20 and Rust, designed with security as a first-class concern. It features a data-oriented ECS architecture, bgfx-based rendering, and Rust-backed security validation.

## Features

- 🛡️ **Security First**: All external inputs validated through Rust security core
- 🎮 **ECS Architecture**: Data-oriented design for cache-friendly performance
- 🎨 **bgfx Rendering**: Cross-platform graphics abstraction
- 🔒 **Rust Security Core**: Memory-safe validation, hashing, encryption
- 📦 **Resource Management**: Async loading, caching, validation
- 🧪 **Test Coverage**: Comprehensive unit and integration tests

## Architecture

```
phoenix-engine/
├── CMakeLists.txt          # Build configuration
├── include/phoenix/        # Public API headers
│   ├── core/              # Engine core, logging, timing
│   ├── math/              # Vector, matrix, quaternion math
│   ├── render/            # Rendering system (bgfx)
│   ├── scene/             # ECS scene management
│   ├── resource/          # Asset loading and caching
│   └── security/          # Security interfaces
├── src/                    # Implementation files
├── tests/                  # Unit and integration tests
├── examples/               # Example applications
├── rust-security-core/     # Rust security library
└── docs/                   # Documentation
    ├── API-DESIGN.md       # API design document
    └── CODING-STANDARDS.md # MISRA C++ 2023 standards
```

## Building

### Prerequisites

- CMake 3.20+
- C++20 compatible compiler (GCC 11+, Clang 13+, MSVC 2022+)
- Rust 1.70+ (for security core)
- Git

### Build Steps

```bash
# Clone the repository
git clone https://github.com/phoenix-engine/phoenix-engine.git
cd phoenix-engine

# Create build directory
mkdir build && cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . --config Release

# Run tests
ctest --config Release
```

### Cross-Platform

Phoenix Engine supports:
- **Windows**: Visual Studio 2022+, MinGW
- **Linux**: GCC, Clang
- **macOS**: Xcode, Clang

## Usage

### Basic Example

```cpp
#include <phoenix/core/engine.hpp>
#include <phoenix/scene/scene.hpp>

using namespace phoenix;

int main() {
    // Initialize engine
    core::EngineConfig config;
    config.title = "My Phoenix App";
    config.width = 1920;
    config.height = 1080;
    
    auto& engine = core::Engine::instance();
    PHOENIX_CHECK(engine.initialize(config));
    
    // Create scene
    auto& scene = engine.getSceneSystem();
    auto entity = scene.createEntity("Player");
    
    // Run engine
    PHOENIX_CHECK(engine.run());
    
    return 0;
}
```

### Security Validation

```cpp
#include <phoenix/security/security_manager.hpp>

using namespace phoenix::security;

// Validate file before loading
auto& security = SecurityManager::instance();
PHOENIX_CHECK(security.validateFile("assets/texture.png", ValidationLevel::Strict));

// Load safely
auto texture = resourceManager.load<Texture>("assets/texture.png");
```

## API Documentation

- [API Design](docs/API-DESIGN.md) - Core interface specifications
- [Coding Standards](docs/CODING-STANDARDS.md) - MISRA C++ 2023 compliance

## Security

Phoenix Engine follows security-first principles:

1. **Input Validation**: All external inputs validated through Rust
2. **Memory Safety**: RAII, smart pointers, no raw new/delete
3. **Bounds Checking**: All array accesses checked
4. **Integer Safety**: Overflow checks on arithmetic
5. **Path Validation**: No directory traversal attacks

See [CODING-STANDARDS.md](docs/CODING-STANDARDS.md) for details.

## Testing

```bash
# Run all tests
ctest

# Run specific test category
ctest -R Security
ctest -R Math
ctest -R Render

# With coverage
cmake .. -DPHOENIX_COVERAGE=ON
cmake --build .
ctest
```

## Contributing

1. Follow [CODING-STANDARDS.md](docs/CODING-STANDARDS.md)
2. Add tests for new features
3. Ensure MISRA compliance
4. Update documentation

## License

MIT License - see LICENSE file for details.

## Version

- **Version**: 1.0.0
- **C++ Standard**: C++20
- **MISRA Compliance**: C++ 2023 Full
