#include "phoenix/mobile/power/PowerManager.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace phoenix::mobile {

PowerManager& PowerManager::getInstance() {
    static PowerManager instance;
    return instance;
}

bool PowerManager::initialize(const PowerConfig& config) {
    if (isInitialized_) {
        return true;
    }

    config_ = config;
    targetFrameTime_ = config.targetFrameTime;
    resolutionScale_.current = 1.0f;
    resolutionScale_.target = 1.0f;
    
    lastUpdate_ = std::chrono::steady_clock::now();
    isInitialized_ = true;
    
    std::cout << "[PowerManager] Initialized with target FPS: " 
              << (1000.0f / targetFrameTime_) << std::endl;
    
    return true;
}

void PowerManager::shutdown() {
    isInitialized_ = false;
    powerStateCallbacks_.clear();
    thermalCallbacks_.clear();
    batteryCallbacks_.clear();
    
    std::cout << "[PowerManager] Shutdown complete" << std::endl;
}

void PowerManager::update(float deltaTime) {
    if (!isInitialized_) return;
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<float>(now - lastUpdate_).count();
    lastUpdate_ = now;
    
    // Update power state based on conditions
    updatePowerState();
    
    // Update thermal throttling
    if (config_.enableThermalThrottling) {
        updateThermalThrottling();
    }
    
    // Smooth resolution scale transitions
    if (config_.enableDynamicResolution) {
        updateResolutionScale(elapsed);
    }
}

void PowerManager::setFrameRateMode(FrameRateMode mode) {
    frameRateMode_ = mode;
    
    switch (mode) {
        case FrameRateMode::FPS_30:
            targetFrameTime_ = 33.33f;
            break;
        case FrameRateMode::FPS_60:
            targetFrameTime_ = 16.67f;
            break;
        case FrameRateMode::FPS_90:
            targetFrameTime_ = 11.11f;
            break;
        case FrameRateMode::FPS_120:
            targetFrameTime_ = 8.33f;
            break;
        case FrameRateMode::Adaptive:
            targetFrameTime_ = config_.targetFrameTime;
            break;
    }
    
    std::cout << "[PowerManager] Frame rate mode set to " 
              << (1000.0f / targetFrameTime_) << " FPS" << std::endl;
}

void PowerManager::setResolutionScale(float scale) {
    resolutionScale_.target = std::clamp(scale, resolutionScale_.min, resolutionScale_.max);
}

void PowerManager::onEnterBackground() {
    if (!config_.enableBackgroundPause) return;
    
    isInBackground_ = true;
    backgroundEnterTime_ = std::chrono::steady_clock::now();
    
    // Store previous state
    auto previousState = currentState_;
    currentState_ = PowerState::Background;
    
    // Reduce to minimum settings
    setFrameRateMode(FrameRateMode::FPS_30);
    setResolutionScale(0.5f);
    
    std::cout << "[PowerManager] Entered background mode" << std::endl;
    
    notifyPowerStateChanged();
}

void PowerManager::onEnterForeground() {
    if (!isInBackground_) return;
    
    isInBackground_ = false;
    
    // Calculate how long we were in background
    auto backgroundDuration = std::chrono::steady_clock::now() - backgroundEnterTime_;
    auto seconds = std::chrono::duration<float>(backgroundDuration).count();
    
    // Restore normal operation
    setFrameRateMode(FrameRateMode::FPS_60);
    setResolutionScale(1.0f);
    currentState_ = PowerState::Normal;
    
    std::cout << "[PowerManager] Returned to foreground after " 
              << seconds << " seconds" << std::endl;
    
    notifyPowerStateChanged();
}

void PowerManager::updateThermalData(float cpuTemp, float gpuTemp) {
    thermalData_.cpuTemp = cpuTemp;
    thermalData_.gpuTemp = gpuTemp;
    
    calculateThermalState();
    notifyThermalStateChanged();
}

void PowerManager::updateBatteryData(float level, bool isCharging) {
    batteryStatus_.level = std::clamp(level, 0.0f, 1.0f);
    batteryStatus_.isCharging = isCharging;
    batteryStatus_.isFull = (level >= 0.99f);
    
    // Update power state based on battery
    if (!isCharging && batteryStatus_.level < config_.batteryCriticalThreshold) {
        currentState_ = PowerState::Critical;
    } else if (!isCharging && batteryStatus_.level < config_.batteryLowThreshold) {
        currentState_ = PowerState::PowerSave;
    }
    
    notifyBatteryStatusChanged();
}

void PowerManager::adjustResolutionForPerformance(float frameTime, float targetFrameTime) {
    if (!config_.enableDynamicResolution || isInBackground_) return;
    
    // If frame time exceeds target by more than 20%, reduce resolution
    if (frameTime > targetFrameTime * 1.2f) {
        float step = config_.resolutionScaleStep;
        resolutionScale_.target = std::max(
            resolutionScale_.min,
            resolutionScale_.target - step
        );
        std::cout << "[PowerManager] Reducing resolution scale to " 
                  << resolutionScale_.target << " (frame time: " << frameTime << "ms)" << std::endl;
    }
    // If frame time is well under target, increase resolution
    else if (frameTime < targetFrameTime * 0.8f) {
        float step = config_.resolutionScaleStep;
        resolutionScale_.target = std::min(
            resolutionScale_.max,
            resolutionScale_.target + step
        );
        std::cout << "[PowerManager] Increasing resolution scale to " 
                  << resolutionScale_.target << " (frame time: " << frameTime << "ms)" << std::endl;
    }
}

void PowerManager::updatePowerState() {
    // Don't override background state
    if (isInBackground_) {
        currentState_ = PowerState::Background;
        return;
    }
    
    // Check critical conditions
    if (thermalData_.state == ThermalState::Critical) {
        currentState_ = PowerState::Thermal;
        return;
    }
    
    if (!batteryStatus_.isCharging && 
        batteryStatus_.level < config_.batteryCriticalThreshold) {
        currentState_ = PowerState::Critical;
        return;
    }
    
    // Check power save conditions
    if (!batteryStatus_.isCharging && 
        batteryStatus_.level < config_.batteryLowThreshold) {
        currentState_ = PowerState::PowerSave;
        return;
    }
    
    // Normal operation
    currentState_ = PowerState::Normal;
}

void PowerManager::updateThermalThrottling() {
    calculateThermalState();
    
    // Apply throttling based on thermal state
    switch (thermalData_.state) {
        case ThermalState::Nominal:
            thermalData_.throttlingFactor = 1.0f;
            break;
        case ThermalState::Fair:
            thermalData_.throttlingFactor = 0.9f;
            break;
        case ThermalState::Serious:
            thermalData_.throttlingFactor = 0.7f;
            break;
        case ThermalState::Critical:
            thermalData_.throttlingFactor = 0.5f;
            break;
    }
    
    // Adjust target frame time based on throttling
    if (thermalData_.throttlingFactor < 1.0f) {
        float adjustedFrameTime = targetFrameTime_ / thermalData_.throttlingFactor;
        targetFrameTime_ = std::min(adjustedFrameTime, config_.maxFrameTime);
    }
}

void PowerManager::updateResolutionScale(float deltaTime) {
    // Smoothly interpolate current scale toward target
    float diff = resolutionScale_.target - resolutionScale_.current;
    if (std::abs(diff) > 0.001f) {
        resolutionScale_.current += diff * resolutionScale_.smoothFactor;
    } else {
        resolutionScale_.current = resolutionScale_.target;
    }
}

void PowerManager::calculateThermalState() {
    float maxTemp = std::max(thermalData_.cpuTemp, thermalData_.gpuTemp);
    
    if (maxTemp >= config_.thermalThreshold) {
        thermalData_.state = ThermalState::Critical;
    } else if (maxTemp >= config_.thermalThreshold - 5.0f) {
        thermalData_.state = ThermalState::Serious;
    } else if (maxTemp >= config_.thermalThreshold - 10.0f) {
        thermalData_.state = ThermalState::Fair;
    } else {
        thermalData_.state = ThermalState::Nominal;
    }
    
    thermalData_.throttlingFactor = 1.0f;
}

void PowerManager::onPowerStateChanged(PowerStateCallback callback) {
    powerStateCallbacks_.push_back(callback);
}

void PowerManager::onThermalStateChanged(ThermalCallback callback) {
    thermalCallbacks_.push_back(callback);
}

void PowerManager::onBatteryStatusChanged(BatteryCallback callback) {
    batteryCallbacks_.push_back(callback);
}

void PowerManager::notifyPowerStateChanged() {
    for (auto& callback : powerStateCallbacks_) {
        try {
            callback(currentState_);
        } catch (...) {
            std::cerr << "[PowerManager] Power state callback exception" << std::endl;
        }
    }
}

void PowerManager::notifyThermalStateChanged() {
    for (auto& callback : thermalCallbacks_) {
        try {
            callback(thermalData_);
        } catch (...) {
            std::cerr << "[PowerManager] Thermal callback exception" << std::endl;
        }
    }
}

void PowerManager::notifyBatteryStatusChanged() {
    for (auto& callback : batteryCallbacks_) {
        try {
            callback(batteryStatus_);
        } catch (...) {
            std::cerr << "[PowerManager] Battery callback exception" << std::endl;
        }
    }
}

} // namespace phoenix::mobile
