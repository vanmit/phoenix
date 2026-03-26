# 动画系统第五课：动画混合

## 🎯 学习目标
- 1D 混合
- 2D 混合
- 添加动画

## 📝 代码示例

```cpp
// 1D 混合
blendTree->set1DBlend("Locomotion", "Speed");
blendTree->addMotion("Walk", 0.0f);
blendTree->addMotion("Run", 5.0f);

// 混合权重
auto pose = blendTree->evaluate();
```

## 🎉 完成动画系统教程!
