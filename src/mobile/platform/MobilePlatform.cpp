#include "phoenix/mobile/platform/MobilePlatform.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>

namespace phoenix::mobile {

// Platform detection
#ifdef __APPLE__
    #include <TargetConditionals.h>
    #define PHOENIX_IOS TARGET_OS_IPHONE
    #define PHOENIX_ANDROID 0
#else
    #define PHOENIX_IOS 0
    #ifdef __ANDROID__
        #define PHOENIX_ANDROID 1
    #else
        #define PHOENIX_ANDROID 0
    #endif
#endif

MobilePlatform& MobilePlatform::getInstance() {
    static MobilePlatform instance;
    return instance;
}

bool MobilePlatform::initialize(const PlatformConfig& config) {
    if (isInitialized_) {
        return true;
    }

    config_ = config;
    
    // Detect device capabilities
    detectDeviceCapabilities();
    
    // Initialize safe area with defaults
    safeAreaEdges_ = SafeAreaEdges{0.02f, 0.02f, 0.0f, 0.0f};
    
    isInitialized_ = true;
    
    std::cout << "[MobilePlatform] Initialized on " << deviceInfo_.osName 
              << " " << deviceInfo_.osVersion << std::endl;
    std::cout << "[MobilePlatform] Screen: " << deviceInfo_.screenWidth 
              << "x" << deviceInfo_.screenHeight << " @ " << deviceInfo_.refreshRate 
              << "Hz, Density: " << deviceInfo_.screenDensity << std::endl;
    
    if (deviceInfo_.hasNotch) {
        std::cout << "[MobilePlatform] Device has notch" << std::endl;
    }
    
    return true;
}

void MobilePlatform::shutdown() {
    orientationCallbacks_.clear();
    safeAreaCallbacks_.clear();
    notificationCallbacks_.clear();
    activeNotifications_.clear();
    scheduledNotifications_.clear();
    
    isInitialized_ = false;
    
    std::cout << "[MobilePlatform] Shutdown complete" << std::endl;
}

void MobilePlatform::update(float deltaTime) {
    if (!isInitialized_) return;
    
    // Update status bar auto-hide
    if (config_.hideStatusBarOnInteraction && 
        statusBarVisibility_ == StatusBarVisibility::AutoHide) {
        statusBarAutoHideTimer_ -= deltaTime;
        if (statusBarAutoHideTimer_ <= 0) {
            setStatusBarVisibility(StatusBarVisibility::Hidden);
        }
    }
    
    // Check scheduled notifications
    auto now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    for (auto it = scheduledNotifications_.begin(); 
         it != scheduledNotifications_.end();) {
        if (it->scheduledTime > 0 && it->scheduledTime <= now) {
            showNotification(*it);
            it = scheduledNotifications_.erase(it);
        } else {
            ++it;
        }
    }
}

void MobilePlatform::platformInit(void* nativeWindow, void* nativeView) {
    // Platform-specific initialization would happen here
    // This is called from native iOS/Android code
    
#if PHOENIX_IOS
    // iOS-specific setup
    std::cout << "[MobilePlatform] iOS native initialization" << std::endl;
#elif PHOENIX_ANDROID
    // Android-specific setup
    std::cout << "[MobilePlatform] Android native initialization" << std::endl;
#endif
    
    (void)nativeWindow;
    (void)nativeView;
}

void MobilePlatform::updateSafeArea(float top, float bottom, float left, float right) {
    safeAreaEdges_.top = top;
    safeAreaEdges_.bottom = bottom;
    safeAreaEdges_.left = left;
    safeAreaEdges_.right = right;
    
    notifySafeAreaChanged();
    
    std::cout << "[MobilePlatform] Safe area updated: T=" << top 
              << " B=" << bottom << " L=" << left << " R=" << right << std::endl;
}

void MobilePlatform::updateOrientation(DeviceOrientation orientation) {
    if (isOrientationLocked_) {
        std::cout << "[MobilePlatform] Orientation change blocked (locked)" << std::endl;
        return;
    }
    
    currentOrientation_ = orientation;
    notifyOrientationChanged();
    
    const char* orientStr = "Unknown";
    switch (orientation) {
        case DeviceOrientation::Portrait: orientStr = "Portrait"; break;
        case DeviceOrientation::PortraitUpsideDown: orientStr = "Portrait Upside Down"; break;
        case DeviceOrientation::LandscapeLeft: orientStr = "Landscape Left"; break;
        case DeviceOrientation::LandscapeRight: orientStr = "Landscape Right"; break;
    }
    
    std::cout << "[MobilePlatform] Orientation changed to: " << orientStr << std::endl;
}

void MobilePlatform::updateNotchInfo(const NotchInfo& info) {
    notchInfo_ = info;
    std::cout << "[MobilePlatform] Notch info updated: " 
              << (info.hasNotch ? "Has notch" : "No notch") << std::endl;
}

bool MobilePlatform::isPointInSafeArea(float x, float y) const {
    return x >= safeAreaEdges_.left && x <= 1.0f - safeAreaEdges_.right &&
           y >= safeAreaEdges_.top && y <= 1.0f - safeAreaEdges_.bottom;
}

std::vector<float> MobilePlatform::getSafeContentRect() const {
    return {
        safeAreaEdges_.left,
        safeAreaEdges_.top,
        1.0f - safeAreaEdges_.left - safeAreaEdges_.right,
        1.0f - safeAreaEdges_.top - safeAreaEdges_.bottom
    };
}

void MobilePlatform::setStatusBarStyle(StatusBarStyle style) {
    currentStatusBarStyle_ = style;
    
#if PHOENIX_IOS
    // iOS: Would call into UIKit to set preferredStatusBarStyle
    std::cout << "[MobilePlatform] iOS status bar style: " 
              << (style == StatusBarStyle::Light ? "Light" : 
                  style == StatusBarStyle::Dark ? "Dark" : "Default") << std::endl;
#elif PHOENIX_ANDROID
    // Android: Would call into View system for status bar appearance
    std::cout << "[MobilePlatform] Android status bar style updated" << std::endl;
#endif
}

void MobilePlatform::setStatusBarVisibility(StatusBarVisibility visibility) {
    statusBarVisibility_ = visibility;
    
    if (visibility == StatusBarVisibility::AutoHide) {
        statusBarAutoHideTimer_ = config_.statusBarAutoHideTimeout;
    }
    
#if PHOENIX_IOS
    // iOS: Would call setNeedsStatusBarAppearanceUpdate
    std::cout << "[MobilePlatform] iOS status bar visibility: " 
              << (visibility == StatusBarVisibility::Visible ? "Visible" : "Hidden") << std::endl;
#elif PHOENIX_ANDROID
    // Android: Would use WindowManager for fullscreen
    std::cout << "[MobilePlatform] Android status bar visibility updated" << std::endl;
#endif
}

void MobilePlatform::showNotification(const Notification& notification) {
    // Check permissions
    if (!hasNotificationPermissions()) {
        std::cout << "[MobilePlatform] Notification blocked (no permissions)" << std::endl;
        return;
    }
    
    // Add to active notifications
    activeNotifications_.push_back(notification);
    
    // Limit active notifications
    if (activeNotifications_.size() > 5) {
        activeNotifications_.erase(activeNotifications_.begin());
    }
    
    notifyNotification(notification);
    
    std::cout << "[MobilePlatform] Notification shown: " << notification.title << std::endl;
}

void MobilePlatform::dismissNotification(const std::string& id) {
    activeNotifications_.erase(
        std::remove_if(activeNotifications_.begin(), activeNotifications_.end(),
            [&id](const Notification& n) { return n.id == id; }),
        activeNotifications_.end()
    );
    
    std::cout << "[MobilePlatform] Notification dismissed: " << id << std::endl;
}

void MobilePlatform::scheduleNotification(const Notification& notification) {
    scheduledNotifications_.push_back(notification);
    
    std::cout << "[MobilePlatform] Notification scheduled: " << notification.title
              << " at " << notification.scheduledTime << std::endl;
}

void MobilePlatform::cancelNotification(const std::string& id) {
    scheduledNotifications_.erase(
        std::remove_if(scheduledNotifications_.begin(), scheduledNotifications_.end(),
            [&id](const Notification& n) { return n.id == id; }),
        scheduledNotifications_.end()
    );
    
    std::cout << "[MobilePlatform] Notification cancelled: " << id << std::endl;
}

bool MobilePlatform::requestNotificationPermissions() {
#if PHOENIX_IOS
    // iOS: Would request UNUserNotificationCenter authorization
    std::cout << "[MobilePlatform] Requesting iOS notification permissions" << std::endl;
    return true;  // Simplified
#elif PHOENIX_ANDROID
    // Android: Would request POST_NOTIFICATIONS permission (API 33+)
    std::cout << "[MobilePlatform] Requesting Android notification permissions" << std::endl;
    return true;  // Simplified
#else
    return true;
#endif
}

bool MobilePlatform::hasNotificationPermissions() const {
    // In real implementation, would check actual permission status
    return true;
}

void MobilePlatform::setBadgeCount(int count) {
#if PHOENIX_IOS
    // iOS: Would set UIApplication.shared.applicationIconBadgeNumber
    std::cout << "[MobilePlatform] iOS badge count: " << count << std::endl;
#elif PHOENIX_ANDROID
    // Android: Would use ShortcutBadger or notification badge
    std::cout << "[MobilePlatform] Android badge count: " << count << std::endl;
#endif
}

void MobilePlatform::lockOrientation() {
    isOrientationLocked_ = true;
    
#if PHOENIX_IOS
    // iOS: Would set supportedOrientations
#elif PHOENIX_ANDROID
    // Android: Would call setRequestedOrientation
#endif
    
    std::cout << "[MobilePlatform] Orientation locked" << std::endl;
}

void MobilePlatform::unlockOrientation() {
    isOrientationLocked_ = false;
    
#if PHOENIX_IOS
    // iOS: Would clear orientation lock
#elif PHOENIX_ANDROID
    // Android: Would set SCREEN_ORIENTATION_UNSPECIFIED
#endif
    
    std::cout << "[MobilePlatform] Orientation unlocked" << std::endl;
}

void MobilePlatform::onOrientationChanged(OrientationCallback callback) {
    orientationCallbacks_.push_back(callback);
}

void MobilePlatform::onSafeAreaChanged(SafeAreaCallback callback) {
    safeAreaCallbacks_.push_back(callback);
}

void MobilePlatform::onNotification(NotificationCallback callback) {
    notificationCallbacks_.push_back(callback);
}

void MobilePlatform::detectDeviceCapabilities() {
#if PHOENIX_IOS
    deviceInfo_.osName = "iOS";
    deviceInfo_.manufacturer = "Apple";
    deviceInfo_.supportsMultitouch = true;
    deviceInfo_.maxTouchPoints = 10;  // iOS supports up to 11
    
    // Detect device model (simplified)
    // In real implementation, would use sysctlbyname("hw.machine")
    deviceInfo_.model = "iPhone";
    deviceInfo_.hasNotch = true;  // Most modern iPhones
    deviceInfo_.isTablet = false;
    
#elif PHOENIX_ANDROID
    deviceInfo_.osName = "Android";
    deviceInfo_.manufacturer = "Various";
    deviceInfo_.supportsMultitouch = true;
    deviceInfo_.maxTouchPoints = 10;
    
    // Would use Build.MODEL, Build.MANUFACTURER
    deviceInfo_.model = "Android Device";
    deviceInfo_.hasNotch = true;  // Many Android devices
    deviceInfo_.isTablet = false;
    
#else
    deviceInfo_.osName = "Unknown";
    deviceInfo_.model = "Unknown Device";
#endif
    
    // Set default screen info (would be updated from platform)
    deviceInfo_.screenWidth = 1170.0f;   // iPhone 13 Pro width
    deviceInfo_.screenHeight = 2532.0f;
    deviceInfo_.screenDensity = 3.0f;    // @3x
    deviceInfo_.refreshRate = 60.0f;
    deviceInfo_.totalMemory = 6LL * 1024 * 1024 * 1024;  // 6GB default
    
    // Detect if low-end device (simplified)
    deviceInfo_.isLowEndDevice = (deviceInfo_.totalMemory < 4LL * 1024 * 1024 * 1024);
}

void MobilePlatform::updateStatusBar(float deltaTime) {
    // Status bar update logic
}

void MobilePlatform::notifyOrientationChanged() {
    for (auto& callback : orientationCallbacks_) {
        try {
            callback(currentOrientation_);
        } catch (...) {
            std::cerr << "[MobilePlatform] Orientation callback exception" << std::endl;
        }
    }
}

void MobilePlatform::notifySafeAreaChanged() {
    for (auto& callback : safeAreaCallbacks_) {
        try {
            callback(safeAreaEdges_);
        } catch (...) {
            std::cerr << "[MobilePlatform] Safe area callback exception" << std::endl;
        }
    }
}

void MobilePlatform::notifyNotification(const Notification& notification) {
    for (auto& callback : notificationCallbacks_) {
        try {
            callback(notification);
        } catch (...) {
            std::cerr << "[MobilePlatform] Notification callback exception" << std::endl;
        }
    }
}

} // namespace phoenix::mobile
