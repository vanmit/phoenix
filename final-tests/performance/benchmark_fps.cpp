/**
 * Phoenix Engine - FPS Benchmark
 * 
 * 测试不同分辨率和场景复杂度下的帧率表现
 * 目标：1080p 60fps, 4K 30fps
 */

#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <fstream>
#include <iomanip>
#include <numeric>

#include "phoenix/render/device.hpp"
#include "phoenix/render/scene.hpp"
#include "phoenix/core/timer.hpp"

using namespace phoenix;
using namespace std::chrono;

// ============================================================================
// 基准测试配置
// ============================================================================

struct FPSBenchmarkConfig {
    std::string name;
    int width;
    int height;
    int objectCount;
    int durationSeconds;
    bool vsync;
};

struct FPSResult {
    std::string name;
    int width;
    int height;
    double averageFPS;
    double minFPS;
    double maxFPS;
    double frameTimeAvg;
    double frameTimeMin;
    double frameTimeMax;
    int totalFrames;
    bool passed;
};

// ============================================================================
// FPS 基准测试类
// ============================================================================

class FPSBenchmark {
public:
    FPSBenchmark() : device_(nullptr), timer_() {}
    
    bool initialize(const DeviceConfig& config) {
        device_ = Device::create(config);
        return device_ != nullptr;
    }
    
    FPSResult runTest(const FPSBenchmarkConfig& config) {
        FPSResult result;
        result.name = config.name;
        result.width = config.width;
        result.height = config.height;
        
        std::cout << "\n=== Running FPS Test: " << config.name << " ===" << std::endl;
        std::cout << "Resolution: " << config.width << "x" << config.height << std::endl;
        std::cout << "Objects: " << config.objectCount << std::endl;
        std::cout << "Duration: " << config.durationSeconds << "s" << std::endl;
        
        // 调整窗口大小
        if (device_) {
            device_->resize(config.width, config.height);
        }
        
        // 创建测试场景
        auto scene = createTestScene(config.objectCount);
        
        // 预热（warm-up）
        std::cout << "Warming up..." << std::endl;
        for (int i = 0; i < 60; ++i) {
            renderFrame(scene.get());
        }
        
        // 开始测试
        std::cout << "Starting benchmark..." << std::endl;
        
        std::vector<double> frameTimes;
        frameTimes.reserve(config.durationSeconds * 60);
        
        auto startTime = high_resolution_clock::now();
        auto endTime = startTime + seconds(config.durationSeconds);
        
        int frameCount = 0;
        
        while (high_resolution_clock::now() < endTime) {
            auto frameStart = high_resolution_clock::now();
            
            renderFrame(scene.get());
            
            auto frameEnd = high_resolution_clock::now();
            double frameTime = duration<double, std::milli>(frameEnd - frameStart).count();
            
            frameTimes.push_back(frameTime);
            frameCount++;
            
            // 限制最大帧率以避免过热
            if (frameTime < 1.0) {
                std::this_thread::sleep_for(microseconds(100));
            }
        }
        
        // 计算统计
        if (!frameTimes.empty()) {
            double sum = std::accumulate(frameTimes.begin(), frameTimes.end(), 0.0);
            result.frameTimeAvg = sum / frameTimes.size();
            
            auto minmax = std::minmax_element(frameTimes.begin(), frameTimes.end());
            result.frameTimeMin = *minmax.first;
            result.frameTimeMax = *minmax.second;
            
            result.averageFPS = 1000.0 / result.frameTimeAvg;
            result.minFPS = 1000.0 / result.frameTimeMax;
            result.maxFPS = 1000.0 / result.frameTimeMin;
            result.totalFrames = frameCount;
            
            // 检查是否通过
            double targetFPS = (config.height >= 2160) ? 30.0 : 60.0;
            result.passed = (result.minFPS >= targetFPS * 0.9);
        }
        
        // 输出结果
        printResult(result);
        
        return result;
    }
    
private:
    std::unique_ptr<Scene> createTestScene(int objectCount) {
        auto scene = std::make_unique<Scene>();
        
        // 添加测试物体
        for (int i = 0; i < objectCount; ++i) {
            // 创建简单几何体
            float x = (i % 10) * 2.0f;
            float y = (i / 10) * 2.0f;
            float z = (i / 100) * 2.0f;
            
            // 添加物体到场景
            scene->addObject(x, y, z);
        }
        
        // 添加光源
        scene->addDirectionalLight({1.0f, -1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, 1.0f);
        
        return scene;
    }
    
    void renderFrame(Scene* scene) {
        if (!device_ || !scene) return;
        
        // 开始帧
        device_->beginFrame();
        
        // 渲染场景
        device_->render(scene);
        
        // 结束帧
        device_->endFrame();
    }
    
    void printResult(const FPSResult& result) {
        std::cout << "\n--- Results ---" << std::endl;
        std::cout << "Test: " << result.name << std::endl;
        std::cout << "Resolution: " << result.width << "x" << result.height << std::endl;
        std::cout << "Total Frames: " << result.totalFrames << std::endl;
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "FPS - Avg: " << result.averageFPS 
                  << ", Min: " << result.minFPS 
                  << ", Max: " << result.maxFPS << std::endl;
        std::cout << "Frame Time - Avg: " << result.frameTimeAvg << "ms"
                  << ", Min: " << result.frameTimeMin << "ms"
                  << ", Max: " << result.frameTimeMax << "ms" << std::endl;
        std::cout << "Status: " << (result.passed ? "✓ PASSED" : "✗ FAILED") << std::endl;
    }
    
    std::unique_ptr<Device> device_;
    Timer timer_;
};

// ============================================================================
// 主测试函数
// ============================================================================

int main(int argc, char** argv) {
    std::cout << "========================================" << std::endl;
    std::cout << "Phoenix Engine FPS Benchmark" << std::endl;
    std::cout << "========================================" << std::endl;
    
    FPSBenchmark benchmark;
    
    // 初始化设备
    DeviceConfig deviceConfig;
    deviceConfig.width = 1920;
    deviceConfig.height = 1080;
    deviceConfig.vsync = false;
    deviceConfig.fullscreen = false;
    
    if (!benchmark.initialize(deviceConfig)) {
        std::cerr << "Failed to initialize device!" << std::endl;
        return 1;
    }
    
    // 定义测试配置
    std::vector<FPSBenchmarkConfig> tests = {
        {"1080p_Simple", 1920, 1080, 100, 10, false},
        {"1080p_Medium", 1920, 1080, 1000, 10, false},
        {"1080p_Complex", 1920, 1080, 10000, 10, false},
        {"1440p_Simple", 2560, 1440, 100, 10, false},
        {"1440p_Medium", 2560, 1440, 1000, 10, false},
        {"1440p_Complex", 2560, 1440, 10000, 10, false},
        {"4K_Simple", 3840, 2160, 100, 10, false},
        {"4K_Medium", 3840, 2160, 1000, 10, false},
        {"4K_Complex", 3840, 2160, 10000, 10, false},
    };
    
    // 运行所有测试
    std::vector<FPSResult> results;
    
    for (const auto& config : tests) {
        auto result = benchmark.runTest(config);
        results.push_back(result);
    }
    
    // 输出汇总
    std::cout << "\n========================================" << std::endl;
    std::cout << "SUMMARY" << std::endl;
    std::cout << "========================================" << std::endl;
    
    int passed = 0;
    int failed = 0;
    
    for (const auto& result : results) {
        std::cout << std::left << std::setw(20) << result.name << " | "
                  << std::right << std::setw(8) << std::fixed << std::setprecision(1) 
                  << result.averageFPS << " FPS | "
                  << (result.passed ? "✓ PASS" : "✗ FAIL") << std::endl;
        
        if (result.passed) passed++;
        else failed++;
    }
    
    std::cout << "\nTotal: " << passed << " passed, " << failed << " failed" << std::endl;
    
    // 保存结果到文件
    std::ofstream outFile("fps_benchmark_results.json");
    if (outFile.is_open()) {
        outFile << "{\n";
        outFile << "  \"benchmark\": \"FPS Benchmark\",\n";
        outFile << "  \"timestamp\": \"" << __DATE__ << " " << __TIME__ << "\",\n";
        outFile << "  \"results\": [\n";
        
        for (size_t i = 0; i < results.size(); ++i) {
            const auto& r = results[i];
            outFile << "    {\n";
            outFile << "      \"name\": \"" << r.name << "\",\n";
            outFile << "      \"width\": " << r.width << ",\n";
            outFile << "      \"height\": " << r.height << ",\n";
            outFile << "      \"avg_fps\": " << r.averageFPS << ",\n";
            outFile << "      \"min_fps\": " << r.minFPS << ",\n";
            outFile << "      \"max_fps\": " << r.maxFPS << ",\n";
            outFile << "      \"passed\": " << (r.passed ? "true" : "false") << "\n";
            outFile << "    }" << (i < results.size() - 1 ? "," : "") << "\n";
        }
        
        outFile << "  ]\n";
        outFile << "}\n";
        outFile.close();
        std::cout << "\nResults saved to fps_benchmark_results.json" << std::endl;
    }
    
    return (failed == 0) ? 0 : 1;
}
