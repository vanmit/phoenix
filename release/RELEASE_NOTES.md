# Phoenix Engine v1.0.0 - Release Notes

**Release Date:** March 26, 2026  
**Version:** 1.0.0  
**License:** MIT

---

## 🎉 Welcome to Phoenix Engine!

We're thrilled to announce the initial release of Phoenix Engine, a cross-platform 3D rendering engine built with security and performance in mind.

## ✨ What's New

### Core Features

- **Cross-Platform Support**: Windows, Linux, macOS, Android, iOS, and Web
- **Multi-API Rendering**: Vulkan, DirectX 12, Metal, OpenGL, WebGPU, WebGL 2
- **Rust Security Core**: Memory-safe operations with audit logging
- **PBR Rendering**: Physically-based materials and lighting
- **Scene Management**: Full scene graph with spatial acceleration
- **Animation System**: Skeletal animation and morph targets
- **Resource System**: Async loading with hot-reload support

### Platform Packages

This release includes pre-built packages for:

| Platform | Package | Size |
|----------|---------|------|
| Windows | `phoenix-engine-1.0.0-windows-x64.zip` | ~50 MB |
| Linux (DEB) | `phoenix-engine_1.0.0-1_amd64.deb` | ~45 MB |
| Linux (AppImage) | `PhoenixEngine-1.0.0.AppImage` | ~55 MB |
| macOS | `PhoenixEngine-1.0.0.dmg` | ~60 MB |
| Android | `phoenix-engine-1.0.0.apk` | ~35 MB |
| Android (Play) | `phoenix-engine-1.0.0.aab` | ~30 MB |
| iOS | `PhoenixEngine-1.0.0.ipa` | ~40 MB |
| Web (NPM) | `phoenix-engine-1.0.0.tgz` | ~25 MB |
| Source | `phoenix-engine-1.0.0-source.tar.gz` | ~15 MB |

## 📦 Installation

### Windows

```powershell
# Download and extract
Invoke-WebRequest -Uri "https://github.com/phoenix-engine/phoenix-engine/releases/download/v1.0.0/phoenix-engine-1.0.0-windows-x64.zip" -OutFile phoenix-engine.zip
Expand-Archive phoenix-engine.zip -DestinationPath C:\PhoenixEngine

# Run demo
C:\PhoenixEngine\bin\demo.exe
```

### Linux (DEB)

```bash
# Download and install
wget https://github.com/phoenix-engine/phoenix-engine/releases/download/v1.0.0/phoenix-engine_1.0.0-1_amd64.deb
sudo apt install ./phoenix-engine_1.0.0-1_amd64.deb

# Run demo
phoenix-engine-demo
```

### Linux (AppImage)

```bash
# Download and make executable
wget https://github.com/phoenix-engine/phoenix-engine/releases/download/v1.0.0/PhoenixEngine-1.0.0.AppImage
chmod +x PhoenixEngine-1.0.0.AppImage

# Run
./PhoenixEngine-1.0.0.AppImage
```

### macOS

```bash
# Download and mount
curl -LO https://github.com/phoenix-engine/phoenix-engine/releases/download/v1.0.0/PhoenixEngine-1.0.0.dmg
open PhoenixEngine-1.0.0.dmg

# Drag to Applications folder
```

### Android

Download the APK from the releases page and install on your device.

### iOS

Available via TestFlight or direct IPA installation (enterprise).

### Web/NPM

```bash
npm install phoenix-engine
```

### From Source

```bash
git clone https://github.com/phoenix-engine/phoenix-engine.git
cd phoenix-engine
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

## 📖 Documentation

- [Getting Started Guide](https://phoenix-engine.github.io/docs/getting-started)
- [API Reference](https://phoenix-engine.github.io/api)
- [Tutorials](https://phoenix-engine.github.io/tutorials)
- [Examples](https://github.com/phoenix-engine/phoenix-engine/tree/main/examples)

## 🔧 Known Issues

### Windows
- Some integrated GPUs may have Vulkan driver issues

### Linux
- Wayland support is experimental

### macOS
- Metal backend requires macOS 11.0+

### Web
- WebGPU only available in Chrome/Edge 113+

See the [issue tracker](https://github.com/phoenix-engine/phoenix-engine/issues) for a complete list.

## 🛡️ Security

This release includes our Rust-based security core with:
- Memory-safe allocations
- AES-256-GCM encryption
- Comprehensive audit logging
- Formally verified critical algorithms

## 🤝 Contributing

We welcome contributions! Please see our [Contributing Guide](CONTRIBUTING.md) for details.

## 📄 License

Phoenix Engine is released under the MIT License. See [LICENSE](LICENSE) for details.

Third-party libraries are licensed under their respective licenses.

## 🙏 Acknowledgments

Phoenix Engine builds upon the excellent work of:
- [bgfx](https://github.com/bkaradzic/bgfx) - Cross-platform rendering library
- [assimp](https://github.com/assimp/assimp) - Asset import library
- [glm](https://github.com/g-truc/glm) - OpenGL Mathematics
- [libsodium](https://libsodium.org/) - Cryptography library
- [wasmtime](https://wasmtime.dev/) - WebAssembly runtime

## 📞 Support

- **Documentation**: https://phoenix-engine.github.io
- **Issues**: https://github.com/phoenix-engine/phoenix-engine/issues
- **Discussions**: https://github.com/phoenix-engine/phoenix-engine/discussions
- **Email**: phoenix-engine-maintainers@example.com

## 🔮 What's Next

We're already working on v1.1.0 with:
- Ray tracing support
- GPU-driven rendering
- Enhanced terrain system
- VR/AR support

Stay tuned!

---

**Happy Rendering!** 🦄

The Phoenix Engine Team
