# 性能优化第一课：性能分析基础

## 🎯 学习目标
- 性能分析工具
- 帧时间分析
- 瓶颈识别

## 📝 代码示例

```cpp
// 启用性能分析
phoenix::Profiler::enable(true);

// 获取统计
auto stats = renderer->getStats();
std::cout << "FPS: " << stats.fps << std::endl;
std::cout << "Draw Calls: " << stats.drawCalls << std::endl;
```

## 📚 下一步
**下一课**: [第二课：批处理优化](lesson-02-batching.md)
