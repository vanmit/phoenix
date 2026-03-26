# 场景系统第二课：场景节点

## 🎯 学习目标
- 节点变换
- 节点组件
- 节点遍历

## 📝 代码示例

```cpp
auto node = scene->createNode("Node");

// 变换
node->setPosition({1, 2, 3});
node->setRotation({0, 45, 0});
node->setScale({2, 2, 2});

// 添加组件
node->addComponent<Mesh>(mesh);
node->addComponent<Light>(light);

// 遍历
scene->traverse([](phoenix::SceneNode& node) {
    if (node.hasComponent<Mesh>()) {
        renderer->draw(node.get<Mesh>());
    }
});
```

## 📚 下一步
**下一课**: [第三课：场景序列化](lesson-03-serialization.md)
