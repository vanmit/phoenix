#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace phoenix::mobile {

/**
 * @brief Device orientation
 */
enum class DeviceOrientation {
    Unknown,
    Portrait,           // Home button at bottom
    PortraitUpsideDown, // Home button at top
    LandscapeLeft,      // Home button on right
    LandscapeRight      // Home button on left
};

/**
 * @brief Safe area edges
 */
struct SafeAreaEdges {
    float top = 0.0f;
    float bottom = 0.0f;
    float left = 0.0f;
    float right = 0.0f;
};

/**
 * @brief Notch/dynamic island information
 */
struct NotchInfo {
    bool hasNotch = false;
    bool hasDynamicIsland = false;
    float notchWidth = 0.0f;      // Normalized (0-1)
    float notchHeight = 0.0f;     // Normalized (0-1)
    float notchX = 0.0f;          // Normalized X position
    float notchY = 0.0f;          // Normalized Y position
    std::string notchType;        // "notch", "dynamic_island", "hole_punch", etc.
};

/**
 * @brief Status bar style
 */
enum class StatusBarStyle {
    Default,    // System default
    Light,      // Light content (for dark backgrounds)
    Dark        // Dark content (for light backgrounds)
};

/**
 * @brief Status bar visibility
 */
enum class StatusBarVisibility {
    Visible,
    Hidden,
    AutoHide    // Hide on interaction, show on timeout
};

/**
 * @brief Notification types
 */
enum class NotificationType {
    Local,          // Scheduled local notification
    Remote,         // Push notification
    InApp,          // In-app notification banner
    Critical        // Critical alert (iOS)
};

/**
 * @brief Notification data
 */
struct Notification {
    std::string id;
    std::string title;
    std::string body;
    std::string subtitle;
    NotificationType type = NotificationType::InApp;
    std::string imageUrl;
    std::string actionId;
    std::vector<std::string> actions;
    int badge = 0;
    bool sound = true;
    bool vibrate = true;
    int64_t scheduledTime = 0;  // 0 = immediate
    int64_t expireTime = 0;
};

/**
 * @brief Device information
 */
struct DeviceInfo {
    std::string model;
    std::string manufacturer;
    std::string osName;
    std::string osVersion;
    int apiLevel = 0;
    float screenWidth = 0.0f;
    float screenHeight = 0.0f;
    float screenDensity = 1.0f;     // DPI scale factor
    float refreshRate = 60.0f;      // Hz
    int64_t totalMemory = 0;        // Bytes
    int64_t availableMemory = 0;    // Bytes
    bool hasNotch = false;
    bool hasDynamicIsland = false;
    bool supportsStylus = false;
    bool supportsMultitouch = true;
    int maxTouchPoints = 10;
    bool isTablet = false;
    bool isFoldable = false;
    bool isLowEndDevice = false;
};

/**
 * @brief Platform configuration
 */
struct PlatformConfig {
    bool enableSafeAreaHandling = true;
    bool enableNotchAdaptation = true;
    bool enableOrientationHandling = true;
    bool enableStatusBarIntegration = true;
    bool enableNotificationHandling = true;
    bool hideStatusBarOnInteraction = true;
    float statusBarAutoHideTimeout = 3.0f;  // seconds
    bool allowPortrait = true;
    bool allowLandscape = true;
    bool allowUpsideDown = false;
    StatusBarStyle statusBarStyle = StatusBarStyle::Default;
};

/**
 * @brief Main mobile platform handler
 * 
 * Handles:
 * - iOS Notch/Dynamic Island adaptation
 * - Safe area handling
 * - Screen orientation changes
 * - Status bar integration
 * - Notification handling
 */
class MobilePlatform {
public:
    using OrientationCallback = std::function<void(DeviceOrientation)>;
    using SafeAreaCallback = std::function<void(const SafeAreaEdges&)>;
    using NotificationCallback = std::function<void(const Notification&)>;

    static MobilePlatform& getInstance();

    /**
     * @brief Initialize platform handler
     */
    bool initialize(const PlatformConfig& config = PlatformConfig());

    /**
     * @brief Shutdown platform handler
     */
    void shutdown();

    /**
     * @brief Update platform handler (call once per frame)
     */
    void update(float deltaTime);

    /**
     * @brief Get device information
     */
    DeviceInfo getDeviceInfo() const { return deviceInfo_; }

    /**
     * @brief Get current orientation
     */
    DeviceOrientation getOrientation() const { return currentOrientation_; }

    /**
     * @brief Get safe area edges (normalized 0-1)
     */
    SafeAreaEdges getSafeAreaEdges() const { return safeAreaEdges_; }

    /**
     * @brief Get notch information
     */
    NotchInfo getNotchInfo() const { return notchInfo_; }

    /**
     * @brief Check if point is in safe area
     * @param x Normalized X (0-1)
     * @param y Normalized Y (0-1)
     */
    bool isPointInSafeArea(float x, float y) const;

    /**
     * @brief Get safe content rectangle (normalized)
     * @return {x, y, width, height}
     */
    std::vector<float> getSafeContentRect() const;

    /**
     * @brief Set status bar style
     */
    void setStatusBarStyle(StatusBarStyle style);

    /**
     * @brief Set status bar visibility
     */
    void setStatusBarVisibility(StatusBarVisibility visibility);

    /**
     * @brief Get current status bar visibility
     */
    StatusBarVisibility getStatusBarVisibility() const { return statusBarVisibility_; }

    /**
     * @brief Show in-app notification
     */
    void showNotification(const Notification& notification);

    /**
     * @brief Dismiss notification
     */
    void dismissNotification(const std::string& id);

    /**
     * @brief Schedule local notification
     */
    void scheduleNotification(const Notification& notification);

    /**
     * @brief Cancel scheduled notification
     */
    void cancelNotification(const std::string& id);

    /**
     * @brief Request notification permissions
     * @return true if granted
     */
    bool requestNotificationPermissions();

    /**
     * @brief Check notification permissions
     */
    bool hasNotificationPermissions() const;

    /**
     * @brief Set badge count (iOS)
     */
    void setBadgeCount(int count);

    /**
     * @brief Lock orientation to current
     */
    void lockOrientation();

    /**
     * @brief Unlock orientation
     */
    void unlockOrientation();

    /**
     * @brief Register orientation change callback
     */
    void onOrientationChanged(OrientationCallback callback);

    /**
     * @brief Register safe area change callback
     */
    void onSafeAreaChanged(SafeAreaCallback callback);

    /**
     * @brief Register notification callback
     */
    void onNotification(NotificationCallback callback);

    /**
     * @brief Platform-specific initialization (called from native code)
     */
    void platformInit(void* nativeWindow, void* nativeView);

    /**
     * @brief Update safe area from platform (called from native code)
     */
    void updateSafeArea(float top, float bottom, float left, float right);

    /**
     * @brief Update orientation from platform (called from native code)
     */
    void updateOrientation(DeviceOrientation orientation);

    /**
     * @brief Update notch info from platform (called from native code)
     */
    void updateNotchInfo(const NotchInfo& info);

private:
    MobilePlatform() = default;
    ~MobilePlatform() = default;
    MobilePlatform(const MobilePlatform&) = delete;
    MobilePlatform& operator=(const MobilePlatform&) = delete;

    void detectDeviceCapabilities();
    void updateStatusBar(float deltaTime);
    void notifyOrientationChanged();
    void notifySafeAreaChanged();
    void notifyNotification(const Notification& notification);

    PlatformConfig config_;
    DeviceInfo deviceInfo_;
    DeviceOrientation currentOrientation_ = DeviceOrientation::Unknown;
    SafeAreaEdges safeAreaEdges_;
    NotchInfo notchInfo_;
    StatusBarVisibility statusBarVisibility_ = StatusBarVisibility::Visible;
    StatusBarStyle currentStatusBarStyle_ = StatusBarStyle::Default;
    
    std::vector<Notification> activeNotifications_;
    std::vector<Notification> scheduledNotifications_;
    
    std::vector<OrientationCallback> orientationCallbacks_;
    std::vector<SafeAreaCallback> safeAreaCallbacks_;
    std::vector<NotificationCallback> notificationCallbacks_;
    
    float statusBarAutoHideTimer_ = 0.0f;
    bool isOrientationLocked_ = false;
    bool isInitialized_ = false;
};

} // namespace phoenix::mobile
