# Phoenix Engine Mobile Performance Optimization Guide

## Overview

This guide covers best practices for optimizing Phoenix Engine on mobile devices (iOS 13+ / Android 10+) to meet the following targets:

- **Memory**: < 256MB
- **Battery**: < 10% per hour
- **Temperature**: < 45°C
- **Frame Rate**: Stable 30/60 FPS

## Table of Contents

1. [Power Management](#power-management)
2. [Memory Optimization](#memory-optimization)
3. [Touch Input](#touch-input)
4. [Platform Features](#platform-features)
5. [Performance Profiling](#performance-profiling)
6. [Best Practices](#best-practices)

---

## Power Management

### Dynamic Resolution Scaling

Automatically adjust rendering resolution based on performance:

```cpp
auto& powerManager = phoenix::mobile::PowerManager::getInstance();

// Enable dynamic resolution
PowerConfig config;
config.enableDynamicResolution = true;
config.resolutionScaleStep = 0.1f;  // Adjust in 10% increments
config.maxResolutionScaleSteps = 5;  // Max 5 steps down

powerManager.initialize(config);

// Engine will automatically adjust based on frame time
powerManager.adjustResolutionForPerformance(actualFrameTime, targetFrameTime);

// Get current scale for render target sizing
float scale = powerManager.getResolutionScale();
int renderWidth = screenWidth * scale;
int renderHeight = screenHeight * scale;
```

### Frame Rate Limiting

Switch between 30/60 FPS modes based on conditions:

```cpp
// Set frame rate mode
powerManager.setFrameRateMode(FrameRateMode::FPS_60);  // Normal
powerManager.setFrameRateMode(FrameRateMode::FPS_30);  // Power saving

// Adaptive mode (auto-adjusts)
powerManager.setFrameRateMode(FrameRateMode::Adaptive);

// Respond to power state changes
powerManager.onPowerStateChanged([](PowerState state) {
    switch (state) {
        case PowerState::Normal:
            // Full performance
            break;
        case PowerState::PowerSave:
            // Reduce quality
            break;
        case PowerState::Thermal:
            // Throttle aggressively
            break;
        case PowerState::Background:
            // Pause rendering
            break;
    }
});
```

### Thermal Throttling

Monitor and respond to device temperature:

```cpp
// Update thermal data from platform
powerManager.updateThermalData(cpuTemp, gpuTemp);

// Get thermal state
ThermalData thermal = powerManager.getThermalData();
if (thermal.state == ThermalState::Critical) {
    // Take action: reduce resolution, lower frame rate, etc.
    powerManager.setFrameRateMode(FrameRateMode::FPS_30);
    powerManager.setResolutionScale(0.7f);
}

// Monitor throttling factor
float throttling = thermal.throttlingFactor;  // 1.0 = no throttling
```

### Background Pause

Automatically pause when app is backgrounded:

```cpp
// In AppDelegate / Activity
void applicationWillResignActive() {
    phoenix::mobile::onPause();
}

void applicationDidBecomeActive() {
    phoenix::mobile::onResume();
}
```

---

## Memory Optimization

### Texture Compression

Use platform-appropriate compression formats:

```cpp
auto& memoryManager = phoenix::mobile::MemoryManager::getInstance();

// Get recommended compression for platform
TextureCompression compression = MemoryManager::getRecommendedTextureCompression();
// iOS: ASTC_4x4
// Android: ASTC_4x4 (or ETC2 for older devices)

// Calculate compressed size
size_t compressedSize = MemoryManager::calculateCompressedTextureSize(
    2048, 2048,  // Width, Height
    11,          // Mip levels
    TextureCompression::ASTC_4x4
);
// Result: ~1.7 MB vs 16 MB uncompressed (90% reduction!)

// Configure memory manager
MemoryConfig config;
config.defaultTextureCompression = TextureCompression::ASTC_4x4;
config.textureBudgetMB = 128;
memoryManager.initialize(config);
```

### Mesh Compression

Compress geometry with Draco or meshoptimizer:

```cpp
// Get recommended mesh compression
MeshCompression compression = MemoryManager::getRecommendedMeshCompression();
// Returns: MeshCompression::MeshOptimizer

// Configure
config.defaultMeshCompression = MeshCompression::MeshOptimizer;

// Typical compression ratios:
// - Draco: 90-95% reduction
// - meshoptimizer: 70-80% reduction (faster decode)
// - Quantized16: 50% reduction
// - Quantized8: 75% reduction (lossy)
```

### Streaming Loading

Load resources progressively:

```cpp
// Request streaming load
StreamRequest request;
request.path = "assets/characters/hero.mesh";
request.priority = ResourcePriority::High;
request.onComplete = [](bool success) {
    if (success) {
        // Resource loaded
    }
};

memoryManager.requestStreamLoad(request);

// Configure streaming
config.enableStreaming = true;
config.streamingBatchSize = 4;  // Load 4 resources per frame
```

### Memory Warning Handling

Respond to platform memory warnings:

```cpp
// Register callback
memoryManager.onMemoryWarning([](float pressure) {
    if (pressure > 0.9f) {
        // Critical: Unload everything possible
        memoryManager.forceGarbageCollection();
        memoryManager.unloadLRUResources(64);  // Free 64MB
    } else if (pressure > 0.8f) {
        // Warning: Unload unused resources
        memoryManager.unloadLRUResources(32);  // Free 32MB
    }
});

// Auto-unload configuration
config.enableAutoUnload = true;
config.unloadAfterFrames = 300;  // 5 seconds at 60fps
```

### Resource Tracking

Track and manage resource lifetimes:

```cpp
// Register resources
memoryManager.registerTexture("hero_diffuse", sizeInBytes);
memoryManager.registerMesh("hero_mesh", sizeInBytes);

// Mark as accessed (prevents auto-unload)
memoryManager.markResourceAccessed("hero_diffuse");

// Unregister when freed
memoryManager.unregisterTexture("hero_diffuse");

// Get stats
MemoryStats stats = memoryManager.getStats();
float pressure = memoryManager.getMemoryPressure();  // 0.0 - 1.0
```

---

## Touch Input

### Multi-Touch Support

Handle up to 10 simultaneous touch points:

```cpp
auto& touchInput = phoenix::mobile::TouchInput::getInstance();

// In touch handler
void onTouch(MotionEvent event) {
    for (int i = 0; i < event.pointerCount; i++) {
        float x = event.getX(i) / screenWidth;
        float y = event.getY(i) / screenHeight;
        float pressure = event.getPressure(i);
        
        touchInput.beginTouch(event.getPointerId(i), x, y, pressure);
    }
}
```

### Gesture Recognition

Recognize common gestures:

```cpp
touchInput.onGesture([](const Gesture& gesture) {
    switch (gesture.type) {
        case GestureType::Tap:
            // Single tap
            break;
        case GestureType::DoubleTap:
            // Double tap (zoom)
            break;
        case GestureType::Swipe:
            // Swipe direction
            switch (gesture.swipeDirection) {
                case SwipeDirection::Left: /* swipe left */ break;
                case SwipeDirection::Right: /* swipe right */ break;
                case SwipeDirection::Up: /* swipe up */ break;
                case SwipeDirection::Down: /* swipe down */ break;
            }
            break;
        case GestureType::Pinch:
            // Pinch to zoom
            float scale = gesture.scale;  // < 1.0 = pinch in, > 1.0 = pinch out
            break;
        case GestureType::Rotate:
            // Rotation gesture
            float angle = gesture.rotation;  // Radians
            break;
        case GestureType::LongPress:
            // Long press (context menu)
            break;
    }
});
```

### Stylus Pressure

Support pressure-sensitive input:

```cpp
// Enable stylus support
TouchConfig config;
config.enableStylusPressure = true;
touchInput.initialize(config);

// Set stylus data
touchInput.setStylusData(
    touchId,
    true,   // isStylus
    false,  // isEraser
    altitudeAngle,  // Radians from surface
    azimuthAngle    // Radians around stylus axis
);

// Get pressure from touch point
const TouchPoint* point = touchInput.getTouchPoint(touchId);
if (point && point->isStylus) {
    float pressure = point->pressure;      // 0.0 - 1.0
    float altitude = point->altitudeAngle; // Tilt
    float azimuth = point->azimuthAngle;   // Rotation
}
```

### Virtual Controllers

Add on-screen controls:

```cpp
// Add virtual button
touchInput.addVirtualButton(
    "jump_button",
    0.8f, 0.8f,     // Position (normalized)
    0.15f, 0.15f,   // Size
    [](bool pressed) {
        if (pressed) {
            player.jump();
        }
    }
);

// Add virtual analog stick
touchInput.addVirtualStick(
    "movement_stick",
    0.2f, 0.8f,     // Base position
    0.15f,          // Radius
    [](float x, float y) {
        // x, y in range -1.0 to 1.0
        player.move(x, y);
    }
);

// Get stick state
auto [stickX, stickY] = touchInput.getVirtualStickOutput("movement_stick");
```

---

## Platform Features

### Safe Area Handling

Respect notches and rounded corners:

```cpp
auto& platform = phoenix::mobile::MobilePlatform::getInstance();

// Get safe area edges (normalized 0-1)
SafeAreaEdges edges = platform.getSafeAreaEdges();

// Check if point is in safe area
if (platform.isPointInSafeArea(x, y)) {
    // Safe to place UI element
}

// Get safe content rectangle
auto rect = platform.getSafeContentRect();
// Returns: {x, y, width, height} in normalized coordinates

// Respond to safe area changes
platform.onSafeAreaChanged([](const SafeAreaEdges& edges) {
    // Adjust UI layout
    float safeTop = edges.top * screenHeight;
    float safeBottom = edges.bottom * screenHeight;
    // ...
});
```

### Notch Adaptation

Handle notches and dynamic islands:

```cpp
NotchInfo notch = platform.getNotchInfo();

if (notch.hasNotch) {
    // Avoid placing important UI in notch area
    float notchCenterX = notch.notchX * screenWidth;
    float notchWidth = notch.notchWidth * screenWidth;
    
    // Center content away from notch
    if (notch.hasDynamicIsland) {
        // Larger interactive area
    }
}
```

### Orientation Handling

Support multiple orientations:

```cpp
// Configure allowed orientations
PlatformConfig config;
config.allowPortrait = true;
config.allowLandscape = true;
config.allowUpsideDown = false;
platform.initialize(config);

// Lock to current orientation
platform.lockOrientation();

// Unlock to allow rotation
platform.unlockOrientation();

// Respond to orientation changes
platform.onOrientationChanged([](DeviceOrientation orientation) {
    switch (orientation) {
        case DeviceOrientation::Portrait:
            // Adjust UI for portrait
            break;
        case DeviceOrientation::LandscapeRight:
            // Adjust UI for landscape
            break;
        // ...
    }
});
```

### Status Bar Integration

Control status bar appearance:

```cpp
// Set style
platform.setStatusBarStyle(StatusBarStyle::Light);  // For dark backgrounds
platform.setStatusBarStyle(StatusBarStyle::Dark);   // For light backgrounds

// Set visibility
platform.setStatusBarVisibility(StatusBarVisibility::Hidden);
platform.setStatusBarVisibility(StatusBarVisibility::Visible);

// Auto-hide on interaction
PlatformConfig config;
config.hideStatusBarOnInteraction = true;
config.statusBarAutoHideTimeout = 3.0f;  // Seconds
```

---

## Performance Profiling

### FPS Counter

Monitor frame rate:

```cpp
auto& profiler = phoenix::mobile::MobileProfiler::getInstance();

// Get current FPS
float fps = profiler.getCurrentFPS();

// Get statistics
float avgFps = profiler.getAverageFPS(60);   // Last 60 frames
float minFps = profiler.getMinFPS(60);       // Minimum in last 60
float maxFps = profiler.getMaxFPS(60);       // Maximum in last 60
float frameTime = profiler.getCurrentFrameTime();  // ms
```

### Frame Timing

Break down frame time by component:

```cpp
// Begin frame
profiler.beginFrame();

// Time sections
profiler.beginSection("Physics");
// ... physics update ...
profiler.endSection("Physics");

profiler.beginSection("Animation");
// ... animation update ...
profiler.endSection("Animation");

// Or set component times directly
profiler.setFrameTimeComponent("render", renderTimeMs);
profiler.setFrameTimeComponent("ui", uiTimeMs);

// Get breakdown
FrameTiming timing = profiler.getFrameTiming();
// timing.total, timing.cpu, timing.gpu, timing.render, etc.

// End frame
profiler.endFrame();
```

### Memory Monitoring

Track memory usage:

```cpp
// Get current memory
float memoryMB = profiler.getCurrentMemoryMB();
float peakMB = profiler.getPeakMemoryMB();

// Get detailed stats
MemoryStats stats = MemoryManager::getInstance().getStats();
// stats.textureMemory, stats.meshMemory, stats.activeTextures, etc.
```

### Battery Monitoring

Track power consumption:

```cpp
BatteryStats battery = profiler.getBatteryStats();

float level = battery.currentLevel;           // Percentage
float drainRate = battery.drainRate;          // % per hour
int minutesRemaining = battery.estimatedMinutes;
bool isCharging = battery.isCharging;
float power = battery.power;                  // mW

// Update from platform
profiler.updateBatteryData(level, isCharging, voltage, current);
```

### Thermal Monitoring

Monitor device temperature:

```cpp
// Get temperatures
float cpuTemp = profiler.getCPUTemperature();
float gpuTemp = profiler.getGPUTemperature();

// Get thermal zones
auto zones = profiler.getThermalZones();
for (const auto& zone : zones) {
    if (zone.isThrottling) {
        // Device is thermally throttling
    }
}

// Update from platform
profiler.updateThermalData(cpuTemp, gpuTemp);
```

### Performance Alerts

Get notified of performance issues:

```cpp
profiler.onAlert([](const PerformanceAlert& alert) {
    switch (alert.level) {
        case AlertLevel::Warning:
            // Log warning
            break;
        case AlertLevel::Critical:
            // Take immediate action
            break;
    }
    
    // Example: FPS dropped
    if (alert.metric == MetricType::FPS) {
        // Reduce quality settings
    }
    
    // Example: Temperature too high
    if (alert.metric == MetricType::Temperature) {
        // Throttle rendering
    }
});
```

### Export Data

Export performance data for analysis:

```cpp
// Export to CSV
profiler.exportToCSV("/path/to/performance_data.csv");

// CSV columns:
// Timestamp, FPS, FrameTime, CPU%, GPU%, MemoryMB, Battery%, 
// CPUTemp, GPUTemp, DrawCalls, Triangles
```

---

## Best Practices

### General

1. **Profile Early**: Enable profiler from day one of development
2. **Test on Real Devices**: Emulators don't accurately represent performance
3. **Test on Low-End Devices**: Ensure acceptable performance on minimum spec
4. **Monitor Battery**: Use battery monitor during development
5. **Watch Thermals**: Prevent thermal throttling with proactive measures

### Memory

1. **Use Compression**: Always use ASTC/ETC2 for textures
2. **Stream Large Assets**: Don't load everything at startup
3. **Unload Aggressively**: Free resources when not visible
4. **Set Budgets**: Define memory budgets per resource type
5. **Handle Warnings**: Respond to memory warnings immediately

### Power

1. **Dynamic Resolution**: Let the engine adjust based on load
2. **Adaptive FPS**: Use 30 FPS in menus, 60 FPS in gameplay
3. **Pause in Background**: Always pause when backgrounded
4. **Monitor Thermals**: Reduce quality before throttling kicks in
5. **Test Battery Drain**: Measure %/hour during typical gameplay

### Input

1. **Support Multi-Touch**: Don't assume single touch
2. **Add Virtual Controls**: Provide on-screen alternatives
3. **Handle Gestures**: Common gestures improve UX
4. **Support Stylus**: Pressure sensitivity for creative apps
5. **Test on Tablets**: Larger screens need different UI

### Platform

1. **Respect Safe Areas**: Don't draw under notches
2. **Support Orientations**: Allow user choice when possible
3. **Handle Interruptions**: Calls, notifications, etc.
4. **Test on Various Devices**: Different manufacturers, screen sizes
5. **Follow Guidelines**: iOS Human Interface, Material Design

---

## Performance Targets

| Metric | Target | Warning | Critical |
|--------|--------|---------|----------|
| FPS | 60 | < 45 | < 30 |
| Frame Time | 16.7ms | > 22ms | > 33ms |
| Memory | < 200MB | > 220MB | > 240MB |
| CPU Temp | < 40°C | > 42°C | > 45°C |
| GPU Temp | < 40°C | > 42°C | > 45°C |
| Battery Drain | < 8%/hr | > 10%/hr | > 15%/hr |

---

## Troubleshooting

### Low FPS

1. Check frame timing breakdown
2. Reduce resolution scale
3. Lower frame rate target
4. Reduce draw calls
5. Optimize shaders

### High Memory

1. Check texture compression
2. Enable auto-unload
3. Reduce streaming batch size
4. Unload LRU resources
5. Check for leaks

### Overheating

1. Monitor thermal zones
2. Reduce GPU load
3. Lower resolution
4. Cap frame rate
5. Add thermal throttling

### Battery Drain

1. Check for wake locks
2. Reduce screen brightness
3. Optimize rendering
4. Pause when idle
5. Monitor background activity

---

For more information, see the [API Reference](API-REFERENCE.md) and [Example Projects](../../examples/).
