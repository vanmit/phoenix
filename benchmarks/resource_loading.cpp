/**
 * Phoenix Engine - Resource Loading Benchmark
 * 
 * Benchmark for measuring texture and model loading performance
 */

#include <phoenix/resource/asset_loader.hpp>
#include <phoenix/core/timer.hpp>
#include <phoenix/core/logger.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>

namespace phoenix::benchmarks {

using namespace phoenix::core;
using namespace phoenix::resource;

/**
 * Resource Loading Benchmark
 * 
 * Measures:
 * - Texture loading time (PNG, JPEG, KTX2)
 * - Model loading time (glTF, FBX, OBJ)
 * - Memory usage during loading
 * - Async vs sync loading performance
 */
class ResourceLoadingBenchmark {
public:
    struct Result {
        std::string name;
        double loadTimeMs;
        size_t memoryUsed;
        bool success;
        std::string error;
    };
    
    void runTextureLoadingBenchmark(const std::string& testDirectory) {
        Logger::info("Starting texture loading benchmark", "Benchmark");
        
        TextureLoader::Config config;
        config.generateMipMaps = true;
        config.sRGB = true;
        
        TextureLoader loader(config);
        
        std::vector<std::string> testFiles = {
            "test_256.png",
            "test_512.png",
            "test_1024.png",
            "test_2048.png",
            "test_4096.png"
        };
        
        for (const auto& filename : testFiles) {
            std::string path = testDirectory + "/" + filename;
            
            if (!std::filesystem::exists(path)) {
                Logger::warning("Test file not found: " + path, "Benchmark");
                continue;
            }
            
            Timer timer;
            timer.start();
            
            auto texture = loader.load(path);
            
            timer.stop();
            
            if (texture && texture->loadState == LoadState::Loaded) {
                Result result;
                result.name = filename;
                result.loadTimeMs = timer.elapsedMilliseconds();
                result.memoryUsed = loader.estimateMemoryUsage(path);
                result.success = true;
                
                Logger::info(
                    "Loaded " + filename + 
                    " in " + std::to_string(result.loadTimeMs) + "ms",
                    "Benchmark"
                );
            } else {
                Logger::error(
                    "Failed to load " + filename + ": " + 
                    (texture ? texture->loadError : "Unknown error"),
                    "Benchmark"
                );
            }
        }
    }
    
    void runAsyncLoadingBenchmark(const std::string& testDirectory) {
        Logger::info("Starting async loading benchmark", "Benchmark");
        
        TextureLoader::Config config;
        config.generateMipMaps = true;
        
        TextureLoader loader(config);
        
        std::string path = testDirectory + "/test_1024.png";
        
        if (!std::filesystem::exists(path)) {
            Logger::warning("Test file not found: " + path, "Benchmark");
            return;
        }
        
        Timer timer;
        timer.start();
        
        // Load multiple textures asynchronously
        std::vector<std::future<std::unique_ptr<Texture>>> futures;
        
        for (int i = 0; i < 10; i++) {
            futures.push_back(loader.loadAsync(path));
        }
        
        // Wait for all to complete
        int successCount = 0;
        for (auto& future : futures) {
            auto texture = future.get();
            if (texture && texture->loadState == LoadState::Loaded) {
                successCount++;
            }
        }
        
        timer.stop();
        
        Logger::info(
            "Async loaded " + std::to_string(successCount) + "/10 textures in " +
            std::to_string(timer.elapsedMilliseconds()) + "ms",
            "Benchmark"
        );
    }
};

} // namespace phoenix::benchmarks

// Main entry point for standalone benchmark
int main(int argc, char** argv) {
    using namespace phoenix::benchmarks;
    using namespace phoenix::core;
    
    // Initialize logger
    Logger::init(LogLevel::Info, "resource_loading_benchmark.log");
    
    Logger::info("Phoenix Engine Resource Loading Benchmark", "Benchmark");
    
    ResourceLoadingBenchmark benchmark;
    
    std::string testDirectory = "./test_assets";
    
    if (argc > 1) {
        testDirectory = argv[1];
    }
    
    benchmark.runTextureLoadingBenchmark(testDirectory);
    benchmark.runAsyncLoadingBenchmark(testDirectory);
    
    Logger::shutdown();
    
    return 0;
}
