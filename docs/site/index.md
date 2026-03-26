---
layout: home
hero:
  name: Phoenix Engine
  text: Cross-platform 3D Rendering Engine
  tagline: High-performance, security-first rendering for Windows, Linux, macOS, Android, iOS, and Web
  image:
    src: /hero.png
    alt: Phoenix Engine Logo
  actions:
    - theme: brand
      text: Get Started
      link: /guide/introduction
    - theme: alt
      text: View on GitHub
      link: https://github.com/phoenix-engine/phoenix-engine
    - theme: alt
      text: API Reference
      link: /api/overview

features:
  - icon: 🚀
    title: Cross-Platform
    details: Native support for Windows, Linux, macOS, Android, iOS, and Web with a single codebase.
  - icon: 🎨
    title: Multi-API Rendering
    details: Vulkan, DirectX 12, Metal, OpenGL, WebGPU, and WebGL 2 - automatically selected for best performance.
  - icon: 🛡️
    title: Security First
    details: Rust-based security core with memory safety, encryption, and comprehensive audit logging.
  - icon: ⚡
    title: High Performance
    details: Optimized rendering pipeline with PBR, shadows, and advanced lighting techniques.
  - icon: 🎮
    title: Full Feature Set
    details: Scene graph, animation system, resource management, and support for glTF, FBX, and more.
  - icon: 🌐
    title: WebAssembly Ready
    details: Full engine compiled to WASM with JavaScript/TypeScript bindings for web integration.
---

## Quick Example

```cpp
#include <phoenix/engine.h>

int main() {
    // Initialize engine
    phoenix::EngineConfig config;
    config.window_title = "My Application";
    config.window_width = 1920;
    config.window_height = 1080;
    
    auto engine = phoenix::Engine::create(config);
    
    // Load a model
    auto model = engine->load_model("assets/character.glb");
    
    // Create a light
    auto light = engine->create_directional_light();
    light->set_intensity(1.0f);
    light->set_direction({0, -1, 0.5f});
    
    // Main loop
    while (engine->is_running()) {
        engine->update();
        engine->render();
    }
    
    return 0;
}
```

## Installation

### Windows

```powershell
# Download the installer
winget install PhoenixEngine
```

### Linux

```bash
# Ubuntu/Debian
sudo apt install phoenix-engine

# Or use AppImage
wget https://github.com/phoenix-engine/phoenix-engine/releases/latest/download/PhoenixEngine.AppImage
chmod +x PhoenixEngine.AppImage
```

### macOS

```bash
brew install phoenix-engine
```

### Web

```bash
npm install phoenix-engine
```

## Why Phoenix Engine?

### 🏗️ Modern Architecture

Built from the ground up with modern C++17/20 and Rust, Phoenix Engine features a clean, modular architecture that's easy to understand and extend.

### 🔒 Security You Can Trust

Our unique Rust-based security core provides memory safety guarantees for critical operations, while comprehensive audit logging ensures you can track all engine activities.

### 🎯 Production Ready

Used in production environments, Phoenix Engine has been battle-tested with demanding applications requiring both performance and security.

### 📚 Comprehensive Documentation

Extensive documentation, tutorials, and examples help you get up and running quickly. Our API reference is always up to date.

## Community

Join our growing community of developers:

- 💬 [Discord Server](https://discord.gg/phoenix-engine)
- 🐦 [Twitter](https://twitter.com/phoenixengine)
- 📺 [YouTube Tutorials](https://youtube.com/@phoenixengine)
- 💼 [LinkedIn](https://linkedin.com/company/phoenix-engine)

## Sponsors

Phoenix Engine is made possible by our generous sponsors:

[Sponsor logos would go here]

[Become a Sponsor](https://github.com/sponsors/phoenix-engine)

---

Ready to get started? [Check out our getting started guide](/guide/introduction)!
