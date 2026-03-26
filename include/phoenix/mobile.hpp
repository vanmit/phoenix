#pragma once

/**
 * @file mobile.hpp
 * @brief Phoenix Engine Mobile Optimization Module
 * 
 * Complete mobile optimization system for iOS and Android:
 * - Power Management: Dynamic resolution, frame rate limiting, thermal throttling
 * - Memory Optimization: Texture/mesh compression, streaming, auto-unload
 * - Touch Input: Multi-touch, gestures, stylus, virtual controllers
 * - Platform Features: Safe areas, notch adaptation, orientation, notifications
 * - Performance Profiling: FPS, memory, battery, thermal monitoring
 * 
 * @author Phoenix Engine Team
 * @version 1.0.0
 * @date 2026-03-26
 */

#include "phoenix/mobile/power/PowerManager.hpp"
#include "phoenix/mobile/memory/MemoryManager.hpp"
#include "phoenix/mobile/input/TouchInput.hpp"
#include "phoenix/mobile/platform/MobilePlatform.hpp"
#include "phoenix/mobile/profiler/MobileProfiler.hpp"

namespace phoenix::mobile {

/**
 * @brief Mobile engine version
 */
constexpr int VERSION_MAJOR = 1;
constexpr int VERSION_MINOR = 0;
constexpr int VERSION_PATCH = 0;

/**
 * @brief Initialize all mobile subsystems
 * @return true if successful
 */
inline bool initialize() {
    bool success = true;
    
    success &= PowerManager::getInstance().initialize();
    success &= MemoryManager::getInstance().initialize();
    success &= TouchInput::getInstance().initialize();
    success &= MobilePlatform::getInstance().initialize();
    success &= MobileProfiler::getInstance().initialize();
    
    return success;
}

/**
 * @brief Shutdown all mobile subsystems
 */
inline void shutdown() {
    PowerManager::getInstance().shutdown();
    MemoryManager::getInstance().shutdown();
    TouchInput::getInstance().shutdown();
    MobilePlatform::getInstance().shutdown();
    MobileProfiler::getInstance().shutdown();
}

/**
 * @brief Update all mobile subsystems (call once per frame)
 * @param deltaTime Frame time in seconds
 */
inline void update(float deltaTime) {
    PowerManager::getInstance().update(deltaTime);
    MemoryManager::getInstance().update();
    TouchInput::getInstance().update();
    MobilePlatform::getInstance().update(deltaTime);
    MobileProfiler::getInstance().update();
}

/**
 * @brief Begin frame (call at start of frame)
 */
inline void beginFrame() {
    MobileProfiler::getInstance().beginFrame();
}

/**
 * @brief End frame (call at end of frame)
 */
inline void endFrame() {
    MobileProfiler::getInstance().endFrame();
}

/**
 * @brief Handle application pause (background)
 */
inline void onPause() {
    PowerManager::getInstance().onEnterBackground();
}

/**
 * @brief Handle application resume (foreground)
 */
inline void onResume() {
    PowerManager::getInstance().onEnterForeground();
}

/**
 * @brief Handle memory warning from platform
 * @param level Warning level (0-3)
 */
inline void onMemoryWarning(int level) {
    MemoryManager::getInstance().handleMemoryWarning(level);
}

} // namespace phoenix::mobile
