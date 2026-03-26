#include "../../include/phoenix/resource/asset_loader.hpp"
#include <fstream>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <cmath>

namespace phoenix::resource {

// ============ STLLoader ============

STLLoader::STLLoader(const Config& config) : m_config(config) {}

std::set<std::string> STLLoader::getSupportedExtensions() const {
    return {".stl"};
}

AssetType STLLoader::getAssetType() const {
    return AssetType::Model_STL;
}

ValidationResult STLLoader::validate(const std::string& path) const {
    if (path.find("..") != std::string::npos) {
        return ValidationResult::failure("Invalid path: contains '..'");
    }
    
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return ValidationResult::failure("Cannot open file: " + path);
    }
    
    // Check file size
    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    
    if (size < 84) {  // Minimum: 80 byte header + 4 byte triangle count
        return ValidationResult::failure("File too small to be valid STL");
    }
    
    // Check if binary or ASCII
    file.seekg(0, std::ios::beg);
    char header[80];
    file.read(header, 80);
    
    // ASCII STL starts with "solid"
    if (std::memcmp(header, "solid", 5) == 0) {
        // Verify it's actually ASCII by checking for more ASCII content
        file.seekg(0, std::ios::beg);
        std::string line;
        std::getline(file, line);
        
        if (line.find("solid") == std::string::npos) {
            return ValidationResult::failure("Invalid ASCII STL header");
        }
    } else {
        // Binary STL - check triangle count
        uint32_t numTriangles = 0;
        file.read(reinterpret_cast<char*>(&numTriangles), sizeof(numTriangles));
        
        // Validate triangle count matches file size
        std::streamsize expectedSize = 84 + numTriangles * 50;  // 50 bytes per triangle
        
        if (expectedSize != size) {
            // Could still be valid if there's extra data, but warn
            if (expectedSize > size) {
                return ValidationResult::failure("Binary STL triangle count doesn't match file size");
            }
        }
        
        // Sanity check on triangle count
        if (numTriangles == 0 || numTriangles > 100000000) {  // 100M triangles max
            return ValidationResult::failure("Invalid triangle count: " + std::to_string(numTriangles));
        }
    }
    
    return ValidationResult::success();
}

size_t STLLoader::estimateMemoryUsage(const std::string& path) const {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return 0;
    
    std::streamsize size = file.tellg();
    
    // Binary STL: 50 bytes per triangle, ~96 bytes per vertex after processing
    // ASCII STL: larger file size but similar vertex count
    return static_cast<size_t>(size) * 2;
}

std::future<std::unique_ptr<Mesh>> STLLoader::loadAsync(const std::string& path) {
    return std::async(std::launch::async, [this, path]() {
        return load(path);
    });
}

std::unique_ptr<Mesh> STLLoader::load(const std::string& path) {
    auto mesh = std::make_unique<Mesh>();
    mesh->sourcePath = path;
    mesh->loadState = LoadState::Loading;
    mesh->assetType = AssetType::Model_STL;
    
    // Validate first
    auto validation = validate(path);
    if (!validation.isValid) {
        mesh->loadState = LoadState::Failed;
        mesh->loadError = validation.error;
        return mesh;
    }
    
    // Detect format and load
    std::ifstream file(path, std::ios::binary);
    char header[80];
    file.read(header, 80);
    
    if (std::memcmp(header, "solid", 5) == 0) {
        // ASCII STL
        file.close();
        mesh = loadAscii(path);
    } else {
        // Binary STL
        file.close();
        mesh = loadBinary(path);
    }
    
    if (mesh && m_config.centerMesh && mesh->loadState == LoadState::Loaded) {
        // Center the mesh
        math::Vector3 center = mesh->bounds.center;
        
        for (auto& prim : mesh->primitives) {
            float* verts = reinterpret_cast<float*>(prim.vertexData.data());
            size_t vertCount = prim.vertexData.size() / (6 * sizeof(float));  // pos(3) + norm(3)
            
            for (size_t i = 0; i < vertCount; ++i) {
                verts[i * 6 + 0] -= center.x;
                verts[i * 6 + 1] -= center.y;
                verts[i * 6 + 2] -= center.z;
            }
        }
        
        mesh->bounds.min -= center;
        mesh->bounds.max -= center;
        mesh->bounds.center = math::Vector3(0.0f);
    }
    
    return mesh;
}

std::unique_ptr<Mesh> STLLoader::loadBinary(const std::string& path) {
    auto mesh = std::make_unique<Mesh>();
    mesh->sourcePath = path;
    mesh->assetType = AssetType::Model_STL;
    
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        mesh->loadState = LoadState::Failed;
        mesh->loadError = "Failed to open file";
        return mesh;
    }
    
    // Skip header
    file.seekg(80, std::ios::beg);
    
    // Read triangle count
    uint32_t numTriangles = 0;
    file.read(reinterpret_cast<char*>(&numTriangles), sizeof(numTriangles));
    
    if (numTriangles == 0) {
        mesh->loadState = LoadState::Failed;
        mesh->loadError = "No triangles in STL file";
        return mesh;
    }
    
    // Create primitive
    MeshPrimitive prim;
    prim.vertexFormat.stride = 6 * sizeof(float);  // pos(3) + norm(3)
    prim.vertexFormat.addAttribute(VertexAttribute::Position, 3, sizeof(float));
    prim.vertexFormat.addAttribute(VertexAttribute::Normal, 3, sizeof(float));
    
    std::vector<float> vertexData;
    vertexData.reserve(numTriangles * 3 * 6);  // 3 vertices per triangle, 6 floats per vertex
    
    std::vector<uint32_t> indices;
    indices.reserve(numTriangles * 3);
    
    // Read triangles
    struct BinaryTriangle {
        float normal[3];
        float v1[3];
        float v2[3];
        float v3[3];
        uint16_t attribute;
    };
    
    static_assert(sizeof(BinaryTriangle) == 50, "BinaryTriangle must be 50 bytes");
    
    BinaryTriangle tri;
    uint32_t vertexIndex = 0;
    
    for (uint32_t i = 0; i < numTriangles; ++i) {
        file.read(reinterpret_cast<char*>(&tri), sizeof(tri));
        
        if (file.gcount() != sizeof(tri)) {
            break;  // End of file or error
        }
        
        // Add 3 vertices
        for (int v = 0; v < 3; ++v) {
            const float* vertPos = (v == 0) ? tri.v1 : (v == 1) ? tri.v2 : tri.v3;
            
            vertexData.push_back(vertPos[0]);
            vertexData.push_back(vertPos[1]);
            vertexData.push_back(vertPos[2]);
            vertexData.push_back(tri.normal[0]);
            vertexData.push_back(tri.normal[1]);
            vertexData.push_back(tri.normal[2]);
            
            indices.push_back(vertexIndex++);
        }
        
        // Update bounds
        for (int v = 0; v < 3; ++v) {
            const float* vertPos = (v == 0) ? tri.v1 : (v == 1) ? tri.v2 : tri.v3;
            mesh->bounds.expand(math::Vector3(vertPos[0], vertPos[1], vertPos[2]));
        }
    }
    
    prim.vertexData.resize(vertexData.size() * sizeof(float));
    std::memcpy(prim.vertexData.data(), vertexData.data(), vertexData.size() * sizeof(float));
    prim.vertexCount = static_cast<uint32_t>(vertexData.size() / 6);
    prim.indices = std::move(indices);
    prim.indexCount = static_cast<uint32_t>(prim.indices.size());
    
    mesh->primitives.push_back(std::move(prim));
    mesh->loadState = LoadState::Loaded;
    mesh->name = path;
    
    return mesh;
}

std::unique_ptr<Mesh> STLLoader::loadAscii(const std::string& path) {
    auto mesh = std::make_unique<Mesh>();
    mesh->sourcePath = path;
    mesh->assetType = AssetType::Model_STL;
    
    std::ifstream file(path);
    if (!file.is_open()) {
        mesh->loadState = LoadState::Failed;
        mesh->loadError = "Failed to open file";
        return mesh;
    }
    
    MeshPrimitive prim;
    prim.vertexFormat.stride = 6 * sizeof(float);
    prim.vertexFormat.addAttribute(VertexAttribute::Position, 3, sizeof(float));
    prim.vertexFormat.addAttribute(VertexAttribute::Normal, 3, sizeof(float));
    
    std::vector<float> vertexData;
    std::vector<uint32_t> indices;
    uint32_t vertexIndex = 0;
    
    std::string line;
    math::Vector3 currentNormal(0.0f, 0.0f, 1.0f);
    
    while (std::getline(file, line)) {
        // Trim
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        line = line.substr(start);
        
        if (line.find("facet normal") == 0) {
            std::istringstream iss(line);
            std::string keyword;
            iss >> keyword >> keyword;  // "facet" "normal"
            iss >> currentNormal.x >> currentNormal.y >> currentNormal.z;
        }
        else if (line.find("vertex") == 0) {
            std::istringstream iss(line);
            std::string keyword;
            iss >> keyword;
            
            math::Vector3 pos;
            iss >> pos.x >> pos.y >> pos.z;
            
            vertexData.push_back(pos.x);
            vertexData.push_back(pos.y);
            vertexData.push_back(pos.z);
            vertexData.push_back(currentNormal.x);
            vertexData.push_back(currentNormal.y);
            vertexData.push_back(currentNormal.z);
            
            indices.push_back(vertexIndex++);
            
            mesh->bounds.expand(pos);
        }
    }
    
    if (vertexData.empty()) {
        mesh->loadState = LoadState::Failed;
        mesh->loadError = "No vertices found in STL file";
        return mesh;
    }
    
    prim.vertexData.resize(vertexData.size() * sizeof(float));
    std::memcpy(prim.vertexData.data(), vertexData.data(), vertexData.size() * sizeof(float));
    prim.vertexCount = static_cast<uint32_t>(vertexData.size() / 6);
    prim.indices = std::move(indices);
    prim.indexCount = static_cast<uint32_t>(prim.indices.size());
    
    mesh->primitives.push_back(std::move(prim));
    mesh->loadState = LoadState::Loaded;
    mesh->name = path;
    
    return mesh;
}

} // namespace phoenix::resource
