/**
 * Phoenix Engine - Timer
 * 
 * High-resolution timer for profiling and time measurement
 */

#include "../../include/phoenix/core/timer.hpp"
#include <chrono>
#include <thread>

namespace phoenix::core {

// Timer implementation using high_resolution_clock
class TimerImpl {
public:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;
    using Duration = std::chrono::duration<double>;
    
    TimePoint startTime;
    TimePoint lastTickTime;
    double elapsedTime = 0.0;
    bool isRunning = false;
};

Timer::Timer() : m_impl(new TimerImpl()) {
    reset();
}

Timer::~Timer() = default;

void Timer::start() {
    m_impl->startTime = TimerImpl::Clock::now();
    m_impl->lastTickTime = m_impl->startTime;
    m_impl->isRunning = true;
    m_impl->elapsedTime = 0.0;
}

void Timer::stop() {
    if (m_impl->isRunning) {
        auto now = TimerImpl::Clock::now();
        m_impl->elapsedTime += Duration(now - m_impl->lastTickTime).count();
        m_impl->isRunning = false;
    }
}

void Timer::reset() {
    m_impl->startTime = TimerImpl::Clock::now();
    m_impl->lastTickTime = m_impl->startTime;
    m_impl->elapsedTime = 0.0;
    m_impl->isRunning = false;
}

double Timer::elapsed() const {
    auto now = TimerImpl::Clock::now();
    
    if (m_impl->isRunning) {
        return m_impl->elapsedTime + Duration(now - m_impl->lastTickTime).count();
    } else {
        return m_impl->elapsedTime + Duration(now - m_impl->startTime).count();
    }
}

double Timer::delta() const {
    auto now = TimerImpl::Clock::now();
    double delta = Duration(now - m_impl->lastTickTime).count();
    return delta;
}

void Timer::tick() {
    m_impl->lastTickTime = TimerImpl::Clock::now();
}

float Timer::elapsedSeconds() const {
    return static_cast<float>(elapsed());
}

double Timer::elapsedMilliseconds() const {
    return elapsed() * 1000.0;
}

int64_t Timer::elapsedMicroseconds() const {
    return static_cast<int64_t>(elapsed() * 1000000.0);
}

int64_t Timer::elapsedNanoseconds() const {
    return static_cast<int64_t>(elapsed() * 1000000000.0);
}

// Static utility functions
double Timer::getCurrentTime() {
    auto now = TimerImpl::Clock::now();
    static auto epoch = now;
    return Duration(now - epoch).count();
}

void Timer::sleep(double seconds) {
    if (seconds <= 0) return;
    
    auto now = TimerImpl::Clock::now();
    auto duration = std::chrono::duration<double>(seconds);
    std::this_thread::sleep_for(duration);
}

} // namespace phoenix::core
