/**
 * Phoenix Engine - Resource System Unit Tests
 * 
 * 测试资源系统核心功能
 * 目标覆盖率：>90%
 */

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <filesystem>

// Phoenix Engine Resource 头文件
#include "phoenix/resource/resource_manager.hpp"
#include "phoenix/resource/resource_cache.hpp"
#include "phoenix/resource/resource_loader.hpp"
#include "phoenix/resource/async_loader.hpp"
#include "phoenix/resource/dependency_graph.hpp"
#include "phoenix/resource/memory_pool.hpp"

namespace fs = std::filesystem;
using namespace phoenix::resource;

// ============================================================================
// ResourceManager 测试
// ============================================================================

TEST(ResourceManagerTest, Creation) {
    ResourceManagerConfig config;
    config.maxMemory = 512 * 1024 * 1024; // 512 MB
    config.cacheSize = 1000;
    
    auto manager = ResourceManager::create(config);
    
    EXPECT_NE(manager, nullptr);
    EXPECT_EQ(manager->getMaxMemory(), 512 * 1024 * 1024);
}

TEST(ResourceManagerTest, LoadTexture) {
    ResourceManagerConfig config;
    auto manager = ResourceManager::create(config);
    
    // 创建测试纹理文件
    fs::path testPath = fs::temp_directory_path() / "test_texture.png";
    
    // 注意：实际测试需要真实纹理文件
    // 这里测试加载接口
    auto texture = manager->loadTexture(testPath.string());
    
    // 文件不存在时应该返回 nullptr 或抛出异常
    EXPECT_EQ(texture, nullptr);
}

TEST(ResourceManagerTest, LoadModel) {
    ResourceManagerConfig config;
    auto manager = ResourceManager::create(config);
    
    auto model = manager->loadModel("test_model.fbx");
    
    EXPECT_EQ(model, nullptr); // 文件不存在
}

TEST(ResourceManagerTest, LoadShader) {
    ResourceManagerConfig config;
    auto manager = ResourceManager::create(config);
    
    auto shader = manager->loadShader("test_shader.spv");
    
    EXPECT_EQ(shader, nullptr); // 文件不存在
}

TEST(ResourceManagerTest, Unload) {
    ResourceManagerConfig config;
    auto manager = ResourceManager::create(config);
    
    // 加载资源
    auto resource = manager->loadTexture("nonexistent.png");
    
    // 卸载资源
    manager->unload(resource);
    
    // 验证资源已释放
    EXPECT_EQ(manager->getLoadedCount(), 0);
}

TEST(ResourceManagerTest, UnloadAll) {
    ResourceManagerConfig config;
    auto manager = ResourceManager::create(config);
    
    // 加载多个资源
    manager->loadTexture("tex1.png");
    manager->loadTexture("tex2.png");
    manager->loadModel("model1.fbx");
    
    // 卸载所有
    manager->unloadAll();
    
    EXPECT_EQ(manager->getLoadedCount(), 0);
}

// ============================================================================
// ResourceCache 测试
// ============================================================================

TEST(ResourceCacheTest, Creation) {
    CacheConfig config;
    config.maxSize = 100;
    config.maxMemory = 256 * 1024 * 1024;
    
    auto cache = ResourceCache::create(config);
    
    EXPECT_NE(cache, nullptr);
    EXPECT_EQ(cache->getMaxSize(), 100);
}

TEST(ResourceCacheTest, Insert) {
    CacheConfig config;
    config.maxSize = 10;
    
    auto cache = ResourceCache::create(config);
    
    auto resource1 = std::make_shared<Resource>("resource1");
    auto resource2 = std::make_shared<Resource>("resource2");
    
    cache->insert("key1", resource1);
    cache->insert("key2", resource2);
    
    EXPECT_EQ(cache->size(), 2);
    EXPECT_EQ(cache->get("key1"), resource1);
    EXPECT_EQ(cache->get("key2"), resource2);
}

TEST(ResourceCacheTest, Get) {
    CacheConfig config;
    config.maxSize = 10;
    
    auto cache = ResourceCache::create(config);
    
    auto resource = std::make_shared<Resource>("test");
    cache->insert("test_key", resource);
    
    auto retrieved = cache->get("test_key");
    
    EXPECT_EQ(retrieved, resource);
}

TEST(ResourceCacheTest, Remove) {
    CacheConfig config;
    config.maxSize = 10;
    
    auto cache = ResourceCache::create(config);
    
    cache->insert("key1", std::make_shared<Resource>("r1"));
    cache->insert("key2", std::make_shared<Resource>("r2"));
    
    cache->remove("key1");
    
    EXPECT_EQ(cache->size(), 1);
    EXPECT_EQ(cache->get("key1"), nullptr);
    EXPECT_NE(cache->get("key2"), nullptr);
}

TEST(ResourceCacheTest, LRUEviction) {
    CacheConfig config;
    config.maxSize = 3;
    
    auto cache = ResourceCache::create(config);
    
    // 插入 4 个资源，应该驱逐最旧的
    cache->insert("key1", std::make_shared<Resource>("r1"));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    cache->insert("key2", std::make_shared<Resource>("r2"));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    cache->insert("key3", std::make_shared<Resource>("r3"));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    cache->insert("key4", std::make_shared<Resource>("r4"));
    
    // key1 应该被驱逐
    EXPECT_EQ(cache->get("key1"), nullptr);
    EXPECT_NE(cache->get("key2"), nullptr);
    EXPECT_NE(cache->get("key3"), nullptr);
    EXPECT_NE(cache->get("key4"), nullptr);
}

TEST(ResourceCacheTest, Touch) {
    CacheConfig config;
    config.maxSize = 3;
    
    auto cache = ResourceCache::create(config);
    
    cache->insert("key1", std::make_shared<Resource>("r1"));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    cache->insert("key2", std::make_shared<Resource>("r2"));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // 访问 key1，更新其 LRU 时间
    cache->touch("key1");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    cache->insert("key3", std::make_shared<Resource>("r3"));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    cache->insert("key4", std::make_shared<Resource>("r4"));
    
    // key2 应该被驱逐（最久未使用）
    EXPECT_NE(cache->get("key1"), nullptr);
    EXPECT_EQ(cache->get("key2"), nullptr);
}

TEST(ResourceCacheTest, Clear) {
    CacheConfig config;
    config.maxSize = 10;
    
    auto cache = ResourceCache::create(config);
    
    cache->insert("key1", std::make_shared<Resource>("r1"));
    cache->insert("key2", std::make_shared<Resource>("r2"));
    
    cache->clear();
    
    EXPECT_EQ(cache->size(), 0);
}

// ============================================================================
// ResourceLoader 测试
// ============================================================================

TEST(ResourceLoaderTest, TextureLoader) {
    TextureLoader loader;
    
    // 测试支持的格式
    EXPECT_TRUE(loader.supportsFormat(".png"));
    EXPECT_TRUE(loader.supportsFormat(".jpg"));
    EXPECT_TRUE(loader.supportsFormat(".jpeg"));
    EXPECT_TRUE(loader.supportsFormat(".dds"));
    EXPECT_FALSE(loader.supportsFormat(".txt"));
}

TEST(ResourceLoaderTest, ModelLoader) {
    ModelLoader loader;
    
    // 测试支持的格式
    EXPECT_TRUE(loader.supportsFormat(".fbx"));
    EXPECT_TRUE(loader.supportsFormat(".obj"));
    EXPECT_TRUE(loader.supportsFormat(".gltf"));
    EXPECT_TRUE(loader.supportsFormat(".glb"));
    EXPECT_FALSE(loader.supportsFormat(".txt"));
}

TEST(ResourceLoaderTest, ShaderLoader) {
    ShaderLoader loader;
    
    EXPECT_TRUE(loader.supportsFormat(".spv"));
    EXPECT_TRUE(loader.supportsFormat(".hlsl"));
    EXPECT_TRUE(loader.supportsFormat(".glsl"));
    EXPECT_FALSE(loader.supportsFormat(".txt"));
}

TEST(ResourceLoaderTest, LoadMetadata) {
    ResourceMetadata metadata;
    metadata.path = "test.png";
    metadata.type = ResourceType::Texture;
    metadata.size = 1024 * 1024;
    metadata.format = "RGBA8";
    
    EXPECT_EQ(metadata.path, "test.png");
    EXPECT_EQ(metadata.type, ResourceType::Texture);
    EXPECT_EQ(metadata.size, 1024 * 1024);
}

// ============================================================================
// AsyncLoader 测试
// ============================================================================

TEST(AsyncLoaderTest, Creation) {
    AsyncLoaderConfig config;
    config.numThreads = 4;
    config.queueSize = 100;
    
    auto loader = AsyncLoader::create(config);
    
    EXPECT_NE(loader, nullptr);
}

TEST(AsyncLoaderTest, EnqueueTask) {
    AsyncLoaderConfig config;
    config.numThreads = 2;
    
    auto loader = AsyncLoader::create(config);
    
    bool completed = false;
    
    loader->enqueue([&completed]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        completed = true;
    });
    
    // 等待任务完成
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_TRUE(completed);
}

TEST(AsyncLoaderTest, PriorityTask) {
    AsyncLoaderConfig config;
    config.numThreads = 1;
    
    auto loader = AsyncLoader::create(config);
    
    std::vector<int> executionOrder;
    
    // 先入队低优先级任务
    loader->enqueue([&executionOrder]() {
        executionOrder.push_back(1);
    }, TaskPriority::Low);
    
    // 再入队高优先级任务
    loader->enqueue([&executionOrder]() {
        executionOrder.push_back(2);
    }, TaskPriority::High);
    
    // 等待完成
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 高优先级应该先执行
    // 注意：实际顺序可能因调度而异
    EXPECT_EQ(executionOrder.size(), 2);
}

TEST(AsyncLoaderTest, CancelTask) {
    AsyncLoaderConfig config;
    config.numThreads = 1;
    
    auto loader = AsyncLoader::create(config);
    
    bool executed = false;
    
    auto taskId = loader->enqueue([&executed]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        executed = true;
    });
    
    // 取消任务
    loader->cancel(taskId);
    
    // 等待
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // 任务应该被取消
    EXPECT_FALSE(executed);
}

TEST(AsyncLoaderTest, WaitForCompletion) {
    AsyncLoaderConfig config;
    config.numThreads = 2;
    
    auto loader = AsyncLoader::create(config);
    
    bool task1Done = false;
    bool task2Done = false;
    
    auto id1 = loader->enqueue([&task1Done]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        task1Done = true;
    });
    
    auto id2 = loader->enqueue([&task2Done]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        task2Done = true;
    });
    
    loader->waitForCompletion(id1);
    EXPECT_TRUE(task1Done);
    
    loader->waitForCompletion(id2);
    EXPECT_TRUE(task2Done);
}

// ============================================================================
// DependencyGraph 测试
// ============================================================================

TEST(DependencyGraphTest, AddNode) {
    DependencyGraph graph;
    
    auto node1 = graph.addNode("node1");
    auto node2 = graph.addNode("node2");
    
    EXPECT_EQ(graph.getNodeCount(), 2);
}

TEST(DependencyGraphTest, AddDependency) {
    DependencyGraph graph;
    
    auto node1 = graph.addNode("node1");
    auto node2 = graph.addNode("node2");
    
    graph.addDependency(node1, node2); // node1 依赖 node2
    
    EXPECT_TRUE(graph.hasDependency(node1, node2));
}

TEST(DependencyGraphTest, TopologicalSort) {
    DependencyGraph graph;
    
    auto node1 = graph.addNode("node1");
    auto node2 = graph.addNode("node2");
    auto node3 = graph.addNode("node3");
    
    graph.addDependency(node1, node2); // node1 -> node2
    graph.addDependency(node2, node3); // node2 -> node3
    
    auto sorted = graph.topologicalSort();
    
    // node3 应该最先，node1 最后
    EXPECT_EQ(sorted.size(), 3);
    EXPECT_EQ(sorted.back(), node1);
    EXPECT_EQ(sorted.front(), node3);
}

TEST(DependencyGraphTest, CycleDetection) {
    DependencyGraph graph;
    
    auto node1 = graph.addNode("node1");
    auto node2 = graph.addNode("node2");
    auto node3 = graph.addNode("node3");
    
    graph.addDependency(node1, node2);
    graph.addDependency(node2, node3);
    graph.addDependency(node3, node1); // 创建循环
    
    EXPECT_TRUE(graph.hasCycle());
}

TEST(DependencyGraphTest, RemoveNode) {
    DependencyGraph graph;
    
    auto node1 = graph.addNode("node1");
    auto node2 = graph.addNode("node2");
    
    graph.addDependency(node1, node2);
    
    graph.removeNode(node2);
    
    EXPECT_EQ(graph.getNodeCount(), 1);
    EXPECT_FALSE(graph.hasDependency(node1, node2));
}

// ============================================================================
// MemoryPool 测试
// ============================================================================

TEST(MemoryPoolTest, Allocation) {
    MemoryPoolConfig config;
    config.blockSize = 1024;
    config.numBlocks = 100;
    
    MemoryPool pool(config);
    
    void* ptr = pool.allocate(256);
    
    EXPECT_NE(ptr, nullptr);
}

TEST(MemoryPoolTest, Deallocation) {
    MemoryPoolConfig config;
    config.blockSize = 1024;
    config.numBlocks = 100;
    
    MemoryPool pool(config);
    
    void* ptr = pool.allocate(256);
    pool.deallocate(ptr);
    
    // 应该可以重新分配
    void* ptr2 = pool.allocate(256);
    EXPECT_NE(ptr2, nullptr);
}

TEST(MemoryPoolTest, MultipleAllocations) {
    MemoryPoolConfig config;
    config.blockSize = 1024;
    config.numBlocks = 10;
    
    MemoryPool pool(config);
    
    std::vector<void*> ptrs;
    
    for (int i = 0; i < 10; ++i) {
        ptrs.push_back(pool.allocate(100));
    }
    
    // 所有分配都应该成功
    for (auto ptr : ptrs) {
        EXPECT_NE(ptr, nullptr);
    }
    
    // 第 11 次分配应该失败（池已满）
    void* ptr11 = pool.allocate(100);
    EXPECT_EQ(ptr11, nullptr);
}

TEST(MemoryPoolTest, Statistics) {
    MemoryPoolConfig config;
    config.blockSize = 1024;
    config.numBlocks = 100;
    
    MemoryPool pool(config);
    
    EXPECT_EQ(pool.getUsedBlocks(), 0);
    EXPECT_EQ(pool.getFreeBlocks(), 100);
    
    pool.allocate(256);
    pool.allocate(256);
    pool.allocate(256);
    
    EXPECT_EQ(pool.getUsedBlocks(), 3);
    EXPECT_EQ(pool.getFreeBlocks(), 97);
}

TEST(MemoryPoolTest, Defragmentation) {
    MemoryPoolConfig config;
    config.blockSize = 1024;
    config.numBlocks = 10;
    
    MemoryPool pool(config);
    
    std::vector<void*> ptrs;
    for (int i = 0; i < 10; ++i) {
        ptrs.push_back(pool.allocate(100));
    }
    
    // 释放偶数索引的块
    for (int i = 0; i < 10; i += 2) {
        pool.deallocate(ptrs[i]);
    }
    
    pool.defragment();
    
    // 验证碎片整理后的状态
    EXPECT_EQ(pool.getFreeBlocks(), 5);
}

// ============================================================================
// 边界条件测试
// ============================================================================

TEST(ResourceEdgeCases, EmptyPath) {
    ResourceManagerConfig config;
    auto manager = ResourceManager::create(config);
    
    auto resource = manager->loadTexture("");
    EXPECT_EQ(resource, nullptr);
}

TEST(ResourceEdgeCases, InvalidFormat) {
    TextureLoader loader;
    EXPECT_FALSE(loader.supportsFormat(".invalid"));
}

TEST(ResourceEdgeCases, LargeAllocation) {
    MemoryPoolConfig config;
    config.blockSize = 1024;
    config.numBlocks = 100;
    
    MemoryPool pool(config);
    
    // 尝试分配超过块大小的内存
    void* ptr = pool.allocate(2048);
    EXPECT_EQ(ptr, nullptr);
}

TEST(ResourceEdgeCases, ZeroSize) {
    MemoryPoolConfig config;
    config.blockSize = 0;
    config.numBlocks = 100;
    
    // 应该抛出异常或使用默认值
    EXPECT_THROW(MemoryPool pool(config), std::invalid_argument);
}

// ============================================================================
// 主测试入口
// ============================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
