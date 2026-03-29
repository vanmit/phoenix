#include "phoenix/render/BuiltinShaders.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <shaderc/shaderc.hpp>
#include <functional>

namespace phoenix {
namespace render {

// ==================== 静态辅助函数 ====================

const char* BuiltinShaders::getShaderName(BuiltinShaderType type) {
    switch (type) {
        case BuiltinShaderType::BaseVertex:    return "base_vertex";
        case BuiltinShaderType::BaseFragment:  return "base_fragment";
        case BuiltinShaderType::PBRVertex:     return "pbr_vertex";
        case BuiltinShaderType::PBRFragment:   return "pbr_fragment";
        case BuiltinShaderType::SkyboxVertex:  return "skybox_vertex";
        case BuiltinShaderType::SkyboxFragment: return "skybox_fragment";
        case BuiltinShaderType::DebugLine:     return "debug_line";
        // IBL vertex shaders
        case BuiltinShaderType::IBLVertex:     return "ibl_cubemap_vertex";
        case BuiltinShaderType::IBLBRDFVertex: return "ibl_brdf_vertex";
        // IBL fragment shaders
        case BuiltinShaderType::IBLEquirectangularToCubemap: return "ibl_equirectangular_to_cubemap";
        case BuiltinShaderType::IBLIrradianceConvolution:    return "ibl_irradiance_convolution";
        case BuiltinShaderType::IBLPrefilterConvolution:     return "ibl_prefilter_convolution";
        case BuiltinShaderType::IBLBRDFLUT:                  return "ibl_brdf_lut";
        default:                               return "unknown";
    }
}

std::string BuiltinShaders::getShaderFilePath(BuiltinShaderType type, const std::string& basePath) const {
    return basePath + "/" + getShaderName(type) + ".glsl";
}

// ==================== 构造函数/析构函数 ====================

BuiltinShaders::BuiltinShaders() 
    : initialized_(false)
    , stats_{} {
}

BuiltinShaders::~BuiltinShaders() {
    shutdown();
}

// ==================== 初始化/关闭 ====================

bool BuiltinShaders::initialize(const std::string& shaderPath) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (initialized_) {
        return true;
    }
    
    basePath_ = shaderPath;
    
    // 检查目录是否存在
    if (!std::filesystem::exists(basePath_)) {
        // 尝试相对路径（相对于可执行文件目录）
        std::string altPath = "./" + basePath_;
        if (!std::filesystem::exists(altPath)) {
            // 尝试从源码目录
            altPath = "../" + basePath_;
            if (!std::filesystem::exists(altPath)) {
                return false;
            }
        }
        basePath_ = altPath;
    }
    
    // 初始化统计
    stats_.totalShaders = static_cast<size_t>(BuiltinShaderType::Count);
    
    // 加载并编译所有内置着色器
    int successCount = 0;
    for (uint8_t i = 0; i < static_cast<uint8_t>(BuiltinShaderType::Count); ++i) {
        BuiltinShaderType type = static_cast<BuiltinShaderType>(i);
        
        if (loadShaderSource(type, basePath_) && compileShader(type)) {
            ++successCount;
        }
    }
    
    stats_.compiledShaders = successCount;
    stats_.failedShaders = stats_.totalShaders - successCount;
    
    initialized_ = (successCount > 0);
    return initialized_;
}

void BuiltinShaders::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    shaderCache_.clear();
    shaderPaths_.clear();
    basePath_.clear();
    initialized_ = false;
}

// ==================== 着色器加载 ====================

bool BuiltinShaders::loadShaderSource(BuiltinShaderType type, const std::string& basePath) {
    std::string filepath = getShaderFilePath(type, basePath);
    shaderPaths_[type] = filepath;
    
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    
    CompiledShader& shader = shaderCache_[type];
    shader.source = buffer.str();
    shader.sourceHash = computeFileHash(filepath);
    shader.isValid = false;  // 尚未编译
    
    return !shader.source.empty();
}

// ==================== 着色器编译 ====================

bool BuiltinShaders::compileShader(BuiltinShaderType type) {
    auto it = shaderCache_.find(type);
    if (it == shaderCache_.end()) {
        return false;
    }
    
    CompiledShader& shader = it->second;
    
    if (shader.source.empty()) {
        shader.errorMessage = "Empty shader source";
        return false;
    }
    
    // 使用 shaderc 编译
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    
    // 设置优化级别
    options.SetOptimizationLevel(shaderc_optimization_level_performance);
    
    // 设置目标环境
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
    
    // 确定着色器类型
    shaderc_shader_kind kind;
    switch (type) {
        case BuiltinShaderType::BaseVertex:
        case BuiltinShaderType::PBRVertex:
        case BuiltinShaderType::SkyboxVertex:
        case BuiltinShaderType::DebugLine:
        case BuiltinShaderType::IBLVertex:
        case BuiltinShaderType::IBLBRDFVertex:
            kind = shaderc_vertex_shader;
            break;
            
        case BuiltinShaderType::BaseFragment:
        case BuiltinShaderType::PBRFragment:
        case BuiltinShaderType::SkyboxFragment:
        case BuiltinShaderType::IBLEquirectangularToCubemap:
        case BuiltinShaderType::IBLIrradianceConvolution:
        case BuiltinShaderType::IBLPrefilterConvolution:
        case BuiltinShaderType::IBLBRDFLUT:
            kind = shaderc_fragment_shader;
            break;
            
        default:
            shader.errorMessage = "Unknown shader type";
            return false;
    }
    
    // 编译
    auto result = compiler.CompileGlslToSpv(
        shader.source,
        kind,
        getShaderName(type),
        options
    );
    
    // 检查编译结果
    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        shader.errorMessage = std::string(result.GetErrorMessage());
        shader.isValid = false;
        return false;
    }
    
    // 保存 SPIR-V
    shader.spirv.assign(result.cbegin(), result.cend());
    shader.isValid = true;
    shader.errorMessage.clear();
    
    return true;
}

// ==================== 公共接口 ====================

const CompiledShader* BuiltinShaders::getShader(BuiltinShaderType type) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = shaderCache_.find(type);
    if (it != shaderCache_.end() && it->second.isValid) {
        ++stats_.cacheHits;
        return &it->second;
    }
    
    ++stats_.cacheMisses;
    return nullptr;
}

int BuiltinShaders::reloadAllShaders() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    int successCount = 0;
    for (uint8_t i = 0; i < static_cast<uint8_t>(BuiltinShaderType::Count); ++i) {
        BuiltinShaderType type = static_cast<BuiltinShaderType>(i);
        
        if (loadShaderSource(type, basePath_) && compileShader(type)) {
            ++successCount;
        }
    }
    
    stats_.compiledShaders = successCount;
    stats_.failedShaders = stats_.totalShaders - successCount;
    
    return successCount;
}

int BuiltinShaders::checkAndReloadModified() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    int reloadCount = 0;
    for (uint8_t i = 0; i < static_cast<uint8_t>(BuiltinShaderType::Count); ++i) {
        BuiltinShaderType type = static_cast<BuiltinShaderType>(i);
        
        auto pathIt = shaderPaths_.find(type);
        auto cacheIt = shaderCache_.find(type);
        
        if (pathIt != shaderPaths_.end() && cacheIt != shaderCache_.end()) {
            uint64_t currentHash = computeFileHash(pathIt->second);
            if (currentHash != cacheIt->second.sourceHash) {
                // 文件已修改，重新加载并编译
                if (loadShaderSource(type, basePath_) && compileShader(type)) {
                    ++reloadCount;
                }
            }
        }
    }
    
    return reloadCount;
}

BuiltinShaders::Stats BuiltinShaders::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
}

// ==================== 辅助函数 ====================

uint64_t BuiltinShaders::computeFileHash(const std::string& filepath) const {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return 0;
    }
    
    // 简单的 FNV-1a 哈希
    constexpr uint64_t FNV_OFFSET = 14695981039346656037ULL;
    constexpr uint64_t FNV_PRIME = 1099511628211ULL;
    
    uint64_t hash = FNV_OFFSET;
    char buffer[4096];
    
    while (file.read(buffer, sizeof(buffer))) {
        for (size_t i = 0; i < file.gcount(); ++i) {
            hash ^= static_cast<uint64_t>(buffer[i]);
            hash *= FNV_PRIME;
        }
    }
    
    // 处理剩余字节
    for (size_t i = 0; i < static_cast<size_t>(file.gcount()); ++i) {
        hash ^= static_cast<uint64_t>(buffer[i]);
        hash *= FNV_PRIME;
    }
    
    return hash;
}

} // namespace render
} // namespace phoenix
