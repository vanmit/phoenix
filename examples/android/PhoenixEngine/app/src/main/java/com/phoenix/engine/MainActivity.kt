package com.phoenix.engine

import android.opengl.GLSurfaceView
import android.os.Bundle
import android.view.WindowManager
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import phoenix.mobile.*

/**
 * Phoenix Engine Android Example
 * 
 * Demonstrates mobile optimization features:
 * - Power management with dynamic resolution
 * - Memory optimization with texture compression
 * - Touch input with gesture recognition
 * - Performance profiling overlay
 */
class MainActivity : AppCompatActivity() {

    private lateinit var glView: GLSurfaceView
    private lateinit var fpsText: TextView
    private lateinit var memoryText: TextView
    private lateinit var thermalText: TextView
    private lateinit var renderer: PhoenixRenderer

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // Keep screen on
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)

        // Initialize Phoenix Engine mobile subsystems
        initializePhoenixEngine()

        // Setup GLSurfaceView
        glView = GLSurfaceView(this)
        glView.setEGLContextClientVersion(3)  // OpenGL ES 3.0

        // Create renderer
        renderer = PhoenixRenderer(this)
        glView.setRenderer(renderer)
        glView.renderMode = GLSurfaceView.RENDERMODE_CONTINUOUSLY

        setContentView(glView)

        // Setup overlay
        setupOverlay()

        // Configure safe area handling
        MobilePlatform.getInstance().onSafeAreaChanged { edges ->
            runOnUiThread {
                updateSafeAreaEdges(edges)
            }
        }

        // Configure touch input
        TouchInput.getInstance().onGesture { gesture ->
            handleGesture(gesture)
        }
    }

    private fun initializePhoenixEngine() {
        // Initialize all mobile subsystems
        phoenix.mobile.initialize()

        // Configure power management
        val powerConfig = PowerConfig().apply {
            targetFrameTime = 16.67f  // 60 FPS
            enableDynamicResolution = true
            enableThermalThrottling = true
            thermalThreshold = 45.0f
            batteryLowThreshold = 0.20f
            batteryCriticalThreshold = 0.10f
        }
        PowerManager.getInstance().initialize(powerConfig)

        // Configure memory management
        val memoryConfig = MemoryConfig().apply {
            maxMemoryMB = 256
            textureBudgetMB = 128
            meshBudgetMB = 64
            defaultTextureCompression = TextureCompression.ASTC_4x4
            defaultMeshCompression = MeshCompression.MeshOptimizer
        }
        MemoryManager.getInstance().initialize(memoryConfig)

        // Configure touch input
        val touchConfig = TouchConfig().apply {
            enableGestures = true
            enableVirtualController = true
            enableStylusPressure = true
        }
        TouchInput.getInstance().initialize(touchConfig)

        // Configure platform
        val platformConfig = PlatformConfig().apply {
            enableSafeAreaHandling = true
            enableNotchAdaptation = true
            enableOrientationHandling = true
            enableStatusBarIntegration = true
        }
        MobilePlatform.getInstance().initialize(platformConfig)

        // Configure profiler
        val profilerConfig = ProfilerConfig().apply {
            enableFPSCounter = true
            enableMemoryMonitor = true
            enableBatteryMonitor = true
            enableThermalMonitor = true
            showOverlay = true
        }
        MobileProfiler.getInstance().initialize(profilerConfig)
    }

    private fun setupOverlay() {
        // In a real implementation, this would add overlay views
        // For now, we'll use a simple approach
    }

    private fun updateSafeAreaEdges(edges: SafeAreaEdges) {
        // Adjust UI to respect safe area
        val displayMetrics = resources.displayMetrics
        val safeTop = edges.top * displayMetrics.heightPixels
        val safeBottom = edges.bottom * displayMetrics.heightPixels
        val safeLeft = edges.left * displayMetrics.widthPixels
        val safeRight = edges.right * displayMetrics.widthPixels

        println("[PhoenixEngine] Safe area: T=$safeTop L=$safeLeft B=$safeBottom R=$safeRight")
    }

    private fun handleGesture(gesture: Gesture) {
        when (gesture.type) {
            GestureType.Tap -> println("[PhoenixEngine] Tap at (${gesture.x}, ${gesture.y})")
            GestureType.DoubleTap -> println("[PhoenixEngine] Double tap")
            GestureType.Swipe -> println("[PhoenixEngine] Swipe ${gesture.swipeDirection}")
            GestureType.Pinch -> println("[PhoenixEngine] Pinch scale: ${gesture.scale}")
            GestureType.Rotate -> println("[PhoenixEngine] Rotate: ${Math.toDegrees(gesture.rotation.toDouble())}°")
            else -> {}
        }
    }

    override fun onResume() {
        super.onResume()
        glView.onResume()
        phoenix.mobile.onResume()
    }

    override fun onPause() {
        super.onPause()
        glView.onPause()
        phoenix.mobile.onPause()
    }

    override fun onLowMemory() {
        super.onLowMemory()
        // Handle memory warning
        MemoryManager.getInstance().handleMemoryWarning(3)
    }

    override fun onDestroy() {
        super.onDestroy()
        phoenix.mobile.shutdown()
    }
}
