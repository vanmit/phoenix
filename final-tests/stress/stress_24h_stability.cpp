/**
 * Phoenix Engine - 24 Hour Stability Test
 * 
 * 测试长时间运行稳定性
 * 目标：24 小时无崩溃，内存稳定
 */

#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <iomanip>
#include <ctime>

#include "phoenix/render/device.hpp"
#include "phoenix/render/scene.hpp"
#include "phoenix/core/timer.hpp"
#include "phoenix/core/logger.hpp"

using namespace phoenix;
using namespace std::chrono;

// ============================================================================
// 稳定性测试统计
// ============================================================================

struct StabilityStats {
    int64_t totalFrames;
    int64_t totalHours;
    double averageFPS;
    size_t initialMemory;
    size_t peakMemory;
    size_t finalMemory;
    size_t memoryLeak;
    int errorCount;
    bool survived24h;
    std::vector<double> hourlyFPS;
};

// ============================================================================
// 24 小时稳定性测试
// ============================================================================

class StabilityTest24H {
public:
    StabilityTest24H() : device_(nullptr), running_(true) {}
    
    bool initialize(const DeviceConfig& config) {
        device_ = Device::create(config);
        return device_ != nullptr;
    }
    
    StabilityStats runTest(int hours = 24) {
        StabilityStats stats;
        stats.totalFrames = 0;
        stats.totalHours = 0;
        stats.errorCount = 0;
        stats.survived24h = false;
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "24 Hour Stability Test" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Target Duration: " << hours << " hours" << std::endl;
        std::cout << "Start Time: " << getCurrentTime() << std::endl;
        
        // 初始内存
        stats.initialMemory = getProcessMemoryMB();
        stats.peakMemory = stats.initialMemory;
        std::cout << "Initial Memory: " << stats.initialMemory << " MB" << std::endl;
        
        // 创建测试场景
        auto scene = createTestScene();
        
        auto startTime = system_clock::now();
        auto endTime = startTime + hours(hours);
        
        std::vector<double> frameTimes;
        auto hourStart = startTime;
        int hourFrames = 0;
        
        // 主循环
        while (running_ && system_clock::now() < endTime) {
            auto frameStart = high_resolution_clock::now();
            
            try {
                // 更新场景
                updateScene(scene.get());
                
                // 渲染
                renderFrame(scene.get());
                
                stats.totalFrames++;
                hourFrames++;
            } catch (const std::exception& e) {
                stats.errorCount++;
                logError(e.what());
            }
            
            auto frameEnd = high_resolution_clock::now();
            double frameTime = duration<double, std::milli>(frameEnd - frameStart).count();
            frameTimes.push_back(frameTime);
            
            // 每小时报告
            auto now = system_clock::now();
            auto elapsed = duration_cast<hours>(now - hourStart).count();
            
            if (elapsed >= 1) {
                // 计算这一小时的 FPS
                double hourFPS = hourFrames / static_cast<double>(elapsed * 3600);
                stats.hourlyFPS.push_back(hourFPS);
                
                // 内存检查
                size_t currentMemory = getProcessMemoryMB();
                stats.peakMemory = std::max(stats.peakMemory, currentMemory);
                
                // 报告
                reportHourlyStats(elapsed, hourFPS, currentMemory);
                
                // 重置小时计数
                hourStart = now;
                hourFrames = 0;
                frameTimes.clear();
            }
            
            stats.totalHours = duration_cast<hours>(now - startTime).count();
            
            // 限制帧率以避免过热
            if (frameTime < 16.0) {
                std::this_thread::sleep_for(microseconds(100));
            }
        }
        
        // 最终统计
        stats.finalMemory = getProcessMemoryMB();
        stats.memoryLeak = stats.finalMemory - stats.initialMemory;
        stats.averageFPS = calculateAverageFPS(frameTimes);
        stats.survived24h = (stats.totalHours >= hours && stats.errorCount == 0);
        
        // 输出最终报告
        printFinalReport(stats);
        
        return stats;
    }
    
    void stop() {
        running_ = false;
    }
    
private:
    std::unique_ptr<Scene> createTestScene() {
        auto scene = std::make_unique<Scene>();
        
        // 添加测试物体
        for (int i = 0; i < 1000; ++i) {
            scene->addObject(i * 1.0f, 0.0f, 0.0f);
        }
        
        // 添加光源
        scene->addDirectionalLight({1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, 1.0f);
        
        return scene;
    }
    
    void updateScene(Scene* scene) {
        if (!scene) return;
        
        static float rotation = 0.0f;
        rotation += 0.01f;
        scene->setRotation(0.0f, rotation, 0.0f);
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
    
    double calculateAverageFPS(const std::vector<double>& frameTimes) {
        if (frameTimes.empty()) return 0.0;
        
        double sum = 0.0;
        for (double t : frameTimes) {
            sum += t;
        }
        
        double avgFrameTime = sum / frameTimes.size();
        return 1000.0 / avgFrameTime;
    }
    
    std::string getCurrentTime() {
        auto now = system_clock::now();
        auto time = system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
    
    void reportHourlyStats(int64_t hours, double fps, size_t memory) {
        std::cout << "\n[Hour " << hours << "] "
                  << "FPS: " << std::fixed << std::setprecision(1) << fps
                  << ", Memory: " << memory << " MB" << std::endl;
    }
    
    void printFinalReport(const StabilityStats& stats) {
        std::cout << "\n========================================" << std::endl;
        std::cout << "FINAL REPORT" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Total Runtime: " << stats.totalHours << " hours" << std::endl;
        std::cout << "Total Frames: " << stats.totalFrames << std::endl;
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Average FPS: " << stats.averageFPS << std::endl;
        std::cout << "Initial Memory: " << stats.initialMemory << " MB" << std::endl;
        std::cout << "Peak Memory: " << stats.peakMemory << " MB" << std::endl;
        std::cout << "Final Memory: " << stats.finalMemory << " MB" << std::endl;
        std::cout << "Memory Leak: " << stats.memoryLeak << " MB" << std::endl;
        std::cout << "Errors: " << stats.errorCount << std::endl;
        std::cout << "Status: " << (stats.survived24h ? "✓ PASSED" : "✗ FAILED") << std::endl;
        std::cout << "========================================" << std::endl;
    }
    
    void logError(const std::string& message) {
        std::cerr << "[ERROR] " << getCurrentTime() << ": " << message << std::endl;
    }
    
    std::unique_ptr<Device> device_;
    bool running_;
};

// ============================================================================
// 主函数
// ============================================================================

int main(int argc, char** argv) {
    std::cout << "========================================" << std::endl;
    std::cout << "Phoenix Engine 24H Stability Test" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 解析参数
    int testHours = 24;
    if (argc > 1) {
        testHours = std::atoi(argv[1]);
    }
    
    StabilityTest24H test;
    
    // 初始化
    DeviceConfig deviceConfig;
    deviceConfig.width = 1920;
    deviceConfig.height = 1080;
    deviceConfig.vsync = false;
    
    if (!test.initialize(deviceConfig)) {
        std::cerr << "Failed to initialize!" << std::endl;
        return 1;
    }
    
    // 运行测试
    auto stats = test.runTest(testHours);
    
    // 保存结果
    std::ofstream outFile("stability_24h_results.json");
    if (outFile.is_open()) {
        outFile << "{\n";
        outFile << "  \"test\": \"24H Stability Test\",\n";
        outFile << "  \"timestamp\": \"" << __DATE__ << " " << __TIME__ << "\",\n";
        outFile << "  \"target_hours\": " << testHours << ",\n";
        outFile << "  \"actual_hours\": " << stats.totalHours << ",\n";
        outFile << "  \"total_frames\": " << stats.totalFrames << ",\n";
        outFile << "  \"avg_fps\": " << stats.averageFPS << ",\n";
        outFile << "  \"initial_memory_mb\": " << stats.initialMemory << ",\n";
        outFile << "  \"peak_memory_mb\": " << stats.peakMemory << ",\n";
        outFile << "  \"final_memory_mb\": " << stats.finalMemory << ",\n";
        outFile << "  \"memory_leak_mb\": " << stats.memoryLeak << ",\n";
        outFile << "  \"error_count\": " << stats.errorCount << ",\n";
        outFile << "  \"survived_24h\": " << (stats.survived24h ? "true" : "false") << "\n";
        outFile << "}\n";
        outFile.close();
        std::cout << "\nResults saved to stability_24h_results.json" << std::endl;
    }
    
    return stats.survived24h ? 0 : 1;
}
