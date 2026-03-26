/**
 * Phoenix Engine - Stress Test: 10,000 Objects
 * 
 * 测试万级物体渲染性能
 * 目标：稳定 60fps (1080p)
 */

#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <fstream>
#include <iomanip>

#include "phoenix/render/device.hpp"
#include "phoenix/render/scene.hpp"
#include "phoenix/core/timer.hpp"

using namespace phoenix;
using namespace std::chrono;

// ============================================================================
// 压力测试配置
// ============================================================================

struct StressTestConfig {
    int objectCount;
    int durationSeconds;
    int width;
    int height;
    bool enableInstancing;
    bool enableLOD;
    bool enableFrustumCulling;
};

struct StressTestResult {
    int objectCount;
    double averageFPS;
    double minFPS;
    double maxFPS;
    double averageFrameTime;
    size_t peakMemoryMB;
    bool passed;
    std::string notes;
};

// ============================================================================
// 万级物体压力测试
// ============================================================================

class StressTest10KObjects {
public:
    StressTest10KObjects() : device_(nullptr), timer_() {}
    
    bool initialize(const DeviceConfig& config) {
        device_ = Device::create(config);
        return device_ != nullptr;
    }
    
    StressTestResult runTest(const StressTestConfig& config) {
        StressTestResult result;
        result.objectCount = config.objectCount;
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "Stress Test: " << config.objectCount << " Objects" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Duration: " << config.durationSeconds << "s" << std::endl;
        std::cout << "Instancing: " << (config.enableInstancing ? "ON" : "OFF") << std::endl;
        std::cout << "LOD: " << (config.enableLOD ? "ON" : "OFF") << std::endl;
        std::cout << "Frustum Culling: " << (config.enableFrustumCulling ? "ON" : "OFF") << std::endl;
        
        // 调整分辨率
        if (device_) {
            device_->resize(config.width, config.height);
        }
        
        // 创建场景
        std::cout << "\nCreating scene with " << config.objectCount << " objects..." << std::endl;
        auto scene = createStressScene(config.objectCount);
        std::cout << "Scene created." << std::endl;
        
        // 预热
        std::cout << "Warming up..." << std::endl;
        for (int i = 0; i < 60; ++i) {
            renderFrame(scene.get());
        }
        
        // 开始测试
        std::cout << "Starting stress test..." << std::endl;
        
        std::vector<double> frameTimes;
        frameTimes.reserve(config.durationSeconds * 60);
        
        auto startTime = high_resolution_clock::now();
        auto endTime = startTime + seconds(config.durationSeconds);
        
        int frameCount = 0;
        
        while (high_resolution_clock::now() < endTime) {
            auto frameStart = high_resolution_clock::now();
            
            // 更新场景（旋转物体）
            updateScene(scene.get(), frameCount);
            
            renderFrame(scene.get());
            
            auto frameEnd = high_resolution_clock::now();
            double frameTime = duration<double, std::milli>(frameEnd - frameStart).count();
            
            frameTimes.push_back(frameTime);
            frameCount++;
        }
        
        // 计算统计
        if (!frameTimes.empty()) {
            double sum = 0.0;
            double min = frameTimes[0];
            double max = frameTimes[0];
            
            for (double t : frameTimes) {
                sum += t;
                if (t < min) min = t;
                if (t > max) t;
            }
            
            result.averageFrameTime = sum / frameTimes.size();
            result.averageFPS = 1000.0 / result.averageFrameTime;
            result.minFPS = 1000.0 / max;
            result.maxFPS = 1000.0 / min;
            
            // 内存统计
            result.peakMemoryMB = getProcessMemoryMB();
            
            // 检查是否通过
            result.passed = (result.minFPS >= 55.0); // 目标 60fps，允许 5% 波动
            
            if (result.passed) {
                result.notes = "Stable performance";
            } else {
                result.notes = "Performance dropped below target";
            }
        }
        
        // 输出结果
        printResult(result);
        
        return result;
    }
    
private:
    std::unique_ptr<Scene> createStressScene(int objectCount) {
        auto scene = std::make_unique<Scene>();
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> posDist(-50.0f, 50.0f);
        std::uniform_real_distribution<float> scaleDist(0.5f, 2.0f);
        
        for (int i = 0; i < objectCount; ++i) {
            float x = posDist(gen);
            float y = posDist(gen);
            float z = posDist(gen);
            float scale = scaleDist(gen);
            
            // 添加物体
            scene->addObject(x, y, z, scale);
        }
        
        // 添加光源
        scene->addDirectionalLight({0.5f, -1.0f, 0.5f}, {1.0f, 1.0f, 1.0f}, 1.0f);
        scene->addPointLight({0.0f, 10.0f, 0.0f}, {1.0f, 0.8f, 0.6f}, 100.0f);
        
        return scene;
    }
    
    void updateScene(Scene* scene, int frame) {
        if (!scene) return;
        
        // 旋转场景
        float rotation = frame * 0.01f;
        scene->setRotation(rotation, 0.0f, rotation * 0.5f);
    }
    
    void renderFrame(Scene* scene) {
        if (!device_ || !scene) return;
        
        device_->beginFrame();
        device_->render(scene);
        device_->endFrame();
    }
    
    size_t getProcessMemoryMB() {
#ifdef _WIN32
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
            return pmc.WorkingSetSize / (1024 * 1024);
        }
#else
        struct rusage usage;
        if (getrusage(RUSAGE_SELF, &usage) == 0) {
            return usage.ru_maxrss / 1024;
        }
#endif
        return 0;
    }
    
    void printResult(const StressTestResult& result) {
        std::cout << "\n--- Results ---" << std::endl;
        std::cout << "Objects: " << result.objectCount << std::endl;
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "FPS - Avg: " << result.averageFPS 
                  << ", Min: " << result.minFPS 
                  << ", Max: " << result.maxFPS << std::endl;
        std::cout << "Frame Time: " << result.averageFrameTime << " ms" << std::endl;
        std::cout << "Memory: " << result.peakMemoryMB << " MB" << std::endl;
        std::cout << "Status: " << (result.passed ? "✓ PASSED" : "✗ FAILED") << std::endl;
        std::cout << "Notes: " << result.notes << std::endl;
    }
    
    std::unique_ptr<Device> device_;
    Timer timer_;
};

// ============================================================================
// 主函数
// ============================================================================

int main(int argc, char** argv) {
    std::cout << "========================================" << std::endl;
    std::cout << "Phoenix Engine Stress Test" << std::endl;
    std::cout << "10,000 Objects Rendering" << std::endl;
    std::cout << "========================================" << std::endl;
    
    StressTest10KObjects test;
    
    // 初始化设备
    DeviceConfig deviceConfig;
    deviceConfig.width = 1920;
    deviceConfig.height = 1080;
    deviceConfig.vsync = false;
    
    if (!test.initialize(deviceConfig)) {
        std::cerr << "Failed to initialize device!" << std::endl;
        return 1;
    }
    
    // 测试配置
    StressTestConfig config;
    config.objectCount = 10000;
    config.durationSeconds = 30;
    config.width = 1920;
    config.height = 1080;
    config.enableInstancing = true;
    config.enableLOD = true;
    config.enableFrustumCulling = true;
    
    auto result = test.runTest(config);
    
    // 保存结果
    std::ofstream outFile("stress_10k_objects_results.json");
    if (outFile.is_open()) {
        outFile << "{\n";
        outFile << "  \"test\": \"10K Objects Stress Test\",\n";
        outFile << "  \"timestamp\": \"" << __DATE__ << " " << __TIME__ << "\",\n";
        outFile << "  \"object_count\": " << result.objectCount << ",\n";
        outFile << "  \"avg_fps\": " << result.averageFPS << ",\n";
        outFile << "  \"min_fps\": " << result.minFPS << ",\n";
        outFile << "  \"max_fps\": " << result.maxFPS << ",\n";
        outFile << "  \"avg_frame_time_ms\": " << result.averageFrameTime << ",\n";
        outFile << "  \"peak_memory_mb\": " << result.peakMemoryMB << ",\n";
        outFile << "  \"passed\": " << (result.passed ? "true" : "false") << ",\n";
        outFile << "  \"notes\": \"" << result.notes << "\"\n";
        outFile << "}\n";
        outFile.close();
        std::cout << "\nResults saved to stress_10k_objects_results.json" << std::endl;
    }
    
    return result.passed ? 0 : 1;
}
