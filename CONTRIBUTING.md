# Contributing to Phoenix Engine

First off, thank you for considering contributing to Phoenix Engine! It's people like you that make Phoenix Engine such a great project.

## 📋 Table of Contents

- [Code of Conduct](#code-of-conduct)
- [How Can I Contribute?](#how-can-i-contribute)
- [Development Setup](#development-setup)
- [Coding Standards](#coding-standards)
- [Pull Request Process](#pull-request-process)
- [Issue Guidelines](#issue-guidelines)

## Code of Conduct

This project and everyone participating in it is governed by our [Code of Conduct](CODE_OF_CONDUCT.md). By participating, you are expected to uphold this code. Please report unacceptable behavior to phoenix-engine-maintainers@example.com.

## How Can I Contribute?

### Reporting Bugs

Before creating bug reports, please check the existing issues as you might find out that you don't need to create one. When you are creating a bug report, please include as many details as possible:

* **Use a clear and descriptive title**
* **Describe the exact steps to reproduce the problem**
* **Provide specific examples to demonstrate the steps**
* **Describe the behavior you observed and what behavior you expected**
* **Include screenshots if possible**
* **Include system information** (OS, GPU, driver version, etc.)

### Suggesting Enhancements

Enhancement suggestions are tracked as GitHub issues. When creating an enhancement suggestion, please include:

* **Use a clear and descriptive title**
* **Provide a detailed description of the suggested enhancement**
* **Explain why this enhancement would be useful**
* **List some examples of how this enhancement would be used**

### Pull Requests

* Fill in the required template
* Follow the coding standards
* Include tests when applicable
* Update documentation as needed
* Ensure all tests pass

## Development Setup

### Prerequisites

- CMake 3.20+
- C++17 compatible compiler (GCC 9+, Clang 10+, MSVC 2019+)
- Git
- Vulkan SDK (for Vulkan backend)

### Building from Source

```bash
# Clone the repository
git clone https://github.com/phoenix-engine/phoenix-engine.git
cd phoenix-engine

# Create build directory
mkdir build && cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build . -j$(nproc)

# Run tests
ctest --output-on-failure
```

### Platform-Specific Setup

#### Windows
```powershell
# Install Visual Studio 2019+ with C++ workload
# Install Vulkan SDK from https://vulkan.lunarg.com/
# Install CMake from https://cmake.org/
```

#### Linux (Ubuntu/Debian)
```bash
sudo apt-get install build-essential cmake git \
    libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
    libgl1-mesa-dev libvulkan-dev
```

#### macOS
```bash
brew install cmake ninja
# Xcode with command line tools required
```

#### Android
```bash
# Install Android Studio
# Install NDK via SDK Manager
export ANDROID_NDK_HOME=/path/to/ndk
```

#### iOS
```bash
# Xcode 13+ required
# iOS SDK 15.0+
```

#### Web/WASM
```bash
# Install Emscripten SDK
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk && ./emsdk install latest && ./emsdk activate latest
source ./emsdk_env.sh
```

## Coding Standards

### C++ Code Style

We follow a modified Google C++ Style Guide:

* **Indentation**: 4 spaces (no tabs)
* **Line length**: 100 characters maximum
* **Naming conventions**:
  - Classes/Structs: `PascalCase`
  - Functions/Methods: `snake_case`
  - Variables: `snake_case`
  - Constants: `UPPER_SNAKE_CASE`
  - Private members: `m_` prefix
* **Smart pointers**: Use `std::unique_ptr` by default, `std::shared_ptr` when necessary
* **RAII**: Always use RAII for resource management

Example:
```cpp
class ResourceManager {
public:
    explicit ResourceManager(const std::string& resource_path);
    ~ResourceManager();
    
    bool load_texture(const std::string& name);
    void unload_all();
    
private:
    std::string m_resource_path;
    std::unordered_map<std::string, TexturePtr> m_textures;
};
```

### Rust Code Style

We follow the official [Rust Style Guidelines](https://doc.rust-lang.org/1.0.0/style/):

* Use `cargo fmt` to format code
* Use `cargo clippy` for linting
* Follow Rust naming conventions
* Document public APIs with rustdoc

### Documentation

* Use Doxygen-style comments for C++ code
* Include examples for public APIs
* Keep documentation up to date with code changes

Example:
```cpp
/**
 * @brief Load a texture from file
 * 
 * @param path Path to the texture file
 * @param format Expected pixel format
 * @return true if loading succeeded, false otherwise
 * 
 * @throws ResourceException if file cannot be opened
 * 
 * Supported formats: PNG, JPEG, BMP, TGA, DDS
 */
bool load_texture(const std::string& path, PixelFormat format);
```

## Pull Request Process

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Make your changes
4. Run tests and ensure they pass
5. Update documentation as needed
6. Commit your changes (`git commit -m 'Add amazing feature'`)
7. Push to your branch (`git push origin feature/amazing-feature`)
8. Open a Pull Request

### PR Checklist

- [ ] Code follows project style guidelines
- [ ] Self-review of code performed
- [ ] Code is commented where necessary
- [ ] Documentation updated if needed
- [ ] Tests added/updated and passing
- [ ] No new compiler warnings
- [ ] CHANGELOG.md updated (if applicable)

## Issue Guidelines

### Issue Labels

We use the following labels to categorize issues:

* `bug` - Something isn't working
* `enhancement` - New feature or request
* `documentation` - Documentation improvements
* `good first issue` - Good for newcomers
* `help wanted` - Extra attention is needed
* `priority: high` - High priority
* `priority: low` - Low priority
* `status: in progress` - Currently being worked on
* `status: needs triage` - Needs review

### Issue Template

When creating an issue, please use the appropriate template:

* **Bug Report**: Describe the bug, reproduction steps, expected behavior
* **Feature Request**: Describe the feature, use case, proposed solution
* **Documentation**: Describe what's missing or unclear

## Testing

### Running Tests

```bash
# Build and run all tests
cd build
ctest --output-on-failure

# Run specific test
ctest -R test_math --output-on-failure

# Run with coverage
cmake .. -DPHoenix_COVERAGE=ON
cmake --build .
ctest
```

### Writing Tests

* Use the Catch2 testing framework
* Name test files: `test_<module>.cpp`
* Group related tests in test cases
* Test both success and failure cases

Example:
```cpp
TEST_CASE("Vector3 addition", "[math]") {
    Vector3 a(1.0f, 2.0f, 3.0f);
    Vector3 b(4.0f, 5.0f, 6.0f);
    Vector3 c = a + b;
    
    REQUIRE(c.x == Approx(5.0f));
    REQUIRE(c.y == Approx(7.0f));
    REQUIRE(c.z == Approx(9.0f));
}
```

## Release Process

Releases follow semantic versioning (MAJOR.MINOR.PATCH):

* **MAJOR**: Breaking changes
* **MINOR**: New features (backward compatible)
* **PATCH**: Bug fixes (backward compatible)

See [RELEASE_PROCESS.md](docs/RELEASE_PROCESS.md) for detailed release procedures.

## Questions?

Feel free to open an issue for any questions or join our [Discussions](https://github.com/phoenix-engine/phoenix-engine/discussions).

Thank you for contributing to Phoenix Engine! 🦄
