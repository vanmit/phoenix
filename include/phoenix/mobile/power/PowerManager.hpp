#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

namespace phoenix::mobile {

/**
 * @brief Power management states for mobile devices
 */
enum class PowerState {
    Normal,      // Full performance
    PowerSave,   // Reduced performance for battery life
    Thermal,     // Throttled due to heat
    Background,  // Paused/minimized
    Critical     // Critical battery/thermal state
};

/**
 * @brief Frame rate modes
 */
enum class FrameRateMode {
    FPS_30,
    FPS_60,
    FPS_90,
    FPS_120,
    Adaptive   // Auto-adjust based on load
};

/**
 * @brief Thermal state levels
 */
enum class ThermalState {
    Nominal,     // No throttling needed
    Fair,        // Slight throttling
    Serious,     // Moderate throttling
    Critical     // Severe throttling required
};

/**
 * @brief Power management configuration
 */
struct PowerConfig {
    float targetFrameTime = 16.67f;  // 60 FPS default
    float minFrameTime = 33.33f;     // 30 FPS minimum
    float maxFrameTime = 16.67f;     // Max allowed frame time
    float thermalThreshold = 45.0f;  // Celsius
    float batteryLowThreshold = 0.20f;  // 20%
    float batteryCriticalThreshold = 0.10f;  // 10%
    bool enableDynamicResolution = true;
    bool enableThermalThrottling = true;
    bool enableBackgroundPause = true;
    int maxResolutionScaleSteps = 5;
    float resolutionScaleStep = 0.1f;
};

/**
 * @brief Thermal monitoring data
 */
struct ThermalData {
    float cpuTemp = 0.0f;
    float gpuTemp = 0.0f;
    float batteryTemp = 0.0f;
    float skinTemp = 0.0f;
    ThermalState state = ThermalState::Nominal;
    float throttlingFactor = 1.0f;
};

/**
 * @brief Battery status information
 */
struct BatteryStatus {
    float level = 1.0f;           // 0.0 - 1.0
    bool isCharging = false;
    bool isFull = false;
    float voltage = 0.0f;
    float current = 0.0f;
    float power = 0.0f;           // Watts
    int estimatedMinutes = 0;
};

/**
 * @brief Dynamic resolution scaling data
 */
struct ResolutionScale {
    float current = 1.0f;
    float target = 1.0f;
    float min = 0.5f;
    float max = 1.0f;
    float smoothFactor = 0.1f;    // Lerp factor for smooth transitions
};

/**
 * @brief Main power management class for mobile devices
 * 
 * Handles:
 * - Dynamic resolution scaling
 * - Frame rate limiting (30/60/90/120 FPS)
 * - GPU frequency regulation
 * - Background pause mechanism
 * - Thermal throttling detection
 */
class PowerManager {
public:
    using PowerStateCallback = std::function<void(PowerState)>;
    using ThermalCallback = std::function<void(const ThermalData&)>;
    using BatteryCallback = std::function<void(const BatteryStatus&)>;

    static PowerManager& getInstance();

    /**
     * @brief Initialize power manager with configuration
     */
    bool initialize(const PowerConfig& config = PowerConfig());

    /**
     * @brief Shutdown power manager
     */
    void shutdown();

    /**
     * @brief Update power manager (call once per frame)
     */
    void update(float deltaTime);

    /**
     * @brief Get current power state
     */
    PowerState getCurrentState() const { return currentState_; }

    /**
     * @brief Get current frame rate mode
     */
    FrameRateMode getFrameRateMode() const { return frameRateMode_; }

    /**
     * @brief Set target frame rate mode
     */
    void setFrameRateMode(FrameRateMode mode);

    /**
     * @brief Get target frame time in milliseconds
     */
    float getTargetFrameTime() const { return targetFrameTime_; }

    /**
     * @brief Get current resolution scale (0.0 - 1.0)
     */
    float getResolutionScale() const { return resolutionScale_.current; }

    /**
     * @brief Set resolution scale target
     */
    void setResolutionScale(float scale);

    /**
     * @brief Get thermal data
     */
    ThermalData getThermalData() const { return thermalData_; }

    /**
     * @brief Get battery status
     */
    BatteryStatus getBatteryStatus() const { return batteryStatus_; }

    /**
     * @brief Check if app is in background
     */
    bool isInBackground() const { return isInBackground_; }

    /**
     * @brief Register power state change callback
     */
    void onPowerStateChanged(PowerStateCallback callback);

    /**
     * @brief Register thermal state callback
     */
    void onThermalStateChanged(ThermalCallback callback);

    /**
     * @brief Register battery status callback
     */
    void onBatteryStatusChanged(BatteryCallback callback);

    /**
     * @brief Called when app enters background
     */
    void onEnterBackground();

    /**
     * @brief Called when app returns to foreground
     */
    void onEnterForeground();

    /**
     * @brief Update platform-specific thermal data
     * @param cpuTemp CPU temperature in Celsius
     * @param gpuTemp GPU temperature in Celsius
     */
    void updateThermalData(float cpuTemp, float gpuTemp);

    /**
     * @brief Update platform-specific battery data
     */
    void updateBatteryData(float level, bool isCharging);

    /**
     * @brief Adjust resolution based on performance
     * @param frameTime Actual frame time in ms
     * @param targetFrameTime Target frame time in ms
     */
    void adjustResolutionForPerformance(float frameTime, float targetFrameTime);

private:
    PowerManager() = default;
    ~PowerManager() = default;
    PowerManager(const PowerManager&) = delete;
    PowerManager& operator=(const PowerManager&) = delete;

    void updatePowerState();
    void updateThermalThrottling();
    void updateResolutionScale(float deltaTime);
    void calculateThermalState();
    void notifyPowerStateChanged();
    void notifyThermalStateChanged();
    void notifyBatteryStatusChanged();

    PowerConfig config_;
    PowerState currentState_ = PowerState::Normal;
    FrameRateMode frameRateMode_ = FrameRateMode::FPS_60;
    float targetFrameTime_ = 16.67f;
    
    ThermalData thermalData_;
    BatteryStatus batteryStatus_;
    ResolutionScale resolutionScale_;
    
    bool isInBackground_ = false;
    bool isInitialized_ = false;
    
    std::chrono::steady_clock::time_point lastUpdate_;
    std::chrono::steady_clock::time_point backgroundEnterTime_;
    
    std::vector<PowerStateCallback> powerStateCallbacks_;
    std::vector<ThermalCallback> thermalCallbacks_;
    std::vector<BatteryCallback> batteryCallbacks_;
};

} // namespace phoenix::mobile
