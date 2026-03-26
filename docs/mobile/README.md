# Phoenix Engine Mobile Optimization (Phase 4)

Complete mobile optimization system for iOS and Android platforms.

## Features

### 1. Power Management ⚡
- Dynamic resolution scaling
- Frame rate limiting (30/60/90/120 FPS)
- GPU frequency regulation
- Background pause mechanism
- Thermal throttling detection

### 2. Memory Optimization 🧠
- Texture compression (ASTC/ETC2/PVRTC)
- Mesh compression (Draco/meshoptimizer)
- Streaming resource loading
- Memory warning handling
- Automatic resource unloading

### 3. Touch Input 👆
- Multi-touch support (up to 10 points)
- Gesture recognition (tap, swipe, pinch, rotate)
- Stylus pressure sensitivity
- Virtual controllers (buttons and sticks)

### 4. Platform Features 📱
- iOS Notch / Dynamic Island adaptation
- Safe area handling
- Screen orientation support
- Status bar integration
- Notification handling

### 5. Performance Profiling 📊
- FPS counter
- Frame timing breakdown
- Memory monitoring
- Battery consumption tracking
- Thermal monitoring
- Performance alerts

## Technical Specifications

| Requirement | Target | Status |
|------------|--------|--------|
| Platform | iOS 13+ / Android 10+ | ✅ |
| Memory | < 256 MB | ✅ |
| Battery | < 10% / hour | ✅ |
| Temperature | < 45°C | ✅ |
| Frame Rate | 30/60 FPS | ✅ |

## Quick Start

### Initialization

```cpp
#include <phoenix/mobile.hpp>

// Initialize all mobile subsystems
phoenix::mobile::initialize();

// In your game loop
void gameLoop() {
    phoenix::mobile::beginFrame();
    phoenix::mobile::update(deltaTime);
    // ... render ...
    phoenix::mobile::endFrame();
}

// Handle lifecycle
void onPause() { phoenix::mobile::onPause(); }
void onResume() { phoenix::mobile::onResume(); }
```

### Power Management

```cpp
auto& powerManager = phoenix::mobile::PowerManager::getInstance();

// Configure
PowerConfig config;
config.enableDynamicResolution = true;
config.enableThermalThrottling = true;
powerManager.initialize(config);

// Set frame rate
powerManager.setFrameRateMode(FrameRateMode::FPS_60);

// Get current resolution scale for rendering
float scale = powerManager.getResolutionScale();
```

### Memory Management

```cpp
auto& memoryManager = phoenix::mobile::MemoryManager::getInstance();

// Configure
MemoryConfig config;
config.maxMemoryMB = 256;
config.defaultTextureCompression = TextureCompression::ASTC_4x4;
memoryManager.initialize(config);

// Track resources
memoryManager.registerTexture("texture_id", sizeInBytes);
memoryManager.registerMesh("mesh_id", sizeInBytes);
```

### Touch Input

```cpp
auto& touchInput = phoenix::mobile::TouchInput::getInstance();

// Handle gestures
touchInput.onGesture([](const Gesture& gesture) {
    if (gesture.type == GestureType::Pinch) {
        camera.zoom(gesture.scale);
    }
});

// Add virtual controls
touchInput.addVirtualButton("jump", 0.8f, 0.8f, 0.15f, 0.15f,
    [](bool pressed) { if (pressed) player.jump(); });
```

### Performance Profiling

```cpp
auto& profiler = phoenix::mobile::MobileProfiler::getInstance();

// Get metrics
float fps = profiler.getCurrentFPS();
float memoryMB = profiler.getCurrentMemoryMB();
float cpuTemp = profiler.getCPUTemperature();

// Set alerts
profiler.onAlert([](const PerformanceAlert& alert) {
    if (alert.level == AlertLevel::Critical) {
        // Take action
    }
});
```

## Documentation

- [Performance Optimization Guide](PERFORMANCE-OPTIMIZATION-GUIDE.md)
- [Power Consumption Test Report](POWER-CONSUMPTION-TEST-REPORT.md)

## Example Projects

- [iOS Example](../../examples/ios/PhoenixEngine/)
- [Android Example](../../examples/android/PhoenixEngine/)

## API Reference

### PowerManager
- `initialize(config)` - Initialize with configuration
- `setFrameRateMode(mode)` - Set target frame rate
- `setResolutionScale(scale)` - Set resolution scale
- `getThermalData()` - Get thermal status
- `getBatteryStatus()` - Get battery status
- `onEnterBackground()` - Handle background
- `onEnterForeground()` - Handle foreground

### MemoryManager
- `initialize(config)` - Initialize with configuration
- `registerTexture(id, size)` - Track texture
- `registerMesh(id, size)` - Track mesh
- `getStats()` - Get memory statistics
- `handleMemoryWarning(level)` - Handle platform warning
- `unloadLRUResources(mb)` - Free memory

### TouchInput
- `initialize(config)` - Initialize with configuration
- `beginTouch(id, x, y, pressure)` - Touch started
- `moveTouch(id, x, y, pressure)` - Touch moved
- `endTouch(id)` - Touch ended
- `onGesture(callback)` - Register gesture handler
- `addVirtualButton(...)` - Add on-screen button
- `addVirtualStick(...)` - Add on-screen stick

### MobilePlatform
- `initialize(config)` - Initialize with configuration
- `getSafeAreaEdges()` - Get safe area
- `getNotchInfo()` - Get notch information
- `setStatusBarStyle(style)` - Set status bar
- `showNotification(notification)` - Show notification

### MobileProfiler
- `initialize(config)` - Initialize with configuration
- `beginFrame()` - Start frame timing
- `endFrame()` - End frame timing
- `getCurrentFPS()` - Get current FPS
- `getBatteryStats()` - Get battery info
- `onAlert(callback)` - Register alert handler

## Build

```bash
cd phoenix-engine
mkdir build && cd build
cmake ..
make -j8
```

## Testing

Run on real devices for accurate performance measurements:

```bash
# iOS
xcodebuild -scheme PhoenixEngine -destination 'platform=iOS,name=iPhone 15'

# Android
./gradlew assembleDebug
adb install app/build/outputs/apk/debug/app-debug.apk
```

## Performance Targets

| Metric | Target | Warning | Critical |
|--------|--------|---------|----------|
| FPS | 60 | < 45 | < 30 |
| Memory | < 200 MB | > 220 MB | > 240 MB |
| CPU Temp | < 40°C | > 42°C | > 45°C |
| Battery | < 8%/hr | > 10%/hr | > 15%/hr |

## License

Part of Phoenix Engine. See main LICENSE file.

---

**Phase 4 Status**: ✅ Complete  
**Date**: March 26, 2026  
**Version**: 1.0.0
