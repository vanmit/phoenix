# 动画系统第四课：动画状态机

## 🎯 学习目标
- 状态机设计
- 状态过渡
- 参数控制

## 📝 代码示例

```cpp
auto stateMachine = model->getStateMachine();
stateMachine->setState("Idle");
stateMachine->setFloat("Speed", 0.0f);

// 过渡
if (speed > 0.1f) {
    stateMachine->setState("Walk");
}
```

## 📚 下一步
**下一课**: [第五课：动画混合](lesson-05-animation-blending.md)
