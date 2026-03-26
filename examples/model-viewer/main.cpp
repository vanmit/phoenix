/**
 * Phoenix Engine - Model Viewer Example
 * 
 * Demonstrates loading and rendering various 3D model formats,
 * textures, point clouds, and terrain.
 * 
 * Usage: model-viewer [options] <file>
 * 
 * Supported formats:
 *   Models: .gltf, .glb, .fbx, .obj, .stl
 *   Textures: .png, .jpg, .ktx2, .dds
 *   Point Clouds: .las, .laz, .pcd
 *   Terrain: .tif, .tiff, .raw
 */

#include <phoenix/resource/resource_manager.hpp>
#include <phoenix/resource/asset_loader.hpp>
#include <phoenix/resource/mesh.hpp>
#include <phoenix/resource/texture.hpp>
#include <phoenix/resource/point_cloud.hpp>
#include <phoenix/resource/terrain.hpp>

#include <iostream>
#include <string>
#include <chrono>
#include <filesystem>
#include <csignal>

using namespace phoenix::resource;

// Global flag for shutdown
volatile bool g_running = true;

void signalHandler(int signal) {
    g_running = false;
}

// Configuration
struct ViewerConfig {
    std::string filePath;
    bool verbose = false;
    bool wireframe = false;
    bool showBounds = true;
    bool showStats = true;
    float cameraDistance = 10.0f;
    float rotationSpeed = 0.5f;
    uint32_t samples = 4;  // MSAA
    bool vsync = true;
    std::string backgroundColor = "#1a1a2e";
};

// Application state
struct AppState {
    ResourceManager* resourceManager = nullptr;
    
    // Loaded resources
    MeshHandle currentMesh;
    TextureHandle currentTexture;
    PointCloudHandle currentPointCloud;
    TerrainHandle currentTerrain;
    
    // Camera
    math::Vector3 cameraPosition{0.0f, 5.0f, 10.0f};
    math::Vector3 cameraTarget{0.0f, 0.0f, 0.0f};
    math::Quaternion cameraRotation;
    
    // Timing
    float deltaTime = 0.0f;
    float totalTime = 0.0f;
    uint32_t frameCount = 0;
    double lastFrameTime = 0.0;
    double fps = 0.0;
    
    // Stats
    size_t vertexCount = 0;
    size_t triangleCount = 0;
    size_t textureMemory = 0;
};

void printUsage(const char* programName) {
    std::cout << "Phoenix Engine Model Viewer\n";
    std::cout << "===========================\n\n";
    std::cout << "Usage: " << programName << " [options] <file>\n\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help          Show this help message\n";
    std::cout << "  -v, --verbose       Enable verbose output\n";
    std::cout << "  -w, --wireframe     Render in wireframe mode\n";
    std::cout << "  -b, --no-bounds     Hide bounding volumes\n";
    std::cout << "  -s, --no-stats      Hide statistics overlay\n";
    std::cout << "  -d, --distance N    Set camera distance (default: 10)\n";
    std::cout << "  --samples N         Set MSAA samples (default: 4)\n";
    std::cout << "  --no-vsync          Disable VSync\n\n";
    std::cout << "Supported formats:\n";
    std::cout << "  Models: .gltf, .glb, .fbx, .obj, .stl\n";
    std::cout << "  Textures: .png, .jpg, .ktx2, .dds\n";
    std::cout << "  Point Clouds: .las, .laz, .pcd\n";
    std::cout << "  Terrain: .tif, .tiff, .raw\n\n";
    std::cout << "Controls:\n";
    std::cout << "  Left Mouse + Drag   Rotate camera\n";
    std::cout << "  Right Mouse + Drag  Pan camera\n";
    std::cout << "  Scroll Wheel        Zoom in/out\n";
    std::cout << "  F                   Frame on model\n";
    std::cout << "  R                   Reset camera\n";
    std::cout << "  Escape              Exit\n";
}

ViewerConfig parseArguments(int argc, char** argv) {
    ViewerConfig config;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            exit(0);
        }
        else if (arg == "-v" || arg == "--verbose") {
            config.verbose = true;
        }
        else if (arg == "-w" || arg == "--wireframe") {
            config.wireframe = true;
        }
        else if (arg == "-b" || arg == "--no-bounds") {
            config.showBounds = false;
        }
        else if (arg == "-s" || arg == "--no-stats") {
            config.showStats = false;
        }
        else if (arg == "-d" || arg == "--distance") {
            if (i + 1 < argc) {
                config.cameraDistance = std::stof(argv[++i]);
            }
        }
        else if (arg == "--samples") {
            if (i + 1 < argc) {
                config.samples = std::stoul(argv[++i]);
            }
        }
        else if (arg == "--no-vsync") {
            config.vsync = false;
        }
        else if (arg[0] != '-') {
            config.filePath = arg;
        }
    }
    
    return config;
}

bool loadResource(AppState& state, const std::string& path, const ViewerConfig& config) {
    if (!std::filesystem::exists(path)) {
        std::cerr << "Error: File not found: " << path << "\n";
        return false;
    }
    
    std::string ext = path.size() >= 4 ? path.substr(path.size() - 4) : "";
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Determine type and load
    if (ext == ".gltf" || ext == ".glb" || ext == ".fbx" || ext == ".obj" || ext == ".stl") {
        if (config.verbose) {
            std::cout << "Loading model: " << path << "\n";
        }
        
        state.currentMesh = state.resourceManager->loadMesh(path);
        
        if (!state.currentMesh.isValid()) {
            std::cerr << "Error: Failed to load model\n";
            return false;
        }
        
        auto* mesh = state.resourceManager->getMesh(state.currentMesh);
        if (mesh) {
            state.vertexCount = 0;
            state.triangleCount = 0;
            
            for (const auto& prim : mesh->primitives) {
                state.vertexCount += prim.vertexCount;
                state.triangleCount += prim.indexCount / 3;
            }
            
            if (config.verbose) {
                std::cout << "  Vertices: " << state.vertexCount << "\n";
                std::cout << "  Triangles: " << state.triangleCount << "\n";
                std::cout << "  Primitives: " << mesh->primitives.size() << "\n";
                std::cout << "  Materials: " << mesh->materials.size() << "\n";
                std::cout << "  Animations: " << mesh->animations.size() << "\n";
                std::cout << "  Memory: " << (mesh->calculateMemoryUsage() / 1024) << " KB\n";
            }
            
            // Position camera
            if (mesh->bounds.isValid()) {
                state.cameraTarget = mesh->bounds.center;
                state.cameraPosition = state.cameraTarget + 
                                      math::Vector3(0.0f, mesh->bounds.radius * 0.5f, 
                                                   mesh->bounds.radius * state.cameraDistance);
            }
        }
    }
    else if (ext == ".png" || ext == ".jpg" || ext == "tx2" || ext == ".dds") {
        if (config.verbose) {
            std::cout << "Loading texture: " << path << "\n";
        }
        
        state.currentTexture = state.resourceManager->loadTexture(path);
        
        if (!state.currentTexture.isValid()) {
            std::cerr << "Error: Failed to load texture\n";
            return false;
        }
        
        auto* texture = state.resourceManager->getTexture(state.currentTexture);
        if (texture && config.verbose) {
            std::cout << "  Size: " << texture->info.width << "x" << texture->info.height << "\n";
            std::cout << "  Format: " << (texture->isCompressed ? "Compressed" : "Uncompressed") << "\n";
            std::cout << "  MIP levels: " << texture->mipLevels.size() << "\n";
            std::cout << "  Memory: " << (texture->calculateMemoryUsage() / 1024) << " KB\n";
        }
    }
    else if (ext == ".las" || ext == ".laz" || ext == ".pcd") {
        if (config.verbose) {
            std::cout << "Loading point cloud: " << path << "\n";
        }
        
        state.currentPointCloud = state.resourceManager->loadPointCloud(path);
        
        if (!state.currentPointCloud.isValid()) {
            std::cerr << "Error: Failed to load point cloud\n";
            return false;
        }
        
        auto* cloud = state.resourceManager->getPointCloud(state.currentPointCloud);
        if (cloud && config.verbose) {
            std::cout << "  Points: " << cloud->totalPoints << "\n";
            std::cout << "  Octree nodes: " << cloud->octree.size() << "\n";
            std::cout << "  Bounds: " << cloud->bounds.radius << " units\n";
            std::cout << "  Memory: " << (cloud->calculateMemoryUsage() / 1024 / 1024) << " MB\n";
        }
    }
    else if (ext == ".tif" || ext == "ff" || ext == ".raw" || ext == "map") {
        if (config.verbose) {
            std::cout << "Loading terrain: " << path << "\n";
        }
        
        state.currentTerrain = state.resourceManager->loadTerrain(path);
        
        if (!state.currentTerrain.isValid()) {
            std::cerr << "Error: Failed to load terrain\n";
            return false;
        }
        
        auto* terrain = state.resourceManager->getTerrain(state.currentTerrain);
        if (terrain && config.verbose) {
            std::cout << "  Size: " << terrain->width << "x" << terrain->depth << "\n";
            std::cout << "  Chunks: " << (terrain->isStreamed ? "Enabled" : "Disabled") << "\n";
            std::cout << "  Memory: " << (terrain->calculateMemoryUsage() / 1024 / 1024) << " MB\n";
        }
    }
    else {
        std::cerr << "Error: Unsupported file format: " << ext << "\n";
        return false;
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    if (config.verbose) {
        std::cout << "Load time: " << duration.count() << " ms\n";
    }
    
    return true;
}

void update(AppState& state, const ViewerConfig& config) {
    // Calculate delta time
    double currentTime = glfwGetTime();  // Would use actual timing
    state.deltaTime = static_cast<float>(currentTime - state.lastFrameTime);
    state.lastFrameTime = currentTime;
    state.totalTime += state.deltaTime;
    
    // Update FPS
    state.frameCount++;
    if (state.frameCount % 60 == 0) {
        state.fps = 1.0 / state.deltaTime;
    }
    
    // Update resource manager time
    // state.resourceManager->update(state.deltaTime);
    
    // Update terrain LOD if loaded
    if (state.currentTerrain.isValid()) {
        auto* terrain = state.resourceManager->getTerrain(state.currentTerrain);
        if (terrain) {
            terrain->updateLOD(state.cameraPosition);
        }
    }
    
    // Update point cloud streaming if loaded
    if (state.currentPointCloud.isValid()) {
        auto* cloud = state.resourceManager->getPointCloud(state.currentPointCloud);
        if (cloud && cloud->isStreamed) {
            // Stream points based on camera position
        }
    }
}

void render(AppState& state, const ViewerConfig& config) {
    // Clear screen
    // glClearColor(...);
    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Render mesh
    if (state.currentMesh.isValid()) {
        auto* mesh = state.resourceManager->getMesh(state.currentMesh);
        if (mesh) {
            // For each primitive
            for (const auto& prim : mesh->primitives) {
                // Bind material
                // Bind vertex buffer
                // Bind index buffer
                // Draw
            }
            
            // Render bounds
            if (config.showBounds && mesh->bounds.isValid()) {
                // Draw bounding sphere
            }
        }
    }
    
    // Render point cloud
    if (state.currentPointCloud.isValid()) {
        auto* cloud = state.resourceManager->getPointCloud(state.currentPointCloud);
        if (cloud) {
            // Get visible points
            auto visiblePoints = cloud->getVisiblePoints(
                state.cameraPosition,
                math::Vector3(0.0f, 0.0f, -1.0f),  // Would use actual camera dir
                60.0f,  // FOV
                1.0f,   // Quality
                1000000 // Max points
            );
            
            // Render points
        }
    }
    
    // Render terrain
    if (state.currentTerrain.isValid()) {
        auto* terrain = state.resourceManager->getTerrain(state.currentTerrain);
        if (terrain) {
            // Render terrain chunks based on LOD
            if (terrain->isStreamed) {
                for (const auto& row : terrain->chunks) {
                    for (const auto& chunk : row) {
                        if (chunk.isLoaded) {
                            // Render chunk at appropriate LOD
                        }
                    }
                }
            } else {
                // Render full terrain
            }
        }
    }
    
    // Render stats overlay
    if (config.showStats) {
        // Draw FPS
        // Draw memory usage
        // Draw resource info
    }
}

void printStats(const AppState& state, const ResourceManager& manager) {
    std::cout << "\n=== Resource Statistics ===\n";
    std::cout << "Loaded resources: " << manager.getLoadedResourceCount() << "\n";
    std::cout << "Memory usage: " << (manager.getMemoryUsage() / 1024 / 1024) << " MB\n";
    std::cout << "Memory budget: " << (manager.getMemoryBudget() / 1024 / 1024) << " MB\n";
    std::cout << "Pending loads: " << manager.getPendingLoadCount() << "\n";
    
    if (state.currentMesh.isValid()) {
        std::cout << "\nCurrent mesh:\n";
        std::cout << "  Vertices: " << state.vertexCount << "\n";
        std::cout << "  Triangles: " << state.triangleCount << "\n";
    }
}

int main(int argc, char** argv) {
    // Parse arguments
    ViewerConfig config = parseArguments(argc, argv);
    
    if (config.filePath.empty()) {
        std::cerr << "Error: No input file specified\n";
        printUsage(argv[0]);
        return 1;
    }
    
    // Setup signal handler
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    // Initialize resource manager
    ResourceManager::Config rmConfig;
    rmConfig.maxMemoryBudget = 1024 * 1024 * 1024;  // 1GB
    rmConfig.numLoaderThreads = 2;
    rmConfig.enableCaching = true;
    rmConfig.enableStreaming = true;
    
    ResourceManager resourceManager(rmConfig);
    
    if (!resourceManager.initialize()) {
        std::cerr << "Error: Failed to initialize resource manager\n";
        return 1;
    }
    
    // Application state
    AppState state;
    state.resourceManager = &resourceManager;
    state.lastFrameTime = glfwGetTime();  // Would use actual timing
    
    // Load resource
    if (!loadResource(state, config.filePath, config)) {
        resourceManager.shutdown();
        return 1;
    }
    
    std::cout << "Phoenix Engine Model Viewer\n";
    std::cout << "Press Escape to exit\n\n";
    
    // Main loop
    while (g_running) {
        // Process events
        // glfwPollEvents();
        
        // Update
        update(state, config);
        
        // Render
        render(state, config);
        
        // Swap buffers
        // glfwSwapBuffers();
    }
    
    // Print final stats
    if (config.verbose) {
        printStats(state, resourceManager);
    }
    
    // Cleanup
    resourceManager.shutdown();
    
    std::cout << "Model viewer closed\n";
    return 0;
}
