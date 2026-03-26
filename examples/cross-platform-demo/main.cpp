/**
 * @file main.cpp
 * @brief Phoenix Engine 跨平台演示程序
 * 
 * 本示例展示如何在所有支持的平台上运行 Phoenix Engine:
 * - Windows (DX12)
 * - Linux (Vulkan)
 * - macOS (Metal)
 * - iOS (Metal)
 * - Android (Vulkan/OpenGL ES)
 * - Web (WebGPU/WebGL)
 */

#include "phoenix/platform/application.hpp"
#include "phoenix/platform/window.hpp"
#include "phoenix/platform/input.hpp"
#include "phoenix/platform/timer.hpp"
#include <cstdio>
#include <cmath>

using namespace phoenix;
using namespace phoenix::platform;

// ============================================================================
// 演示应用类
// ============================================================================

class CrossPlatformDemo : public Application {
public:
    bool initialize(const ApplicationConfig& config) override {
        if (!Application::initialize(config)) {
            return false;
        }
        
        printf("[Demo] Phoenix Engine Cross-Platform Demo\n");
        printf("[Demo] Platform: %s\n", PHOENIX_PLATFORM_NAME);
        printf("[Demo] Render Backend: %s\n", renderBackendToString(getRenderBackend()));
        
        // 设置回调
        ApplicationCallbacks callbacks;
        callbacks.onInit = [this]() { onInit(); };
        callbacks.onShutdown = [this]() { onShutdown(); };
        callbacks.onUpdate = [this]() { onUpdate(); };
        callbacks.onRender = [this](float dt) { onRender(dt); };
        callbacks.onResize = [this](uint32_t w, uint32_t h) { onResize(w, h); };
        callbacks.onInput = [this](const InputEvent& e) { onInput(e); };
        setCallbacks(callbacks);
        
        return true;
    }
    
private:
    float rotation_ = 0.0f;
    float cubeVertices_[8 * 3] = {
        // 前
        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        // 后
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
    };
    
    uint32_t cubeIndices_[36] = {
        0, 1, 2, 0, 2, 3,    // 前
        1, 5, 6, 1, 6, 2,    // 右
        5, 4, 7, 5, 7, 6,    // 后
        4, 0, 3, 4, 3, 7,    // 左
        3, 2, 6, 3, 6, 7,    // 上
        4, 5, 1, 4, 1, 0,    // 下
    };
    
    void onInit() {
        printf("[Demo] Application initialized\n");
        printf("[Demo] Window size: %dx%d\n", getWindow()->getWidth(), getWindow()->getHeight());
        printf("[Demo] Target FPS: %d\n", getConfig().targetFPS);
        
        // 打印平台能力
        const auto& caps = getCapabilities();
        printf("[Demo] Platform Capabilities:\n");
        printf("  - Vulkan: %s\n", caps.supportsVulkan ? "Yes" : "No");
        printf("  - DX12: %s\n", caps.supportsDX12 ? "Yes" : "No");
        printf("  - Metal: %s\n", caps.supportsMetal ? "Yes" : "No");
        printf("  - WebGPU: %s\n", caps.supportsWebGPU ? "Yes" : "No");
        printf("  - Touch: %s\n", caps.supportsTouch ? "Yes" : "No");
        printf("  - Gamepad: %s\n", caps.supportsGamepad ? "Yes" : "No");
        printf("  - Max Memory: %d MB\n", caps.maxMemoryMB);
    }
    
    void onShutdown() {
        printf("[Demo] Application shutdown\n");
    }
    
    void onUpdate() {
        // 更新逻辑
        rotation_ += 0.5f * getTimeInfo().deltaTime;
        if (rotation_ > 360.0f) {
            rotation_ -= 360.0f;
        }
        
        // 检查输入
        if (Input::isKeyPressed(KeyCode::Escape)) {
            requestExit();
        }
        
        // 打印 FPS
        static double fpsTimer = 0.0;
        fpsTimer += getTimeInfo().deltaTime;
        if (fpsTimer >= 1.0) {
            printf("[Demo] FPS: %.1f\n", getTimeInfo().fps);
            fpsTimer = 0.0;
        }
    }
    
    void onRender(float deltaTime) {
        // 渲染逻辑
        // 在实际应用中，这里会调用渲染 API
        
        // 清除屏幕
        auto* window = getWindow();
        if (window) {
            // 这里会调用实际的渲染后端
        }
    }
    
    void onResize(uint32_t width, uint32_t height) {
        printf("[Demo] Window resized: %dx%d\n", width, height);
    }
    
    void onInput(const InputEvent& event) {
        switch (event.type) {
            case EventType::KeyDown:
                printf("[Demo] Key down: %d\n", static_cast<int>(event.keyCode));
                break;
            case EventType::KeyUp:
                printf("[Demo] Key up: %d\n", static_cast<int>(event.keyCode));
                break;
            case EventType::MouseButtonDown:
                printf("[Demo] Mouse button down: %d at (%.1f, %.1f)\n",
                       static_cast<int>(event.button), event.posX, event.posY);
                break;
            case EventType::TouchBegan:
                printf("[Demo] Touch began: id=%d at (%.1f, %.1f)\n",
                       event.touchId, event.posX, event.posY);
                break;
            default:
                break;
        }
    }
};

// ============================================================================
// 应用入口
// ============================================================================

#if defined(PHOENIX_PLATFORM_WEB)
// Web 平台使用 Emscripten 入口
#include <emscripten/emscripten.h>

static CrossPlatformDemo* g_app = nullptr;

static void mainLoop() {
    if (g_app && !g_app->shouldExit()) {
        g_app->run();
    } else {
        emscripten_cancel_main_loop();
    }
}

extern "C" int main() {
    g_app = new CrossPlatformDemo();
    
    ApplicationConfig config;
    config.name = "Phoenix Cross-Platform Demo";
    config.windowConfig.width = 800;
    config.windowConfig.height = 600;
    config.targetFPS = 60;
    
    if (!g_app->initialize(config)) {
        return 1;
    }
    
    emscripten_set_main_loop(mainLoop, 0, 1);
    
    delete g_app;
    return 0;
}

#elif defined(PHOENIX_PLATFORM_ANDROID)
// Android 平台使用 android_main 入口
#include "phoenix/platform/android/android_platform.hpp"

void android_main(struct android_app* state) {
    auto& platform = android::AndroidPlatform::getInstance();
    platform.initialize(state->activity);
    
    CrossPlatformDemo app;
    ApplicationConfig config;
    config.name = "Phoenix Cross-Platform Demo";
    config.targetFPS = 60;
    config.enablePowerSaving = true;
    
    if (app.initialize(config)) {
        app.run();
    }
    
    platform.shutdown();
}

#else
// 桌面平台标准入口
PHOENIX_APP_MAIN()
#endif
