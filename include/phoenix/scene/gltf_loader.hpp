#pragma once

#include "animation_types.hpp"
#include "skeleton.hpp"
#include "animator.hpp"
#include "../resource/asset_loader.hpp"
#include <string>
#include <vector>
#include <memory>

namespace phoenix {
namespace scene {

/**
 * @brief glTF 动画加载器
 * 
 * 支持加载 glTF 2.0 格式的模型和动画
 */
class GLTFLoader {
public:
    GLTFLoader() = default;
    ~GLTFLoader() = default;
    
    // ========================================================================
    // 加载结果
    // ========================================================================
    
    struct LoadResult {
        bool success{false};                              ///< 是否成功
        std::string errorMessage;                         ///< 错误信息
        
        std::shared_ptr<Skeleton> skeleton;               ///< 骨骼
        std::vector<std::shared_ptr<AnimationClip>> clips; ///< 动画剪辑
        std::vector<MorphTarget> morphTargets;            ///< 形变目标
        SkinnedMeshData meshData;                         ///< 网格数据
    };
    
    // ========================================================================
    // 加载方法
    // ========================================================================
    
    /**
     * @brief 从文件加载 glTF
     * @param filePath 文件路径
     * @return 加载结果
     */
    LoadResult loadFromFile(const std::string& filePath);
    
    /**
     * @brief 从内存加载 glTF
     * @param data JSON 数据
     * @param baseDir 基础目录（用于加载外部资源）
     * @return 加载结果
     */
    LoadResult loadFromMemory(const std::string& data, const std::string& baseDir = "");
    
    /**
     * @brief 仅加载动画剪辑
     */
    std::vector<std::shared_ptr<AnimationClip>> loadAnimations(const std::string& filePath);
    
    /**
     * @brief 仅加载骨骼
     */
    std::shared_ptr<Skeleton> loadSkeleton(const std::string& filePath);
    
    // ========================================================================
    // 工具方法
    // ========================================================================
    
    /**
     * @brief 检查文件是否为 glTF 格式
     */
    static bool isGLTFFile(const std::string& filePath);
    
    /**
     * @brief 获取 glTF 版本
     */
    static std::string getGLTFVersion(const std::string& filePath);
    
private:
    struct GLTFData;  // 内部数据结构
    
    std::unique_ptr<GLTFData> data_;
    
    /**
     * @brief 解析 JSON 数据
     */
    bool parseJSON(const std::string& json);
    
    /**
     * @brief 加载二进制数据（glb 格式）
     */
    bool loadBinary(const std::string& filePath);
    
    /**
     * @brief 解析骨骼
     */
    void parseSkeletons();
    
    /**
     * @brief 解析动画剪辑
     */
    void parseAnimations();
    
    /**
     * @brief 解析形变目标
     */
    void parseMorphTargets();
    
    /**
     * @brief 解析网格数据
     */
    void parseMeshes();
    
    /**
     * @brief 加载缓冲区数据
     */
    bool loadBuffer(const std::string& uri, std::vector<uint8_t>& outData);
    
    /**
     * @brief 从缓冲区读取数据
     */
    template<typename T>
    bool readBuffer(size_t bufferIndex, size_t byteOffset, size_t count, std::vector<T>& outData);
};

/**
 * @brief 动画资源加载器
 * 
 * 集成到资源管理系统
 */
class AnimationAssetLoader : public resource::IAssetLoader {
public:
    AnimationAssetLoader() = default;
    ~AnimationAssetLoader() override = default;
    
    /**
     * @brief 加载资产
     */
    resource::AssetPtr load(const std::string& path) override;
    
    /**
     * @brief 卸载资产
     */
    void unload(resource::AssetPtr asset) override;
    
    /**
     * @brief 获取支持的扩展名
     */
    std::vector<std::string> supportedExtensions() const override;
};

} // namespace scene
} // namespace phoenix
