# 第九课：相机控制

学习如何实现相机控制系统。

## 🎯 学习目标

- 理解相机矩阵
- 实现第一人称相机
- 实现轨道相机
- 相机裁剪

## 📝 代码示例

```cpp
#include <phoenix/phoenix.hpp>

int main() {
    phoenix::Application app("Camera Control");
    auto window = app.createWindow(800, 600);
    auto renderer = phoenix::RenderDevice::create();
    renderer->initialize(window);
    
    // 1. 创建相机
    auto camera = std::make_unique<phoenix::Camera>();
    camera->setPosition({0.0f, 0.0f, 5.0f});
    camera->setTarget({0.0f, 0.0f, 0.0f});
    camera->setUp({0.0f, 1.0f, 0.0f});
    camera->setFOV(45.0f);
    camera->setAspect(800.0f / 600.0f);
    camera->setNear(0.1f);
    camera->setFar(100.0f);
    
    // 2. 第一人称控制
    float yaw = 0.0f, pitch = 0.0f;
    
    app.onMouseMove([&](float dx, float dy) {
        yaw += dx * 0.1f;
        pitch += dy * 0.1f;
        pitch = std::clamp(pitch, -89.0f, 89.0f);
        
        camera->setDirection(yaw, pitch);
    });
    
    app.onKeyPress([&](int key) {
        float speed = 0.1f;
        switch (key) {
            case phoenix::Key::W:
                camera->moveForward(speed);
                break;
            case phoenix::Key::S:
                camera->moveBackward(speed);
                break;
            case phoenix::Key::A:
                camera->moveLeft(speed);
                break;
            case phoenix::Key::D:
                camera->moveRight(speed);
                break;
        }
    });
    
    // 3. 主循环
    while (app.isRunning()) {
        app.pollEvents();
        
        renderer->clear({0.1f, 0.1f, 0.2f, 1.0f});
        
        // 更新相机矩阵
        auto viewMatrix = camera->getViewMatrix();
        auto projMatrix = camera->getProjectionMatrix();
        
        // 传递给着色器
        shader->setUniform("viewMatrix", viewMatrix);
        shader->setUniform("projMatrix", projMatrix);
        
        // 渲染场景
        renderer->draw(pipeline, 0, vertexCount);
        renderer->present();
    }
    
    return 0;
}
```

## 📚 下一步

**下一课**: [第十课：完整场景](lesson-10-complete-scene.md)
