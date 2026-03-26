#pragma once

#include "platform_types.hpp"
#include <array>
#include <string>
#include <vector>
#include <functional>

namespace phoenix {
namespace platform {

// ============================================================================
// 键盘状态
// ============================================================================

class Keyboard {
public:
    /**
     * @brief 检查按键是否按下
     */
    [[nodiscard]] static bool isKeyDown(KeyCode key);
    
    /**
     * @brief 检查按键是否刚按下
     */
    [[nodiscard]] static bool isKeyPressed(KeyCode key);
    
    /**
     * @brief 检查按键是否刚释放
     */
    [[nodiscard]] static bool isKeyUp(KeyCode key);
    
    /**
     * @brief 获取所有按下的键
     */
    [[nodiscard]] static std::vector<KeyCode> getPressedKeys();
    
    /**
     * @brief 设置按键回调
     */
    using KeyCallback = std::function<void(KeyCode key, int32_t scanCode, int32_t action, int32_t mods)>;
    static void setKeyCallback(KeyCallback callback);
    
    /**
     * @brief 设置文本输入回调
     */
    using CharCallback = std::function<void(uint32_t codepoint)>;
    static void setCharCallback(CharCallback callback);
    
    /**
     * @brief 获取修饰键状态
     */
    [[nodiscard]] static int32_t getModifiers();
    
    /**
     * @brief 检查修饰键
     */
    [[nodiscard]] static bool isShiftDown();
    [[nodiscard]] static bool isControlDown();
    [[nodiscard]] static bool isAltDown();
    [[nodiscard]] static bool isSuperDown();
    
    /**
     * @brief 更新键盘状态 (内部调用)
     */
    static void update();
    
    /**
     * @brief 重置状态
     */
    static void reset();
};

// ============================================================================
// 鼠标状态
// ============================================================================

class Mouse {
public:
    /**
     * @brief 检查鼠标按钮是否按下
     */
    [[nodiscard]] static bool isButtonDown(MouseButton button);
    
    /**
     * @brief 检查鼠标按钮是否刚按下
     */
    [[nodiscard]] static bool isButtonPressed(MouseButton button);
    
    /**
     * @brief 检查鼠标按钮是否刚释放
     */
    [[nodiscard]] static bool isButtonUp(MouseButton button);
    
    /**
     * @brief 获取鼠标位置
     */
    [[nodiscard]] static double getPositionX();
    [[nodiscard]] static double getPositionY();
    [[nodiscard]] static void getPosition(double& x, double& y);
    
    /**
     * @brief 获取鼠标 delta
     */
    [[nodiscard]] static double getDeltaX();
    [[nodiscard]] static double getDeltaY();
    
    /**
     * @brief 获取滚轮 delta
     */
    [[nodiscard]] static double getWheelDelta();
    
    /**
     * @brief 设置鼠标位置
     */
    static void setPosition(double x, double y);
    
    /**
     * @brief 设置鼠标按钮回调
     */
    using ButtonCallback = std::function<void(int32_t button, int32_t action, int32_t mods)>;
    static void setButtonCallback(ButtonCallback callback);
    
    /**
     * @brief 设置鼠标移动回调
     */
    using MoveCallback = std::function<void(double xpos, double ypos)>;
    static void setMoveCallback(MoveCallback callback);
    
    /**
     * @brief 设置滚轮回调
     */
    using ScrollCallback = std::function<void(double xoffset, double yoffset)>;
    static void setScrollCallback(ScrollCallback callback);
    
    /**
     * @brief 更新鼠标状态
     */
    static void update();
    
    /**
     * @brief 重置状态
     */
    static void reset();
};

// ============================================================================
// 触摸状态
// ============================================================================

struct TouchPoint {
    int32_t id = -1;
    float x = 0.0f;
    float y = 0.0f;
    float pressure = 1.0f;
    bool active = false;
};

class Touch {
public:
    /**
     * @brief 获取活动触摸点数量
     */
    [[nodiscard]] static int32_t getActiveCount();
    
    /**
     * @brief 获取所有触摸点
     */
    [[nodiscard]] static std::vector<TouchPoint> getActiveTouches();
    
    /**
     * @brief 检查触摸点是否存在
     */
    [[nodiscard]] static bool isTouchActive(int32_t id);
    
    /**
     * @brief 获取触摸点位置
     */
    [[nodiscard]] static bool getTouchPosition(int32_t id, float& x, float& y);
    
    /**
     * @brief 获取触摸点压力
     */
    [[nodiscard]] static float getTouchPressure(int32_t id);
    
    /**
     * @brief 设置触摸回调
     */
    using TouchCallback = std::function<void(int32_t id, float x, float y, float pressure, int32_t action)>;
    static void setTouchCallback(TouchCallback callback);
    
    /**
     * @brief 更新触摸状态
     */
    static void update();
    
    /**
     * @brief 重置状态
     */
    static void reset();
};

// ============================================================================
// 游戏手柄
// ============================================================================

struct GamepadState {
    std::array<bool, 15> buttons{};
    std::array<float, 6> axes{};  // LeftX, LeftY, RightX, RightY, LeftTrigger, RightTrigger
    std::string name;
    bool connected = false;
};

enum class GamepadButton : uint8_t {
    A = 0,
    B = 1,
    X = 2,
    Y = 3,
    LeftBumper = 4,
    RightBumper = 5,
    Back = 6,
    Start = 7,
    Guide = 8,
    LeftThumb = 9,
    RightThumb = 10,
    DPadUp = 11,
    DPadDown = 12,
    DPadLeft = 13,
    DPadRight = 14,
};

enum class GamepadAxis : uint8_t {
    LeftX = 0,
    LeftY = 1,
    RightX = 2,
    RightY = 3,
    LeftTrigger = 4,
    RightTrigger = 5,
};

class Gamepad {
public:
    /**
     * @brief 检查游戏手柄是否连接
     */
    [[nodiscard]] static bool isConnected(int32_t index = 0);
    
    /**
     * @brief 获取游戏手柄名称
     */
    [[nodiscard]] static std::string getName(int32_t index = 0);
    
    /**
     * @brief 检查按钮是否按下
     */
    [[nodiscard]] static bool isButtonDown(GamepadButton button, int32_t index = 0);
    
    /**
     * @brief 获取轴值 (-1.0 到 1.0)
     */
    [[nodiscard]] static float getAxis(GamepadAxis axis, int32_t index = 0);
    
    /**
     * @brief 获取振动支持
     */
    [[nodiscard]] static bool hasVibration(int32_t index = 0);
    
    /**
     * @brief 设置振动
     */
    static void setVibration(float leftMotor, float rightMotor, int32_t index = 0);
    
    /**
     * @brief 获取所有连接的游戏手柄
     */
    [[nodiscard]] static std::vector<int32_t> getConnectedGamepads();
    
    /**
     * @brief 设置连接回调
     */
    using ConnectionCallback = std::function<void(int32_t index, bool connected)>;
    static void setConnectionCallback(ConnectionCallback callback);
    
    /**
     * @brief 更新游戏手柄状态
     */
    static void update();
};

// ============================================================================
// 传感器
// ============================================================================

class Accelerometer {
public:
    [[nodiscard]] static bool isAvailable();
    [[nodiscard]] static float getX();
    [[nodiscard]] static float getY();
    [[nodiscard]] static float getZ();
    [[nodiscard]] static void getAcceleration(float& x, float& y, float& z);
    static void setUpdateRate(float hz);
};

class Gyroscope {
public:
    [[nodiscard]] static bool isAvailable();
    [[nodiscard]] static float getX();
    [[nodiscard]] static float getY();
    [[nodiscard]] static float getZ();
    [[nodiscard]] static void getRotation(float& x, float& y, float& z);
    static void setUpdateRate(float hz);
};

class Magnetometer {
public:
    [[nodiscard]] static bool isAvailable();
    [[nodiscard]] static float getX();
    [[nodiscard]] static float getY();
    [[nodiscard]] static float getZ();
    [[nodiscard]] static void getMagneticField(float& x, float& y, float& z);
};

// ============================================================================
// 输入管理器
// ============================================================================

class InputManager {
public:
    /**
     * @brief 初始化输入系统
     */
    static bool initialize();
    
    /**
     * @brief 关闭输入系统
     */
    static void shutdown();
    
    /**
     * @brief 更新所有输入状态
     */
    static void update();
    
    /**
     * @brief 获取平台能力
     */
    [[nodiscard]] static PlatformCapabilities getCapabilities();
    
    /**
     * @brief 处理原生事件 (内部调用)
     */
    static void processNativeEvent(const InputEvent& event);
};

} // namespace platform
} // namespace phoenix
