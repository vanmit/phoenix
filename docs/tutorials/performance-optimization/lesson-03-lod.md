# 性能优化第三课：LOD 系统

## 🎯 学习目标
- LOD 概念
- 距离计算
- 无缝过渡

## 📝 代码示例

```cpp
float distance = camera.distanceTo(object.position);

if (distance > 100.0f) {
    object.setLOD(2);  // 低细节
} else if (distance > 50.0f) {
    object.setLOD(1);  // 中细节
} else {
    object.setLOD(0);  // 高细节
}
```

## 📚 下一步
**下一课**: [第四课：内存优化](lesson-04-memory.md)
