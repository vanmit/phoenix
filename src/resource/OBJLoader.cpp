#include "../../include/phoenix/resource/asset_loader.hpp"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <algorithm>
#include <cmath>

namespace phoenix::resource {

// ============ OBJLoader ============

OBJLoader::OBJLoader(const Config& config) : m_config(config) {}

std::set<std::string> OBJLoader::getSupportedExtensions() const {
    return {".obj"};
}

AssetType OBJLoader::getAssetType() const {
    return AssetType::Model_OBJ;
}

ValidationResult OBJLoader::validate(const std::string& path) const {
    if (path.find("..") != std::string::npos) {
        return ValidationResult::failure("Invalid path: contains '..'");
    }
    
    std::ifstream file(path);
    if (!file.is_open()) {
        return ValidationResult::failure("Cannot open file: " + path);
    }
    
    // Check for OBJ header (should start with # or v/f/g/o)
    std::string line;
    while (std::getline(file, line)) {
        // Skip empty lines
        if (line.empty()) continue;
        
        // Skip comments
        if (line[0] == '#') continue;
        
        // Check for valid OBJ keywords
        char keyword = line[0];
        if (keyword == 'v' || keyword == 'f' || keyword == 'g' || 
            keyword == 'o' || keyword == 'u' || keyword == 'm' ||
            keyword == 's' || keyword == 'l' || keyword == 'p') {
            return ValidationResult::success();
        }
        
        // First non-comment line should be a valid keyword
        break;
    }
    
    return ValidationResult::failure("Invalid OBJ file format");
}

size_t OBJLoader::estimateMemoryUsage(const std::string& path) const {
    std::ifstream file(path);
    if (!file.is_open()) return 0;
    
    // Count lines for rough estimate
    size_t lineCount = 0;
    std::string line;
    while (std::getline(file, line)) {
        ++lineCount;
    }
    
    // Rough estimate: ~100 bytes per vertex/face line
    return lineCount * 100;
}

std::future<std::unique_ptr<Mesh>> OBJLoader::loadAsync(const std::string& path) {
    return std::async(std::launch::async, [this, path]() {
        return load(path);
    });
}

std::unique_ptr<Mesh> OBJLoader::load(const std::string& path) {
    auto mesh = std::make_unique<Mesh>();
    mesh->sourcePath = path;
    mesh->loadState = LoadState::Loading;
    mesh->assetType = AssetType::Model_OBJ;
    
    // Validate first
    auto validation = validate(path);
    if (!validation.isValid) {
        mesh->loadState = LoadState::Failed;
        mesh->loadError = validation.error;
        return mesh;
    }
    
    std::ifstream file(path);
    if (!file.is_open()) {
        mesh->loadState = LoadState::Failed;
        mesh->loadError = "Failed to open file";
        return mesh;
    }
    
    // Temporary storage
    std::vector<math::Vector3> positions;
    std::vector<math::Vector3> normals;
    std::vector<math::Vector2> texcoords;
    
    struct FaceVertex {
        int32_t posIndex = -1;
        int32_t texIndex = -1;
        int32_t normIndex = -1;
    };
    
    std::vector<FaceVertex> faceVertices;
    std::string currentMaterial;
    uint32_t currentMaterialIndex = 0;
    
    std::unordered_map<VertexKey, uint32_t, VertexKeyHash> vertexMap;
    std::vector<float> vertexData;
    std::vector<uint32_t> indices;
    
    std::string line;
    while (std::getline(file, line)) {
        // Trim whitespace
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        line = line.substr(start);
        
        // Skip comments
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        std::string keyword;
        iss >> keyword;
        
        if (keyword == "v") {
            // Vertex position
            math::Vector3 pos;
            iss >> pos.x >> pos.y >> pos.z;
            positions.push_back(pos);
        }
        else if (keyword == "vn") {
            // Vertex normal
            math::Vector3 norm;
            iss >> norm.x >> norm.y >> norm.z;
            normals.push_back(norm);
        }
        else if (keyword == "vt") {
            // Texture coordinate
            math::Vector2 tex;
            iss >> tex.x >> tex.y;
            texcoords.push_back(tex);
        }
        else if (keyword == "f") {
            // Face
            std::string vertexStr;
            std::vector<FaceVertex> faceVerts;
            
            while (iss >> vertexStr) {
                FaceVertex fv;
                
                // Parse v/vt/vn format
                size_t slash1 = vertexStr.find('/');
                if (slash1 == std::string::npos) {
                    fv.posIndex = std::stoi(vertexStr) - 1;
                } else {
                    fv.posIndex = std::stoi(vertexStr.substr(0, slash1)) - 1;
                    
                    size_t slash2 = vertexStr.find('/', slash1 + 1);
                    if (slash2 == std::string::npos) {
                        // v/vn format (no texcoord)
                        if (slash1 + 1 < vertexStr.size()) {
                            fv.normIndex = std::stoi(vertexStr.substr(slash1 + 1)) - 1;
                        }
                    } else {
                        // v/vt/vn format
                        if (slash1 + 1 < slash2) {
                            fv.texIndex = std::stoi(vertexStr.substr(slash1 + 1, slash2 - slash1 - 1)) - 1;
                        }
                        if (slash2 + 1 < vertexStr.size()) {
                            fv.normIndex = std::stoi(vertexStr.substr(slash2 + 1)) - 1;
                        }
                    }
                }
                
                faceVerts.push_back(fv);
            }
            
            // Triangulate (fan triangulation for polygons)
            if (faceVerts.size() >= 3) {
                for (size_t i = 1; i < faceVerts.size() - 1; ++i) {
                    FaceVertex verts[3] = {faceVerts[0], faceVerts[i], faceVerts[i + 1]};
                    
                    for (const auto& fv : verts) {
                        VertexKey key{fv.posIndex, fv.normIndex, fv.texIndex};
                        
                        auto it = vertexMap.find(key);
                        if (it == vertexMap.end()) {
                            // New vertex
                            uint32_t newIndex = vertexData.size() / 8;  // 8 floats per vertex (pos+norm+uv)
                            
                            // Add position
                            if (fv.posIndex >= 0 && fv.posIndex < static_cast<int32_t>(positions.size())) {
                                const auto& pos = positions[fv.posIndex];
                                vertexData.push_back(pos.x);
                                vertexData.push_back(pos.y);
                                vertexData.push_back(pos.z);
                            } else {
                                vertexData.push_back(0.0f);
                                vertexData.push_back(0.0f);
                                vertexData.push_back(0.0f);
                            }
                            
                            // Add normal
                            if (fv.normIndex >= 0 && fv.normIndex < static_cast<int32_t>(normals.size())) {
                                const auto& norm = normals[fv.normIndex];
                                vertexData.push_back(norm.x);
                                vertexData.push_back(norm.y);
                                vertexData.push_back(norm.z);
                            } else {
                                vertexData.push_back(0.0f);
                                vertexData.push_back(1.0f);
                                vertexData.push_back(0.0f);
                            }
                            
                            // Add texcoord
                            if (fv.texIndex >= 0 && fv.texIndex < static_cast<int32_t>(texcoords.size())) {
                                const auto& tex = texcoords[fv.texIndex];
                                vertexData.push_back(tex.x);
                                vertexData.push_back(tex.y);
                            } else {
                                vertexData.push_back(0.0f);
                                vertexData.push_back(0.0f);
                            }
                            
                            vertexMap[key] = newIndex;
                            indices.push_back(newIndex);
                        } else {
                            indices.push_back(it->second);
                        }
                    }
                }
            }
        }
        else if (keyword == "usemtl") {
            // Material reference
            iss >> currentMaterial;
            // Would need to look up material index from MTL file
        }
        else if (keyword == "mtllib") {
            // MTL file reference
            std::string mtlFile;
            iss >> mtlFile;
            
            if (m_config.loadMaterials) {
                // Load MTL file
                // Implementation would parse MTL and create materials
            }
        }
    }
    
    // Create primitive
    if (!vertexData.empty()) {
        MeshPrimitive prim;
        prim.vertexData.resize(vertexData.size() * sizeof(float));
        std::memcpy(prim.vertexData.data(), vertexData.data(), vertexData.size() * sizeof(float));
        
        prim.vertexFormat.stride = 8 * sizeof(float);  // pos(3) + norm(3) + uv(2)
        prim.vertexFormat.addAttribute(VertexAttribute::Position, 3, sizeof(float));
        prim.vertexFormat.addAttribute(VertexAttribute::Normal, 3, sizeof(float));
        prim.vertexFormat.addAttribute(VertexAttribute::UV0, 2, sizeof(float));
        
        prim.vertexCount = static_cast<uint32_t>(vertexData.size() / 8);
        prim.indices = std::move(indices);
        prim.indexCount = static_cast<uint32_t>(prim.indices.size());
        prim.materialIndex = currentMaterialIndex;
        
        // Calculate bounds
        for (uint32_t i = 0; i < prim.vertexCount; ++i) {
            math::Vector3 pos(
                vertexData[i * 8 + 0],
                vertexData[i * 8 + 1],
                vertexData[i * 8 + 2]
            );
            prim.bounds.expand(pos);
        }
        
        mesh->primitives.push_back(std::move(prim));
        mesh->bounds = mesh->primitives[0].bounds;
    }
    
    // Generate normals if needed
    if (m_config.generateNormals && normals.empty()) {
        // Would generate normals from triangles
    }
    
    // Generate tangents if needed
    if (m_config.generateTangents) {
        // Would calculate tangents
    }
    
    mesh->loadState = LoadState::Loaded;
    mesh->name = path;
    
    return mesh;
}

} // namespace phoenix::resource
