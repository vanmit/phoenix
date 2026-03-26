//
//  ViewController.m
//  PhoenixEngine iOS Example
//
//  Demonstrates mobile optimization features:
//  - Safe area handling for notch devices
//  - Touch input with gesture recognition
//  - Performance profiling overlay
//

#import "ViewController.h"
#import <phoenix/mobile.hpp>
#import <GLKit/GLKit.h>

@interface ViewController () <EAGLViewDelegate>

@property (nonatomic, strong) EAGLContext *context;
@property (nonatomic, strong) EAGLView *glView;
@property (nonatomic, strong) UILabel *fpsLabel;
@property (nonatomic, strong) UILabel *memoryLabel;
@property (nonatomic, strong) UILabel *thermalLabel;

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    
    // Setup OpenGL ES context
    self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
    if (!self.context) {
        NSLog(@"[PhoenixEngine] Failed to create ES3 context, trying ES2");
        self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    }
    
    // Setup GL view
    self.glView = [[EAGLView alloc] initWithFrame:self.view.bounds];
    self.glView.delegate = self;
    [self.glView setDrawableColorFormat:GLKViewDrawableColorFormatRGBA8888];
    [self.glView setDrawableDepthFormat:GLKViewDrawableDepthFormat24];
    [self.glView setContext:self.context];
    [self.view addSubview:self.glView];
    
    // Setup performance overlay
    [self setupOverlay];
    
    // Configure safe area handling
    phoenix::mobile::MobilePlatform::getInstance().onSafeAreaChanged(^(const phoenix::mobile::SafeAreaEdges& edges) {
        dispatch_async(dispatch_get_main_queue(), ^{
            [self updateSafeAreaEdges:edges];
        });
    });
    
    // Configure touch input
    phoenix::mobile::TouchInput::getInstance().onGesture(^(const phoenix::mobile::Gesture& gesture) {
        [self handleGesture:gesture];
    });
    
    // Start rendering
    [self.glView setEnableSetNeedsDisplay:NO];
    [self startRendering];
}

- (void)setupOverlay {
    // FPS Label
    self.fpsLabel = [[UILabel alloc] initWithFrame:CGRectMake(10, 50, 150, 20)];
    self.fpsLabel.textColor = [UIColor whiteColor];
    self.fpsLabel.font = [UIFont monospacedSystemFontOfSize:14 weight:UIFontWeightMedium];
    self.fpsLabel.backgroundColor = [[UIColor blackColor] colorWithAlphaComponent:0.5];
    self.fpsLabel.text = @"FPS: --";
    [self.view addSubview:self.fpsLabel];
    
    // Memory Label
    self.memoryLabel = [[UILabel alloc] initWithFrame:CGRectMake(10, 75, 150, 20)];
    self.memoryLabel.textColor = [UIColor whiteColor];
    self.memoryLabel.font = [UIFont monospacedSystemFontOfSize:14 weight:UIFontWeightMedium];
    self.memoryLabel.backgroundColor = [[UIColor blackColor] colorWithAlphaComponent:0.5];
    self.memoryLabel.text = @"Memory: -- MB";
    [self.view addSubview:self.memoryLabel];
    
    // Thermal Label
    self.thermalLabel = [[UILabel alloc] initWithFrame:CGRectMake(10, 100, 150, 20)];
    self.thermalLabel.textColor = [UIColor whiteColor];
    self.thermalLabel.font = [UIFont monospacedSystemFontOfSize:14 weight:UIFontWeightMedium];
    self.thermalLabel.backgroundColor = [[UIColor blackColor] colorWithAlphaComponent:0.5];
    self.thermalLabel.text = @"Temp: --°C";
    [self.view addSubview:self.thermalLabel];
}

- (void)updateSafeAreaEdges:(const phoenix::mobile::SafeAreaEdges&)edges {
    // Adjust content to respect safe area
    UIEdgeInsets safeArea = UIEdgeInsetsMake(
        edges.top * self.view.bounds.size.height,
        edges.left * self.view.bounds.size.width,
        edges.bottom * self.view.bounds.size.height,
        edges.right * self.view.bounds.size.width
    );
    
    NSLog(@"[PhoenixEngine] Safe area: T=%.2f L=%.2f B=%.2f R=%.2f",
          safeArea.top, safeArea.left, safeArea.bottom, safeArea.right);
}

- (void)handleGesture:(const phoenix::mobile::Gesture&)gesture {
    switch (gesture.type) {
        case phoenix::mobile::GestureType::Tap:
            NSLog(@"[PhoenixEngine] Tap detected at (%.2f, %.2f)", gesture.x, gesture.y);
            break;
        case phoenix::mobile::GestureType::DoubleTap:
            NSLog(@"[PhoenixEngine] Double tap detected");
            break;
        case phoenix::mobile::GestureType::Swipe:
            NSLog(@"[PhoenixEngine] Swipe detected");
            break;
        case phoenix::mobile::GestureType::Pinch:
            NSLog(@"[PhoenixEngine] Pinch detected, scale: %.2f", gesture.scale);
            break;
        default:
            break;
    }
}

- (void)startRendering {
    CADisplayLink *displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(render)];
    [displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
}

- (void)render {
    @autoreleasepool {
        // Begin frame profiling
        phoenix::mobile::beginFrame();
        
        // Update mobile subsystems
        phoenix::mobile::update(0.016f);  // Assume 60 FPS
        
        // Get performance metrics
        auto& profiler = phoenix::mobile::MobileProfiler::getInstance();
        float fps = profiler.getCurrentFPS();
        float memoryMB = profiler.getCurrentMemoryMB();
        float cpuTemp = profiler.getCPUTemperature();
        
        // Update overlay
        dispatch_async(dispatch_get_main_queue(), ^{
            self.fpsLabel.text = [NSString stringWithFormat:@"FPS: %.1f", fps];
            self.memoryLabel.text = [NSString stringWithFormat:@"Memory: %.1f MB", memoryMB];
            self.thermalLabel.text = [NSString stringWithFormat:@"CPU: %.1f°C", cpuTemp];
            
            // Color code based on performance
            if (fps < 30) {
                self.fpsLabel.textColor = [UIColor redColor];
            } else if (fps < 50) {
                self.fpsLabel.textColor = [UIColor yellowColor];
            } else {
                self.fpsLabel.textColor = [UIColor greenColor];
            }
        });
        
        // Render with Phoenix Engine
        [self renderScene];
        
        // End frame profiling
        phoenix::mobile::endFrame();
    }
}

- (void)renderScene {
    // Phoenix Engine rendering would happen here
    // This is a placeholder for the actual render call
    
    EAGLContext *currentContext = [EAGLContext currentContext];
    if (currentContext != self.context) {
        [EAGLContext setCurrentContext:self.context];
    }
    
    // Clear screen
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Render Phoenix Engine scene here
    // phoenix::render::renderScene();
    
    [self.glView display];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    
    // Handle memory warning
    phoenix::mobile::onMemoryWarning(2);
    
    // Release any cached data
    [[ResourceManager sharedManager] purgeUnusedResources];
}

- (void)dealloc {
    [EAGLContext setCurrentContext:nil];
}

@end
