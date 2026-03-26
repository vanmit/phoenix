/**
 * @file benchmark_scene.cpp
 * @brief Phoenix Engine Scene System Benchmarks
 * 
 * Performance benchmarks for scene graph, spatial structures, and rendering.
 * Tests with 10,000+ objects for realistic workloads.
 */

#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <iomanip>
#include <algorithm>

#include "../../../include/phoenix/scene/scene.hpp"
#include "../../../include/phoenix/scene/scene_node.hpp"
#include "../../../include/phoenix/scene/octree.hpp"
#include "../../../include/phoenix/scene/bvh.hpp"
#include "../../../include/phoenix/scene/lod.hpp"
#include "../../../include/phoenix/scene/ecs.hpp"
#include "../../../include/phoenix/math/frustum.hpp"

using namespace phoenix;
using namespace phoenix::scene;
using namespace phoenix::math;

// ============================================================================
// Benchmark Utilities
// ============================================================================

struct BenchmarkResult {
    std::string name;
    double durationMs;
    size_t objectCount;
    double objectsPerSecond;
};

class Timer {
public:
    void start() {
        start_ = std::chrono::high_resolution_clock::now();
    }
    
    double elapsedMs() const {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - start_).count();
    }
    
private:
    std::chrono::high_resolution_clock::time_point start_;
};

std::vector<BenchmarkResult> results;

void printResults() {
    std::cout << "\n";
    std::cout << "=============================================================================\n";
    std::cout << "                    Phoenix Engine Scene System Benchmarks\n";
    std::cout << "=============================================================================\n\n";
    
    std::cout << std::left << std::setw(50) << "Test" 
              << std::right << std::setw(12) << "Objects"
              << std::setw(15) << "Time (ms)"
              << std::setw(18) << "Ops/sec" << "\n";
    std::cout << std::string(95, '-') << "\n";
    
    for (const auto& r : results) {
        std::cout << std::left << std::setw(50) << r.name
                  << std::right << std::setw(12) << r.objectCount
                  << std::setw(15) << std::fixed << std::setprecision(2) << r.durationMs
                  << std::setw(18) << std::scientific << std::setprecision(2) << r.objectsPerSecond << "\n";
    }
    
    std::cout << "=============================================================================\n";
}

// ============================================================================
// Scene Graph Benchmarks
// ============================================================================

BenchmarkResult benchmarkSceneCreation(size_t nodeCount) {
    Timer timer;
    timer.start();
    
    Scene scene("Benchmark");
    
    for (size_t i = 0; i < nodeCount; ++i) {
        auto node = std::make_shared<SceneNode>("Node" + std::to_string(i));
        node->setPosition(Vector3(
            static_cast<float>(i % 100),
            static_cast<float>((i / 100) % 100),
            static_cast<float>(i / 10000)
        ));
        scene.addNode(node);
    }
    
    double duration = timer.elapsedMs();
    
    return {
        "Scene Graph Creation",
        duration,
        nodeCount,
        nodeCount / (duration / 1000.0)
    };
}

BenchmarkResult benchmarkHierarchyTraversal(size_t nodeCount) {
    Scene scene("Benchmark");
    
    // Create flat hierarchy
    for (size_t i = 0; i < nodeCount; ++i) {
        auto node = std::make_shared<SceneNode>("Node" + std::to_string(i));
        scene.addNode(node);
    }
    
    Timer timer;
    timer.start();
    
    const int iterations = 100;
    size_t visitCount = 0;
    
    for (int i = 0; i < iterations; ++i) {
        scene.traverse([&visitCount](SceneNode&) {
            ++visitCount;
        });
    }
    
    double duration = timer.elapsedMs() / iterations;
    
    return {
        "Hierarchy Traversal (flat)",
        duration,
        nodeCount,
        visitCount / iterations / (duration / 1000.0)
    };
}

BenchmarkResult benchmarkDeepHierarchy(size_t depth, size_t breadth) {
    Scene scene("Benchmark");
    
    // Create deep hierarchy
    std::vector<SceneNode::Ptr> currentLevel;
    currentLevel.push_back(scene.getRoot()->shared_from_this());
    
    for (size_t d = 0; d < depth; ++d) {
        std::vector<SceneNode::Ptr> nextLevel;
        
        for (auto& parent : currentLevel) {
            for (size_t b = 0; b < breadth; ++b) {
                auto child = std::make_shared<SceneNode>(
                    "D" + std::to_string(d) + "B" + std::to_string(b)
                );
                parent->addChild(child);
                nextLevel.push_back(child);
            }
        }
        
        currentLevel = std::move(nextLevel);
    }
    
    const size_t totalNodes = scene.getNodeCount();
    
    Timer timer;
    timer.start();
    
    const int iterations = 10;
    
    for (int i = 0; i < iterations; ++i) {
        scene.updateTransforms();
    }
    
    double duration = timer.elapsedMs() / iterations;
    
    return {
        "Deep Hierarchy Update (d=" + std::to_string(depth) + ", b=" + std::to_string(breadth) + ")",
        duration,
        totalNodes,
        totalNodes / (duration / 1000.0)
    };
}

BenchmarkResult benchmarkTransformPropagation(size_t nodeCount) {
    Scene scene("Benchmark");
    
    // Create chain hierarchy
    SceneNode::Ptr parent = scene.getRoot();
    
    for (size_t i = 0; i < nodeCount; ++i) {
        auto child = std::make_shared<SceneNode>("Node" + std::to_string(i));
        parent->addChild(child);
        parent = child;
    }
    
    Timer timer;
    timer.start();
    
    // Modify root transform
    scene.getRoot()->setPosition(Vector3(100, 100, 100));
    scene.getRoot()->setRotation(Quaternion::fromAxisAngle(Vector3::unitY(), 0.5f));
    
    // Propagate through hierarchy
    scene.updateTransforms();
    
    double duration = timer.elapsedMs();
    
    return {
        "Transform Propagation (chain)",
        duration,
        nodeCount,
        nodeCount / (duration / 1000.0)
    };
}

// ============================================================================
// Spatial Structure Benchmarks
// ============================================================================

BenchmarkResult benchmarkOctreeBuild(size_t objectCount) {
    BoundingBox rootBounds(Vector3(-500, -500, -500), Vector3(500, 500, 500));
    Octree octree(rootBounds, 8, 1.0f, 16);
    
    std::vector<SceneNode::Ptr> nodes;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-400, 400);
    
    for (size_t i = 0; i < objectCount; ++i) {
        auto node = std::make_shared<SceneNode>("Object" + std::to_string(i));
        node->setPosition(Vector3(dist(gen), dist(gen), dist(gen)));
        node->setLocalBoundingBox(BoundingBox(
            node->getPosition() - Vector3(1, 1, 1),
            node->getPosition() + Vector3(1, 1, 1)
        ));
        nodes.push_back(node);
    }
    
    Timer timer;
    timer.start();
    
    for (auto& node : nodes) {
        octree.insert(node.get(), node->getLocalBoundingBox());
    }
    
    double duration = timer.elapsedMs();
    
    return {
        "Octree Build",
        duration,
        objectCount,
        objectCount / (duration / 1000.0)
    };
}

BenchmarkResult benchmarkOctreeQuery(size_t objectCount, size_t queryCount) {
    BoundingBox rootBounds(Vector3(-500, -500, -500), Vector3(500, 500, 500));
    Octree octree(rootBounds, 8, 1.0f, 16);
    
    std::vector<SceneNode::Ptr> nodes;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-400, 400);
    
    for (size_t i = 0; i < objectCount; ++i) {
        auto node = std::make_shared<SceneNode>("Object" + std::to_string(i));
        node->setPosition(Vector3(dist(gen), dist(gen), dist(gen)));
        nodes.push_back(node);
        octree.insert(node.get(), node->getLocalBoundingBox());
    }
    
    Timer timer;
    timer.start();
    
    size_t totalResults = 0;
    
    for (size_t i = 0; i < queryCount; ++i) {
        Vector3 queryPoint(dist(gen), dist(gen), dist(gen));
        auto results = octree.query(queryPoint);
        totalResults += results.size();
    }
    
    double duration = timer.elapsedMs();
    
    return {
        "Octree Query (" + std::to_string(queryCount) + " queries)",
        duration,
        queryCount,
        queryCount / (duration / 1000.0)
    };
}

BenchmarkResult benchmarkBVHBuild(size_t objectCount) {
    std::vector<SceneNode*> nodes;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-400, 400);
    
    for (size_t i = 0; i < objectCount; ++i) {
        auto node = std::make_shared<SceneNode>("Object" + std::to_string(i));
        node->setPosition(Vector3(dist(gen), dist(gen), dist(gen)));
        node->setLocalBoundingBox(BoundingBox(
            node->getPosition() - Vector3(1, 1, 1),
            node->getPosition() + Vector3(1, 1, 1)
        ));
        nodes.push_back(node.get());
    }
    
    Timer timer;
    timer.start();
    
    BVH bvh(nodes, true); // Use SAH
    
    double duration = timer.elapsedMs();
    
    return {
        "BVH Build (SAH)",
        duration,
        objectCount,
        objectCount / (duration / 1000.0)
    };
}

BenchmarkResult benchmarkBVHBuildSimple(size_t objectCount) {
    std::vector<SceneNode*> nodes;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-400, 400);
    
    for (size_t i = 0; i < objectCount; ++i) {
        auto node = std::make_shared<SceneNode>("Object" + std::to_string(i));
        node->setPosition(Vector3(dist(gen), dist(gen), dist(gen)));
        nodes.push_back(node.get());
    }
    
    Timer timer;
    timer.start();
    
    BVH bvh(nodes, false); // Simple build
    
    double duration = timer.elapsedMs();
    
    return {
        "BVH Build (Simple)",
        duration,
        objectCount,
        objectCount / (duration / 1000.0)
    };
}

BenchmarkResult benchmarkBVHRaycast(size_t objectCount, size_t rayCount) {
    std::vector<SceneNode*> nodes;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-400, 400);
    
    for (size_t i = 0; i < objectCount; ++i) {
        auto node = std::make_shared<SceneNode>("Object" + std::to_string(i));
        node->setPosition(Vector3(dist(gen), dist(gen), dist(gen)));
        nodes.push_back(node.get());
    }
    
    BVH bvh(nodes, false);
    
    Timer timer;
    timer.start();
    
    std::uniform_real_distribution<float> dirDist(-1, 1);
    size_t hits = 0;
    
    for (size_t i = 0; i < rayCount; ++i) {
        Vector3 origin(dist(gen), dist(gen), dist(gen));
        Vector3 dir(dirDist(gen), dirDist(gen), dirDist(gen)).normalized();
        
        auto hit = bvh.raycast(origin, dir);
        if (hit.object) ++hits;
    }
    
    double duration = timer.elapsedMs();
    
    return {
        "BVH Raycast (" + std::to_string(rayCount) + " rays)",
        duration,
        rayCount,
        rayCount / (duration / 1000.0)
    };
}

// ============================================================================
// Culling Benchmarks
// ============================================================================

BenchmarkResult benchmarkFrustumCulling(size_t objectCount) {
    Scene scene("Benchmark");
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-500, 500);
    
    for (size_t i = 0; i < objectCount; ++i) {
        auto node = std::make_shared<SceneNode>("Object" + std::to_string(i), NodeType::Mesh);
        node->setPosition(Vector3(dist(gen), dist(gen), dist(gen)));
        node->setLocalBoundingBox(BoundingBox(
            node->getPosition() - Vector3(5, 5, 5),
            node->getPosition() + Vector3(5, 5, 5)
        ));
        scene.addNode(node);
    }
    
    scene.updateTransforms();
    
    // Create view frustum
    Matrix4 view = Matrix4::lookAt(Vector3(0, 0, 0), Vector3(0, 0, -1), Vector3::unitY());
    Matrix4 proj = Matrix4::perspective(1.047f, 16.0f/9.0f, 0.1f, 1000.0f);
    Frustum frustum = Frustum::fromViewProjection(proj * view);
    
    Timer timer;
    timer.start();
    
    const int iterations = 10;
    
    for (int i = 0; i < iterations; ++i) {
        scene.cullFrustum(frustum);
    }
    
    double duration = timer.elapsedMs() / iterations;
    
    return {
        "Frustum Culling",
        duration,
        objectCount,
        objectCount / (duration / 1000.0)
    };
}

// ============================================================================
// LOD Benchmarks
// ============================================================================

BenchmarkResult benchmarkLODSystem(size_t lodCount) {
    LODSystem::Config config;
    config.updateInterval = 0.0f;
    config.maxUpdatesPerFrame = 10000;
    
    LODSystem system(config);
    
    for (size_t i = 0; i < lodCount; ++i) {
        auto lod = std::make_shared<LODComponent>();
        lod->addLevel(LODLevel(LODQuality::VeryHigh, 10.0f));
        lod->addLevel(LODLevel(LODQuality::High, 25.0f));
        lod->addLevel(LODLevel(LODQuality::Medium, 50.0f));
        lod->addLevel(LODLevel(LODQuality::Low, 100.0f));
        lod->addLevel(LODLevel(LODQuality::VeryLow, 200.0f));
        system.registerLOD(lod);
    }
    
    Timer timer;
    timer.start();
    
    const int iterations = 10;
    
    for (int i = 0; i < iterations; ++i) {
        Vector3 camPos(i * 10, 0, 0);
        system.update(camPos, 0.016f);
    }
    
    double duration = timer.elapsedMs() / iterations;
    
    return {
        "LOD System Update",
        duration,
        lodCount,
        lodCount / (duration / 1000.0)
    };
}

// ============================================================================
// ECS Benchmarks
// ============================================================================

BenchmarkResult benchmarkECSCreation(size_t entityCount) {
    Timer timer;
    timer.start();
    
    ECSWorld world;
    world.registerComponent<TransformComponent>();
    
    std::vector<Entity> entities;
    entities.reserve(entityCount);
    
    for (size_t i = 0; i < entityCount; ++i) {
        Entity e = world.createEntity();
        world.addComponent<TransformComponent>(e, TransformComponent());
        entities.push_back(e);
    }
    
    double duration = timer.elapsedMs();
    
    return {
        "ECS Entity Creation",
        duration,
        entityCount,
        entityCount / (duration / 1000.0)
    };
}

BenchmarkResult benchmarkECSComponentAccess(size_t entityCount) {
    ECSWorld world;
    world.registerComponent<TransformComponent>();
    
    std::vector<Entity> entities;
    
    for (size_t i = 0; i < entityCount; ++i) {
        Entity e = world.createEntity();
        world.addComponent<TransformComponent>(e, TransformComponent());
        entities.push_back(e);
    }
    
    Timer timer;
    timer.start();
    
    const int iterations = 100;
    
    for (int i = 0; i < iterations; ++i) {
        for (const auto& e : entities) {
            volatile auto& comp = world.getComponent<TransformComponent>(e);
            (void)comp;
        }
    }
    
    double duration = timer.elapsedMs() / iterations;
    
    return {
        "ECS Component Access (100 iterations)",
        duration,
        entityCount,
        (entityCount * 100) / (duration / 1000.0)
    };
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "Phoenix Engine Scene System Benchmarks\n";
    std::cout << "Testing with 10,000+ objects for realistic workloads\n\n";
    
    // Scene Graph Benchmarks
    std::cout << "Running Scene Graph benchmarks...\n";
    results.push_back(benchmarkSceneCreation(10000));
    results.push_back(benchmarkHierarchyTraversal(10000));
    results.push_back(benchmarkDeepHierarchy(5, 10)); // 5 levels, 10 breadth = ~100k nodes
    results.push_back(benchmarkTransformPropagation(1000));
    
    // Spatial Structure Benchmarks
    std::cout << "Running Spatial Structure benchmarks...\n";
    results.push_back(benchmarkOctreeBuild(10000));
    results.push_back(benchmarkOctreeQuery(10000, 1000));
    results.push_back(benchmarkBVHBuild(10000));
    results.push_back(benchmarkBVHBuildSimple(10000));
    results.push_back(benchmarkBVHRaycast(10000, 10000));
    
    // Culling Benchmarks
    std::cout << "Running Culling benchmarks...\n";
    results.push_back(benchmarkFrustumCulling(10000));
    
    // LOD Benchmarks
    std::cout << "Running LOD benchmarks...\n";
    results.push_back(benchmarkLODSystem(1000));
    
    // ECS Benchmarks
    std::cout << "Running ECS benchmarks...\n";
    results.push_back(benchmarkECSCreation(10000));
    results.push_back(benchmarkECSComponentAccess(10000));
    
    // Print results
    printResults();
    
    return 0;
}
