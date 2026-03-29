#pragma once

#include "phoenix/render/Types.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>

namespace phoenix {
namespace render {

/**
 * @brief 内置着色器编译结果缓存
 */
struct CompiledShader {
    std::vector<uint32_t> spirv;      // SPIR-V 二进制数据
    std::string source;                // 原始 GLSL 源码
    uint64_t sourceHash = 0;           // 源码哈希值（用于热重载检测）
    bool isValid = false;              // 编译是否成功
    std::string errorMessage;          // 编译错误信息（如果有）
};

/**
 * @brief 内置着色器类型枚举
 */
enum class BuiltinShaderType : uint8_t {
    BaseVertex = 0,
    BaseFragment,
    PBRVertex,
    PBRFragment,
    SkyboxVertex,
    SkyboxFragment,
    DebugLine,
    // IBL vertex shaders
    IBLVertex,
    IBLBRDFVertex,
    // IBL fragment shaders
    IBLEquirectangularToCubemap,
    IBLIrradianceConvolution,
    IBLPrefilterConvolution,
    IBLBRDFLUT,
    Count  // 必须保持在最后
};

/**
 * @brief BuiltinShaders - 内置着色器管理系统
 * 
 * 负责：
 * 1. 加载内置着色器源码（从 assets/shaders/）
 * 2. 使用 shaderc 编译为 SPIR-V
 * 3. 缓存编译结果避免重复编译
 * 4. 提供着色器访问接口
 * 5. 支持热重载检测
 */
class BuiltinShaders {
public:
    BuiltinShaders();
    ~BuiltinShaders();
    
    // 禁止拷贝
    BuiltinShaders(const BuiltinShaders&) = delete;
    BuiltinShaders& operator=(const BuiltinShaders&) = delete;
    
    /**
     * @brief 初始化内置着色器系统
     * @param shaderPath 着色器源文件目录路径
     * @return 是否初始化成功
     */
    bool initialize(const std::string& shaderPath = "assets/shaders");
    
    /**
     * @brief 关闭并释放资源
     */
    void shutdown();
    
    /**
     * @brief 获取指定类型的编译后着色器
     * @param type 着色器类型
     * @return 编译后的着色器数据，如果失败返回 nullptr
     */
    const CompiledShader* getShader(BuiltinShaderType type) const;
    
    /**
     * @brief 获取着色器名称
     * @param type 着色器类型
     * @return 着色器文件名（不含扩展名）
     */
    static const char* getShaderName(BuiltinShaderType type);
    
    /**
     * @brief 重新编译所有着色器（用于热重载）
     * @return 成功编译的数量
     */
    int reloadAllShaders();
    
    /**
     * @brief 检查并重新编译已修改的着色器
     * @return 重新编译的数量
     */
    int checkAndReloadModified();
    
    /**
     * @brief 获取编译统计信息
     */
    struct Stats {
        size_t totalShaders = 0;
        size_t compiledShaders = 0;
        size_t failedShaders = 0;
        size_t cacheHits = 0;
        size_t cacheMisses = 0;
    };
    Stats getStats() const;
    
    /**
     * @brief 判断是否已初始化
     */
    bool isInitialized() const { return initialized_; }

private:
    /**
     * @brief 加载单个着色器源码
     */
    bool loadShaderSource(BuiltinShaderType type, const std::string& basePath);
    
    /**
     * @brief 编译单个着色器为 SPIR-V
     */
    bool compileShader(BuiltinShaderType type);
    
    /**
     * @brief 计算文件哈希值（用于检测修改）
     */
    uint64_t computeFileHash(const std::string& filepath) const;
    
    /**
     * @brief 获取着色器文件路径
     */
    std::string getShaderFilePath(BuiltinShaderType type, const std::string& basePath) const;
    
    // 着色器缓存
    mutable std::unordered_map<BuiltinShaderType, CompiledShader> shaderCache_;
    
    // 文件路径缓存
    std::unordered_map<BuiltinShaderType, std::string> shaderPaths_;
    
    // 基础路径
    std::string basePath_;
    
    // 初始化状态
    bool initialized_ = false;
    
    // 统计信息
    mutable Stats stats_;
    
    // 线程安全
    mutable std::mutex mutex_;
};

} // namespace render
} // namespace phoenix
