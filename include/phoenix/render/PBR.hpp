#pragma once

#include "Types.hpp"
#include "RenderDevice.hpp"
#include "Resources.hpp"
#include "Shader.hpp"
#include <memory>
#include <array>
#include <string>
#include <filesystem>

namespace phoenix {
namespace render {

/**
 * @brief PBR 材质常量
 */
namespace PBRConstants {
    constexpr float EPSILON = 1e-6f;
    constexpr float PI = 3.14159265359f;
    constexpr float MAX_REFLECTANCE = 0.04f; // 电介质基础反射率
    constexpr uint32_t MAX_IBL_MIP_LEVELS = 8;
    constexpr uint32_t IBL_DIFFUSE_SIZE = 512;
    constexpr uint32_t IBL_SPECULAR_SIZE = 256;
    constexpr uint32_t IBL_BRDF_LUT_SIZE = 256;
}

/**
 * @brief Cook-Torrance BRDF 模型组件
 */
struct BRDFComponents {
    float NDF;      // 法线分布函数 (D)
    float G;        // 几何遮蔽函数 (G)
    float F;        // 菲涅尔函数 (F)
    float specular; // 最终镜面反射
    float diffuse;  // 漫反射
};

/**
 * @brief PBR 材质属性
 */
struct PBRMaterialProperties {
    // 基础颜色 (Albedo)
    Color albedo = Color(1, 1, 1, 1);
    
    // 金属度 (0 = 电介质, 1 = 金属)
    float metallic = 0.0f;
    
    // 粗糙度 (0 = 光滑, 1 = 粗糙)
    float roughness = 1.0f;
    
    // 环境光遮蔽
    float ao = 1.0f;
    
    // 法线缩放
    float normalScale = 1.0f;
    
    // 自发光颜色
    Color emissive = Color(0, 0, 0, 1);
    
    // 自发光强度
    float emissiveIntensity = 0.0f;
    
    // 清漆层 (Clear Coat)
    float clearCoat = 0.0f;
    float clearCoatRoughness = 0.0f;
    
    // 各向异性 (-1 到 1)
    float anisotropy = 0.0f;
    
    // 各向异性旋转 (0 到 1)
    float anisotropyRotation = 0.0f;
    
    // 折射率 (IOR)
    float ior = 1.5f;
    
    // 厚度 (用于次表面散射近似)
    float thickness = 0.0f;
    
    // 吸收系数 (用于透明材质)
    Color absorption = Color(0, 0, 0, 1);
    
    // 基础反射率 (替代 IOR)
    float baseReflectance = PBRConstants::MAX_REFLECTANCE;
    
    // 顶点颜色混合
    bool useVertexColor = false;
    
    // 双面渲染
    bool doubleSided = false;
    
    // Alpha 测试
    float alphaCutoff = 0.5f;
    bool alphaTest = false;
    
    // 混合模式
    enum class BlendMode {
        Opaque,
        Mask,
        Fade,
        Additive,
        Multiply
    } blendMode = BlendMode::Opaque;
};

/**
 * @brief PBR 纹理集
 */
struct PBRTextureSet {
    TextureHandle albedoMap;          // 基础颜色贴图
    TextureHandle metallicRoughnessMap; // 金属度/粗糙度贴图 (RG 通道)
    TextureHandle normalMap;          // 法线贴图
    TextureHandle aoMap;              // 环境光遮蔽贴图
    TextureHandle emissiveMap;        // 自发光贴图
    TextureHandle clearCoatMap;       // 清漆层贴图
    TextureHandle clearCoatRoughnessMap; // 清漆层粗糙度贴图
    TextureHandle anisotropyMap;      // 各向异性贴图
    TextureHandle thicknessMap;       // 厚度贴图 (SSS)
    TextureHandle iorMap;             // IOR 贴图
    
    // 纹理变换
    struct TextureTransform {
        float uvScale[2] = {1.0f, 1.0f};
        float uvOffset[2] = {0.0f, 0.0f};
        float rotation = 0.0f;
        uint32_t texCoordSet = 0;
    } transform;
    
    bool isValid() const {
        return albedoMap.valid() || metallicRoughnessMap.valid() || 
               normalMap.valid() || aoMap.valid();
    }
    
    void destroy(RenderDevice& device);
};

/**
 * @brief IBL (图像基光照) 数据
 */
struct IBLData {
    TextureHandle environmentMap;     // 环境贴图 (equirectangular)
    TextureHandle irradianceMap;      // 辐照度贴图 (diffuse IBL)
    TextureHandle prefilteredMap;     // 预过滤环境贴图 (specular IBL)
    TextureHandle brdfLUT;            // BRDF 查找表
    
    uint32_t environmentSize = 0;
    uint32_t irradianceSize = 0;
    uint32_t prefilteredSize = 0;
    uint32_t brdfLUTSize = 0;
    
    bool isValid() const {
        return environmentMap.valid() && irradianceMap.valid() && 
               prefilteredMap.valid() && brdfLUT.valid();
    }
    
    void destroy(RenderDevice& device);
};

/**
 * @brief 采样器描述
 */
struct SamplerDesc {
    // 过滤模式
    enum class Filter {
        Point,
        Bilinear,
        Trilinear,
        Anisotropic
    } minFilter = Filter::Trilinear;
    
    Filter magFilter = Filter::Trilinear;
    Filter mipFilter = Filter::Trilinear;
    
    // 地址模式
    enum class AddressMode {
        Clamp,
        Repeat,
        Mirror,
        Border,
        MirrorOnce
    } addressU = AddressMode::Repeat;
    AddressMode addressV = AddressMode::Repeat;
    AddressMode addressW = AddressMode::Repeat;
    
    // 各向异性
    uint32_t anisotropyLevel = 16;
    
    // 比较
    bool comparison = false;
    DepthFunc comparisonFunc = DepthFunc::LessEqual;
    
    // LOD
    float minLOD = 0.0f;
    float maxLOD = 100.0f;
    float lodBias = 0.0f;
    
    // 边框颜色
    Color borderColor = Color(0, 0, 0, 1);
};

/**
 * @brief 采样器
 */
class Sampler {
public:
    Sampler() = default;
    ~Sampler();
    
    bool create(RenderDevice& device, const SamplerDesc& desc);
    void destroy();
    
    [[nodiscard]] uint32_t getHandle() const { return handle_; }
    [[nodiscard]] const SamplerDesc& getDesc() const { return desc_; }
    
private:
    RenderDevice* device_ = nullptr;
    uint32_t handle_ = 0;
    SamplerDesc desc_;
};

/**
 * @brief 采样器管理器
 */
class SamplerManager {
public:
    static SamplerManager& instance();
    
    void initialize(RenderDevice& device);
    void shutdown();
    
    /**
     * @brief 获取或创建采样器
     */
    Sampler* getSampler(const SamplerDesc& desc);
    
    /**
     * @brief 获取常用采样器
     */
    Sampler* getPointSampler();
    Sampler* getLinearSampler();
    Sampler* getAnisotropicSampler();
    
private:
    RenderDevice* device_ = nullptr;
    std::vector<std::unique_ptr<Sampler>> samplers_;
    Sampler* pointSampler_ = nullptr;
    Sampler* linearSampler_ = nullptr;
    Sampler* anisotropicSampler_ = nullptr;
};

/**
 * @brief PBR 材质实例
 */
class PBRMaterial {
public:
    PBRMaterial() = default;
    ~PBRMaterial();
    
    /**
     * @brief 创建材质
     */
    bool create(RenderDevice& device, ShaderCompiler& compiler);
    
    /**
     * @brief 销毁材质
     */
    void destroy();
    
    /**
     * @brief 设置材质属性
     */
    void setProperties(const PBRMaterialProperties& props) { properties_ = props; }
    [[nodiscard]] const PBRMaterialProperties& getProperties() const { return properties_; }
    
    /**
     * @brief 设置纹理
     */
    void setTextures(const PBRTextureSet& textures) { textures_ = textures; }
    [[nodiscard]] const PBRTextureSet& getTextures() const { return textures_; }
    
    /**
     * @brief 获取程序句柄
     */
    [[nodiscard]] ProgramHandle getProgram() const { return program_; }
    
    /**
     * @brief 获取渲染状态
     */
    [[nodiscard]] const RenderState& getRenderState() const { return renderState_; }
    
    /**
     * @brief 更新 Uniform
     */
    void updateUniforms(UniformBuffer& buffer, const float* modelMatrix);
    
    /**
     * @brief 绑定纹理
     */
    void bindTextures(RenderDevice& device, uint32_t baseStage = 0);
    
private:
    RenderDevice* device_ = nullptr;
    ProgramHandle program_;
    PBRMaterialProperties properties_;
    PBRTextureSet textures_;
    RenderState renderState_;
    UniformBuffer uniformBuffer_;
};

/**
 * @brief PBR 渲染器
 */
class PBRRenderer {
public:
    PBRRenderer();
    ~PBRRenderer();
    
    /**
     * @brief 初始化
     */
    bool initialize(RenderDevice& device, ShaderCompiler& compiler);
    
    /**
     * @brief 关闭
     */
    void shutdown();
    
    /**
     * @brief 设置 IBL
     */
    void setIBL(const IBLData& ibl) { ibl_ = ibl; }
    [[nodiscard]] const IBLData& getIBL() const { return ibl_; }
    
    /**
     * @brief 生成 IBL 贴图
     */
    bool generateIBL(RenderDevice& device, TextureHandle environmentMap);
    
    /**
     * @brief 加载 IBL 从文件
     */
    bool loadIBLFromFile(RenderDevice& device, const std::filesystem::path& path);
    
    /**
     * @brief 渲染 PBR 材质
     */
    void render(RenderDevice& device, const PBRMaterial& material,
                const DrawCall& drawCall, const float* viewProj, const float* model);
    
    /**
     * @brief 设置光照参数
     */
    void setDirectionalLight(const float* direction, const Color& color, float intensity);
    void setPointLight(const float* position, const Color& color, float intensity, float radius);
    
    /**
     * @brief 设置相机位置 (用于 IBL 视差)
     */
    void setCameraPosition(const float* position);
    
private:
    RenderDevice* device_ = nullptr;
    ShaderCompiler* compiler_ = nullptr;
    
    IBLData ibl_;
    ProgramHandle pbrProgram_;
    ProgramHandle iblEquirectangularProgram_;
    ProgramHandle iblIrradianceProgram_;
    ProgramHandle iblPrefilterProgram_;
    ProgramHandle brdfLUTProgram_;
    
    struct LightData {
        float position[4];
        float direction[4];
        Color color;
        float intensity;
        float radius;
        float spotAngle;
        uint32_t type; // 0=directional, 1=point, 2=spot
        float padding[3];
    };
    
    std::array<LightData, 16> lights_;
    uint32_t lightCount_ = 0;
    
    float cameraPosition_[3] = {0, 0, 0};
    
    UniformBuffer materialUniforms_;
    UniformBuffer lightUniforms_;
    
    FrameBufferHandle tempFrameBuffer_;
    TextureHandle tempColor_;
    TextureHandle tempDepth_;
};

/**
 * @brief Cook-Torrance BRDF 计算
 */
namespace BRDF {
    /**
     * @brief 计算法线分布函数 (D) - GGX/Trowbridge-Reitz
     */
    float distributionGGX(float NdotH, float roughness);
    
    /**
     * @brief 计算几何遮蔽函数 (G) - Schlick-GGX
     */
    float geometrySchlickGGX(float NdotV, float roughness);
    float geometrySmith(float NdotV, float NdotL, float roughness);
    
    /**
     * @brief 计算菲涅尔函数 (F) - Schlick 近似
     */
    Color fresnelSchlick(float cosTheta, const Color& F0);
    
    /**
     * @brief 计算菲涅尔函数 (F) - Schlick 近似 (各向异性)
     */
    Color fresnelSchlickRoughness(float cosTheta, const Color& F0, float roughness);
    
    /**
     * @brief 完整 Cook-Torrance BRDF 计算
     */
    BRDFComponents cookTorrance(
        const float* N,  // 法线
        const float* V,  // 视线方向
        const float* L,  // 光线方向
        const float* H,  // 半程向量
        const PBRMaterialProperties& material
    );
    
    /**
     * @brief 计算漫反射 (Lambert + 能量守恒)
     */
    Color diffuseLambert(const Color& albedo, float metallic);
    
    /**
     * @brief 计算基础反射率 F0
     */
    Color calculateF0(const PBRMaterialProperties& material);
    
    /**
     * @brief 各向异性 NDF
     */
    float distributionAnisotropicGGX(
        float NdotH, float XdotH, float YdotH,
        float roughnessX, float roughnessY
    );
}

/**
 * @brief IBL 工具函数
 */
namespace IBLUtils {
    /**
     * @brief 从 Equirectangular 生成立方体贴图
     */
    bool generateCubemap(RenderDevice& device, TextureHandle source, 
                         uint32_t size, TextureHandle& outCubemap);
    
    /**
     * @brief 生成辐照度贴图 (Diffuse IBL)
     */
    bool generateIrradianceMap(RenderDevice& device, TextureHandle cubemap,
                               uint32_t size, TextureHandle& outIrradiance);
    
    /**
     * @brief 生成预过滤环境贴图 (Specular IBL)
     */
    bool generatePrefilteredMap(RenderDevice& device, TextureHandle cubemap,
                                uint32_t size, TextureHandle& outPrefiltered);
    
    /**
     * @brief 生成 BRDF 查找表
     */
    bool generateBRDFLUT(RenderDevice& device, uint32_t size, TextureHandle& outBRDFLUT);
    
    /**
     * @brief 计算辐照度卷积
     */
    void convolveDiffuse(const float* src, float* dst, uint32_t size);
    
    /**
     * @brief 计算预过滤卷积
     */
    void convolveSpecular(const float* src, float* dst, uint32_t size, 
                          float roughness, uint32_t mipLevel);
}

} // namespace render
} // namespace phoenix
