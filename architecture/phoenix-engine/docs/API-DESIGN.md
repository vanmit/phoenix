# Phoenix Engine - API Design Document

## 1. Overview

Phoenix Engine is a security-first, cross-platform game engine designed with modern C++20 and Rust integration. This document outlines the core API design principles and interfaces.

## 2. Design Principles

### 2.1 Core Tenets

1. **Security First**: All external inputs must be validated through the Rust security core
2. **Data-Oriented Design**: Cache-friendly, ECS-based architecture
3. **Zero-Copy Where Possible**: Minimize memory allocations and copies
4. **Explicit Error Handling**: No exceptions in core paths, use `Result<T>` pattern
5. **Thread Safety**: All public APIs are thread-safe by default

### 2.2 Naming Conventions

- **Classes/Structs**: PascalCase (e.g., `EntityManager`, `RenderPipeline`)
- **Functions**: camelCase (e.g., `createEntity()`, `loadAsset()`)
- **Constants**: UPPER_SNAKE_CASE (e.g., `MAX_ENTITIES`, `DEFAULT_FOV`)
- **Private members**: m_prefix (e.g., `m_renderer`, `m_entities`)
- **Namespaces**: Lowercase with nested structure (e.g., `phoenix::core::memory`)

## 3. Core API

### 3.1 Engine Interface

```cpp
namespace phoenix::core {

class Engine {
public:
    // Singleton access
    static Engine& instance();
    
    // Lifecycle
    Result<void> initialize(const EngineConfig& config);
    Result<void> run();
    void shutdown();
    
    // State
    bool isRunning() const;
    f32 getDeltaTime() const;
    f64 getTimestamp() const;
    
    // Systems access
    RenderSystem& getRenderSystem();
    SceneSystem& getSceneSystem();
    ResourceSystem& getResourceSystem();
    SecurityManager& getSecurityManager();
    
private:
    Engine() = default;
    ~Engine() = default;
    
    // Non-copyable, non-movable
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&&) = delete;
    Engine& operator=(Engine&&) = delete;
    
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace phoenix::core
```

### 3.2 Configuration

```cpp
namespace phoenix::core {

struct EngineConfig {
    std::string title = "Phoenix Engine";
    u32 width = 1920;
    u32 height = 1080;
    bool fullscreen = false;
    bool vsync = true;
    u32 maxFps = 60;
    
    // Security settings
    bool enableValidation = true;
    SecurityLevel securityLevel = SecurityLevel::High;
    
    // Logging
    LogLevel logLevel = LogLevel::Info;
    std::string logFile = "phoenix.log";
};

enum class SecurityLevel : u8 {
    Minimal = 0,    // Development only
    Standard = 1,   // Default
    High = 2,       // Production
    Maximum = 3     // Security-critical
};

} // namespace phoenix::core
```

## 4. Math API

### 4.1 Vector Types

```cpp
namespace phoenix::math {

template<typename T, u32 N>
struct Vector {
    T data[N];
    
    constexpr Vector() noexcept;
    constexpr Vector(T value) noexcept;  // Splat
    constexpr Vector(const std::array<T, N>& arr) noexcept;
    
    // Access
    constexpr T& operator[](u32 index);
    constexpr const T& operator[](u32 index) const;
    
    // Operations
    constexpr Vector operator+(const Vector& other) const;
    constexpr Vector operator-(const Vector& other) const;
    constexpr Vector operator*(T scalar) const;
    constexpr Vector operator/(T scalar) const;
    
    // Methods
    constexpr T length() const;
    constexpr T lengthSquared() const;
    constexpr Vector normalized() const;
    constexpr T dot(const Vector& other) const;
    constexpr Vector cross(const Vector& other) const;  // 3D only
};

using Vector2 = Vector<f32, 2>;
using Vector3 = Vector<f32, 3>;
using Vector4 = Vector<f32, 4>;
using Vector2d = Vector<f64, 2>;
using Vector3d = Vector<f64, 3>;
using Vector4d = Vector<f64, 4>;

} // namespace phoenix::math
```

### 4.2 Matrix4

```cpp
namespace phoenix::math {

class Matrix4 {
public:
    Matrix4() noexcept;  // Identity
    Matrix4(const f32* data) noexcept;
    
    // Static constructors
    static Matrix4 identity();
    static Matrix4 translation(const Vector3& t);
    static Matrix4 rotation(const Vector3& axis, f32 angle);
    static Matrix4 rotationX(f32 angle);
    static Matrix4 rotationY(f32 angle);
    static Matrix4 rotationZ(f32 angle);
    static Matrix4 scale(const Vector3& s);
    static Matrix4 perspective(f32 fov, f32 aspect, f32 near, f32 far);
    static Matrix4 orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far);
    static Matrix4 lookAt(const Vector3& eye, const Vector3& target, const Vector3& up);
    
    // Operations
    Matrix4 operator*(const Matrix4& other) const;
    Vector4 operator*(const Vector4& vec) const;
    Matrix4 transpose() const;
    Matrix4 inverse() const;
    
    // Access
    f32* data();
    const f32* data() const;
    
private:
    f32 m_data[16];
};

} // namespace phoenix::math
```

### 4.3 Quaternion

```cpp
namespace phoenix::math {

class Quaternion {
public:
    Quaternion() noexcept;  // Identity
    Quaternion(f32 x, f32 y, f32 z, f32 w) noexcept;
    
    // Static constructors
    static Quaternion identity();
    static Quaternion fromAxisAngle(const Vector3& axis, f32 angle);
    static Quaternion fromEuler(const Vector3& euler);
    static Quaternion fromRotationMatrix(const Matrix4& mat);
    
    // Operations
    Quaternion operator*(const Quaternion& other) const;
    Vector3 operator*(const Vector3& vec) const;
    Quaternion conjugate() const;
    Quaternion inverse() const;
    Quaternion normalized() const;
    
    // Interpolation
    static Quaternion slerp(const Quaternion& a, const Quaternion& b, f32 t);
    
    // Conversion
    Matrix4 toMatrix4() const;
    Vector3 toEuler() const;
    
private:
    f32 m_x, m_y, m_z, m_w;
};

} // namespace phoenix::math
```

## 5. Render API

### 5.1 Renderer

```cpp
namespace phoenix::render {

class Renderer {
public:
    Result<void> initialize(const RenderConfig& config);
    void shutdown();
    
    // Frame management
    Result<void> beginFrame();
    Result<void> endFrame();
    
    // Rendering
    void submit(const RenderBatch& batch);
    void submitMesh(const Mesh& mesh, const Transform& transform, const Material& material);
    
    // View management
    ViewId createView(const ViewConfig& config);
    void destroyView(ViewId id);
    void setViewTransform(ViewId id, const Matrix4& view, const Matrix4& proj);
    
    // Framebuffer
    FramebufferId createFramebuffer(const FramebufferConfig& config);
    void setFramebuffer(FramebufferId id);
    
    // State
    void setClearColor(const Color& color);
    void setViewport(u32 x, u32 y, u32 width, u32 height);
    void enableDepthTest(bool enable);
    void enableBlend(bool enable);
    
private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace phoenix::render
```

### 5.2 Mesh

```cpp
namespace phoenix::render {

struct Vertex {
    Vector3 position;
    Vector3 normal;
    Vector2 texCoord;
    Vector4 tangent;
    Color color;
};

class Mesh {
public:
    Result<void> create(const MeshData& data);
    void destroy();
    
    // Access
    const Vertex* vertices() const;
    u32 vertexCount() const;
    const u16* indices() const;
    u32 indexCount() const;
    
    // Bounds
    const BoundingBox& bounds() const;
    
    // Upload to GPU
    Result<void> upload();
    
private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<u16> indices;
    Topology topology = Topology::TriangleList;
};

} // namespace phoenix::render
```

## 6. Scene API (ECS)

### 6.1 Entity System

```cpp
namespace phoenix::scene {

using EntityId = u64;
using ComponentId = u32;

class Scene {
public:
    // Entity management
    EntityId createEntity();
    EntityId createEntity(const std::string& name);
    void destroyEntity(EntityId id);
    bool isValid(EntityId id) const;
    
    // Component management
    template<typename T>
    T& addComponent(EntityId id);
    
    template<typename T>
    T* getComponent(EntityId id);
    
    template<typename T>
    const T* getComponent(EntityId id) const;
    
    template<typename T>
    void removeComponent(EntityId id);
    
    template<typename T>
    bool hasComponent(EntityId id) const;
    
    // Query
    template<typename... Components>
    Query<Components...> query();
    
    // Hierarchy
    void setParent(EntityId child, EntityId parent);
    EntityId getParent(EntityId id) const;
    const std::vector<EntityId>& getChildren(EntityId id) const;
    
    // Access
    const std::string& getName(EntityId id) const;
    void setName(EntityId id, const std::string& name);
    
private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace phoenix::scene
```

### 6.2 Components

```cpp
namespace phoenix::scene {

struct Transform {
    Vector3 position = Vector3(0.0f);
    Quaternion rotation = Quaternion::identity();
    Vector3 scale = Vector3(1.0f);
    
    Matrix4 localMatrix() const;
    Matrix4 worldMatrix() const;
    Vector3 forward() const;
    Vector3 up() const;
    Vector3 right() const;
};

struct MeshRenderer {
    render::MeshId mesh;
    render::MaterialId material;
    bool castShadows = true;
    bool receiveShadows = true;
};

struct Camera {
    f32 fov = 90.0f;
    f32 aspect = 16.0f / 9.0f;
    f32 nearPlane = 0.1f;
    f32 farPlane = 1000.0f;
    bool orthographic = false;
    f32 orthoSize = 10.0f;
    
    Matrix4 projectionMatrix() const;
    Matrix4 viewMatrix(const Transform& transform) const;
};

struct Light {
    enum class Type : u8 {
        Directional,
        Point,
        Spot
    };
    
    Type type = Type::Directional;
    Color color = Color(1.0f, 1.0f, 1.0f);
    f32 intensity = 1.0f;
    f32 range = 10.0f;      // Point/Spot
    f32 spotAngle = 45.0f;  // Spot
};

} // namespace phoenix::scene
```

## 7. Resource API

### 7.1 Resource Manager

```cpp
namespace phoenix::resource {

template<typename T>
using ResourceHandle = std::shared_ptr<T>;

class ResourceManager {
public:
    // Loading
    template<typename T>
    Result<ResourceHandle<T>> load(const std::string& path);
    
    template<typename T>
    Result<ResourceHandle<T>> loadAsync(const std::string& path);
    
    // Cache access
    template<typename T>
    ResourceHandle<T> get(const std::string& path) const;
    
    // Unloading
    void unload(const std::string& path);
    void unloadAll();
    
    // Async operations
    void update(f32 deltaTime);
    bool isLoadingComplete() const;
    
    // Validation (through Rust security core)
    Result<void> validateAsset(const std::string& path);
    
private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace phoenix::resource
```

### 7.2 Asset Types

```cpp
namespace phoenix::resource {

class Texture {
public:
    Result<void> loadFromFile(const std::string& path);
    Result<void> loadFromMemory(const u8* data, size_t size);
    
    u32 width() const;
    u32 height() const;
    u32 mipLevels() const;
    TextureFormat format() const;
    
    render::TextureId gpuHandle() const;
    
private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

class Shader {
public:
    Result<void> loadFromFile(const std::string& vertexPath, const std::string& fragmentPath);
    Result<void> loadFromSource(const std::string& vertexSrc, const std::string& fragmentSrc);
    
    render::ShaderId gpuHandle() const;
    
    // Uniform binding
    void setUniform(const std::string& name, const Vector3& value);
    void setUniform(const std::string& name, const Matrix4& value);
    void setUniform(const std::string& name, f32 value);
    void setUniform(const std::string& name, s32 value);
    
private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace phoenix::resource
```

## 8. Security API

### 8.1 Security Manager

```cpp
namespace phoenix::security {

class SecurityManager {
public:
    static SecurityManager& instance();
    
    // Initialization (must be called first)
    Result<void> initialize(const SecurityConfig& config);
    void shutdown();
    
    // Validation (Rust-backed)
    Result<void> validateFile(const std::string& path, ValidationLevel level);
    Result<void> validateMemory(const void* data, size_t size);
    Result<void> validateNetwork(const std::string& url);
    
    // Encryption
    Result<std::vector<u8>> encrypt(const std::vector<u8>& data, const Key& key);
    Result<std::vector<u8>> decrypt(const std::vector<u8>& data, const Key& key);
    
    // Hashing
    Result<std::vector<u8>> hash(const std::vector<u8>& data, HashAlgorithm algo);
    
    // Secure random
    Result<void> randomBytes(std::vector<u8>& buffer);
    
    // Audit logging
    void logSecurityEvent(const SecurityEvent& event);
    const std::vector<SecurityEvent>& getAuditLog() const;
    
private:
    SecurityManager() = default;
    ~SecurityManager() = default;
    
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace phoenix::security
```

### 8.2 Security Types

```cpp
namespace phoenix::security {

enum class ValidationLevel : u8 {
    None = 0,      // Skip validation
    Basic = 1,     // Format check only
    Standard = 2,  // Format + content validation
    Strict = 3     // Full validation with Rust security core
};

enum class HashAlgorithm : u8 {
    SHA256,
    SHA512,
    BLAKE3
};

struct SecurityConfig {
    ValidationLevel defaultLevel = ValidationLevel::Strict;
    bool enableAuditLog = true;
    u32 maxAuditLogSize = 10000;
    std::string allowedPaths;  // Whitelist
    std::string blockedPaths;  // Blacklist
};

struct SecurityEvent {
    enum class Type : u8 {
        ValidationSuccess,
        ValidationFailure,
        EncryptionOperation,
        DecryptionOperation,
        AccessDenied,
        SuspiciousActivity
    };
    
    Type type;
    f64 timestamp;
    std::string description;
    std::string source;
    bool critical;
};

} // namespace phoenix::security
```

## 9. Error Handling

### 9.1 Result Type

```cpp
namespace phoenix {

template<typename T>
class Result {
public:
    // Success construction
    Result(T value);
    Result& operator=(T value);
    
    // Error construction
    static Result error(ErrorCode code, const std::string& message);
    
    // State checks
    bool isSuccess() const;
    bool isError() const;
    
    // Access (undefined behavior if error)
    T& value();
    const T& value() const;
    T valueOr(T defaultValue) const;
    
    // Error access
    ErrorCode errorCode() const;
    const std::string& errorMessage() const;
    
    // Monadic operations
    template<typename F>
    auto map(F&& func) const;
    
    template<typename F>
    Result<T> andThen(F&& func) const;
    
private:
    // Implementation uses std::variant or similar
};

enum class ErrorCode : u32 {
    Success = 0,
    InvalidParameter = 1,
    NotFound = 2,
    AlreadyExists = 3,
    OutOfMemory = 4,
    InvalidState = 5,
    SecurityViolation = 6,
    IOError = 7,
    NetworkError = 8,
    ValidationError = 9,
    InternalError = 100
};

} // namespace phoenix
```

## 10. Threading Model

### 10.1 Thread Safety Guarantees

- **Engine**: Single-threaded (main thread only)
- **Scene**: Thread-safe for component access, entity creation requires main thread
- **Renderer**: Thread-safe for command submission, initialization on main thread
- **ResourceManager**: Thread-safe, async loading on worker threads
- **SecurityManager**: Thread-safe, all operations are immutable

### 10.2 Task System

```cpp
namespace phoenix::core {

class TaskSystem {
public:
    static TaskSystem& instance();
    
    Result<void> initialize(u32 workerThreadCount);
    void shutdown();
    
    // Task submission
    TaskId submit(std::function<void()> task);
    TaskId submit(std::function<void()> task, TaskPriority priority);
    
    // Wait for completion
    void wait(TaskId id);
    void waitAll();
    
    // Futures
    template<typename T>
    Future<T> async(std::function<T()> task);
    
private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace phoenix::core
```

## 11. Version Information

```cpp
namespace phoenix {

struct Version {
    static constexpr u32 Major = PHOENIX_VERSION_MAJOR;
    static constexpr u32 Minor = PHOENIX_VERSION_MINOR;
    static constexpr u32 Patch = PHOENIX_VERSION_PATCH;
    
    static const char* string();
    static u32 packed();
};

} // namespace phoenix
```

---

## Appendix A: Type Definitions

```cpp
namespace phoenix {

// Fixed-width types
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using s8 = std::int8_t;
using s16 = std::int16_t;
using s32 = std::int32_t;
using s64 = std::int64_t;

using f32 = float;
using f64 = double;

// Handles
using Handle = u32;
using TextureId = Handle;
using ShaderId = Handle;
using MeshId = Handle;
using MaterialId = Handle;
using ViewId = Handle;
using FramebufferId = Handle;

} // namespace phoenix
```

## Appendix B: Color Types

```cpp
namespace phoenix {

struct Color {
    f32 r, g, b, a;
    
    constexpr Color() : r(0), g(0), b(0), a(1) {}
    constexpr Color(f32 rr, f32 gg, f32 bb, f32 aa = 1.0f) 
        : r(rr), g(gg), b(bb), a(aa) {}
    
    static constexpr Color Black() { return Color(0, 0, 0, 1); }
    static constexpr Color White() { return Color(1, 1, 1, 1); }
    static constexpr Color Red() { return Color(1, 0, 0, 1); }
    static constexpr Color Green() { return Color(0, 1, 0, 1); }
    static constexpr Color Blue() { return Color(0, 0, 1, 1); }
    
    // Conversions
    u32 toRGBA() const;
    u32 toABGR() const;
    
    // Operations
    Color operator+(const Color& other) const;
    Color operator*(f32 scalar) const;
    Color lerp(const Color& other, f32 t) const;
};

} // namespace phoenix
```
