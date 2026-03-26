#include "../../include/phoenix/resource/texture.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace phoenix::resource {

// ============ Texture ============

size_t Texture::calculateMemoryUsage() const {
    size_t total = 0;
    
    for (const auto& mip : mipLevels) {
        total += mip.dataSize;
    }
    
    return total;
}

uint8_t Texture::getFormatSize(TextureFormat format) {
    switch (format) {
        case TextureFormat::R8: return 1;
        case TextureFormat::RG8: return 2;
        case TextureFormat::RGB8: return 3;
        case TextureFormat::RGBA8: return 4;
        
        case TextureFormat::R16F: return 2;
        case TextureFormat::RG16F: return 4;
        case TextureFormat::RGB16F: return 6;
        case TextureFormat::RGBA16F: return 8;
        
        case TextureFormat::R32F: return 4;
        case TextureFormat::RG32F: return 8;
        case TextureFormat::RGB32F: return 12;
        case TextureFormat::RGBA32F: return 16;
        
        // Compressed formats - bytes per block
        case TextureFormat::BC1_RGBA: return 8;   // 4x4 block
        case TextureFormat::BC2_RGBA: return 16;  // 4x4 block
        case TextureFormat::BC3_RGBA: return 16;  // 4x4 block
        case TextureFormat::BC4_R: return 8;      // 4x4 block
        case TextureFormat::BC5_RG: return 16;    // 4x4 block
        case TextureFormat::BC6H_UF16: return 16; // 4x4 block
        case TextureFormat::BC6H_SF16: return 16; // 4x4 block
        case TextureFormat::BC7_RGBA: return 16;  // 4x4 block
        
        // ASTC - bytes per block
        case TextureFormat::ASTC_4x4: return 16;
        case TextureFormat::ASTC_5x4: return 16;
        case TextureFormat::ASTC_5x5: return 16;
        case TextureFormat::ASTC_6x5: return 16;
        case TextureFormat::ASTC_6x6: return 16;
        case TextureFormat::ASTC_8x5: return 16;
        case TextureFormat::ASTC_8x6: return 16;
        case TextureFormat::ASTC_8x8: return 16;
        case TextureFormat::ASTC_10x5: return 16;
        case TextureFormat::ASTC_10x6: return 16;
        case TextureFormat::ASTC_10x8: return 16;
        case TextureFormat::ASTC_10x10: return 16;
        case TextureFormat::ASTC_12x10: return 16;
        case TextureFormat::ASTC_12x12: return 16;
        
        // ETC2
        case TextureFormat::ETC2_RGB8: return 8;   // 4x4 block
        case TextureFormat::ETC2_SRGB8: return 8;  // 4x4 block
        case TextureFormat::ETC2_RGB8_A1: return 8; // 4x4 block
        case TextureFormat::ETC2_RGBA8: return 16;  // 4x4 block
        
        // Depth
        case TextureFormat::D16: return 2;
        case TextureFormat::D24: return 3;
        case TextureFormat::D32F: return 4;
        case TextureFormat::D24S8: return 4;
        case TextureFormat::D32FS8: return 5;
        
        default: return 4;
    }
}

void Texture::getBlockDimensions(TextureFormat format, uint32_t& width, uint32_t& height) {
    width = 1;
    height = 1;
    
    switch (format) {
        case TextureFormat::BC1_RGBA:
        case TextureFormat::BC2_RGBA:
        case TextureFormat::BC3_RGBA:
        case TextureFormat::BC4_R:
        case TextureFormat::BC5_RG:
        case TextureFormat::BC6H_UF16:
        case TextureFormat::BC6H_SF16:
        case TextureFormat::BC7_RGBA:
        case TextureFormat::ETC2_RGB8:
        case TextureFormat::ETC2_SRGB8:
        case TextureFormat::ETC2_RGB8_A1:
        case TextureFormat::ETC2_RGBA8:
            width = 4;
            height = 4;
            break;
            
        case TextureFormat::ASTC_4x4: width = 4; height = 4; break;
        case TextureFormat::ASTC_5x4: width = 5; height = 4; break;
        case TextureFormat::ASTC_5x5: width = 5; height = 5; break;
        case TextureFormat::ASTC_6x5: width = 6; height = 5; break;
        case TextureFormat::ASTC_6x6: width = 6; height = 6; break;
        case TextureFormat::ASTC_8x5: width = 8; height = 5; break;
        case TextureFormat::ASTC_8x6: width = 8; height = 6; break;
        case TextureFormat::ASTC_8x8: width = 8; height = 8; break;
        case TextureFormat::ASTC_10x5: width = 10; height = 5; break;
        case TextureFormat::ASTC_10x6: width = 10; height = 6; break;
        case TextureFormat::ASTC_10x8: width = 10; height = 8; break;
        case TextureFormat::ASTC_10x10: width = 10; height = 10; break;
        case TextureFormat::ASTC_12x10: width = 12; height = 10; break;
        case TextureFormat::ASTC_12x12: width = 12; height = 12; break;
            
        default:
            break;
    }
}

void Texture::generateMipMaps() {
    if (mipLevels.empty() || mipLevels[0].data.empty()) {
        return;
    }
    
    const auto& baseLevel = mipLevels[0];
    uint32_t width = baseLevel.width;
    uint32_t height = baseLevel.height;
    uint32_t depth = baseLevel.depth;
    
    uint32_t numLevels = calculateMipLevels(width, height, depth);
    
    mipLevels.resize(numLevels);
    mipLevels[0] = baseLevel;
    
    for (uint32_t level = 1; level < numLevels; ++level) {
        uint32_t prevWidth = mipLevels[level - 1].width;
        uint32_t prevHeight = mipLevels[level - 1].height;
        uint32_t prevDepth = mipLevels[level - 1].depth;
        
        uint32_t currWidth = std::max(1u, prevWidth / 2);
        uint32_t currHeight = std::max(1u, prevHeight / 2);
        uint32_t currDepth = std::max(1u, prevDepth / 2);
        
        MipLevel& mip = mipLevels[level];
        mip.level = level;
        mip.width = currWidth;
        mip.height = currHeight;
        mip.depth = currDepth;
        
        // Simple box filter for MIP generation
        const uint8_t* src = mipLevels[level - 1].data.data();
        size_t pixelSize = getFormatSize(info.format);
        
        // For compressed formats, we'd need to decompress first
        // This is a simplified implementation for uncompressed formats
        if (!isCompressed) {
            size_t mipSize = currWidth * currHeight * currDepth * pixelSize;
            mip.data.resize(mipSize);
            mip.dataSize = mipSize;
            
            uint8_t* dst = mip.data.data();
            
            for (uint32_t z = 0; z < currDepth; ++z) {
                for (uint32_t y = 0; y < currHeight; ++y) {
                    for (uint32_t x = 0; x < currWidth; ++x) {
                        // Average 2x2x2 block
                        math::Color4 color(0.0f);
                        int count = 0;
                        
                        for (uint32_t dz = 0; dz < 2 && z * 2 + dz < prevDepth; ++dz) {
                            for (uint32_t dy = 0; dy < 2 && y * 2 + dy < prevHeight; ++dy) {
                                for (uint32_t dx = 0; dx < 2 && x * 2 + dx < prevWidth; ++dx) {
                                    uint32_t srcX = x * 2 + dx;
                                    uint32_t srcY = y * 2 + dy;
                                    uint32_t srcZ = z * 2 + dz;
                                    
                                    size_t srcOffset = ((srcZ * prevHeight + srcY) * prevWidth + srcX) * pixelSize;
                                    const uint8_t* srcPixel = src + srcOffset;
                                    
                                    if (pixelSize >= 1) color.r += srcPixel[0] / 255.0f;
                                    if (pixelSize >= 2) color.g += srcPixel[1] / 255.0f;
                                    if (pixelSize >= 3) color.b += srcPixel[2] / 255.0f;
                                    if (pixelSize >= 4) color.a += srcPixel[3] / 255.0f;
                                    ++count;
                                }
                            }
                        }
                        
                        float invCount = 1.0f / count;
                        color *= invCount;
                        
                        size_t dstOffset = ((z * currHeight + y) * currWidth + x) * pixelSize;
                        uint8_t* dstPixel = dst + dstOffset;
                        
                        if (pixelSize >= 1) dstPixel[0] = static_cast<uint8_t>(color.r * 255.0f);
                        if (pixelSize >= 2) dstPixel[1] = static_cast<uint8_t>(color.g * 255.0f);
                        if (pixelSize >= 3) dstPixel[2] = static_cast<uint8_t>(color.b * 255.0f);
                        if (pixelSize >= 4) dstPixel[3] = static_cast<uint8_t>(color.a * 255.0f);
                    }
                }
            }
        }
    }
}

ValidationResult Texture::validate() const {
    if (info.width == 0 || info.height == 0) {
        return ValidationResult::failure("Texture has zero dimensions");
    }
    
    if (mipLevels.empty()) {
        return ValidationResult::failure("Texture has no MIP levels");
    }
    
    // Validate MIP chain
    uint32_t expectedWidth = info.width;
    uint32_t expectedHeight = info.height;
    uint32_t expectedDepth = info.depth;
    
    for (const auto& mip : mipLevels) {
        if (mip.width != expectedWidth || mip.height != expectedHeight || mip.depth != expectedDepth) {
            return ValidationResult::failure("MIP level " + std::to_string(mip.level) + 
                                           " has unexpected dimensions");
        }
        
        if (mip.data.empty() && mip.dataSize == 0 && !isCompressed) {
            return ValidationResult::failure("MIP level " + std::to_string(mip.level) + 
                                           " has no data");
        }
        
        // Check for reasonable data size
        if (!isCompressed) {
            uint8_t pixelSize = getFormatSize(info.format);
            size_t expectedSize = mip.width * mip.height * mip.depth * pixelSize;
            
            if (mip.dataSize > 0 && mip.dataSize < expectedSize) {
                return ValidationResult::failure("MIP level " + std::to_string(mip.level) + 
                                               " data size too small");
            }
        }
        
        expectedWidth = std::max(1u, expectedWidth / 2);
        expectedHeight = std::max(1u, expectedHeight / 2);
        expectedDepth = std::max(1u, expectedDepth / 2);
    }
    
    return ValidationResult::success();
}

void Texture::calculateMipDimensions(uint32_t baseWidth, uint32_t baseHeight,
                                     uint32_t baseDepth, uint32_t level,
                                     uint32_t& width, uint32_t& height, uint32_t& depth) {
    width = std::max(1u, baseWidth >> level);
    height = std::max(1u, baseHeight >> level);
    depth = std::max(1u, baseDepth >> level);
}

uint32_t Texture::calculateMipLevels(uint32_t width, uint32_t height, uint32_t depth) {
    uint32_t maxDim = std::max({width, height, depth});
    uint32_t levels = 0;
    
    while (maxDim > 0) {
        ++levels;
        maxDim >>= 1;
    }
    
    return levels;
}

} // namespace phoenix::resource
