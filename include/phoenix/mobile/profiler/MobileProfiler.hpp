#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace phoenix::mobile {

/**
 * @brief Performance metric types
 */
enum class MetricType {
    FPS,
    FrameTime,
    CPUUsage,
    GPUUsage,
    MemoryUsage,
    BatteryLevel,
    BatteryDrain,
    Temperature,
    DrawCalls,
    TriangleCount,
    TextureMemory,
    VertexMemory
};

/**
 * @brief Performance sample data
 */
struct PerformanceSample {
    uint64_t timestamp = 0;
    float fps = 0.0f;
    float frameTime = 0.0f;
    float cpuUsage = 0.0f;
    float gpuUsage = 0.0f;
    float memoryMB = 0.0f;
    float batteryLevel = 0.0f;
    float batteryDrainRate = 0.0f;  // % per hour
    float cpuTemp = 0.0f;
    float gpuTemp = 0.0f;
    int drawCalls = 0;
    int triangleCount = 0;
    float textureMemoryMB = 0.0f;
    float vertexMemoryMB = 0.0f;
};

/**
 * @brief Frame timing data
 */
struct FrameTiming {
    float total = 0.0f;
    float cpu = 0.0f;
    float gpu = 0.0f;
    float vsync = 0.0f;
    float render = 0.0f;
    float physics = 0.0f;
    float animation = 0.0f;
    float ui = 0.0f;
    float other = 0.0f;
};

/**
 * @brief Thermal zone data
 */
struct ThermalZone {
    std::string name;
    float temperature = 0.0f;
    float maxTemp = 0.0f;
    float throttleTemp = 0.0f;
    bool isThrottling = false;
};

/**
 * @brief Battery statistics
 */
struct BatteryStats {
    float currentLevel = 100.0f;      // Percentage
    float startLevel = 100.0f;        // Level at session start
    float drainRate = 0.0f;           // % per hour
    float estimatedMinutes = 0;       // Minutes remaining
    bool isCharging = false;
    bool isFull = false;
    float voltage = 0.0f;
    float current = 0.0f;             // mA
    float power = 0.0f;               // mW
    int chargeCycles = 0;
    float health = 100.0f;            // Battery health %
};

/**
 * @brief Profiler configuration
 */
struct ProfilerConfig {
    bool enableFPSCounter = true;
    bool enableMemoryMonitor = true;
    bool enableBatteryMonitor = true;
    bool enableThermalMonitor = true;
    bool enableFrameTiming = true;
    bool enableGPUProfiler = true;
    bool showOverlay = false;
    int sampleHistorySize = 300;      // 5 seconds at 60fps
    float updateInterval = 0.1f;      // Seconds between updates
    float warningFPS = 30.0f;
    float criticalFPS = 20.0f;
    float warningTemp = 40.0f;        // Celsius
    float criticalTemp = 45.0f;       // Celsius
    float warningMemoryMB = 200.0f;
    float criticalMemoryMB = 240.0f;
};

/**
 * @brief Performance alert levels
 */
enum class AlertLevel {
    None,
    Warning,
    Critical
};

/**
 * @brief Performance alert
 */
struct PerformanceAlert {
    MetricType metric;
    AlertLevel level;
    std::string message;
    float value;
    float threshold;
    uint64_t timestamp;
};

/**
 * @brief Main mobile performance profiler
 * 
 * Handles:
 * - FPS counter
 * - Memory monitoring
 * - Battery consumption tracking
 * - Thermal monitoring
 * - Frame timing breakdown
 * - Performance alerts
 */
class MobileProfiler {
public:
    using AlertCallback = std::function<void(const PerformanceAlert&)>;
    using SampleCallback = std::function<void(const PerformanceSample&)>;

    static MobileProfiler& getInstance();

    /**
     * @brief Initialize profiler
     */
    bool initialize(const ProfilerConfig& config = ProfilerConfig());

    /**
     * @brief Shutdown profiler
     */
    void shutdown();

    /**
     * @brief Begin frame (call at start of frame)
     */
    void beginFrame();

    /**
     * @brief End frame (call at end of frame)
     */
    void endFrame();

    /**
     * @brief Update profiler (call once per frame)
     */
    void update();

    /**
     * @brief Get current FPS
     */
    float getCurrentFPS() const { return currentSample_.fps; }

    /**
     * @brief Get average FPS over last N frames
     */
    float getAverageFPS(int frames = 60) const;

    /**
     * @brief Get minimum FPS over last N frames
     */
    float getMinFPS(int frames = 60) const;

    /**
     * @brief Get maximum FPS over last N frames
     */
    float getMaxFPS(int frames = 60) const;

    /**
     * @brief Get current frame time in ms
     */
    float getCurrentFrameTime() const { return currentSample_.frameTime; }

    /**
     * @brief Get frame timing breakdown
     */
    FrameTiming getFrameTiming() const { return currentFrameTiming_; }

    /**
     * @brief Get current memory usage in MB
     */
    float getCurrentMemoryMB() const { return currentSample_.memoryMB; }

    /**
     * @brief Get peak memory usage in MB
     */
    float getPeakMemoryMB() const { return peakMemoryMB_; }

    /**
     * @brief Get battery statistics
     */
    BatteryStats getBatteryStats() const { return batteryStats_; }

    /**
     * @brief Get current CPU temperature
     */
    float getCPUTemperature() const { return currentSample_.cpuTemp; }

    /**
     * @brief Get current GPU temperature
     */
    float getGPUTemperature() const { return currentSample_.gpuTemp; }

    /**
     * @brief Get thermal zones
     */
    std::vector<ThermalZone> getThermalZones() const { return thermalZones_; }

    /**
     * @brief Get current performance sample
     */
    PerformanceSample getCurrentSample() const { return currentSample_; }

    /**
     * @brief Get sample history
     */
    const std::vector<PerformanceSample>& getSampleHistory() const { return sampleHistory_; }

    /**
     * @brief Get recent alerts
     */
    const std::vector<PerformanceAlert>& getRecentAlerts() const { return recentAlerts_; }

    /**
     * @brief Set render statistics
     */
    void setRenderStats(int drawCalls, int triangleCount);

    /**
     * @brief Set memory statistics
     */
    void setMemoryStats(float textureMB, float vertexMB);

    /**
     * @brief Begin timing section
     * @param name Section name
     */
    void beginSection(const std::string& name);

    /**
     * @brief End timing section
     */
    void endSection(const std::string& name);

    /**
     * @brief Set frame timing component
     */
    void setFrameTimeComponent(const std::string& component, float timeMs);

    /**
     * @brief Update platform battery data
     */
    void updateBatteryData(float level, bool isCharging, float voltage, float current);

    /**
     * @brief Update platform thermal data
     */
    void updateThermalData(float cpuTemp, float gpuTemp);

    /**
     * @brief Enable/disable overlay display
     */
    void setShowOverlay(bool show);

    /**
     * @brief Check if overlay is shown
     */
    bool isShowingOverlay() const { return config_.showOverlay; }

    /**
     * @brief Reset statistics
     */
    void resetStats();

    /**
     * @brief Export performance data to CSV
     * @param filePath Output file path
     */
    bool exportToCSV(const std::string& filePath);

    /**
     * @brief Register alert callback
     */
    void onAlert(AlertCallback callback);

    /**
     * @brief Register sample callback
     */
    void onSample(SampleCallback callback);

private:
    MobileProfiler() = default;
    ~MobileProfiler() = default;
    MobileProfiler(const MobileProfiler&) = delete;
    MobileProfiler& operator=(const MobileProfiler&) = delete;

    void updateFPS();
    void updateMemory();
    void updateBattery();
    void updateThermal();
    void checkAlerts();
    void addSample();
    void notifyAlert(const PerformanceAlert& alert);
    void notifySample(const PerformanceSample& sample);
    float calculateAverage(const std::vector<float>& values) const;

    ProfilerConfig config_;
    PerformanceSample currentSample_;
    FrameTiming currentFrameTiming_;
    BatteryStats batteryStats_;
    std::vector<ThermalZone> thermalZones_;
    
    std::vector<PerformanceSample> sampleHistory_;
    std::vector<PerformanceAlert> recentAlerts_;
    std::vector<float> fpsHistory_;
    
    std::vector<AlertCallback> alertCallbacks_;
    std::vector<SampleCallback> sampleCallbacks_;
    
    std::chrono::steady_clock::time_point lastUpdateTime_;
    std::chrono::steady_clock::time_point sessionStartTime_;
    
    float peakMemoryMB_ = 0.0f;
    float sessionStartBattery_ = 100.0f;
    uint32_t frameCount_ = 0;
    uint32_t fpsFrameCount_ = 0;
    float fpsAccumulator_ = 0.0f;
    
    bool isInitialized_ = false;
    
    // Section timing
    std::chrono::steady_clock::time_point frameStartTime_;
    std::chrono::steady_clock::time_point sectionStartTimes_[16];
    float sectionTimes_[16] = {0};
    int sectionIndex_ = 0;
};

} // namespace phoenix::mobile
