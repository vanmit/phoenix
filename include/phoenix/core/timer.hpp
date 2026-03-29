/**
 * Phoenix Engine - Timer Header
 * 
 * High-resolution timer for profiling and time measurement
 */

#pragma once

#include <cstdint>
#include <memory>

namespace phoenix::core {

/**
 * @brief High-resolution timer implementation
 */
class Timer {
public:
    /**
     * @brief Construct a new Timer
     */
    Timer();
    
    /**
     * @brief Destroy the Timer
     */
    ~Timer();
    
    /**
     * @brief Start the timer
     */
    void start();
    
    /**
     * @brief Stop the timer
     */
    void stop();
    
    /**
     * @brief Reset the timer to zero
     */
    void reset();
    
    /**
     * @brief Get elapsed time in seconds
     * @return Elapsed time in seconds
     */
    double elapsed() const;
    
    /**
     * @brief Get delta time since last tick
     * @return Delta time in seconds
     */
    double delta() const;
    
    /**
     * @brief Update the tick time for delta calculation
     */
    void tick();
    
    /**
     * @brief Get elapsed time in seconds (float)
     * @return Elapsed time in seconds
     */
    float elapsedSeconds() const;
    
    /**
     * @brief Get elapsed time in milliseconds
     * @return Elapsed time in milliseconds
     */
    double elapsedMilliseconds() const;
    
    /**
     * @brief Get elapsed time in microseconds
     * @return Elapsed time in microseconds
     */
    int64_t elapsedMicroseconds() const;
    
    /**
     * @brief Get elapsed time in nanoseconds
     * @return Elapsed time in nanoseconds
     */
    int64_t elapsedNanoseconds() const;
    
    /**
     * @brief Get current time since epoch in seconds
     * @return Current time in seconds
     */
    static double getCurrentTime();
    
    /**
     * @brief Sleep for specified duration
     * @param seconds Duration in seconds
     */
    static void sleep(double seconds);

private:
    class TimerImpl;
    std::unique_ptr<TimerImpl> m_impl;
};

} // namespace phoenix::core
