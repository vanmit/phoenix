#include "phoenix/render/DeferredRenderer.hpp"
#include <cstring>
#include <cmath>
#include <algorithm>

namespace phoenix {
namespace render {

// ============================================================================
// Light Implementation
// ============================================================================

void Light::getBoundingBox(float min[3], float max[3]) const {
    switch (type) {
        case LightType::Directional:
            // 方向光无边界 (无限远)
            min[0] = min[1] = min[2] = -std::numeric_limits<float>::max();
            max[0] = max[1] = max[2] = std::numeric_limits<float>::max();
            break;
            
        case LightType::Point:
            min[0] = position[0] - range;
            min[1] = position[1] - range;
            min[2] = position[2] - range;
            max[0] = position[0] + range;
            max[1] = position[1] + range;
            max[2] = position[2] + range;
            break;
            
        case LightType::Spot: {
            const float halfAngle = spotAngle * 0.5f * (3.14159f / 180.0f);
            const float radius = std::tan(halfAngle) * range;
            
            min[0] = position[0] - radius;
            min[1] = position[1] - radius;
            min[2] = position[2] - range;
            max[0] = position[0] + radius;
            max[1] = position[1] + radius;
            max[2] = position[2];
            break;
        }
        
        default:
            min[0] = min[1] = min[2] = 0;
            max[0] = max[1] = max[2] = 0;
    }
}

bool Light::isInRange(const float* point) const {
    switch (type) {
        case LightType::Directional:
            return true; // 总是影响
            
        case LightType::Point: {
            const float dx = point[0] - position[0];
            const float dy = point[1] - position[1];
            const float dz = point[2] - position[2];
            const float distSq = dx*dx + dy*dy + dz*dz;
            return distSq <= range * range;
        }
        
        case LightType::Spot: {
            // 检查距离
            const float dx = point[0] - position[0];
            const float dy = point[1] - position[1];
            const float dz = point[2] - position[2];
            const float distSq = dx*dx + dy*dy + dz*dz;
            if (distSq > range * range) {
                return false;
            }
            
            // 检查角度
            const float dist = std::sqrt(distSq);
            if (dist < 0.001f) {
                return true;
            }
            
            const float toPoint[3] = {dx/dist, dy/dist, dz/dist};
            const float dot = toPoint[0]*direction[0] + 
                             toPoint[1]*direction[1] + 
                             toPoint[2]*direction[2];
            
            const float cosAngle = std::cos(spotAngle * 0.5f * (3.14159f / 180.0f));
            return dot >= cosAngle;
        }
        
        default:
            return false;
    }
}

// ============================================================================
// LightManager Implementation
// ============================================================================

LightManager::LightManager() = default;

uint32_t LightManager::addLight(const Light& light) {
    uint32_t id;
    
    if (freeIndices_.empty()) {
        id = static_cast<uint32_t>(lights_.size());
        lights_.push_back(light);
        lightActive_.push_back(true);
    } else {
        id = freeIndices_.back();
        freeIndices_.pop_back();
        lights_[id] = light;
        lightActive_[id] = true;
    }
    
    return id;
}

void LightManager::removeLight(uint32_t id) {
    if (id < lights_.size() && lightActive_[id]) {
        lightActive_[id] = false;
        freeIndices_.push_back(id);
    }
}

void LightManager::updateLight(uint32_t id, const Light& light) {
    if (id < lights_.size() && lightActive_[id]) {
        lights_[id] = light;
    }
}

const Light* LightManager::getLight(uint32_t id) const {
    if (id < lights_.size() && lightActive_[id]) {
        return &lights_[id];
    }
    return nullptr;
}

uint32_t LightManager::getDirectionalLightCount() const {
    uint32_t count = 0;
    for (size_t i = 0; i < lights_.size(); ++i) {
        if (lightActive_[i] && lights_[i].type == LightType::Directional) {
            count++;
        }
    }
    return count;
}

uint32_t LightManager::getPointLightCount() const {
    uint32_t count = 0;
    for (size_t i = 0; i < lights_.size(); ++i) {
        if (lightActive_[i] && lights_[i].type == LightType::Point) {
            count++;
        }
    }
    return count;
}

uint32_t LightManager::getSpotLightCount() const {
    uint32_t count = 0;
    for (size_t i = 0; i < lights_.size(); ++i) {
        if (lightActive_[i] && lights_[i].type == LightType::Spot) {
            count++;
        }
    }
    return count;
}

void LightManager::clear() {
    lights_.clear();
    lightActive_.clear();
    freeIndices_.clear();
    lightGrid_.clear();
}

std::vector<uint32_t> LightManager::cullLights(
    const float* frustumPlanes, 
    uint32_t planeCount
) {
    std::vector<uint32_t> visible;
    
    for (size_t i = 0; i < lights_.size(); ++i) {
        if (!lightActive_[i]) continue;
        
        const auto& light = lights_[i];
        float min[3], max[3];
        light.getBoundingBox(min, max);
        
        // AABB vs 视锥体测试
        bool inside = true;
        for (uint32_t p = 0; p < planeCount; ++p) {
            const float* plane = &frustumPlanes[p * 4];
            
            // 测试最近的角点
            float testX = (plane[0] > 0) ? max[0] : min[0];
            float testY = (plane[1] > 0) ? max[1] : min[1];
            float testZ = (plane[2] > 0) ? max[2] : min[2];
            
            const float dist = plane[0]*testX + plane[1]*testY + 
                              plane[2]*testZ + plane[3];
            
            if (dist < 0) {
                inside = false;
                break;
            }
        }
        
        if (inside) {
            visible.push_back(static_cast<uint32_t>(i));
        }
    }
    
    return visible;
}

void LightManager::buildLightGrid(
    const float* viewProj,
    uint32_t screenWidth,
    uint32_t screenHeight,
    uint32_t tileSize
) {
    gridWidth_ = (screenWidth + tileSize - 1) / tileSize;
    gridHeight_ = (screenHeight + tileSize - 1) / tileSize;
    
    const uint32_t tileCount = gridWidth_ * gridHeight_;
    lightGrid_.resize(tileCount);
    
    // 清除所有 tile
    for (auto& tile : lightGrid_) {
        tile.clear();
    }
    
    // 对每个光源，找到它影响的 tile
    for (size_t i = 0; i < lights_.size(); ++i) {
        if (!lightActive_[i]) continue;
        
        const auto& light = lights_[i];
        
        // 计算光源在屏幕空间的边界
        // (简化实现)
        
        // 添加光源到重叠的 tile
        for (uint32_t ty = 0; ty < gridHeight_; ++ty) {
            for (uint32_t tx = 0; tx < gridWidth_; ++tx) {
                // 测试 tile 是否与光源相交
                // if (intersects) {
                    const uint32_t tileIndex = ty * gridWidth_ + tx;
                    if (lightGrid_[tileIndex].size() < MAX_LIGHTS_PER_TILE) {
                        lightGrid_[tileIndex].push_back(static_cast<uint32_t>(i));
                    }
                // }
            }
        }
    }
}

void LightManager::buildLightClusters(
    const float* viewProj,
    float nearPlane,
    float farPlane,
    uint32_t depthClusters,
    uint32_t xyClusters
) {
    useClustering_ = true;
    
    const uint32_t totalClusters = xyClusters * xyClusters * depthClusters;
    clusters_.resize(totalClusters);
    
    // 计算每个 cluster 的边界
    for (uint32_t z = 0; z < depthClusters; ++z) {
        const float zNorm = static_cast<float>(z) / depthClusters;
        const float zLinear = nearPlane + (farPlane - nearPlane) * zNorm;
        
        for (uint32_t y = 0; y < xyClusters; ++y) {
            for (uint32_t x = 0; x < xyClusters; ++x) {
                const uint32_t clusterIdx = z * xyClusters * xyClusters + 
                                           y * xyClusters + x;
                
                auto& cluster = clusters_[clusterIdx];
                cluster.lightCount = 0;
                
                // 计算 cluster 边界
                // (简化)
            }
        }
    }
    
    // 分配光源到 cluster
    for (size_t i = 0; i < lights_.size(); ++i) {
        if (!lightActive_[i]) continue;
        
        const auto& light = lights_[i];
        
        // 找到光源影响的 cluster
        // (简化实现)
    }
}

const uint32_t* LightManager::getTileLights(uint32_t tileX, uint32_t tileY,
                                             uint32_t& count) const {
    if (tileX >= gridWidth_ || tileY >= gridHeight_) {
        count = 0;
        return nullptr;
    }
    
    const uint32_t tileIndex = tileY * gridWidth_ + tileX;
    const auto& lights = lightGrid_[tileIndex];
    
    count = static_cast<uint32_t>(lights.size());
    return lights.data();
}

// ============================================================================
// DeferredRenderer Implementation
// ============================================================================

DeferredRenderer::DeferredRenderer() = default;

DeferredRenderer::~DeferredRenderer() {
    shutdown();
}

bool DeferredRenderer::initialize(RenderDevice& device, ShaderCompiler& compiler,
                                   const DeferredConfig& config) {
    device_ = &device;
    compiler_ = &compiler;
    config_ = config;
    
    // 创建 G-Buffer 纹理
    {
        TextureDesc desc;
        desc.type = TextureType::Texture2D;
        desc.width = config.width;
        desc.height = config.height;
        desc.renderTarget = true;
        
        // Albedo
        desc.format = config.gbufferFormat.albedo;
        albedoTexture_.create(device, desc);
        
        // Normal
        desc.format = config.gbufferFormat.normal;
        normalTexture_.create(device, desc);
        
        // Material
        desc.format = config.gbufferFormat.material;
        materialTexture_.create(device, desc);
        
        // Depth
        desc.format = config.gbufferFormat.depth;
        desc.depthStencil = true;
        depthTexture_.create(device, desc);
    }
    
    // 创建 G-Buffer 帧缓冲
    {
        FrameBufferDesc desc;
        desc.width = config.width;
        desc.height = config.height;
        desc.colorAttachments = {albedoTexture_, normalTexture_, materialTexture_};
        desc.depthAttachment = depthTexture_;
        gBufferFrameBuffer_.create(device, desc);
    }
    
    // 创建输出纹理
    {
        TextureDesc desc;
        desc.type = TextureType::Texture2D;
        desc.width = config.width;
        desc.height = config.height;
        desc.format = TextureFormat::RGBA8;
        desc.renderTarget = true;
        outputTexture_.create(device, desc);
        
        FrameBufferDesc fbDesc;
        fbDesc.width = config.width;
        fbDesc.height = config.height;
        fbDesc.colorAttachments = {outputTexture_};
        outputFrameBuffer_.create(device, fbDesc);
    }
    
    // 创建着色器程序
    // geometryProgram_ = compiler.createGeometryShader(...);
    // lightingProgram_ = compiler.createLightingShader(...);
    
    // 创建全屏四边形
    createFullscreenQuad();
    
    // 初始化 Uniform 缓冲
    geometryUniforms_.initialize(256);
    lightingUniforms_.initialize(512);
    lightDataBuffer_.initialize(MAX_LIGHTS_TOTAL * sizeof(Light));
    
    // 初始化矩阵
    std::memset(viewMatrix_, 0, sizeof(viewMatrix_));
    std::memset(projectionMatrix_, 0, sizeof(projectionMatrix_));
    std::memset(invViewMatrix_, 0, sizeof(invViewMatrix_));
    std::memset(invProjectionMatrix_, 0, sizeof(invProjectionMatrix_));
    
    // 初始化后处理堆栈
    if (!postProcessStack_.initialize(device, compiler)) {
        return false;
    }
    
    // 设置后处理分辨率
    postProcessStack_.resize(config.width, config.height);
    
    return true;
}

void DeferredRenderer::shutdown() {
    albedoTexture_ = TextureHandle();
    normalTexture_ = TextureHandle();
    materialTexture_ = TextureHandle();
    depthTexture_ = TextureHandle();
    outputTexture_ = TextureHandle();
    
    gBufferFrameBuffer_ = FrameBufferHandle();
    outputFrameBuffer_ = FrameBufferHandle();
    
    geometryUniforms_.reset();
    lightingUniforms_.reset();
    lightDataBuffer_.reset();
    
    postProcessStack_.shutdown();
    
    lightManager_.clear();
    geometryDrawCalls_.clear();
    
    device_ = nullptr;
    compiler_ = nullptr;
}

void DeferredRenderer::resize(uint32_t width, uint32_t height) {
    config_.width = width;
    config_.height = height;
    
    // 重新创建 G-Buffer 和输出纹理
    // (实现略)
    
    // 调整后处理分辨率
    postProcessStack_.resize(width, height);
}

void DeferredRenderer::beginGeometryPass(RenderDevice& device) {
    // 绑定 G-Buffer 帧缓冲
    device.setFrameBuffer(gBufferFrameBuffer_);
    
    // 清除
    device.clear(0, ClearFlags::All, Color(0,0,0,0), 1.0f);
    
    stats_.geometryDrawCalls = 0;
}

void DeferredRenderer::endGeometryPass(RenderDevice& device) {
    // 解绑帧缓冲
    device.setFrameBuffer(FrameBufferHandle());
}

void DeferredRenderer::beginLightingPass(RenderDevice& device) {
    // 绑定输出帧缓冲
    device.setFrameBuffer(outputFrameBuffer_);
    
    // 构建光源数据
    buildLightData();
    
    stats_.lightCount = lightManager_.getLights().size();
}

void DeferredRenderer::endLightingPass(RenderDevice& device) {
    // 解绑帧缓冲
    device.setFrameBuffer(FrameBufferHandle());
}

void DeferredRenderer::addGeometryDrawCall(const DrawCall& drawCall) {
    geometryDrawCalls_.push_back(drawCall);
}

uint32_t DeferredRenderer::addLight(const Light& light) {
    return lightManager_.addLight(light);
}

void DeferredRenderer::removeLight(uint32_t id) {
    lightManager_.removeLight(id);
}

void DeferredRenderer::updateLight(uint32_t id, const Light& light) {
    lightManager_.updateLight(id, light);
}

TextureHandle DeferredRenderer::getAlbedoTexture() const {
    return albedoTexture_;
}

TextureHandle DeferredRenderer::getNormalTexture() const {
    return normalTexture_;
}

TextureHandle DeferredRenderer::getMaterialTexture() const {
    return materialTexture_;
}

TextureHandle DeferredRenderer::getDepthTexture() const {
    return depthTexture_;
}

void DeferredRenderer::setViewProjection(const float* view, const float* projection) {
    std::memcpy(viewMatrix_, view, sizeof(float) * 16);
    std::memcpy(projectionMatrix_, projection, sizeof(float) * 16);
    
    // 计算逆矩阵
    // invertMatrix(view, invViewMatrix_);
    // invertMatrix(projection, invProjectionMatrix_);
}

void DeferredRenderer::setCameraPosition(const float* position) {
    std::memcpy(cameraPosition_, position, sizeof(float) * 3);
}

void DeferredRenderer::createFullscreenQuad() {
    // 创建全屏四边形顶点缓冲
    // (实现略)
}

void DeferredRenderer::buildLightData() {
    // 将光源数据写入 Uniform 缓冲
    const auto& lights = lightManager_.getLights();
    
    lightDataBuffer_.reset();
    
    uint32_t lightCount = 0;
    for (size_t i = 0; i < lights.size() && lightCount < MAX_LIGHTS_TOTAL; ++i) {
        // 写入光源数据
        lightCount++;
    }
}

// ============================================================================
// HybridRenderer Implementation
// ============================================================================

HybridRenderer::HybridRenderer() = default;

HybridRenderer::~HybridRenderer() {
    shutdown();
}

bool HybridRenderer::initialize(RenderDevice& device, ShaderCompiler& compiler,
                                 const DeferredConfig& deferredConfig) {
    device_ = &device;
    
    if (!deferredRenderer_.initialize(device, compiler, deferredConfig)) {
        return false;
    }
    
    if (!forwardRenderer_.initialize(device)) {
        return false;
    }
    
    return true;
}

void HybridRenderer::shutdown() {
    deferredRenderer_.shutdown();
    forwardRenderer_.shutdown();
    device_ = nullptr;
}

void DeferredRenderer::applyPostProcess(RenderDevice& device, TextureHandle input,
                                         TextureHandle output, uint32_t viewId) {
    // 设置 G-Buffer 纹理 (用于 SSAO 等效果)
    postProcessStack_.setGBufferTextures(normalTexture_, depthTexture_);
    
    // 渲染后处理效果链
    postProcessStack_.render(device, input, output, viewId);
}

void HybridRenderer::renderOpaque(RenderDevice& device, 
                                   const std::vector<DrawCall>& drawCalls) {
    deferredRenderer_.beginGeometryPass(device);
    
    for (const auto& drawCall : drawCalls) {
        deferredRenderer_.addGeometryDrawCall(drawCall);
    }
    
    deferredRenderer_.endGeometryPass(device);
    deferredRenderer_.beginLightingPass(device);
    deferredRenderer_.endLightingPass(device);
}

void HybridRenderer::renderTransparent(RenderDevice& device,
                                        const std::vector<DrawCall>& drawCalls) {
    // 使用延迟渲染的输出作为输入
    // 前向渲染透明物体
    
    forwardRenderer_.beginFrame();
    
    for (const auto& drawCall : drawCalls) {
        // forwardRenderer_.addDrawCall(drawCall);
    }
    
    forwardRenderer_.endFrame();
}

// ============================================================================
// GBufferUtils Implementation
// ============================================================================

namespace GBufferUtils {

void encodeNormal(const float* normal, float* outEncoded) {
    // 从 [-1,1] 编码到 [0,1]
    outEncoded[0] = normal[0] * 0.5f + 0.5f;
    outEncoded[1] = normal[1] * 0.5f + 0.5f;
    outEncoded[2] = normal[2] * 0.5f + 0.5f;
    outEncoded[3] = 1.0f;
}

void decodeNormal(const float* encoded, float* outNormal) {
    // 从 [0,1] 解码到 [-1,1]
    outNormal[0] = encoded[0] * 2.0f - 1.0f;
    outNormal[1] = encoded[1] * 2.0f - 1.0f;
    outNormal[2] = encoded[2] * 2.0f - 1.0f;
    
    // 重新归一化
    const float len = std::sqrt(outNormal[0]*outNormal[0] + 
                                outNormal[1]*outNormal[1] + 
                                outNormal[2]*outNormal[2]);
    if (len > 0.001f) {
        outNormal[0] /= len;
        outNormal[1] /= len;
        outNormal[2] /= len;
    }
}

float encodeDepth(float depth, float nearPlane, float farPlane) {
    // 非线性深度编码
    return (2.0f * nearPlane) / (farPlane + nearPlane - depth * (farPlane - nearPlane));
}

float decodeDepth(float encoded, float nearPlane, float farPlane) {
    return (2.0f * nearPlane - encoded * (farPlane + nearPlane)) / 
           (-encoded * (farPlane - nearPlane));
}

void reconstructWorldPosition(
    float screenX, float screenY,
    float depth,
    const float* invViewProj,
    float* outWorldPos
) {
    // NDC 坐标
    const float ndcX = screenX * 2.0f - 1.0f;
    const float ndcY = 1.0f - screenY * 2.0f;
    const float ndcZ = depth * 2.0f - 1.0f;
    
    // 变换到世界空间
    float homogeneous[4] = {ndcX, ndcY, ndcZ, 1.0f};
    float world[4];
    
    // multiplyMatrixVector(invViewProj, homogeneous, world);
    
    outWorldPos[0] = world[0] / world[3];
    outWorldPos[1] = world[1] / world[3];
    outWorldPos[2] = world[2] / world[3];
}

void encodeRGBM(const Color& color, float* outRGBM, float maxRange) {
    const float maxRGB = std::max({color.r, color.g, color.b});
    const float M = std::clamp(maxRGB / maxRange, 0.0f, 1.0f);
    
    outRGBM[0] = color.r / (M * maxRange);
    outRGBM[1] = color.g / (M * maxRange);
    outRGBM[2] = color.b / (M * maxRange);
    outRGBM[3] = M;
}

Color decodeRGBM(const float* rgbm, float maxRange) {
    return Color(
        rgbm[0] * rgbm[3] * maxRange,
        rgbm[1] * rgbm[3] * maxRange,
        rgbm[2] * rgbm[3] * maxRange,
        1.0f
    );
}

} // namespace GBufferUtils

} // namespace render
} // namespace phoenix
