//
//  AppDelegate.m
//  PhoenixEngine iOS Example
//

#import "AppDelegate.h"
#import "ViewController.h"
#import <phoenix/mobile.hpp>

@implementation AppDelegate

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    
    // Initialize Phoenix Engine mobile subsystems
    phoenix::mobile::initialize();
    
    // Configure power management
    phoenix::mobile::PowerConfig powerConfig;
    powerConfig.targetFrameTime = 16.67f;  // 60 FPS
    powerConfig.enableDynamicResolution = true;
    powerConfig.enableThermalThrottling = true;
    powerConfig.thermalThreshold = 45.0f;
    phoenix::mobile::PowerManager::getInstance().initialize(powerConfig);
    
    // Configure memory management
    phoenix::mobile::MemoryConfig memoryConfig;
    memoryConfig.maxMemoryMB = 256;
    memoryConfig.textureBudgetMB = 128;
    memoryConfig.meshBudgetMB = 64;
    memoryConfig.defaultTextureCompression = phoenix::mobile::TextureCompression::ASTC_4x4;
    phoenix::mobile::MemoryManager::getInstance().initialize(memoryConfig);
    
    // Configure profiler
    phoenix::mobile::ProfilerConfig profilerConfig;
    profilerConfig.enableFPSCounter = true;
    profilerConfig.enableMemoryMonitor = true;
    profilerConfig.enableBatteryMonitor = true;
    profilerConfig.enableThermalMonitor = true;
    profilerConfig.showOverlay = true;
    phoenix::mobile::MobileProfiler::getInstance().initialize(profilerConfig);
    
    // Setup window and root view controller
    self.window = [[UIWindow alloc] initWithFrame:[UIScreen mainScreen].bounds];
    ViewController *viewController = [[ViewController alloc] initWithNibName:nil bundle:nil];
    self.window.rootViewController = viewController;
    [self.window makeKeyAndVisible];
    
    NSLog(@"[PhoenixEngine] iOS app launched with mobile optimizations enabled");
    
    return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application {
    // App about to enter background
    phoenix::mobile::onPause();
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
    // App is in background
    phoenix::mobile::onPause();
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
    // App returning to foreground
    phoenix::mobile::onResume();
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
    // App is active
    phoenix::mobile::onResume();
}

- (void)applicationWillTerminate:(UIApplication *)application {
    // Cleanup
    phoenix::mobile::shutdown();
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application {
    // Handle memory warning
    phoenix::mobile::onMemoryWarning(2);  // Level 2 warning
}

@end
