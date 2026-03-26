#pragma once

#include "platform_types.hpp"
#include <functional>
#include <string>
#include <vector>

namespace phoenix {
namespace platform {

// ============================================================================
// 窗口配置
// ============================================================================

struct WindowConfig {
    std::string title = "Phoenix Engine";
    uint32_t width = 1920;
    uint32_t height = 1080;
    bool resizable = true;
    bool decorated = true;
    bool visible = true;
    bool transparent = false;
    bool alwaysOnTop = false;
    bool fullscreen = false;
    bool highDPI = true;
    bool vsync = true;
    uint32_t samples = 1;
    int32_t monitorIndex = 0;
    
    // OpenGL/WebGL 上下文配置
    uint32_t glMajorVersion = 4;
    uint32_t glMinorVersion = 5;
    bool glForwardCompat = true;
    bool glDebugContext = false;
};

// ============================================================================
// 窗口事件回调
// ============================================================================

using WindowEventCallback = std::function<void(const InputEvent&)>;
using WindowResizeCallback = std::function<void(uint32_t width, uint32_t height)>;
using WindowCloseCallback = std::function<void()>;

// ============================================================================
// 窗口类
// ============================================================================

class Window {
public:
    virtual ~Window() = default;
    
    /**
     * @brief 创建窗口
     */
    virtual bool create(const WindowConfig& config) = 0;
    
    /**
     * @brief 销毁窗口
     */
    virtual void destroy() = 0;
    
    /**
     * @brief 检查窗口是否有效
     */
    [[nodiscard]] virtual bool isValid() const = 0;
    
    /**
     * @brief 获取原生窗口句柄
     */
    [[nodiscard]] virtual NativeWindowHandle getNativeHandle() const = 0;
    
    /**
     * @brief 获取原生表面句柄
     */
    [[nodiscard]] virtual NativeSurfaceHandle getSurfaceHandle() const = 0;
    
    /**
     * @brief 轮询窗口事件
     * @return 是否收到退出事件
     */
    virtual bool pollEvents() = 0;
    
    /**
     * @brief 等待事件
     * @param timeout 超时时间 (秒), 0 表示无限等待
     * @return 是否收到事件
     */
    virtual bool waitEvents(double timeout = 0.0) = 0;
    
    /**
     * @brief 交换缓冲区
     */
    virtual void swapBuffers() = 0;
    
    // ==================== 窗口属性 ====================
    
    [[nodiscard]] virtual std::string getTitle() const = 0;
    virtual void setTitle(const std::string& title) = 0;
    
    [[nodiscard]] virtual uint32_t getWidth() const = 0;
    [[nodiscard]] virtual uint32_t getHeight() const = 0;
    [[nodiscard]] virtual uint32_t getFramebufferWidth() const = 0;
    [[nodiscard]] virtual uint32_t getFramebufferHeight() const = 0;
    
    virtual void setSize(uint32_t width, uint32_t height) = 0;
    virtual void setPosition(int32_t x, int32_t y) = 0;
    
    [[nodiscard]] virtual int32_t getPositionX() const = 0;
    [[nodiscard]] virtual int32_t getPositionY() const = 0;
    
    [[nodiscard]] virtual bool isFocused() const = 0;
    [[nodiscard]] virtual bool isMinimized() const = 0;
    [[nodiscard]] virtual bool isMaximized() const = 0;
    [[nodiscard]] virtual bool isVisible() const = 0;
    [[nodiscard]] virtual bool isFullscreen() const = 0;
    
    virtual void setFullscreen(bool fullscreen, int32_t monitorIndex = 0) = 0;
    virtual void minimize() = 0;
    virtual void maximize() = 0;
    virtual void restore() = 0;
    virtual void show() = 0;
    virtual void hide() = 0;
    virtual void focus() = 0;
    virtual void requestAttention() = 0;
    
    // ==================== 光标 ====================
    
    enum class CursorShape {
        Arrow,
        IBeam,
        Crosshair,
        Hand,
        HResize,
        VResize,
        NotAllowed,
        Hidden
    };
    
    virtual void setCursor(CursorShape shape) = 0;
    virtual void setCursorPos(double x, double y) = 0;
    virtual void getCursorPos(double& x, double& y) const = 0;
    virtual void setCursorMode(int32_t mode) = 0;  // 0=Normal, 1=Hidden, 2=Disabled
    virtual int32_t getCursorMode() const = 0;
    
    // ==================== 事件回调 ====================
    
    virtual void setEventCallback(WindowEventCallback callback) = 0;
    virtual void setResizeCallback(WindowResizeCallback callback) = 0;
    virtual void setCloseCallback(WindowCloseCallback callback) = 0;
    
    // ==================== 监视器 ====================
    
    [[nodiscard]] static std::vector<MonitorInfo> getMonitors();
    [[nodiscard]] static MonitorInfo getPrimaryMonitor();
    [[nodiscard]] static std::vector<VideoMode> getVideoModes(int32_t monitorIndex = 0);
    
    // ==================== 工厂方法 ====================
    
    /**
     * @brief 创建平台特定的窗口实例
     */
    [[nodiscard]] static std::unique_ptr<Window> create();
};

} // namespace platform
} // namespace phoenix
