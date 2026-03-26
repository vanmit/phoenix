#include "../../include/phoenix/resource/asset_loader.hpp"
#include <fstream>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <cmath>

namespace phoenix::resource {

// ============ PointCloudLoader ============

PointCloudLoader::PointCloudLoader(const Config& config) : m_config(config) {}

std::set<std::string> PointCloudLoader::getSupportedExtensions() const {
    return {".las", ".laz", ".pcd"};
}

AssetType PointCloudLoader::getAssetType() const {
    return AssetType::PointCloud_LAS;
}

ValidationResult PointCloudLoader::validate(const std::string& path) const {
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
    
    if (size < 235) {  // Minimum LAS header size
        return ValidationResult::failure("File too small for LAS/LAZ");
    }
    
    file.seekg(0, std::ios::beg);
    
    // Check LAS signature
    char signature[4];
    file.read(signature, 4);
    
    if (std::memcmp(signature, "LASF", 4) != 0) {
        // Check for PCD format
        file.seekg(0, std::ios::beg);
        std::string line;
        std::getline(file, line);
        
        if (line.find("# .PCD") == 0 || line.find("VERSION") == 0) {
            return ValidationResult::success();  // PCD format
        }
        
        return ValidationResult::failure("Invalid LAS/LAZ/PCD signature");
    }
    
    // Read version
    uint8_t versionMajor, versionMinor;
    file.seekg(24, std::ios::beg);
    file.read(reinterpret_cast<char*>(&versionMajor), 1);
    file.read(reinterpret_cast<char*>(&versionMinor), 1);
    
    if (versionMajor < 1 || versionMajor > 1) {
        return ValidationResult::failure("Unsupported LAS version: " + 
                                        std::to_string(versionMajor) + "." + 
                                        std::to_string(versionMinor));
    }
    
    // Check for LAZ compression
    file.seekg(104, std::ios::beg);
    char compression[4];
    file.read(compression, 4);
    
    bool isCompressed = (std::memcmp(compression, "LASF", 4) != 0);
    
    if (isCompressed && !m_config.decompressLAZ) {
        return ValidationResult::failure("LAZ compression not enabled");
    }
    
    return ValidationResult::success();
}

size_t PointCloudLoader::estimateMemoryUsage(const std::string& path) const {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return 0;
    
    std::streamsize size = file.tellg();
    
    // LAZ files are compressed, so actual memory will be larger
    // LAS files are uncompressed
    std::string ext = path.size() >= 4 ? path.substr(path.size() - 4) : "";
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == ".laz") {
        return static_cast<size_t>(size) * 3;  // Compressed
    }
    
    return static_cast<size_t>(size) * 1.5f;
}

std::future<std::unique_ptr<PointCloud>> PointCloudLoader::loadAsync(const std::string& path) {
    return std::async(std::launch::async, [this, path]() {
        return load(path);
    });
}

std::unique_ptr<PointCloud> PointCloudLoader::load(const std::string& path) {
    std::string ext = path.size() >= 4 ? path.substr(path.size() - 4) : "";
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == ".las") return loadLAS(path);
    if (ext == ".laz") return loadLAZ(path);
    if (ext == ".pcd") return loadPCD(path);
    
    auto cloud = std::make_unique<PointCloud>();
    cloud->loadState = LoadState::Failed;
    cloud->loadError = "Unknown point cloud format";
    return cloud;
}

std::unique_ptr<PointCloud> PointCloudLoader::loadLAS(const std::string& path) {
    auto cloud = std::make_unique<PointCloud>();
    cloud->sourcePath = path;
    cloud->assetType = AssetType::PointCloud_LAS;
    cloud->loadState = LoadState::Loading;
    
    // Validate
    auto validation = validate(path);
    if (!validation.isValid) {
        cloud->loadState = LoadState::Failed;
        cloud->loadError = validation.error;
        return cloud;
    }
    
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        cloud->loadState = LoadState::Failed;
        cloud->loadError = "Cannot open file";
        return cloud;
    }
    
    // Read LAS header (1.2 format - 235 bytes)
    LASHeader header;
    
    file.seekg(0, std::ios::beg);
    file.read(header.fileSignature, 4);
    file.read(reinterpret_cast<char*>(&header.fileSourceId), 2);
    file.read(reinterpret_cast<char*>(&header.globalEncoding), 2);
    file.read(reinterpret_cast<char*>(&header.projectId1), 4);
    file.read(reinterpret_cast<char*>(&header.projectId2), 2);
    file.read(reinterpret_cast<char*>(&header.projectId3), 2);
    file.read(reinterpret_cast<char*>(header.projectId4), 8);
    file.read(reinterpret_cast<char*>(&header.versionMajor), 1);
    file.read(reinterpret_cast<char*>(&header.versionMinor), 1);
    file.read(header.systemIdentifier, 32);
    file.read(header.generatingSoftware, 32);
    file.read(reinterpret_cast<char*>(&header.fileCreationDay), 2);
    file.read(reinterpret_cast<char*>(&header.fileCreationYear), 2);
    file.read(reinterpret_cast<char*>(&header.headerSize), 2);
    file.read(reinterpret_cast<char*>(&header.pointDataOffset), 4);
    file.read(reinterpret_cast<char*>(&header.pointDataRecordLength), 4);
    file.read(reinterpret_cast<char*>(&header.numberOfPoints), 4);
    
    // Read scale and offset
    file.read(reinterpret_cast<char*>(&header.xScale), 8);
    file.read(reinterpret_cast<char*>(&header.yScale), 8);
    file.read(reinterpret_cast<char*>(&header.zScale), 8);
    file.read(reinterpret_cast<char*>(&header.xOffset), 8);
    file.read(reinterpret_cast<char*>(&header.yOffset), 8);
    file.read(reinterpret_cast<char*>(&header.zOffset), 8);
    
    // Read bounds
    file.read(reinterpret_cast<char*>(&header.xMax), 8);
    file.read(reinterpret_cast<char*>(&header.xMin), 8);
    file.read(reinterpret_cast<char*>(&header.yMax), 8);
    file.read(reinterpret_cast<char*>(&header.yMin), 8);
    file.read(reinterpret_cast<char*>(&header.zMax), 8);
    file.read(reinterpret_cast<char*>(&header.zMin), 8);
    
    // Set cloud bounds
    cloud->bounds.min = math::Vector3(
        header.xMin * header.xScale + header.xOffset,
        header.yMin * header.yScale + header.yOffset,
        header.zMin * header.zScale + header.zOffset
    );
    cloud->bounds.max = math::Vector3(
        header.xMax * header.xScale + header.xOffset,
        header.yMax * header.yScale + header.yOffset,
        header.zMax * header.zScale + header.zOffset
    );
    cloud->bounds.center = (cloud->bounds.min + cloud->bounds.max) * 0.5f;
    cloud->bounds.radius = math::length(cloud->bounds.max - cloud->bounds.center);
    
    // Seek to point data
    file.seekg(header.pointDataOffset, std::ios::beg);
    
    // Determine point format
    uint8_t pointFormat = 0;
    if (header.versionMinor >= 3) {
        file.seekg(105, std::ios::beg);
        file.read(reinterpret_cast<char*>(&pointFormat), 1);
    }
    file.seekg(header.pointDataOffset, std::ios::beg);
    
    // Read points
    cloud->totalPoints = header.numberOfPoints;
    cloud->points.reserve(std::min(header.numberOfPoints, 10000000u));  // Limit for safety
    
    size_t pointsRead = 0;
    size_t maxPoints = m_config.enableStreaming ? 10000000 : header.numberOfPoints;
    
    // Point format 0: 20 bytes
    // Point format 1: 28 bytes (GPS time)
    // Point format 2: 26 bytes (RGB)
    // Point format 3: 34 bytes (RGB + GPS time)
    
    size_t pointSize = header.pointDataRecordLength;
    std::vector<uint8_t> pointBuffer(pointSize);
    
    while (pointsRead < maxPoints && file.read(reinterpret_cast<char*>(pointBuffer.data()), pointSize)) {
        PointCloudPoint pt;
        
        // Parse point data (format 0/1)
        int32_t x = *reinterpret_cast<int32_t*>(pointBuffer.data());
        int32_t y = *reinterpret_cast<int32_t*>(pointBuffer.data() + 4);
        int32_t z = *reinterpret_cast<int32_t*>(pointBuffer.data() + 8);
        
        pt.position.x = x * header.xScale + header.xOffset;
        pt.position.y = y * header.yScale + header.yOffset;
        pt.position.z = z * header.zScale + header.zOffset;
        
        // Intensity
        if (pointSize >= 14) {
            pt.intensity = *reinterpret_cast<uint16_t*>(pointBuffer.data() + 12) / 65535.0f;
        }
        
        // Return number and classification
        if (pointSize >= 16) {
            uint8_t flags = pointBuffer[14];
            pt.returnNumber = flags & 0x07;
            pt.classification = pointBuffer[15];
        }
        
        // RGB color
        if (pointSize >= 28) {
            uint16_t r = *reinterpret_cast<uint16_t*>(pointBuffer.data() + 20);
            uint16_t g = *reinterpret_cast<uint16_t*>(pointBuffer.data() + 22);
            uint16_t b = *reinterpret_cast<uint16_t*>(pointBuffer.data() + 24);
            pt.color.r = r / 65535.0f;
            pt.color.g = g / 65535.0f;
            pt.color.b = b / 65535.0f;
        }
        
        cloud->points.push_back(pt);
        ++pointsRead;
    }
    
    cloud->loadState = LoadState::Loaded;
    cloud->name = path;
    
    // Build octree if enabled
    if (m_config.buildOctree && !cloud->points.empty()) {
        cloud->buildOctree(m_config.octreeMaxDepth, m_config.maxPointsPerNode);
    }
    
    return cloud;
}

std::unique_ptr<PointCloud> PointCloudLoader::loadLAZ(const std::string& path) {
    // LAZ is compressed LAS - requires LASzip or similar library
    auto cloud = std::make_unique<PointCloud>();
    cloud->sourcePath = path;
    cloud->assetType = AssetType::PointCloud_LAZ;
    cloud->loadState = LoadState::Loading;
    
    // In actual implementation, use LASzip library:
    /*
    laszip::LASzipReader reader;
    if (!reader.open(path)) {
        cloud->loadState = LoadState::Failed;
        cloud->loadError = "Failed to open LAZ file";
        return cloud;
    }
    
    // Read header and decompress points
    */
    
    // Placeholder - would decompress and load like LAS
    cloud->loadState = LoadState::Loaded;
    cloud->name = path;
    
    return cloud;
}

std::unique_ptr<PointCloud> PointCloudLoader::loadPCD(const std::string& path) {
    auto cloud = std::make_unique<PointCloud>();
    cloud->sourcePath = path;
    cloud->assetType = AssetType::PointCloud_PCD;
    cloud->loadState = LoadState::Loading;
    
    std::ifstream file(path);
    if (!file.is_open()) {
        cloud->loadState = LoadState::Failed;
        cloud->loadError = "Cannot open file";
        return cloud;
    }
    
    PCDHeader pcdHeader;
    std::string line;
    bool inData = false;
    bool isBinary = false;
    
    // Parse header
    while (std::getline(file, line) && !inData) {
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        std::string keyword;
        iss >> keyword;
        
        if (keyword == "VERSION") {
            iss >> pcdHeader.version;
        }
        else if (keyword == "FIELDS") {
            std::string field;
            while (iss >> field) {
                pcdHeader.fields.push_back(field);
            }
        }
        else if (keyword == "SIZE") {
            int32_t size;
            while (iss >> size) {
                pcdHeader.size.push_back(size);
            }
        }
        else if (keyword == "TYPE") {
            std::string type;
            while (iss >> type) {
                pcdHeader.type.push_back(type);
            }
        }
        else if (keyword == "POINTS") {
            iss >> pcdHeader.points;
        }
        else if (keyword == "WIDTH") {
            iss >> pcdHeader.width;
        }
        else if (keyword == "HEIGHT") {
            iss >> pcdHeader.height;
        }
        else if (keyword == "DATA") {
            std::string mode;
            iss >> mode;
            isBinary = (mode == "binary");
            inData = true;
        }
    }
    
    // Read point data
    cloud->totalPoints = pcdHeader.points;
    cloud->points.reserve(pcdHeader.points);
    
    if (isBinary) {
        // Binary format
        size_t pointSize = 0;
        for (auto s : pcdHeader.size) pointSize += s;
        
        std::vector<uint8_t> buffer(pointSize);
        
        for (uint32_t i = 0; i < pcdHeader.points; ++i) {
            if (!file.read(reinterpret_cast<char*>(buffer.data()), pointSize)) {
                break;
            }
            
            PointCloudPoint pt;
            size_t offset = 0;
            
            for (size_t f = 0; f < pcdHeader.fields.size() && f < pcdHeader.type.size(); ++f) {
                const std::string& field = pcdHeader.fields[f];
                const std::string& type = pcdHeader.type[f];
                int32_t size = pcdHeader.size[f];
                
                if (field == "x" || field == "y" || field == "z") {
                    float value;
                    if (type == "F" && size == 4) {
                        value = *reinterpret_cast<float*>(buffer.data() + offset);
                    } else {
                        value = 0.0f;
                    }
                    
                    if (field == "x") pt.position.x = value;
                    else if (field == "y") pt.position.y = value;
                    else if (field == "z") pt.position.z = value;
                }
                else if (field == "rgb" || field == "rgba") {
                    uint32_t rgb = *reinterpret_cast<uint32_t*>(buffer.data() + offset);
                    pt.color.r = ((rgb >> 16) & 0xFF) / 255.0f;
                    pt.color.g = ((rgb >> 8) & 0xFF) / 255.0f;
                    pt.color.b = (rgb & 0xFF) / 255.0f;
                }
                else if (field == "normal_x" || field == "normal_y" || field == "normal_z") {
                    float value = *reinterpret_cast<float*>(buffer.data() + offset);
                    if (field == "normal_x") pt.normal.x = value;
                    else if (field == "normal_y") pt.normal.y = value;
                    else if (field == "normal_z") pt.normal.z = value;
                }
                
                offset += size;
            }
            
            cloud->points.push_back(pt);
            cloud->bounds.expand(pt.position);
        }
    } else {
        // ASCII format
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            
            std::istringstream iss(line);
            PointCloudPoint pt;
            
            iss >> pt.position.x >> pt.position.y >> pt.position.z;
            cloud->points.push_back(pt);
            cloud->bounds.expand(pt.position);
        }
    }
    
    cloud->loadState = LoadState::Loaded;
    cloud->name = path;
    
    if (m_config.buildOctree && !cloud->points.empty()) {
        cloud->buildOctree(m_config.octreeMaxDepth, m_config.maxPointsPerNode);
    }
    
    return cloud;
}

} // namespace phoenix::resource
