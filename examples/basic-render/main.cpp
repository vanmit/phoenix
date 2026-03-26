/**
 * @file main.cpp
 * @brief Phoenix Engine 基础渲染示例
 * 
 * 演示如何使用 Phoenix Engine 渲染系统:
 * - 初始化渲染设备
 * - 创建几何体 (三角形/立方体)
 * - 设置着色器程序
 * - 提交绘制调用
 */

#include "phoenix/render/RenderDevice.hpp"
#include "phoenix/render/Shader.hpp"
#include "phoenix/render/Pipeline.hpp"
#include "phoenix/render/Resources.hpp"
#include <iostream>
#include <chrono>
#include <thread>

using namespace phoenix::render;

/**
 * @brief 示例应用类
 */
class BasicRenderExample {
public:
    BasicRenderExample() = default;
    ~BasicRenderExample() {
        shutdown();
    }

    /**
     * @brief 初始化示例
     */
    bool initialize(uint32_t width = 1280, uint32_t height = 720) {
        std::cout << "Initializing Phoenix Engine Render System...\n";
        
        // 1. 初始化渲染设备
        DeviceConfig deviceConfig;
        deviceConfig.backend = RenderBackend::Vulkan;
        deviceConfig.enableDebugInfo = true;
        deviceConfig.enableValidation = false;
        
        SwapChainConfig swapChain;
        swapChain.width = width;
        swapChain.height = height;
        swapChain.windowHandle = nullptr; // 无窗口模式用于测试
        swapChain.vsync = true;
        
        if (!device_.initialize(deviceConfig, swapChain)) {
            std::cerr << "Failed to initialize render device\n";
            return false;
        }
        
        std::cout << "  Render backend: " << device_.getDeviceInfo().backendName << "\n";
        std::cout << "  Device: " << device_.getDeviceInfo().device << "\n";
        
        // 2. 初始化着色器编译器
        if (!shaderCompiler_.initialize()) {
            std::cerr << "Failed to initialize shader compiler\n";
            return false;
        }
        
        // 3. 初始化资源管理器
        resourceManager_.initialize(device_);
        
        // 4. 创建几何体
        if (!createGeometry()) {
            return false;
        }
        
        // 5. 创建着色器程序
        if (!createShaders()) {
            return false;
        }
        
        // 6. 初始化渲染器
        renderer_.initialize(device_);
        
        std::cout << "Initialization complete!\n\n";
        return true;
    }

    /**
     * @brief 运行示例
     */
    void run() {
        std::cout << "Starting render loop...\n";
        std::cout << "Press Ctrl+C to exit\n\n";
        
        uint64_t frameCount = 0;
        auto startTime = std::chrono::high_resolution_clock::now();
        
        bool running = true;
        while (running && frameCount < 100) { // 运行 100 帧用于测试
            frame();
            frameCount++;
            
            // 限制帧率
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        
        std::cout << "\nRender statistics:\n";
        std::cout << "  Frames: " << frameCount << "\n";
        std::cout << "  Duration: " << duration << " ms\n";
        if (duration > 0) {
            std::cout << "  FPS: " << (frameCount * 1000.0 / duration) << "\n";
        }
    }

    /**
     * @brief 关闭示例
     */
    void shutdown() {
        std::cout << "Shutting down...\n";
        
        renderer_.shutdown();
        resourceManager_.shutdown();
        shaderCompiler_.shutdown();
        device_.shutdown();
        
        std::cout << "Shutdown complete.\n";
    }

private:
    RenderDevice device_;
    ShaderCompiler shaderCompiler_;
    ResourceManager resourceManager_;
    ForwardRenderer renderer_;
    
    // 资源
    VertexBuffer* triangleVB_ = nullptr;
    IndexBuffer* triangleIB_ = nullptr;
    VertexBuffer* cubeVB_ = nullptr;
    IndexBuffer* cubeIB_ = nullptr;
    
    ProgramHandle shaderProgram_;
    
    /**
     * @brief 创建几何体
     */
    bool createGeometry() {
        std::cout << "Creating geometry...\n";
        
        // 创建三角形
        {
            MeshData triangle = BuiltinMeshes::createTriangle();
            
            VertexLayout layout;
            layout.begin()
                .addPosition()
                .addNormal()
                .addTexCoord()
                .end();
            
            BufferDesc vbDesc;
            vbDesc.type = BufferType::Vertex;
            vbDesc.size = static_cast<uint32_t>(triangle.positions.size() * sizeof(float) +
                                                triangle.normals.size() * sizeof(float) +
                                                triangle.texCoords.size() * sizeof(float));
            vbDesc.stride = layout.getStride();
            
            // 合并顶点数据
            std::vector<float> vertices;
            for (uint32_t i = 0; i < triangle.vertexCount(); ++i) {
                vertices.insert(vertices.end(), 
                    triangle.positions.begin() + i * 3, 
                    triangle.positions.begin() + i * 3 + 3);
                vertices.insert(vertices.end(),
                    triangle.normals.begin() + i * 3,
                    triangle.normals.begin() + i * 3 + 3);
                vertices.insert(vertices.end(),
                    triangle.texCoords.begin() + i * 2,
                    triangle.texCoords.begin() + i * 2 + 2);
            }
            
            vbDesc.data = vertices.data();
            
            triangleVB_ = resourceManager_.createVertexBuffer(vbDesc, layout);
            if (!triangleVB_) {
                std::cerr << "  Failed to create triangle vertex buffer\n";
                return false;
            }
            
            BufferDesc ibDesc;
            ibDesc.type = BufferType::Index32;
            ibDesc.size = static_cast<uint32_t>(triangle.indices.size() * sizeof(uint32_t));
            ibDesc.data = triangle.indices.data();
            
            triangleIB_ = resourceManager_.createIndexBuffer(ibDesc);
            if (!triangleIB_) {
                std::cerr << "  Failed to create triangle index buffer\n";
                return false;
            }
            
            std::cout << "  Triangle: " << triangle.vertexCount() << " vertices, "
                      << triangle.indexCount() << " indices\n";
        }
        
        // 创建立方体
        {
            MeshData cube = BuiltinMeshes::createCube(1.0f);
            
            VertexLayout layout;
            layout.begin()
                .addPosition()
                .addNormal()
                .addTexCoord()
                .end();
            
            BufferDesc vbDesc;
            vbDesc.type = BufferType::Vertex;
            vbDesc.stride = layout.getStride();
            
            // 合并顶点数据
            std::vector<float> vertices;
            for (uint32_t i = 0; i < cube.vertexCount(); ++i) {
                vertices.insert(vertices.end(),
                    cube.positions.begin() + i * 3,
                    cube.positions.begin() + i * 3 + 3);
                vertices.insert(vertices.end(),
                    cube.normals.begin() + i * 3,
                    cube.normals.begin() + i * 3 + 3);
                vertices.insert(vertices.end(),
                    cube.texCoords.begin() + i * 2,
                    cube.texCoords.begin() + i * 2 + 2);
            }
            
            vbDesc.size = static_cast<uint32_t>(vertices.size() * sizeof(float));
            vbDesc.data = vertices.data();
            
            cubeVB_ = resourceManager_.createVertexBuffer(vbDesc, layout);
            if (!cubeVB_) {
                std::cerr << "  Failed to create cube vertex buffer\n";
                return false;
            }
            
            BufferDesc ibDesc;
            ibDesc.type = BufferType::Index32;
            ibDesc.size = static_cast<uint32_t>(cube.indices.size() * sizeof(uint32_t));
            ibDesc.data = cube.indices.data();
            
            cubeIB_ = resourceManager_.createIndexBuffer(ibDesc);
            if (!cubeIB_) {
                std::cerr << "  Failed to create cube index buffer\n";
                return false;
            }
            
            std::cout << "  Cube: " << cube.vertexCount() << " vertices, "
                      << cube.indexCount() << " indices\n";
        }
        
        return true;
    }

    /**
     * @brief 创建着色器
     */
    bool createShaders() {
        std::cout << "Creating shaders...\n";
        
        // 简化示例：使用预定义的程序句柄
        // 实际使用需要从文件加载或编译 SPIR-V
        
        // 创建简单的顶点着色器 (GLSL)
        const char* vsSource = R"(
#version 450
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texcoord;

layout(location = 0) out vec3 v_normal;
layout(location = 1) out vec2 v_texcoord;

layout(std140, binding = 0) uniform Uniforms {
    mat4 u_model;
    mat4 u_view;
    mat4 u_projection;
};

void main() {
    v_normal = a_normal;
    v_texcoord = a_texcoord;
    gl_Position = u_projection * u_view * u_model * vec4(a_position, 1.0);
}
)";
        
        // 创建简单的片段着色器 (GLSL)
        const char* fsSource = R"(
#version 450
layout(location = 0) in vec3 v_normal;
layout(location = 1) in vec2 v_texcoord;

layout(location = 0) out vec4 out_color;

void main() {
    vec3 color = vec3(1.0, 1.0, 1.0);
    float brightness = dot(v_normal, vec3(0.0, 0.0, 1.0)) * 0.5 + 0.5;
    out_color = vec4(color * brightness, 1.0);
}
)";
        
        BX_UNUSED(vsSource, fsSource);
        
        // 注意：实际使用需要编译着色器
        // 这里仅做演示
        
        std::cout << "  Shaders created (placeholder)\n";
        return true;
    }

    /**
     * @brief 单帧渲染
     */
    void frame() {
        // 开始帧
        device_.beginFrame(0);
        
        // 清除屏幕
        device_.clear(0, ClearFlags::All, Color(0.1f, 0.1f, 0.2f, 1.0f), 1.0f, 0);
        
        // 设置渲染状态
        RenderState state = RenderState::opaqueState();
        
        // 创建绘制调用
        DrawCall drawCall;
        drawCall.program = shaderProgram_;
        drawCall.vertexBuffer = triangleVB_ ? triangleVB_->getHandle() : BufferHandle();
        drawCall.indexBuffer = triangleIB_ ? triangleIB_->getHandle() : BufferHandle();
        drawCall.indexCount = 3;
        
        // 提交绘制
        renderer_.beginFrame();
        renderer_.addDrawCall(drawCall);
        
        // 结束帧
        FrameStats stats = device_.endFrame(true);
        
        // 打印统计信息 (每 30 帧)
        static uint32_t frameCounter = 0;
        if (++frameCounter % 30 == 0) {
            std::cout << "Frame " << stats.frameNumber 
                      << " - Time: " << stats.frameTime << " ms"
                      << ", Draw calls: " << stats.drawCalls << "\n";
        }
    }
};

/**
 * @brief 主函数
 */
int main(int argc, char* argv[]) {
    std::cout << "========================================\n";
    std::cout << "  Phoenix Engine - Basic Render Example\n";
    std::cout << "========================================\n\n";
    
    BasicRenderExample example;
    
    if (!example.initialize()) {
        std::cerr << "Failed to initialize example\n";
        return 1;
    }
    
    example.run();
    
    return 0;
}
