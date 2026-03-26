#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <memory>
#include <functional>
#include <array>

namespace phoenix {
namespace platform {

// ============================================================================
// 平台检测宏
// ============================================================================

#if defined(_WIN32) || defined(_WIN64)
    #define PHOENIX_PLATFORM_WINDOWS 1
    #define PHOENIX_PLATFORM_NAME "Windows"
#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE
        #define PHOENIX_PLATFORM_IOS 1
        #define PHOENIX_PLATFORM_NAME "iOS"
    #else
        #define PHOENIX_PLATFORM_MACOS 1
        #define PHOENIX_PLATFORM_NAME "macOS"
    #endif
#elif defined(__ANDROID__)
    #define PHOENIX_PLATFORM_ANDROID 1
    #define PHOENIX_PLATFORM_NAME "Android"
#elif defined(__linux__)
    #define PHOENIX_PLATFORM_LINUX 1
    #define PHOENIX_PLATFORM_NAME "Linux"
#elif defined(__EMSCRIPTEN__)
    #define PHOENIX_PLATFORM_WEB 1
    #define PHOENIX_PLATFORM_NAME "Web"
#else
    #define PHOENIX_PLATFORM_UNKNOWN 1
    #define PHOENIX_PLATFORM_NAME "Unknown"
#endif

// ============================================================================
// 渲染后端类型
// ============================================================================

enum class RenderBackendType : uint8_t {
    None = 0,
    Vulkan,
    DirectX11,
    DirectX12,
    Metal,
    OpenGL,
    OpenGL_ES,
    WebGL,
    WebGL2,
    WebGPU,
    Null
};

inline const char* renderBackendToString(RenderBackendType backend) {
    switch (backend) {
        case RenderBackendType::Vulkan: return "Vulkan";
        case RenderBackendType::DirectX11: return "DirectX11";
        case RenderBackendType::DirectX12: return "DirectX12";
        case RenderBackendType::Metal: return "Metal";
        case RenderBackendType::OpenGL: return "OpenGL";
        case RenderBackendType::OpenGL_ES: return "OpenGL ES";
        case RenderBackendType::WebGL: return "WebGL";
        case RenderBackendType::WebGL2: return "WebGL2";
        case RenderBackendType::WebGPU: return "WebGPU";
        case RenderBackendType::Null: return "Null";
        default: return "Unknown";
    }
}

// ============================================================================
// 平台能力
// ============================================================================

struct PlatformCapabilities {
    // 渲染能力
    RenderBackendType preferredBackend = RenderBackendType::None;
    bool supportsVulkan = false;
    bool supportsDX12 = false;
    bool supportsMetal = false;
    bool supportsWebGPU = false;
    bool supportsComputeShaders = false;
    bool supportsTessellation = false;
    bool supportsRayTracing = false;
    
    // 窗口系统
    bool supportsWindowed = false;
    bool supportsFullscreen = false;
    bool supportsMultiMonitor = false;
    bool supportsHighDPI = false;
    
    // 输入
    bool supportsKeyboard = false;
    bool supportsMouse = false;
    bool supportsTouch = false;
    bool supportsGamepad = false;
    bool supportsAccelerometer = false;
    bool supportsGyroscope = false;
    
    // 内存约束 (MB)
    uint32_t maxMemoryMB = 4096;
    uint32_t recommendedMemoryMB = 2048;
    
    // 性能目标
    uint32_t targetFPS = 60;
    bool supportsVSync = true;
    bool supportsAdaptiveSync = false;
    
    // 特性
    bool supportsAsyncCompute = false;
    bool supportsVariableRateShading = false;
    bool supportsSamplerFeedback = false;
};

// ============================================================================
// 窗口句柄
// ============================================================================

struct NativeWindowHandle {
#if defined(PHOENIX_PLATFORM_WINDOWS)
    void* hwnd = nullptr;           // HWND
    void* hinstance = nullptr;      // HINSTANCE
#elif defined(PHOENIX_PLATFORM_MACOS)
    void* nsWindow = nullptr;       // NSWindow*
    void* nsView = nullptr;         // NSView*
#elif defined(PHOENIX_PLATFORM_IOS)
    void* uiWindow = nullptr;       // UIWindow*
    void* uiView = nullptr;         // UIView*
#elif defined(PHOENIX_PLATFORM_LINUX)
    void* xDisplay = nullptr;       // Display*
    void* xWindow = nullptr;        // Window
    void* waylandDisplay = nullptr; // wl_display*
    void* waylandSurface = nullptr; // wl_surface*
#elif defined(PHOENIX_PLATFORM_ANDROID)
    void* surface = nullptr;        // ANativeWindow*
#elif defined(PHOENIX_PLATFORM_WEB)
    int32_t canvasId = 0;           // Emscripten canvas ID
#else
    void* handle = nullptr;
#endif
};

// ============================================================================
// 表面句柄 (用于渲染)
// ============================================================================

struct NativeSurfaceHandle {
#if defined(PHOENIX_PLATFORM_WINDOWS)
    void* dxgiFactory = nullptr;    // IDXGIFactory*
    void* dxgiAdapter = nullptr;    // IDXGIAdapter*
#elif defined(PHOENIX_PLATFORM_MACOS) || defined(PHOENIX_PLATFORM_IOS)
    void* metalDevice = nullptr;    // id<MTLDevice>
    void* metalLayer = nullptr;     // CAMetalLayer*
#elif defined(PHOENIX_PLATFORM_LINUX)
    void* vkSurface = nullptr;      // VkSurfaceKHR
#elif defined(PHOENIX_PLATFORM_ANDROID)
    void* vkSurface = nullptr;      // VkSurfaceKHR
#elif defined(PHOENIX_PLATFORM_WEB)
    void* gpuAdapter = nullptr;     // WGPUAdapter
    void* gpuDevice = nullptr;      // WGPUDevice
#else
    void* handle = nullptr;
#endif
};

// ============================================================================
// 输入事件
// ============================================================================

enum class EventType : uint8_t {
    None = 0,
    
    // 窗口事件
    WindowCreated,
    WindowDestroyed,
    WindowResized,
    WindowMoved,
    WindowFocused,
    WindowUnfocused,
    WindowMinimized,
    WindowMaximized,
    WindowRestored,
    WindowCloseRequested,
    
    // 键盘事件
    KeyDown,
    KeyUp,
    KeyRepeat,
    TextInput,
    
    // 鼠标事件
    MouseMoved,
    MouseButtonDown,
    MouseButtonUp,
    MouseWheel,
    
    // 触摸事件
    TouchBegan,
    TouchMoved,
    TouchEnded,
    TouchCancelled,
    
    // 游戏手柄事件
    GamepadConnected,
    GamepadDisconnected,
    GamepadButton,
    GamepadAxis,
    
    // 传感器事件
    Accelerometer,
    Gyroscope,
    Magnetometer,
    Orientation,
    
    // 应用事件
    AppSuspend,
    AppResume,
    AppTerminate,
    AppMemoryWarning,
};

enum class KeyCode : uint16_t {
    Unknown = 0,
    
    // 字母
    A = 4, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    
    // 数字
    Num1 = 30, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9, Num0,
    
    // 功能键
    F1 = 58, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    
    // 控制键
    Escape = 41,
    Tab = 43,
    CapsLock = 57,
    Shift = 225,
    Control = 224,
    Alt = 226,
    Super = 227,
    
    // 导航键
    Up = 82,
    Down = 81,
    Left = 80,
    Right = 79,
    
    // 其他
    Space = 44,
    Enter = 40,
    Backspace = 42,
    Delete = 76,
};

enum class MouseButton : uint8_t {
    None = 0,
    Left = 1,
    Right = 2,
    Middle = 3,
    X1 = 4,
    X2 = 5,
};

struct InputEvent {
    EventType type = EventType::None;
    uint64_t timestamp = 0;
    
    // 键盘
    KeyCode keyCode = KeyCode::Unknown;
    uint16_t scanCode = 0;
    uint8_t modifiers = 0;  // Ctrl=1, Shift=2, Alt=4, Super=8
    char32_t textChar = 0;
    
    // 鼠标/触摸
    float posX = 0.0f;
    float posY = 0.0f;
    float deltaX = 0.0f;
    float deltaY = 0.0f;
    MouseButton button = MouseButton::None;
    float pressure = 1.0f;
    int32_t touchId = -1;
    float wheelDelta = 0.0f;
    
    // 传感器
    float accelX = 0.0f, accelY = 0.0f, accelZ = 0.0f;
    float gyroX = 0.0f, gyroY = 0.0f, gyroZ = 0.0f;
    
    // 游戏手柄
    uint8_t gamepadIndex = 0;
    uint8_t gamepadButton = 0;
    float gamepadAxisValue = 0.0f;
};

// ============================================================================
// 时间信息
// ============================================================================

struct TimeInfo {
    double deltaTime = 0.0;         // 秒
    double totalTime = 0.0;         // 秒
    double appStartTime = 0.0;      // 秒
    uint64_t frameIndex = 0;
    float fps = 0.0f;
    uint32_t frameTimeMs = 0;
};

// ============================================================================
// 系统信息
// ============================================================================

struct SystemInfo {
    std::string osName;
    std::string osVersion;
    std::string cpuModel;
    uint32_t cpuCores = 0;
    uint32_t cpuThreads = 0;
    uint64_t totalMemoryMB = 0;
    uint64_t availableMemoryMB = 0;
    std::string gpuModel;
    std::string gpuVendor;
    uint64_t gpuMemoryMB = 0;
    RenderBackendType renderBackend = RenderBackendType::None;
};

// ============================================================================
// 视频模式
// ============================================================================

struct VideoMode {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t refreshRate = 60;
    uint8_t redBits = 8;
    uint8_t greenBits = 8;
    uint8_t blueBits = 8;
    uint8_t alphaBits = 8;
    uint8_t depthBits = 24;
    uint8_t stencilBits = 8;
    bool stereo = false;
};

// ============================================================================
// 监视器信息
// ============================================================================

struct MonitorInfo {
    std::string name;
    int32_t x = 0;
    int32_t y = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    float contentScaleX = 1.0f;
    float contentScaleY = 1.0f;
    VideoMode primaryMode;
    std::array<VideoMode, 16> modes;
    uint32_t modeCount = 0;
};

} // namespace platform
} // namespace phoenix
