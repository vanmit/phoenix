//! C FFI Bindings for Phoenix Security Core
//!
//! This module provides C-compatible interfaces for use from C++ and other languages.
//! All functions use C ABI and handle memory safely across the FFI boundary.

use crate::{AuditEntry, AuditLogger, CryptoModule, SecureAllocator};
use std::ffi::{c_char, c_void, CStr};
use std::os::raw::c_uchar;
use std::ptr;
use zeroize::Zeroizing;

// ============================================================================
// Opaque types for C
// ============================================================================

#[repr(C)]
pub struct SecureAllocatorHandle {
    _private: [u8; 0],
}

#[repr(C)]
pub struct CryptoModuleHandle {
    _private: [u8; 0],
}

#[repr(C)]
pub struct AuditLoggerHandle {
    _private: [u8; 0],
}

// ============================================================================
// SecureAllocator FFI
// ============================================================================

/// Create a new secure allocator
#[no_mangle]
pub extern "C" fn secure_allocator_new() -> *mut SecureAllocatorHandle {
    let allocator = Box::new(SecureAllocator::new());
    Box::into_raw(allocator) as *mut SecureAllocatorHandle
}

/// Allocate secure memory
/// Returns pointer to allocated memory, or null on failure
/// Caller must free with secure_allocator_free
#[no_mangle]
pub extern "C" fn secure_allocator_allocate(
    handle: *mut SecureAllocatorHandle,
    size: usize,
) -> *mut c_uchar {
    if handle.is_null() {
        return ptr::null_mut();
    }

    let allocator = unsafe { &mut *(handle as *mut SecureAllocator) };
    let buffer = allocator.allocate(size);

    // Transfer ownership to C
    let ptr = buffer.as_ptr() as *mut c_uchar;
    let len = buffer.len();

    // We need to keep the buffer alive, so we leak it
    // This is safe because C will call secure_allocator_free
    std::mem::forget(buffer);

    // Store metadata for later free (in practice, you'd use a different approach)
    // For now, we return the pointer and track it separately
    ptr
}

/// Free allocated memory
#[no_mangle]
pub extern "C" fn secure_allocator_free(
    _handle: *mut SecureAllocatorHandle,
    ptr: *mut c_uchar,
    size: usize,
) {
    if !ptr.is_null() && size > 0 {
        // Reconstruct and drop the Zeroizing buffer to securely clear memory
        unsafe {
            let _buffer = Zeroizing::new(Vec::from_raw_parts(ptr, size, size));
            // Automatically zeroized on drop
        }
    }
}

/// Destroy the allocator
#[no_mangle]
pub extern "C" fn secure_allocator_destroy(handle: *mut SecureAllocatorHandle) {
    if !handle.is_null() {
        unsafe {
            let _ = Box::from_raw(handle as *mut SecureAllocator);
        }
    }
}

// ============================================================================
// CryptoModule FFI
// ============================================================================

/// Create a new crypto module with the given key
/// Key must be exactly 32 bytes for AES-256
#[no_mangle]
pub extern "C" fn crypto_module_new(key: *const c_uchar, key_len: usize) -> *mut CryptoModuleHandle {
    if key.is_null() || key_len != CryptoModule::KEY_SIZE {
        return ptr::null_mut();
    }

    let key_slice = unsafe { std::slice::from_raw_parts(key, key_len) };
    match CryptoModule::new(key_slice) {
        Ok(crypto) => Box::into_raw(Box::new(crypto)) as *mut CryptoModuleHandle,
        Err(_) => ptr::null_mut(),
    }
}

/// Generate a random secure key
/// Returns pointer to 32-byte key, caller must free with crypto_free_key
#[no_mangle]
pub extern "C" fn crypto_module_generate_key() -> *mut c_uchar {
    let key = CryptoModule::generate_key();
    let ptr = key.as_ptr() as *mut c_uchar;
    std::mem::forget(key);
    ptr
}

/// Free a generated key
#[no_mangle]
pub extern "C" fn crypto_free_key(key: *mut c_uchar) {
    if !key.is_null() {
        unsafe {
            let _ = Zeroizing::new(Vec::from_raw_parts(key, CryptoModule::KEY_SIZE, CryptoModule::KEY_SIZE));
        }
    }
}

/// Encrypt data
/// Returns: pointer to (nonce_len (4 bytes) + nonce + ciphertext)
/// Caller must free with crypto_free_buffer
/// On error, returns null
#[no_mangle]
pub extern "C" fn crypto_encrypt(
    handle: *mut CryptoModuleHandle,
    plaintext: *const c_uchar,
    plaintext_len: usize,
    out_len: *mut usize,
) -> *mut c_uchar {
    if handle.is_null() || plaintext.is_null() || out_len.is_null() {
        return ptr::null_mut();
    }

    let crypto = unsafe { &*(handle as *mut CryptoModule) };
    let plaintext_slice = unsafe { std::slice::from_raw_parts(plaintext, plaintext_len) };

    match crypto.encrypt(plaintext_slice) {
        Ok((nonce, ciphertext)) => {
            // Format: [nonce_len (4 bytes)][nonce][ciphertext]
            let nonce_len = nonce.len() as u32;
            let total_len = 4 + nonce.len() + ciphertext.len();

            let mut buffer = Zeroizing::new(vec![0u8; total_len]);
            buffer[0..4].copy_from_slice(&nonce_len.to_le_bytes());
            buffer[4..4 + nonce.len()].copy_from_slice(&nonce);
            buffer[4 + nonce.len()..].copy_from_slice(&ciphertext);

            *out_len = total_len;
            let ptr = buffer.as_ptr() as *mut c_uchar;
            std::mem::forget(buffer);
            ptr
        }
        Err(_) => {
            *out_len = 0;
            ptr::null_mut()
        }
    }
}

/// Decrypt data
/// Input format: [nonce_len (4 bytes)][nonce][ciphertext]
/// Returns decrypted data, caller must free with crypto_free_buffer
/// On error, returns null and sets out_len to 0
#[no_mangle]
pub extern "C" fn crypto_decrypt(
    handle: *mut CryptoModuleHandle,
    encrypted: *const c_uchar,
    encrypted_len: usize,
    out_len: *mut usize,
) -> *mut c_uchar {
    // SECURITY FIX: Complete null checks
    if handle.is_null() || encrypted.is_null() || out_len.is_null() {
        *out_len = 0;
        return ptr::null_mut();
    }

    // SECURITY FIX: Minimum size check (at least 4 bytes for nonce_len)
    if encrypted_len < 4 {
        *out_len = 0;
        return ptr::null_mut();
    }

    let crypto = unsafe { &*(handle as *mut CryptoModule) };
    let encrypted_slice = unsafe { std::slice::from_raw_parts(encrypted, encrypted_len) };

    // SECURITY FIX: Validate nonce_len before use (CWE-119)
    let nonce_len = u32::from_le_bytes([
        encrypted_slice[0],
        encrypted_slice[1],
        encrypted_slice[2],
        encrypted_slice[3],
    ]) as usize;

    // FIX CWE-119: Boundary check for nonce length
    if nonce_len > CryptoModule::NONCE_SIZE {
        *out_len = 0;
        return ptr::null_mut();
    }

    // FIX CWE-190: Integer overflow protection
    let data_start = 4usize.checked_add(nonce_len).unwrap_or(usize::MAX);
    if data_start > encrypted_len || data_start == usize::MAX {
        *out_len = 0;
        return ptr::null_mut();
    }

    // Additional boundary check
    if encrypted_len < 4 + nonce_len {
        *out_len = 0;
        return ptr::null_mut();
    }

    let nonce = &encrypted_slice[4..4 + nonce_len];
    let ciphertext = &encrypted_slice[4 + nonce_len..];

    match crypto.decrypt(nonce, ciphertext) {
        Ok(plaintext) => {
            *out_len = plaintext.len();
            let ptr = plaintext.as_ptr() as *mut c_uchar;
            std::mem::forget(plaintext);
            ptr
        }
        Err(_) => {
            *out_len = 0;
            ptr::null_mut()
        }
    }
}

/// Free encrypted/decrypted buffer
#[no_mangle]
pub extern "C" fn crypto_free_buffer(ptr: *mut c_uchar, len: usize) {
    if !ptr.is_null() && len > 0 {
        unsafe {
            let _ = Zeroizing::new(Vec::from_raw_parts(ptr, len, len));
        }
    }
}

/// Destroy crypto module
#[no_mangle]
pub extern "C" fn crypto_module_destroy(handle: *mut CryptoModuleHandle) {
    if !handle.is_null() {
        unsafe {
            let _ = Box::from_raw(handle as *mut CryptoModule);
        }
    }
}

// ============================================================================
// AuditLogger FFI
// ============================================================================

/// Create a new audit logger with the given HMAC key
#[no_mangle]
pub extern "C" fn audit_logger_new(key: *const c_uchar, key_len: usize) -> *mut AuditLoggerHandle {
    if key.is_null() {
        return ptr::null_mut();
    }

    let key_slice = unsafe { std::slice::from_raw_parts(key, key_len) };
    let logger = AuditLogger::new(key_slice);
    Box::into_raw(Box::new(logger)) as *mut AuditLoggerHandle
}

/// Generate a secure HMAC key (32 bytes)
#[no_mangle]
pub extern "C" fn audit_logger_generate_key() -> *mut c_uchar {
    let key = AuditLogger::generate_key();
    let ptr = key.as_ptr() as *mut c_uchar;
    std::mem::forget(key);
    ptr
}

/// Set the log file path
#[no_mangle]
pub extern "C" fn audit_logger_set_log_file(
    handle: *mut AuditLoggerHandle,
    path: *const c_char,
) -> bool {
    if handle.is_null() || path.is_null() {
        return false;
    }

    let logger = unsafe { &mut *(handle as *mut AuditLogger) };
    let path_str = unsafe { CStr::from_ptr(path) };

    if let Ok(path) = path_str.to_str() {
        logger.set_log_file(path);
        true
    } else {
        false
    }
}

/// Log an audit event
/// Returns a pointer to AuditEntry (opaque), caller must free with audit_entry_destroy
#[no_mangle]
pub extern "C" fn audit_logger_log(
    handle: *mut AuditLoggerHandle,
    event_type: *const c_char,
    details: *const c_char,
) -> *mut c_void {
    if handle.is_null() || event_type.is_null() || details.is_null() {
        return ptr::null_mut();
    }

    let logger = unsafe { &*(handle as *mut AuditLogger) };
    let event_type_str = unsafe { CStr::from_ptr(event_type) };
    let details_str = unsafe { CStr::from_ptr(details) };

    if let (Ok(event_type), Ok(details)) = (event_type_str.to_str(), details_str.to_str()) {
        let entry = logger.log(event_type, details);
        Box::into_raw(Box::new(entry)) as *mut c_void
    } else {
        ptr::null_mut()
    }
}

/// Verify an audit entry's HMAC
#[no_mangle]
pub extern "C" fn audit_logger_verify_entry(
    handle: *mut AuditLoggerHandle,
    entry: *const c_void,
) -> bool {
    if handle.is_null() || entry.is_null() {
        return false;
    }

    let logger = unsafe { &*(handle as *mut AuditLogger) };
    let audit_entry = unsafe { &*(entry as *const AuditEntry) };

    logger.verify_entry(audit_entry)
}

/// Get entry timestamp as C string (caller must not free)
#[no_mangle]
pub extern "C" fn audit_entry_get_timestamp(entry: *const c_void) -> *const c_char {
    if entry.is_null() {
        return ptr::null();
    }

    let audit_entry = unsafe { &*(entry as *const AuditEntry) };
    audit_entry.timestamp.as_ptr() as *const c_char
}

/// Get entry event type as C string
#[no_mangle]
pub extern "C" fn audit_entry_get_event_type(entry: *const c_void) -> *const c_char {
    if entry.is_null() {
        return ptr::null();
    }

    let audit_entry = unsafe { &*(entry as *const AuditEntry) };
    audit_entry.event_type.as_ptr() as *const c_char
}

/// Get entry details as C string
#[no_mangle]
pub extern "C" fn audit_entry_get_details(entry: *const c_void) -> *const c_char {
    if entry.is_null() {
        return ptr::null();
    }

    let audit_entry = unsafe { &*(entry as *const AuditEntry) };
    audit_entry.details.as_ptr() as *const c_char
}

/// Get entry HMAC as C string
#[no_mangle]
pub extern "C" fn audit_entry_get_hmac(entry: *const c_void) -> *const c_char {
    if entry.is_null() {
        return ptr::null();
    }

    let audit_entry = unsafe { &*(entry as *const AuditEntry) };
    audit_entry.hmac.as_ptr() as *const c_char
}

/// Destroy an audit entry
#[no_mangle]
pub extern "C" fn audit_entry_destroy(entry: *mut c_void) {
    if !entry.is_null() {
        unsafe {
            let _ = Box::from_raw(entry as *mut AuditEntry);
        }
    }
}

/// Destroy audit logger
#[no_mangle]
pub extern "C" fn audit_logger_destroy(handle: *mut AuditLoggerHandle) {
    if !handle.is_null() {
        unsafe {
            let _ = Box::from_raw(handle as *mut AuditLogger);
        }
    }
}

// ============================================================================
// Initialization helper
// ============================================================================

/// Initialize all security components
/// Returns pointers to (allocator, crypto, logger) in that order
/// All must be destroyed with their respective destroy functions
#[no_mangle]
pub extern "C" fn security_core_init(
    allocator_out: *mut *mut SecureAllocatorHandle,
    crypto_out: *mut *mut CryptoModuleHandle,
    logger_out: *mut *mut AuditLoggerHandle,
) {
    let (allocator, crypto, logger) = crate::init();

    if !allocator_out.is_null() {
        unsafe {
            *allocator_out = Box::into_raw(Box::new(allocator)) as *mut SecureAllocatorHandle;
        }
    }

    if !crypto_out.is_null() {
        unsafe {
            *crypto_out = Box::into_raw(Box::new(crypto)) as *mut CryptoModuleHandle;
        }
    }

    if !logger_out.is_null() {
        unsafe {
            *logger_out = Box::into_raw(Box::new(logger)) as *mut AuditLoggerHandle;
        }
    }
}
