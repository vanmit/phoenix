#include "phoenix/mobile/profiler/MobileProfiler.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

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

MobileProfiler& MobileProfiler::getInstance() {
    static MobileProfiler instance;
    return instance;
}

bool MobileProfiler::initialize(const ProfilerConfig& config) {
    if (isInitialized_) {
        return true;
    }

    config_ = config;
    
    // Initialize sample history
    sampleHistory_.reserve(config.sampleHistorySize);
    fpsHistory_.reserve(config.sampleHistorySize);
    
    // Initialize session start time
    sessionStartTime_ = std::chrono::steady_clock::now();
    lastUpdateTime_ = sessionStartTime_;
    
    // Initialize thermal zones
#if PHOENIX_IOS
    thermalZones_ = {
        {"CPU", 0.0f, 90.0f, 85.0f, false},
        {"GPU", 0.0f, 90.0f, 85.0f, false},
        {"Battery", 0.0f, 60.0f, 50.0f, false},
        {"Skin", 0.0f, 45.0f, 40.0f, false}
    };
#elif PHOENIX_ANDROID
    thermalZones_ = {
        {"cpu", 0.0f, 90.0f, 85.0f, false},
        {"gpu", 0.0f, 90.0f, 85.0f, false},
        {"battery", 0.0f, 60.0f, 50.0f, false},
        {"skin", 0.0f, 45.0f, 40.0f, false}
    };
#else
    thermalZones_ = {
        {"CPU", 0.0f, 90.0f, 85.0f, false},
        {"GPU", 0.0f, 90.0f, 85.0f, false}
    };
#endif
    
    isInitialized_ = true;
    
    std::cout << "[MobileProfiler] Initialized" << std::endl;
    std::cout << "[MobileProfiler] FPS Counter: " << (config.enableFPSCounter ? "ON" : "OFF") << std::endl;
    std::cout << "[MobileProfiler] Memory Monitor: " << (config.enableMemoryMonitor ? "ON" : "OFF") << std::endl;
    std::cout << "[MobileProfiler] Battery Monitor: " << (config.enableBatteryMonitor ? "ON" : "OFF") << std::endl;
    std::cout << "[MobileProfiler] Thermal Monitor: " << (config.enableThermalMonitor ? "ON" : "OFF") << std::endl;
    
    return true;
}

void MobileProfiler::shutdown() {
    alertCallbacks_.clear();
    sampleCallbacks_.clear();
    sampleHistory_.clear();
    fpsHistory_.clear();
    recentAlerts_.clear();
    
    isInitialized_ = false;
    
    std::cout << "[MobileProfiler] Shutdown complete" << std::endl;
}

void MobileProfiler::beginFrame() {
    if (!isInitialized_) return;
    
    frameStartTime_ = std::chrono::steady_clock::now();
    sectionIndex_ = 0;
}

void MobileProfiler::endFrame() {
    if (!isInitialized_) return;
    
    auto frameEndTime = std::chrono::steady_clock::now();
    auto frameDuration = std::chrono::duration<float, std::milli>(frameEndTime - frameStartTime_).count();
    
    currentFrameTiming_.total = frameDuration;
    
    // Calculate "other" time
    float accountedTime = currentFrameTiming_.cpu + currentFrameTiming_.gpu +
                         currentFrameTiming_.render + currentFrameTiming_.physics +
                         currentFrameTiming_.animation + currentFrameTiming_.ui;
    currentFrameTiming_.other = std::max(0.0f, frameDuration - accountedTime);
    
    fpsFrameCount_++;
    fpsAccumulator_ += frameDuration;
    frameCount_++;
    
    currentSample_.frameTime = frameDuration;
}

void MobileProfiler::update() {
    if (!isInitialized_) return;
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<float>(now - lastUpdateTime_).count();
    
    // Update at configured interval
    if (elapsed < config_.updateInterval) return;
    
    lastUpdateTime_ = now;
    
    // Update FPS
    if (config_.enableFPSCounter) {
        updateFPS();
    }
    
    // Update memory
    if (config_.enableMemoryMonitor) {
        updateMemory();
    }
    
    // Update battery
    if (config_.enableBatteryMonitor) {
        updateBattery();
    }
    
    // Update thermal
    if (config_.enableThermalMonitor) {
        updateThermal();
    }
    
    // Add sample to history
    addSample();
    
    // Check for alerts
    checkAlerts();
}

float MobileProfiler::getAverageFPS(int frames) const {
    if (fpsHistory_.empty()) return currentSample_.fps;
    
    int count = std::min(frames, static_cast<int>(fpsHistory_.size()));
    float sum = 0.0f;
    for (int i = 0; i < count; ++i) {
        sum += fpsHistory_[fpsHistory_.size() - 1 - i];
    }
    return sum / count;
}

float MobileProfiler::getMinFPS(int frames) const {
    if (fpsHistory_.empty()) return currentSample_.fps;
    
    int count = std::min(frames, static_cast<int>(fpsHistory_.size()));
    float min = fpsHistory_.front();
    for (int i = 0; i < count; ++i) {
        min = std::min(min, fpsHistory_[i]);
    }
    return min;
}

float MobileProfiler::getMaxFPS(int frames) const {
    if (fpsHistory_.empty()) return currentSample_.fps;
    
    int count = std::min(frames, static_cast<int>(fpsHistory_.size()));
    float max = fpsHistory_.front();
    for (int i = 0; i < count; ++i) {
        max = std::max(max, fpsHistory_[i]);
    }
    return max;
}

void MobileProfiler::setRenderStats(int drawCalls, int triangleCount) {
    currentSample_.drawCalls = drawCalls;
    currentSample_.triangleCount = triangleCount;
}

void MobileProfiler::setMemoryStats(float textureMB, float vertexMB) {
    currentSample_.textureMemoryMB = textureMB;
    currentSample_.vertexMemoryMB = vertexMB;
}

void MobileProfiler::beginSection(const std::string& name) {
    if (sectionIndex_ >= 16) return;
    sectionStartTimes_[sectionIndex_] = std::chrono::steady_clock::now();
    sectionIndex_++;
}

void MobileProfiler::endSection(const std::string& name) {
    if (sectionIndex_ <= 0) return;
    sectionIndex_--;
    
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration<float, std::milli>(now - sectionStartTimes_[sectionIndex_]).count();
    sectionTimes_[sectionIndex_] = duration;
}

void MobileProfiler::setFrameTimeComponent(const std::string& component, float timeMs) {
    if (component == "cpu") currentFrameTiming_.cpu = timeMs;
    else if (component == "gpu") currentFrameTiming_.gpu = timeMs;
    else if (component == "render") currentFrameTiming_.render = timeMs;
    else if (component == "physics") currentFrameTiming_.physics = timeMs;
    else if (component == "animation") currentFrameTiming_.animation = timeMs;
    else if (component == "ui") currentFrameTiming_.ui = timeMs;
}

void MobileProfiler::updateBatteryData(float level, bool isCharging, float voltage, float current) {
    batteryStats_.currentLevel = level;
    batteryStats_.isCharging = isCharging;
    batteryStats_.isFull = (level >= 99.0f);
    batteryStats_.voltage = voltage;
    batteryStats_.current = current;
    batteryStats_.power = voltage * current;
    
    // Calculate drain rate
    if (!isCharging && sessionStartBattery_ > 0) {
        auto elapsed = std::chrono::duration<float>(
            std::chrono::steady_clock::now() - sessionStartTime_).count();
        float hours = elapsed / 3600.0f;
        if (hours > 0.01f) {
            float drained = sessionStartBattery_ - level;
            batteryStats_.drainRate = drained / hours;
        }
    }
    
    // Estimate remaining time
    if (!isCharging && batteryStats_.drainRate > 0) {
        batteryStats_.estimatedMinutes = static_cast<int>(
            (batteryStats_.currentLevel / batteryStats_.drainRate) * 60);
    } else {
        batteryStats_.estimatedMinutes = -1;
    }
    
    currentSample_.batteryLevel = level;
    currentSample_.batteryDrainRate = batteryStats_.drainRate;
}

void MobileProfiler::updateThermalData(float cpuTemp, float gpuTemp) {
    currentSample_.cpuTemp = cpuTemp;
    currentSample_.gpuTemp = gpuTemp;
    
    // Update thermal zones
    if (!thermalZones_.empty()) {
        thermalZones_[0].temperature = cpuTemp;
        thermalZones_[0].isThrottling = (cpuTemp >= thermalZones_[0].throttleTemp);
        
        if (thermalZones_.size() > 1) {
            thermalZones_[1].temperature = gpuTemp;
            thermalZones_[1].isThrottling = (gpuTemp >= thermalZones_[1].throttleTemp);
        }
    }
}

void MobileProfiler::setShowOverlay(bool show) {
    config_.showOverlay = show;
    std::cout << "[MobileProfiler] Overlay " << (show ? "enabled" : "disabled") << std::endl;
}

void MobileProfiler::resetStats() {
    sampleHistory_.clear();
    fpsHistory_.clear();
    recentAlerts_.clear();
    peakMemoryMB_ = 0.0f;
    frameCount_ = 0;
    fpsFrameCount_ = 0;
    fpsAccumulator_ = 0.0f;
    
    sessionStartTime_ = std::chrono::steady_clock::now();
    if (batteryStats_.currentLevel > 0) {
        sessionStartBattery_ = batteryStats_.currentLevel;
    }
    
    std::cout << "[MobileProfiler] Statistics reset" << std::endl;
}

bool MobileProfiler::exportToCSV(const std::string& filePath) {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "[MobileProfiler] Failed to open file: " << filePath << std::endl;
        return false;
    }
    
    // Write header
    file << "Timestamp,FPS,FrameTime,CPU%,GPU%,MemoryMB,Battery%,CPUTemp,GPUTemp,DrawCalls,Triangles\n";
    
    // Write samples
    file << std::fixed << std::setprecision(2);
    for (const auto& sample : sampleHistory_) {
        file << sample.timestamp << ","
             << sample.fps << ","
             << sample.frameTime << ","
             << sample.cpuUsage << ","
             << sample.gpuUsage << ","
             << sample.memoryMB << ","
             << sample.batteryLevel << ","
             << sample.cpuTemp << ","
             << sample.gpuTemp << ","
             << sample.drawCalls << ","
             << sample.triangleCount << "\n";
    }
    
    file.close();
    std::cout << "[MobileProfiler] Exported " << sampleHistory_.size() 
              << " samples to " << filePath << std::endl;
    
    return true;
}

void MobileProfiler::onAlert(AlertCallback callback) {
    alertCallbacks_.push_back(callback);
}

void MobileProfiler::onSample(SampleCallback callback) {
    sampleCallbacks_.push_back(callback);
}

void MobileProfiler::updateFPS() {
    if (fpsAccumulator_ > 0) {
        currentSample_.fps = (fpsFrameCount_ * 1000.0f) / fpsAccumulator_;
    }
    
    fpsHistory_.push_back(currentSample_.fps);
    if (fpsHistory_.size() > static_cast<size_t>(config_.sampleHistorySize)) {
        fpsHistory_.erase(fpsHistory_.begin());
    }
    
    fpsFrameCount_ = 0;
    fpsAccumulator_ = 0.0f;
}

void MobileProfiler::updateMemory() {
    // Platform-specific memory query would go here
    // This is a placeholder that would call into native code
    
#if PHOENIX_IOS
    // iOS: Would use mach_task_basic_info
#elif PHOENIX_ANDROID
    // Android: Would use ActivityManager.getMemoryInfo()
#endif
    
    // Update peak
    peakMemoryMB_ = std::max(peakMemoryMB_, currentSample_.memoryMB);
}

void MobileProfiler::updateBattery() {
    // Battery data is updated via updateBatteryData from platform
}

void MobileProfiler::updateThermal() {
    // Thermal data is updated via updateThermalData from platform
    
    // Check thermal throttling
    for (auto& zone : thermalZones_) {
        zone.isThrottling = (zone.temperature >= zone.throttleTemp);
    }
}

void MobileProfiler::checkAlerts() {
    // FPS alerts
    if (currentSample_.fps > 0) {
        if (currentSample_.fps < config_.criticalFPS) {
            PerformanceAlert alert;
            alert.metric = MetricType::FPS;
            alert.level = AlertLevel::Critical;
            alert.message = "Critical: FPS below " + std::to_string(config_.criticalFPS);
            alert.value = currentSample_.fps;
            alert.threshold = config_.criticalFPS;
            alert.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
            notifyAlert(alert);
        } else if (currentSample_.fps < config_.warningFPS) {
            PerformanceAlert alert;
            alert.metric = MetricType::FPS;
            alert.level = AlertLevel::Warning;
            alert.message = "Warning: FPS below " + std::to_string(config_.warningFPS);
            alert.value = currentSample_.fps;
            alert.threshold = config_.warningFPS;
            alert.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
            notifyAlert(alert);
        }
    }
    
    // Temperature alerts
    float maxTemp = std::max(currentSample_.cpuTemp, currentSample_.gpuTemp);
    if (maxTemp > 0) {
        if (maxTemp >= config_.criticalTemp) {
            PerformanceAlert alert;
            alert.metric = MetricType::Temperature;
            alert.level = AlertLevel::Critical;
            alert.message = "Critical: Temperature at " + std::to_string(maxTemp) + "°C";
            alert.value = maxTemp;
            alert.threshold = config_.criticalTemp;
            alert.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
            notifyAlert(alert);
        } else if (maxTemp >= config_.warningTemp) {
            PerformanceAlert alert;
            alert.metric = MetricType::Temperature;
            alert.level = AlertLevel::Warning;
            alert.message = "Warning: Temperature at " + std::to_string(maxTemp) + "°C";
            alert.value = maxTemp;
            alert.threshold = config_.warningTemp;
            alert.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
            notifyAlert(alert);
        }
    }
    
    // Memory alerts
    if (currentSample_.memoryMB > 0) {
        if (currentSample_.memoryMB >= config_.criticalMemoryMB) {
            PerformanceAlert alert;
            alert.metric = MetricType::MemoryUsage;
            alert.level = AlertLevel::Critical;
            alert.message = "Critical: Memory at " + std::to_string(currentSample_.memoryMB) + "MB";
            alert.value = currentSample_.memoryMB;
            alert.threshold = config_.criticalMemoryMB;
            alert.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
            notifyAlert(alert);
        } else if (currentSample_.memoryMB >= config_.warningMemoryMB) {
            PerformanceAlert alert;
            alert.metric = MetricType::MemoryUsage;
            alert.level = AlertLevel::Warning;
            alert.message = "Warning: Memory at " + std::to_string(currentSample_.memoryMB) + "MB";
            alert.value = currentSample_.memoryMB;
            alert.threshold = config_.warningMemoryMB;
            alert.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
            notifyAlert(alert);
        }
    }
}

void MobileProfiler::addSample() {
    currentSample_.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    sampleHistory_.push_back(currentSample_);
    
    if (sampleHistory_.size() > static_cast<size_t>(config_.sampleHistorySize)) {
        sampleHistory_.erase(sampleHistory_.begin());
    }
    
    notifySample(currentSample_);
}

void MobileProfiler::notifyAlert(const PerformanceAlert& alert) {
    recentAlerts_.push_back(alert);
    
    // Keep last 50 alerts
    if (recentAlerts_.size() > 50) {
        recentAlerts_.erase(recentAlerts_.begin());
    }
    
    for (auto& callback : alertCallbacks_) {
        try {
            callback(alert);
        } catch (...) {
            std::cerr << "[MobileProfiler] Alert callback exception" << std::endl;
        }
    }
    
    std::cout << "[MobileProfiler] ALERT: " << alert.message << std::endl;
}

void MobileProfiler::notifySample(const PerformanceSample& sample) {
    for (auto& callback : sampleCallbacks_) {
        try {
            callback(sample);
        } catch (...) {
            std::cerr << "[MobileProfiler] Sample callback exception" << std::endl;
        }
    }
}

float MobileProfiler::calculateAverage(const std::vector<float>& values) const {
    if (values.empty()) return 0.0f;
    float sum = 0.0f;
    for (float v : values) sum += v;
    return sum / values.size();
}

} // namespace phoenix::mobile
