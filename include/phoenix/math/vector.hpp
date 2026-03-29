#pragma once

/**
 * @brief Unified vector header - includes all vector types
 */

#include "vector3.hpp"
#include "vector4.hpp"

namespace phoenix::math {

// Type aliases for convenience
using Vector2 = Vector3;  // Simplified - use Vector3 with z=0 for 2D
using Vec2 = Vector2;
using Vec3 = Vector3;
using Vec4 = Vector4;

} // namespace phoenix::math
