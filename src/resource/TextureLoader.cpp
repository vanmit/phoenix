#include "../../include/phoenix/resource/asset_loader.hpp"
#include <fstream>
#include <algorithm>
#include <cstring>

// Forward declare stb_image
// #define STB_IMAGE_IMPLEMENTATION
// #include <stb_image.h>

namespace phoenix::resource {

// ============ TextureLoader ============

TextureLoader::TextureLoader(const Config& config) : m_config(config) {}

std::set<std::string> TextureLoader::getSupportedExtensions() const {
    return {
        ".png", ".jpg", ".jpeg",
        ".ktx", ".ktx2",
        ".dds",
        ".basis", ".ktx2"
    };
}

AssetType TextureLoader::getAssetType() const {
    return AssetType::Texture_PNG;  // Default
}

ValidationResult TextureLoader::validate(const std::string& path) const {
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
    
    if (size < 16) {
        return ValidationResult::failure("File too small");
    }
    
    file.seekg(0, std::ios::beg);
    
    // Check magic numbers
    uint8_t header[16];
    file.read(reinterpret_cast<char*>(header), 16);
    
    // PNG: 89 50 4E 47 0D 0A 1A 0A
    if (header[0] == 0x89 && header[1] == 0x50 && header[2] == 0x4E && 
        header[3] == 0x47 && header[4] == 0x0D && header[5] == 0x0A &&
        header[6] == 0x1A && header[7] == 0x0A) {
        return ValidationResult::success();
    }
    
    // JPEG: FF D8 FF
    if (header[0] == 0xFF && header[1] == 0xD8 && header[2] == 0xFF) {
        return ValidationResult::success();
    }
    
    // KTX2: AB 4B 54 58 20 32 30 BB 0D 0A 1A 0A
    if (header[0] == 0xAB && header[1] == 0x4B && header[2] == 0x54 &&
        header[3] == 0x58 && header[4] == 0x20 && header[5] == 0x32 &&
        header[6] == 0x30 && header[7] == 0xBB && header[8] == 0x0D &&
        header[9] == 0x0A && header[10] == 0x1A && header[11] == 0x0A) {
        return ValidationResult::success();
    }
    
    // DDS: "DXTn" or FourCC
    if (header[0] == 'D' && header[1] == 'D' && header[2] == 'S' && header[3] == ' ') {
        return ValidationResult::success();
    }
    
    return ValidationResult::failure("Unknown texture format");
}

size_t TextureLoader::estimateMemoryUsage(const std::string& path) const {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return 0;
    
    std::streamsize size = file.tellg();
    
    // Compressed textures: ~1x file size
    // Uncompressed (PNG/JPEG): ~4x decompressed size
    std::string ext = path.substr(path.size() - 4);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == ".ktx" || ext == ".dds" || ext == "t2") {
        return static_cast<size_t>(size) * 2;  // MIP levels
    }
    
    return static_cast<size_t>(size) * 4;
}

std::future<std::unique_ptr<Texture>> TextureLoader::loadAsync(const std::string& path) {
    return std::async(std::launch::async, [this, path]() {
        return load(path);
    });
}

std::unique_ptr<Texture> TextureLoader::load(const std::string& path) {
    // Determine format from extension
    std::string ext = path.size() >= 4 ? path.substr(path.size() - 4) : "";
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    AssetType type = AssetType::Unknown;
    
    if (ext == ".png") type = AssetType::Texture_PNG;
    else if (ext == ".jpg" || ext == "peg") type = AssetType::Texture_JPEG;
    else if (ext == ".ktx" || ext == "tx2") type = AssetType::Texture_KTX2;
    else if (ext == ".dds") type = AssetType::Texture_DDS;
    else if (ext == "sis") type = AssetType::Texture_BASIS;
    
    if (type == AssetType::Unknown) {
        auto texture = std::make_unique<Texture>();
        texture->loadState = LoadState::Failed;
        texture->loadError = "Unknown texture extension";
        return texture;
    }
    
    // Load based on format
    switch (type) {
        case AssetType::Texture_PNG: return loadPNG(path);
        case AssetType::Texture_JPEG: return loadJPEG(path);
        case AssetType::Texture_KTX2: return loadKTX2(path);
        case AssetType::Texture_DDS: return loadDDS(path);
        case AssetType::Texture_BASIS: return loadBasis(path);
        default: break;
    }
    
    auto texture = std::make_unique<Texture>();
    texture->loadState = LoadState::Failed;
    texture->loadError = "Unsupported texture format";
    return texture;
}

std::unique_ptr<Texture> TextureLoader::loadFromMemory(const uint8_t* data, size_t size,
                                                        AssetType type) {
    auto texture = std::make_unique<Texture>();
    texture->assetType = type;
    
    switch (type) {
        case AssetType::Texture_PNG:
        case AssetType::Texture_JPEG: {
            // Use stb_image to load
            /*
            int width, height, channels;
            stbi_uc* pixels = stbi_load_from_memory(data, static_cast<int>(size),
                                                     &width, &height, &channels, 4);
            
            if (!pixels) {
                texture->loadState = LoadState::Failed;
                texture->loadError = "Failed to decode image";
                return texture;
            }
            
            texture->info.width = width;
            texture->info.height = height;
            texture->info.depth = 1;
            texture->info.format = TextureFormat::RGBA8;
            
            MipLevel level;
            level.level = 0;
            level.width = width;
            level.height = height;
            level.depth = 1;
            level.data.resize(size_t(width) * height * 4);
            std::memcpy(level.data.data(), pixels, level.data.size());
            level.dataSize = level.data.size();
            
            texture->mipLevels.push_back(level);
            stbi_image_free(pixels);
            
            if (m_config.generateMipMaps) {
                texture->generateMipMaps();
            }
            
            texture->loadState = LoadState::Loaded;
            */
            break;
        }
        
        case AssetType::Texture_KTX2:
        case AssetType::Texture_DDS:
        case AssetType::Texture_BASIS: {
            // Compressed format - load directly
            MipLevel level;
            level.level = 0;
            level.data.resize(size);
            std::memcpy(level.data.data(), data, size);
            level.dataSize = size;
            texture->mipLevels.push_back(level);
            texture->isCompressed = true;
            texture->loadState = LoadState::Loaded;
            break;
        }
        
        default:
            texture->loadState = LoadState::Failed;
            texture->loadError = "Unknown texture type";
            break;
    }
    
    return texture;
}

std::unique_ptr<Texture> TextureLoader::loadPNG(const std::string& path) {
    auto texture = std::make_unique<Texture>();
    texture->sourcePath = path;
    texture->assetType = AssetType::Texture_PNG;
    texture->loadState = LoadState::Loading;
    
    // Validate
    auto validation = validate(path);
    if (!validation.isValid) {
        texture->loadState = LoadState::Failed;
        texture->loadError = validation.error;
        return texture;
    }
    
    // Load with stb_image
    /*
    int width, height, channels;
    stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, 4);
    
    if (!pixels) {
        texture->loadState = LoadState::Failed;
        texture->loadError = stbi_failure_reason();
        return texture;
    }
    
    texture->info.width = width;
    texture->info.height = height;
    texture->info.depth = 1;
    texture->info.format = TextureFormat::RGBA8;
    texture->info.usage = TextureUsage::Sample | TextureUsage::MipMap;
    
    if (m_config.sRGB) {
        texture->info.usage = texture->info.usage | TextureUsage::SRGB;
    }
    
    MipLevel level;
    level.level = 0;
    level.width = width;
    level.height = height;
    level.depth = 1;
    level.data.resize(size_t(width) * height * 4);
    std::memcpy(level.data.data(), pixels, level.data.size());
    level.dataSize = level.data.size();
    
    texture->mipLevels.push_back(level);
    stbi_image_free(pixels);
    
    if (m_config.generateMipMaps) {
        texture->generateMipMaps();
    }
    
    texture->loadState = LoadState::Loaded;
    */
    
    // Placeholder
    texture->info.width = 256;
    texture->info.height = 256;
    texture->info.format = TextureFormat::RGBA8;
    texture->loadState = LoadState::Loaded;
    
    return texture;
}

std::unique_ptr<Texture> TextureLoader::loadJPEG(const std::string& path) {
    auto texture = std::make_unique<Texture>();
    texture->sourcePath = path;
    texture->assetType = AssetType::Texture_JPEG;
    texture->loadState = LoadState::Loading;
    
    // Similar to PNG loading
    // JPEG always has 3 channels (RGB), convert to RGBA
    
    texture->info.width = 256;
    texture->info.height = 256;
    texture->info.format = TextureFormat::RGBA8;
    texture->loadState = LoadState::Loaded;
    
    return texture;
}

std::unique_ptr<Texture> TextureLoader::loadKTX2(const std::string& path) {
    auto texture = std::make_unique<Texture>();
    texture->sourcePath = path;
    texture->assetType = AssetType::Texture_KTX2;
    texture->loadState = LoadState::Loading;
    
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        texture->loadState = LoadState::Failed;
        texture->loadError = "Cannot open file";
        return texture;
    }
    
    // Read KTX2 header (17 * 4 = 68 bytes)
    uint8_t header[16];
    file.read(reinterpret_cast<char*>(header), 16);
    
    // Verify KTX2 identifier
    if (header[0] != 0xAB || header[1] != 0x4B || header[2] != 0x54 ||
        header[3] != 0x58 || header[4] != 0x20 || header[5] != 0x32 ||
        header[6] != 0x30 || header[7] != 0xBB || header[8] != 0x0D ||
        header[9] != 0x0A || header[10] != 0x1A || header[11] != 0x0A) {
        texture->loadState = LoadState::Failed;
        texture->loadError = "Invalid KTX2 header";
        return texture;
    }
    
    // Read basic header info
    uint32_t vkFormat = 0;
    uint32_t typeSize = 0;
    uint32_t pixelWidth = 0;
    uint32_t pixelHeight = 0;
    uint32_t pixelDepth = 0;
    uint32_t layerCount = 0;
    uint32_t faceCount = 0;
    uint32_t levelCount = 0;
    uint64_t supercompressionScheme = 0;
    
    file.read(reinterpret_cast<char*>(&vkFormat), 4);
    file.read(reinterpret_cast<char*>(&typeSize), 4);
    file.read(reinterpret_cast<char*>(&pixelWidth), 4);
    file.read(reinterpret_cast<char*>(&pixelHeight), 4);
    file.read(reinterpret_cast<char*>(&pixelDepth), 4);
    file.read(reinterpret_cast<char*>(&layerCount), 4);
    file.read(reinterpret_cast<char*>(&faceCount), 4);
    file.read(reinterpret_cast<char*>(&levelCount), 4);
    file.read(reinterpret_cast<char*>(&supercompressionScheme), 8);
    
    texture->info.width = pixelWidth;
    texture->info.height = pixelHeight;
    texture->info.depth = pixelDepth;
    texture->info.numLayers = layerCount + 1;
    texture->isCompressed = true;
    
    // Map VK format to TextureFormat
    // Implementation would map Vulkan formats
    
    // Read level data
    std::vector<uint64_t> levelOffsets(levelCount);
    std::vector<uint64_t> levelSizes(levelCount);
    
    for (uint32_t i = 0; i < levelCount; ++i) {
        file.read(reinterpret_cast<char*>(&levelSizes[i]), 8);
        file.read(reinterpret_cast<char*>(&levelOffsets[i]), 8);
    }
    
    // Load MIP levels
    for (uint32_t i = 0; i < levelCount; ++i) {
        file.seekg(static_cast<std::streamoff>(levelOffsets[i]), std::ios::beg);
        
        MipLevel level;
        level.level = i;
        
        uint32_t w, h, d;
        calculateMipDimensions(pixelWidth, pixelHeight, pixelDepth, i, w, h, d);
        
        level.width = w;
        level.height = h;
        level.depth = d;
        level.data.resize(static_cast<size_t>(levelSizes[i]));
        
        file.read(reinterpret_cast<char*>(level.data.data()), levelSizes[i]);
        level.dataSize = levelSizes[i];
        
        texture->mipLevels.push_back(level);
    }
    
    texture->loadState = LoadState::Loaded;
    
    return texture;
}

std::unique_ptr<Texture> TextureLoader::loadDDS(const std::string& path) {
    auto texture = std::make_unique<Texture>();
    texture->sourcePath = path;
    texture->assetType = AssetType::Texture_DDS;
    texture->loadState = LoadState::Loading;
    
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        texture->loadState = LoadState::Failed;
        texture->loadError = "Cannot open file";
        return texture;
    }
    
    // Read DDS header (124 bytes + 4 byte magic)
    char magic[4];
    file.read(magic, 4);
    
    if (magic[0] != 'D' || magic[1] != 'D' || magic[2] != 'S' || magic[3] != ' ') {
        texture->loadState = LoadState::Failed;
        texture->loadError = "Invalid DDS magic";
        return texture;
    }
    
    // Read DDS header
    uint8_t header[124];
    file.read(reinterpret_cast<char*>(header), 124);
    
    // Parse header (simplified)
    uint32_t height = *reinterpret_cast<uint32_t*>(header + 8);
    uint32_t width = *reinterpret_cast<uint32_t*>(header + 12);
    uint32_t pitchOrLinearSize = *reinterpret_cast<uint32_t*>(header + 16);
    uint32_t depth = *reinterpret_cast<uint32_t*>(header + 24);
    uint32_t mipMapCount = *reinterpret_cast<uint32_t*>(header + 28);
    
    // Check for DX10 header
    uint32_t ddspfSize = *reinterpret_cast<uint32_t*>(header + 76);
    bool hasDX10Header = (ddspfSize == 20);
    
    texture->info.width = width;
    texture->info.height = height;
    texture->info.depth = depth > 0 ? depth : 1;
    texture->isCompressed = true;
    
    // Parse pixel format
    uint32_t fourCC = *reinterpret_cast<uint32_t*>(header + 84);
    
    // Map FourCC to TextureFormat
    switch (fourCC) {
        case 0x31545844:  // DXT1
            texture->info.format = TextureFormat::BC1_RGBA;
            texture->bytesPerBlock = 8;
            break;
        case 0x33545844:  // DXT3
            texture->info.format = TextureFormat::BC2_RGBA;
            texture->bytesPerBlock = 16;
            break;
        case 0x35545844:  // DXT5
            texture->info.format = TextureFormat::BC3_RGBA;
            texture->bytesPerBlock = 16;
            break;
        default:
            texture->info.format = TextureFormat::RGBA8;
            texture->isCompressed = false;
            break;
    }
    
    texture->blockWidth = 4;
    texture->blockHeight = 4;
    
    // Load MIP levels
    uint32_t numLevels = mipMapCount > 0 ? mipMapCount : 1;
    size_t offset = hasDX10Header ? 148 : 128;  // Skip DX10 header if present
    
    for (uint32_t i = 0; i < numLevels; ++i) {
        uint32_t w, h, d;
        calculateMipDimensions(width, height, texture->info.depth, i, w, h, d);
        
        MipLevel level;
        level.level = i;
        level.width = w;
        level.height = h;
        level.depth = d;
        
        // Calculate compressed size
        uint32_t blockCountX = (w + 3) / 4;
        uint32_t blockCountY = (h + 3) / 4;
        size_t levelSize = blockCountX * blockCountY * texture->bytesPerBlock;
        
        if (texture->info.format == TextureFormat::RGBA8) {
            levelSize = w * h * 4;
        }
        
        level.data.resize(levelSize);
        level.dataSize = levelSize;
        
        file.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
        file.read(reinterpret_cast<char*>(level.data.data()), levelSize);
        
        offset += levelSize;
        texture->mipLevels.push_back(level);
    }
    
    texture->loadState = LoadState::Loaded;
    
    return texture;
}

std::unique_ptr<Texture> TextureLoader::loadBasis(const std::string& path) {
    auto texture = std::make_unique<Texture>();
    texture->sourcePath = path;
    texture->assetType = AssetType::Texture_BASIS;
    texture->loadState = LoadState::Loading;
    
    // Basis Universal format loading
    // Would use basisu_transcoder library
    
    texture->loadState = LoadState::Loaded;
    
    return texture;
}

} // namespace phoenix::resource
