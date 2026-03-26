#include "phoenix/scene/gltf_loader.hpp"
#include <fstream>
#include <sstream>
#include <cstring>

// 简化的 JSON 解析（实际项目应使用 rapidjson 或 nlohmann/json）
namespace {
    // 简单的 JSON 值提取辅助函数
    std::string extractString(const std::string& json, const std::string& key) {
        std::string searchKey = "\"" + key + "\"";
        size_t pos = json.find(searchKey);
        if (pos == std::string::npos) return "";
        
        pos = json.find(':', pos);
        if (pos == std::string::npos) return "";
        
        pos = json.find('"', pos + 1);
        if (pos == std::string::npos) return "";
        
        size_t end = json.find('"', pos + 1);
        if (end == std::string::npos) return "";
        
        return json.substr(pos + 1, end - pos - 1);
    }
    
    float extractFloat(const std::string& json, const std::string& key, float defaultValue = 0.0f) {
        std::string searchKey = "\"" + key + "\"";
        size_t pos = json.find(searchKey);
        if (pos == std::string::npos) return defaultValue;
        
        pos = json.find(':', pos);
        if (pos == std::string::npos) return defaultValue;
        
        pos++;
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
        
        std::string numStr;
        while (pos < json.size() && (isdigit(json[pos]) || json[pos] == '.' || json[pos] == '-' || json[pos] == '+' || json[pos] == 'e' || json[pos] == 'E')) {
            numStr += json[pos++];
        }
        
        if (numStr.empty()) return defaultValue;
        return std::stof(numStr);
    }
    
    int extractInt(const std::string& json, const std::string& key, int defaultValue = 0) {
        return static_cast<int>(extractFloat(json, key, static_cast<float>(defaultValue)));
    }
    
    bool extractBool(const std::string& json, const std::string& key, bool defaultValue = false) {
        std::string searchKey = "\"" + key + "\"";
        size_t pos = json.find(searchKey);
        if (pos == std::string::npos) return defaultValue;
        
        pos = json.find(':', pos);
        if (pos == std::string::npos) return defaultValue;
        
        return json.find("true", pos) < json.find(',', pos);
    }
}

namespace phoenix {
namespace scene {

// ============================================================================
// GLTFLoader 内部数据结构
// ============================================================================

struct GLTFLoader::GLTFData {
    std::string jsonContent;
    std::vector<std::vector<uint8_t>> buffers;
    std::vector<std::string> bufferViews;
    std::vector<std::string> accessors;
    std::vector<std::string> nodes;
    std::vector<std::string> skins;
    std::vector<std::string> animations;
    std::vector<std::string> meshes;
    std::string basePath;
};

// ============================================================================
// GLTFLoader 实现
// ============================================================================

GLTFLoader::LoadResult GLTFLoader::loadFromFile(const std::string& filePath) {
    LoadResult result;
    result.success = false;
    
    data_ = std::make_unique<GLTFData>();
    data_->basePath = filePath.substr(0, filePath.find_last_of("/\\"));
    
    // 检查文件扩展名
    bool isGLB = filePath.size() > 4 && 
                 filePath.substr(filePath.size() - 4) == ".glb";
    
    if (isGLB) {
        if (!loadBinary(filePath)) {
            result.errorMessage = "Failed to load binary glTF";
            return result;
        }
    } else {
        // 加载 JSON 文件
        std::ifstream file(filePath);
        if (!file.is_open()) {
            result.errorMessage = "Cannot open file: " + filePath;
            return result;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        data_->jsonContent = buffer.str();
        
        // 加载外部缓冲区
        // TODO: 解析 JSON 并加载引用的缓冲区
    }
    
    // 解析数据
    if (!parseJSON(data_->jsonContent)) {
        result.errorMessage = "Failed to parse glTF JSON";
        return result;
    }
    
    // 解析各个组件
    parseSkeletons();
    parseAnimations();
    parseMorphTargets();
    parseMeshes();
    
    result.success = true;
    return result;
}

GLTFLoader::LoadResult GLTFLoader::loadFromMemory(const std::string& data, const std::string& baseDir) {
    LoadResult result;
    result.success = false;
    
    data_ = std::make_unique<GLTFData>();
    data_->jsonContent = data;
    data_->basePath = baseDir;
    
    if (!parseJSON(data)) {
        result.errorMessage = "Failed to parse glTF JSON";
        return result;
    }
    
    parseSkeletons();
    parseAnimations();
    parseMorphTargets();
    parseMeshes();
    
    result.success = true;
    return result;
}

std::vector<std::shared_ptr<AnimationClip>> GLTFLoader::loadAnimations(const std::string& filePath) {
    auto result = loadFromFile(filePath);
    return result.clips;
}

std::shared_ptr<Skeleton> GLTFLoader::loadSkeleton(const std::string& filePath) {
    auto result = loadFromFile(filePath);
    return result.skeleton;
}

bool GLTFLoader::isGLTFFile(const std::string& filePath) {
    if (filePath.size() < 5) return false;
    
    std::string ext = filePath.substr(filePath.size() - 5);
    return ext == ".gltf" || ext == ".glb";
}

std::string GLTFLoader::getGLTFVersion(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) return "";
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    return extractString(content, "version");
}

bool GLTFLoader::parseJSON(const std::string& json) {
    // 简化的 JSON 解析
    // 实际项目应使用完整的 JSON 库
    
    // 验证 glTF
    std::string assetType = extractString(json, "asset");
    if (assetType.empty()) {
        return false;
    }
    
    return true;
}

bool GLTFLoader::loadBinary(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    // 读取 glb 头部
    uint32_t magic, version, length;
    file.read(reinterpret_cast<char*>(&magic), 4);
    file.read(reinterpret_cast<char*>(&version), 4);
    file.read(reinterpret_cast<char*>(&length), 4);
    
    if (magic != 0x46546C67) {  // "glTF"
        return false;
    }
    
    // 读取 JSON chunk
    uint32_t jsonLength, jsonType;
    file.read(reinterpret_cast<char*>(&jsonLength), 4);
    file.read(reinterpret_cast<char*>(&jsonType), 4);
    
    if (jsonType != 0x4E4F534A) {  // "JSON"
        return false;
    }
    
    data_->jsonContent.resize(jsonLength);
    file.read(&data_->jsonContent[0], jsonLength);
    
    // 读取 BIN chunk（如果有）
    while (file.peek() != EOF) {
        uint32_t chunkLength, chunkType;
        file.read(reinterpret_cast<char*>(&chunkLength), 4);
        file.read(reinterpret_cast<char*>(&chunkType), 4);
        
        if (chunkType == 0x004E4942) {  // "BIN"
            std::vector<uint8_t> buffer(chunkLength);
            file.read(reinterpret_cast<char*>(buffer.data()), chunkLength);
            data_->buffers.push_back(std::move(buffer));
        } else {
            file.seekg(chunkLength, std::ios::cur);
        }
    }
    
    return true;
}

void GLTFLoader::parseSkeletons() {
    // 简化的骨骼解析
    // 实际项目需要完整的 glTF 解析
    
    if (!data_) return;
    
    // 查找 skins 数组
    size_t skinsPos = data_->jsonContent.find("\"skins\"");
    if (skinsPos == std::string::npos) {
        return;
    }
    
    // 创建骨骼
    data_->skeleton = std::make_shared<Skeleton>();
    
    // TODO: 完整解析 glTF 骨骼结构
}

void GLTFLoader::parseAnimations() {
    if (!data_) return;
    
    // 查找 animations 数组
    size_t animsPos = data_->jsonContent.find("\"animations\"");
    if (animsPos == std::string::npos) {
        return;
    }
    
    // TODO: 解析动画剪辑
    // 需要解析：
    // - samplers (输入/输出 accessor)
    // - channels (target node 和 path)
    // - 关键帧数据
}

void GLTFLoader::parseMorphTargets() {
    if (!data_) return;
    
    // 查找 meshes 中的 morph targets
    // TODO: 解析形变目标
}

void GLTFLoader::parseMeshes() {
    if (!data_) return;
    
    // 解析网格数据
    // TODO: 解析顶点、索引、骨骼权重
}

bool GLTFLoader::loadBuffer(const std::string& uri, std::vector<uint8_t>& outData) {
    // 检查是否为 data URI
    if (uri.substr(0, 5) == "data:") {
        // 解析 base64 数据
        // TODO: 实现 base64 解码
        return false;
    }
    
    // 加载外部文件
    std::string fullPath = data_->basePath + "/" + uri;
    std::ifstream file(fullPath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    outData.resize(size);
    file.read(reinterpret_cast<char*>(outData.data()), size);
    
    return true;
}

template<typename T>
bool GLTFLoader::readBuffer(size_t bufferIndex, size_t byteOffset, size_t count, std::vector<T>& outData) {
    if (bufferIndex >= data_->buffers.size()) {
        return false;
    }
    
    const auto& buffer = data_->buffers[bufferIndex];
    size_t requiredSize = byteOffset + count * sizeof(T);
    
    if (requiredSize > buffer.size()) {
        return false;
    }
    
    outData.resize(count);
    std::memcpy(outData.data(), buffer.data() + byteOffset, count * sizeof(T));
    
    return true;
}

// ============================================================================
// AnimationAssetLoader 实现
// ============================================================================

resource::AssetPtr AnimationAssetLoader::load(const std::string& path) {
    GLTFLoader loader;
    auto result = loader.loadFromFile(path);
    
    if (!result.success) {
        return nullptr;
    }
    
    // 创建资产对象
    // TODO: 实现资产封装
    
    return nullptr;
}

void AnimationAssetLoader::unload(resource::AssetPtr asset) {
    // TODO: 实现卸载
}

std::vector<std::string> AnimationAssetLoader::supportedExtensions() const {
    return {".gltf", ".glb"};
}

} // namespace scene
} // namespace phoenix
