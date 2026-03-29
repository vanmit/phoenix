#include "../../include/phoenix/resource/asset_loader.hpp"
#include <tiny_gltf.h>
#include <fstream>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <queue>
#include <cmath>

namespace phoenix::resource {

// ============ Helper Functions ============

namespace {

/**
 * @brief Convert glTF component type to byte size
 */
size_t getComponentSize(int componentType) {
    switch (componentType) {
        case TINYGLTF_COMPONENT_TYPE_BYTE:
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            return 1;
        case TINYGLTF_COMPONENT_TYPE_SHORT:
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            return 2;
        case TINYGLTF_COMPONENT_TYPE_INT:
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
        case TINYGLTF_COMPONENT_TYPE_FLOAT:
            return 4;
        case TINYGLTF_COMPONENT_TYPE_DOUBLE:
            return 8;
        default:
            return 0;
    }
}

/**
 * @brief Get number of components for glTF type
 */
int getNumComponents(int type) {
    switch (type) {
        case TINYGLTF_TYPE_SCALAR: return 1;
        case TINYGLTF_TYPE_VEC2: return 2;
        case TINYGLTF_TYPE_VEC3: return 3;
        case TINYGLTF_TYPE_VEC4: return 4;
        case TINYGLTF_TYPE_MAT2: return 4;
        case TINYGLTF_TYPE_MAT3: return 9;
        case TINYGLTF_TYPE_MAT4: return 16;
        default: return 0;
    }
}

/**
 * @brief Read value from accessor based on component type
 */
template<typename T>
T readValue(const uint8_t* data, int componentType) {
    switch (componentType) {
        case TINYGLTF_COMPONENT_TYPE_BYTE:
            return static_cast<T>(*reinterpret_cast<const int8_t*>(data));
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            return static_cast<T>(*reinterpret_cast<const uint8_t*>(data));
        case TINYGLTF_COMPONENT_TYPE_SHORT:
            return static_cast<T>(*reinterpret_cast<const int16_t*>(data));
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            return static_cast<T>(*reinterpret_cast<const uint16_t*>(data));
        case TINYGLTF_COMPONENT_TYPE_INT:
            return static_cast<T>(*reinterpret_cast<const int32_t*>(data));
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            return static_cast<T>(*reinterpret_cast<const uint32_t*>(data));
        case TINYGLTF_COMPONENT_TYPE_FLOAT:
            return static_cast<T>(*reinterpret_cast<const float*>(data));
        case TINYGLTF_COMPONENT_TYPE_DOUBLE:
            return static_cast<T>(*reinterpret_cast<const double*>(data));
        default:
            return T{};
    }
}

/**
 * @brief Convert glTF interpolation mode to string
 */
std::string getInterpolationMode(int mode) {
    switch (mode) {
        case TINYGLTF_INTERPOLATION_LINEAR: return "LINEAR";
        case TINYGLTF_INTERPOLATION_STEP: return "STEP";
        case TINYGLTF_INTERPOLATION_CUBICSPLINE: return "CUBICSPLINE";
        default: return "LINEAR";
    }
}

/**
 * @brief Calculate bounding volume from vertices
 */
BoundingVolume calculateBounds(const std::vector<math::Vector3>& positions) {
    BoundingVolume bounds;
    for (const auto& pos : positions) {
        bounds.expand(pos);
    }
    return bounds;
}

/**
 * @brief Linear interpolation
 */
template<typename T>
T lerp(const T& a, const T& b, float t) {
    return a + (b - a) * t;
}

/**
 * @brief Spherical linear interpolation for quaternions
 */
math::Quaternion slerp(const math::Quaternion& a, const math::Quaternion& b, float t) {
    float dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    
    // If dot product is negative, negate one quaternion
    math::Quaternion q2 = b;
    if (dot < 0.0f) {
        q2 = math::Quaternion(-b.x, -b.y, -b.z, -b.w);
        dot = -dot;
    }
    
    // Clamp dot to [-1, 1] to avoid numerical issues
    dot = std::max(-1.0f, std::min(1.0f, dot));
    
    // If quaternions are close, use linear interpolation
    if (dot > 0.9995f) {
        math::Quaternion result = math::Quaternion(
            lerp(a.x, q2.x, t),
            lerp(a.y, q2.y, t),
            lerp(a.z, q2.z, t),
            lerp(a.w, q2.w, t)
        );
        return math::normalize(result);
    }
    
    float theta = std::acos(dot);
    float sinTheta = std::sin(theta);
    
    float wa = std::sin((1.0f - t) * theta) / sinTheta;
    float wb = std::sin(t * theta) / sinTheta;
    
    return math::Quaternion(
        wa * a.x + wb * q2.x,
        wa * a.y + wb * q2.y,
        wa * a.z + wb * q2.z,
        wa * a.w + wb * q2.w
    );
}

/**
 * @brief Catmull-Rom spline interpolation for cubic spline
 */
math::Vector3 cubicSplineInterp(
    const math::Vector3& prevVal, const math::Vector3& prevTan,
    const math::Vector3& currVal, const math::Vector3& currTan,
    const math::Vector3& nextVal, const math::Vector3& nextTan,
    float t) {
    
    float t2 = t * t;
    float t3 = t2 * t;
    
    float h00 = 2.0f * t3 - 3.0f * t2 + 1.0f;
    float h10 = t3 - 2.0f * t2 + t;
    float h01 = -2.0f * t3 + 3.0f * t2;
    float h11 = t3 - t2;
    
    return math::Vector3(
        h00 * currVal.x + h10 * currTan.x + h01 * nextVal.x + h11 * nextTan.x,
        h00 * currVal.y + h10 * currTan.y + h01 * nextVal.y + h11 * nextTan.y,
        h00 * currVal.z + h10 * currTan.z + h01 * nextVal.z + h11 * nextTan.z
    );
}

} // anonymous namespace

// ============ GLTFLoader Implementation ============

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
    // Security: Validate path for traversal attacks
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
        return ValidationResult::failure("File exceeds maximum buffer size (" + 
                                       std::to_string(m_config.maxBufferSize) + " bytes)");
    }
    
    // Read and validate format
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
                                               std::to_string(version) + 
                                               " (only 2.0 is supported)");
            }
        } else if (ext == ".gltf") {
            // JSON-based glTF: check for valid JSON structure
            file.seekg(0, std::ios::beg);
            std::string content(size, '\0');
            file.read(&content[0], size);
            
            // Basic validation: must start with { and contain "asset"
            if (content.empty() || content[0] != '{') {
                return ValidationResult::failure("Invalid glTF JSON format");
            }
            
            if (content.find("\"asset\"") == std::string::npos) {
                return ValidationResult::failure("Missing required 'asset' property");
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
    // Includes vertex data, indices, textures references, animation data
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
    
    // Load using tinygltf
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err, warn;
    
    bool isBinary = path.size() >= 4 && 
                    path.substr(path.size() - 4) == ".glb";
    
    bool ret = isBinary ?
        loader.LoadBinaryFromFile(&model, &err, &warn, path) :
        loader.LoadASCIIFromFile(&model, &err, &warn, path);
    
    if (!ret) {
        mesh->loadState = LoadState::Failed;
        mesh->loadError = "Failed to load glTF: " + err;
        return mesh;
    }
    
    // Validate glTF version
    if (model.asset.version != "2.0") {
        mesh->loadState = LoadState::Failed;
        mesh->loadError = "Unsupported glTF version: " + model.asset.version + 
                         " (only 2.0 is supported)";
        return mesh;
    }
    
    // Store warnings
    if (!warn.empty()) {
        // Could log warnings here
    }
    
    mesh->name = model.asset.name.empty() ? 
                 std::filesystem::path(path).stem().string() : 
                 model.asset.name;
    mesh->assetType = isBinary ? AssetType::Model_GLTF_BINARY : AssetType::Model_GLTF;
    
    try {
        // Process materials first (needed by meshes)
        for (const auto& gltfMaterial : model.materials) {
            Material mat;
            processMaterial(&gltfMaterial, mat);
            mesh->materials.push_back(std::move(mat));
        }
        
        // Process meshes
        for (const auto& gltfMesh : model.meshes) {
            processMesh(&gltfMesh, *mesh, path);
        }
        
        // Process skins (if enabled)
        if (m_config.loadSkins && !model.skins.empty()) {
            for (const auto& gltfSkin : model.skins) {
                processSkin(&gltfSkin, *mesh, model);
            }
        }
        
        // Process animations (if enabled)
        if (m_config.loadAnimations && !model.animations.empty()) {
            for (const auto& gltfAnimation : model.animations) {
                processAnimation(&gltfAnimation, *mesh, model);
            }
        }
        
        // Calculate overall bounds
        for (const auto& prim : mesh->primitives) {
            mesh->bounds.expand(prim.bounds);
        }
        
        // Calculate memory usage
        mesh->memoryUsage.currentUsage = mesh->calculateMemoryUsage();
        mesh->memoryUsage.peakUsage = mesh->memoryUsage.currentUsage;
        
        mesh->loadState = LoadState::Loaded;
        
    } catch (const std::exception& e) {
        mesh->loadState = LoadState::Failed;
        mesh->loadError = std::string("Exception during loading: ") + e.what();
    }
    
    return mesh;
}

std::unique_ptr<Mesh> GLTFLoader::loadFromMemory(const uint8_t* data, size_t size,
                                                  const std::string& basePath) {
    auto mesh = std::make_unique<Mesh>();
    mesh->sourcePath = basePath;
    mesh->loadState = LoadState::Loading;
    
    if (size < 12) {  // Minimum GLB header
        mesh->loadState = LoadState::Failed;
        mesh->loadError = "Data too small for GLB (minimum 12 bytes)";
        return mesh;
    }
    
    // Check GLB magic
    uint32_t magic = *reinterpret_cast<const uint32_t*>(data);
    if (magic != 0x46546C67) {
        mesh->loadState = LoadState::Failed;
        mesh->loadError = "Invalid GLB magic number";
        return mesh;
    }
    
    // Check version
    uint32_t version = *reinterpret_cast<const uint32_t*>(data + 4);
    if (version != 2) {
        mesh->loadState = LoadState::Failed;
        mesh->loadError = "Unsupported glTF version: " + std::to_string(version);
        return mesh;
    }
    
    // Load using tinygltf
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err, warn;
    
    bool ret = loader.LoadBinaryFromMemory(&model, &err, &warn, data, size);
    
    if (!ret) {
        mesh->loadState = LoadState::Failed;
        mesh->loadError = "Failed to load glTF from memory: " + err;
        return mesh;
    }
    
    mesh->name = basePath.empty() ? "memory_model" : 
                 std::filesystem::path(basePath).stem().string();
    mesh->assetType = AssetType::Model_GLTF_BINARY;
    
    try {
        // Process materials
        for (const auto& gltfMaterial : model.materials) {
            Material mat;
            processMaterial(&gltfMaterial, mat);
            mesh->materials.push_back(std::move(mat));
        }
        
        // Process meshes
        for (const auto& gltfMesh : model.meshes) {
            processMesh(&gltfMesh, *mesh, basePath);
        }
        
        // Process skins
        if (m_config.loadSkins && !model.skins.empty()) {
            for (const auto& gltfSkin : model.skins) {
                processSkin(&gltfSkin, *mesh, model);
            }
        }
        
        // Process animations
        if (m_config.loadAnimations && !model.animations.empty()) {
            for (const auto& gltfAnimation : model.animations) {
                processAnimation(&gltfAnimation, *mesh, model);
            }
        }
        
        // Calculate bounds
        for (const auto& prim : mesh->primitives) {
            mesh->bounds.expand(prim.bounds);
        }
        
        mesh->memoryUsage.currentUsage = mesh->calculateMemoryUsage();
        mesh->memoryUsage.peakUsage = mesh->memoryUsage.currentUsage;
        
        mesh->loadState = LoadState::Loaded;
        
    } catch (const std::exception& e) {
        mesh->loadState = LoadState::Failed;
        mesh->loadError = std::string("Exception during loading: ") + e.what();
    }
    
    return mesh;
}

void GLTFLoader::processMesh(const void* gltfMeshPtr, Mesh& outMesh, 
                             const std::string& basePath) {
    const tinygltf::Mesh* gltfMesh = 
        static_cast<const tinygltf::Mesh*>(gltfMeshPtr);
    
    for (const auto& gltfPrimitive : gltfMesh->primitives) {
        MeshPrimitive prim;
        
        // Get accessor indices
        int positionsAccessor = -1, normalsAccessor = -1;
        int tangentsAccessor = -1, colorsAccessor = -1;
        int uv0Accessor = -1, uv1Accessor = -1;
        int jointsAccessor = -1, weightsAccessor = -1;
        
        // Find required and optional attributes
        for (const auto& [semantic, index] : gltfPrimitive.attributes) {
            if (semantic == "POSITION") positionsAccessor = index;
            else if (semantic == "NORMAL") normalsAccessor = index;
            else if (semantic == "TANGENT") tangentsAccessor = index;
            else if (semantic == "COLOR_0") colorsAccessor = index;
            else if (semantic == "TEXCOORD_0") uv0Accessor = index;
            else if (semantic == "TEXCOORD_1") uv1Accessor = index;
            else if (semantic == "JOINTS_0") jointsAccessor = index;
            else if (semantic == "WEIGHTS_0") weightsAccessor = index;
        }
        
        // POSITION is required
        if (positionsAccessor < 0) {
            // Skip primitive without positions
            continue;
        }
        
        const auto& model = *static_cast<const tinygltf::Model*>(
            reinterpret_cast<const uint8_t*>(&gltfMesh->extras) - 
            offsetof(tinygltf::Model, meshes));
        
        const auto& posAccessor = model.accessors[positionsAccessor];
        const auto& posBufferView = model.bufferViews[posAccessor.bufferView];
        const auto& posBuffer = model.buffers[posBufferView.buffer];
        
        // Calculate vertex stride based on attributes
        uint32_t stride = 0;
        uint32_t posOffset = 0, normOffset = 0, tanOffset = 0, colorOffset = 0;
        uint32_t uv0Offset = 0, uv1Offset = 0, jointOffset = 0, weightOffset = 0;
        
        bool hasNormals = normalsAccessor >= 0;
        bool hasTangents = tangentsAccessor >= 0;
        bool hasColors = colorsAccessor >= 0;
        bool hasUV0 = uv0Accessor >= 0;
        bool hasUV1 = uv1Accessor >= 0;
        bool hasSkinning = jointsAccessor >= 0 && weightsAccessor >= 0;
        
        // Position (vec3, 4 bytes each = 12 bytes)
        posOffset = stride;
        stride += 12;
        
        // Normal (vec3, 12 bytes)
        if (hasNormals) {
            normOffset = stride;
            stride += 12;
        }
        
        // Tangent (vec4, 16 bytes)
        if (hasTangents) {
            tanOffset = stride;
            stride += 16;
        }
        
        // Color (vec4, 16 bytes - normalized)
        if (hasColors) {
            colorOffset = stride;
            stride += 16;
        }
        
        // UV0 (vec2, 8 bytes)
        if (hasUV0) {
            uv0Offset = stride;
            stride += 8;
        }
        
        // UV1 (vec2, 8 bytes)
        if (hasUV1) {
            uv1Offset = stride;
            stride += 8;
        }
        
        // Joint indices (uvec4, 16 bytes)
        if (hasSkinning) {
            jointOffset = stride;
            stride += 16;
            
            weightOffset = stride;
            stride += 16;  // vec4 weights
        }
        
        prim.vertexFormat.stride = stride;
        prim.hasSkinning = hasSkinning;
        
        // Build vertex format descriptor
        prim.vertexFormat.addAttribute(VertexAttribute::Position, 3, 4);
        if (hasNormals) {
            prim.vertexFormat.addAttribute(VertexAttribute::Normal, 3, 4);
        }
        if (hasTangents) {
            prim.vertexFormat.addAttribute(VertexAttribute::Tangent, 4, 4);
        }
        if (hasColors) {
            prim.vertexFormat.addAttribute(VertexAttribute::Color, 4, 4, true);
        }
        if (hasUV0) {
            prim.vertexFormat.addAttribute(VertexAttribute::UV0, 2, 4);
        }
        if (hasUV1) {
            prim.vertexFormat.addAttribute(VertexAttribute::UV1, 2, 4);
        }
        if (hasSkinning) {
            prim.vertexFormat.addAttribute(VertexAttribute::JointIndices, 4, 4);
            prim.vertexFormat.addAttribute(VertexAttribute::JointWeights, 4, 4);
        }
        
        // Load vertex data
        uint32_t vertexCount = posAccessor.count;
        prim.vertexCount = vertexCount;
        prim.vertexData.resize(vertexCount * stride);
        
        std::vector<math::Vector3> positions(vertexCount);
        
        // Read positions
        {
            const auto& bufferData = posBuffer.data;
            size_t componentSize = getComponentSize(posAccessor.componentType);
            int numComponents = getNumComponents(posAccessor.type);
            size_t byteOffset = posBufferView.byteOffset + posAccessor.byteOffset;
            size_t byteStride = posBufferView.byteStride > 0 ? 
                               posBufferView.byteStride : 
                               (componentSize * numComponents);
            
            for (uint32_t i = 0; i < vertexCount; ++i) {
                const uint8_t* src = bufferData.data() + byteOffset + i * byteStride;
                math::Vector3 pos(
                    readValue<float>(src, posAccessor.componentType),
                    readValue<float>(src + componentSize, posAccessor.componentType),
                    readValue<float>(src + componentSize * 2, posAccessor.componentType)
                );
                positions[i] = pos;
                
                // Write to interleaved vertex data
                uint8_t* dst = prim.vertexData.data() + i * stride + posOffset;
                *reinterpret_cast<math::Vector3*>(dst) = pos;
            }
        }
        
        // Read normals
        if (hasNormals) {
            const auto& normAccessor = model.accessors[normalsAccessor];
            const auto& normBufferView = model.bufferViews[normAccessor.bufferView];
            const auto& bufferData = model.buffers[normBufferView.buffer].data;
            
            size_t componentSize = getComponentSize(normAccessor.componentType);
            size_t byteOffset = normBufferView.byteOffset + normAccessor.byteOffset;
            size_t byteStride = normBufferView.byteStride > 0 ? 
                               normBufferView.byteStride : 
                               (componentSize * 3);
            
            for (uint32_t i = 0; i < vertexCount; ++i) {
                const uint8_t* src = bufferData.data() + byteOffset + i * byteStride;
                math::Vector3 norm(
                    readValue<float>(src, normAccessor.componentType),
                    readValue<float>(src + componentSize, normAccessor.componentType),
                    readValue<float>(src + componentSize * 2, normAccessor.componentType)
                );
                
                uint8_t* dst = prim.vertexData.data() + i * stride + normOffset;
                *reinterpret_cast<math::Vector3*>(dst) = norm;
            }
        }
        
        // Read tangents
        if (hasTangents) {
            const auto& tanAccessor = model.accessors[tangentsAccessor];
            const auto& tanBufferView = model.bufferViews[tanAccessor.bufferView];
            const auto& bufferData = model.buffers[tanBufferView.buffer].data;
            
            size_t componentSize = getComponentSize(tanAccessor.componentType);
            size_t byteOffset = tanBufferView.byteOffset + tanAccessor.byteOffset;
            size_t byteStride = tanBufferView.byteStride > 0 ? 
                               tanBufferView.byteStride : 
                               (componentSize * 4);
            
            for (uint32_t i = 0; i < vertexCount; ++i) {
                const uint8_t* src = bufferData.data() + byteOffset + i * byteStride;
                math::Vector4 tan(
                    readValue<float>(src, tanAccessor.componentType),
                    readValue<float>(src + componentSize, tanAccessor.componentType),
                    readValue<float>(src + componentSize * 2, tanAccessor.componentType),
                    readValue<float>(src + componentSize * 3, tanAccessor.componentType)
                );
                
                uint8_t* dst = prim.vertexData.data() + i * stride + tanOffset;
                *reinterpret_cast<math::Vector4*>(dst) = tan;
            }
        }
        
        // Read colors
        if (hasColors) {
            const auto& colAccessor = model.accessors[colorsAccessor];
            const auto& colBufferView = model.bufferViews[colAccessor.bufferView];
            const auto& bufferData = model.buffers[colBufferView.buffer].data;
            
            size_t componentSize = getComponentSize(colAccessor.componentType);
            size_t byteOffset = colBufferView.byteOffset + colAccessor.byteOffset;
            size_t byteStride = colBufferView.byteStride > 0 ? 
                               colBufferView.byteStride : 
                               (componentSize * 4);
            
            for (uint32_t i = 0; i < vertexCount; ++i) {
                const uint8_t* src = bufferData.data() + byteOffset + i * byteStride;
                math::Color4 col(
                    readValue<float>(src, colAccessor.componentType),
                    readValue<float>(src + componentSize, colAccessor.componentType),
                    readValue<float>(src + componentSize * 2, colAccessor.componentType),
                    readValue<float>(src + componentSize * 3, colAccessor.componentType)
                );
                
                uint8_t* dst = prim.vertexData.data() + i * stride + colorOffset;
                *reinterpret_cast<math::Color4*>(dst) = col;
            }
        }
        
        // Read UVs
        if (hasUV0) {
            const auto& uvAccessor = model.accessors[uv0Accessor];
            const auto& uvBufferView = model.bufferViews[uvAccessor.bufferView];
            const auto& bufferData = model.buffers[uvBufferView.buffer].data;
            
            size_t componentSize = getComponentSize(uvAccessor.componentType);
            size_t byteOffset = uvBufferView.byteOffset + uvAccessor.byteOffset;
            size_t byteStride = uvBufferView.byteStride > 0 ? 
                               uvBufferView.byteStride : 
                               (componentSize * 2);
            
            for (uint32_t i = 0; i < vertexCount; ++i) {
                const uint8_t* src = bufferData.data() + byteOffset + i * byteStride;
                math::Vector2 uv(
                    readValue<float>(src, uvAccessor.componentType),
                    readValue<float>(src + componentSize, uvAccessor.componentType)
                );
                
                uint8_t* dst = prim.vertexData.data() + i * stride + uv0Offset;
                *reinterpret_cast<math::Vector2*>(dst) = uv;
            }
        }
        
        // Read skinning data
        if (hasSkinning) {
            // Joints
            const auto& jointAccessor = model.accessors[jointsAccessor];
            const auto& jointBufferView = model.bufferViews[jointAccessor.bufferView];
            const auto& bufferData = model.buffers[jointBufferView.buffer].data;
            
            size_t componentSize = getComponentSize(jointAccessor.componentType);
            size_t byteOffset = jointBufferView.byteOffset + jointAccessor.byteOffset;
            size_t byteStride = jointBufferView.byteStride > 0 ? 
                               jointBufferView.byteStride : 
                               (componentSize * 4);
            
            for (uint32_t i = 0; i < vertexCount; ++i) {
                const uint8_t* src = bufferData.data() + byteOffset + i * byteStride;
                math::Vector4 joints(
                    readValue<float>(src, jointAccessor.componentType),
                    readValue<float>(src + componentSize, jointAccessor.componentType),
                    readValue<float>(src + componentSize * 2, jointAccessor.componentType),
                    readValue<float>(src + componentSize * 3, jointAccessor.componentType)
                );
                
                uint8_t* dst = prim.vertexData.data() + i * stride + jointOffset;
                *reinterpret_cast<math::Vector4*>(dst) = joints;
            }
            
            // Weights
            const auto& weightAccessor = model.accessors[weightsAccessor];
            const auto& weightBufferView = model.bufferViews[weightAccessor.bufferView];
            const auto& weightBufferData = model.buffers[weightBufferView.buffer].data;
            
            componentSize = getComponentSize(weightAccessor.componentType);
            byteOffset = weightBufferView.byteOffset + weightAccessor.byteOffset;
            byteStride = weightBufferView.byteStride > 0 ? 
                        weightBufferView.byteStride : 
                        (componentSize * 4);
            
            for (uint32_t i = 0; i < vertexCount; ++i) {
                const uint8_t* src = weightBufferData.data() + byteOffset + i * byteStride;
                math::Vector4 weights(
                    readValue<float>(src, weightAccessor.componentType),
                    readValue<float>(src + componentSize, weightAccessor.componentType),
                    readValue<float>(src + componentSize * 2, weightAccessor.componentType),
                    readValue<float>(src + componentSize * 3, weightAccessor.componentType)
                );
                
                uint8_t* dst = prim.vertexData.data() + i * stride + weightOffset;
                *reinterpret_cast<math::Vector4*>(dst) = weights;
            }
        }
        
        // Read indices
        if (gltfPrimitive.indices >= 0) {
            const auto& idxAccessor = model.accessors[gltfPrimitive.indices];
            const auto& idxBufferView = model.bufferViews[idxAccessor.bufferView];
            const auto& bufferData = model.buffers[idxBufferView.buffer].data;
            
            size_t componentSize = getComponentSize(idxAccessor.componentType);
            size_t byteOffset = idxBufferView.byteOffset + idxAccessor.byteOffset;
            size_t byteStride = componentSize;  // Indices are tightly packed
            
            prim.indexCount = idxAccessor.count;
            prim.indices.resize(prim.indexCount);
            
            for (uint32_t i = 0; i < prim.indexCount; ++i) {
                const uint8_t* src = bufferData.data() + byteOffset + i * byteStride;
                
                switch (idxAccessor.componentType) {
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                        prim.indices[i] = *reinterpret_cast<const uint8_t*>(src);
                        break;
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                        prim.indices[i] = *reinterpret_cast<const uint16_t*>(src);
                        break;
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                        prim.indices[i] = *reinterpret_cast<const uint32_t*>(src);
                        break;
                    default:
                        prim.indices[i] = 0;
                        break;
                }
            }
        } else {
            // No indices - generate them
            prim.indexCount = vertexCount;
            prim.indices.resize(vertexCount);
            for (uint32_t i = 0; i < vertexCount; ++i) {
                prim.indices[i] = i;
            }
        }
        
        // Set material
        prim.materialIndex = gltfPrimitive.material >= 0 ? 
                            static_cast<uint32_t>(gltfPrimitive.material) : 0;
        
        // Calculate bounds
        prim.bounds = calculateBounds(positions);
        
        outMesh.primitives.push_back(std::move(prim));
    }
}

void GLTFLoader::processMaterial(const void* gltfMaterialPtr, Material& outMaterial) {
    const tinygltf::Material* gltfMaterial = 
        static_cast<const tinygltf::Material*>(gltfMaterialPtr);
    
    outMaterial.name = gltfMaterial->name;
    
    // PBR Metallic-Roughness properties
    const auto& pbr = gltfMaterial->pbrMetallicRoughness;
    
    // Base color
    if (pbr.baseColorFactor.size() >= 4) {
        outMaterial.baseColorFactor = math::Color4(
            pbr.baseColorFactor[0],
            pbr.baseColorFactor[1],
            pbr.baseColorFactor[2],
            pbr.baseColorFactor[3]
        );
    }
    outMaterial.baseColorTexture = pbr.baseColorTexture.textureIndex;
    outMaterial.baseColorTexCoord = pbr.baseColorTexture.texCoord;
    
    // Metallic roughness
    outMaterial.metallicFactor = pbr.metallicFactor;
    outMaterial.roughnessFactor = pbr.roughnessFactor;
    outMaterial.metallicRoughnessTexture = pbr.metallicRoughnessTexture.textureIndex;
    outMaterial.metallicRoughnessTexCoord = pbr.metallicRoughnessTexture.texCoord;
    
    // Normal texture
    if (gltfMaterial->normalTexture.textureIndex >= 0) {
        outMaterial.normalTexture = gltfMaterial->normalTexture.textureIndex;
        outMaterial.normalScale = gltfMaterial->normalTexture.scale;
        outMaterial.normalTexCoord = gltfMaterial->normalTexture.texCoord;
    }
    
    // Occlusion texture
    if (gltfMaterial->occlusionTexture.textureIndex >= 0) {
        outMaterial.occlusionTexture = gltfMaterial->occlusionTexture.textureIndex;
        outMaterial.occlusionStrength = gltfMaterial->occlusionTexture.strength;
        outMaterial.occlusionTexCoord = gltfMaterial->occlusionTexture.texCoord;
    }
    
    // Emissive
    if (gltfMaterial->emissiveFactor.size() >= 3) {
        outMaterial.emissiveFactor = math::Color3(
            gltfMaterial->emissiveFactor[0],
            gltfMaterial->emissiveFactor[1],
            gltfMaterial->emissiveFactor[2]
        );
    }
    if (gltfMaterial->emissiveTexture.textureIndex >= 0) {
        outMaterial.emissiveTexture = gltfMaterial->emissiveTexture.textureIndex;
        outMaterial.emissiveTexCoord = gltfMaterial->emissiveTexture.texCoord;
    }
    
    // Alpha mode
    if (gltfMaterial->alphaMode == "OPAQUE") {
        outMaterial.alphaMode = Material::AlphaMode::Opaque;
    } else if (gltfMaterial->alphaMode == "MASK") {
        outMaterial.alphaMode = Material::AlphaMode::Mask;
        outMaterial.alphaCutoff = gltfMaterial->alphaCutoff;
    } else if (gltfMaterial->alphaMode == "BLEND") {
        outMaterial.alphaMode = Material::AlphaMode::Blend;
    }
    
    // Double sided
    outMaterial.doubleSided = gltfMaterial->doubleSided;
}

void GLTFLoader::processSkin(const void* gltfSkinPtr, Mesh& outMesh, 
                             const tinygltf::Model& model) {
    const tinygltf::Skin* gltfSkin = static_cast<const tinygltf::Skin*>(gltfSkinPtr);
    
    // Get inverse bind matrices
    std::vector<math::Matrix4> inverseBindMatrices;
    
    if (gltfSkin->inverseBindMatrices >= 0) {
        const auto& ibmAccessor = model.accessors[gltfSkin->inverseBindMatrices];
        const auto& ibmBufferView = model.bufferViews[ibmAccessor.bufferView];
        const auto& bufferData = model.buffers[ibmBufferView.buffer].data;
        
        size_t byteOffset = ibmBufferView.byteOffset + ibmAccessor.byteOffset;
        size_t byteStride = ibmBufferView.byteStride > 0 ? 
                           ibmBufferView.byteStride : 64;  // mat4 = 16 floats * 4 bytes
        
        for (uint32_t i = 0; i < ibmAccessor.count; ++i) {
            const uint8_t* src = bufferData.data() + byteOffset + i * byteStride;
            const float* data = reinterpret_cast<const float*>(src);
            
            // glTF uses column-major order
            math::Matrix4 ibm(
                data[0], data[1], data[2], data[3],
                data[4], data[5], data[6], data[7],
                data[8], data[9], data[10], data[11],
                data[12], data[13], data[14], data[15]
            );
            inverseBindMatrices.push_back(ibm);
        }
    }
    
    // Build joint hierarchy from skeleton
    std::vector<int32_t> jointToIndex(gltfSkin->joints.size(), -1);
    std::vector<int32_t> parentIndices(gltfSkin->joints.size(), -1);
    
    // Find skeleton root
    int32_t skeletonRoot = gltfSkin->skeleton >= 0 ? 
                          gltfSkin->skeleton : 
                          (gltfSkin->joints.empty() ? -1 : gltfSkin->joints[0]);
    
    // Build node parent map for the entire model
    std::unordered_map<int32_t, int32_t> nodeParentMap;
    for (int32_t nodeIdx = 0; nodeIdx < static_cast<int32_t>(model.nodes.size()); ++nodeIdx) {
        const auto& node = model.nodes[nodeIdx];
        for (int32_t childIdx : node.children) {
            nodeParentMap[childIdx] = nodeIdx;
        }
    }
    
    // Create joints
    outMesh.joints.clear();
    outMesh.joints.reserve(gltfSkin->joints.size());
    
    for (size_t i = 0; i < gltfSkin->joints.size(); ++i) {
        int32_t nodeIdx = gltfSkin->joints[i];
        
        Joint joint;
        joint.name = nodeIdx >= 0 && nodeIdx < static_cast<int32_t>(model.nodes.size()) ?
                    model.nodes[nodeIdx].name : 
                    ("joint_" + std::to_string(i));
        
        // Get inverse bind matrix
        if (i < inverseBindMatrices.size()) {
            joint.inverseBindMatrix = inverseBindMatrices[i];
        } else {
            joint.inverseBindMatrix = math::Matrix4::identity();
        }
        
        // Find parent joint
        int32_t parentIndex = -1;
        if (nodeIdx >= 0) {
            auto it = nodeParentMap.find(nodeIdx);
            if (it != nodeParentMap.end()) {
                int32_t parentNodeIdx = it->second;
                
                // Find if parent is also a joint
                for (size_t j = 0; j < gltfSkin->joints.size(); ++j) {
                    if (gltfSkin->joints[j] == parentNodeIdx) {
                        parentIndex = static_cast<int32_t>(j);
                        break;
                    }
                }
            }
        }
        joint.parentIndex = parentIndex;
        
        // Calculate initial transform from node
        if (nodeIdx >= 0 && nodeIdx < static_cast<int32_t>(model.nodes.size())) {
            const auto& node = model.nodes[nodeIdx];
            
            math::Matrix4 transform = math::Matrix4::identity();
            
            // Apply translation
            if (node.translation.size() >= 3) {
                math::Matrix4 translation = math::Matrix4::translation(
                    node.translation[0], node.translation[1], node.translation[2]
                );
                transform = transform * translation;
            }
            
            // Apply rotation
            if (node.rotation.size() >= 4) {
                math::Quaternion rot(
                    node.rotation[0], node.rotation[1], 
                    node.rotation[2], node.rotation[3]
                );
                math::Matrix4 rotation = math::Matrix4::rotation(rot);
                transform = transform * rotation;
            }
            
            // Apply scale
            if (node.scale.size() >= 3) {
                math::Matrix4 scale = math::Matrix4::scaling(
                    node.scale[0], node.scale[1], node.scale[2]
                );
                transform = transform * scale;
            }
            
            joint.transform = transform;
        }
        
        outMesh.joints.push_back(std::move(joint));
        jointToIndex[i] = static_cast<int32_t>(outMesh.joints.size() - 1);
    }
    
    // Set skeleton root
    for (size_t i = 0; i < gltfSkin->joints.size(); ++i) {
        if (gltfSkin->joints[i] == skeletonRoot) {
            outMesh.skeletonRoot = static_cast<int32_t>(i);
            break;
        }
    }
    
    if (outMesh.skeletonRoot < 0 && !outMesh.joints.empty()) {
        outMesh.skeletonRoot = 0;  // Default to first joint
    }
}

void GLTFLoader::processAnimation(const void* gltfAnimationPtr, Mesh& outMesh,
                                   const tinygltf::Model& model) {
    const tinygltf::Animation* gltfAnimation = 
        static_cast<const tinygltf::Animation*>(gltfAnimationPtr);
    
    AnimationClip clip;
    clip.name = gltfAnimation->name.empty() ? 
                ("animation_" + std::to_string(&gltfAnimation - model.animations.data())) :
                gltfAnimation->name;
    clip.isLooping = true;  // Default to looping
    
    // First pass: collect all samplers' data
    std::vector<std::vector<float>> samplerTimes(gltfAnimation->samplers.size());
    std::vector<std::vector<math::Vector3>> samplerValues(gltfAnimation->samplers.size());
    std::vector<std::vector<math::Vector3>> samplerInTangents(gltfAnimation->samplers.size());
    std::vector<std::vector<math::Vector3>> samplerOutTangents(gltfAnimation->samplers.size());
    std::vector<std::vector<math::Quaternion>> samplerRotations(gltfAnimation->samplers.size());
    std::vector<int> samplerInterpolations(gltfAnimation->samplers.size());
    
    for (size_t i = 0; i < gltfAnimation->samplers.size(); ++i) {
        const auto& sampler = gltfAnimation->samplers[i];
        
        // Read input (times)
        const auto& inputAccessor = model.accessors[sampler.input];
        const auto& inputBufferView = model.bufferViews[inputAccessor.bufferView];
        const auto& inputData = model.buffers[inputBufferView.buffer].data;
        
        size_t componentSize = getComponentSize(inputAccessor.componentType);
        size_t byteOffset = inputBufferView.byteOffset + inputAccessor.byteOffset;
        size_t byteStride = componentSize;
        
        samplerTimes[i].resize(inputAccessor.count);
        for (uint32_t j = 0; j < inputAccessor.count; ++j) {
            const uint8_t* src = inputData.data() + byteOffset + j * byteStride;
            samplerTimes[i][j] = readValue<float>(src, inputAccessor.componentType);
        }
        
        // Read output (values)
        const auto& outputAccessor = model.accessors[sampler.output];
        const auto& outputBufferView = model.bufferViews[outputAccessor.bufferView];
        const auto& outputData = model.buffers[outputBufferView.buffer].data;
        
        componentSize = getComponentSize(outputAccessor.componentType);
        int numComponents = getNumComponents(outputAccessor.type);
        byteOffset = outputBufferView.byteOffset + outputAccessor.byteOffset;
        byteStride = outputBufferView.byteStride > 0 ? 
                    outputBufferView.byteStride : 
                    (componentSize * numComponents);
        
        samplerInterpolations[i] = sampler.interpolation;
        
        uint32_t valueCount = outputAccessor.count / numComponents;
        
        // Check if this is a rotation channel (VEC4 = quaternion)
        bool isRotation = (numComponents == 4);
        
        if (isRotation) {
            samplerRotations[i].resize(valueCount);
        } else {
            samplerValues[i].resize(valueCount);
        }
        
        if (sampler.interpolation == TINYGLTF_INTERPOLATION_CUBICSPLINE) {
            // Cubic spline has in-tangent, value, out-tangent for each keyframe
            if (isRotation) {
                // For rotation, tangents are also quaternions (but we store as Vector3 for simplicity)
                samplerInTangents[i].resize(valueCount);
                samplerOutTangents[i].resize(valueCount);
            } else {
                samplerInTangents[i].resize(valueCount);
                samplerOutTangents[i].resize(valueCount);
            }
            
            for (uint32_t j = 0; j < valueCount; ++j) {
                const uint8_t* src = outputData.data() + byteOffset + j * byteStride * 3;
                
                if (isRotation) {
                    // In-tangent (store first 3 components)
                    math::Quaternion inTan(
                        readValue<float>(src, outputAccessor.componentType),
                        readValue<float>(src + componentSize, outputAccessor.componentType),
                        readValue<float>(src + componentSize * 2, outputAccessor.componentType),
                        readValue<float>(src + componentSize * 3, outputAccessor.componentType)
                    );
                    // Store as Vector3 for tangent data (simplification)
                    samplerInTangents[i][j] = math::Vector3(inTan.x, inTan.y, inTan.z);
                    
                    // Value (quaternion)
                    math::Quaternion quat(
                        readValue<float>(src + componentSize * 4, outputAccessor.componentType),
                        readValue<float>(src + componentSize * 5, outputAccessor.componentType),
                        readValue<float>(src + componentSize * 6, outputAccessor.componentType),
                        readValue<float>(src + componentSize * 7, outputAccessor.componentType)
                    );
                    samplerRotations[i][j] = quat;
                    
                    // Out-tangent (store first 3 components)
                    math::Quaternion outTan(
                        readValue<float>(src + componentSize * 8, outputAccessor.componentType),
                        readValue<float>(src + componentSize * 9, outputAccessor.componentType),
                        readValue<float>(src + componentSize * 10, outputAccessor.componentType),
                        readValue<float>(src + componentSize * 11, outputAccessor.componentType)
                    );
                    samplerOutTangents[i][j] = math::Vector3(outTan.x, outTan.y, outTan.z);
                } else {
                    // In-tangent
                    math::Vector3 inTan(
                        readValue<float>(src, outputAccessor.componentType),
                        readValue<float>(src + componentSize, outputAccessor.componentType),
                        readValue<float>(src + componentSize * 2, outputAccessor.componentType)
                    );
                    samplerInTangents[i][j] = inTan;
                    
                    // Value
                    math::Vector3 value(
                        readValue<float>(src + componentSize * 3, outputAccessor.componentType),
                        readValue<float>(src + componentSize * 4, outputAccessor.componentType),
                        readValue<float>(src + componentSize * 5, outputAccessor.componentType)
                    );
                    samplerValues[i][j] = value;
                    
                    // Out-tangent
                    math::Vector3 outTan(
                        readValue<float>(src + componentSize * 6, outputAccessor.componentType),
                        readValue<float>(src + componentSize * 7, outputAccessor.componentType),
                        readValue<float>(src + componentSize * 8, outputAccessor.componentType)
                    );
                    samplerOutTangents[i][j] = outTan;
                }
            }
        } else {
            // Linear or step interpolation
            if (isRotation) {
                // Read quaternion data (rotation channel)
                for (uint32_t j = 0; j < valueCount; ++j) {
                    const uint8_t* src = outputData.data() + byteOffset + j * byteStride;
                    
                    math::Quaternion quat(
                        readValue<float>(src, outputAccessor.componentType),
                        readValue<float>(src + componentSize, outputAccessor.componentType),
                        readValue<float>(src + componentSize * 2, outputAccessor.componentType),
                        readValue<float>(src + componentSize * 3, outputAccessor.componentType)
                    );
                    samplerRotations[i][j] = quat;
                }
            } else {
                // Read vec3 data (translation/scale channels)
                for (uint32_t j = 0; j < valueCount; ++j) {
                    const uint8_t* src = outputData.data() + byteOffset + j * byteStride;
                    
                    math::Vector3 value(
                        readValue<float>(src, outputAccessor.componentType),
                        readValue<float>(src + componentSize, outputAccessor.componentType),
                        readValue<float>(src + componentSize * 2, outputAccessor.componentType)
                    );
                    samplerValues[i][j] = value;
                }
            }
        }
    }
    
    // Second pass: build channels
    std::unordered_map<uint32_t, AnimationChannel> channelMap;
    
    for (const auto& channel : gltfAnimation->channels) {
        if (channel.sampler < 0 || 
            channel.sampler >= static_cast<int>(samplerTimes.size())) {
            continue;
        }
        
        if (channel.target_node < 0 || 
            channel.target_node >= static_cast<int>(model.nodes.size())) {
            continue;
        }
        
        // Find joint index for this node
        int32_t jointIndex = -1;
        for (size_t i = 0; i < outMesh.joints.size(); ++i) {
            // Match by node index (would need to store node index in joint)
            // For now, we'll create a new channel for each target
            jointIndex = static_cast<int32_t>(i);
            break;
        }
        
        if (jointIndex < 0) {
            continue;
        }
        
        // Get or create channel for this joint
        auto it = channelMap.find(jointIndex);
        if (it == channelMap.end()) {
            AnimationChannel newChannel;
            newChannel.jointIndex = jointIndex;
            channelMap[jointIndex] = std::move(newChannel);
            it = channelMap.find(jointIndex);
        }
        
        AnimationChannel& animChannel = it->second;
        
        // Create keyframes
        const auto& times = samplerTimes[channel.sampler];
        const auto& values = samplerValues[channel.sampler];
        const auto& inTangents = samplerInTangents[channel.sampler];
        const auto& outTangents = samplerOutTangents[channel.sampler];
        int interpolation = samplerInterpolations[channel.sampler];
        
        size_t numKeyframes = times.size();
        
        for (size_t k = 0; k < numKeyframes; ++k) {
            Keyframe kf;
            kf.time = times[k];
            
            // Apply based on target path
            if (channel.target_path == "translation") {
                kf.position = values[k];
                // Keep existing rotation and scale
                if (!animChannel.keyframes.empty()) {
                    kf.rotation = animChannel.keyframes.back().rotation;
                    kf.scale = animChannel.keyframes.back().scale;
                }
            } else if (channel.target_path == "rotation") {
                // glTF stores rotation as quaternion [x,y,z,w] - read from samplerRotations
                if (channel.sampler < static_cast<int>(samplerRotations.size()) && 
                    k < samplerRotations[channel.sampler].size()) {
                    kf.rotation = samplerRotations[channel.sampler][k];
                } else {
                    kf.rotation = math::Quaternion(0, 0, 0, 1);  // Fallback to identity
                }
                // Keep existing position and scale
                if (!animChannel.keyframes.empty()) {
                    kf.position = animChannel.keyframes.back().position;
                    kf.scale = animChannel.keyframes.back().scale;
                }
            } else if (channel.target_path == "scale") {
                kf.scale = values[k];
                if (!animChannel.keyframes.empty()) {
                    kf.position = animChannel.keyframes.back().position;
                    kf.rotation = animChannel.keyframes.back().rotation;
                }
            }
            
            animChannel.keyframes.push_back(kf);
        }
    }
    
    // Convert map to vector
    for (auto& [jointIndex, channel] : channelMap) {
        clip.channels.push_back(std::move(channel));
    }
    
    // Calculate duration
    float maxTime = 0.0f;
    for (const auto& channel : clip.channels) {
        for (const auto& kf : channel.keyframes) {
            maxTime = std::max(maxTime, kf.time);
        }
    }
    clip.duration = maxTime;
    clip.fps = clip.duration > 0.0f ? 
               static_cast<float>(clip.channels.empty() ? 30 : 
               clip.channels[0].keyframes.size()) / clip.duration : 30.0f;
    
    outMesh.animations.push_back(std::move(clip));
}

} // namespace phoenix::resource
