#include "../../include/phoenix/resource/asset_loader.hpp"
#include <fstream>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <cmath>

namespace phoenix::resource {

// ============ TerrainLoader ============

TerrainLoader::TerrainLoader(const Config& config) : m_config(config) {}

std::set<std::string> TerrainLoader::getSupportedExtensions() const {
    return {".tif", ".tiff", ".raw", ".heightmap"};
}

AssetType TerrainLoader::getAssetType() const {
    return AssetType::Terrain_GeoTIFF;
}

ValidationResult TerrainLoader::validate(const std::string& path) const {
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
    
    if (size < 8) {
        return ValidationResult::failure("File too small");
    }
    
    file.seekg(0, std::ios::beg);
    
    std::string ext = path.size() >= 4 ? path.substr(path.size() - 4) : "";
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == ".tif" || ext == "ff") {
        // TIFF/GeoTIFF: Check magic number
        uint16_t magic = 0;
        file.read(reinterpret_cast<char*>(&magic), 2);
        
        // Little endian: 0x4949 ("II")
        // Big endian: 0x4D4D ("MM")
        if (magic != 0x4949 && magic != 0x4D4D) {
            return ValidationResult::failure("Invalid TIFF magic number");
        }
        
        // Check version (should be 42)
        uint16_t version = 0;
        file.read(reinterpret_cast<char*>(&version), 2);
        
        if (version != 42) {
            return ValidationResult::failure("Invalid TIFF version");
        }
    }
    else if (ext == ".raw" || ext == "map") {
        // RAW heightmap - just check it's not empty
        if (size < 256) {  // Minimum reasonable size
            return ValidationResult::failure("RAW file too small");
        }
    }
    
    return ValidationResult::success();
}

size_t TerrainLoader::estimateMemoryUsage(const std::string& path) const {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return 0;
    
    std::streamsize size = file.tellg();
    
    // GeoTIFF can be compressed, RAW is uncompressed
    std::string ext = path.size() >= 4 ? path.substr(path.size() - 4) : "";
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == ".tif" || ext == "ff") {
        return static_cast<size_t>(size) * 3;  // Decompressed + normals + splat
    }
    
    return static_cast<size_t>(size) * 2;  // RAW + normals
}

std::future<std::unique_ptr<Terrain>> TerrainLoader::loadAsync(const std::string& path) {
    return std::async(std::launch::async, [this, path]() {
        return load(path);
    });
}

std::unique_ptr<Terrain> TerrainLoader::load(const std::string& path) {
    std::string ext = path.size() >= 4 ? path.substr(path.size() - 4) : "";
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == ".tif" || ext == "ff") return loadGeoTIFF(path);
    if (ext == ".raw" || ext == "map") return loadHeightmapRAW(path);
    
    auto terrain = std::make_unique<Terrain>();
    terrain->loadState = LoadState::Failed;
    terrain->loadError = "Unknown terrain format";
    return terrain;
}

std::unique_ptr<Terrain> TerrainLoader::loadGeoTIFF(const std::string& path) {
    auto terrain = std::make_unique<Terrain>();
    terrain->sourcePath = path;
    terrain->assetType = AssetType::Terrain_GeoTIFF;
    terrain->loadState = LoadState::Loading;
    terrain->verticalScale = m_config.verticalScale;
    terrain->verticalOffset = m_config.verticalOffset;
    
    // Validate
    auto validation = validate(path);
    if (!validation.isValid) {
        terrain->loadState = LoadState::Failed;
        terrain->loadError = validation.error;
        return terrain;
    }
    
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        terrain->loadState = LoadState::Failed;
        terrain->loadError = "Cannot open file";
        return terrain;
    }
    
    // In actual implementation, use GDAL library:
    /*
    GDALDataset* dataset = GDALOpen(path.c_str(), GA_ReadOnly);
    if (!dataset) {
        terrain->loadState = LoadState::Failed;
        terrain->loadError = GDALGetLastErrorMsg();
        return terrain;
    }
    
    GDALRasterBand* band = dataset->GetRasterBand(1);
    int width = band->GetXSize();
    int height = band->GetYSize();
    
    terrain->width = width;
    terrain->depth = height;
    
    // Read height data
    std::vector<float> heights(width * height);
    band->RasterIO(GF_Read, 0, 0, width, height,
                   heights.data(), width, height, GDT_Float32, 0, 0);
    
    terrain->heightData = std::move(heights);
    
    // Read GeoTIFF metadata
    double geoTransform[6];
    if (dataset->GetGeoTransform(geoTransform) == CE_None) {
        terrain->geoMetadata.originX = geoTransform[0];
        terrain->geoMetadata.originY = geoTransform[3];
        terrain->geoMetadata.pixelSizeX = geoTransform[1];
        terrain->geoMetadata.pixelSizeY = geoTransform[5];
    }
    
    // Read projection
    const char* projection = dataset->GetProjectionRef();
    if (projection) {
        terrain->geoMetadata.projectionWKT = projection;
    }
    
    // Calculate min/max elevation
    double min, max;
    band->ComputeRasterMinMax(&min, &max, FALSE);
    terrain->geoMetadata.minElevation = min;
    terrain->geoMetadata.maxElevation = max;
    
    GDALClose(dataset);
    */
    
    // Placeholder implementation
    terrain->width = 1024;
    terrain->depth = 1024;
    terrain->heightData.resize(1024 * 1024, 0.5f);
    terrain->loadState = LoadState::Loaded;
    terrain->name = path;
    
    // Generate normals
    if (m_config.generateNormals) {
        terrain->generateNormals();
    }
    
    // Build chunks for streaming
    if (m_config.useStreaming) {
        terrain->buildChunks(m_config.chunkSize);
    }
    
    return terrain;
}

std::unique_ptr<Terrain> TerrainLoader::loadHeightmapRAW(const std::string& path) {
    auto terrain = std::make_unique<Terrain>();
    terrain->sourcePath = path;
    terrain->assetType = AssetType::Terrain_Heightmap;
    terrain->loadState = LoadState::Loading;
    terrain->verticalScale = m_config.verticalScale;
    terrain->verticalOffset = m_config.verticalOffset;
    
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        terrain->loadState = LoadState::Failed;
        terrain->loadError = "Cannot open file";
        return terrain;
    }
    
    // Get file size
    file.seekg(0, std::ios::end);
    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // Try to determine dimensions
    // Common formats: 512x512, 1024x1024, 2048x2048, etc.
    // 16-bit heightmap: 2 bytes per pixel
    // 8-bit heightmap: 1 byte per pixel
    
    uint32_t size = 0;
    
    // Check if it's a square texture
    uint32_t sqrtSize = static_cast<uint32_t>(std::sqrt(fileSize / 2));  // Assume 16-bit
    
    if (sqrtSize * sqrtSize * 2 == static_cast<size_t>(fileSize)) {
        // 16-bit heightmap
        size = sqrtSize;
    }
    else if (sqrtSize * sqrtSize == static_cast<size_t>(fileSize)) {
        // 8-bit heightmap
        size = sqrtSize;
    }
    else {
        // Try common sizes
        uint32_t commonSizes[] = {64, 128, 256, 512, 1024, 2048, 4096};
        
        for (uint32_t s : commonSizes) {
            if (s * s * 2 == static_cast<size_t>(fileSize)) {
                size = s;
                break;
            }
            if (s * s == static_cast<size_t>(fileSize)) {
                size = s;
                break;
            }
        }
    }
    
    if (size == 0) {
        terrain->loadState = LoadState::Failed;
        terrain->loadError = "Could not determine heightmap dimensions";
        return terrain;
    }
    
    terrain->width = size;
    terrain->depth = size;
    terrain->heightData.resize(size * size);
    
    // Read height data
    bool is16Bit = (fileSize == size * size * 2);
    
    if (is16Bit) {
        std::vector<uint16_t> rawData(size * size);
        file.read(reinterpret_cast<char*>(rawData.data()), fileSize);
        
        for (size_t i = 0; i < rawData.size(); ++i) {
            terrain->heightData[i] = rawData[i] / 65535.0f;
        }
    } else {
        std::vector<uint8_t> rawData(size * size);
        file.read(reinterpret_cast<char*>(rawData.data()), fileSize);
        
        for (size_t i = 0; i < rawData.size(); ++i) {
            terrain->heightData[i] = rawData[i] / 255.0f;
        }
    }
    
    // Calculate bounds
    float minH = FLT_MAX, maxH = -FLT_MAX;
    for (float h : terrain->heightData) {
        minH = std::min(minH, h);
        maxH = std::max(maxH, h);
    }
    
    terrain->bounds.min = math::Vector3(0.0f, minH * terrain->verticalScale + terrain->verticalOffset, 0.0f);
    terrain->bounds.max = math::Vector3(static_cast<float>(size - 1), 
                                        maxH * terrain->verticalScale + terrain->verticalOffset,
                                        static_cast<float>(size - 1));
    terrain->bounds.center = (terrain->bounds.min + terrain->bounds.max) * 0.5f;
    terrain->bounds.radius = math::length(terrain->bounds.max - terrain->bounds.center);
    
    // Generate normals
    if (m_config.generateNormals) {
        terrain->generateNormals();
    }
    
    // Load splatmap if exists
    if (m_config.loadSplatmap) {
        std::string splatPath = path;
        size_t dotPos = splatPath.rfind('.');
        if (dotPos != std::string::npos) {
            splatPath = splatPath.substr(0, dotPos) + "_splat.raw";
            
            std::ifstream splatFile(splatPath, std::ios::binary);
            if (splatFile.is_open()) {
                terrain->splatData.resize(size * size);
                splatFile.read(reinterpret_cast<char*>(terrain->splatData.data()),
                              size * size * sizeof(math::Color4));
            }
        }
    }
    
    // Build chunks for streaming
    if (m_config.useStreaming) {
        terrain->buildChunks(m_config.chunkSize);
    }
    
    terrain->loadState = LoadState::Loaded;
    terrain->name = path;
    
    return terrain;
}

} // namespace phoenix::resource
