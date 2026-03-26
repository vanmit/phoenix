#pragma once

#include "../platform_types.hpp"
#include <android/native_window.h>
#include <android/input.h>
#include <android/sensor.h>
#include <android/looper.h>
#include <EGL/egl.h>
#include <GLES3/gl32.h>
#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace phoenix {
namespace platform {
namespace android {

// ============================================================================
// Android 应用状态
// ============================================================================

struct AndroidAppState {
    ANativeWindow* window = nullptr;
    AInputQueue* inputQueue = nullptr;
    ASensorEventQueue* sensorEventQueue = nullptr;
    ALooper* looper = nullptr;
    
    int32_t activityState = 0;  // APP_CMD_*
    bool hasFocus = false;
    bool isPaused = false;
    bool isDestroyed = false;
    
    int32_t screenWidth = 0;
    int32_t screenHeight = 0;
    int32_t framebufferWidth = 0;
    int32_t framebufferHeight = 0;
    float density = 1.0f;
    
    JavaVM* javaVM = nullptr;
    jobject activityObject = nullptr;
};

// ============================================================================
// Android 输入事件
// ============================================================================

struct AndroidInputEvent {
    int32_t type = 0;  // AINPUT_EVENT_TYPE_*
    int32_t source = 0;  // AINPUT_SOURCE_*
    int32_t action = 0;  // AMOTION_EVENT_ACTION_*
    int32_t flags = 0;
    int32_t metaState = 0;
    
    // 触摸/鼠标
    float x = 0.0f;
    float y = 0.0f;
    float pressure = 1.0f;
    float size = 1.0f;
    int32_t pointerId = 0;
    int32_t pointerCount = 1;
    
    // 键盘
    int32_t keyCode = 0;
    int32_t scanCode = 0;
    
    // 传感器
    float values[3] = {0.0f, 0.0f, 0.0f};
    int32_t sensorType = 0;
    
    int64_t eventTime = 0;
    int64_t downTime = 0;
};

// ============================================================================
// Android 传感器
// ============================================================================

enum class AndroidSensorType {
    Accelerometer = ASENSOR_TYPE_ACCELEROMETER,
    Gyroscope = ASENSOR_TYPE_GYROSCOPE,
    Magnetometer = ASENSOR_TYPE_MAGNETIC_FIELD,
    Light = ASENSOR_TYPE_LIGHT,
    Proximity = ASENSOR_TYPE_PROXIMITY,
    RotationVector = ASENSOR_TYPE_ROTATION_VECTOR,
    LinearAcceleration = ASENSOR_TYPE_LINEAR_ACCELERATION,
    Gravity = ASENSOR_TYPE_GRAVITY,
    AmbientTemperature = ASENSOR_TYPE_AMBIENT_TEMPERATURE,
    Pressure = ASENSOR_TYPE_PRESSURE,
    Humidity = ASENSOR_TYPE_RELATIVE_HUMIDITY,
    HeartRate = ASENSOR_TYPE_HEART_RATE,
};

struct AndroidSensor {
    ASensor* sensor = nullptr;
    AndroidSensorType type = AndroidSensorType::Accelerometer;
    std::string name;
    std::string vendor;
    int32_t handle = 0;
    int32_t typeCode = 0;
    
    float minDelay = 0.0f;      // 微秒
    float maxRange = 0.0f;
    float resolution = 0.0f;
    float power = 0.0f;         // mA
    float maxSampleRate = 0.0f; // Hz
    float fifoMaxEventCount = 0.0f;
    float fifoReservedEventCount = 0.0f;
    
    bool isWakeUp = false;
    bool isDynamic = false;
    bool isOnChangeMode = false;
};

// ============================================================================
// Android 平台类
// ============================================================================

class AndroidPlatform {
public:
    /**
     * @brief 获取单例实例
     */
    [[nodiscard]] static AndroidPlatform& getInstance();
    
    /**
     * @brief 初始化平台
     */
    bool initialize(ANativeActivity* activity);
    
    /**
     * @brief 关闭平台
     */
    void shutdown();
    
    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] bool isInitialized() const { return initialized_; }
    
    /**
     * @brief 获取应用状态
     */
    [[nodiscard]] const AndroidAppState& getAppState() const { return state_; }
    
    /**
     * @brief 获取原生窗口
     */
    [[nodiscard]] ANativeWindow* getNativeWindow() const { return state_.window; }
    
    /**
     * @brief 获取窗口尺寸
     */
    [[nodiscard]] int32_t getWindowWidth() const { return state_.screenWidth; }
    [[nodiscard]] int32_t getWindowHeight() const { return state_.screenHeight; }
    
    /**
     * @brief 获取帧缓冲尺寸
     */
    [[nodiscard]] int32_t getFramebufferWidth() const { return state_.framebufferWidth; }
    [[nodiscard]] int32_t getFramebufferHeight() const { return state_.framebufferHeight; }
    
    /**
     * @brief 获取屏幕密度
     */
    [[nodiscard]] float getDensity() const { return state_.density; }
    
    // ==================== 应用生命周期 ====================
    
    /**
     * @brief 处理应用命令
     */
    void handleAppCommand(int32_t cmd);
    
    /**
     * @brief 处理输入事件
     */
    int32_t handleInputEvent(AInputEvent* event);
    
    /**
     * @brief 处理传感器事件
     */
    void handleSensorEvent(ASensorEvent* event);
    
    /**
     * @brief 检查应用是否活动
     */
    [[nodiscard]] bool isAppActive() const;
    
    /**
     * @brief 检查应用有焦点
     */
    [[nodiscard]] bool hasFocus() const { return state_.hasFocus; }
    
    // ==================== 输入 ====================
    
    /**
     * @brief 轮询输入事件
     */
    bool pollInputEvents(std::vector<AndroidInputEvent>& events);
    
    /**
     * @brief 获取触摸状态
     */
    [[nodiscard]] int32_t getTouchCount() const;
    [[nodiscard]] bool getTouchPosition(int32_t pointerId, float& x, float& y) const;
    
    /**
     * @brief 获取按键状态
     */
    [[nodiscard]] bool isKeyDown(int32_t keyCode) const;
    
    // ==================== 传感器 ====================
    
    /**
     * @brief 获取可用传感器列表
     */
    [[nodiscard]] std::vector<AndroidSensor> getAvailableSensors();
    
    /**
     * @brief 获取特定类型的传感器
     */
    [[nodiscard]] AndroidSensor* getSensor(AndroidSensorType type);
    
    /**
     * @brief 启用传感器
     */
    bool enableSensor(AndroidSensorType type, int32_t samplingPeriodUs = 66666);  // 默认 60Hz
    
    /**
     * @brief 禁用传感器
     */
    void disableSensor(AndroidSensorType type);
    
    /**
     * @brief 获取加速度计数据
     */
    [[nodiscard]] bool getAccelerometer(float& x, float& y, float& z);
    
    /**
     * @brief 获取陀螺仪数据
     */
    [[nodiscard]] bool getGyroscope(float& x, float& y, float& z);
    
    /**
     * @brief 获取磁场数据
     */
    [[nodiscard]] bool getMagnetometer(float& x, float& y, float& z);
    
    /**
     * @brief 获取旋转矢量
     */
    [[nodiscard]] bool getRotationVector(float& x, float& y, float& z, float& w);
    
    // ==================== EGL 管理 ====================
    
    /**
     * @brief 初始化 EGL
     */
    bool initializeEGL();
    
    /**
     * @brief 创建 EGL 表面
     */
    bool createEGLSurface();
    
    /**
     * @brief 使 EGL 当前
     */
    bool makeEGLCurrent();
    
    /**
     * @brief 交换缓冲区
     */
    bool swapEGLBuffers();
    
    /**
     * @brief 关闭 EGL
     */
    void shutdownEGL();
    
    /**
     * @brief 获取 EGL 显示
     */
    [[nodiscard]] EGLDisplay getEGLDisplay() const { return eglDisplay_; }
    
    /**
     * @brief 获取 EGL 表面
     */
    [[nodiscard]] EGLSurface getEGLSurface() const { return eglSurface_; }
    
    /**
     * @brief 获取 EGL 上下文
     */
    [[nodiscard]] EGLContext getEGLContext() const { return eglContext_; }
    
    // ==================== JNI 交互 ====================
    
    /**
     * @brief 获取 JavaVM
     */
    [[nodiscard]] JavaVM* getJavaVM() const { return state_.javaVM; }
    
    /**
     * @brief 获取 Activity 对象
     */
    [[nodiscard]] jobject getActivityObject() const { return state_.activityObject; }
    
    /**
     * @brief 附加当前线程到 JVM
     */
    [[nodiscard]] JNIEnv* attachCurrentThread();
    
    /**
     * @brief 分离当前线程从 JVM
     */
    void detachCurrentThread();
    
    /**
     * @brief 调用 Java 方法
     */
    jobject callJavaMethod(const char* className, const char* methodName, const char* signature, ...);
    
    // ==================== 系统信息 ====================
    
    /**
     * @brief 获取 Android API 级别
     */
    [[nodiscard]] int32_t getApiLevel() const;
    
    /**
     * @brief 获取 Android 版本
     */
    [[nodiscard]] std::string getAndroidVersion() const;
    
    /**
     * @brief 获取设备型号
     */
    [[nodiscard]] std::string getDeviceModel() const;
    
    /**
     * @brief 获取设备制造商
     */
    [[nodiscard]] std::string getDeviceManufacturer() const;
    
    /**
     * @brief 获取可用内存
     */
    [[nodiscard]] int64_t getAvailableMemory() const;
    
    /**
     * @brief 获取总内存
     */
    [[nodiscard]] int64_t getTotalMemory() const;
    
    /**
     * @brief 检查功能支持
     */
    [[nodiscard]] bool hasSystemFeature(const char* feature) const;
    
    // ==================== 功耗管理 ====================
    
    /**
     * @brief 获取电池状态
     */
    struct BatteryInfo {
        int32_t level = 0;          // 0-100
        int32_t scale = 100;
        bool isCharging = false;
        bool isUSBConnected = false;
        bool isACConnected = false;
        bool isWirelessCharging = false;
    };
    
    [[nodiscard]] BatteryInfo getBatteryInfo() const;
    
    /**
     * @brief 设置低功耗模式
     */
    void setPowerSaveMode(bool enabled);
    
    /**
     * @brief 检查低功耗模式
     */
    [[nodiscard]] bool isPowerSaveMode() const;
    
    /**
     * @brief 保持屏幕常亮
     */
    void keepScreenOn(bool keepOn);
    
    /**
     * @brief 设置亮度
     */
    void setScreenBrightness(float brightness);  // 0.0 - 1.0
    
    // ==================== 回调 ====================
    
    using WindowResizeCallback = std::function<void(int32_t width, int32_t height)>;
    using FocusCallback = std::function<void(bool hasFocus)>;
    using PauseCallback = std::function<void()>;
    using ResumeCallback = std::function<void()>;
    using TerminateCallback = std::function<void()>;
    using LowMemoryCallback = std::function<void()>;
    
    void setWindowResizeCallback(WindowResizeCallback callback) { windowResizeCallback_ = callback; }
    void setFocusCallback(FocusCallback callback) { focusCallback_ = callback; }
    void setPauseCallback(PauseCallback callback) { pauseCallback_ = callback; }
    void setResumeCallback(ResumeCallback callback) { resumeCallback_ = callback; }
    void setTerminateCallback(TerminateCallback callback) { terminateCallback_ = callback; }
    void setLowMemoryCallback(LowMemoryCallback callback) { lowMemoryCallback_ = callback; }

private:
    AndroidPlatform() = default;
    ~AndroidPlatform();
    
    AndroidPlatform(const AndroidPlatform&) = delete;
    AndroidPlatform& operator=(const AndroidPlatform&) = delete;
    
    AndroidAppState state_;
    bool initialized_ = false;
    
    // EGL
    EGLDisplay eglDisplay_ = EGL_NO_DISPLAY;
    EGLSurface eglSurface_ = EGL_NO_SURFACE;
    EGLContext eglContext_ = EGL_NO_CONTEXT;
    
    // 传感器
    ASensorManager* sensorManager_ = nullptr;
    std::vector<AndroidSensor> sensors_;
    
    // 回调
    WindowResizeCallback windowResizeCallback_;
    FocusCallback focusCallback_;
    PauseCallback pauseCallback_;
    ResumeCallback resumeCallback_;
    TerminateCallback terminateCallback_;
    LowMemoryCallback lowMemoryCallback_;
    
    void initSensors();
    void cleanupSensors();
    void updateWindowState();
};

// ============================================================================
// Android 应用入口宏
// ============================================================================

#define PHOENIX_ANDROID_APP_MAIN() \
    void android_main(struct android_app* state) { \
        auto& platform = phoenix::platform::android::AndroidPlatform::getInstance(); \
        platform.initialize(state->activity); \
        /* 应用主循环 */ \
        platform.shutdown(); \
    }

} // namespace android
} // namespace platform
} // namespace phoenix
