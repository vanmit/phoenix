/**
 * Phoenix Engine - Memory Benchmark
 * 
 * 测试内存占用
 * 目标：<512MB (桌面), <256MB (移动)
 */

#include <iostream>
#include <vector>
#include <fstream>
#include <iomanip>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <unistd.h>
#include <sys/resource.h>
#endif

#include "phoenix/resource/resource_manager.hpp"
#include "phoenix/render/device.hpp"
#include "phoenix/scene/scene.hpp"

using namespace phoenix;

// ============================================================================
// 内存统计
// ============================================================================

struct MemoryStats {
    size_t totalVirtual;      // 虚拟内存
    size_t totalPhysical;     // 物理内存
    size_t resourceMemory;    // 资源占用
    size_t renderMemory;      // 渲染内存
    size_t sceneMemory;       // 场景内存
    size_t peakMemory;        // 峰值内存
};

// ============================================================================
// 平台相关内存查询
// ============================================================================

class MemoryMonitor {
public:
    static MemoryStats getCurrentMemory() {
        MemoryStats stats;
        std::memset(&stats, 0, sizeof(stats));
        
#ifdef _WIN32
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
            stats.totalPhysical = pmc.WorkingSetSize;
            stats.totalVirtual = pmc.PagefileUsage;
        }
#else
        struct rusage usage;
        if (getrusage(RUSAGE_SELF, &usage) == 0) {
            stats.totalPhysical = usage.ru_maxrss * 1024; // KB to bytes
            stats.totalVirtual = 0; // Linux 需要读取 /proc/self/status
        }
#endif
        
        return stats;
    }
    
    static size_t getProcessMemoryMB() {
        auto stats = getCurrentMemory();
        return stats.totalPhysical / (1024 * 1024);
    }
    
    static void printMemoryUsage(const std::string& label) {
        auto stats = getCurrentMemory();
        std::cout << std::fixed << std::setprecision(2);
        std::cout << label << ": " 
                  << (stats.totalPhysical / (1024.0 * 1024.0)) << " MB" << std::endl;
    }
};

// ============================================================================
// 内存基准测试
// ============================================================================

class MemoryBenchmark {
public:
    MemoryBenchmark() : resourceManager_(nullptr) {}
    
    bool initialize() {
        ResourceManagerConfig config;
        config.maxMemory = 512 * 1024 * 1024;
        resourceManager_ = ResourceManager::create(config);
        return resourceManager_ != nullptr;
    }
    
    MemoryStats runBenchmark() {
        MemoryStats stats;
        stats.peakMemory = 0;
        
        std::cout << "\n=== Memory Benchmark ===" << std::endl;
        
        // 基准线
        MemoryMonitor::printMemoryUsage("Baseline");
        auto baseline = MemoryMonitor::getCurrentMemory();
        
        // 测试 1: 纹理加载
        std::cout << "\nLoading textures..." << std::endl;
        loadTextures(100);
        MemoryMonitor::printMemoryUsage("After 100 textures");
        stats.peakMemory = std::max(stats.peakMemory, MemoryMonitor::getProcessMemoryMB());
        
        // 测试 2: 模型加载
        std::cout << "Loading models..." << std::endl;
        loadModels(50);
        MemoryMonitor::printMemoryUsage("After 50 models");
        stats.peakMemory = std::max(stats.peakMemory, MemoryMonitor::getProcessMemoryMB());
        
        // 测试 3: 场景构建
        std::cout << "Building scene..." << std::endl;
        buildScene(1000);
        MemoryMonitor::printMemoryUsage("After scene build");
        stats.peakMemory = std::max(stats.peakMemory, MemoryMonitor::getProcessMemoryMB());
        
        // 测试 4: 资源缓存
        std::cout << "Testing cache..." << std::endl;
        testCache();
        MemoryMonitor::printMemoryUsage("After cache test");
        stats.peakMemory = std::max(stats.peakMemory, MemoryMonitor::getProcessMemoryMB());
        
        // 最终统计
        auto final = MemoryMonitor::getCurrentMemory();
        stats.totalPhysical = final.totalPhysical;
        stats.totalVirtual = final.totalVirtual;
        
        std::cout << "\n--- Peak Memory: " << stats.peakMemory << " MB ---" << std::endl;
        
        // 检查是否通过
        bool passed = (stats.peakMemory < 512);
        std::cout << "Status: " << (passed ? "✓ PASSED (<512MB)" : "✗ FAILED (>=512MB)") << std::endl;
        
        return stats;
    }
    
private:
    void loadTextures(int count) {
        for (int i = 0; i < count; ++i) {
            // 模拟纹理加载
            if (resourceManager_) {
                resourceManager_->loadTexture("test_texture_" + std::to_string(i) + ".png");
            }
        }
    }
    
    void loadModels(int count) {
        for (int i = 0; i < count; ++i) {
            if (resourceManager_) {
                resourceManager_->loadModel("test_model_" + std::to_string(i) + ".fbx");
            }
        }
    }
    
    void buildScene(int objectCount) {
        auto scene = std::make_unique<Scene>();
        
        for (int i = 0; i < objectCount; ++i) {
            scene->addObject(i * 1.0f, 0.0f, 0.0f);
        }
    }
    
    void testCache() {
        // 测试缓存机制
        if (resourceManager_) {
            resourceManager_->clearCache();
        }
    }
    
    std::unique_ptr<ResourceManager> resourceManager_;
};

// ============================================================================
// 主函数
// ============================================================================

int main(int argc, char** argv) {
    std::cout << "========================================" << std::endl;
    std::cout << "Phoenix Engine Memory Benchmark" << std::endl;
    std::cout << "========================================" << std::endl;
    
    MemoryBenchmark benchmark;
    
    if (!benchmark.initialize()) {
        std::cerr << "Failed to initialize benchmark!" << std::endl;
        return 1;
    }
    
    auto stats = benchmark.runBenchmark();
    
    // 保存结果
    std::ofstream outFile("memory_benchmark_results.json");
    if (outFile.is_open()) {
        outFile << "{\n";
        outFile << "  \"benchmark\": \"Memory Benchmark\",\n";
        outFile << "  \"timestamp\": \"" << __DATE__ << " " << __TIME__ << "\",\n";
        outFile << "  \"peak_memory_mb\": " << stats.peakMemory << ",\n";
        outFile << "  \"target_mb\": 512,\n";
        outFile << "  \"passed\": " << (stats.peakMemory < 512 ? "true" : "false") << "\n";
        outFile << "}\n";
        outFile.close();
        std::cout << "\nResults saved to memory_benchmark_results.json" << std::endl;
    }
    
    return (stats.peakMemory < 512) ? 0 : 1;
}
