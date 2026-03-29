#pragma once

#include "Types.hpp"
#include "RenderDevice.hpp"
#include <shaderc/shaderc.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <filesystem>

namespace phoenix {
namespace render {

/**
 * @brief 着色器阶段
 */
enum class ShaderStage : uint8_t {
    Vertex,
    Fragment,
    Compute,
    Geometry,
    Hull,
    Domain
};

/**
 * @brief 着色器类型
 */
enum class ShaderType : uint8_t {
    GLSL,
    HLSL,
    SPIRV,
    MetalSL,
    Essence
};

/**
 * @brief Uniform 类型
 */
enum class UniformType : uint8_t {
    Int1, Int2, Int3, Int4,
    Float1, Float2, Float3, Float4,
    Mat3, Mat4,
    Sampler2D, Sampler3D, SamplerCube, Sampler2DArray,
    Image2D, Image3D, ImageCube
};

/**
 * @brief Uniform 绑定信息
 */
struct UniformInfo {
    std::string name;
    UniformType type;
    uint16_t regIndex;
    uint16_t regCount;
    uint16_t stageFlags; // ShaderStage bitmask
};

/**
 * @brief 着色器程序描述
 */
struct ProgramDesc {
    ShaderHandle vertexShader;
    ShaderHandle fragmentShader;
    ShaderHandle computeShader;
    std::vector<UniformInfo> uniforms;
    uint64_t typeHash;
};

/**
 * @brief 材质参数
 */
struct MaterialParam {
    std::string name;
    UniformType type;
    union {
        int32_t i[4];
        float f[4];
        TextureHandle texture;
    } value;
};

/**
 * @brief 材质模板定义
 */
struct MaterialTemplate {
    std::string name;
    std::string vertexShaderPath;
    std::string fragmentShaderPath;
    std::vector<UniformInfo> uniforms;
    std::vector<MaterialParam> defaultParams;
    RenderState renderState;
    
    // PBR 基础参数
    struct PBRParams {
        Color baseColor = Color(1, 1, 1, 1);
        float metallic = 0.0f;
        float roughness = 1.0f;
        float ao = 1.0f;
        TextureHandle baseColorMap;
        TextureHandle metallicRoughnessMap;
        TextureHandle normalMap;
        TextureHandle aoMap;
        TextureHandle emissiveMap;
        Color emissiveColor = Color(0, 0, 0, 1);
    } pbr;
};

/**
 * @brief SPIR-V 编译选项
 */
struct SpirvOptions {
    enum class OptimizerLevel {
        None,
        Size,
        Performance
    };

    OptimizerLevel optimization = OptimizerLevel::Performance;
    bool debugInfo = false;
    bool stripDebugInfo = true;
    std::vector<std::string> defines;
    std::vector<std::string> includePaths;
    std::string entryPoint = "main";
    ShaderStage stage = ShaderStage::Vertex;
};

/**
 * @brief 着色器编译器
 * 
 * 支持 SPIR-V 编译 (glslang/shaderc)
 * 提供着色器热重载功能
 */
class ShaderCompiler {
public:
    ShaderCompiler();
    ~ShaderCompiler();

    /**
     * @brief 初始化编译器
     */
    bool initialize();

    /**
     * @brief 关闭编译器
     */
    void shutdown();

    /**
     * @brief 编译 GLSL/HLSL 到 SPIR-V
     * @param source 着色器源码
     * @param options 编译选项
     * @param outSpirv 输出 SPIR-V 二进制
     * @param outError 错误信息
     * @return 是否成功
     */
    bool compileToSpirv(const std::string& source, const SpirvOptions& options,
                        std::vector<uint32_t>& outSpirv, std::string& outError);

    /**
     * @brief 编译 GLSL 到 SPIR-V 文件 (简化版本)
     * @param source 着色器源码
     * @param outputPath 输出文件路径
     * @param type 着色器类型
     * @return 是否成功
     */
    bool compileToSpirv(const std::string& source, const std::string& outputPath, ShaderType type);

    /**
     * @brief 编译文件到 SPIR-V
     */
    bool compileFileToSpirv(const std::filesystem::path& path, const SpirvOptions& options,
                            std::vector<uint32_t>& outSpirv, std::string& outError);

    /**
     * @brief 从 SPIR-V 创建着色器
     * @param device 渲染设备
     * @param spirv SPIR-V 二进制
     * @param stage 着色器阶段
     * @return 着色器句柄
     */
    ShaderHandle createShaderFromSpirv(RenderDevice& device, 
                                        const std::vector<uint32_t>& spirv,
                                        ShaderStage stage);

    /**
     * @brief 从文件创建着色器
     */
    ShaderHandle createShaderFromFile(RenderDevice& device,
                                       const std::filesystem::path& path,
                                       ShaderStage stage,
                                       const SpirvOptions& options = {});

    /**
     * @brief 创建着色器程序
     */
    ProgramHandle createProgram(RenderDevice& device,
                                 ShaderHandle vs, ShaderHandle fs);

    /**
     * @brief 销毁着色器
     */
    void destroyShader(ShaderHandle handle);

    /**
     * @brief 销毁程序
     */
    void destroyProgram(ProgramHandle handle);

    /**
     * @brief 热重载着色器
     * @param handle 着色器句柄
     * @param newPath 新文件路径
     * @return 是否成功
     */
    bool hotReloadShader(ShaderHandle handle, const std::filesystem::path& newPath);

    /**
     * @brief 注册热重载监听
     */
    using HotReloadCallback = std::function<void(ShaderHandle, bool success)>;
    void registerHotReloadCallback(ShaderHandle handle, HotReloadCallback callback);

    /**
     * @brief 获取 Uniform 信息
     */
    [[nodiscard]] const std::vector<UniformInfo>& getUniforms(ProgramHandle handle) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    struct ShaderEntry {
        std::filesystem::path sourcePath;
        std::vector<uint32_t> spirv;
        HotReloadCallback callback;
        uint64_t lastModified;
    };
    
    std::unordered_map<uint32_t, ShaderEntry> shaderEntries_;
    std::unordered_map<uint32_t, std::vector<UniformInfo>> programUniforms_;
    
    uint32_t toIndex(ShaderHandle handle) const { return handle.index(); }
    uint32_t toIndex(ProgramHandle handle) const { return handle.index(); }
};

/**
 * @brief Uniform 缓冲区管理
 */
class UniformBuffer {
public:
    UniformBuffer() = default;
    
    /**
     * @brief 初始化 Uniform 缓冲区
     * @param size 缓冲区大小
     */
    void initialize(uint32_t size);

    /**
     * @brief 写入数据
     * @param offset 偏移
     * @param data 数据指针
     * @param size 数据大小
     */
    void write(uint32_t offset, const void* data, uint32_t size);

    /**
     * @brief 写入单个值
     */
    void writeFloat(uint32_t offset, float value);
    void writeVec2(uint32_t offset, float x, float y);
    void writeVec3(uint32_t offset, float x, float y, float z);
    void writeVec4(uint32_t offset, float x, float y, float z, float w);
    void writeMat4(uint32_t offset, const float* matrix);

    /**
     * @brief 获取缓冲区数据
     */
    [[nodiscard]] const void* data() const { return data_.data(); }
    [[nodiscard]] void* data() { return data_.data(); }
    
    /**
     * @brief 获取缓冲区大小
     */
    [[nodiscard]] uint32_t size() const { return static_cast<uint32_t>(data_.size()); }

    /**
     * @brief 重置缓冲区
     */
    void reset();

private:
    std::vector<uint8_t> data_;
    uint32_t offset_ = 0;
};

/**
 * @brief 材质实例
 */
class Material {
public:
    Material() = default;
    
    /**
     * @brief 从模板创建材质
     */
    void createFromTemplate(const MaterialTemplate& template_);

    /**
     * @brief 设置程序
     */
    void setProgram(ProgramHandle program) { program_ = program; }

    /**
     * @brief 获取程序
     */
    [[nodiscard]] ProgramHandle getProgram() const { return program_; }

    /**
     * @brief 设置 Uniform
     */
    void setFloat(const std::string& name, float value);
    void setVec3(const std::string& name, float x, float y, float z);
    void setVec4(const std::string& name, float x, float y, float z, float w);
    void setMat4(const std::string& name, const float* matrix);
    void setTexture(const std::string& name, TextureHandle texture);

    /**
     * @brief 设置 PBR 参数
     */
    void setBaseColor(const Color& color);
    void setMetallic(float value);
    void setRoughness(float value);
    void setEmissive(const Color& color);

    /**
     * @brief 获取渲染状态
     */
    [[nodiscard]] const RenderState& getRenderState() const { return renderState_; }

    /**
     * @brief 设置渲染状态
     */
    void setRenderState(const RenderState& state) { renderState_ = state; }

private:
    ProgramHandle program_;
    RenderState renderState_;
    std::unordered_map<std::string, MaterialParam> params_;
    UniformBuffer uniformBuffer_;
};

/**
 * @brief 内置着色器
 */
class BuiltinShaders {
public:
    /**
     * @brief 创建内置着色器
     */
    static ProgramHandle createStandardShader(RenderDevice& device, ShaderCompiler& compiler);
    static ProgramHandle createPBRShader(RenderDevice& device, ShaderCompiler& compiler);
    static ProgramHandle createSkyboxShader(RenderDevice& device, ShaderCompiler& compiler);
    static ProgramHandle createDebugLineShader(RenderDevice& device, ShaderCompiler& compiler);
    
    /**
     * @brief 获取内置着色器源码
     */
    static const char* getVertexShaderSource(const char* name);
    static const char* getFragmentShaderSource(const char* name);
};

} // namespace render
} // namespace phoenix
