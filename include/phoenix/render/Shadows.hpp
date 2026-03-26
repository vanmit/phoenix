#pragma once

#include "Types.hpp"
#include "RenderDevice.hpp"
#include "Resources.hpp"
#include "Shader.hpp"
#include <array>
#include <vector>
#include <memory>

namespace phoenix {
namespace render {

/**
 * @brief 阴影常量
 */
namespace ShadowConstants {
    constexpr uint32_t MAX_CASCADES = 4;
    constexpr uint32_t SHADOW_MAP_SIZE = 2048;
    constexpr uint32_t SHADOW_MAP_SIZE_SMALL = 1024;
    constexpr uint32_t SHADOW_MAP_SIZE_LARGE = 4096;
    constexpr float CASCADE_SPLIT_LAMBDA = 0.8f; // 级联分布参数
    constexpr float PCF_KERNEL_SIZE = 2.0f;
    constexpr float VSM_MIN_BIAS = 0.0001f;
    constexpr float VSM_LIGHT_BLEED_REDUCTION = 0.4f;
}

/**
 * @brief 阴影映射类型
 */
enum class ShadowMapType {
    Standard,   // 标准深度贴图
    VSM,        // 方差阴影贴图
    CSM,        // 级联阴影贴图
    Omnidirectional // 全向阴影 (点光源)
};

/**
 * @brief 阴影质量预设
 */
enum class ShadowQuality {
    Low,    // 512x512, 1 级联
    Medium, // 1024x1024, 2 级联
    High,   // 2048x2048, 4 级联
    Ultra   // 4096x4096, 4 级联 + VSM
};

/**
 * @brief 级联配置
 */
struct CascadeConfig {
    uint32_t cascadeCount = ShadowConstants::MAX_CASCADES;
    float splitLambda = ShadowConstants::CASCADE_SPLIT_LAMBDA;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
    float blendWidth = 0.1f; // 级联混合宽度
    
    // 每级联的视锥体
    struct CascadeData {
        float splitNear;
        float splitFar;
        float orthoSize;
        float matrix[16]; // 级联变换矩阵
    } cascades[ShadowConstants::MAX_CASCADES];
};

/**
 * @brief PCF 滤波配置
 */
struct PCFConfig {
    uint32_t kernelSize = 3; // 3x3, 5x5, 7x7
    float kernelRadius = 1.0f;
    bool useSoftTransition = true;
    float softness = 0.5f;
    
    // 预定义的滤波权重
    static constexpr float kernel3x3[9] = {
        1.0f/16.0f, 2.0f/16.0f, 1.0f/16.0f,
        2.0f/16.0f, 4.0f/16.0f, 2.0f/16.0f,
        1.0f/16.0f, 2.0f/16.0f, 1.0f/16.0f
    };
    
    static constexpr float kernel5x5[25] = {
        1.0f/273.0f,  4.0f/273.0f,  7.0f/273.0f,  4.0f/273.0f,  1.0f/273.0f,
        4.0f/273.0f, 16.0f/273.0f, 26.0f/273.0f, 16.0f/273.0f,  4.0f/273.0f,
        7.0f/273.0f, 26.0f/273.0f, 41.0f/273.0f, 26.0f/273.0f,  7.0f/273.0f,
        4.0f/273.0f, 16.0f/273.0f, 26.0f/273.0f, 16.0f/273.0f,  4.0f/273.0f,
        1.0f/273.0f,  4.0f/273.0f,  7.0f/273.0f,  4.0f/273.0f,  1.0f/273.0f
    };
};

/**
 * @brief VSM 配置
 */
struct VSMConfig {
    bool enabled = false;
    float minVariance = ShadowConstants::VSM_MIN_BIAS;
    float lightBleedReduction = ShadowConstants::VSM_LIGHT_BLEED_REDUCTION;
    float maxDarkness = 0.3f;
    
    // VSM 使用 RGBA16F 存储深度矩 (depth, depth^2)
    TextureFormat format = TextureFormat::RGBA16F;
};

/**
 * @brief 阴影剔除配置
 */
struct ShadowCullingConfig {
    bool enabled = true;
    float backFaceCulling = 0.01f; // 背面剔除阈值
    float depthBoundsTest = true;
    float tileBasedCulling = true;
    uint32_t tileSize = 16;
};

/**
 * @brief 阴影贴图描述
 */
struct ShadowMapDesc {
    ShadowMapType type = ShadowMapType::CSM;
    uint32_t width = ShadowConstants::SHADOW_MAP_SIZE;
    uint32_t height = ShadowConstants::SHADOW_MAP_SIZE;
    uint32_t arraySize = ShadowConstants::MAX_CASCADES;
    TextureFormat format = TextureFormat::Depth24;
    bool useVSM = false;
    bool usePCF = true;
    ShadowQuality quality = ShadowQuality::High;
};

/**
 * @brief 阴影贴图
 */
class ShadowMap {
public:
    ShadowMap() = default;
    ~ShadowMap();
    
    /**
     * @brief 创建阴影贴图
     */
    bool create(RenderDevice& device, const ShadowMapDesc& desc);
    
    /**
     * @brief 销毁阴影贴图
     */
    void destroy();
    
    /**
     * @brief 调整大小
     */
    void resize(uint32_t width, uint32_t height);
    
    /**
     * @brief 获取深度纹理
     */
    [[nodiscard]] TextureHandle getDepthTexture() const { return depthTexture_; }
    
    /**
     * @brief 获取帧缓冲
     */
    [[nodiscard]] FrameBufferHandle getFrameBuffer() const { return frameBuffer_; }
    
    /**
     * @brief 获取描述
     */
    [[nodiscard]] const ShadowMapDesc& getDesc() const { return desc_; }
    
    /**
     * @brief 获取宽度
     */
    [[nodiscard]] uint32_t getWidth() const { return desc_.width; }
    
    /**
     * @brief 获取高度
     */
    [[nodiscard]] uint32_t getHeight() const { return desc_.height; }
    
private:
    RenderDevice* device_ = nullptr;
    ShadowMapDesc desc_;
    TextureHandle depthTexture_;
    TextureHandle momentTexture_; // VSM 用
    FrameBufferHandle frameBuffer_;
};

/**
 * @brief 级联阴影数据
 */
struct CascadeShadowData {
    // 每级联的视锥体边界
    struct FrustumBounds {
        float corners[8][3]; // 8 个角点
        float center[3];
        float radius;
        float nearPlane;
        float farPlane;
    } bounds[ShadowConstants::MAX_CASCADES];
    
    // 每级联的光空间矩阵
    float lightMatrices[ShadowConstants::MAX_CASCADES][16];
    
    // 级联分割距离
    float splitDistances[ShadowConstants::MAX_CASCADES + 1];
    
    // 当前激活的级联
    uint32_t activeCascade = 0;
    
    /**
     * @brief 计算级联分割
     */
    void calculateSplits(float nearPlane, float farPlane, float lambda);
    
    /**
     * @brief 计算级联视锥体
     */
    void calculateCascadeFrustums(
        const float* viewProj,
        const float* invViewProj,
        uint32_t cascadeCount
    );
    
    /**
     * @brief 计算光空间矩阵
     */
    void calculateLightMatrices(
        const float* lightDir,
        const float* cascadeBounds,
        uint32_t cascadeCount
    );
    
    /**
     * @brief 获取当前级联索引
     */
    uint32_t getCascadeIndex(const float* viewPos) const;
};

/**
 * @brief 阴影渲染器
 */
class ShadowRenderer {
public:
    ShadowRenderer();
    ~ShadowRenderer();
    
    /**
     * @brief 初始化
     */
    bool initialize(RenderDevice& device, ShaderCompiler& compiler);
    
    /**
     * @brief 关闭
     */
    void shutdown();
    
    /**
     * @brief 设置阴影质量
     */
    void setShadowQuality(ShadowQuality quality);
    
    /**
     * @brief 设置级联配置
     */
    void setCascadeConfig(const CascadeConfig& config) { cascadeConfig_ = config; }
    [[nodiscard]] const CascadeConfig& getCascadeConfig() const { return cascadeConfig_; }
    
    /**
     * @brief 设置 PCF 配置
     */
    void setPCFConfig(const PCFConfig& config) { pcfConfig_ = config; }
    [[nodiscard]] const PCFConfig& getPCFConfig() const { return pcfConfig_; }
    
    /**
     * @brief 设置 VSM 配置
     */
    void setVSMConfig(const VSMConfig& config) { vsmConfig_ = config; }
    [[nodiscard]] const VSMConfig& getVSMConfig() const { return vsmConfig_; }
    
    /**
     * @brief 开始阴影通道渲染
     */
    void beginShadowPass(RenderDevice& device);
    
    /**
     * @brief 渲染阴影贴图
     */
    void renderShadowMap(
        RenderDevice& device,
        const float* lightDir,
        const float* viewProj,
        const std::vector<DrawCall>& drawCalls
    );
    
    /**
     * @brief 结束阴影通道
     */
    void endShadowPass(RenderDevice& device);
    
    /**
     * @brief 获取阴影贴图
     */
    [[nodiscard]] ShadowMap& getShadowMap() { return shadowMap_; }
    [[nodiscard]] const ShadowMap& getShadowMap() const { return shadowMap_; }
    
    /**
     * @brief 获取级联数据
     */
    [[nodiscard]] const CascadeShadowData& getCascadeData() const { return cascadeData_; }
    
    /**
     * @brief 获取阴影矩阵数组 (用于着色器)
     */
    [[nodiscard]] const float* getShadowMatrices() const { 
        return reinterpret_cast<const float*>(cascadeData_.lightMatrices);
    }
    
    /**
     * @brief 计算当前级联
     */
    void updateCascades(const float* viewProj, const float* invViewProj);
    
    /**
     * @brief 阴影剔除测试
     */
    bool isVisibleInShadow(const float* boundsMin, const float* boundsMax, 
                           uint32_t cascadeIndex) const;
    
private:
    RenderDevice* device_ = nullptr;
    ShaderCompiler* compiler_ = nullptr;
    
    ShadowMap shadowMap_;
    CascadeShadowData cascadeData_;
    CascadeConfig cascadeConfig_;
    PCFConfig pcfConfig_;
    VSMConfig vsmConfig_;
    ShadowCullingConfig cullingConfig_;
    
    ProgramHandle shadowProgram_;
    ProgramHandle shadowVSMProgram_;
    ProgramHandle shadowArrayProgram_;
    
    FrameBufferHandle cascadeFrameBuffers_[ShadowConstants::MAX_CASCADES];
    
    UniformBuffer shadowUniforms_;
    
    bool inShadowPass_ = false;
    
    // 统计信息
    struct Stats {
        uint32_t cascadesRendered;
        uint32_t drawCallsShadow;
        uint32_t culledObjects;
        float shadowPassTime;
    } stats_;
};

/**
 * @brief 点光源阴影 (全向阴影)
 */
class OmnidirectionalShadow {
public:
    OmnidirectionalShadow() = default;
    ~OmnidirectionalShadow();
    
    /**
     * @brief 创建全向阴影贴图
     */
    bool create(RenderDevice& device, uint32_t size);
    
    /**
     * @brief 销毁
     */
    void destroy();
    
    /**
     * @brief 渲染全向阴影
     */
    void render(RenderDevice& device, const float* lightPos, float range,
                const std::vector<DrawCall>& drawCalls);
    
    [[nodiscard]] TextureHandle getCubeDepth() const { return cubeDepth_; }
    [[nodiscard]] FrameBufferHandle getFrameBuffer() const { return frameBuffer_; }
    
private:
    RenderDevice* device_ = nullptr;
    TextureHandle cubeDepth_;
    FrameBufferHandle frameBuffer_;
    uint32_t size_ = 0;
    
    // 6 个面的投影矩阵
    float projectionMatrices_[6][16];
};

/**
 * @brief 阴影工具函数
 */
namespace ShadowUtils {
    /**
     * @brief 计算最优的正交投影尺寸
     */
    float calculateOptimalOrthoSize(
        const float* frustumCorners,
        const float* lightDir,
        float padding = 1.2f
    );
    
    /**
     * @brief 将点变换到光空间
     */
    void transformToLightSpace(
        const float* point,
        const float* lightMatrix,
        float* outLightSpace
    );
    
    /**
     * @brief 计算级联混合因子
     */
    float calculateCascadeBlend(
        float depth,
        float cascadeNear,
        float cascadeFar,
        float blendWidth
    );
    
    /**
     * @brief PCF 滤波
     */
    float filterPCF(
        const float* shadowMap,
        uint32_t width, uint32_t height,
        float u, float v,
        float texelSize,
        const PCFConfig& config
    );
    
    /**
     * @brief VSM 可见性计算
     */
    float calculateVSMVisibility(
        float depth,
        float moment1,
        float moment2,
        float minVariance,
        float lightBleedReduction
    );
    
    /**
     * @brief 生成阴影深度偏置矩阵
     */
    void createShadowBiasMatrix(float* matrix, float bias = 0.002f);
}

} // namespace render
} // namespace phoenix
