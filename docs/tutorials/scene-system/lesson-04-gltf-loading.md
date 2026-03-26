# 场景系统第四课：GLTF 加载

## 🎯 学习目标
- GLTF 格式
- 加载模型
- 材质和纹理

## 📝 代码示例

```cpp
phoenix::GLTFLoader loader;
auto model = loader.load("models/character.glb");

scene->addNode(model);
```

## 📚 下一步
**下一课**: [第五课：场景管理](lesson-05-scene-management.md)
