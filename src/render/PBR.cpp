#include "phoenix/render/PBR.hpp"
#include <cstring>
#include <cmath>
#include <algorithm>

namespace phoenix {
namespace render {

// ============================================================================
// PBRTextureSet Implementation
// ============================================================================

void PBRTextureSet::destroy(RenderDevice& device) {
    // 纹理由资源管理器管理，这里不清理
    albedoMap = TextureHandle();
    metallicRoughnessMap = TextureHandle();
    normalMap = TextureHandle();
    aoMap = TextureHandle();
    emissiveMap = TextureHandle();
    clearCoatMap = TextureHandle();
    clearCoatRoughnessMap = TextureHandle();
    anisotropyMap = TextureHandle();
    thicknessMap = TextureHandle();
    iorMap = TextureHandle();
}

// ============================================================================
// IBLData Implementation
// ============================================================================

void IBLData::destroy(RenderDevice& device) {
    environmentMap = TextureHandle();
    irradianceMap = TextureHandle();
    prefilteredMap = TextureHandle();
    brdfLUT = TextureHandle();
}

// ============================================================================
// Sampler Implementation
// ============================================================================

Sampler::~Sampler() {
    destroy();
}

bool Sampler::create(RenderDevice& device, const SamplerDesc& desc) {
    device_ = &device;
    desc_ = desc;
    
    // TODO: 创建实际的 bgfx 采样器
    // handle_ = bgfx::createSampler(...);
    handle_ = 1; // 临时值
    
    return handle_ != 0;
}

void Sampler::destroy() {
    if (handle_ != 0 && device_) {
        // bgfx::destroy(handle_);
        handle_ = 0;
    }
}

// ============================================================================
// SamplerManager Implementation
// ============================================================================

SamplerManager& SamplerManager::instance() {
    static SamplerManager instance;
    return instance;
}

void SamplerManager::initialize(RenderDevice& device) {
    device_ = &device;
    
    // 创建点采样
    SamplerDesc pointDesc;
    pointDesc.minFilter = SamplerDesc::Filter::Point;
    pointDesc.magFilter = SamplerDesc::Filter::Point;
    pointDesc.mipFilter = SamplerDesc::Filter::Point;
    pointSampler_ = getSampler(pointDesc);
    
    // 创建线性采样
    SamplerDesc linearDesc;
    linearDesc.minFilter = SamplerDesc::Filter::Trilinear;
    linearDesc.magFilter = SamplerDesc::Filter::Trilinear;
    linearDesc.mipFilter = SamplerDesc::Filter::Trilinear;
    linearSampler_ = getSampler(linearDesc);
    
    // 创建各向异性采样
    SamplerDesc anisoDesc;
    anisoDesc.minFilter = SamplerDesc::Filter::Anisotropic;
    anisoDesc.magFilter = SamplerDesc::Filter::Anisotropic;
    anisoDesc.mipFilter = SamplerDesc::Filter::Anisotropic;
    anisoDesc.anisotropyLevel = 16;
    anisotropicSampler_ = getSampler(anisoDesc);
}

void SamplerManager::shutdown() {
    samplers_.clear();
    pointSampler_ = nullptr;
    linearSampler_ = nullptr;
    anisotropicSampler_ = nullptr;
    device_ = nullptr;
}

Sampler* SamplerManager::getSampler(const SamplerDesc& desc) {
    // 查找现有采样器
    for (auto& sampler : samplers_) {
        const auto& existing = sampler->getDesc();
        if (existing.minFilter == desc.minFilter &&
            existing.magFilter == desc.magFilter &&
            existing.mipFilter == desc.mipFilter &&
            existing.addressU == desc.addressU &&
            existing.addressV == desc.addressV &&
            existing.addressW == desc.addressW &&
            existing.anisotropyLevel == desc.anisotropyLevel) {
            return sampler.get();
        }
    }
    
    // 创建新采样器
    auto sampler = std::make_unique<Sampler>();
    if (sampler->create(*device_, desc)) {
        return samplers_.emplace_back(std::move(sampler)).get();
    }
    
    return nullptr;
}

Sampler* SamplerManager::getPointSampler() {
    return pointSampler_;
}

Sampler* SamplerManager::getLinearSampler() {
    return linearSampler_;
}

Sampler* SamplerManager::getAnisotropicSampler() {
    return anisotropicSampler_;
}

// ============================================================================
// PBRMaterial Implementation
// ============================================================================

PBRMaterial::~PBRMaterial() {
    destroy();
}

bool PBRMaterial::create(RenderDevice& device, ShaderCompiler& compiler) {
    device_ = &device;
    
    // 创建 PBR 程序
    program_ = BuiltinShaders::createPBRShader(device, compiler);
    
    if (!program_.valid()) {
        return false;
    }
    
    // 初始化 Uniform 缓冲
    uniformBuffer_.initialize(256); // 足够存储所有材质参数
    
    // 设置默认渲染状态
    renderState_ = RenderState::opaqueState();
    
    return true;
}

void PBRMaterial::destroy() {
    uniformBuffer_.reset();
    textures_.destroy(*device_);
    // 程序由着色器编译器管理
    program_ = ProgramHandle();
    device_ = nullptr;
}

void PBRMaterial::updateUniforms(UniformBuffer& buffer, const float* modelMatrix) {
    buffer.reset();
    
    // 写入材质属性
    buffer.writeVec4(0, properties_.albedo.r, properties_.albedo.g, 
                     properties_.albedo.b, properties_.albedo.a);
    buffer.writeFloat(16, properties_.metallic);
    buffer.writeFloat(20, properties_.roughness);
    buffer.writeFloat(24, properties_.ao);
    buffer.writeFloat(28, properties_.normalScale);
    
    // 自发光
    buffer.writeVec4(32, properties_.emissive.r * properties_.emissiveIntensity,
                     properties_.emissive.g * properties_.emissiveIntensity,
                     properties_.emissive.b * properties_.emissiveIntensity,
                     properties_.emissive.a);
    
    // 清漆层
    buffer.writeFloat(48, properties_.clearCoat);
    buffer.writeFloat(52, properties_.clearCoatRoughness);
    
    // 各向异性
    buffer.writeFloat(56, properties_.anisotropy);
    buffer.writeFloat(60, properties_.anisotropyRotation);
    
    // IOR
    buffer.writeFloat(64, properties_.ior);
    
    // 基础反射率
    buffer.writeFloat(68, properties_.baseReflectance);
    
    // 纹理变换
    buffer.writeVec2(72, textures_.transform.uvScale[0], textures_.transform.uvScale[1]);
    buffer.writeVec2(80, textures_.transform.uvOffset[0], textures_.transform.uvOffset[1]);
    buffer.writeFloat(88, textures_.transform.rotation);
    buffer.writeFloat(92, static_cast<float>(textures_.transform.texCoordSet));
    
    // 模型矩阵
    if (modelMatrix) {
        buffer.writeMat4(96, modelMatrix);
    }
}

void PBRMaterial::bindTextures(RenderDevice& device, uint32_t baseStage) {
    uint32_t stage = baseStage;
    
    if (textures_.albedoMap.valid()) {
        // device.setTexture(stage++, textures_.albedoMap);
    }
    if (textures_.metallicRoughnessMap.valid()) {
        // device.setTexture(stage++, textures_.metallicRoughnessMap);
    }
    if (textures_.normalMap.valid()) {
        // device.setTexture(stage++, textures_.normalMap);
    }
    if (textures_.aoMap.valid()) {
        // device.setTexture(stage++, textures_.aoMap);
    }
    if (textures_.emissiveMap.valid()) {
        // device.setTexture(stage++, textures_.emissiveMap);
    }
}

// ============================================================================
// PBRRenderer Implementation
// ============================================================================

PBRRenderer::PBRRenderer() = default;

PBRRenderer::~PBRRenderer() {
    shutdown();
}

bool PBRRenderer::initialize(RenderDevice& device, ShaderCompiler& compiler) {
    device_ = &device;
    compiler_ = &compiler;
    
    // 创建 PBR 着色器程序
    pbrProgram_ = BuiltinShaders::createPBRShader(device, compiler);
    
    if (!pbrProgram_.valid()) {
        return false;
    }
    
    // 创建 IBL 相关程序
    // iblEquirectangularProgram_ = ...
    // iblIrradianceProgram_ = ...
    // iblPrefilterProgram_ = ...
    // brdfLUTProgram_ = ...
    
    // 初始化 Uniform 缓冲
    materialUniforms_.initialize(512);
    lightUniforms_.initialize(1024);
    
    // 初始化灯光
    std::memset(lights_.data(), 0, sizeof(lights_));
    
    return true;
}

void PBRRenderer::shutdown() {
    ibl_.destroy(*device_);
    materialUniforms_.reset();
    lightUniforms_.reset();
    
    if (tempFrameBuffer_.valid()) {
        // bgfx::destroy(tempFrameBuffer_);
        tempFrameBuffer_ = FrameBufferHandle();
    }
    
    device_ = nullptr;
    compiler_ = nullptr;
}

bool PBRRenderer::generateIBL(RenderDevice& device, TextureHandle environmentMap) {
    if (!environmentMap.valid()) {
        return false;
    }
    
    // 1. 生成立方体贴图
    // 2. 生成辐照度贴图
    // 3. 生成预过滤环境贴图
    // 4. 生成 BRDF LUT
    
    // 这里使用 IBLUtils 工具函数
    ibl_.environmentMap = environmentMap;
    
    // TODO: 实现完整的 IBL 生成管线
    // IBLUtils::generateCubemap(...)
    // IBLUtils::generateIrradianceMap(...)
    // IBLUtils::generatePrefilteredMap(...)
    // IBLUtils::generateBRDFLUT(...)
    
    return true;
}

bool PBRRenderer::loadIBLFromFile(RenderDevice& device, const std::filesystem::path& path) {
    // 加载环境贴图
    TextureDesc desc;
    desc.type = TextureType::TextureCube;
    desc.format = TextureFormat::RGBA16F;
    desc.generateMips = true;
    
    Texture envMap;
    if (!envMap.loadFromFile(device, path)) {
        return false;
    }
    
    return generateIBL(device, envMap.getHandle());
}

void PBRRenderer::render(RenderDevice& device, const PBRMaterial& material,
                         const DrawCall& drawCall, const float* viewProj, 
                         const float* model) {
    if (!material.getProgram().valid()) {
        return;
    }
    
    // 更新材质 Uniform
    UniformBuffer localUniforms;
    localUniforms.initialize(256);
    material.updateUniforms(localUniforms, model);
    
    // 绑定纹理
    material.bindTextures(device, 0);
    
    // 绑定 IBL 纹理
    if (ibl_.isValid()) {
        // device.setTexture(8, ibl_.irradianceMap);
        // device.setTexture(9, ibl_.prefilteredMap);
        // device.setTexture(10, ibl_.brdfLUT);
    }
    
    // 提交绘制
    // device.submit(0, material.getProgram());
}

void PBRRenderer::setDirectionalLight(const float* direction, const Color& color, 
                                       float intensity) {
    if (lightCount_ >= lights_.size()) {
        return;
    }
    
    auto& light = lights_[lightCount_++];
    light.type = 0; // Directional
    std::memcpy(light.direction, direction, sizeof(float) * 3);
    light.color = color;
    light.intensity = intensity;
}

void PBRRenderer::setPointLight(const float* position, const Color& color, 
                                 float intensity, float radius) {
    if (lightCount_ >= lights_.size()) {
        return;
    }
    
    auto& light = lights_[lightCount_++];
    light.type = 1; // Point
    std::memcpy(light.position, position, sizeof(float) * 3);
    light.color = color;
    light.intensity = intensity;
    light.radius = radius;
}

void PBRRenderer::setCameraPosition(const float* position) {
    std::memcpy(cameraPosition_, position, sizeof(float) * 3);
}

// ============================================================================
// BRDF Implementation
// ============================================================================

namespace BRDF {

float distributionGGX(float NdotH, float roughness) {
    const float a = roughness * roughness;
    const float a2 = a * a;
    const float NdotH2 = NdotH * NdotH;
    
    const float num = a2;
    const float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    const float result = num / (PBRConstants::PI * denom * denom);
    
    return result;
}

float geometrySchlickGGX(float NdotV, float roughness) {
    const float r = roughness + 1.0f;
    const float k = (r * r) / 8.0f;
    
    const float num = NdotV;
    const float denom = NdotV * (1.0f - k) + k;
    
    return num / denom;
}

float geometrySmith(float NdotV, float NdotL, float roughness) {
    const float ggx2 = geometrySchlickGGX(NdotV, roughness);
    const float ggx1 = geometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

Color fresnelSchlick(float cosTheta, const Color& F0) {
    const float clampedCos = std::max(0.0f, std::min(1.0f, cosTheta));
    const float power = std::pow(1.0f - clampedCos, 5.0f);
    
    return Color(
        F0.r + (1.0f - F0.r) * power,
        F0.g + (1.0f - F0.g) * power,
        F0.b + (1.0f - F0.b) * power,
        1.0f
    );
}

Color fresnelSchlickRoughness(float cosTheta, const Color& F0, float roughness) {
    const float clampedCos = std::max(0.0f, std::min(1.0f, cosTheta));
    const float power = std::pow(1.0f - clampedCos, 5.0f);
    
    return Color(
        F0.r + (std::max(Vec3(F0) - F0, 0.0f) * power),
        F0.g + (std::max(Vec3(F0) - F0, 0.0f) * power),
        F0.b + (std::max(Vec3(F0) - F0, 0.0f) * power),
        1.0f
    );
}

BRDFComponents cookTorrance(const float* N, const float* V, const float* L, 
                             const float* H, const PBRMaterialProperties& material) {
    BRDFComponents result;
    
    // 计算点积
    const float NdotL = std::max(N[0]*L[0] + N[1]*L[1] + N[2]*L[2], 0.0f);
    const float NdotV = std::max(N[0]*V[0] + N[1]*V[1] + N[2]*V[2], 0.0f);
    const float NdotH = std::max(N[0]*H[0] + N[1]*H[1] + N[2]*H[2], 0.0f);
    const float VdotH = std::max(V[0]*H[0] + V[1]*H[1] + V[2]*H[2], 0.0f);
    
    if (NdotL <= 0.0f || NdotV <= 0.0f) {
        result.NDF = 0.0f;
        result.G = 0.0f;
        result.F = 0.0f;
        result.specular = 0.0f;
        result.diffuse = 0.0f;
        return result;
    }
    
    // 计算 F0
    const Color F0 = calculateF0(material);
    
    // NDF - GGX
    result.NDF = distributionGGX(NdotH, material.roughness);
    
    // G - Geometry
    result.G = geometrySmith(NdotV, NdotL, material.roughness);
    
    // F - Fresnel
    const Color F = fresnelSchlick(VdotH, F0);
    result.F = F.r; // 简化为标量
    
    // Cook-Torrance BRDF
    const float numerator = result.NDF * result.G * result.F;
    const float denominator = 4.0f * NdotV * NdotL + PBRConstants::EPSILON;
    result.specular = numerator / std::max(denominator, PBRConstants::EPSILON);
    
    // 能量守恒的漫反射
    result.diffuse = (1.0f - result.F) * (1.0f - material.metallic);
    
    return result;
}

Color diffuseLambert(const Color& albedo, float metallic) {
    const float diffuseFactor = 1.0f - metallic;
    return Color(
        albedo.r * diffuseFactor / PBRConstants::PI,
        albedo.g * diffuseFactor / PBRConstants::PI,
        albedo.b * diffuseFactor / PBRConstants::PI,
        albedo.a
    );
}

Color calculateF0(const PBRMaterialProperties& material) {
    // 电介质的基础反射率
    const float baseReflectance = material.baseReflectance;
    
    // 金属的 F0 就是 albedo
    return Color(
        baseReflectance + (material.albedo.r - baseReflectance) * material.metallic,
        baseReflectance + (material.albedo.g - baseReflectance) * material.metallic,
        baseReflectance + (material.albedo.b - baseReflectance) * material.metallic,
        1.0f
    );
}

float distributionAnisotropicGGX(float NdotH, float XdotH, float YdotH,
                                  float roughnessX, float roughnessY) {
    const float aX = roughnessX * roughnessX;
    const float aY = roughnessY * roughnessY;
    
    const float aX2 = aX * aX;
    const float aY2 = aY * aY;
    
    const float num = 1.0f;
    const float denom = PBRConstants::PI * aX * aY;
    const float elliptic = XdotH * XdotH / aX2 + YdotH * YdotH / aY2 + NdotH * NdotH;
    
    return num / (denom * elliptic * elliptic);
}

} // namespace BRDF

// ============================================================================
// IBLUtils Implementation
// ============================================================================

namespace IBLUtils {

bool generateCubemap(RenderDevice& device, TextureHandle source, 
                     uint32_t size, TextureHandle& outCubemap) {
    // TODO: 实现 equirectangular 到 cubemap 的转换
    return true;
}

bool generateIrradianceMap(RenderDevice& device, TextureHandle cubemap,
                           uint32_t size, TextureHandle& outIrradiance) {
    // TODO: 实现辐照度卷积
    return true;
}

bool generatePrefilteredMap(RenderDevice& device, TextureHandle cubemap,
                            uint32_t size, TextureHandle& outPrefiltered) {
    // TODO: 实现预过滤卷积 (多 mip 级别)
    return true;
}

bool generateBRDFLUT(RenderDevice& device, uint32_t size, TextureHandle& outBRDFLUT) {
    // TODO: 实现 BRDF LUT 生成
    return true;
}

void convolveDiffuse(const float* src, float* dst, uint32_t size) {
    // CPU 卷积实现 (备用)
    // 实际应该用 GPU compute shader
}

void convolveSpecular(const float* src, float* dst, uint32_t size, 
                      float roughness, uint32_t mipLevel) {
    // CPU 卷积实现 (备用)
}

} // namespace IBLUtils

} // namespace render
} // namespace phoenix
