# 场景系统第一课：场景图基础

## 🎯 学习目标
- 理解场景图概念
- 创建场景
- 节点层次结构

## 📝 代码示例

```cpp
#include <phoenix/phoenix.hpp>

int main() {
    phoenix::Application app("Scene Graph");
    auto scene = std::make_unique<phoenix::Scene>();
    
    // 创建根节点
    auto root = scene->createNode("Root");
    
    // 创建子节点
    auto player = root->createChild("Player");
    player->setPosition({0, 0, 0});
    
    auto weapon = player->createChild("Weapon");
    weapon->setPosition({0.5f, 0, 0.5f});
    
    // 场景图:
    // Root
    // └── Player
    //     └── Weapon
    
    return 0;
}
```

## 📚 下一步
**下一课**: [第二课：场景节点](lesson-02-scene-nodes.md)
