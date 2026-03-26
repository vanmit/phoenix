# Phoenix Engine 性能优化指南

## 📊 性能分析

### 内置性能分析器

```cpp
// 启用性能分析
export PHOENIX_PROFILER=1

// 或使用 API
phoenix::Profiler::enable(true);
phoenix::Profiler::setOutputFile("trace.json");
```

### 性能指标

```cpp
auto stats = renderer->getStats();

std::cout << "FPS: " << stats.fps << std::endl;
std::cout << "Frame Time: " << stats.frameTime << " ms" << std::endl;
std::cout << "Draw Calls: " << stats.drawCalls << std::endl;
std::cout << "Triangles: " << stats.triangles << std::endl;
std::cout << "VRAM Usage: " << stats.vramUsage << " MB" << std::endl;
```

## 🎯 渲染优化

### 1. 减少绘制调用

```cpp
// ❌ 低效 - 每个对象一次绘制调用
for (auto& object : objects) {
    renderer->draw(object.pipeline);
}

// ✅ 高效 - 批处理
BatchRenderer batcher;
for (auto& object : objects) {
    batcher.submit(object.mesh, object.material, object.transform);
}
batcher.flush();  // 按材质分组
```

### 2. 实例化渲染

```cpp
// ❌ 低效 - 多次绘制
for (int i = 0; i < 1000; i++) {
    transform.setPosition(positions[i]);
    renderer->draw(pipeline);
}

// ✅ 高效 - 实例化
std::vector<Transform> transforms(1000);
// 填充 transforms...

auto instanceBuffer = renderer->createVertexBuffer(
    transforms.data(),
    transforms.size() * sizeof(Transform)
);

renderer->drawInstanced(pipeline, vertexCount, 1000);
```

### 3. 视锥体剔除

```cpp
class Culler {
public:
    std::vector<Mesh*> cull(const Frustum& frustum, 
                            const std::vector<Mesh*>& meshes) {
        std::vector<Mesh*> visible;
        
        for (auto* mesh : meshes) {
            if (frustum.intersects(mesh->getBoundingBox())) {
                visible.push_back(mesh);
            }
        }
        
        return visible;
    }
};

// 使用
auto visible = culler.cull(camera.getFrustum(), allMeshes);
for (auto* mesh : visible) {
    renderer->draw(mesh->pipeline);
}
```

### 4. LOD 系统

```cpp
class LODManager {
public:
    void update(const Camera& camera) {
        for (auto& lod : lods_) {
            float distance = camera.distanceTo(lod.position);
            
            if (distance > lod.thresholds[2]) {
                lod.currentLevel = 3;  // 最低细节
            } else if (distance > lod.thresholds[1]) {
                lod.currentLevel = 2;
            } else if (distance > lod.thresholds[0]) {
                lod.currentLevel = 1;
            } else {
                lod.currentLevel = 0;  // 最高细节
            }
        }
    }
};
```

## 💾 内存优化

### 1. 纹理压缩

```cpp
// 使用压缩纹理格式
phoenix::TextureConfig config;
config.format = phoenix::TextureFormat::BC7;  // 高质量压缩
// 或
config.format = phoenix::TextureFormat::BC3;  // 带 alpha
// 或 (移动端)
config.format = phoenix::TextureFormat::ASTC_4x4;

auto texture = renderer->createTexture(config, data);

// 内存对比:
// RGBA8: 16 bytes/pixel
// BC7:   8 bytes/pixel (50% 节省)
// ASTC:  8 bytes/pixel (50% 节省)
```

### 2. 网格优化

```cpp
// 使用网格优化工具
MeshOptimizer optimizer;

// 顶点缓存优化
optimizer.optimizeVertexCache(vertices, indices);

// 顶点获取优化
optimizer.optimizeVertexFetch(vertices, indices);

// 简化 (LOD 生成)
auto simplified = optimizer.simplify(vertices, indices, 
                                      targetVertexCount);
```

### 3. 内存池

```cpp
class ResourcePool {
public:
    ResourcePool(size_t blockSize) : blockSize_(blockSize) {}
    
    void* allocate(size_t size) {
        // 从池中分配，避免频繁 malloc/free
        return pool_.allocate(size);
    }
    
    void deallocate(void* ptr) {
        pool_.deallocate(ptr);
    }
    
private:
    MemoryPool pool_;
    size_t blockSize_;
};
```

## ⚡ CPU 优化

### 1. 多线程渲染

```cpp
class ParallelRenderer {
public:
    void render(Scene& scene) {
        // 主线程：更新场景
        scene.update();
        
        // 工作线程：录制命令缓冲
        std::vector<std::future<CommandBuffer>> futures;
        
        for (size_t i = 0; i < chunks_.size(); i++) {
            futures.push_back(pool_.submit([&, i]() {
                return recordCommands(chunks_[i]);
            }));
        }
        
        // 收集命令缓冲
        std::vector<CommandBuffer> commands;
        for (auto& future : futures) {
            commands.push_back(future.get());
        }
        
        // 渲染线程：提交
        renderer->submit(commands);
    }
    
private:
    ThreadPool pool_;
    std::vector<SceneChunk> chunks_;
};
```

### 2. 数据局部性

```cpp
// ❌ 差 - 指针追逐
class GameObject {
    Transform* transform;
    Mesh* mesh;
    Material* material;
};

// ✅ 好 - 连续内存
struct GameObjectArray {
    std::vector<Transform> transforms;
    std::vector<Mesh*> meshes;
    std::vector<Material*> materials;
    
    void update() {
        // 连续访问，缓存友好
        for (size_t i = 0; i < transforms.size(); i++) {
            transforms[i].update();
        }
    }
};
```

### 3. 避免动态分配

```cpp
// ❌ 差 - 每帧分配
void render() {
    std::vector<Mesh*> visible;
    for (auto& obj : objects) {
        if (isVisible(obj)) {
            visible.push_back(&obj.mesh);  // 分配!
        }
    }
}

// ✅ 好 - 重用容器
class Renderer {
    std::vector<Mesh*> visibleCache;
    
    void render() {
        visibleCache.clear();  // 不清空内存
        for (auto& obj : objects) {
            if (isVisible(obj)) {
                visibleCache.push_back(&obj.mesh);
            }
        }
    }
};
```

## 📱 移动端优化

### 1. 功耗优化

```cpp
// 限制帧率
config.maxFPS = 30;  // 或 60

// 降低分辨率
config.renderScale = 0.75f;  // 75% 分辨率

// 减少后处理
config.bloom = false;
config.ssao = false;
config.shadows = false;
```

### 2. 内存限制

```cpp
// 纹理预算
const size_t TEXTURE_BUDGET = 100 * 1024 * 1024;  // 100MB

// 网格预算
const size_t MESH_BUDGET = 50 * 1024 * 1024;  // 50MB

// 监控使用
auto stats = renderer->getStats();
if (stats.textureMemory > TEXTURE_BUDGET) {
    resourceManager.evictUnusedTextures();
}
```

### 3. 热节流

```cpp
class ThermalManager {
public:
    void update() {
        float temp = platform.getTemperature();
        
        if (temp > 45.0f) {
            // 降低质量
            config.shadowResolution = 512;
            config.msaaSamples = 0;
        }
        
        if (temp > 50.0f) {
            // 降低帧率
            config.maxFPS = 30;
        }
    }
};
```

## 🔍 性能分析工具

### 1. 帧时间分析

```cpp
class FrameAnalyzer {
public:
    void beginFrame() {
        frameStart_ = std::chrono::high_resolution_clock::now();
    }
    
    void endFrame() {
        auto frameEnd = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<
            std::chrono::microseconds
        >(frameEnd - frameStart_).count();
        
        frameTimes_.push_back(duration);
        
        if (frameTimes_.size() > 100) {
            frameTimes_.erase(frameTimes_.begin());
        }
        
        float avg = std::accumulate(
            frameTimes_.begin(), 
            frameTimes_.end(), 
            0.0f
        ) / frameTimes_.size() / 1000.0f;
        
        std::cout << "Avg Frame Time: " << avg << " ms" 
                  << " (" << 1000.0f / avg << " FPS)" << std::endl;
    }
    
private:
    std::vector<float> frameTimes_;
    std::chrono::time_point<std::chrono::high_resolution_clock> frameStart_;
};
```

### 2. 内存分析

```cpp
class MemoryTracker {
public:
    void* allocate(size_t size, const char* tag) {
        void* ptr = malloc(size);
        allocations_[ptr] = {size, tag};
        totalMemory_ += size;
        return ptr;
    }
    
    void deallocate(void* ptr) {
        auto it = allocations_.find(ptr);
        if (it != allocations_.end()) {
            totalMemory_ -= it->second.size;
            allocations_.erase(it);
        }
        free(ptr);
    }
    
    void report() {
        std::cout << "Total Memory: " << totalMemory_ / 1024 << " KB" 
                  << std::endl;
        std::cout << "Allocations: " << allocations_.size() << std::endl;
    }
    
private:
    std::unordered_map<void*, AllocationInfo> allocations_;
    size_t totalMemory_ = 0;
};
```

## 📊 优化检查清单

### 渲染

- [ ] 使用批处理减少绘制调用
- [ ] 使用实例化渲染重复对象
- [ ] 实现视锥体剔除
- [ ] 实现 LOD 系统
- [ ] 使用 occlusion culling
- [ ] 减少状态切换
- [ ] 使用纹理数组

### 内存

- [ ] 使用压缩纹理
- [ ] 优化网格拓扑
- [ ] 使用内存池
- [ ] 及时释放未用资源
- [ ] 流式加载大资源

### CPU

- [ ] 多线程渲染命令录制
- [ ] 优化数据布局 (SoA)
- [ ] 避免每帧动态分配
- [ ] 使用对象池
- [ ] 减少锁竞争

### GPU

- [ ] 减少过度绘制
- [ ] 使用合适的纹理格式
- [ ] 优化着色器复杂度
- [ ] 使用 Early-Z
- [ ] 避免 GPU-CPU 同步

---
*最后更新：2026-03-26*
