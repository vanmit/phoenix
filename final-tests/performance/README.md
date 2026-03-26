# Phoenix Engine Performance Benchmarks

**测试目标**:
- 帧率：1080p 60fps, 4K 30fps
- 内存：<512MB (桌面), <256MB (移动)
- 加载时间：小型<1s, 中型<3s, 大型<10s

---

## 基准测试文件

- `benchmark_fps.cpp` - 帧率测试
- `benchmark_memory.cpp` - 内存占用测试
- `benchmark_loading.cpp` - 加载时间测试
- `benchmark_gpu_cpu.cpp` - GPU/CPU 性能分析

---

## 运行基准测试

```bash
mkdir build && cd build
cmake .. -DPHOENIX_BUILD_BENCHMARKS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)

# 运行所有基准测试
./bin/performance_benchmark

# 单独运行
./bin/benchmark_fps
./bin/benchmark_memory
./bin/benchmark_loading
./bin/benchmark_gpu_cpu
```

---

## 性能目标

### 帧率基准

| 场景 | 分辨率 | 目标 FPS | 最低 FPS |
|------|--------|---------|---------|
| 简单 | 1080p | 60 | 55 |
| 简单 | 1440p | 60 | 55 |
| 简单 | 4K | 30 | 27 |
| 中等 | 1080p | 60 | 50 |
| 中等 | 1440p | 60 | 45 |
| 中等 | 4K | 30 | 25 |
| 复杂 | 1080p | 60 | 45 |
| 复杂 | 1440p | 60 | 40 |
| 复杂 | 4K | 30 | 24 |

### 内存基准

| 平台 | 空闲 | 简单场景 | 中等场景 | 复杂场景 |
|------|------|---------|---------|---------|
| Windows | <200MB | <300MB | <400MB | <512MB |
| Linux | <200MB | <300MB | <400MB | <512MB |
| macOS | <200MB | <300MB | <400MB | <512MB |
| iOS | <100MB | <150MB | <200MB | <256MB |
| Android | <100MB | <150MB | <200MB | <256MB |
| Web | <200MB | <300MB | <400MB | <512MB |

### 加载时间基准

| 资源类型 | 大小 | 目标 | SSD | HDD |
|---------|------|------|-----|-----|
| 纹理 (4K) | 16MB | <2s | <1s | <2s |
| 模型 (高模) | 50MB | <3s | <2s | <3s |
| 场景 (小型) | 10MB | <1s | <0.5s | <1s |
| 场景 (中型) | 100MB | <3s | <2s | <3s |
| 场景 (大型) | 500MB | <10s | <5s | <10s |

---

## 输出格式

基准测试结果将输出为 JSON 和 Markdown 格式：

```json
{
  "timestamp": "2026-03-26T13:00:00Z",
  "platform": "Windows 11",
  "gpu": "NVIDIA RTX 4090",
  "cpu": "AMD Ryzen 9 7950X",
  "memory": "32GB DDR5",
  "tests": {
    "fps_1080p": { "avg": 144.5, "min": 120.0, "max": 165.0 },
    "fps_4k": { "avg": 60.2, "min": 45.0, "max": 75.0 },
    "memory_mb": { "peak": 456.7, "average": 389.2 }
  }
}
```

---

*Phoenix Engine Performance Benchmark Suite*
