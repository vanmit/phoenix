#include "../../include/phoenix/resource/asset_loader.hpp"
#include <fstream>
#include <algorithm>

namespace phoenix::resource {

// ============ FBXLoader ============

FBXLoader::FBXLoader(const Config& config) : m_config(config) {}

std::set<std::string> FBXLoader::getSupportedExtensions() const {
    return {".fbx"};
}

AssetType FBXLoader::getAssetType() const {
    return AssetType::Model_FBX;
}

ValidationResult FBXLoader::validate(const std::string& path) const {
    // Security: Validate path
    if (path.find("..") != std::string::npos) {
        return ValidationResult::failure("Invalid path: contains '..'");
    }
    
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return ValidationResult::failure("Cannot open file: " + path);
    }
    
    std::streamsize size = file.tellg();
    if (size <= 0) {
        return ValidationResult::failure("File is empty");
    }
    
    // Check FBX header
    file.seekg(0, std::ios::beg);
    char header[23];
    file.read(header, 23);
    
    // FBX files start with "Kaydara FBX Binary\032\032"
    const char* expectedHeader = "Kaydara FBX Binary\032\032";
    if (std::memcmp(header, expectedHeader, 20) != 0) {
        // Could be ASCII FBX - check for ASCII header
        file.seekg(0, std::ios::beg);
        char asciiHeader[64];
        file.read(asciiHeader, 64);
        
        if (std::strstr(asciiHeader, "; FBX") == nullptr) {
            return ValidationResult::failure("Invalid FBX header");
        }
    }
    
    return ValidationResult::success();
}

size_t FBXLoader::estimateMemoryUsage(const std::string& path) const {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return 0;
    
    std::streamsize size = file.tellg();
    // FBX can expand significantly when loaded
    return static_cast<size_t>(size) * 5;
}

std::future<std::unique_ptr<Mesh>> FBXLoader::loadAsync(const std::string& path) {
    return std::async(std::launch::async, [this, path]() {
        return load(path);
    });
}

std::unique_ptr<Mesh> FBXLoader::load(const std::string& path) {
    auto mesh = std::make_unique<Mesh>();
    mesh->sourcePath = path;
    mesh->loadState = LoadState::Loading;
    
    // Validate first
    auto validation = validate(path);
    if (!validation.isValid) {
        mesh->loadState = LoadState::Failed;
        mesh->loadError = validation.error;
        return mesh;
    }
    
    // In actual implementation, use Assimp here:
    /*
    Assimp::Importer importer;
    
    // Set import properties
    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_READ_ALL_ANIMATIONS, m_config.loadAnimations);
    importer.SetPropertyBool(AI_CONFIG_POSTPROCESS_TRIANGULATE, m_config.triangulate);
    importer.SetPropertyBool(AI_CONFIG_POSTPROCESS_GENERATE_NORMALS, m_config.generateNormals);
    importer.SetPropertyBool(AI_CONFIG_POSTPROCESS_GENERATE_SMOOTH_NORMALS, m_config.generateNormals);
    importer.SetPropertyBool(AI_CONFIG_POSTPROCESS_CALC_TANGENT_SPACE, m_config.generateTangents);
    
    if (m_config.scale != 1.0f) {
        importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR, m_config.scale);
    }
    
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate |
        aiProcess_GenerateNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_OptimizeMeshes |
        aiProcess_JoinIdenticalVertices |
        aiProcess_SortByPType |
        (m_config.optimizeMesh ? aiProcess_ImproveCacheLocality : 0) |
        (m_config.loadAnimations ? aiProcess_PreTransformVertices : 0)
    );
    
    if (!scene) {
        mesh->loadState = LoadState::Failed;
        mesh->loadError = importer.GetErrorString();
        return mesh;
    }
    
    // Process scene...
    */
    
    // Placeholder implementation
    mesh->name = path;
    mesh->assetType = AssetType::Model_FBX;
    mesh->loadState = LoadState::Loaded;
    
    return mesh;
}

} // namespace phoenix::resource
