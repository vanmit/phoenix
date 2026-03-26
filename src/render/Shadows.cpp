#include "phoenix/render/Shadows.hpp"
#include <cstring>
#include <cmath>
#include <algorithm>

namespace phoenix {
namespace render {

// ============================================================================
// CascadeShadowData Implementation
// ============================================================================

void CascadeShadowData::calculateSplits(float nearPlane, float farPlane, float lambda) {
    splitDistances[0] = nearPlane;
    
    for (uint32_t i = 1; i <= cascadeConfig_.cascadeCount; ++i) {
        const float ratio = static_cast<float>(i) / cascadeConfig_.cascadeCount;
        
        // 均匀分布
        const float uniform = nearPlane + (farPlane - nearPlane) * ratio;
        
        // 对数分布
        const float logarithmic = nearPlane * std::pow(farPlane / nearPlane, ratio);
        
        // 混合
        splitDistances[i] = uniform * (1.0f - lambda) + logarithmic * lambda;
    }
}

void CascadeShadowData::calculateCascadeFrustums(
    const float* viewProj,
    const float* invViewProj,
    uint32_t cascadeCount
) {
    // 标准视锥体角点 (NDC 空间)
    const float ndcCorners[8][3] = {
        {-1, -1, -1}, {1, -1, -1}, {-1, 1, -1}, {1, 1, -1},  // 近平面
        {-1, -1, 1},  {1, -1, 1},  {-1, 1, 1},  {1, 1, 1}    // 远平面
    };
    
    for (uint32_t i = 0; i < cascadeCount; ++i) {
        auto& cascade = bounds[i];
        
        // 计算当前级联的近平面和远平面
        const float nearDist = splitDistances[i];
        const float farDist = splitDistances[i + 1];
        
        // 变换角点到世界空间
        for (int j = 0; j < 8; ++j) {
            float ndcPos[4] = {ndcCorners[j][0], ndcCorners[j][1], 
                               (j < 4) ? -1.0f : 1.0f, 1.0f};
            
            // 根据级联调整 Z
            ndcPos[2] = (j < 4) ? -nearDist : -farDist;
            
            // 变换到世界空间
            float worldPos[4];
            // multiplyMatrixVector(invViewProj, ndcPos, worldPos);
            
            cascade.corners[j][0] = worldPos[0] / worldPos[3];
            cascade.corners[j][1] = worldPos[1] / worldPos[3];
            cascade.corners[j][2] = worldPos[2] / worldPos[3];
        }
        
        // 计算中心点和半径
        cascade.center[0] = 0;
        cascade.center[1] = 0;
        cascade.center[2] = 0;
        
        for (int j = 0; j < 8; ++j) {
            cascade.center[0] += cascade.corners[j][0];
            cascade.center[1] += cascade.corners[j][1];
            cascade.center[2] += cascade.corners[j][2];
        }
        
        cascade.center[0] /= 8.0f;
        cascade.center[1] /= 8.0f;
        cascade.center[2] /= 8.0f;
        
        cascade.radius = 0;
        for (int j = 0; j < 8; ++j) {
            const float dx = cascade.corners[j][0] - cascade.center[0];
            const float dy = cascade.corners[j][1] - cascade.center[1];
            const float dz = cascade.corners[j][2] - cascade.center[2];
            const float dist = std::sqrt(dx*dx + dy*dy + dz*dz);
            cascade.radius = std::max(cascade.radius, dist);
        }
    }
}

void CascadeShadowData::calculateLightMatrices(
    const float* lightDir,
    const float* cascadeBounds,
    uint32_t cascadeCount
) {
    for (uint32_t i = 0; i < cascadeCount; ++i) {
        const auto& cascade = bounds[i];
        
        // 计算光空间视图矩阵
        float lightView[16];
        // createLookAt(cascade.center, cascade.center + lightDir, up, lightView);
        
        // 计算最优的正交投影尺寸
        const float orthoSize = ShadowUtils::calculateOptimalOrthoSize(
            reinterpret_cast<const float(*)[3]>(cascade.corners),
            lightDir
        );
        
        bounds[i].orthoSize = orthoSize;
        
        // 创建正交投影矩阵
        float lightProj[16];
        // createOrtho(orthoSize, orthoSize, -cascade.farPlane, cascade.farPlane, lightProj);
        
        // 组合矩阵
        // multiplyMatrices(lightProj, lightView, lightMatrices[i]);
    }
}

uint32_t CascadeShadowData::getCascadeIndex(const float* viewPos) const {
    const float viewDist = std::sqrt(
        viewPos[0]*viewPos[0] + 
        viewPos[1]*viewPos[1] + 
        viewPos[2]*viewPos[2]
    );
    
    for (uint32_t i = 0; i < cascadeConfig_.cascadeCount; ++i) {
        if (viewDist < splitDistances[i + 1]) {
            return i;
        }
    }
    
    return cascadeConfig_.cascadeCount - 1;
}

// ============================================================================
// ShadowMap Implementation
// ============================================================================

ShadowMap::~ShadowMap() {
    destroy();
}

bool ShadowMap::create(RenderDevice& device, const ShadowMapDesc& desc) {
    device_ = &device;
    desc_ = desc;
    
    // 创建深度纹理
    TextureDesc texDesc;
    texDesc.type = (desc.arraySize > 1) ? TextureType::Texture2DArray : TextureType::Texture2D;
    texDesc.format = desc.useVSM ? TextureFormat::RGBA16F : desc.format;
    texDesc.width = desc.width;
    texDesc.height = desc.height;
    texDesc.depth = desc.arraySize;
    texDesc.depthStencil = true;
    
    // depthTexture_.create(device, texDesc);
    
    // 创建帧缓冲
    FrameBufferDesc fbDesc;
    fbDesc.width = desc.width;
    fbDesc.height = desc.height;
    fbDesc.depthAttachment = depthTexture_;
    
    // frameBuffer_.create(device, fbDesc);
    
    return depthTexture_.valid();
}

void ShadowMap::destroy() {
    if (device_) {
        depthTexture_ = TextureHandle();
        momentTexture_ = TextureHandle();
        frameBuffer_ = FrameBufferHandle();
    }
    device_ = nullptr;
}

void ShadowMap::resize(uint32_t width, uint32_t height) {
    if (device_) {
        destroy();
        desc_.width = width;
        desc_.height = height;
        create(*device_, desc_);
    }
}

// ============================================================================
// ShadowRenderer Implementation
// ============================================================================

ShadowRenderer::ShadowRenderer() = default;

ShadowRenderer::~ShadowRenderer() {
    shutdown();
}

bool ShadowRenderer::initialize(RenderDevice& device, ShaderCompiler& compiler) {
    device_ = &device;
    compiler_ = &compiler;
    
    // 创建阴影贴图
    ShadowMapDesc desc;
    desc.quality = ShadowQuality::High;
    desc.width = ShadowConstants::SHADOW_MAP_SIZE;
    desc.height = ShadowConstants::SHADOW_MAP_SIZE;
    desc.arraySize = ShadowConstants::MAX_CASCADES;
    desc.type = ShadowMapType::CSM;
    
    if (!shadowMap_.create(device, desc)) {
        return false;
    }
    
    // 创建着色器程序
    // shadowProgram_ = compiler.createShadowShader(...);
    // shadowVSMProgram_ = compiler.createVSMShader(...);
    // shadowArrayProgram_ = compiler.createShadowArrayShader(...);
    
    // 初始化 Uniform 缓冲
    shadowUniforms_.initialize(512);
    
    // 初始化级联配置
    cascadeConfig_.cascadeCount = ShadowConstants::MAX_CASCADES;
    cascadeConfig_.splitLambda = ShadowConstants::CASCADE_SPLIT_LAMBDA;
    cascadeConfig_.nearPlane = 0.5f;
    cascadeConfig_.farPlane = 100.0f;
    cascadeConfig_.blendWidth = 0.1f;
    
    // 初始化 PCF 配置
    pcfConfig_.kernelSize = 3;
    pcfConfig_.kernelRadius = 1.0f;
    pcfConfig_.useSoftTransition = true;
    
    return true;
}

void ShadowRenderer::shutdown() {
    shadowMap_.destroy();
    shadowUniforms_.reset();
    
    device_ = nullptr;
    compiler_ = nullptr;
}

void ShadowRenderer::setShadowQuality(ShadowQuality quality) {
    uint32_t size = ShadowConstants::SHADOW_MAP_SIZE;
    uint32_t cascades = ShadowConstants::MAX_CASCADES;
    
    switch (quality) {
        case ShadowQuality::Low:
            size = 512;
            cascades = 1;
            break;
        case ShadowQuality::Medium:
            size = 1024;
            cascades = 2;
            break;
        case ShadowQuality::High:
            size = 2048;
            cascades = 4;
            break;
        case ShadowQuality::Ultra:
            size = 4096;
            cascades = 4;
            vsmConfig_.enabled = true;
            break;
    }
    
    shadowMap_.resize(size, size);
    cascadeConfig_.cascadeCount = cascades;
}

void ShadowRenderer::beginShadowPass(RenderDevice& device) {
    inShadowPass_ = true;
    stats_.drawCallsShadow = 0;
    stats_.cascadesRendered = 0;
    stats_.culledObjects = 0;
}

void ShadowRenderer::renderShadowMap(
    RenderDevice& device,
    const float* lightDir,
    const float* viewProj,
    const std::vector<DrawCall>& drawCalls
) {
    if (!inShadowPass_) {
        return;
    }
    
    // 计算逆矩阵
    float invViewProj[16];
    // invertMatrix(viewProj, invViewProj);
    
    // 更新级联
    updateCascades(viewProj, invViewProj);
    
    // 对每个级联渲染
    for (uint32_t i = 0; i < cascadeConfig_.cascadeCount; ++i) {
        const auto& cascade = cascadeData_.bounds[i];
        
        // 设置视口到阴影贴图
        Viewport vp;
        vp.x = 0;
        vp.y = 0;
        vp.width = shadowMap_.getWidth();
        vp.height = shadowMap_.getHeight();
        // device.setViewport(vp);
        
        // 清除深度
        // device.clear(i, ClearFlags::Depth, Color(0,0,0,1), 1.0f);
        
        // 设置级联变换矩阵
        const float* lightMatrix = cascadeData_.lightMatrices[i];
        
        // 渲染可见物体
        for (const auto& drawCall : drawCalls) {
            // 检查物体是否在级联视锥体内
            // if (isVisibleInShadow(...)) {
            //     提交绘制
            // }
            stats_.drawCallsShadow++;
        }
        
        stats_.cascadesRendered++;
    }
}

void ShadowRenderer::endShadowPass(RenderDevice& device) {
    inShadowPass_ = false;
}

void ShadowRenderer::updateCascades(const float* viewProj, const float* invViewProj) {
    // 计算级联分割
    cascadeData_.calculateSplits(
        cascadeConfig_.nearPlane,
        cascadeConfig_.farPlane,
        cascadeConfig_.splitLambda
    );
    
    // 计算级联视锥体
    cascadeData_.calculateCascadeFrustums(
        viewProj, invViewProj,
        cascadeConfig_.cascadeCount
    );
    
    // 计算光空间矩阵 (假设方向光)
    float lightDir[3] = {0.5f, -1.0f, 0.3f}; // 示例
    // normalize(lightDir);
    
    cascadeData_.calculateLightMatrices(
        lightDir,
        reinterpret_cast<const float*>(cascadeData_.bounds),
        cascadeConfig_.cascadeCount
    );
}

bool ShadowRenderer::isVisibleInShadow(const float* boundsMin, const float* boundsMax,
                                        uint32_t cascadeIndex) const {
    if (!cullingConfig_.enabled) {
        return true;
    }
    
    // AABB vs 级联视锥体测试
    const auto& cascade = cascadeData_.bounds[cascadeIndex];
    
    // 简化：检查 AABB 是否在级联球体内
    float center[3] = {
        (boundsMin[0] + boundsMax[0]) * 0.5f,
        (boundsMin[1] + boundsMax[1]) * 0.5f,
        (boundsMin[2] + boundsMax[2]) * 0.5f
    };
    
    float dx = center[0] - cascade.center[0];
    float dy = center[1] - cascade.center[1];
    float dz = center[2] - cascade.center[2];
    float dist = std::sqrt(dx*dx + dy*dy + dz*dz);
    
    float radius = std::sqrt(
        std::pow((boundsMax[0] - boundsMin[0]) * 0.5f, 2) +
        std::pow((boundsMax[1] - boundsMin[1]) * 0.5f, 2) +
        std::pow((boundsMax[2] - boundsMin[2]) * 0.5f, 2)
    );
    
    return dist < (cascade.radius + radius);
}

// ============================================================================
// OmnidirectionalShadow Implementation
// ============================================================================

OmnidirectionalShadow::~OmnidirectionalShadow() {
    destroy();
}

bool OmnidirectionalShadow::create(RenderDevice& device, uint32_t size) {
    device_ = &device;
    size_ = size;
    
    // 创建立方体深度贴图
    TextureDesc desc;
    desc.type = TextureType::TextureCube;
    desc.format = TextureFormat::Depth24;
    desc.width = size;
    desc.height = size;
    desc.depth = 1;
    desc.depthStencil = true;
    
    // cubeDepth_.create(device, desc);
    
    // 创建帧缓冲
    // frameBuffer_.create(device, ...);
    
    // 初始化 6 个面的投影矩阵
    const float aspect = 1.0f;
    const float nearPlane = 0.1f;
    const float farPlane = 100.0f;
    
    // 6 个方向：+X, -X, +Y, -Y, +Z, -Z
    // createPerspectiveFOV(90°, aspect, near, far, projectionMatrices_[i]);
    
    return cubeDepth_.valid();
}

void OmnidirectionalShadow::destroy() {
    if (device_) {
        cubeDepth_ = TextureHandle();
        frameBuffer_ = FrameBufferHandle();
    }
    device_ = nullptr;
}

void OmnidirectionalShadow::render(RenderDevice& device, const float* lightPos, 
                                    float range, const std::vector<DrawCall>& drawCalls) {
    // 渲染 6 个面
    for (uint32_t face = 0; face < 6; ++face) {
        // 设置面方向
        // 渲染场景
    }
}

// ============================================================================
// ShadowUtils Implementation
// ============================================================================

namespace ShadowUtils {

float calculateOptimalOrthoSize(
    const float* frustumCorners,
    const float* lightDir,
    float padding
) {
    // 将角点变换到光空间
    float lightSpaceCorners[8][3];
    
    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    
    for (int i = 0; i < 8; ++i) {
        // 变换到光空间 (简化)
        lightSpaceCorners[i][0] = frustumCorners[i][0];
        lightSpaceCorners[i][1] = frustumCorners[i][1];
        
        minX = std::min(minX, lightSpaceCorners[i][0]);
        maxX = std::max(maxX, lightSpaceCorners[i][0]);
        minY = std::min(minY, lightSpaceCorners[i][1]);
        maxY = std::max(maxY, lightSpaceCorners[i][1]);
    }
    
    const float width = maxX - minX;
    const float height = maxY - minY;
    
    return std::max(width, height) * padding;
}

void transformToLightSpace(
    const float* point,
    const float* lightMatrix,
    float* outLightSpace
) {
    // 矩阵 * 向量变换
    // multiplyMatrixVector(lightMatrix, point, outLightSpace);
}

float calculateCascadeBlend(
    float depth,
    float cascadeNear,
    float cascadeFar,
    float blendWidth
) {
    const float blendStart = cascadeFar - blendWidth;
    
    if (depth < blendStart) {
        return 0.0f;
    }
    
    if (depth > cascadeFar) {
        return 1.0f;
    }
    
    return (depth - blendStart) / blendWidth;
}

float filterPCF(
    const float* shadowMap,
    uint32_t width, uint32_t height,
    float u, float v,
    float texelSize,
    const PCFConfig& config
) {
    const int kernelSize = config.kernelSize;
    const float halfKernel = (kernelSize - 1) * 0.5f;
    
    float shadow = 0.0f;
    float weightSum = 0.0f;
    
    for (int y = 0; y < kernelSize; ++y) {
        for (int x = 0; x < kernelSize; ++x) {
            const float offsetU = (x - halfKernel) * texelSize;
            const float offsetV = (y - halfKernel) * texelSize;
            
            // 采样深度
            // float depth = sampleShadowMap(shadowMap, u + offsetU, v + offsetV);
            
            // 比较
            // if (currentDepth > depth) shadow += weight;
            
            shadow += 1.0f; // 简化
            weightSum += 1.0f;
        }
    }
    
    return shadow / weightSum;
}

float calculateVSMVisibility(
    float depth,
    float moment1,
    float moment2,
    float minVariance,
    float lightBleedReduction
) {
    // 计算方差
    const float variance = moment2 - moment1 * moment1;
    const float clampedVariance = std::max(variance, minVariance);
    
    // Chebyshev 不等式
    const float d = depth - moment1;
    float pMax = 0.0f;
    
    if (d > 0.0f) {
        pMax = clampedVariance / (clampedVariance + d * d);
    } else {
        pMax = 1.0f;
    }
    
    // 减少光泄漏
    const float visibility = std::max(pMax, lightBleedReduction);
    
    return visibility;
}

void createShadowBiasMatrix(float* matrix, float bias) {
    // 创建从 [-1,1] 到 [0,1] 的变换矩阵
    // 并添加深度偏置
    
    // | 0.5  0    0    0   |
    // | 0    0.5  0    0   |
    // | 0    0    0.5  0   |
    // | 0.5  0.5  0.5  1   |
    
    // 添加偏置到 w 分量
}

} // namespace ShadowUtils

} // namespace render
} // namespace phoenix
