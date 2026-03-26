# 场景系统第五课：场景管理

## 🎯 学习目标
- 场景优化
- 剔除系统
- 多场景管理

## 📝 代码示例

```cpp
// 视锥体剔除
auto visible = scene->cull(camera.getFrustum());

// 多场景
phoenix::SceneManager manager;
manager.loadScene("level1");
manager.loadScene("level2");
```

## 🎉 完成场景系统教程!
