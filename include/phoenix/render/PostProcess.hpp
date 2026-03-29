#pragma once

#include "Types.hpp"
#include "RenderDevice.hpp"
#include "Resources.hpp"
#include "Shader.hpp"
#include "../math/vector.hpp"
#include "../math/matrix.hpp"
#include <vector>
#include <memory>
#include <functional>
#include <string>

namespace phoenix {
namespace render {

/**
 * @brief 后处理常量
 */
namespace PostProcessConstants {
    constexpr uint32_t MAX_EFFECTS = 32;
    constexpr uint32_t MAX_EFFECT_CHAIN = 16;
    constexpr uint32_t DEFAULT_DOWNSAMPLE_FACTOR = 1;
    constexpr uint32_t BLOOM_MIP_LEVELS = 6;
    constexpr uint32_t SSAO_KERNEL_SIZE = 16;
    constexpr uint32_t SSAO_NOISE_SIZE = 4;
    constexpr float TAA_HISTORY_LENGTH = 8.0f;
}

/**
 * @brief 后处理效果类型
 */
enum class PostProcessEffectType {
    Bloom,
    ToneMapping,
    SSAO,
    FXAA,
    TAA,
    ColorGrading,
    Vignette,
    FilmGrain,
    ChromaticAberration,
    MotionBlur,
    DepthOfField,
    AmbientOcclusion,
    Custom
};

/**
 * @brief 色调映射算法
 */
enum class ToneMappingAlgorithm {
    None,       // 无色调映射
    Reinhard,   // 经典 Reinhard
    Reinhard2,  // 改进版 (带白点)
    ACES,       // ACES 电影曲线
    ACESApprox, // ACES 近似
    Uncharted2, // Uncharted 2
    HejlDawson, // Hejl-Dawson
    Hable,      // Hable (UC2 改进)
    Neutral     // 中性色调映射
};

/**
 * @brief 抗锯齿模式
 */
enum class AntiAliasingMode {
    None,
    FXAA,       // 快速近似抗锯齿
    FXAAHigh,   // FXAA 高质量
    SMAA,       // 子像素形态抗锯齿
    TAA,        // 时间性抗锯齿
    DLSS,       // DLSS (需要硬件支持)
    FSR         // FidelityFX Super Resolution
};

/**
 * @brief Bloom 配置
 */
struct BloomConfig {
    bool enabled = true;
    float threshold = 1.0f;       // 亮度阈值
    float thresholdSoft = 0.5f;   // 阈值软过渡
    float intensity = 1.0f;       // Bloom 强度
    float scatter = 0.5f;         // 散射
    float tint[3] = {1, 1, 1};    // 色调
    uint32_t iterations = 4;      // 降采样迭代次数
    float blurRadius = 1.0f;      // 模糊半径
    bool useAnamorphic = false;   // 变形镜头效果
    float anamorphicRatio = 0.3f; // 变形比率
    
    // 预过滤
    bool usePreFilter = true;
    float knee = 0.05f;           // 膝盖值 (平滑过渡)
};

/**
 * @brief 色调映射配置
 */
struct ToneMappingConfig {
    ToneMappingAlgorithm algorithm = ToneMappingAlgorithm::ACES;
    float exposure = 1.0f;
    float gamma = 2.2f;
    float whitePoint = 1.0f;
    float contrast = 1.0f;
    float saturation = 1.0f;
    bool autoExposure = false;
    float autoExposureSpeed = 0.5f;
    float minEV100 = -10.0f;
    float maxEV100 = 20.0f;
    float middleGray = 0.18f;
};

/**
 * @brief SSAO 配置
 */
struct SSAOConfig {
    bool enabled = true;
    float radius = 0.5f;          // 采样半径
    float bias = 0.025f;          // 偏差
    float intensity = 1.0f;       // 强度
    float scale = 1.0f;           // 缩放
    float sharpness = 0.75f;      // 锐度
    uint32_t sampleCount = 16;    // 采样数
    bool useNoise = true;         // 使用噪声纹理
    bool useBlur = true;          // 使用模糊
    enum class BlurType {
        Gaussian,
        Bilateral,
        EdgeAware
    } blurType = BlurType::Bilateral;
    uint32_t blurIterations = 2;
    float depthMipScale = 0.1f;
    
    // 法线敏感
    bool useNormals = true;
    float normalThreshold = 0.1f;
};

/**
 * @brief FXAA 配置
 */
struct FXAAConfig {
    bool enabled = true;
    enum class Quality {
        Low,
        Medium,
        High,
        Ultra,
        Extreme
    } quality = Quality::High;
    
    float subpixelQuality = 0.75f;
    float edgeThreshold = 0.166f;
    float edgeThresholdMin = 0.0833f;
    uint32_t iterations = 12;
    
    // 预设
    static FXAAConfig getPreset(Quality q);
};

/**
 * @brief TAA 配置
 */
struct TAAConfig {
    bool enabled = true;
    float blendFactor = 0.1f;     // 历史混合因子
    float motionBlurFactor = 0.5f;
    bool useCatmullRom = true;    // Catmull-Rom 重采样
    float sharpness = 0.5f;       // 锐化
    bool useVarianceClipping = true;
    float varianceThreshold = 0.1f;
    bool useNeighborhoodClamping = true;
    uint32_t historyLength = 8;
    float jitterScale = 1.0f;
};

/**
 * @brief 颜色分级配置
 */
struct ColorGradingConfig {
    bool enabled = true;
    
    // 基本调整
    float temperature = 0.0f;     // 色温
    float tint = 0.0f;            // 色调
    float saturation = 0.0f;      // 饱和度
    float contrast = 0.0f;        // 对比度
    float brightness = 0.0f;      // 亮度
    float gamma = 0.0f;           // Gamma
    
    // 阴影/中间调/高光
    Color lift = Color(0, 0, 0, 1);
    Color gamma_ = Color(0, 0, 0, 1);
    Color gain = Color(0, 0, 0, 1);
    
    // 曲线 (简化: 3 点)
    float shadows[3] = {0, 0, 0};
    float midtones[3] = {0, 0, 0};
    float highlights[3] = {0, 0, 0};
    
    // LUT
    bool useLUT = false;
    TextureHandle lutTexture;
    float lutIntensity = 1.0f;
    
    // 通道混合
    float channelMix[3][3] = {
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1}
    };
};

/**
 * @brief 暗角配置
 */
struct VignetteConfig {
    bool enabled = false;
    float intensity = 0.4f;
    float smoothness = 0.5f;
    float roundness = 1.0f;
    float centerX = 0.5f;
    float centerY = 0.5f;
    Color color = Color(0, 0, 0, 1);
    bool useOval = true;
};

/**
 * @brief 胶片颗粒配置
 */
struct FilmGrainConfig {
    bool enabled = false;
    float intensity = 0.35f;
    float size = 1.0f;
    float speed = 1.0f;
    bool useColored = true;
};

/**
 * @brief 色差配置
 */
struct ChromaticAberrationConfig {
    bool enabled = false;
    float intensity = 0.003f;
    float sampleCount = 3.0f;
    bool useSpectral = false;
};

/**
 * @brief 运动模糊配置
 */
struct MotionBlurConfig {
    bool enabled = true;
    float shutterSpeed = 1000.0f;     // 快门速度 (1/1000 秒)
    float intensity = 1.0f;           // 模糊强度
    uint32_t sampleCount = 16;        // 采样数
    float maxBlurDistance = 50.0f;    // 最大模糊距离 (像素)
    bool useCameraMotion = true;      // 相机运动模糊
    bool useObjectMotion = true;      // 物体运动模糊
    float cameraVelocityScale = 1.0f; // 相机速度缩放
    bool useDepthFade = true;         // 深度衰减
    float depthFadeStart = 0.5f;      // 深度衰减起始
    float depthFadeEnd = 1.0f;        // 深度衰减结束
};

/**
 * @brief 景深配置
 */
struct DepthOfFieldConfig {
    bool enabled = true;
    enum class Quality {
        Low,      // 快速近似
        Medium,   // 平衡质量
        High,     // 高质量
        Ultra     // 极致质量 (Bokeh)
    } quality = Quality::High;
    
    float focalDistance = 10.0f;      // 焦距 (米)
    float focalLength = 50.0f;        // 镜头焦距 (mm)
    float aperture = 2.8f;            // 光圈 (f-stop)
    float maxCoC = 8.0f;              // 最大弥散圆 (像素)
    uint32_t sampleCount = 16;        // 采样数
    float nearBlur = 1.0f;            // 前景模糊强度
    float farBlur = 1.0f;             // 背景模糊强度
    math::Vec2 sensorSize = math::Vec2(36, 24);   // 传感器尺寸 (mm，全画幅)
    
    // Bokeh 设置
    enum class BokehShape {
        Circle,     // 圆形
        Hexagon,    // 六边形
        Octagon     // 八边形
    } bokehShape = BokehShape::Circle;
    float bokehRotation = 0.0f;       // Bokeh 旋转
    float bokehIntensity = 1.0f;      // Bokeh 强度
    bool useAnamorphic = false;       // 变形镜头 Bokeh
    float anamorphicRatio = 1.5f;     // 变形比率
    
    // 高级设置
    bool useForegroundSeparation = true;  // 前景分离
    float foregroundThreshold = 0.3f;     // 前景阈值
    bool useVignette = false;             // 渐晕效果
    float vignetteIntensity = 0.3f;       // 渐晕强度
};

/**
 * @brief 后处理效果基类
 */
class PostProcessEffect {
public:
    PostProcessEffect() = default;
    virtual ~PostProcessEffect() = default;
    
    /**
     * @brief 获取效果类型
     */
    [[nodiscard]] virtual PostProcessEffectType getType() const = 0;
    
    /**
     * @brief 获取效果名称
     */
    [[nodiscard]] virtual const char* getName() const = 0;
    
    /**
     * @brief 初始化
     */
    virtual bool initialize(RenderDevice& device, ShaderCompiler& compiler) = 0;
    
    /**
     * @brief 关闭
     */
    virtual void shutdown() = 0;
    
    /**
     * @brief 渲染效果
     */
    virtual void render(RenderDevice& device, TextureHandle input, 
                        TextureHandle output, uint32_t viewId) = 0;
    
    /**
     * @brief 是否启用
     */
    [[nodiscard]] virtual bool isEnabled() const = 0;
    
    /**
     * @brief 设置启用状态
     */
    virtual void setEnabled(bool enabled) = 0;
    
    /**
     * @brief 获取输入输出尺寸
     */
    [[nodiscard]] virtual uint32_t getWidth() const = 0;
    [[nodiscard]] virtual uint32_t getHeight() const = 0;
    
    /**
     * @brief 调整大小
     */
    virtual void resize(uint32_t width, uint32_t height) = 0;
};

/**
 * @brief Bloom 效果
 */
class BloomEffect : public PostProcessEffect {
public:
    BloomEffect() = default;
    
    [[nodiscard]] PostProcessEffectType getType() const override { 
        return PostProcessEffectType::Bloom; 
    }
    [[nodiscard]] const char* getName() const override { return "Bloom"; }
    
    bool initialize(RenderDevice& device, ShaderCompiler& compiler) override;
    void shutdown() override;
    void render(RenderDevice& device, TextureHandle input, 
                TextureHandle output, uint32_t viewId) override;
    [[nodiscard]] bool isEnabled() const override { return config_.enabled; }
    void setEnabled(bool enabled) override { config_.enabled = enabled; }
    [[nodiscard]] uint32_t getWidth() const override { width_; }
    [[nodiscard]] uint32_t getHeight() const override { height_; }
    void resize(uint32_t width, uint32_t height) override;
    
    void setConfig(const BloomConfig& config) { config_ = config; }
    [[nodiscard]] const BloomConfig& getConfig() const { return config_; }
    
private:
    RenderDevice* device_ = nullptr;
    BloomConfig config_;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    
    ProgramHandle thresholdProgram_;
    ProgramHandle blurProgram_;
    ProgramHandle combineProgram_;
    
    std::vector<TextureHandle> mipChain_;
    FrameBufferHandle mipFrameBuffers_[BLOOM_MIP_LEVELS];
    
    UniformBuffer bloomUniforms_;
};

/**
 * @brief 色调映射效果
 */
class ToneMappingEffect : public PostProcessEffect {
public:
    ToneMappingEffect() = default;
    
    [[nodiscard]] PostProcessEffectType getType() const override { 
        return PostProcessEffectType::ToneMapping; 
    }
    [[nodiscard]] const char* getName() const override { return "ToneMapping"; }
    
    bool initialize(RenderDevice& device, ShaderCompiler& compiler) override;
    void shutdown() override;
    void render(RenderDevice& device, TextureHandle input, 
                TextureHandle output, uint32_t viewId) override;
    [[nodiscard]] bool isEnabled() const override { return true; } // 总是启用
    void setEnabled(bool) override {}
    [[nodiscard]] uint32_t getWidth() const override { width_; }
    [[nodiscard]] uint32_t getHeight() const override { height_; }
    void resize(uint32_t width, uint32_t height) override;
    
    void setConfig(const ToneMappingConfig& config) { config_ = config; }
    [[nodiscard]] const ToneMappingConfig& getConfig() const { return config_; }
    
private:
    RenderDevice* device_ = nullptr;
    ToneMappingConfig config_;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    
    ProgramHandle toneMappingProgram_;
    UniformBuffer toneMappingUniforms_;
    
    float currentExposure_ = 1.0f;
};

/**
 * @brief SSAO 效果
 */
class SSAOEffect : public PostProcessEffect {
public:
    SSAOEffect() = default;
    
    [[nodiscard]] PostProcessEffectType getType() const override { 
        return PostProcessEffectType::SSAO; 
    }
    [[nodiscard]] const char* getName() const override { return "SSAO"; }
    
    bool initialize(RenderDevice& device, ShaderCompiler& compiler) override;
    void shutdown() override;
    void render(RenderDevice& device, TextureHandle input, 
                TextureHandle output, uint32_t viewId) override;
    [[nodiscard]] bool isEnabled() const override { return config_.enabled; }
    void setEnabled(bool enabled) override { config_.enabled = enabled; }
    [[nodiscard]] uint32_t getWidth() const override { width_; }
    [[nodiscard]] uint32_t getHeight() const override { height_; }
    void resize(uint32_t width, uint32_t height) override;
    
    void setConfig(const SSAOConfig& config) { config_ = config; }
    [[nodiscard]] const SSAOConfig& getConfig() const { return config_; }
    
    /**
     * @brief 设置 G-Buffer 纹理
     */
    void setGBufferTextures(TextureHandle normal, TextureHandle depth);
    
private:
    RenderDevice* device_ = nullptr;
    SSAOConfig config_;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    
    TextureHandle normalTexture_;
    TextureHandle depthTexture_;
    
    ProgramHandle ssaoProgram_;
    ProgramHandle blurProgram_;
    
    TextureHandle noiseTexture_;
    TextureHandle aoTexture_;
    TextureHandle blurredAoTexture_;
    FrameBufferHandle aoFrameBuffer_;
    FrameBufferHandle blurFrameBuffer_;
    
    UniformBuffer ssaoUniforms_;
    
    // 采样核
    float sampleKernel[SSAOConstants::SSAO_KERNEL_SIZE][3];
    float noiseMatrix[SSAOConstants::SSAO_NOISE_SIZE * SSAOConstants::SSAO_NOISE_SIZE * 4];
    
    void generateSampleKernel();
    void generateNoiseTexture();
};

/**
 * @brief FXAA 效果
 */
class FXAAEffect : public PostProcessEffect {
public:
    FXAAEffect() = default;
    
    [[nodiscard]] PostProcessEffectType getType() const override { 
        return PostProcessEffectType::FXAA; 
    }
    [[nodiscard]] const char* getName() const override { return "FXAA"; }
    
    bool initialize(RenderDevice& device, ShaderCompiler& compiler) override;
    void shutdown() override;
    void render(RenderDevice& device, TextureHandle input, 
                TextureHandle output, uint32_t viewId) override;
    [[nodiscard]] bool isEnabled() const override { return config_.enabled; }
    void setEnabled(bool enabled) override { config_.enabled = enabled; }
    [[nodiscard]] uint32_t getWidth() const override { width_; }
    [[nodiscard]] uint32_t getHeight() const override { height_; }
    void resize(uint32_t width, uint32_t height) override;
    
    void setConfig(const FXAAConfig& config) { config_ = config; }
    [[nodiscard]] const FXAAConfig& getConfig() const { return config_; }
    
private:
    RenderDevice* device_ = nullptr;
    FXAAConfig config_;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    
    ProgramHandle fxaaProgram_;
    UniformBuffer fxaaUniforms_;
};

/**
 * @brief 颜色分级效果
 */
class ColorGradingEffect : public PostProcessEffect {
public:
    ColorGradingEffect() = default;
    
    [[nodiscard]] PostProcessEffectType getType() const override { 
        return PostProcessEffectType::ColorGrading; 
    }
    [[nodiscard]] const char* getName() const override { return "ColorGrading"; }
    
    bool initialize(RenderDevice& device, ShaderCompiler& compiler) override;
    void shutdown() override;
    void render(RenderDevice& device, TextureHandle input, 
                TextureHandle output, uint32_t viewId) override;
    [[nodiscard]] bool isEnabled() const override { return config_.enabled; }
    void setEnabled(bool enabled) override { config_.enabled = enabled; }
    [[nodiscard]] uint32_t getWidth() const override { width_; }
    [[nodiscard]] uint32_t getHeight() const override { height_; }
    void resize(uint32_t width, uint32_t height) override;
    
    void setConfig(const ColorGradingConfig& config) { config_ = config; }
    [[nodiscard]] const ColorGradingConfig& getConfig() const { return config_; }
    
private:
    RenderDevice* device_ = nullptr;
    ColorGradingConfig config_;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    
    ProgramHandle colorGradingProgram_;
    UniformBuffer colorGradingUniforms_;
};

/**
 * @brief TAA (时间性抗锯齿) 效果
 */
class TAAEffect : public PostProcessEffect {
public:
    TAAEffect() = default;
    
    [[nodiscard]] PostProcessEffectType getType() const override { 
        return PostProcessEffectType::TAA; 
    }
    [[nodiscard]] const char* getName() const override { return "TAA"; }
    
    bool initialize(RenderDevice& device, ShaderCompiler& compiler) override;
    void shutdown() override;
    void render(RenderDevice& device, TextureHandle input, 
                TextureHandle output, uint32_t viewId) override;
    [[nodiscard]] bool isEnabled() const override { return config_.enabled; }
    void setEnabled(bool enabled) override { config_.enabled = enabled; }
    [[nodiscard]] uint32_t getWidth() const override { width_; }
    [[nodiscard]] uint32_t getHeight() const override { height_; }
    void resize(uint32_t width, uint32_t height) override;
    
    void setConfig(const TAAConfig& config) { config_ = config; }
    [[nodiscard]] const TAAConfig& getConfig() const { return config_; }
    
    /**
     * @brief 设置历史帧纹理
     */
    void setHistoryTexture(TextureHandle history) { historyTexture_ = history; }
    
    /**
     * @brief 获取历史帧纹理
     */
    [[nodiscard]] TextureHandle getHistoryTexture() const { return historyTexture_; }
    
    /**
     * @brief 重置历史
     */
    void resetHistory();
    
    /**
     * @brief 设置运动矢量纹理
     */
    void setMotionVectorTexture(TextureHandle motion) { motionVectorTexture_ = motion; }
    
    /**
     * @brief 设置深度纹理
     */
    void setDepthTexture(TextureHandle depth) { depthTexture_ = depth; }
    
    /**
     * @brief 更新变换矩阵
     */
    void setTransformMatrices(const math::Matrix4& prevViewProj, const math::Matrix4& currViewProj,
                              const math::Matrix4& inverseProjection);
    
private:
    RenderDevice* device_ = nullptr;
    TAAConfig config_;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    
    ProgramHandle taaProgram_;
    UniformBuffer taaUniforms_;
    
    TextureHandle historyTexture_;
    TextureHandle motionVectorTexture_;
    TextureHandle depthTexture_;
    FrameBufferHandle historyFrameBuffer_;
    
    math::Matrix4 prevViewProj_;
    math::Matrix4 currViewProj_;
    math::Matrix4 inverseProjection_;
    
    bool historyValid_ = false;
    uint32_t frameCount_ = 0;
    
    void createHistoryTexture();
    void updateUniforms();
};

/**
 * @brief 运动模糊效果
 */
class MotionBlurEffect : public PostProcessEffect {
public:
    MotionBlurEffect() = default;
    
    [[nodiscard]] PostProcessEffectType getType() const override { 
        return PostProcessEffectType::MotionBlur; 
    }
    [[nodiscard]] const char* getName() const override { return "MotionBlur"; }
    
    bool initialize(RenderDevice& device, ShaderCompiler& compiler) override;
    void shutdown() override;
    void render(RenderDevice& device, TextureHandle input, 
                TextureHandle output, uint32_t viewId) override;
    [[nodiscard]] bool isEnabled() const override { return config_.enabled; }
    void setEnabled(bool enabled) override { config_.enabled = enabled; }
    [[nodiscard]] uint32_t getWidth() const override { width_; }
    [[nodiscard]] uint32_t getHeight() const override { height_; }
    void resize(uint32_t width, uint32_t height) override;
    
    void setConfig(const MotionBlurConfig& config) { config_ = config; }
    [[nodiscard]] const MotionBlurConfig& getConfig() const { return config_; }
    
    /**
     * @brief 设置运动矢量纹理
     */
    void setMotionVectorTexture(TextureHandle motion) { motionVectorTexture_ = motion; }
    
    /**
     * @brief 设置深度纹理
     */
    void setDepthTexture(TextureHandle depth) { depthTexture_ = depth; }
    
    /**
     * @brief 设置相机速度
     */
    void setCameraVelocity(const math::Vec3& velocity) { cameraVelocity_ = velocity; }
    
private:
    RenderDevice* device_ = nullptr;
    MotionBlurConfig config_;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    
    ProgramHandle motionBlurProgram_;
    UniformBuffer motionBlurUniforms_;
    
    TextureHandle motionVectorTexture_;
    TextureHandle depthTexture_;
    
    math::Vec3 cameraVelocity_;
    
    void updateUniforms();
};

/**
 * @brief 景深效果
 */
class DepthOfFieldEffect : public PostProcessEffect {
public:
    DepthOfFieldEffect() = default;
    
    [[nodiscard]] PostProcessEffectType getType() const override { 
        return PostProcessEffectType::DepthOfField; 
    }
    [[nodiscard]] const char* getName() const override { return "DepthOfField"; }
    
    bool initialize(RenderDevice& device, ShaderCompiler& compiler) override;
    void shutdown() override;
    void render(RenderDevice& device, TextureHandle input, 
                TextureHandle output, uint32_t viewId) override;
    [[nodiscard]] bool isEnabled() const override { return config_.enabled; }
    void setEnabled(bool enabled) override { config_.enabled = enabled; }
    [[nodiscard]] uint32_t getWidth() const override { width_; }
    [[nodiscard]] uint32_t getHeight() const override { height_; }
    void resize(uint32_t width, uint32_t height) override;
    
    void setConfig(const DepthOfFieldConfig& config) { config_ = config; }
    [[nodiscard]] const DepthOfFieldConfig& getConfig() const { return config_; }
    
    /**
     * @brief 设置深度纹理
     */
    void setDepthTexture(TextureHandle depth) { depthTexture_ = depth; }
    
    /**
     * @brief 设置焦距 (自动对焦)
     */
    void setFocalDistance(float distance) { config_.focalDistance = distance; }
    
    /**
     * @brief 设置光圈
     */
    void setAperture(float fstop) { config_.aperture = fstop; }
    
private:
    RenderDevice* device_ = nullptr;
    DepthOfFieldConfig config_;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    
    ProgramHandle dofProgram_;
    UniformBuffer dofUniforms_;
    
    TextureHandle depthTexture_;
    TextureHandle cocTexture_;       // 预计算的 CoC 纹理
    FrameBufferHandle cocFrameBuffer_;
    
    void updateUniforms();
    void calculateCoCTexture(RenderDevice& device, uint32_t viewId);
};

/**
 * @brief 后处理效果链
 */
class PostProcessChain {
public:
    PostProcessChain();
    ~PostProcessChain();
    
    /**
     * @brief 初始化
     */
    bool initialize(RenderDevice& device, ShaderCompiler& compiler);
    
    /**
     * @brief 关闭
     */
    void shutdown();
    
    /**
     * @brief 添加效果
     */
    void addEffect(std::unique_ptr<PostProcessEffect> effect);
    
    /**
     * @brief 移除效果
     */
    void removeEffect(PostProcessEffectType type);
    
    /**
     * @brief 获取效果
     */
    template<typename T>
    T* getEffect() {
        for (auto& effect : effects_) {
            if (auto* typed = dynamic_cast<T*>(effect.get())) {
                return typed;
            }
        }
        return nullptr;
    }
    
    /**
     * @brief 渲染效果链
     */
    void render(RenderDevice& device, TextureHandle input, 
                TextureHandle output, uint32_t viewId);
    
    /**
     * @brief 调整大小
     */
    void resize(uint32_t width, uint32_t height);
    
    /**
     * @brief 获取中间纹理
     */
    [[nodiscard]] TextureHandle getIntermediateTexture(uint32_t index) const;
    
    /**
     * @brief 获取 ping-pong 纹理
     */
    [[nodiscard]] TextureHandle getPingPongA() const { return pingPongA_; }
    [[nodiscard]] TextureHandle getPingPongB() const { return pingPongB_; }
    
private:
    RenderDevice* device_ = nullptr;
    std::vector<std::unique_ptr<PostProcessEffect>> effects_;
    
    TextureHandle pingPongA_;
    TextureHandle pingPongB_;
    FrameBufferHandle pingPongA_FB_;
    FrameBufferHandle pingPongB_FB_;
    
    uint32_t width_ = 0;
    uint32_t height_ = 0;
};

/**
 * @brief 后处理堆栈
 */
class PostProcessStack {
public:
    PostProcessStack();
    ~PostProcessStack();
    
    /**
     * @brief 初始化
     */
    bool initialize(RenderDevice& device, ShaderCompiler& compiler);
    
    /**
     * @brief 关闭
     */
    void shutdown();
    
    /**
     * @brief 调整大小
     */
    void resize(uint32_t width, uint32_t height);
    
    /**
     * @brief 渲染
     */
    void render(RenderDevice& device, TextureHandle input, 
                TextureHandle output, uint32_t viewId);
    
    // 配置访问
    [[nodiscard]] BloomConfig& getBloomConfig() { return bloomConfig_; }
    [[nodiscard]] ToneMappingConfig& getToneMappingConfig() { return toneMappingConfig_; }
    [[nodiscard]] SSAOConfig& getSSAOConfig() { return ssaoConfig_; }
    [[nodiscard]] FXAAConfig& getFXAAConfig() { return fxaaConfig_; }
    [[nodiscard]] TAAConfig& getTAAConfig() { return taaConfig_; }
    [[nodiscard]] ColorGradingConfig& getColorGradingConfig() { return colorGradingConfig_; }
    [[nodiscard]] VignetteConfig& getVignetteConfig() { return vignetteConfig_; }
    [[nodiscard]] FilmGrainConfig& getFilmGrainConfig() { return filmGrainConfig_; }
    [[nodiscard]] ChromaticAberrationConfig& getChromaticAberrationConfig() { 
        return chromaticAberrationConfig_; 
    }
    
    // 启用/禁用
    void enableBloom(bool enabled) { bloomConfig_.enabled = enabled; }
    void enableSSAO(bool enabled) { ssaoConfig_.enabled = enabled; }
    void enableFXAA(bool enabled) { fxaaConfig_.enabled = enabled; }
    void enableTAA(bool enabled) { taaConfig_.enabled = enabled; }
    void enableColorGrading(bool enabled) { colorGradingConfig_.enabled = enabled; }
    void enableMotionBlur(bool enabled) { motionBlurConfig_.enabled = enabled; }
    void enableDepthOfField(bool enabled) { depthOfFieldConfig_.enabled = enabled; }
    
    // 质量档位
    enum class QualityPreset {
        Low,
        Medium,
        High,
        Ultra
    };
    void setQualityPreset(QualityPreset preset);
    
    // 设置 G-Buffer (用于 SSAO)
    void setGBufferTextures(TextureHandle normal, TextureHandle depth);
    
    // 设置 IBL (用于反射)
    void setIBL(const IBLData* ibl) { ibl_ = ibl; }
    
    /**
     * @brief 获取统计信息
     */
    struct Stats {
        uint32_t activeEffects;
        float postProcessTime;
        uint32_t passes;
    };
    [[nodiscard]] const Stats& getStats() const { return stats_; }
    
private:
    RenderDevice* device_ = nullptr;
    ShaderCompiler* compiler_ = nullptr;
    
    // 配置
    BloomConfig bloomConfig_;
    ToneMappingConfig toneMappingConfig_;
    SSAOConfig ssaoConfig_;
    FXAAConfig fxaaConfig_;
    TAAConfig taaConfig_;
    ColorGradingConfig colorGradingConfig_;
    VignetteConfig vignetteConfig_;
    FilmGrainConfig filmGrainConfig_;
    ChromaticAberrationConfig chromaticAberrationConfig_;
    MotionBlurConfig motionBlurConfig_;
    DepthOfFieldConfig depthOfFieldConfig_;
    
    // 效果
    std::unique_ptr<BloomEffect> bloomEffect_;
    std::unique_ptr<ToneMappingEffect> toneMappingEffect_;
    std::unique_ptr<SSAOEffect> ssaoEffect_;
    std::unique_ptr<FXAAEffect> fxaaEffect_;
    std::unique_ptr<ColorGradingEffect> colorGradingEffect_;
    std::unique_ptr<TAAEffect> taaEffect_;
    std::unique_ptr<MotionBlurEffect> motionBlurEffect_;
    std::unique_ptr<DepthOfFieldEffect> depthOfFieldEffect_;
    
    // 中间纹理
    std::vector<TextureHandle> intermediateTextures_;
    std::vector<FrameBufferHandle> intermediateFrameBuffers_;
    
    const IBLData* ibl_ = nullptr;
    
    Stats stats_;
    
    ProgramHandle fullscreenQuadProgram_;
    BufferHandle fullscreenQuadVB_;
    BufferHandle fullscreenQuadIB_;
    
    void createFullscreenQuad();
    void updateUniforms();
};

/**
 * @brief 色调映射函数
 */
namespace ToneMapping {
    Color reinhard(const Color& hdr, float exposure);
    Color reinhard2(const Color& hdr, float exposure, float whitePoint);
    Color aces(const Color& hdr, float exposure);
    Color acesApprox(const Color& hdr, float exposure);
    Color uncharted2(const Color& hdr, float exposure);
    Color hejlDawson(const Color& hdr, float exposure);
    Color hable(const Color& hdr, float exposure);
    Color neutral(const Color& hdr, float exposure);
    
    /**
     * @brief 应用色调映射
     */
    Color apply(const Color& hdr, ToneMappingAlgorithm algorithm, 
                float exposure, float gamma);
}

} // namespace render
} // namespace phoenix
