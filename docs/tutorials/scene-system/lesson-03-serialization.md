# 场景系统第三课：场景序列化

## 🎯 学习目标
- 保存场景
- 加载场景
- 场景格式

## 📝 代码示例

```cpp
// 保存场景
phoenix::SceneSerializer serializer;
serializer.save(scene, "scene.json");

// 加载场景
auto loadedScene = serializer.load("scene.json");
```

## 📚 下一步
**下一课**: [第四课：GLTF 加载](lesson-04-gltf-loading.md)
