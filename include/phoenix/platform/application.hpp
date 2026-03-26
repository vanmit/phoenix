#pragma once

#include "platform_types.hpp"
#include "window.hpp"
#include "input.hpp"
#include "timer.hpp"
#include <memory>
#include <string>
#include <functional>

namespace phoenix {
namespace platform {

// ============================================================================
// 应用配置
// ============================================================================

struct ApplicationConfig {
    std::string name = "Phoenix Application";
    std::string version = "1.0.0";
    std::string organization = "Phoenix Engine";
    
    WindowConfig windowConfig;
    
    RenderBackendType preferredBackend = RenderBackendType::Vulkan;
    bool enableValidation = false;
    bool enableDebugOverlay = false;
    
    uint32_t targetFPS = 60;
    bool limitFPS = true;
    bool runInBackground = false;
    
    // 移动端优化
    bool enablePowerSaving = false;
    bool enableLowMemoryMode = false;
    uint32_t maxMemoryMB = 256;  // 移动端默认 256MB
};

// ============================================================================
// 应用生命周期回调
// ============================================================================

struct ApplicationCallbacks {
    std::function<void()> onInit;
    std::function<void()> onShutdown;
    std::function<void()> onUpdate;
    std::function<void(float deltaTime)> onRender;
    std::function<void(uint32_t width, uint32_t height)> onResize;
    std::function<void(const InputEvent&)> onInput;
    std::function<void()> onSuspend;
    std::function<void()> onResume;
    std::function<void()> onMemoryWarning;
};

// ============================================================================
// 应用类
// ============================================================================

class Application {
public:
    /**
     * @brief 获取单例实例
     */
    [[nodiscard]] static Application& getInstance();
    
    /**
     * @brief 初始化应用
     */
    bool initialize(const ApplicationConfig& config);
    
    /**
     * @brief 运行应用主循环
     * @return 退出码
     */
    int run();
    
    /**
     * @brief 请求退出
     */
    void requestExit();
    
    /**
     * @brief 检查是否应该退出
     */
    [[nodiscard]] bool shouldExit() const { return shouldExit_; }
    
    /**
     * @brief 获取应用配置
     */
    [[nodiscard]] const ApplicationConfig& getConfig() const { return config_; }
    
    /**
     * @brief 获取窗口
     */
    [[nodiscard]] Window* getWindow() { return window_.get(); }
    [[nodiscard]] const Window* getWindow() const { return window_.get(); }
    
    /**
     * @brief 获取时间信息
     */
    [[nodiscard]] const TimeInfo& getTimeInfo() const { return timeInfo_; }
    
    /**
     * @brief 获取帧计时器
     */
    [[nodiscard]] FrameTimer& getFrameTimer() { return frameTimer_; }
    
    /**
     * @brief 获取系统信息
     */
    [[nodiscard]] const SystemInfo& getSystemInfo() const { return systemInfo_; }
    
    /**
     * @brief 获取平台能力
     */
    [[nodiscard]] const PlatformCapabilities& getCapabilities() const { return capabilities_; }
    
    /**
     * @brief 设置回调
     */
    void setCallbacks(const ApplicationCallbacks& callbacks) { callbacks_ = callbacks; }
    
    /**
     * @brief 获取渲染后端类型
     */
    [[nodiscard]] RenderBackendType getRenderBackend() const;
    
    /**
     * @brief 获取应用运行时间 (秒)
     */
    [[nodiscard]] double getRunTime() const;
    
    /**
     * @brief 获取应用是否在前台
     */
    [[nodiscard]] bool isForeground() const { return isForeground_; }
    
    /**
     * @brief 获取应用是否活动
     */
    [[nodiscard]] bool isActive() const { return isActive_; }

protected:
    Application() = default;
    virtual ~Application();
    
    // 禁止拷贝
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    
    /**
     * @brief 主循环迭代
     */
    void tick();
    
    /**
     * @brief 处理事件
     */
    void processEvents();
    
    /**
     * @brief 更新逻辑
     */
    void update();
    
    /**
     * @brief 渲染
     */
    void render();
    
    /**
     * @brief 限制帧率
     */
    void limitFrameRate();
    
    /**
     * @brief 检测平台能力
     */
    void detectCapabilities();
    
    /**
     * @brief 收集系统信息
     */
    void collectSystemInfo();

private:
    ApplicationConfig config_;
    ApplicationCallbacks callbacks_;
    
    std::unique_ptr<Window> window_;
    FrameTimer frameTimer_;
    TimeInfo timeInfo_;
    SystemInfo systemInfo_;
    PlatformCapabilities capabilities_;
    
    bool initialized_ = false;
    bool shouldExit_ = false;
    bool isForeground_ = true;
    bool isActive_ = true;
    
    double lastFrameTime_ = 0.0;
    double frameLimit_ = 0.0;
};

// ============================================================================
// 应用入口宏
// ============================================================================

#define PHOENIX_APP_MAIN() \
    int main(int argc, char* argv[]) { \
        auto& app = phoenix::platform::Application::getInstance(); \
        phoenix::platform::ApplicationConfig config; \
        config.windowConfig.width = 1920; \
        config.windowConfig.height = 1080; \
        app.initialize(config); \
        return app.run(); \
    }

#define PHOENIX_APP_MAIN_CONFIG(customConfig) \
    int main(int argc, char* argv[]) { \
        auto& app = phoenix::platform::Application::getInstance(); \
        app.initialize(customConfig); \
        return app.run(); \
    }

} // namespace platform
} // namespace phoenix
