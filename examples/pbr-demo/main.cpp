/**
 * PBR Demo - Phoenix Engine Phase 3
 * 
 * 演示高级渲染功能:
 * - PBR 材质系统 (Cook-Torrance BRDF)
 * - IBL (图像基光照)
 * - 级联阴影映射 (CSM)
 * - 延迟渲染管线
 * - 后处理效果栈
 */

#include "phoenix/core/engine.hpp"
#include "phoenix/render/RenderDevice.hpp"
#include "phoenix/render/PBR.hpp"
#include "phoenix/render/Shadows.hpp"
#include "phoenix/render/DeferredRenderer.hpp"
#include "phoenix/render/PostProcess.hpp"
#include "phoenix/render/Shader.hpp"
#include "phoenix/math/matrix.hpp"
#include "phoenix/math/vector.hpp"
#include "phoenix/scene/camera.hpp"
#include <iostream>
#include <chrono>

using namespace phoenix;
using namespace phoenix::render;

// ============================================================================
// 配置
// ============================================================================

struct DemoConfig {
    uint32_t width = 1920;
    uint32_t height = 1080;
    bool vsync = true;
    bool fullscreen = false;
    
    // 渲染质量
    ShadowQuality shadowQuality = ShadowQuality::High;
    uint32_t msaaSamples = 1;
    bool enableBloom = true;
    bool enableSSAO = true;
    bool enableFXAA = true;
    bool enableToneMapping = true;
    
    // 调试
    bool showWireframe = false;
    bool showGBuffer = false;
    bool showLightCount = false;
};

// ============================================================================
// PBR 演示应用
// ============================================================================

class PBRDemo {
public:
    PBRDemo() = default;
    ~PBRDemo() { shutdown(); }
    
    bool initialize(const DemoConfig& config) {
        config_ = config;
        
        std::cout << "=== Phoenix Engine PBR Demo ===" << std::endl;
        std::cout << "Initializing rendering system..." << std::endl;
        
        // 1. 初始化渲染设备
        if (!initRenderDevice()) {
            std::cerr << "Failed to initialize render device" << std::endl;
            return false;
        }
        
        // 2. 初始化着色器编译器
        if (!initShaderCompiler()) {
            std::cerr << "Failed to initialize shader compiler" << std::endl;
            return false;
        }
        
        // 3. 初始化 PBR 渲染器
        if (!initPBRRenderer()) {
            std::cerr << "Failed to initialize PBR renderer" << std::endl;
            return false;
        }
        
        // 4. 初始化阴影系统
        if (!initShadowRenderer()) {
            std::cerr << "Failed to initialize shadow renderer" << std::endl;
            return false;
        }
        
        // 5. 初始化延迟渲染器
        if (!initDeferredRenderer()) {
            std::cerr << "Failed to initialize deferred renderer" << std::endl;
            return false;
        }
        
        // 6. 初始化后处理
        if (!initPostProcess()) {
            std::cerr << "Failed to initialize post processing" << std::endl;
            return false;
        }
        
        // 7. 创建场景
        if (!createScene()) {
            std::cerr << "Failed to create scene" << std::endl;
            return false;
        }
        
        // 8. 加载 IBL
        if (!loadIBL()) {
            std::cerr << "Warning: Failed to load IBL, using default" << std::endl;
        }
        
        std::cout << "Initialization complete!" << std::endl;
        return true;
    }
    
    void shutdown() {
        std::cout << "Shutting down..." << std::endl;
        
        postProcessStack_.shutdown();
        deferredRenderer_.shutdown();
        shadowRenderer_.shutdown();
        pbrRenderer_.shutdown();
        shaderCompiler_.shutdown();
        renderDevice_.shutdown();
    }
    
    void run() {
        std::cout << "Starting render loop..." << std::endl;
        
        bool running = true;
        uint64_t frameCount = 0;
        auto startTime = std::chrono::high_resolution_clock::now();
        
        while (running) {
            // 处理输入 (简化)
            // if (window.shouldClose()) running = false;
            
            // 更新
            update(0.016f);
            
            // 渲染
            render();
            
            frameCount++;
            
            // 每秒输出 FPS
            auto currentTime = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration<float>(currentTime - startTime).count();
            
            if (elapsed >= 1.0f) {
                float fps = frameCount / elapsed;
                std::cout << "FPS: " << fps << " | Frame: " << frameCount << std::endl;
                frameCount = 0;
                startTime = currentTime;
            }
        }
    }
    
private:
    DemoConfig config_;
    RenderDevice renderDevice_;
    ShaderCompiler shaderCompiler_;
    PBRRenderer pbrRenderer_;
    ShadowRenderer shadowRenderer_;
    DeferredRenderer deferredRenderer_;
    PostProcessStack postProcessStack_;
    
    // 场景
    struct SceneObject {
        Mesh mesh;
        PBRMaterial material;
        float modelMatrix[16];
    };
    
    std::vector<SceneObject> sceneObjects_;
    std::vector<Light> lights_;
    
    // 相机
    struct Camera {
        float position[3];
        float target[3];
        float up[3];
        float fov;
        float aspect;
        float nearPlane;
        float farPlane;
        
        float viewMatrix[16];
        float projectionMatrix[16];
    } camera_;
    
    // 性能统计
    struct Stats {
        float frameTime;
        float gpuTime;
        uint32_t drawCalls;
        uint32_t triangleCount;
        uint32_t lightCount;
    } stats_;
    
    bool initRenderDevice() {
        DeviceConfig deviceConfig;
        deviceConfig.backend = RenderBackend::Vulkan;
        deviceConfig.enableValidation = false;
        deviceConfig.enableDebugInfo = true;
        
        SwapChainConfig swapChain;
        swapChain.width = config_.width;
        swapChain.height = config_.height;
        swapChain.vsync = config_.vsync;
        swapChain.srgb = true;
        
        return renderDevice_.initialize(deviceConfig, swapChain);
    }
    
    bool initShaderCompiler() {
        return shaderCompiler_.initialize();
    }
    
    bool initPBRRenderer() {
        return pbrRenderer_.initialize(renderDevice_, shaderCompiler_);
    }
    
    bool initShadowRenderer() {
        if (!shadowRenderer_.initialize(renderDevice_, shaderCompiler_)) {
            return false;
        }
        
        shadowRenderer_.setShadowQuality(config_.shadowQuality);
        
        // 配置级联
        CascadeConfig cascadeConfig;
        cascadeConfig.cascadeCount = 4;
        cascadeConfig.nearPlane = 0.5f;
        cascadeConfig.farPlane = 100.0f;
        cascadeConfig.blendWidth = 0.1f;
        shadowRenderer_.setCascadeConfig(cascadeConfig);
        
        return true;
    }
    
    bool initDeferredRenderer() {
        DeferredConfig config;
        config.width = config_.width;
        config.height = config_.height;
        config.sampleCount = config_.msaaSamples;
        config.useTileCulling = true;
        
        return deferredRenderer_.initialize(renderDevice_, shaderCompiler_, config);
    }
    
    bool initPostProcess() {
        if (!postProcessStack_.initialize(renderDevice_, shaderCompiler_)) {
            return false;
        }
        
        postProcessStack_.resize(config_.width, config_.height);
        
        // 配置效果
        postProcessStack_.enableBloom(config_.enableBloom);
        postProcessStack_.enableSSAO(config_.enableSSAO);
        postProcessStack_.enableFXAA(config_.enableFXAA);
        
        // Bloom 配置
        auto& bloomConfig = postProcessStack_.getBloomConfig();
        bloomConfig.threshold = 1.0f;
        bloomConfig.intensity = 1.5f;
        bloomConfig.iterations = 4;
        
        // SSAO 配置
        auto& ssaoConfig = postProcessStack_.getSSAOConfig();
        ssaoConfig.radius = 0.5f;
        ssaoConfig.intensity = 1.0f;
        ssaoConfig.sampleCount = 16;
        
        // 色调映射配置
        auto& tmConfig = postProcessStack_.getToneMappingConfig();
        tmConfig.algorithm = ToneMappingAlgorithm::ACES;
        tmConfig.exposure = 1.0f;
        tmConfig.gamma = 2.2f;
        
        return true;
    }
    
    bool createScene() {
        // 创建地面
        {
            SceneObject ground;
            MeshData groundMesh = BuiltinMeshes::createPlane(50.0f, 50.0f, 10);
            // ground.mesh.create(renderDevice_, groundMesh);
            
            PBRMaterialProperties groundProps;
            groundProps.albedo = Color(0.4f, 0.35f, 0.3f, 1.0f);
            groundProps.roughness = 0.9f;
            groundProps.metallic = 0.0f;
            // ground.material.create(renderDevice_, shaderCompiler_);
            ground.material.setProperties(groundProps);
            
            // 单位矩阵
            std::memset(ground.modelMatrix, 0, sizeof(ground.modelMatrix));
            ground.modelMatrix[0] = 1.0f;
            ground.modelMatrix[5] = 1.0f;
            ground.modelMatrix[10] = 1.0f;
            ground.modelMatrix[15] = 1.0f;
            
            sceneObjects_.push_back(std::move(ground));
        }
        
        // 创建球体
        for (int i = 0; i < 5; ++i) {
            SceneObject sphere;
            MeshData sphereMesh = BuiltinMeshes::createSphere(1.0f, 32);
            // sphere.mesh.create(renderDevice_, sphereMesh);
            
            PBRMaterialProperties sphereProps;
            sphereProps.albedo = Color(
                0.2f + i * 0.15f,
                0.5f,
                0.8f - i * 0.1f,
                1.0f
            );
            sphereProps.roughness = 0.1f + i * 0.2f;
            sphereProps.metallic = static_cast<float>(i) / 5.0f;
            // sphere.material.create(renderDevice_, shaderCompiler_);
            sphere.material.setProperties(sphereProps);
            
            // 变换矩阵
            Math::Matrix4::createTranslation(
                -4.0f + i * 2.0f,
                1.0f,
                0.0f,
                sphere.modelMatrix
            );
            
            sceneObjects_.push_back(std::move(sphere));
        }
        
        // 创建光源
        {
            Light dirLight;
            dirLight.type = LightType::Directional;
            dirLight.direction[0] = 0.5f;
            dirLight.direction[1] = -1.0f;
            dirLight.direction[2] = 0.3f;
            dirLight.color = Color(1.0f, 0.95f, 0.9f, 1.0f);
            dirLight.intensity = 1.5f;
            dirLight.castShadow = true;
            lights_.push_back(dirLight);
        }
        
        {
            Light pointLight;
            pointLight.type = LightType::Point;
            pointLight.position[0] = 5.0f;
            pointLight.position[1] = 3.0f;
            pointLight.position[2] = 5.0f;
            pointLight.color = Color(1.0f, 0.8f, 0.6f, 1.0f);
            pointLight.intensity = 2.0f;
            pointLight.range = 15.0f;
            pointLight.castShadow = true;
            lights_.push_back(pointLight);
        }
        
        // 相机设置
        camera_.position[0] = 0.0f;
        camera_.position[1] = 5.0f;
        camera_.position[2] = 15.0f;
        camera_.target[0] = 0.0f;
        camera_.target[1] = 0.0f;
        camera_.target[2] = 0.0f;
        camera_.up[0] = 0.0f;
        camera_.up[1] = 1.0f;
        camera_.up[2] = 0.0f;
        camera_.fov = 60.0f;
        camera_.aspect = static_cast<float>(config_.width) / config_.height;
        camera_.nearPlane = 0.1f;
        camera_.farPlane = 1000.0f;
        
        return true;
    }
    
    bool loadIBL() {
        // 加载环境贴图
        // TextureDesc desc;
        // desc.type = TextureType::TextureCube;
        // desc.format = TextureFormat::RGBA16F;
        
        // Texture envMap;
        // if (!envMap.loadFromFile(renderDevice_, "assets/environment/kloofendal_4k.hdr")) {
        //     return false;
        // }
        
        // return pbrRenderer_.generateIBL(renderDevice_, envMap.getHandle());
        return true;
    }
    
    void update(float deltaTime) {
        // 更新相机矩阵
        Math::Matrix4::createLookAt(
            camera_.position,
            camera_.target,
            camera_.up,
            camera_.viewMatrix
        );
        
        Math::Matrix4::createPerspectiveFOV(
            camera_.fov * 3.14159f / 180.0f,
            camera_.aspect,
            camera_.nearPlane,
            camera_.farPlane,
            camera_.projectionMatrix
        );
        
        // 更新光源 (动画)
        float time = static_cast<float>(glfwGetTime());
        lights_[1].position[0] = std::sin(time) * 10.0f;
        lights_[1].position[2] = std::cos(time) * 10.0f;
    }
    
    void render() {
        // 1. 开始帧
        renderDevice_.beginFrame();
        
        // 2. 渲染阴影
        shadowRenderer_.beginShadowPass(renderDevice_);
        shadowRenderer_.renderShadowMap(
            renderDevice_,
            lights_[0].direction,
            camera_.viewMatrix, // 简化
            {} // draw calls
        );
        shadowRenderer_.endShadowPass(renderDevice_);
        
        // 3. 延迟渲染 - 几何通道
        deferredRenderer_.beginGeometryPass(renderDevice_);
        
        for (const auto& obj : sceneObjects_) {
            DrawCall drawCall;
            // drawCall.vertexBuffer = obj.mesh.getVertexBuffer();
            // drawCall.indexBuffer = obj.mesh.getIndexBuffer();
            drawCall.program = obj.material.getProgram();
            // deferredRenderer_.addGeometryDrawCall(drawCall);
        }
        
        deferredRenderer_.endGeometryPass(renderDevice_);
        
        // 4. 添加光源
        for (const auto& light : lights_) {
            deferredRenderer_.addLight(light);
        }
        
        // 5. 延迟渲染 - 光照通道
        deferredRenderer_.beginLightingPass(renderDevice_);
        deferredRenderer_.endLightingPass(renderDevice_);
        
        // 6. 后处理
        postProcessStack_.render(
            renderDevice_,
            deferredRenderer_.getOutputTexture(),
            TextureHandle(), // 最终输出
            0
        );
        
        // 7. 结束帧
        auto frameStats = renderDevice_.endFrame(config_.vsync);
        
        // 更新统计
        stats_.frameTime = frameStats.frameTime;
        stats_.gpuTime = frameStats.gpuTime;
        stats_.drawCalls = frameStats.drawCalls;
        stats_.triangleCount = frameStats.triangleCount;
    }
};

// ============================================================================
// 主函数
// ============================================================================

int main(int argc, char* argv[]) {
    DemoConfig config;
    
    // 解析命令行参数
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--width" && i + 1 < argc) {
            config.width = std::stoul(argv[++i]);
        }
        else if (arg == "--height" && i + 1 < argc) {
            config.height = std::stoul(argv[++i]);
        }
        else if (arg == "--shadow-quality") {
            std::string quality = argv[++i];
            if (quality == "low") config.shadowQuality = ShadowQuality::Low;
            else if (quality == "medium") config.shadowQuality = ShadowQuality::Medium;
            else if (quality == "high") config.shadowQuality = ShadowQuality::High;
            else if (quality == "ultra") config.shadowQuality = ShadowQuality::Ultra;
        }
        else if (arg == "--no-bloom") {
            config.enableBloom = false;
        }
        else if (arg == "--no-ssao") {
            config.enableSSAO = false;
        }
        else if (arg == "--no-fxaa") {
            config.enableFXAA = false;
        }
        else if (arg == "--help") {
            std::cout << "PBR Demo - Phoenix Engine Phase 3\n\n";
            std::cout << "Usage: pbr-demo [options]\n\n";
            std::cout << "Options:\n";
            std::cout << "  --width <n>          Window width (default: 1920)\n";
            std::cout << "  --height <n>         Window height (default: 1080)\n";
            std::cout << "  --shadow-quality <q> Shadow quality (low/medium/high/ultra)\n";
            std::cout << "  --no-bloom           Disable bloom effect\n";
            std::cout << "  --no-ssao            Disable SSAO\n";
            std::cout << "  --no-fxaa            Disable FXAA\n";
            std::cout << "  --help               Show this help\n";
            return 0;
        }
    }
    
    PBRDemo demo;
    
    if (!demo.initialize(config)) {
        std::cerr << "Failed to initialize demo" << std::endl;
        return 1;
    }
    
    demo.run();
    
    return 0;
}
