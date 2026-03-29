#pragma once

#include "Types.hpp"
#include "RenderDevice.hpp"
#include "Resources.hpp"
#include "Shader.hpp"
#include "PBR.hpp"
#include "PostProcess.hpp"
#include <array>
#include <vector>
#include <memory>
#include <unordered_map>

namespace phoenix {
namespace render {

/**
 * @brief 延迟渲染常量
 */
namespace DeferredConstants {
    constexpr uint32_t MAX_GBUFFER_SIZE = 4096;
    constexpr uint32_t MIN_GBUFFER_SIZE = 256;
    constexpr uint32_t MAX_LIGHTS_PER_TILE = 64;
    constexpr uint32_t TILE_SIZE = 16;
    constexpr uint32_t MAX_LIGHTS_TOTAL = 1024;
    constexpr uint32_t MAX_CLUSTERED_LIGHTS = 256;
    constexpr float CLUSTER_DEPTH_BASE = 1.0f;
    constexpr float CLUSTER_DEPTH_SCALE = 1.1f;
}

/**
 * @brief G-Buffer 格式配置
 */
struct GBufferFormat {
    TextureFormat albedo = TextureFormat::RGBA8;      // RGB: Albedo, A: Alpha
    TextureFormat normal = TextureFormat::RGBA16F;    // XYZ: Normal, W: unused
    TextureFormat material = TextureFormat::RGBA8;    // R: Roughness, G: Metallic, B: AO, A: Emissive
    TextureFormat depth = TextureFormat::Depth24;
    bool useRGBE = false;  // 使用 RGBE 编码节省带宽
    bool useCompression = false; // 使用 BC 压缩
};

/**
 * @brief 延迟渲染配置
 */
struct DeferredConfig {
    uint32_t width = 1920;
    uint32_t height = 1080;
    GBufferFormat gbufferFormat;
    uint32_t sampleCount = 1;
    bool useTileCulling = true;
    bool useClusteredCulling = false;
    uint32_t tileSize = DeferredConstants::TILE_SIZE;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    uint32_t depthClusters = 32;
    uint32_t xyClusters = 16;
};

/**
 * @brief 光源类型
 */
enum class LightType {
    Directional,
    Point,
    Spot,
    Area, // 区域光 (矩形/圆盘)
    Tube  // 管状光
};

/**
 * @brief 光源数据
 */
struct Light {
    LightType type = LightType::Point;
    
    // 位置/方向
    float position[3];
    float direction[3]; // 聚光灯方向 / 方向光方向
    
    // 颜色/强度
    Color color = Color(1, 1, 1, 1);
    float intensity = 1.0f;
    
    // 范围 (点光源/聚光灯)
    float range = 10.0f;
    float radius = 0.1f; // 光源半径 (软阴影)
    
    // 聚光灯参数
    float spotAngle = 45.0f; // 角度 (度)
    float spotSoftness = 0.1f;
    
    // 区域光参数
    float width = 1.0f;
    float height = 1.0f;
    
    // 阴影
    bool castShadow = true;
    float shadowBias = 0.002f;
    float shadowNormalBias = 0.0f;
    
    // 启用状态
    bool enabled = true;
    
    // 填充对齐
    float padding[3];
    
    /**
     * @brief 获取光源包围盒
     */
    void getBoundingBox(float min[3], float max[3]) const;
    
    /**
     * @brief 检查点是否在光源范围内
     */
    bool isInRange(const float* point) const;
};

/**
 * @brief 光源管理器
 */
class LightManager {
public:
    LightManager();
    
    /**
     * @brief 添加光源
     */
    uint32_t addLight(const Light& light);
    
    /**
     * @brief 移除光源
     */
    void removeLight(uint32_t id);
    
    /**
     * @brief 更新光源
     */
    void updateLight(uint32_t id, const Light& light);
    
    /**
     * @brief 获取光源
     */
    [[nodiscard]] const Light* getLight(uint32_t id) const;
    
    /**
     * @brief 获取所有光源
     */
    [[nodiscard]] const std::vector<Light>& getLights() const { return lights_; }
    
    /**
     * @brief 获取方向光数量
     */
    [[nodiscard]] uint32_t getDirectionalLightCount() const;
    
    /**
     * @brief 获取点光源数量
     */
    [[nodiscard]] uint32_t getPointLightCount() const;
    
    /**
     * @brief 获取聚光灯数量
     */
    [[nodiscard]] uint32_t getSpotLightCount() const;
    
    /**
     * @brief 清除所有光源
     */
    void clear();
    
    /**
     * @brief 光源剔除 (视锥体)
     */
    std::vector<uint32_t> cullLights(const float* frustumPlanes, uint32_t planeCount = 6);
    
    /**
     * @brief Tile-based 剔除
     */
    void buildLightGrid(
        const float* viewProj,
        uint32_t screenWidth,
        uint32_t screenHeight,
        uint32_t tileSize
    );
    
    /**
     * @brief Clustered 剔除
     */
    void buildLightClusters(
        const float* viewProj,
        float nearPlane,
        float farPlane,
        uint32_t depthClusters,
        uint32_t xyClusters
    );
    
    /**
     * @brief 获取 Tile 光源列表
     */
    [[nodiscard]] const uint32_t* getTileLights(uint32_t tileX, uint32_t tileY, 
                                                 uint32_t& count) const;
    
private:
    std::vector<Light> lights_;
    std::vector<bool> lightActive_;
    std::vector<uint32_t> freeIndices_;
    
    // Tile grid
    std::vector<std::vector<uint32_t>> lightGrid_;
    uint32_t gridWidth_ = 0;
    uint32_t gridHeight_ = 0;
    
    // Cluster data
    struct Cluster {
        uint32_t lightIndices[DeferredConstants::MAX_LIGHTS_PER_TILE];
        uint32_t lightCount;
        float boundsMin[3];
        float boundsMax[3];
    };
    std::vector<Cluster> clusters_;
    bool useClustering_ = false;
};

/**
 * @brief 延迟渲染器
 */
class DeferredRenderer {
public:
    DeferredRenderer();
    ~DeferredRenderer();
    
    /**
     * @brief 初始化
     */
    bool initialize(RenderDevice& device, ShaderCompiler& compiler, 
                    const DeferredConfig& config);
    
    /**
     * @brief 关闭
     */
    void shutdown();
    
    /**
     * @brief 调整大小
     */
    void resize(uint32_t width, uint32_t height);
    
    /**
     * @brief 开始几何通道
     */
    void beginGeometryPass(RenderDevice& device);
    
    /**
     * @brief 结束几何通道
     */
    void endGeometryPass(RenderDevice& device);
    
    /**
     * @brief 开始光照通道
     */
    void beginLightingPass(RenderDevice& device);
    
    /**
     * @brief 结束光照通道
     */
    void endLightingPass(RenderDevice& device);
    
    /**
     * @brief 添加几何绘制调用
     */
    void addGeometryDrawCall(const DrawCall& drawCall);
    
    /**
     * @brief 添加光源
     */
    uint32_t addLight(const Light& light);
    void removeLight(uint32_t id);
    void updateLight(uint32_t id, const Light& light);
    
    /**
     * @brief 获取光源管理器
     */
    [[nodiscard]] LightManager& getLightManager() { return lightManager_; }
    
    /**
     * @brief 设置环境光
     */
    void setAmbientLight(const Color& color) { ambientLight_ = color; }
    [[nodiscard]] const Color& getAmbientLight() const { return ambientLight_; }
    
    /**
     * @brief 设置 IBL
     */
    void setIBL(const IBLData& ibl) { ibl_ = &ibl; }
    
    /**
     * @brief 获取 G-Buffer 纹理
     */
    [[nodiscard]] TextureHandle getAlbedoTexture() const;
    [[nodiscard]] TextureHandle getNormalTexture() const;
    [[nodiscard]] TextureHandle getMaterialTexture() const;
    [[nodiscard]] TextureHandle getDepthTexture() const;
    
    /**
     * @brief 获取最终输出
     */
    [[nodiscard]] TextureHandle getOutputTexture() const { return outputTexture_; }
    [[nodiscard]] FrameBufferHandle getOutputFrameBuffer() const { return outputFrameBuffer_; }
    
    /**
     * @brief 获取后处理堆栈
     */
    [[nodiscard]] PostProcessStack& getPostProcessStack() { return postProcessStack_; }
    [[nodiscard]] const PostProcessStack& getPostProcessStack() const { return postProcessStack_; }
    
    /**
     * @brief 应用后处理
     */
    void applyPostProcess(RenderDevice& device, TextureHandle input, 
                          TextureHandle output, uint32_t viewId);
    
    /**
     * @brief 获取配置
     */
    [[nodiscard]] const DeferredConfig& getConfig() const { return config_; }
    
    /**
     * @brief 设置视图投影矩阵
     */
    void setViewProjection(const float* view, const float* projection);
    void setCameraPosition(const float* position);
    
    /**
     * @brief 获取统计信息
     */
    struct Stats {
        uint32_t geometryDrawCalls;
        uint32_t lightCount;
        uint32_t culledLights;
        uint32_t tileCount;
        float geometryPassTime;
        float lightingPassTime;
    };
    [[nodiscard]] const Stats& getStats() const { return stats_; }
    
private:
    RenderDevice* device_ = nullptr;
    ShaderCompiler* compiler_ = nullptr;
    
    DeferredConfig config_;
    
    // G-Buffer
    TextureHandle albedoTexture_;
    TextureHandle normalTexture_;
    TextureHandle materialTexture_;
    TextureHandle depthTexture_;
    FrameBufferHandle gBufferFrameBuffer_;
    
    // 输出
    TextureHandle outputTexture_;
    FrameBufferHandle outputFrameBuffer_;
    
    // 程序
    ProgramHandle geometryProgram_;
    ProgramHandle lightingProgram_;
    ProgramHandle lightingTileProgram_;
    ProgramHandle lightingClusterProgram_;
    ProgramHandle fullscreenQuadProgram_;
    
    // 光源
    LightManager lightManager_;
    Color ambientLight_ = Color(0.03f, 0.03f, 0.03f, 1);
    const IBLData* ibl_ = nullptr;
    
    // 矩阵
    float viewMatrix_[16];
    float projectionMatrix_[16];
    float invViewMatrix_[16];
    float invProjectionMatrix_[16];
    float cameraPosition_[3];
    
    // Uniform 缓冲
    UniformBuffer geometryUniforms_;
    UniformBuffer lightingUniforms_;
    UniformBuffer lightDataBuffer_;
    
    // 绘制调用
    std::vector<DrawCall> geometryDrawCalls_;
    
    // 统计
    Stats stats_;
    
    // 辅助几何体
    BufferHandle fullscreenQuadVB_;
    BufferHandle fullscreenQuadIB_;
    
    // 后处理
    PostProcessStack postProcessStack_;
    
    void createFullscreenQuad();
    void buildLightData();
};

/**
 * @brief 混合渲染器 (支持前向 + 延迟)
 */
class HybridRenderer {
public:
    HybridRenderer();
    ~HybridRenderer();
    
    /**
     * @brief 初始化
     */
    bool initialize(RenderDevice& device, ShaderCompiler& compiler,
                    const DeferredConfig& deferredConfig);
    
    /**
     * @brief 关闭
     */
    void shutdown();
    
    /**
     * @brief 渲染不透明物体 (延迟)
     */
    void renderOpaque(RenderDevice& device, const std::vector<DrawCall>& drawCalls);
    
    /**
     * @brief 渲染透明物体 (前向)
     */
    void renderTransparent(RenderDevice& device, const std::vector<DrawCall>& drawCalls);
    
    /**
     * @brief 获取延迟渲染器
     */
    [[nodiscard]] DeferredRenderer& getDeferredRenderer() { return deferredRenderer_; }
    
private:
    RenderDevice* device_ = nullptr;
    DeferredRenderer deferredRenderer_;
    ForwardRenderer forwardRenderer_;
    
    ProgramHandle transparentProgram_;
    TextureHandle resolvedTexture_;
};

/**
 * @brief G-Buffer 编码/解码工具
 */
namespace GBufferUtils {
    /**
     * @brief 编码法线到 [0,1]
     */
    void encodeNormal(const float* normal, float* outEncoded);
    
    /**
     * @brief 解码法线
     */
    void decodeNormal(const float* encoded, float* outNormal);
    
    /**
     * @brief 编码深度 (非线性)
     */
    float encodeDepth(float depth, float nearPlane, float farPlane);
    
    /**
     * @brief 解码深度
     */
    float decodeDepth(float encoded, float nearPlane, float farPlane);
    
    /**
     * @brief 从深度重建世界位置
     */
    void reconstructWorldPosition(
        float screenX, float screenY,
        float depth,
        const float* invViewProj,
        float* outWorldPos
    );
    
    /**
     * @brief RGBM 编码 (HDR)
     */
    void encodeRGBM(const Color& color, float* outRGBM, float maxRange = 6.0f);
    
    /**
     * @brief RGBM 解码
     */
    Color decodeRGBM(const float* rgbm, float maxRange = 6.0f);
}

} // namespace render
} // namespace phoenix
