#include "../../include/phoenix/resource/asset_loader.hpp"
#include <fstream>
#include <sstream>
#include <cstring>
#include <algorithm>

// Forward declare tinygltf types to avoid heavy include
// In actual implementation, include: #include <tiny_gltf.h>

namespace phoenix::resource {

// ============ GLTFLoader ============

GLTFLoader::GLTFLoader(const Config& config) : m_config(config) {}

std::set<std::string> GLTFLoader::getSupportedExtensions() const {
    return {".gltf", ".glb"};
}

AssetType GLTFLoader::getAssetType() const {
    return AssetType::Model_GLTF;
}

bool GLTFLoader::canLoad(const std::string& path) const {
    if (path.size() < 5) return false;
    
    std::string ext = path.substr(path.size() - 5);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    return ext == ".gltf" || ext == ".glb";
}

ValidationResult GLTFLoader::validate(const std::string& path) const {
    // Security: Validate path
    if (path.find("..") != std::string::npos) {
        return ValidationResult::failure("Invalid path: contains '..'");
    }
    
    // Check file exists and is readable
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return ValidationResult::failure("Cannot open file: " + path);
    }
    
    std::streamsize size = file.tellg();
    
    // Check file size limits
    if (size <= 0) {
        return ValidationResult::failure("File is empty");
    }
    
    if (static_cast<size_t>(size) > m_config.maxBufferSize) {
        return ValidationResult::failure("File exceeds maximum buffer size");
    }
    
    // Read header to validate format
    file.seekg(0, std::ios::beg);
    
    if (path.size() >= 4) {
        std::string ext = path.substr(path.size() - 4);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        if (ext == ".glb") {
            // GLB binary format: check magic number
            uint32_t magic = 0;
            file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
            
            if (magic != 0x46546C67) {  // "glTF"
                return ValidationResult::failure("Invalid GLB magic number");
            }
            
            // Check version
            uint32_t version = 0;
            file.read(reinterpret_cast<char*>(&version), sizeof(version));
            
            if (version != 2) {
                return ValidationResult::failure("Unsupported glTF version: " + 
                                               std::to_string(version));
            }
        }
    }
    
    return ValidationResult::success();
}

size_t GLTFLoader::estimateMemoryUsage(const std::string& path) const {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return 0;
    
    std::streamsize size = file.tellg();
    
    // Rough estimate: 3-5x file size for decompressed data
    return static_cast<size_t>(size) * 4;
}

bool GLTFLoader::validateURI(const std::string& uri) const {
    if (!m_config.limitExternalURIs) {
        return true;
    }
    
    // Check if it's an external URI
    if (uri.find("://") != std::string::npos) {
        // Extract domain
        size_t start = uri.find("://") + 3;
        size_t end = uri.find('/', start);
        
        if (end == std::string::npos) {
            end = uri.size();
        }
        
        std::string domain = uri.substr(start, end - start);
        
        // Check against whitelist
        if (m_config.allowedExternalDomains.empty()) {
            return false;  // No domains allowed by default
        }
        
        for (const auto& allowed : m_config.allowedExternalDomains) {
            if (domain == allowed || domain.ends_with("." + allowed)) {
                return true;
            }
        }
        
        return false;
    }
    
    // Relative URI - check for path traversal
    if (uri.find("..") != std::string::npos) {
        return false;
    }
    
    return true;
}

std::future<std::unique_ptr<Mesh>> GLTFLoader::loadAsync(const std::string& path) {
    return std::async(std::launch::async, [this, path]() {
        return load(path);
    });
}

std::unique_ptr<Mesh> GLTFLoader::load(const std::string& path) {
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
    
    // In actual implementation, use tinygltf here:
    /*
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err, warn;
    
    bool ret = path.ends_with(".glb") ?
        loader.LoadBinaryFromFile(&model, &err, &warn, path) :
        loader.LoadASCIIFromFile(&model, &err, &warn, path);
    
    if (!ret) {
        mesh->loadState = LoadState::Failed;
        mesh->loadError = err;
        return mesh;
    }
    
    // Process scenes, meshes, materials, animations, skins
    */
    
    // Placeholder implementation
    mesh->name = path;
    mesh->assetType = path.ends_with(".glb") ? 
                      AssetType::Model_GLTF_BINARY : AssetType::Model_GLTF;
    mesh->loadState = LoadState::Loaded;
    
    return mesh;
}

std::unique_ptr<Mesh> GLTFLoader::loadFromMemory(const uint8_t* data, size_t size,
                                                  const std::string& basePath) {
    auto mesh = std::make_unique<Mesh>();
    mesh->sourcePath = basePath;
    mesh->loadState = LoadState::Loading;
    
    if (size < 12) {  // Minimum GLB header
        mesh->loadState = LoadState::Failed;
        mesh->loadError = "Data too small for GLB";
        return mesh;
    }
    
    // Check GLB magic
    uint32_t magic = *reinterpret_cast<const uint32_t*>(data);
    if (magic != 0x46546C67) {
        mesh->loadState = LoadState::Failed;
        mesh->loadError = "Invalid GLB magic number";
        return mesh;
    }
    
    // In actual implementation, use tinygltf LoadBinaryFromMemory
    mesh->name = basePath;
    mesh->assetType = AssetType::Model_GLTF_BINARY;
    mesh->loadState = LoadState::Loaded;
    
    return mesh;
}

// Placeholder methods - actual implementation would use tinygltf
void GLTFLoader::processNode(const void* node, Mesh& mesh, int32_t parentJoint) {
    // Implementation would recursively process glTF nodes
}

void GLTFLoader::processMesh(const void* gltfMesh, Mesh& outMesh, const std::string& basePath) {
    // Implementation would process glTF mesh primitives
}

void GLTFLoader::processMaterial(const void* gltfMaterial, Material& outMaterial) {
    // Implementation would process glTF PBR material
}

void GLTFLoader::processAnimation(const void* gltfAnimation, Mesh& outMesh) {
    // Implementation would process glTF animations
}

void GLTFLoader::processSkin(const void* gltfSkin, Mesh& outMesh) {
    // Implementation would process glTF skinning data
}

} // namespace phoenix::resource
