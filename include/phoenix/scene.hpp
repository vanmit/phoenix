#pragma once

/**
 * @file phoenix/scene.hpp
 * @brief Phoenix Engine Scene Management System
 * 
 * Complete scene management including:
 * - Scene graph with hierarchical transforms
 * - Spatial acceleration structures (Octree, BVH)
 * - Culling systems (Frustum, Distance, Backface)
 * - Entity Component System (ECS)
 * - Level of Detail (LOD) management
 * - glTF-compatible serialization
 */

// Core scene types
#include "scene/scene_node.hpp"
#include "scene/scene.hpp"
#include "scene/transform.hpp"

// Spatial acceleration
#include "scene/octree.hpp"
#include "scene/bvh.hpp"

// Culling
#include "math/frustum.hpp"
#include "math/bounding.hpp"

// ECS
#include "scene/ecs.hpp"

// LOD
#include "scene/lod.hpp"

// Serialization
#include "scene/scene_serializer.hpp"

/**
 * @namespace phoenix::scene
 * @brief Scene management components
 * 
 * The scene module provides comprehensive 3D scene management:
 * 
 * ## Scene Graph
 * - Hierarchical node structure with parent-child relationships
 * - Transform propagation with dirty flag optimization
 * - Visitor pattern for flexible traversal
 * 
 * ## Spatial Acceleration
 * - Octree for efficient spatial queries and culling
 * - BVH (Bounding Volume Hierarchy) for ray tracing
 * - SAH (Surface Area Heuristic) for optimal tree construction
 * 
 * ## Culling Systems
 * - Frustum culling for view volume testing
 * - Distance culling for LOD threshold
 * - Backface culling for camera-facing tests
 * - Occlusion culling preparation (Hi-Z)
 * 
 * ## Entity Component System
 * - Handle-based entity management with generation counters
 * - SoA (Structure of Arrays) component storage
 * - Type-safe component access
 * - System scheduling and event bus
 * 
 * ## Level of Detail
 * - Distance-based LOD selection
 * - Screen-space error estimation
 * - Morph transitions between levels
 * - HLOD (Hierarchical LOD) support
 * 
 * ## Performance Features
 * - Cache-friendly data layouts
 * - Multi-threading support
 * - Memory pool optimization
 * - Incremental updates
 */

namespace phoenix {
namespace scene {

/**
 * @brief Library version
 */
constexpr int VERSION_MAJOR = 0;
constexpr int VERSION_MINOR = 1;
constexpr int VERSION_PATCH = 0;

/**
 * @brief Initialize scene system
 * 
 * Call once at application startup.
 */
inline void initialize() {
    // Pre-allocate any global resources if needed
}

/**
 * @brief Shutdown scene system
 * 
 * Call once at application exit.
 */
inline void shutdown() {
    // Clean up global resources
}

} // namespace scene
} // namespace phoenix
