#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace phoenix::mobile {

/**
 * @brief Maximum number of simultaneous touch points
 */
constexpr int MAX_TOUCH_POINTS = 10;

/**
 * @brief Touch point state
 */
enum class TouchState {
    None,       // No touch
    Began,      // Touch started
    Moved,      // Touch moved
    Stationary, // Touch unchanged
    Ended,      // Touch ended
    Cancelled   // Touch cancelled
};

/**
 * @brief Individual touch point data
 */
struct TouchPoint {
    int id = -1;                    // Unique touch identifier
    TouchState state = TouchState::None;
    float x = 0.0f;                 // X position (0-1 normalized)
    float y = 0.0f;                 // Y position (0-1 normalized)
    float previousX = 0.0f;         // Previous X position
    float previousY = 0.0f;         // Previous Y position
    float pressure = 1.0f;          // Pressure (0-1, 1 for finger)
    float radiusX = 1.0f;           // Contact radius X
    float radiusY = 1.0f;           // Contact radius Y
    float altitudeAngle = 0.0f;     // Stylus altitude (radians)
    float azimuthAngle = 0.0f;      // Stylus azimuth (radians)
    bool isStylus = false;          // True if stylus input
    bool isEraser = false;          // True if stylus eraser
    uint64_t timestamp = 0;         // Touch timestamp
};

/**
 * @brief Gesture types
 */
enum class GestureType {
    None,
    Tap,
    DoubleTap,
    LongPress,
    Swipe,
    Pinch,
    Rotate,
    Pan
};

/**
 * @brief Swipe direction
 */
enum class SwipeDirection {
    None,
    Up,
    Down,
    Left,
    Right
};

/**
 * @brief Recognized gesture data
 */
struct Gesture {
    GestureType type = GestureType::None;
    float x = 0.0f;                 // Gesture center X
    float y = 0.0f;                 // Gesture center Y
    float deltaX = 0.0f;            // Delta X (for pan/swipe)
    float deltaY = 0.0f;            // Delta Y (for pan/swipe)
    float scale = 1.0f;             // Pinch scale factor
    float rotation = 0.0f;          // Rotation angle (radians)
    float velocity = 0.0f;          // Gesture velocity
    SwipeDirection swipeDirection = SwipeDirection::None;
    int touchCount = 0;             // Number of touches involved
    uint64_t duration = 0;          // Gesture duration in ms
};

/**
 * @brief Virtual controller button
 */
struct VirtualButton {
    std::string id;
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.1f;
    float height = 0.1f;
    bool isPressed = false;
    bool wasPressed = false;
    std::function<void(bool)> onPress;
};

/**
 * @brief Virtual analog stick
 */
struct VirtualStick {
    std::string id;
    float baseX = 0.0f;
    float baseY = 0.0f;
    float baseRadius = 0.15f;
    float stickX = 0.0f;
    float stickY = 0.0f;
    float deltaX = 0.0f;            // Output X (-1 to 1)
    float deltaY = 0.0f;            // Output Y (-1 to 1)
    float magnitude = 0.0f;         // Stick magnitude (0-1)
    float angle = 0.0f;             // Stick angle (radians)
    int touchId = -1;               // Assigned touch ID
    bool isActive = false;
    std::function<void(float, float)> onMove;
};

/**
 * @brief Touch input configuration
 */
struct TouchConfig {
    float tapTimeout = 300;             // ms for tap detection
    float doubleTapTimeout = 300;       // ms between taps
    float longPressTimeout = 500;       // ms for long press
    float swipeMinDistance = 0.1f;      // Min distance for swipe (normalized)
    float swipeMinVelocity = 0.5f;      // Min velocity for swipe
    float pinchMinScale = 0.1f;         // Min pinch scale
    float rotateMinAngle = 5.0f;        // Min rotation angle (degrees)
    bool enableGestures = true;
    bool enableVirtualController = true;
    bool enableStylusPressure = true;
    int maxVirtualButtons = 8;
    int maxVirtualSticks = 2;
};

/**
 * @brief Touch input statistics
 */
struct TouchStats {
    int activeTouches = 0;
    int totalTaps = 0;
    int totalGestures = 0;
    float avgPressure = 0.0f;
    uint64_t lastGestureTime = 0;
};

/**
 * @brief Main touch input handler for mobile devices
 * 
 * Handles:
 * - Multi-touch support (up to 10 points)
 * - Gesture recognition (pinch, swipe, tap, etc.)
 * - Stylus pressure sensitivity
 * - Virtual controller (buttons and sticks)
 */
class TouchInput {
public:
    using TouchCallback = std::function<void(const TouchPoint&)>;
    using GestureCallback = std::function<void(const Gesture&)>;
    using VirtualButtonCallback = std::function<void(const std::string&, bool)>;
    using VirtualStickCallback = std::function<void(const std::string&, float, float)>;

    static TouchInput& getInstance();

    /**
     * @brief Initialize touch input system
     */
    bool initialize(const TouchConfig& config = TouchConfig());

    /**
     * @brief Shutdown touch input system
     */
    void shutdown();

    /**
     * @brief Update touch input (call once per frame)
     */
    void update();

    /**
     * @brief Begin touch event
     * @param id Touch point ID
     * @param x X position (0-1)
     * @param y Y position (0-1)
     * @param pressure Pressure value (0-1)
     * @param timestamp Event timestamp
     */
    void beginTouch(int id, float x, float y, float pressure = 1.0f, 
                   uint64_t timestamp = 0);

    /**
     * @brief Move touch event
     */
    void moveTouch(int id, float x, float y, float pressure = 1.0f,
                  float radiusX = 1.0f, float radiusY = 1.0f,
                  uint64_t timestamp = 0);

    /**
     * @brief End touch event
     */
    void endTouch(int id, uint64_t timestamp = 0);

    /**
     * @brief Cancel touch event
     */
    void cancelTouch(int id);

    /**
     * @brief Set stylus-specific data
     */
    void setStylusData(int id, bool isStylus, bool isEraser,
                      float altitudeAngle, float azimuthAngle);

    /**
     * @brief Get touch point by ID
     */
    const TouchPoint* getTouchPoint(int id) const;

    /**
     * @brief Get all active touch points
     */
    std::vector<TouchPoint> getActiveTouches() const;

    /**
     * @brief Get number of active touches
     */
    int getActiveTouchCount() const;

    /**
     * @brief Check if screen is being touched
     */
    bool isTouching() const;

    /**
     * @brief Get last recognized gesture
     */
    Gesture getLastGesture() const { return lastGesture_; }

    /**
     * @brief Add virtual button
     */
    bool addVirtualButton(const std::string& id, float x, float y,
                         float width, float height,
                         VirtualButtonCallback callback = nullptr);

    /**
     * @brief Add virtual analog stick
     */
    bool addVirtualStick(const std::string& id, float x, float y,
                        float radius,
                        VirtualStickCallback callback = nullptr);

    /**
     * @brief Remove virtual button
     */
    void removeVirtualButton(const std::string& id);

    /**
     * @brief Remove virtual stick
     */
    void removeVirtualStick(const std::string& id);

    /**
     * @brief Get virtual button state
     */
    bool getVirtualButtonState(const std::string& id) const;

    /**
     * @brief Get virtual stick output
     */
    std::pair<float, float> getVirtualStickOutput(const std::string& id) const;

    /**
     * @brief Register touch callback
     */
    void onTouch(TouchCallback callback);

    /**
     * @brief Register gesture callback
     */
    void onGesture(GestureCallback callback);

    /**
     * @brief Get touch statistics
     */
    TouchStats getStats() const { return stats_; }

    /**
     * @brief Reset touch statistics
     */
    void resetStats();

    /**
     * @brief Convert screen coordinates to normalized (0-1)
     */
    static std::pair<float, float> screenToNormalized(float screenX, float screenY,
                                                      float screenWidth, float screenHeight);

    /**
     * @brief Convert normalized to screen coordinates
     */
    static std::pair<float, float> normalizedToScreen(float normX, float normY,
                                                      float screenWidth, float screenHeight);

private:
    TouchInput() = default;
    ~TouchInput() = default;
    TouchInput(const TouchInput&) = delete;
    TouchInput& operator=(const TouchInput&) = delete;

    void updateGestures();
    void updateVirtualController();
    void detectTap(const TouchPoint& point);
    void detectSwipe(const TouchPoint& point);
    void detectPinch();
    void detectRotate();
    void detectLongPress();
    bool isPointInButton(float x, float y, const VirtualButton& button) const;
    bool isPointInStick(float x, float y, const VirtualStick& stick) const;
    void notifyTouch(const TouchPoint& point);
    void notifyGesture(const Gesture& gesture);

    TouchConfig config_;
    TouchStats stats_;
    
    std::array<TouchPoint, MAX_TOUCH_POINTS> touchPoints_;
    std::vector<Gesture> recentGestures_;
    Gesture lastGesture_;
    
    std::vector<VirtualButton> virtualButtons_;
    std::vector<VirtualStick> virtualSticks_;
    
    std::vector<TouchCallback> touchCallbacks_;
    std::vector<GestureCallback> gestureCallbacks_;
    
    std::chrono::steady_clock::time_point lastUpdateTime_;
    uint64_t lastTapTime_ = 0;
    float lastTapX_ = 0.0f;
    float lastTapY_ = 0.0f;
    
    bool isInitialized_ = false;
};

} // namespace phoenix::mobile
