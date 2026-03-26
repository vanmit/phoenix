# Changelog

All notable changes to Phoenix Engine will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-03-26

### 🎉 Initial Release

#### Added
- **Cross-platform rendering engine** with support for Windows, Linux, macOS, Android, iOS, and Web
- **Dual backend architecture**: C++ native + WebAssembly
- **Multi-GPU API support**: Vulkan, DirectX 12, Metal, OpenGL 4.5, WebGPU, WebGL 2.0
- **Rust security core** for memory-safe operations
  - Secure memory allocator
  - Audit logging system
  - AES-256-GCM encryption module
- **Scene management system**
  - Scene graph with spatial acceleration
  - Frustum and occlusion culling
  - LOD (Level of Detail) system
- **Resource management**
  - Asynchronous resource loading
  - Resource caching and pooling
  - Hot-reloading support
- **PBR rendering pipeline**
  - Physically-based materials
  - Image-based lighting (IBL)
  - Shadow mapping (PCF, VSM)
  - Deferred rendering path
- **Animation system**
  - Skeletal animation
  - Morph targets
  - Animation blending
- **Format support**
  - glTF 2.0 (preferred)
  - FBX
  - OBJ
  - Point clouds
  - Heightfield terrain
- **WebAssembly integration**
  - Full engine compiled to WASM
  - JavaScript/TypeScript bindings
  - React, Vue, and vanilla JS examples
- **Mobile optimizations**
  - ARM NEON optimizations
  - Power-efficient rendering
  - Touch input handling
- **Formal verification**
  - Coq proofs for critical algorithms
  - Memory safety guarantees
  - Security property verification

#### Security Features
- Memory-safe Rust core for critical operations
- Smart pointer enforcement in C++ codebase
- Comprehensive audit logging
- Encrypted asset storage
- WASM sandboxing for web deployment
- Security penetration testing completed

#### Documentation
- API reference documentation
- Architecture design documents
- Coding standards guide
- Security audit reports
- Integration guides for all platforms
- Tutorial examples

#### Development Tools
- CMake-based build system
- Cross-compilation support
- Automated testing framework
- Benchmark suite
- Profiling tools integration

### Technical Specifications

| Component | Version | License |
|-----------|---------|---------|
| Phoenix Engine Core | 1.0.0 | MIT |
| Rust Security Core | 1.0.0 | MIT |
| bgfx | 1.131 | BSD-2 |
| assimp | 5.4.0 | BSD-3 |
| glm | 1.0.1 | MIT |
| libsodium | 1.0.20 | ISC |
| wasmtime | 18.0 | Apache-2.0 |

### Platform Support

| Platform | Status | Minimum Version |
|----------|--------|-----------------|
| Windows | ✅ Ready | Windows 10 (1903+) |
| Linux | ✅ Ready | Ubuntu 20.04+ / RHEL 8+ |
| macOS | ✅ Ready | macOS 11.0+ |
| Android | ✅ Ready | Android 10+ (API 29) |
| iOS | ✅ Ready | iOS 15.0+ |
| Web | ✅ Ready | WebGPU-enabled browsers |

### Known Issues
- WebGPU support limited to Chrome/Edge 113+
- Some FBX animations may require manual adjustment
- Mobile shader compilation may cause initial stutter

### Upgrade Notes
- This is the initial release, no upgrade path required

---

## [Unreleased]

### Planned for v1.1.0
- Ray tracing support (DXR, VKRT)
- GPU-driven rendering pipeline
- Enhanced terrain system
- Particle system improvements
- VR/AR support

### Under Consideration
- DLSS/FSR integration
- Multi-GPU support
- Networked rendering
- Scripting layer (Lua/Python)

---

## Version History

| Version | Release Date | Status |
|---------|--------------|--------|
| 1.0.0 | 2026-03-26 | ✅ Released |
| 0.1.0-alpha | 2026-01-15 | ⏭️ Skipped to 1.0.0 |

---

*For detailed migration guides, see [docs/migration/](./docs/migration/)*
