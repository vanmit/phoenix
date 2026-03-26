#pragma once

#include "../platform_types.hpp"
#include "../window.hpp"
#include <windows.h>
#include <string>
#include <memory>

namespace phoenix {
namespace platform {
namespace windows {

// ============================================================================
// Win32 窗口配置
// ============================================================================

struct Win32WindowConfig {
    std::wstring title = L"Phoenix Engine";
    uint32_t width = 1920;
    uint32_t height = 1080;
    bool resizable = true;
    bool decorated = true;
    bool visible = true;
    bool fullscreen = false;
    bool highDPI = true;
    bool vsync = true;
    HINSTANCE hInstance = nullptr;
    int32_t showCommand = SW_SHOWDEFAULT;
};

// ============================================================================
// Win32 窗口类
// ============================================================================

class Win32Window : public Window {
public:
    Win32Window();
    virtual ~Win32Window();
    
    // 禁止拷贝
    Win32Window(const Win32Window&) = delete;
    Win32Window& operator=(const Win32Window&) = delete;
    
    /**
     * @brief 创建窗口
     */
    bool create(const WindowConfig& config) override;
    
    /**
     * @brief 销毁窗口
     */
    void destroy() override;
    
    /**
     * @brief 检查窗口是否有效
     */
    [[nodiscard]] bool isValid() const override { return hwnd_ != nullptr; }
    
    /**
     * @brief 获取原生窗口句柄
     */
    [[nodiscard]] NativeWindowHandle getNativeHandle() const override;
    
    /**
     * @brief 获取原生表面句柄
     */
    [[nodiscard]] NativeSurfaceHandle getSurfaceHandle() const override;
    
    /**
     * @brief 轮询窗口事件
     */
    [[nodiscard]] bool pollEvents() override;
    
    /**
     * @brief 等待事件
     */
    [[nodiscard]] bool waitEvents(double timeout = 0.0) override;
    
    /**
     * @brief 交换缓冲区
     */
    void swapBuffers() override;
    
    // ==================== 窗口属性 ====================
    
    [[nodiscard]] std::string getTitle() const override;
    void setTitle(const std::string& title) override;
    
    [[nodiscard]] uint32_t getWidth() const override;
    [[nodiscard]] uint32_t getHeight() const override;
    [[nodiscard]] uint32_t getFramebufferWidth() const override;
    [[nodiscard]] uint32_t getFramebufferHeight() const override;
    
    void setSize(uint32_t width, uint32_t height) override;
    void setPosition(int32_t x, int32_t y) override;
    
    [[nodiscard]] int32_t getPositionX() const override;
    [[nodiscard]] int32_t getPositionY() const override;
    
    [[nodiscard]] bool isFocused() const override;
    [[nodiscard]] bool isMinimized() const override;
    [[nodiscard]] bool isMaximized() const override;
    [[nodiscard]] bool isVisible() const override;
    [[nodiscard]] bool isFullscreen() const override;
    
    void setFullscreen(bool fullscreen, int32_t monitorIndex = 0) override;
    void minimize() override;
    void maximize() override;
    void restore() override;
    void show() override;
    void hide() override;
    void focus() override;
    void requestAttention() override;
    
    // ==================== 光标 ====================
    
    void setCursor(CursorShape shape) override;
    void setCursorPos(double x, double y) override;
    void getCursorPos(double& x, double& y) const override;
    void setCursorMode(int32_t mode) override;
    [[nodiscard]] int32_t getCursorMode() const override;
    
    // ==================== 事件回调 ====================
    
    void setEventCallback(WindowEventCallback callback) override { eventCallback_ = callback; }
    void setResizeCallback(WindowResizeCallback callback) override { resizeCallback_ = callback; }
    void setCloseCallback(WindowCloseCallback callback) override { closeCallback_ = callback; }
    
    // ==================== Win32 特定方法 ====================
    
    /**
     * @brief 获取 Win32 窗口句柄
     */
    [[nodiscard]] HWND getHWND() const { return hwnd_; }
    
    /**
     * @brief 获取 Win32 实例句柄
     */
    [[nodiscard]] HINSTANCE getHINSTANCE() const { return hinstance_; }
    
    /**
     * @brief 处理 Win32 消息
     */
    LRESULT handleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    
    /**
     * @brief 注册窗口类
     */
    static bool registerWindowClass(HINSTANCE hInstance);
    
    /**
     * @brief 注销窗口类
     */
    static void unregisterWindowClass(HINSTANCE hInstance);

private:
    HWND hwnd_ = nullptr;
    HINSTANCE hinstance_ = nullptr;
    
    WindowConfig config_;
    std::wstring title_;
    
    WindowEventCallback eventCallback_;
    WindowResizeCallback resizeCallback_;
    WindowCloseCallback closeCallback_;
    
    bool fullscreen_ = false;
    bool minimized_ = false;
    bool maximized_ = false;
    bool focused_ = false;
    
    // 全屏前的窗口状态
    WINDOWPLACEMENT windowedPlacement_ = {};
    MONITORINFOEXW monitorInfo_ = {};
    
    // 高 DPI 支持
    float dpiScaleX_ = 1.0f;
    float dpiScaleY_ = 1.0f;
    
    // 光标状态
    int32_t cursorMode_ = 0;
    CursorShape cursorShape_ = CursorShape::Arrow;
    
    void updateDPI();
    void updateWindowState();
    void enterFullscreen(int32_t monitorIndex);
    void exitFullscreen();
    void onResize(uint32_t width, uint32_t height);
    void onFocus(bool focused);
    
    static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

// ============================================================================
// DXGI 交换链管理
// ============================================================================

class DXGIManager {
public:
    /**
     * @brief 初始化 DXGI
     */
    static bool initialize();
    
    /**
     * @brief 关闭 DXGI
     */
    static void shutdown();
    
    /**
     * @brief 获取 DXGI 工厂
     */
    [[nodiscard]] static void* getFactory();  // IDXGIFactory*
    
    /**
     * @brief 枚举适配器
     */
    [[nodiscard]] static std::vector<std::string> enumerateAdapters();
    
    /**
     * @brief 获取主适配器
     */
    [[nodiscard]] static std::string getPrimaryAdapter();
    
    /**
     * @brief 检查 Tearing 支持
     */
    [[nodiscard]] static bool supportsTearing();

private:
    static void* factory_;  // IDXGIFactory*
    static bool initialized_;
};

} // namespace windows
} // namespace platform
} // namespace phoenix
