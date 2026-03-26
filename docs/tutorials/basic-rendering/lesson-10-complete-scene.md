# 第十课：完整场景

综合运用所有知识创建完整场景。

## 🎯 学习目标

- 整合所有渲染技术
- 创建完整 3D 场景
- 性能优化
- 最终效果

## 📝 完整示例

```cpp
#include <phoenix/phoenix.hpp>

class SceneApp {
public:
    bool initialize() {
        // 1. 创建窗口和渲染器
        window = app.createWindow(1280, 720, "Complete Scene");
        renderer = phoenix::RenderDevice::create();
        renderer->initialize(window);
        
        // 2. 创建相机
        camera = std::make_unique<phoenix::Camera>();
        camera->setPosition({0, 5, 10});
        camera->setTarget({0, 0, 0});
        
        // 3. 创建光源
        createLights();
        
        // 4. 加载模型
        loadModels();
        
        // 5. 创建材质
        createMaterials();
        
        return true;
    }
    
    void run() {
        while (app.isRunning()) {
            app.pollEvents();
            update();
            render();
        }
    }
    
private:
    void update() {
        // 更新相机
        handleInput();
        
        // 更新动画
        updateAnimations();
    }
    
    void render() {
        // 清屏
        renderer->clear({0.1f, 0.1f, 0.2f, 1.0f});
        
        // 设置相机
        shader->setUniform("view", camera->getViewMatrix());
        shader->setUniform("projection", camera->getProjectionMatrix());
        
        // 渲染所有对象
        for (auto& object : objects) {
            object.draw(renderer);
        }
        
        renderer->present();
    }
    
    void createLights() {
        // 方向光
        lights.push_back({
            .type = phoenix::LightType::Directional,
            .direction = {-0.5f, -1.0f, -0.3f},
            .color = {1.0f, 1.0f, 0.9f},
            .intensity = 1.0f
        });
        
        // 点光源
        lights.push_back({
            .type = phoenix::LightType::Point,
            .position = {0, 3, 0},
            .color = {1.0f, 0.8f, 0.6f},
            .intensity = 2.0f,
            .range = 10.0f
        });
    }
    
    void loadModels() {
        // 加载地面
        auto ground = createPlane(20, 20);
        objects.push_back(ground);
        
        // 加载一些模型
        for (int i = 0; i < 10; i++) {
            auto model = createBox();
            model->setPosition({
                (i % 5 - 2.5f) * 2.0f,
                0.5f,
                (i / 5 - 0.5f) * 2.0f
            });
            objects.push_back(model);
        }
    }
    
    phoenix::Application app;
    std::shared_ptr<phoenix::Window> window;
    std::shared_ptr<phoenix::RenderDevice> renderer;
    std::unique_ptr<phoenix::Camera> camera;
    std::vector<phoenix::Light> lights;
    std::vector<std::unique_ptr<GameObject>> objects;
};

int main() {
    SceneApp app;
    if (!app.initialize()) {
        return -1;
    }
    app.run();
    return 0;
}
```

## 🎉 恭喜完成基础渲染教程!

你现在已经掌握了：
- ✅ 窗口创建和管理
- ✅ 渲染器初始化
- ✅ 顶点和索引缓冲区
- ✅ 纹理映射
- ✅ 光照系统
- ✅ 材质系统
- ✅ 相机控制
- ✅ 完整场景渲染

## 📚 下一步

继续学习：
- [场景系统教程](../scene-system/lesson-01-scene-graph.md)
- [动画系统教程](../animation-system/lesson-01-skeletal-animation.md)
- [着色器编写教程](../shader-writing/lesson-01-shader-basics.md)
