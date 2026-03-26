# 动画系统第一课：骨骼动画基础

## 🎯 学习目标
- 理解骨骼系统
- 加载骨骼
- 骨骼变换

## 📝 代码示例

```cpp
auto skeleton = model->getSkeleton();

// 获取骨骼变换
auto transforms = skeleton->getBoneTransforms();

// 更新骨骼
skeleton->update(deltaTime);
```

## 📚 下一步
**下一课**: [第二课：关键帧动画](lesson-02-keyframe-animation.md)
