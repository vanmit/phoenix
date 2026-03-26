# 性能优化第二课：批处理优化

## 🎯 学习目标
- 绘制调用批处理
- 状态排序
- 减少 GPU 状态切换

## 📝 代码示例

```cpp
// 按材质排序
std::sort(objects.begin(), objects.end(),
    [](const auto& a, const auto& b) {
        return a.material < b.material;
    });

// 批量绘制
for (auto& batch : batches) {
    renderer->draw(batch.pipeline, batch.vertices);
}
```

## 📚 下一步
**下一课**: [第三课：LOD 系统](lesson-03-lod.md)
