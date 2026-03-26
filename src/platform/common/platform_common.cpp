#include "phoenix/platform/platform_types.hpp"
#include "phoenix/platform/application.hpp"
#include <cstring>

namespace phoenix {
namespace platform {

// ============================================================================
// 平台能力检测
// ============================================================================

PlatformCapabilities Window::getMonitorsCapabilities() {
    PlatformCapabilities caps;
    
#if defined(PHOENIX_PLATFORM_WINDOWS)
    caps.preferredBackend = RenderBackendType::DirectX12;
    caps.supportsDX12 = true;
    caps.supportsVulkan = true;
    caps.supportsWindowed = true;
    caps.supportsFullscreen = true;
    caps.supportsMultiMonitor = true;
    caps.supportsHighDPI = true;
    caps.supportsKeyboard = true;
    caps.supportsMouse = true;
    caps.supportsGamepad = true;
    caps.maxMemoryMB = 16384;
    caps.recommendedMemoryMB = 8192;
    
#elif defined(PHOENIX_PLATFORM_MACOS)
    caps.preferredBackend = RenderBackendType::Metal;
    caps.supportsMetal = true;
    caps.supportsVulkan = false;  // macOS 不支持 Vulkan
    caps.supportsWindowed = true;
    caps.supportsFullscreen = true;
    caps.supportsMultiMonitor = true;
    caps.supportsHighDPI = true;
    caps.supportsKeyboard = true;
    caps.supportsMouse = true;
    caps.supportsTrackpad = true;
    caps.supportsGamepad = true;
    caps.maxMemoryMB = 16384;
    caps.recommendedMemoryMB = 8192;
    
#elif defined(PHOENIX_PLATFORM_IOS)
    caps.preferredBackend = RenderBackendType::Metal;
    caps.supportsMetal = true;
    caps.supportsWindowed = false;
    caps.supportsFullscreen = true;
    caps.supportsHighDPI = true;
    caps.supportsTouch = true;
    caps.supportsAccelerometer = true;
    caps.supportsGyroscope = true;
    caps.supportsGamepad = true;
    caps.maxMemoryMB = 4096;  // iOS 设备内存限制
    caps.recommendedMemoryMB = 2048;
    caps.targetFPS = 60;
    
#elif defined(PHOENIX_PLATFORM_LINUX)
    caps.preferredBackend = RenderBackendType::Vulkan;
    caps.supportsVulkan = true;
    caps.supportsWindowed = true;
    caps.supportsFullscreen = true;
    caps.supportsMultiMonitor = true;
    caps.supportsHighDPI = true;
    caps.supportsKeyboard = true;
    caps.supportsMouse = true;
    caps.supportsGamepad = true;
    caps.maxMemoryMB = 32768;
    caps.recommendedMemoryMB = 16384;
    
#elif defined(PHOENIX_PLATFORM_ANDROID)
    caps.preferredBackend = RenderBackendType::Vulkan;
    caps.supportsVulkan = true;
    caps.supportsWindowed = false;
    caps.supportsFullscreen = true;
    caps.supportsHighDPI = true;
    caps.supportsTouch = true;
    caps.supportsAccelerometer = true;
    caps.supportsGyroscope = true;
    caps.supportsGamepad = true;
    caps.maxMemoryMB = 4096;
    caps.recommendedMemoryMB = 2048;
    caps.targetFPS = 60;
    caps.enablePowerSaving = true;
    
#elif defined(PHOENIX_PLATFORM_WEB)
    caps.preferredBackend = RenderBackendType::WebGPU;
    caps.supportsWebGPU = true;
    caps.supportsWindowed = false;
    caps.supportsFullscreen = true;
    caps.supportsHighDPI = true;
    caps.supportsKeyboard = true;
    caps.supportsMouse = true;
    caps.supportsTouch = true;
    caps.supportsGamepad = true;
    caps.maxMemoryMB = 2048;  // Web 内存限制
    caps.recommendedMemoryMB = 1024;
    caps.targetFPS = 60;
#else
    caps.preferredBackend = RenderBackendType::Null;
    caps.maxMemoryMB = 1024;
    caps.recommendedMemoryMB = 512;
#endif
    
    return caps;
}

// ============================================================================
// 系统信息收集
// ============================================================================

void Application::collectSystemInfo() {
#if defined(PHOENIX_PLATFORM_WINDOWS)
    systemInfo_.osName = "Windows";
    // TODO: 获取详细 Windows 版本信息
    
#elif defined(PHOENIX_PLATFORM_MACOS)
    systemInfo_.osName = "macOS";
    
#elif defined(PHOENIX_PLATFORM_IOS)
    systemInfo_.osName = "iOS";
    
#elif defined(PHOENIX_PLATFORM_LINUX)
    systemInfo_.osName = "Linux";
    
#elif defined(PHOENIX_PLATFORM_ANDROID)
    systemInfo_.osName = "Android";
    
#elif defined(PHOENIX_PLATFORM_WEB)
    systemInfo_.osName = "Web";
#endif
    
    // CPU 信息
#ifdef _WIN32
    // Windows CPU 检测
#else
    // Linux/Unix CPU 检测
    systemInfo_.cpuCores = sysconf(_SC_NPROCESSORS_ONLN);
#endif
    
    // 内存信息
#ifdef _WIN32
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    if (GlobalMemoryStatusEx(&status)) {
        systemInfo_.totalMemoryMB = status.ullTotalPhys / (1024 * 1024);
        systemInfo_.availableMemoryMB = status.ullAvailPhys / (1024 * 1024);
    }
#else
    long pages = sysconf(_SC_PHYS_PAGES);
    long pageSize = sysconf(_SC_PAGESIZE);
    if (pages > 0 && pageSize > 0) {
        systemInfo_.totalMemoryMB = (pages * pageSize) / (1024 * 1024);
    }
#endif
}

// ============================================================================
// 平台能力检测
// ============================================================================

void Application::detectCapabilities() {
    capabilities_ = Window::getMonitorsCapabilities();
    
    // 根据平台调整配置
    if (capabilities_.maxMemoryMB < 1024) {
        // 低端设备
        config_.enableLowMemoryMode = true;
        config_.maxMemoryMB = capabilities_.maxMemoryMB;
    }
    
    if (capabilities_.supportsTouch) {
        // 移动设备优化
        config_.enablePowerSaving = true;
    }
}

} // namespace platform
} // namespace phoenix
