#include "phoenix/mobile/input/TouchInput.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace phoenix::mobile {

TouchInput& TouchInput::getInstance() {
    static TouchInput instance;
    return instance;
}

bool TouchInput::initialize(const TouchConfig& config) {
    if (isInitialized_) {
        return true;
    }

    config_ = config;
    
    // Initialize touch points
    for (auto& point : touchPoints_) {
        point.id = -1;
        point.state = TouchState::None;
    }
    
    lastUpdateTime_ = std::chrono::steady_clock::now();
    isInitialized_ = true;
    
    std::cout << "[TouchInput] Initialized with " << MAX_TOUCH_POINTS 
              << " touch points" << std::endl;
    
    return true;
}

void TouchInput::shutdown() {
    touchCallbacks_.clear();
    gestureCallbacks_.clear();
    virtualButtons_.clear();
    virtualSticks_.clear();
    
    for (auto& point : touchPoints_) {
        point.id = -1;
        point.state = TouchState::None;
    }
    
    isInitialized_ = false;
    
    std::cout << "[TouchInput] Shutdown complete" << std::endl;
}

void TouchInput::update() {
    if (!isInitialized_) return;
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - lastUpdateTime_).count();
    lastUpdateTime_ = now;
    
    // Update all touch points
    for (auto& point : touchPoints_) {
        if (point.state == TouchState::Began) {
            point.state = TouchState::Stationary;
        }
    }
    
    // Update gesture recognition
    if (config_.enableGestures) {
        updateGestures();
    }
    
    // Update virtual controller
    if (config_.enableVirtualController) {
        updateVirtualController();
    }
    
    // Update stats
    stats_.activeTouches = getActiveTouchCount();
}

void TouchInput::beginTouch(int id, float x, float y, float pressure, uint64_t timestamp) {
    if (!isInitialized_) return;
    
    // Find free touch point or update existing
    TouchPoint* point = nullptr;
    for (auto& tp : touchPoints_) {
        if (tp.id == id) {
            point = &tp;
            break;
        }
        if (tp.id == -1 && point == nullptr) {
            point = &tp;
        }
    }
    
    if (!point) return;
    
    point->id = id;
    point->state = TouchState::Began;
    point->x = std::clamp(x, 0.0f, 1.0f);
    point->y = std::clamp(y, 0.0f, 1.0f);
    point->previousX = point->x;
    point->previousY = point->y;
    point->pressure = std::clamp(pressure, 0.0f, 1.0f);
    point->timestamp = timestamp;
    
    notifyTouch(*point);
    detectTap(*point);
}

void TouchInput::moveTouch(int id, float x, float y, float pressure,
                           float radiusX, float radiusY, uint64_t timestamp) {
    if (!isInitialized_) return;
    
    for (auto& point : touchPoints_) {
        if (point.id == id) {
            point.previousX = point.x;
            point.previousY = point.y;
            point.x = std::clamp(x, 0.0f, 1.0f);
            point.y = std::clamp(y, 0.0f, 1.0f);
            point.pressure = std::clamp(pressure, 0.0f, 1.0f);
            point.radiusX = radiusX;
            point.radiusY = radiusY;
            point.state = TouchState::Moved;
            point.timestamp = timestamp;
            
            notifyTouch(*point);
            detectSwipe(*point);
            return;
        }
    }
}

void TouchInput::endTouch(int id, uint64_t timestamp) {
    if (!isInitialized_) return;
    
    for (auto& point : touchPoints_) {
        if (point.id == id) {
            point.state = TouchState::Ended;
            point.timestamp = timestamp;
            
            notifyTouch(*point);
            
            // Clear touch point
            point.id = -1;
            point.state = TouchState::None;
            return;
        }
    }
}

void TouchInput::cancelTouch(int id) {
    if (!isInitialized_) return;
    
    for (auto& point : touchPoints_) {
        if (point.id == id) {
            point.state = TouchState::Cancelled;
            notifyTouch(*point);
            
            point.id = -1;
            point.state = TouchState::None;
            return;
        }
    }
}

void TouchInput::setStylusData(int id, bool isStylus, bool isEraser,
                               float altitudeAngle, float azimuthAngle) {
    for (auto& point : touchPoints_) {
        if (point.id == id) {
            point.isStylus = isStylus;
            point.isEraser = isEraser;
            point.altitudeAngle = altitudeAngle;
            point.azimuthAngle = azimuthAngle;
            return;
        }
    }
}

const TouchPoint* TouchInput::getTouchPoint(int id) const {
    for (const auto& point : touchPoints_) {
        if (point.id == id) {
            return &point;
        }
    }
    return nullptr;
}

std::vector<TouchPoint> TouchInput::getActiveTouches() const {
    std::vector<TouchPoint> active;
    for (const auto& point : touchPoints_) {
        if (point.id != -1 && point.state != TouchState::None) {
            active.push_back(point);
        }
    }
    return active;
}

int TouchInput::getActiveTouchCount() const {
    int count = 0;
    for (const auto& point : touchPoints_) {
        if (point.id != -1 && point.state != TouchState::None) {
            count++;
        }
    }
    return count;
}

bool TouchInput::isTouching() const {
    return getActiveTouchCount() > 0;
}

bool TouchInput::addVirtualButton(const std::string& id, float x, float y,
                                  float width, float height,
                                  VirtualButtonCallback callback) {
    if (virtualButtons_.size() >= static_cast<size_t>(config_.maxVirtualButtons)) {
        std::cerr << "[TouchInput] Max virtual buttons reached" << std::endl;
        return false;
    }
    
    VirtualButton button;
    button.id = id;
    button.x = x;
    button.y = y;
    button.width = width;
    button.height = height;
    button.onPress = callback;
    
    virtualButtons_.push_back(button);
    std::cout << "[TouchInput] Added virtual button: " << id << std::endl;
    return true;
}

bool TouchInput::addVirtualStick(const std::string& id, float x, float y,
                                 float radius,
                                 VirtualStickCallback callback) {
    if (virtualSticks_.size() >= static_cast<size_t>(config_.maxVirtualSticks)) {
        std::cerr << "[TouchInput] Max virtual sticks reached" << std::endl;
        return false;
    }
    
    VirtualStick stick;
    stick.id = id;
    stick.baseX = x;
    stick.baseY = y;
    stick.baseRadius = radius;
    stick.onMove = callback;
    
    virtualSticks_.push_back(stick);
    std::cout << "[TouchInput] Added virtual stick: " << id << std::endl;
    return true;
}

void TouchInput::removeVirtualButton(const std::string& id) {
    virtualButtons_.erase(
        std::remove_if(virtualButtons_.begin(), virtualButtons_.end(),
            [&id](const VirtualButton& b) { return b.id == id; }),
        virtualButtons_.end()
    );
}

void TouchInput::removeVirtualStick(const std::string& id) {
    virtualSticks_.erase(
        std::remove_if(virtualSticks_.begin(), virtualSticks_.end(),
            [&id](const VirtualStick& s) { return s.id == id; }),
        virtualSticks_.end()
    );
}

bool TouchInput::getVirtualButtonState(const std::string& id) const {
    for (const auto& button : virtualButtons_) {
        if (button.id == id) {
            return button.isPressed;
        }
    }
    return false;
}

std::pair<float, float> TouchInput::getVirtualStickOutput(const std::string& id) const {
    for (const auto& stick : virtualSticks_) {
        if (stick.id == id) {
            return {stick.deltaX, stick.deltaY};
        }
    }
    return {0.0f, 0.0f};
}

void TouchInput::onTouch(TouchCallback callback) {
    touchCallbacks_.push_back(callback);
}

void TouchInput::onGesture(GestureCallback callback) {
    gestureCallbacks_.push_back(callback);
}

void TouchInput::resetStats() {
    stats_ = TouchStats();
}

std::pair<float, float> TouchInput::screenToNormalized(float screenX, float screenY,
                                                        float screenWidth, float screenHeight) {
    return {
        screenX / screenWidth,
        screenY / screenHeight
    };
}

std::pair<float, float> TouchInput::normalizedToScreen(float normX, float normY,
                                                        float screenWidth, float screenHeight) {
    return {
        normX * screenWidth,
        normY * screenHeight
    };
}

void TouchInput::updateGestures() {
    detectLongPress();
    detectPinch();
    detectRotate();
}

void TouchInput::updateVirtualController() {
    auto activeTouches = getActiveTouches();
    
    // Update buttons
    for (auto& button : virtualButtons_) {
        bool wasPressed = button.isPressed;
        button.isPressed = false;
        
        for (const auto& touch : activeTouches) {
            if (isPointInButton(touch.x, touch.y, button)) {
                button.isPressed = true;
                break;
            }
        }
        
        if (button.isPressed != wasPressed && button.onPress) {
            button.onPress(button.isPressed);
        }
    }
    
    // Update sticks
    for (auto& stick : virtualSticks_) {
        if (!stick.isActive) {
            // Look for new touch in stick area
            for (const auto& touch : activeTouches) {
                if (isPointInStick(touch.x, touch.y, stick) && stick.touchId == -1) {
                    stick.touchId = touch.id;
                    stick.isActive = true;
                    break;
                }
            }
        } else {
            // Find assigned touch
            const TouchPoint* assignedTouch = nullptr;
            for (const auto& touch : activeTouches) {
                if (touch.id == stick.touchId) {
                    assignedTouch = &touch;
                    break;
                }
            }
            
            if (assignedTouch) {
                // Calculate stick displacement
                float dx = assignedTouch->x - stick.baseX;
                float dy = assignedTouch->y - stick.baseY;
                float distance = std::sqrt(dx * dx + dy * dy);
                
                // Clamp to stick radius
                if (distance > stick.baseRadius) {
                    float scale = stick.baseRadius / distance;
                    dx *= scale;
                    dy *= scale;
                }
                
                // Update stick position
                stick.stickX = stick.baseX + dx;
                stick.stickY = stick.baseY + dy;
                
                // Calculate output (-1 to 1)
                stick.deltaX = dx / stick.baseRadius;
                stick.deltaY = dy / stick.baseRadius;
                stick.magnitude = std::min(1.0f, distance / stick.baseRadius);
                stick.angle = std::atan2(dy, dx);
                
                if (stick.onMove) {
                    stick.onMove(stick.deltaX, stick.deltaY);
                }
            } else {
                // Touch ended, reset stick
                stick.touchId = -1;
                stick.isActive = false;
                stick.stickX = stick.baseX;
                stick.stickY = stick.baseY;
                stick.deltaX = 0.0f;
                stick.deltaY = 0.0f;
                stick.magnitude = 0.0f;
            }
        }
    }
}

void TouchInput::detectTap(const TouchPoint& point) {
    if (point.state != TouchState::Began) return;
    
    auto now = std::chrono::steady_clock::now();
    auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    
    // Check for double tap
    if (lastTapTime_ > 0) {
        float timeDiff = static_cast<float>(nowMs - lastTapTime_);
        float distance = std::sqrt(
            std::pow(point.x - lastTapX_, 2) +
            std::pow(point.y - lastTapY_, 2)
        );
        
        if (timeDiff < config_.doubleTapTimeout && distance < 0.05f) {
            Gesture gesture;
            gesture.type = GestureType::DoubleTap;
            gesture.x = point.x;
            gesture.y = point.y;
            gesture.touchCount = 1;
            gesture.duration = timeDiff;
            
            notifyGesture(gesture);
            stats_.totalGestures++;
            lastTapTime_ = 0;
            return;
        }
    }
    
    // Store for potential double tap
    lastTapTime_ = nowMs;
    lastTapX_ = point.x;
    lastTapY_ = point.y;
}

void TouchInput::detectSwipe(const TouchPoint& point) {
    if (point.state != TouchState::Moved && point.state != TouchState::Ended) return;
    
    float dx = point.x - point.previousX;
    float dy = point.y - point.previousY;
    float distance = std::sqrt(dx * dx + dy * dy);
    
    if (distance < config_.swipeMinDistance) return;
    
    // Calculate velocity (simplified)
    float velocity = distance / 0.016f;  // Assume 60fps
    
    if (velocity < config_.swipeMinVelocity) return;
    
    Gesture gesture;
    gesture.type = GestureType::Swipe;
    gesture.x = point.x;
    gesture.y = point.y;
    gesture.deltaX = dx;
    gesture.deltaY = dy;
    gesture.velocity = velocity;
    gesture.touchCount = 1;
    
    // Determine direction
    if (std::abs(dx) > std::abs(dy)) {
        gesture.swipeDirection = (dx > 0) ? SwipeDirection::Right : SwipeDirection::Left;
    } else {
        gesture.swipeDirection = (dy > 0) ? SwipeDirection::Down : SwipeDirection::Up;
    }
    
    notifyGesture(gesture);
    stats_.totalGestures++;
}

void TouchInput::detectPinch() {
    auto activeTouches = getActiveTouches();
    if (activeTouches.size() < 2) return;
    
    // Use first two touches
    const auto& t1 = activeTouches[0];
    const auto& t2 = activeTouches[1];
    
    float currentDistance = std::sqrt(
        std::pow(t2.x - t1.x, 2) +
        std::pow(t2.y - t1.y, 2)
    );
    
    float previousDistance = std::sqrt(
        std::pow(t2.previousX - t1.previousX, 2) +
        std::pow(t2.previousY - t1.previousY, 2)
    );
    
    if (previousDistance < 0.001f) return;
    
    float scale = currentDistance / previousDistance;
    if (std::abs(scale - 1.0f) < config_.pinchMinScale) return;
    
    Gesture gesture;
    gesture.type = GestureType::Pinch;
    gesture.x = (t1.x + t2.x) / 2.0f;
    gesture.y = (t1.y + t2.y) / 2.0f;
    gesture.scale = scale;
    gesture.touchCount = 2;
    
    notifyGesture(gesture);
    stats_.totalGestures++;
}

void TouchInput::detectRotate() {
    auto activeTouches = getActiveTouches();
    if (activeTouches.size() < 2) return;
    
    const auto& t1 = activeTouches[0];
    const auto& t2 = activeTouches[1];
    
    float currentAngle = std::atan2(t2.y - t1.y, t2.x - t1.x);
    float previousAngle = std::atan2(t2.previousY - t1.previousY, t2.previousX - t1.previousX);
    
    float angleDiff = currentAngle - previousAngle;
    // Normalize to -PI to PI
    while (angleDiff > M_PI) angleDiff -= 2 * M_PI;
    while (angleDiff < -M_PI) angleDiff += 2 * M_PI;
    
    float angleDegrees = std::abs(angleDiff * 180.0f / M_PI);
    if (angleDegrees < config_.rotateMinAngle) return;
    
    Gesture gesture;
    gesture.type = GestureType::Rotate;
    gesture.x = (t1.x + t2.x) / 2.0f;
    gesture.y = (t1.y + t2.y) / 2.0f;
    gesture.rotation = angleDiff;
    gesture.touchCount = 2;
    
    notifyGesture(gesture);
    stats_.totalGestures++;
}

void TouchInput::detectLongPress() {
    auto now = std::chrono::steady_clock::now();
    
    for (const auto& point : touchPoints_) {
        if (point.state == TouchState::Stationary && point.id != -1) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - lastUpdateTime_).count();
            
            if (elapsed > config_.longPressTimeout) {
                Gesture gesture;
                gesture.type = GestureType::LongPress;
                gesture.x = point.x;
                gesture.y = point.y;
                gesture.touchCount = 1;
                gesture.duration = elapsed;
                
                notifyGesture(gesture);
                stats_.totalGestures++;
                
                // Reset to prevent repeated triggers
                lastUpdateTime_ = now;
            }
        }
    }
}

bool TouchInput::isPointInButton(float x, float y, const VirtualButton& button) const {
    float halfW = button.width / 2.0f;
    float halfH = button.height / 2.0f;
    
    return x >= button.x - halfW && x <= button.x + halfW &&
           y >= button.y - halfH && y <= button.y + halfH;
}

bool TouchInput::isPointInStick(float x, float y, const VirtualStick& stick) const {
    float dx = x - stick.baseX;
    float dy = y - stick.baseY;
    return (dx * dx + dy * dy) <= (stick.baseRadius * stick.baseRadius);
}

void TouchInput::notifyTouch(const TouchPoint& point) {
    for (auto& callback : touchCallbacks_) {
        try {
            callback(point);
        } catch (...) {
            std::cerr << "[TouchInput] Touch callback exception" << std::endl;
        }
    }
}

void TouchInput::notifyGesture(const Gesture& gesture) {
    lastGesture_ = gesture;
    stats_.lastGestureTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    for (auto& callback : gestureCallbacks_) {
        try {
            callback(gesture);
        } catch (...) {
            std::cerr << "[TouchInput] Gesture callback exception" << std::endl;
        }
    }
}

} // namespace phoenix::mobile
