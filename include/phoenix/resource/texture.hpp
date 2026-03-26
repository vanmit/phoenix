#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <memory>

#include "../math/vector.hpp"
#include "types.hpp"

namespace phoenix::resource {

/**
 * @brief Texture format enumeration
 */
enum class TextureFormat : uint8_t {
    // Uncompressed formats
    R8 = 0,
    RG8,
    RGB8,
    RGBA8,
    R16F,
    RG16F,
    RGB16F,
    RGBA16F,
    R32F,
    RG32F,
    RGB32F,
    RGBA32F,
    
    // Compressed formats (BC/DXT)
    BC1_RGBA,    // DXT1
    BC2_RGBA,    // DXT3
    BC3_RGBA,    // DXT5
    BC4_R,       // RGTC1
    BC5_RG,      // RGTC2
    BC6H_UF16,   // BPTC float
    BC6H_SF16,   // BPTC float signed
    BC7_RGBA,    // BPTC
    
    // Compressed formats (ASTC)
    ASTC_4x4,
    ASTC_5x4,
    ASTC_5x5,
    ASTC_6x5,
    ASTC_6x6,
    ASTC_8x5,
    ASTC_8x6,
    ASTC_8x8,
    ASTC_10x5,
    ASTC_10x6,
    ASTC_10x8,
    ASTC_10x10,
    ASTC_12x10,
    ASTC_12x12,
    
    // Compressed formats (ETC2)
    ETC2_RGB8,
    ETC2_SRGB8,
    ETC2_RGB8_A1,
    ETC2_RGBA8,
    
    // Depth formats
    D16,
    D24,
    D32F,
    D24S8,
    D32FS8
};

/**
 * @brief Texture type
 */
enum class TextureType : uint8_t {
    Texture2D = 0,
    Texture3D,
    TextureCube,
    Texture2DArray,
    TextureCubeArray
};

/**
 * @brief Texture usage hints
 */
enum class TextureUsage : uint16_t {
    Sample = 1 << 0,
    RenderTarget = 1 << 1,
    DepthStencil = 1 << 2,
    Storage = 1 << 3,
    Dynamic = 1 << 4,
    SRGB = 1 << 5,
    MipMap = 1 << 6,
    AutoGenMips = 1 << 7
};

inline TextureUsage operator|(TextureUsage a, TextureUsage b) {
    return static_cast<TextureUsage>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
}

/**
 * @brief Texture creation info
 */
struct TextureCreateInfo {
    TextureType type = TextureType::Texture2D;
    TextureFormat format = TextureFormat::RGBA8;
    TextureUsage usage = TextureUsage::Sample | TextureUsage::MipMap;
    
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t depth = 1;
    uint32_t numLayers = 1;
    uint32_t numMips = 1;
    
    const void* initialData = nullptr;
    size_t initialDataSize = 0;
    
    std::string debugName;
};

/**
 * @brief MIP level descriptor
 */
struct MipLevel {
    uint32_t level = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t depth = 0;
    std::vector<uint8_t> data;
    size_t dataSize = 0;
};

/**
 * @brief Texture resource
 */
class Texture {
public:
    std::string name;
    std::string sourcePath;
    AssetType assetType = AssetType::Unknown;
    
    // Properties
    TextureCreateInfo info;
    std::vector<MipLevel> mipLevels;
    
    // KTX2/DDS specific
    bool isCompressed = false;
    uint32_t compressionFormat = 0;  // FourCC or similar
    
    // ASTC/BC block dimensions
    uint32_t blockWidth = 1;
    uint32_t blockHeight = 1;
    uint32_t bytesPerBlock = 0;
    
    // State
    LoadState loadState = LoadState::Pending;
    std::string loadError;
    MemoryBudget memoryUsage;
    
    // Native handle (set by renderer)
    void* nativeHandle = nullptr;
    
    Texture() = default;
    ~Texture() = default;
    
    // Move semantics
    Texture(Texture&&) noexcept = default;
    Texture& operator=(Texture&&) noexcept = default;
    
    // Copy semantics
    Texture(const Texture&) = default;
    Texture& operator=(const Texture&) = default;
    
    /**
     * @brief Calculate memory usage in bytes
     */
    size_t calculateMemoryUsage() const;
    
    /**
     * @brief Get format size in bytes per pixel
     */
    static uint8_t getFormatSize(TextureFormat format);
    
    /**
     * @brief Get block dimensions for compressed format
     */
    static void getBlockDimensions(TextureFormat format, uint32_t& width, uint32_t& height);
    
    /**
     * @brief Generate MIP chain from base level
     */
    void generateMipMaps();
    
    /**
     * @brief Validate texture data
     */
    ValidationResult validate() const;
    
    /**
     * @brief Calculate MIP level dimensions
     */
    static void calculateMipDimensions(uint32_t baseWidth, uint32_t baseHeight, 
                                       uint32_t baseDepth, uint32_t level,
                                       uint32_t& width, uint32_t& height, uint32_t& depth);
    
    /**
     * @brief Get number of MIP levels for given dimensions
     */
    static uint32_t calculateMipLevels(uint32_t width, uint32_t height, uint32_t depth = 1);
};

/**
 * @brief Texture sampler state
 */
struct SamplerState {
    enum class Filter : uint8_t {
        Point,
        Linear,
        Anisotropic
    };
    
    enum class AddressMode : uint8_t {
        Wrap,
        Mirror,
        Clamp,
        Border
    };
    
    Filter minFilter = Filter::Linear;
    Filter magFilter = Filter::Linear;
    Filter mipFilter = Filter::Linear;
    
    AddressMode addressU = AddressMode::Wrap;
    AddressMode addressV = AddressMode::Wrap;
    AddressMode addressW = AddressMode::Wrap;
    
    float mipLODBias = 0.0f;
    float maxAnisotropy = 1.0f;
    
    float minLOD = -FLT_MAX;
    float maxLOD = FLT_MAX;
    
    bool compareEnabled = false;
    enum class CompareFunc : uint8_t {
        Never, Less, Equal, LessEqual,
        Greater, NotEqual, GreaterEqual, Always
    } compareFunc = CompareFunc::Less;
    
    math::Color4 borderColor{0.0f, 0.0f, 0.0f, 1.0f};
};

} // namespace phoenix::resource
