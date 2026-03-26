#include "../../include/phoenix/resource/terrain.hpp"
#include <algorithm>
#include <cmath>

namespace phoenix::resource {

// ============ Terrain ============

size_t Terrain::calculateMemoryUsage() const {
    size_t total = 0;
    
    total += heightData.size() * sizeof(float);
    total += normalData.size() * sizeof(math::Vector3);
    total += splatData.size() * sizeof(math::Color4);
    
    for (const auto& chunkRow : chunks) {
        for (const auto& chunk : chunkRow) {
            total += chunk.heights.size() * sizeof(float);
        }
    }
    
    for (const auto& veg : vegetation) {
        total += veg.instances.size() * sizeof(VegetationInstance);
    }
    
    return total;
}

math::Vector3 Terrain::calculateNormal(uint32_t x, uint32_t y) const {
    if (x >= width || y >= depth) {
        return math::Vector3(0.0f, 1.0f, 0.0f);
    }
    
    float hL = (x > 0) ? getHeight(x - 1, y) : getHeight(x, y);
    float hR = (x < width - 1) ? getHeight(x + 1, y) : getHeight(x, y);
    float hD = (y > 0) ? getHeight(x, y - 1) : getHeight(x, y);
    float hU = (y < depth - 1) ? getHeight(x, y + 1) : getHeight(x, y);
    
    math::Vector3 normal;
    normal.x = hL - hR;
    normal.y = 2.0f;  // Vertical scale factor
    normal.z = hD - hU;
    
    return math::normalize(normal);
}

float Terrain::getHeight(uint32_t x, uint32_t y) const {
    if (x >= width || y >= depth || heightData.empty()) {
        return 0.0f;
    }
    
    float h = heightData[y * width + x];
    return h * verticalScale + verticalOffset;
}

float Terrain::getHeightAtWorldPosition(const math::Vector3& worldPos) const {
    math::Vector2 uv = worldToUV(worldPos);
    
    if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f) {
        return 0.0f;
    }
    
    // Bilinear interpolation
    float fx = uv.x * (width - 1);
    float fy = uv.y * (depth - 1);
    
    uint32_t x0 = static_cast<uint32_t>(std::floor(fx));
    uint32_t y0 = static_cast<uint32_t>(std::floor(fy));
    uint32_t x1 = std::min(x0 + 1, width - 1);
    uint32_t y1 = std::min(y0 + 1, depth - 1);
    
    float tx = fx - x0;
    float ty = fy - y0;
    
    float h00 = getHeight(x0, y0);
    float h10 = getHeight(x1, y0);
    float h01 = getHeight(x0, y1);
    float h11 = getHeight(x1, y1);
    
    float h0 = math::lerp(h00, h10, tx);
    float h1 = math::lerp(h01, h11, tx);
    
    return math::lerp(h0, h1, ty);
}

math::Color4 Terrain::sampleSplat(uint32_t x, uint32_t y) const {
    if (x >= width || y >= depth || splatData.empty()) {
        return math::Color4(1.0f, 0.0f, 0.0f, 0.0f);  // Default to first layer
    }
    
    return splatData[y * width + x];
}

void Terrain::buildChunks(uint32_t chunkSize) {
    if (width == 0 || depth == 0) return;
    
    this->chunkSize = chunkSize;
    chunksPerSide = (width + chunkSize - 1) / chunkSize;
    
    chunks.resize(chunksPerSide);
    for (auto& row : chunks) {
        row.resize(chunksPerSide);
    }
    
    for (uint32_t cy = 0; cy < chunksPerSide; ++cy) {
        for (uint32_t cx = 0; cx < chunksPerSide; ++cx) {
            TerrainChunk& chunk = chunks[cy][cx];
            chunk.chunkX = cx;
            chunk.chunkY = cy;
            
            uint32_t startX = cx * chunkSize;
            uint32_t startY = cy * chunkSize;
            uint32_t endX = std::min(startX + chunkSize, width);
            uint32_t endY = std::min(startY + chunkSize, depth);
            
            chunk.resolution = endX - startX;
            chunk.heights.resize(chunk.resolution * chunk.resolution);
            
            for (uint32_t y = 0; y < chunk.resolution; ++y) {
                for (uint32_t x = 0; x < chunk.resolution; ++x) {
                    chunk.heights[y * chunk.resolution + x] = 
                        getHeight(startX + x, startY + y);
                }
            }
            
            // Setup neighbor references
            if (cy > 0) chunk.neighbors[0] = &chunks[cy - 1][cx];  // North
            if (cx < chunksPerSide - 1) chunk.neighbors[1] = &chunks[cy][cx + 1];  // East
            if (cy < chunksPerSide - 1) chunk.neighbors[2] = &chunks[cy + 1][cx];  // South
            if (cx > 0) chunk.neighbors[3] = &chunks[cy][cx - 1];  // West
        }
    }
    
    isStreamed = true;
}

void Terrain::updateLOD(const math::Vector3& cameraPos, float maxError) {
    if (!isStreamed) return;
    
    for (auto& row : chunks) {
        for (auto& chunk : row) {
            // Calculate distance to chunk center
            math::Vector3 chunkPos = uvToWorld(
                math::Vector2(
                    (chunk.chunkX + 0.5f) / chunksPerSide,
                    (chunk.chunkY + 0.5f) / chunksPerSide
                ),
                0.0f
            );
            
            float distance = math::length(cameraPos - chunkPos);
            
            // Calculate screen space error
            // Simplified: error increases with distance
            chunk.screenSpaceError = distance * 0.01f;
            
            // Determine LOD level
            if (chunk.screenSpaceError > maxError * 8.0f) {
                chunk.currentLOD = std::min(chunk.maxLOD, 4u);
            } else if (chunk.screenSpaceError > maxError * 4.0f) {
                chunk.currentLOD = std::min(chunk.maxLOD, 3u);
            } else if (chunk.screenSpaceError > maxError * 2.0f) {
                chunk.currentLOD = std::min(chunk.maxLOD, 2u);
            } else if (chunk.screenSpaceError > maxError) {
                chunk.currentLOD = std::min(chunk.maxLOD, 1u);
            } else {
                chunk.currentLOD = 0;
            }
            
            chunk.needsUpdate = true;
        }
    }
}

void Terrain::generateNormals() {
    if (width == 0 || depth == 0) return;
    
    normalData.resize(width * depth);
    
    for (uint32_t y = 0; y < depth; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            normalData[y * width + x] = calculateNormal(x, y);
        }
    }
}

ValidationResult Terrain::validate() const {
    if (width == 0 || depth == 0) {
        return ValidationResult::failure("Terrain has zero dimensions");
    }
    
    size_t expectedSize = width * depth;
    
    if (!heightData.empty() && heightData.size() != expectedSize) {
        return ValidationResult::failure("Height data size mismatch");
    }
    
    if (!normalData.empty() && normalData.size() != expectedSize) {
        return ValidationResult::failure("Normal data size mismatch");
    }
    
    if (!splatData.empty() && splatData.size() != expectedSize) {
        return ValidationResult::failure("Splat data size mismatch");
    }
    
    // Check for NaN/Inf in height data
    for (size_t i = 0; i < std::min(heightData.size(), size_t(1000)); ++i) {
        if (std::isnan(heightData[i]) || std::isinf(heightData[i])) {
            return ValidationResult::failure("Height data contains NaN/Inf at index " + 
                                           std::to_string(i));
        }
    }
    
    // Validate splat weights (should sum to ~1.0)
    for (size_t i = 0; i < std::min(splatData.size(), size_t(100)); ++i) {
        float sum = splatData[i].r + splatData[i].g + splatData[i].b + splatData[i].a;
        if (sum < 0.9f || sum > 1.1f) {
            // Warning only - sometimes splatmaps don't sum to exactly 1
        }
    }
    
    return ValidationResult::success();
}

BoundingVolume Terrain::getWorldBounds() const {
    BoundingVolume bounds;
    
    if (heightData.empty()) {
        bounds.center = math::Vector3(0.0f);
        bounds.radius = 0.0f;
        bounds.min = math::Vector3(0.0f);
        bounds.max = math::Vector3(0.0f);
        return bounds;
    }
    
    float minH = FLT_MAX;
    float maxH = -FLT_MAX;
    
    for (float h : heightData) {
        float worldH = h * verticalScale + verticalOffset;
        minH = std::min(minH, worldH);
        maxH = std::max(maxH, worldH);
    }
    
    bounds.min = math::Vector3(0.0f, minH, 0.0f);
    bounds.max = math::Vector3(static_cast<float>(width), maxH, static_cast<float>(depth));
    bounds.center = (bounds.min + bounds.max) * 0.5f;
    bounds.radius = math::length(bounds.max - bounds.center);
    
    return bounds;
}

math::Vector2 Terrain::worldToUV(const math::Vector3& worldPos) const {
    math::Vector2 uv;
    
    if (width > 1) {
        uv.x = worldPos.x / static_cast<float>(width - 1);
    } else {
        uv.x = 0.0f;
    }
    
    if (depth > 1) {
        uv.y = worldPos.z / static_cast<float>(depth - 1);
    } else {
        uv.y = 0.0f;
    }
    
    return uv;
}

math::Vector3 Terrain::uvToWorld(const math::Vector2& uv, float height) const {
    return math::Vector3(
        uv.x * static_cast<float>(width - 1),
        height * verticalScale + verticalOffset,
        uv.y * static_cast<float>(depth - 1)
    );
}

} // namespace phoenix::resource
