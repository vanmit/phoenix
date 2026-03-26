# 动画系统第二课：关键帧动画

## 🎯 学习目标
- 关键帧概念
- 插值计算
- 动画播放

## 📝 代码示例

```cpp
auto animation = model->getAnimation("Walk");
animation->play();
animation->setTime(time);

// 插值
auto pose = animation->sample(time);
```

## 📚 下一步
**下一课**: [第三课：形态动画](lesson-03-morph-animation.md)
