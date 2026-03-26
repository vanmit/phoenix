#pragma once

#include "platform_types.hpp"
#include <cstdint>
#include <string>

namespace phoenix {
namespace platform {

// ============================================================================
// 高精度计时器
// ============================================================================

class Timer {
public:
    /**
     * @brief 初始化计时器
     */
    static void initialize();
    
    /**
     * @brief 获取当前时间 (秒)
     */
    [[nodiscard]] static double getTime();
    
    /**
     * @brief 获取当前时间 (毫秒)
     */
    [[nodiscard]] static double getTimeMs();
    
    /**
     * @brief 获取当前时间 (微秒)
     */
    [[nodiscard]] static uint64_t getTimeUs();
    
    /**
     * @brief 获取当前时间 (纳秒)
     */
    [[nodiscard]] static uint64_t getTimeNs();
    
    /**
     * @brief 获取频率 (Hz)
     */
    [[nodiscard]] static uint64_t getFrequency();
    
    /**
     * @brief 睡眠指定时间
     * @param seconds 秒数
     */
    static void sleep(double seconds);
    
    /**
     * @brief 睡眠指定毫秒
     */
    static void sleepMs(uint32_t ms);
    
    /**
     * @brief 睡眠指定微秒
     */
    static void sleepUs(uint32_t us);
};

// ============================================================================
// 帧计时器
// ============================================================================

class FrameTimer {
public:
    /**
     * @brief 开始新帧
     */
    void beginFrame();
    
    /**
     * @brief 结束帧并计算统计
     */
    void endFrame();
    
    /**
     * @brief 获取帧时间 (秒)
     */
    [[nodiscard]] double getFrameTime() const { return frameTime_; }
    
    /**
     * @brief 获取帧时间 (毫秒)
     */
    [[nodiscard]] float getFrameTimeMs() const { return static_cast<float>(frameTime_ * 1000.0); }
    
    /**
     * @brief 获取 FPS
     */
    [[nodiscard]] float getFPS() const { return fps_; }
    
    /**
     * @brief 获取总时间 (秒)
     */
    [[nodiscard]] double getTotalTime() const { return totalTime_; }
    
    /**
     * @brief 获取帧索引
     */
    [[nodiscard]] uint64_t getFrameIndex() const { return frameIndex_; }
    
    /**
     * @brief 获取 TimeInfo
     */
    [[nodiscard]] TimeInfo getTimeInfo() const;
    
    /**
     * @brief 重置计时器
     */
    void reset();

private:
    double frameTime_ = 0.0;
    double totalTime_ = 0.0;
    double appStartTime_ = 0.0;
    float fps_ = 0.0f;
    uint64_t frameIndex_ = 0;
    uint32_t frameTimeMs_ = 0;
    
    // FPS 计算
    double fpsAccumulator_ = 0.0;
    uint32_t fpsFrameCount_ = 0;
};

// ============================================================================
// 性能分析器
// ============================================================================

struct ProfileSample {
    std::string name;
    uint64_t startTime = 0;
    uint64_t endTime = 0;
    uint64_t durationNs = 0;
    uint32_t depth = 0;
    uint32_t threadId = 0;
};

class Profiler {
public:
    /**
     * @brief 开始采样
     */
    static void beginSample(const char* name);
    
    /**
     * @brief 结束采样
     */
    static void endSample();
    
    /**
     * @brief 记录瞬时事件
     */
    static void recordEvent(const char* name);
    
    /**
     * @brief 获取采样数据
     */
    [[nodiscard]] static const ProfileSample* getSamples();
    
    /**
     * @brief 获取采样数量
     */
    [[nodiscard]] static uint32_t getSampleCount();
    
    /**
     * @brief 重置采样数据
     */
    static void reset();
    
    /**
     * @brief 导出到 Chrome Trace 格式
     */
    static void exportToChromeTrace(const char* filename);
    
    /**
     * @brief 启用/禁用分析
     */
    static void setEnabled(bool enabled);
    [[nodiscard]] static bool isEnabled();
};

// ============================================================================
// RAII 性能分析作用域
// ============================================================================

class ProfileScope {
public:
    explicit ProfileScope(const char* name) : name_(name) {
        if (Profiler::isEnabled()) {
            Profiler::beginSample(name);
        }
    }
    
    ~ProfileScope() {
        if (Profiler::isEnabled()) {
            Profiler::endSample();
        }
    }
    
private:
    const char* name_;
};

// ============================================================================
// 辅助宏
// ============================================================================

#define PHOENIX_PROFILE_SCOPE(name) \
    phoenix::platform::ProfileScope PHOENIX_CONCAT(profile_scope, __LINE__)(name)

#define PHOENIX_PROFILE_FUNCTION() \
    PHOENIX_PROFILE_SCOPE(__FUNCTION__)

#define PHOENIX_CONCAT_IMPL(x, y) x##y
#define PHOENIX_CONCAT(x, y) PHOENIX_CONCAT_IMPL(x, y)

} // namespace platform
} // namespace phoenix
