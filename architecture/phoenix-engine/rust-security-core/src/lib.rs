//! Phoenix Engine Security Core
//! 
//! Rust-backed security validation library for Phoenix Engine.
//! Provides file validation, encryption, hashing, and secure random generation.

use std::ffi::{CStr, CString};
use std::os::raw::{c_char, c_void};
use std::path::Path;
use std::slice;

// ============================================================================
// Error Types
// ============================================================================

#[repr(u32)]
pub enum SecurityErrorCode {
    Success = 0,
    InvalidParameter = 1,
    ValidationError = 2,
    PathTraversal = 3,
    InvalidFormat = 4,
    EncryptionError = 5,
    DecryptionError = 6,
    HashError = 7,
    IoError = 8,
}

pub struct SecurityResult {
    pub code: SecurityErrorCode,
    pub message: *mut c_char,
}

impl SecurityResult {
    pub fn success() -> Self {
        SecurityResult {
            code: SecurityErrorCode::Success,
            message: std::ptr::null_mut(),
        }
    }

    pub fn error(code: SecurityErrorCode, msg: &str) -> Self {
        let c_str = CString::new(msg).unwrap_or_default();
        SecurityResult {
            code,
            message: c_str.into_raw(),
        }
    }
}

// ============================================================================
// Path Validation
// ============================================================================

/// Validates that a path does not contain directory traversal attacks
/// 
/// # Safety
/// `path` must be a valid null-terminated C string
#[no_mangle]
pub unsafe extern "C" fn phoenix_validate_path(path: *const c_char) -> SecurityResult {
    if path.is_null() {
        return SecurityResult::error(
            SecurityErrorCode::InvalidParameter,
            "Null path pointer"
        );
    }

    let path_str = match CStr::from_ptr(path).to_str() {
        Ok(s) => s,
        Err(_) => {
            return SecurityResult::error(
                SecurityErrorCode::InvalidParameter,
                "Invalid UTF-8 in path"
            );
        }
    };

    // Check for path traversal patterns
    let dangerous_patterns = [
        "..",
        "//",
        "\\",
        "~",
        "$",
        "`",
        "|",
        ";",
        "&",
    ];

    for pattern in &dangerous_patterns {
        if path_str.contains(pattern) {
            return SecurityResult::error(
                SecurityErrorCode::PathTraversal,
                &format!("Dangerous pattern '{}' in path", pattern)
            );
        }
    }

    // Normalize and validate
    let normalized = normpath::normalize(path_str);
    if normalized.is_err() {
        return SecurityResult::error(
            SecurityErrorCode::ValidationError,
            "Failed to normalize path"
        );
    }

    SecurityResult::success()
}

// ============================================================================
// File Content Validation
// ============================================================================

/// Validates file content based on expected format
/// 
/// # Safety
/// `data` must point to valid memory of `size` bytes
#[no_mangle]
pub unsafe extern "C" fn phoenix_validate_file_content(
    data: *const u8,
    size: usize,
    format: *const c_char,
) -> SecurityResult {
    if data.is_null() || size == 0 {
        return SecurityResult::error(
            SecurityErrorCode::InvalidParameter,
            "Invalid data pointer or size"
        );
    }

    let format_str = if format.is_null() {
        "binary"
    } else {
        CStr::from_ptr(format).to_str().unwrap_or("binary")
    };

    let content = slice::from_raw_parts(data, size);

    // Basic size validation (prevent DoS)
    const MAX_FILE_SIZE: usize = 100 * 1024 * 1024; // 100 MB
    if size > MAX_FILE_SIZE {
        return SecurityResult::error(
            SecurityErrorCode::ValidationError,
            &format!("File size {} exceeds maximum {}", size, MAX_FILE_SIZE)
        );
    }

    // Format-specific validation
    match format_str {
        "png" => validate_png(content),
        "jpg" | "jpeg" => validate_jpeg(content),
        "glb" | "gltf" => validate_gltf(content),
        "shader" => validate_shader(content),
        _ => SecurityResult::success(), // Unknown format, basic checks only
    }
}

fn validate_png(data: &[u8]) -> SecurityResult {
    // PNG signature: 89 50 4E 47 0D 0A 1A 0A
    const PNG_SIGNATURE: [u8; 8] = [0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A];
    
    if data.len() < 8 {
        return SecurityResult::error(
            SecurityErrorCode::InvalidFormat,
            "PNG file too small"
        );
    }

    if data[0..8] != PNG_SIGNATURE {
        return SecurityResult::error(
            SecurityErrorCode::InvalidFormat,
            "Invalid PNG signature"
        );
    }

    SecurityResult::success()
}

fn validate_jpeg(data: &[u8]) -> SecurityResult {
    // JPEG signature: FF D8 FF
    if data.len() < 3 {
        return SecurityResult::error(
            SecurityErrorCode::InvalidFormat,
            "JPEG file too small"
        );
    }

    if data[0] != 0xFF || data[1] != 0xD8 || data[2] != 0xFF {
        return SecurityResult::error(
            SecurityErrorCode::InvalidFormat,
            "Invalid JPEG signature"
        );
    }

    SecurityResult::success()
}

fn validate_gltf(data: &[u8]) -> SecurityResult {
    // GLB signature: "glTF"
    if data.len() < 12 {
        return SecurityResult::error(
            SecurityErrorCode::InvalidFormat,
            "GLB file too small"
        );
    }

    if &data[0..4] != b"glTF" {
        return SecurityResult::error(
            SecurityErrorCode::InvalidFormat,
            "Invalid GLB signature"
        );
    }

    SecurityResult::success()
}

fn validate_shader(data: &[u8]) -> SecurityResult {
    // Basic shader validation - check for reasonable content
    if data.is_empty() {
        return SecurityResult::error(
            SecurityErrorCode::InvalidFormat,
            "Empty shader file"
        );
    }

    // Check for valid UTF-8
    if std::str::from_utf8(data).is_err() {
        return SecurityResult::error(
            SecurityErrorCode::InvalidFormat,
            "Shader must be valid UTF-8"
        );
    }

    SecurityResult::success()
}

// ============================================================================
// Hashing
// ============================================================================

/// Computes SHA256 hash of data
/// 
/// # Safety
/// `data` must point to valid memory of `size` bytes
/// `out` must point to at least 32 bytes of writable memory
#[no_mangle]
pub unsafe extern "C" fn phoenix_hash_sha256(
    data: *const u8,
    size: usize,
    out: *mut u8,
) -> SecurityResult {
    use sha2::{Sha256, Digest};

    if data.is_null() || size == 0 || out.is_null() {
        return SecurityResult::error(
            SecurityErrorCode::InvalidParameter,
            "Invalid parameters"
        );
    }

    let content = slice::from_raw_parts(data, size);
    let mut hasher = Sha256::new();
    hasher.update(content);
    let result = hasher.finalize();

    slice::from_raw_parts_mut(out, 32).copy_from_slice(&result);

    SecurityResult::success()
}

/// Computes BLAKE3 hash of data
/// 
/// # Safety
/// `data` must point to valid memory of `size` bytes
/// `out` must point to at least 32 bytes of writable memory
#[no_mangle]
pub unsafe extern "C" fn phoenix_hash_blake3(
    data: *const u8,
    size: usize,
    out: *mut u8,
) -> SecurityResult {
    if data.is_null() || size == 0 || out.is_null() {
        return SecurityResult::error(
            SecurityErrorCode::InvalidParameter,
            "Invalid parameters"
        );
    }

    let content = slice::from_raw_parts(data, size);
    let result = blake3::hash(content);

    slice::from_raw_parts_mut(out, 32).copy_from_slice(result.as_bytes());

    SecurityResult::success()
}

// ============================================================================
// Secure Random
// ============================================================================

/// Generates cryptographically secure random bytes
/// 
/// # Safety
/// `out` must point to at least `size` bytes of writable memory
#[no_mangle]
pub unsafe extern "C" fn phoenix_random_bytes(
    out: *mut u8,
    size: usize,
) -> SecurityResult {
    use rand::RngCore;

    if out.is_null() || size == 0 {
        return SecurityResult::error(
            SecurityErrorCode::InvalidParameter,
            "Invalid parameters"
        );
    }

    let buffer = slice::from_raw_parts_mut(out, size);
    rand::thread_rng().fill_bytes(buffer);

    SecurityResult::success()
}

// ============================================================================
// Memory Management
// ============================================================================

/// Frees a string returned by the security library
/// 
/// # Safety
/// `ptr` must be a valid pointer returned by this library
#[no_mangle]
pub unsafe extern "C" fn phoenix_free_string(ptr: *mut c_char) {
    if !ptr.is_null() {
        drop(CString::from_raw(ptr));
    }
}

// ============================================================================
// Initialization
// ============================================================================

/// Initializes the security library
#[no_mangle]
pub extern "C" fn phoenix_security_init() -> SecurityResult {
    // Initialize logging, etc.
    SecurityResult::success()
}

/// Shuts down the security library
#[no_mangle]
pub extern "C" fn phoenix_security_shutdown() {
    // Cleanup
}

// ============================================================================
// Tests
// ============================================================================

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_validate_safe_path() {
        unsafe {
            let path = CString::new("assets/textures/texture.png").unwrap();
            let result = phoenix_validate_path(path.as_ptr());
            assert_eq!(result.code, SecurityErrorCode::Success);
            phoenix_free_string(result.message);
        }
    }

    #[test]
    fn test_validate_path_traversal() {
        unsafe {
            let path = CString::new("../../../etc/passwd").unwrap();
            let result = phoenix_validate_path(path.as_ptr());
            assert_eq!(result.code, SecurityErrorCode::PathTraversal);
            phoenix_free_string(result.message);
        }
    }

    #[test]
    fn test_validate_png() {
        let png_data = vec![
            0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A,
            0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
        ];
        unsafe {
            let result = phoenix_validate_file_content(
                png_data.as_ptr(),
                png_data.len(),
                CString::new("png").unwrap().as_ptr(),
            );
            assert_eq!(result.code, SecurityErrorCode::Success);
            phoenix_free_string(result.message);
        }
    }
}
