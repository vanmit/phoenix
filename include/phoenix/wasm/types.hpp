/**
 * Phoenix Engine - WASM Types Header
 * 
 * Type definitions and error codes for WASM bindings
 */

#pragma once

#include <cstdint>
#include <cstddef>

namespace phoenix::wasm {

/**
 * @brief WASM initialization configuration
 */
struct WasmInitConfig {
    uint32_t width = 800;
    uint32_t height = 600;
    bool enableValidation = false;
    bool enableDebugOutput = false;
    const char* title = "Phoenix Engine";
};

/**
 * @brief WASM error codes
 */
enum class WasmErrorCode : int32_t {
    Success = 0,
    AlreadyInitialized = -1,
    InvalidConfigPointer = -2,
    ResolutionTooLarge = -3,
    InitializationFailed = -4,
    InvalidParameter = -5,
    OutOfMemory = -6,
    UnsupportedFeature = -7,
    InternalError = -8
};

/**
 * @brief Convert error code to string
 * @param code Error code
 * @return String description
 */
inline const char* wasmErrorToString(WasmErrorCode code) {
    switch (code) {
        case WasmErrorCode::Success: return "Success";
        case WasmErrorCode::AlreadyInitialized: return "Already initialized";
        case WasmErrorCode::InvalidConfigPointer: return "Invalid config pointer";
        case WasmErrorCode::ResolutionTooLarge: return "Resolution too large";
        case WasmErrorCode::InitializationFailed: return "Initialization failed";
        case WasmErrorCode::InvalidParameter: return "Invalid parameter";
        case WasmErrorCode::OutOfMemory: return "Out of memory";
        case WasmErrorCode::UnsupportedFeature: return "Unsupported feature";
        case WasmErrorCode::InternalError: return "Internal error";
        default: return "Unknown error";
    }
}

/**
 * @brief Maximum allowed resolution
 */
constexpr uint32_t MAX_RESOLUTION = 16384;

/**
 * @brief Default canvas width
 */
constexpr uint32_t DEFAULT_WIDTH = 800;

/**
 * @brief Default canvas height
 */
constexpr uint32_t DEFAULT_HEIGHT = 600;

} // namespace phoenix::wasm
