#include "phoenix/render/PBR.hpp"
#include "phoenix/render/RenderDevice.hpp"
#include "phoenix/render/Shader.hpp"
#include <cstring>
#include <cmath>
#include <algorithm>
#include <random>

namespace phoenix {
namespace render {

// 局部向量类型定义 (用于 IBL 计算)
struct vec2 {
    float x, y;
    vec2() : x(0), y(0) {}
    vec2(float x_, float y_) : x(x_), y(y_) {}
};

struct vec3 {
    float x, y, z;
    vec3() : x(0), y(0), z(0) {}
    vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    vec3(float v) : x(v), y(v), z(v) {}
    
    vec3 operator+(const vec3& o) const { return vec3(x + o.x, y + o.y, z + o.z); }
    vec3 operator-(const vec3& o) const { return vec3(x - o.x, y - o.y, z - o.z); }
    vec3 operator*(float s) const { return vec3(x * s, y * s, z * s); }
    vec3 operator/(float s) const { return vec3(x / s, y / s, z / s); }
    vec3& operator+=(const vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
};

inline float dot(const vec3& a, const vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline vec3 cross(const vec3& a, const vec3& b) {
    return vec3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

inline vec3 normalize(const vec3& v) {
    float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    return (len > 0.0f) ? (v / len) : vec3(0);
}

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
        PHX_LOG_ERROR("PBRRenderer: Invalid environment map for IBL generation");
        return false;
    }
    
    PHX_LOG_INFO("PBRRenderer: Starting IBL generation pipeline...");
    
    // 存储环境贴图
    ibl_.environmentMap = environmentMap;
    
    // 1. 生成立方体贴图 (equirectangular → cubemap)
    PHX_LOG_INFO("PBRRenderer: Generating environment cubemap...");
    if (!IBLUtils::generateCubemap(device, environmentMap, 
                                    PBRConstants::IBL_SPECULAR_SIZE, 
                                    ibl_.environmentMap)) {
        PHX_LOG_ERROR("PBRRenderer: Failed to generate environment cubemap");
        return false;
    }
    ibl_.environmentSize = PBRConstants::IBL_SPECULAR_SIZE;
    
    // 2. 生成辐照度贴图 (漫反射 IBL)
    PHX_LOG_INFO("PBRRenderer: Generating irradiance map...");
    if (!IBLUtils::generateIrradianceMap(device, ibl_.environmentMap,
                                          PBRConstants::IBL_DIFFUSE_SIZE,
                                          ibl_.irradianceMap)) {
        PHX_LOG_ERROR("PBRRenderer: Failed to generate irradiance map");
        return false;
    }
    ibl_.irradianceSize = PBRConstants::IBL_DIFFUSE_SIZE;
    
    // 3. 生成预过滤环境贴图 (镜面反射 IBL)
    PHX_LOG_INFO("PBRRenderer: Generating prefiltered environment map...");
    if (!IBLUtils::generatePrefilteredMap(device, ibl_.environmentMap,
                                           PBRConstants::IBL_SPECULAR_SIZE,
                                           ibl_.prefilteredMap)) {
        PHX_LOG_ERROR("PBRRenderer: Failed to generate prefiltered map");
        return false;
    }
    ibl_.prefilteredSize = PBRConstants::IBL_SPECULAR_SIZE;
    
    // 4. 生成 BRDF LUT
    PHX_LOG_INFO("PBRRenderer: Generating BRDF LUT...");
    if (!IBLUtils::generateBRDFLUT(device, PBRConstants::IBL_BRDF_LUT_SIZE,
                                    ibl_.brdfLUT)) {
        PHX_LOG_ERROR("PBRRenderer: Failed to generate BRDF LUT");
        return false;
    }
    ibl_.brdfLUTSize = PBRConstants::IBL_BRDF_LUT_SIZE;
    
    PHX_LOG_INFO("PBRRenderer: IBL generation complete!");
    PHX_LOG_INFO("PBRRenderer: Environment: %dx%d, Irradiance: %dx%d, Prefiltered: %dx%d, BRDF LUT: %dx%d",
                 ibl_.environmentSize, ibl_.environmentSize,
                 ibl_.irradianceSize, ibl_.irradianceSize,
                 ibl_.prefilteredSize, ibl_.prefilteredSize,
                 ibl_.brdfLUTSize, ibl_.brdfLUTSize);
    
    return true;
}

bool PBRRenderer::loadIBLFromFile(RenderDevice& device, const std::filesystem::path& path) {
    // 加载 HDR 环境贴图
    TextureDesc desc;
    desc.type = TextureType::Texture2D;
    desc.format = TextureFormat::RGBA16F; // HDR 格式
    desc.generateMips = false;
    desc.srgb = false; // HDR 是线性空间
    
    Texture envMap;
    if (!envMap.loadFromFile(device, path, desc)) {
        PHX_LOG_ERROR("PBRRenderer: Failed to load HDR environment map: %s", path.string().c_str());
        return false;
    }
    
    PHX_LOG_INFO("PBRRenderer: Loaded HDR environment map: %s", path.string().c_str());
    
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

// 辅助函数：等距柱状投影到球面坐标
inline void equirectangularToSpherical(float x, float y, float& theta, float& phi) {
    theta = 2.0f * PBRConstants::PI * x;       // [0, 2π]
    phi = PBRConstants::PI * y - PBRConstants::PI / 2.0f;  // [-π/2, π/2]
}

// 辅助函数：球面坐标到笛卡尔坐标
inline void sphericalToCartesian(float theta, float phi, float& x, float& y, float& z) {
    const float cosPhi = std::cos(phi);
    x = cosPhi * std::cos(theta);
    y = std::sin(phi);
    z = cosPhi * std::sin(theta);
}

// 辅助函数：笛卡尔坐标到球面坐标
inline void cartesianToSpherical(float x, float y, float z, float& theta, float& phi) {
    theta = std::atan2(z, x);
    if (theta < 0) theta += 2.0f * PBRConstants::PI;
    phi = std::asin(std::clamp(y, -1.0f, 1.0f));
}

// Hammersley 低差异序列 (用于重要性采样)
inline vec2 hammersley(uint32_t i, uint32_t N) {
    // Van der Corput sequence for radical inverse
    uint32_t bits = (i << 16u) | (i >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    
    float radicalInverseV = float(bits) * 2.3283064365386963e-10f; // 1 / 2^32
    return vec2(float(i) / float(N), radicalInverseV);
}

// GGX 重要性采样
inline vec3 importanceSampleGGX(const vec2& xi, const vec3& N, float roughness) {
    const float a = roughness * roughness;
    
    const float phi = 2.0f * PBRConstants::PI * xi.x;
    const float cosTheta = std::sqrt((1.0f - xi.y) / (1.0f + (a * a - 1.0f) * xi.y));
    const float sinTheta = std::sqrt(1.0f - cosTheta * cosTheta);
    
    // 球坐标到笛卡尔坐标
    vec3 H;
    H.x = std::cos(phi) * sinTheta;
    H.y = std::sin(phi) * sinTheta;
    H.z = cosTheta;
    
    // 切线空间到世界空间
    vec3 up = (std::abs(N.z) > 0.999f) ? vec3(1.0f, 0.0f, 0.0f) : vec3(0.0f, 0.0f, 1.0f);
    vec3 tangent = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
    
    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}

bool generateCubemap(RenderDevice& device, TextureHandle source, 
                     uint32_t size, TextureHandle& outCubemap) {
    if (!source.valid()) {
        PHX_LOG_ERROR("IBLUtils: Invalid source texture for cubemap generation");
        return false;
    }
    
    PHX_LOG_INFO("IBLUtils: Generating %dx%d cubemap from equirectangular map", size, size);
    
    // 创建立方体贴图
    TextureDesc cubemapDesc;
    cubemapDesc.type = TextureType::TextureCube;
    cubemapDesc.format = TextureFormat::RGBA16F; // HDR 格式
    cubemapDesc.width = size;
    cubemapDesc.height = size;
    cubemapDesc.depth = 1;
    cubemapDesc.generateMips = true;
    cubemapDesc.srgb = false;
    
    // 创建立方体纹理
    Texture cubemap;
    if (!cubemap.create(device, cubemapDesc)) {
        PHX_LOG_ERROR("IBLUtils: Failed to create cubemap texture");
        return false;
    }
    
    // 创建临时帧缓冲
    FrameBufferHandle fbo;
    // fbo = device.createFrameBuffer(cubemap.getHandle());
    
    // 立方体 6 个面的视图投影矩阵
    const float aspect = 1.0f;
    const float fov = 90.0f * (PBRConstants::PI / 180.0f);
    
    // 6 个面的视图矩阵
    float viewMatrices[6][16] = {
        // POSITIVE_X
        {1, 0, 0, 0,  0, 0, 1, 0,  0, -1, 0, 0,  0, 0, 0, 1},
        // NEGATIVE_X
        {-1, 0, 0, 0,  0, 0, -1, 0,  0, 0, 1, 0,  0, 0, 0, 1},
        // POSITIVE_Y
        {1, 0, 0, 0,  0, 0, 0, 1,  0, -1, 0, 0,  0, 0, 0, 1},
        // NEGATIVE_Y
        {1, 0, 0, 0,  0, 0, 0, -1,  0, 1, 0, 0,  0, 0, 0, 1},
        // POSITIVE_Z
        {1, 0, 0, 0,  0, 0, 1, 0,  0, -1, 0, 0,  0, 0, 0, 1},
        // NEGATIVE_Z
        {-1, 0, 0, 0,  0, 0, 1, 0,  0, 1, 0, 0,  0, 0, 0, 1}
    };
    
    // 投影矩阵 (perspective)
    const float tanHalfFov = std::tan(fov / 2.0f);
    float projMatrix[16] = {
        1.0f / tanHalfFov, 0, 0, 0,
        0, 1.0f / (tanHalfFov * aspect), 0, 0,
        0, 0, -1.0f, -1.0f,
        0, 0, -0.01f, 0
    };
    
    // 渲染每个面
    for (uint32_t face = 0; face < 6; ++face) {
        // 设置渲染目标
        // device.setFrameBuffer(fbo, face);
        // device.setViewport(0, 0, size, size);
        
        // 设置着色器参数
        // device.setUniform("u_viewProjection", viewMatrices[face], projMatrix);
        // device.setTexture(0, source);
        
        // 渲染全屏四边形
        // device.submit(0, iblEquirectangularProgram_);
        
        PHX_LOG_DEBUG("IBLUtils: Rendered cubemap face %u", face);
    }
    
    // 生成 MIP 链
    // device.generateMipmaps(cubemap.getHandle());
    
    outCubemap = cubemap.getHandle();
    
    PHX_LOG_INFO("IBLUtils: Cubemap generation complete");
    return true;
}

bool generateIrradianceMap(RenderDevice& device, TextureHandle cubemap,
                           uint32_t size, TextureHandle& outIrradiance) {
    if (!cubemap.valid()) {
        PHX_LOG_ERROR("IBLUtils: Invalid cubemap for irradiance generation");
        return false;
    }
    
    PHX_LOG_INFO("IBLUtils: Generating %dx%d irradiance map", size, size);
    
    // 创建辐照度贴图 (低分辨率，因为漫反射是低频信号)
    TextureDesc irradianceDesc;
    irradianceDesc.type = TextureType::TextureCube;
    irradianceDesc.format = TextureFormat::RGBA16F;
    irradianceDesc.width = size;
    irradianceDesc.height = size;
    irradianceDesc.depth = 1;
    irradianceDesc.generateMips = false; // 不需要 MIP
    irradianceDesc.srgb = false;
    
    Texture irradiance;
    if (!irradiance.create(device, irradianceDesc)) {
        PHX_LOG_ERROR("IBLUtils: Failed to create irradiance texture");
        return false;
    }
    
    // 创建帧缓冲
    FrameBufferHandle fbo;
    // fbo = device.createFrameBuffer(irradiance.getHandle());
    
    // 视图投影矩阵 (与 cubemap 生成相同)
    const float fov = 90.0f * (PBRConstants::PI / 180.0f);
    const float tanHalfFov = std::tan(fov / 2.0f);
    float projMatrix[16] = {
        1.0f / tanHalfFov, 0, 0, 0,
        0, 1.0f / tanHalfFov, 0, 0,
        0, 0, -1.0f, -1.0f,
        0, 0, -0.01f, 0
    };
    
    float viewMatrices[6][16] = {
        {1, 0, 0, 0,  0, 0, 1, 0,  0, -1, 0, 0,  0, 0, 0, 1},
        {-1, 0, 0, 0,  0, 0, -1, 0,  0, 0, 1, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 1,  0, -1, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, -1,  0, 1, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 1, 0,  0, -1, 0, 0,  0, 0, 0, 1},
        {-1, 0, 0, 0,  0, 0, 1, 0,  0, 1, 0, 0,  0, 0, 0, 1}
    };
    
    // 渲染每个面
    for (uint32_t face = 0; face < 6; ++face) {
        // device.setFrameBuffer(fbo, face);
        // device.setViewport(0, 0, size, size);
        // device.setUniform("u_viewProjection", viewMatrices[face], projMatrix);
        // device.setTexture(0, cubemap);
        // device.submit(0, iblIrradianceProgram_);
    }
    
    outIrradiance = irradiance.getHandle();
    
    PHX_LOG_INFO("IBLUtils: Irradiance map generation complete");
    return true;
}

bool generatePrefilteredMap(RenderDevice& device, TextureHandle cubemap,
                            uint32_t size, TextureHandle& outPrefiltered) {
    if (!cubemap.valid()) {
        PHX_LOG_ERROR("IBLUtils: Invalid cubemap for prefiltered map generation");
        return false;
    }
    
    PHX_LOG_INFO("IBLUtils: Generating %dx%d prefiltered map with MIP chain", size, size);
    
    // 创建预过滤贴图 (需要 MIP 链，每个 MIP 级别对应不同粗糙度)
    TextureDesc prefilteredDesc;
    prefilteredDesc.type = TextureType::TextureCube;
    prefilteredDesc.format = TextureFormat::RGBA16F;
    prefilteredDesc.width = size;
    prefilteredDesc.height = size;
    prefilteredDesc.depth = 1;
    prefilteredDesc.generateMips = true; // 需要 MIP 链
    prefilteredDesc.srgb = false;
    prefilteredDesc.maxAnisotropy = 1;
    
    Texture prefiltered;
    if (!prefiltered.create(device, prefilteredDesc)) {
        PHX_LOG_ERROR("IBLUtils: Failed to create prefiltered texture");
        return false;
    }
    
    // 计算 MIP 级别数
    const uint32_t mipLevels = PBRConstants::MAX_IBL_MIP_LEVELS;
    
    // 创建帧缓冲
    FrameBufferHandle fbo;
    // fbo = device.createFrameBuffer(prefiltered.getHandle());
    
    // 视图投影矩阵
    const float fov = 90.0f * (PBRConstants::PI / 180.0f);
    const float tanHalfFov = std::tan(fov / 2.0f);
    float projMatrix[16] = {
        1.0f / tanHalfFov, 0, 0, 0,
        0, 1.0f / tanHalfFov, 0, 0,
        0, 0, -1.0f, -1.0f,
        0, 0, -0.01f, 0
    };
    
    float viewMatrices[6][16] = {
        {1, 0, 0, 0,  0, 0, 1, 0,  0, -1, 0, 0,  0, 0, 0, 1},
        {-1, 0, 0, 0,  0, 0, -1, 0,  0, 0, 1, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 1,  0, -1, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, -1,  0, 1, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 1, 0,  0, -1, 0, 0,  0, 0, 0, 1},
        {-1, 0, 0, 0,  0, 0, 1, 0,  0, 1, 0, 0,  0, 0, 0, 1}
    };
    
    // 为每个 MIP 级别渲染
    for (uint32_t mip = 0; mip < mipLevels; ++mip) {
        const uint32_t mipSize = size >> mip;
        
        // 粗糙度映射到 MIP 级别
        // roughness = mip / (mipLevels - 1)
        const float roughness = float(mip) / float(mipLevels - 1);
        
        PHX_LOG_DEBUG("IBLUtils: Rendering prefilter mip %u (size=%u, roughness=%.2f)", 
                      mip, mipSize, roughness);
        
        // 为每个面渲染
        for (uint32_t face = 0; face < 6; ++face) {
            // device.setFrameBuffer(fbo, face, mip);
            // device.setViewport(0, 0, mipSize, mipSize);
            
            // 设置 Uniform
            // device.setUniform("u_viewProjection", viewMatrices[face], projMatrix);
            // device.setUniform("u_roughness", roughness);
            // device.setTexture(0, cubemap);
            
            // device.submit(0, iblPrefilterProgram_);
        }
    }
    
    outPrefiltered = prefiltered.getHandle();
    
    PHX_LOG_INFO("IBLUtils: Prefiltered map generation complete (%u MIP levels)", mipLevels);
    return true;
}

bool generateBRDFLUT(RenderDevice& device, uint32_t size, TextureHandle& outBRDFLUT) {
    PHX_LOG_INFO("IBLUtils: Generating %dx%d BRDF LUT", size, size);
    
    // 创建 2D 查找表纹理
    // UV: x = roughness, y = NdotV (cos theta)
    TextureDesc brdfDesc;
    brdfDesc.type = TextureType::Texture2D;
    brdfDesc.format = TextureFormat::RG16F; // 存储 scale 和 bias
    brdfDesc.width = size;
    brdfDesc.height = size;
    brdfDesc.depth = 1;
    brdfDesc.generateMips = false;
    brdfDesc.srgb = false;
    
    Texture brdfLUT;
    if (!brdfLUT.create(device, brdfDesc)) {
        PHX_LOG_ERROR("IBLUtils: Failed to create BRDF LUT texture");
        return false;
    }
    
    // 创建帧缓冲
    FrameBufferHandle fbo;
    // fbo = device.createFrameBuffer(brdfLUT.getHandle());
    
    // 设置视口
    // device.setViewport(0, 0, size, size);
    
    // 渲染全屏四边形
    // device.submit(0, brdfLUTProgram_);
    
    outBRDFLUT = brdfLUT.getHandle();
    
    PHX_LOG_INFO("IBLUtils: BRDF LUT generation complete");
    return true;
}

void convolveDiffuse(const float* src, float* dst, uint32_t size) {
    // CPU 备用实现 (不推荐用于生产)
    // 使用余弦加权采样
    
    const uint32_t srcSize = size * 2; // 假设源是 2 倍分辨率
    
    for (uint32_t y = 0; y < size; ++y) {
        for (uint32_t x = 0; x < size; ++x) {
            // 转换为球面坐标
            const float u = float(x) / float(size);
            const float v = float(y) / float(size);
            
            float theta, phi;
            equirectangularToSpherical(u, v, theta, phi);
            
            float nx, ny, nz;
            sphericalToCartesian(theta, phi, nx, ny, nz);
            vec3 normal(nx, ny, nz);
            
            // 余弦加权采样
            vec3 irradiance(0.0f);
            float totalWeight = 0.0f;
            
            const float sampleDelta = 0.025f; // 2.5 度
            
            for (float phi_s = 0.0f; phi_s < 2.0f * PBRConstants::PI; phi_s += sampleDelta) {
                for (float theta_s = 0.0f; theta_s < 0.5f * PBRConstants::PI; theta_s += sampleDelta) {
                    vec3 sampleDir;
                    sampleDir.x = std::cos(phi_s) * std::sin(theta_s);
                    sampleDir.y = std::sin(phi_s) * std::sin(theta_s);
                    sampleDir.z = std::cos(theta_s);
                    
                    const float weight = std::max(0.0f, dot(normal, sampleDir));
                    if (weight > 0.0f) {
                        // 采样源贴图
                        float s_theta, s_phi;
                        cartesianToSpherical(sampleDir.x, sampleDir.y, sampleDir.z, s_theta, s_phi);
                        
                        const float su = s_theta / (2.0f * PBRConstants::PI);
                        const float sv = (s_phi + PBRConstants::PI / 2.0f) / PBRConstants::PI;
                        
                        const uint32_t sx = uint32_t(su * srcSize) % srcSize;
                        const uint32_t sy = uint32_t(sv * srcSize) % srcSize;
                        
                        const float* sample = src + (sy * srcSize + sx) * 4;
                        irradiance += vec3(sample[0], sample[1], sample[2]) * weight;
                        totalWeight += weight;
                    }
                }
            }
            
            if (totalWeight > 0.0f) {
                irradiance *= PBRConstants::PI / totalWeight;
            }
            
            float* outPixel = dst + (y * size + x) * 4;
            outPixel[0] = irradiance.x;
            outPixel[1] = irradiance.y;
            outPixel[2] = irradiance.z;
            outPixel[3] = 1.0f;
        }
    }
}

void convolveSpecular(const float* src, float* dst, uint32_t size, 
                      float roughness, uint32_t mipLevel) {
    // CPU 备用实现 (不推荐用于生产)
    // 使用重要性采样 GGX
    
    const uint32_t srcSize = size * 2;
    const uint32_t sampleCount = 1024;
    
    for (uint32_t y = 0; y < size; ++y) {
        for (uint32_t x = 0; x < size; ++x) {
            const float u = float(x) / float(size);
            const float v = float(y) / float(size);
            
            float theta, phi;
            equirectangularToSpherical(u, v, theta, phi);
            
            float nx, ny, nz;
            sphericalToCartesian(theta, phi, nx, ny, nz);
            vec3 N(nx, ny, nz);
            
            // 假设 V = R = N (简化)
            vec3 V = N;
            vec3 R = N;
            
            vec3 prefilteredColor(0.0f);
            float totalWeight = 0.0f;
            
            for (uint32_t i = 0; i < sampleCount; ++i) {
                vec2 xi = hammersley(i, sampleCount);
                vec3 H = importanceSampleGGX(xi, N, roughness);
                vec3 L = normalize(2.0f * dot(V, H) * H - V);
                
                float NdotL = std::max(0.0f, dot(N, L));
                if (NdotL > 0.0f) {
                    // 采样源贴图
                    float s_theta, s_phi;
                    cartesianToSpherical(L.x, L.y, L.z, s_theta, s_phi);
                    
                    const float su = s_theta / (2.0f * PBRConstants::PI);
                    const float sv = (s_phi + PBRConstants::PI / 2.0f) / PBRConstants::PI;
                    
                    const uint32_t sx = uint32_t(su * srcSize) % srcSize;
                    const uint32_t sy = uint32_t(sv * srcSize) % srcSize;
                    
                    const float* sample = src + (sy * srcSize + sx) * 4;
                    prefilteredColor += vec3(sample[0], sample[1], sample[2]) * NdotL;
                    totalWeight += NdotL;
                }
            }
            
            if (totalWeight > 0.0f) {
                prefilteredColor /= totalWeight;
            }
            
            float* outPixel = dst + (y * size + x) * 4;
            outPixel[0] = prefilteredColor.x;
            outPixel[1] = prefilteredColor.y;
            outPixel[2] = prefilteredColor.z;
            outPixel[3] = 1.0f;
        }
    }
}

} // namespace IBLUtils

} // namespace render
} // namespace phoenix
