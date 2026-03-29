#include "phoenix/render/Shader.hpp"
#include <bgfx/bgfx.h>
#include <shaderc/shaderc.hpp>
#include <cstring>
#include <algorithm>
#include <fstream>

namespace phoenix {
namespace render {

// ==================== ShaderCompiler 内部实现 ====================

struct ShaderCompiler::Impl {
    bool initialized = false;
    
    // 着色器源码缓存 (用于热重载)
    std::unordered_map<uint32_t, std::string> shaderSources;
    
    // Uniform 名称到索引的映射
    std::unordered_map<std::string, UniformHandle> uniformHandles;
    uint16_t nextUniformIndex = 0;
    
    UniformHandle getUniformHandle(const std::string& name) {
        auto it = uniformHandles.find(name);
        if (it != uniformHandles.end()) {
            return it->second;
        }
        UniformHandle handle(nextUniformIndex++);
        uniformHandles[name] = handle;
        return handle;
    }
};

// ==================== ShaderCompiler 实现 ====================

ShaderCompiler::ShaderCompiler() : impl_(std::make_unique<Impl>()) {}

ShaderCompiler::~ShaderCompiler() {
    shutdown();
}

bool ShaderCompiler::initialize() {
    if (impl_->initialized) {
        return true;
    }
    
    // glslang/shaderc 初始化
    // 注意：实际使用需要链接 glslang 和 shaderc 库
    impl_->initialized = true;
    
    return true;
}

void ShaderCompiler::shutdown() {
    shaderEntries_.clear();
    programUniforms_.clear();
    impl_->initialized = false;
}

bool ShaderCompiler::compileToSpirv(const std::string& source, const SpirvOptions& options,
                                     std::vector<uint32_t>& outSpirv, std::string& outError) {
    shaderc::Compiler compiler;
    shaderc::CompileOptions compileOptions;
    
    // Set optimization level
    switch (options.optimization) {
        case SpirvOptions::OptimizerLevel::None:
            compileOptions.SetOptimizationLevel(shaderc_optimization_level_zero);
            break;
        case SpirvOptions::OptimizerLevel::Size:
            compileOptions.SetOptimizationLevel(shaderc_optimization_level_size);
            break;
        case SpirvOptions::OptimizerLevel::Performance:
        default:
            compileOptions.SetOptimizationLevel(shaderc_optimization_level_performance);
            break;
    }
    
    // Set target environment
    compileOptions.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
    
    // Add defines
    for (const auto& define : options.defines) {
        // shaderc handles defines through source preprocessing
    }
    
    // Determine shader kind
    shaderc_shader_kind kind;
    switch (options.stage) {
        case ShaderStage::Vertex: kind = shaderc_vertex_shader; break;
        case ShaderStage::Fragment: kind = shaderc_fragment_shader; break;
        case ShaderStage::Compute: kind = shaderc_compute_shader; break;
        case ShaderStage::Geometry: kind = shaderc_geometry_shader; break;
        case ShaderStage::Hull: kind = shaderc_tess_control_shader; break;
        case ShaderStage::Domain: kind = shaderc_tess_evaluation_shader; break;
        default:
            outError = "Unknown shader stage";
            return false;
    }
    
    auto result = compiler.CompileGlslToSpv(source, kind, "shader.glsl", compileOptions);
    
    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        outError = "Shader compilation failed: " + std::string(result.GetErrorMessage());
        return false;
    }
    
    outSpirv.assign(result.cbegin(), result.cend());
    return true;
}

bool ShaderCompiler::compileToSpirv(const std::string& source, const std::string& outputPath, 
                                     ShaderType type) {
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    
    options.SetOptimizationLevel(shaderc_optimization_level_performance);
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
    
    shaderc_shader_kind kind;
    switch (type) {
        case ShaderType::Vertex: kind = shaderc_vertex_shader; break;
        case ShaderType::Fragment: kind = shaderc_fragment_shader; break;
        case ShaderType::Compute: kind = shaderc_compute_shader; break;
        default: 
            Logger::error("Unsupported shader type for GLSL compilation");
            return false;
    }
    
    auto result = compiler.CompileGlslToSpv(source, kind, "shader.glsl", options);
    
    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        Logger::error("Shader compilation failed: " + std::string(result.GetErrorMessage()));
        return false;
    }
    
    // Write SPIR-V to file
    std::ofstream out(outputPath, std::ios::binary);
    if (!out.is_open()) {
        Logger::error("Failed to open output file: " + outputPath);
        return false;
    }
    
    out.write(reinterpret_cast<const char*>(result.cbegin()), 
              result.size() * sizeof(uint32_t));
    
    if (!out.good()) {
        Logger::error("Failed to write SPIR-V to file: " + outputPath);
        return false;
    }
    
    return true;
}

bool ShaderCompiler::compileFileToSpirv(const std::filesystem::path& path, 
                                         const SpirvOptions& options,
                                         std::vector<uint32_t>& outSpirv, 
                                         std::string& outError) {
    // 读取文件
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        outError = "Failed to open file: " + path.string();
        return false;
    }
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::string source(size, '\0');
    if (!file.read(source.data(), size)) {
        outError = "Failed to read file: " + path.string();
        return false;
    }
    
    return compileToSpirv(source, options, outSpirv, outError);
}

ShaderHandle ShaderCompiler::createShaderFromSpirv(RenderDevice& device,
                                                    const std::vector<uint32_t>& spirv,
                                                    ShaderStage stage) {
    BX_UNUSED(device);
    
    bgfx::ShaderHandle handle = bgfx::createShader(
        bgfx::makeRef(spirv.data(), static_cast<uint32_t>(spirv.size() * sizeof(uint32_t)))
    );
    
    if (!handle.idx) {
        return ShaderHandle();
    }
    
    return ShaderHandle(handle.idx);
}

ShaderHandle ShaderCompiler::createShaderFromFile(RenderDevice& device,
                                                   const std::filesystem::path& path,
                                                   ShaderStage stage,
                                                   const SpirvOptions& options) {
    // 尝试加载预编译的 SPIR-V
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return ShaderHandle();
    }
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint32_t> spirv(size / sizeof(uint32_t));
    if (!file.read(reinterpret_cast<char*>(spirv.data()), size)) {
        return ShaderHandle();
    }
    
    ShaderHandle handle = createShaderFromSpirv(device, spirv, stage);
    
    // 保存用于热重载
    if (handle.valid()) {
        ShaderEntry entry;
        entry.sourcePath = path;
        entry.spirv = spirv;
        entry.lastModified = std::filesystem::last_write_time(path).time_since_epoch().count();
        shaderEntries_[handle.index()] = entry;
    }
    
    return handle;
}

ProgramHandle ShaderCompiler::createProgram(RenderDevice& device,
                                             ShaderHandle vs, ShaderHandle fs) {
    BX_UNUSED(device);
    
    if (!vs.valid() || !fs.valid()) {
        return ProgramHandle();
    }
    
    bgfx::ProgramHandle handle = bgfx::createProgram(
        static_cast<bgfx::ShaderHandle::Handle>(vs.index()),
        static_cast<bgfx::ShaderHandle::Handle>(fs.index()),
        true // destroy shaders when program is destroyed
    );
    
    if (!handle.idx) {
        return ProgramHandle();
    }
    
    ProgramHandle programHandle(handle.idx);
    
    // 创建 Uniform 信息 (简化版本)
    std::vector<UniformInfo> uniforms;
    programUniforms_[programHandle.index()] = uniforms;
    
    return programHandle;
}

void ShaderCompiler::destroyShader(ShaderHandle handle) {
    if (!handle.valid()) return;
    
    auto it = shaderEntries_.find(handle.index());
    if (it != shaderEntries_.end()) {
        shaderEntries_.erase(it);
    }
    
    bgfx::destroy(static_cast<bgfx::ShaderHandle::Handle>(handle.index()));
}

void ShaderCompiler::destroyProgram(ProgramHandle handle) {
    if (!handle.valid()) return;
    
    auto it = programUniforms_.find(handle.index());
    if (it != programUniforms_.end()) {
        programUniforms_.erase(it);
    }
    
    bgfx::destroy(static_cast<bgfx::ProgramHandle::Handle>(handle.index()));
}

bool ShaderCompiler::hotReloadShader(ShaderHandle handle, const std::filesystem::path& newPath) {
    auto it = shaderEntries_.find(handle.index());
    if (it == shaderEntries_.end()) {
        return false;
    }
    
    // 重新编译
    std::vector<uint32_t> newSpirv;
    std::string error;
    
    if (!compileFileToSpirv(newPath, SpirvOptions{}, newSpirv, error)) {
        if (it->second.callback) {
            it->second.callback(handle, false);
        }
        return false;
    }
    
    // 更新着色器
    bgfx::ShaderHandle bgfxHandle = static_cast<bgfx::ShaderHandle::Handle>(handle.index());
    bgfx::destroy(bgfxHandle);
    
    bgfxHandle = bgfx::createShader(
        bgfx::makeRef(newSpirv.data(), static_cast<uint32_t>(newSpirv.size() * sizeof(uint32_t)))
    );
    
    if (!bgfxHandle.idx) {
        return false;
    }
    
    // 更新缓存
    it->second.spirv = newSpirv;
    it->second.sourcePath = newPath;
    it->second.lastModified = std::filesystem::last_write_time(newPath).time_since_epoch().count();
    
    if (it->second.callback) {
        it->second.callback(handle, true);
    }
    
    return true;
}

void ShaderCompiler::registerHotReloadCallback(ShaderHandle handle, HotReloadCallback callback) {
    auto it = shaderEntries_.find(handle.index());
    if (it != shaderEntries_.end()) {
        it->second.callback = callback;
    }
}

const std::vector<UniformInfo>& ShaderCompiler::getUniforms(ProgramHandle handle) const {
    static std::vector<UniformInfo> empty;
    auto it = programUniforms_.find(handle.index());
    if (it != programUniforms_.end()) {
        return it->second;
    }
    return empty;
}

// ==================== UniformBuffer 实现 ====================

void UniformBuffer::initialize(uint32_t size) {
    data_.resize(size, 0);
    offset_ = 0;
}

void UniformBuffer::write(uint32_t offset, const void* data, uint32_t size) {
    if (offset + size > data_.size()) {
        data_.resize(offset + size);
    }
    std::memcpy(data_.data() + offset, data, size);
}

void UniformBuffer::writeFloat(uint32_t offset, float value) {
    write(offset, &value, sizeof(float));
}

void UniformBuffer::writeVec2(uint32_t offset, float x, float y) {
    float data[2] = {x, y};
    write(offset, data, sizeof(data));
}

void UniformBuffer::writeVec3(uint32_t offset, float x, float y, float z) {
    float data[3] = {x, y, z};
    write(offset, data, sizeof(data));
}

void UniformBuffer::writeVec4(uint32_t offset, float x, float y, float z, float w) {
    float data[4] = {x, y, z, w};
    write(offset, data, sizeof(data));
}

void UniformBuffer::writeMat4(uint32_t offset, const float* matrix) {
    write(offset, matrix, 16 * sizeof(float));
}

void UniformBuffer::reset() {
    offset_ = 0;
}

// ==================== Material 实现 ====================

void Material::createFromTemplate(const MaterialTemplate& template_) {
    renderState_ = template_.renderState;
    
    // 复制默认参数
    for (const auto& param : template_.defaultParams) {
        params_[param.name] = param;
    }
    
    // 设置 PBR 参数
    setBaseColor(template_.pbr.baseColor);
    setMetallic(template_.pbr.metallic);
    setRoughness(template_.pbr.roughness);
    setEmissive(template_.pbr.emissiveColor);
}

void Material::setFloat(const std::string& name, float value) {
    MaterialParam param;
    param.name = name;
    param.type = UniformType::Float1;
    param.value.f[0] = value;
    params_[name] = param;
}

void Material::setVec3(const std::string& name, float x, float y, float z) {
    MaterialParam param;
    param.name = name;
    param.type = UniformType::Float3;
    param.value.f[0] = x;
    param.value.f[1] = y;
    param.value.f[2] = z;
    params_[name] = param;
}

void Material::setVec4(const std::string& name, float x, float y, float z, float w) {
    MaterialParam param;
    param.name = name;
    param.type = UniformType::Float4;
    param.value.f[0] = x;
    param.value.f[1] = y;
    param.value.f[2] = z;
    param.value.f[3] = w;
    params_[name] = param;
}

void Material::setMat4(const std::string& name, const float* matrix) {
    MaterialParam param;
    param.name = name;
    param.type = UniformType::Mat4;
    std::memcpy(param.value.f, matrix, 16 * sizeof(float));
    params_[name] = param;
}

void Material::setTexture(const std::string& name, TextureHandle texture) {
    MaterialParam param;
    param.name = name;
    param.type = UniformType::Sampler2D;
    param.value.texture = texture;
    params_[name] = param;
}

void Material::setBaseColor(const Color& color) {
    setVec4("u_baseColor", color.r, color.g, color.b, color.a);
}

void Material::setMetallic(float value) {
    setFloat("u_metallic", value);
}

void Material::setRoughness(float value) {
    setFloat("u_roughness", value);
}

void Material::setEmissive(const Color& color) {
    setVec3("u_emissive", color.r, color.g, color.b);
}

// ==================== BuiltinShaders 实现 ====================

ProgramHandle BuiltinShaders::createStandardShader(RenderDevice& device, ShaderCompiler& compiler) {
    BX_UNUSED(device, compiler);
    // 返回预创建的标准着色器
    return ProgramHandle();
}

ProgramHandle BuiltinShaders::createPBRShader(RenderDevice& device, ShaderCompiler& compiler) {
    BX_UNUSED(device, compiler);
    return ProgramHandle();
}

ProgramHandle BuiltinShaders::createSkyboxShader(RenderDevice& device, ShaderCompiler& compiler) {
    BX_UNUSED(device, compiler);
    return ProgramHandle();
}

ProgramHandle BuiltinShaders::createDebugLineShader(RenderDevice& device, ShaderCompiler& compiler) {
    BX_UNUSED(device, compiler);
    return ProgramHandle();
}

const char* BuiltinShaders::getVertexShaderSource(const char* name) {
    BX_UNUSED(name);
    return nullptr;
}

const char* BuiltinShaders::getFragmentShaderSource(const char* name) {
    BX_UNUSED(name);
    return nullptr;
}

} // namespace render
} // namespace phoenix
