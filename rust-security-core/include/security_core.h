/**
 * Phoenix Security Core - C/C++ Header
 *
 * This header provides C-compatible interfaces to the Phoenix Engine
 * security core library, written in Rust for memory safety.
 *
 * Features:
 * - Secure memory allocation with automatic zeroization
 * - AES-256-GCM encryption/decryption
 * - Audit logging with HMAC integrity verification
 *
 * @copyright Phoenix Engine Team
 * @license MIT
 */

#ifndef PHOENIX_SECURITY_CORE_H
#define PHOENIX_SECURITY_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Opaque Handle Types
// ============================================================================

typedef struct SecureAllocatorHandle SecureAllocatorHandle;
typedef struct CryptoModuleHandle CryptoModuleHandle;
typedef struct AuditLoggerHandle AuditLoggerHandle;

// ============================================================================
// Constants
// ============================================================================

#define PHOENIX_AES_KEY_SIZE 32      // AES-256 key size in bytes
#define PHOENIX_AES_NONCE_SIZE 12    // GCM nonce size in bytes
#define PHOENIX_HMAC_KEY_SIZE 32     // HMAC-SHA256 key size in bytes

// ============================================================================
// SecureAllocator API
// ============================================================================

/**
 * Create a new secure allocator instance.
 *
 * @return Pointer to allocator handle, or NULL on failure.
 *         Must be freed with secure_allocator_destroy().
 */
SecureAllocatorHandle* secure_allocator_new(void);

/**
 * Allocate secure memory of the given size.
 *
 * Memory is automatically zeroized before deallocation to prevent
 * sensitive data leakage.
 *
 * @param handle  Allocator handle
 * @param size    Number of bytes to allocate
 * @return        Pointer to allocated memory, or NULL on failure.
 *                Must be freed with secure_allocator_free().
 */
uint8_t* secure_allocator_allocate(SecureAllocatorHandle* handle, size_t size);

/**
 * Free previously allocated secure memory.
 *
 * @param handle  Allocator handle
 * @param ptr     Pointer to memory to free
 * @param size    Size of the allocation in bytes
 */
void secure_allocator_free(SecureAllocatorHandle* handle, uint8_t* ptr, size_t size);

/**
 * Destroy the allocator and release all resources.
 *
 * @param handle  Allocator handle to destroy
 */
void secure_allocator_destroy(SecureAllocatorHandle* handle);

// ============================================================================
// CryptoModule API
// ============================================================================

/**
 * Create a new crypto module with the given AES-256 key.
 *
 * @param key      Pointer to 32-byte AES key
 * @param key_len  Length of the key (must be exactly 32)
 * @return         Pointer to crypto module handle, or NULL on failure.
 *                 Must be freed with crypto_module_destroy().
 */
CryptoModuleHandle* crypto_module_new(const uint8_t* key, size_t key_len);

/**
 * Generate a cryptographically secure random AES-256 key.
 *
 * @return Pointer to 32-byte key. Must be freed with crypto_free_key().
 */
uint8_t* crypto_module_generate_key(void);

/**
 * Free a generated key.
 *
 * @param key  Pointer to key to free
 */
void crypto_free_key(uint8_t* key);

/**
 * Encrypt data using AES-256-GCM.
 *
 * Output format: [nonce_len (4 bytes)][nonce][ciphertext]
 * The nonce is included with the ciphertext for later decryption.
 *
 * @param handle         Crypto module handle
 * @param plaintext      Data to encrypt
 * @param plaintext_len  Length of plaintext in bytes
 * @param out_len        Output: length of encrypted data
 * @return               Pointer to encrypted data, or NULL on failure.
 *                       Must be freed with crypto_free_buffer().
 */
uint8_t* crypto_encrypt(
    CryptoModuleHandle* handle,
    const uint8_t* plaintext,
    size_t plaintext_len,
    size_t* out_len
);

/**
 * Decrypt data using AES-256-GCM.
 *
 * Input format: [nonce_len (4 bytes)][nonce][ciphertext]
 *
 * @param handle         Crypto module handle
 * @param encrypted      Encrypted data (including nonce)
 * @param encrypted_len  Length of encrypted data
 * @param out_len        Output: length of decrypted data (0 on failure)
 * @return               Pointer to decrypted plaintext, or NULL on failure.
 *                       Must be freed with crypto_free_buffer().
 */
uint8_t* crypto_decrypt(
    CryptoModuleHandle* handle,
    const uint8_t* encrypted,
    size_t encrypted_len,
    size_t* out_len
);

/**
 * Free encrypted/decrypted buffer.
 *
 * @param ptr  Pointer to buffer to free
 * @param len  Length of buffer in bytes
 */
void crypto_free_buffer(uint8_t* ptr, size_t len);

/**
 * Destroy the crypto module and release all resources.
 *
 * @param handle  Crypto module handle to destroy
 */
void crypto_module_destroy(CryptoModuleHandle* handle);

// ============================================================================
// AuditLogger API
// ============================================================================

/**
 * Create a new audit logger with the given HMAC key.
 *
 * @param key      Pointer to HMAC key
 * @param key_len  Length of HMAC key in bytes
 * @return         Pointer to logger handle, or NULL on failure.
 *                 Must be freed with audit_logger_destroy().
 */
AuditLoggerHandle* audit_logger_new(const uint8_t* key, size_t key_len);

/**
 * Generate a cryptographically secure HMAC key.
 *
 * @return Pointer to 32-byte HMAC key. Must be freed with crypto_free_key().
 */
uint8_t* audit_logger_generate_key(void);

/**
 * Set the log file path for persistent storage.
 *
 * @param handle  Logger handle
 * @param path    Path to log file (null-terminated string)
 * @return        true on success, false on failure
 */
bool audit_logger_set_log_file(AuditLoggerHandle* handle, const char* path);

/**
 * Log an audit event with timestamp and HMAC.
 *
 * @param handle      Logger handle
 * @param event_type  Type of event (e.g., "LOGIN", "ACCESS", "ERROR")
 * @param details     Event details (null-terminated string)
 * @return            Pointer to audit entry, or NULL on failure.
 *                    Must be freed with audit_entry_destroy().
 */
void* audit_logger_log(
    AuditLoggerHandle* handle,
    const char* event_type,
    const char* details
);

/**
 * Verify the HMAC integrity of an audit entry.
 *
 * @param handle  Logger handle (must use same key as when entry was created)
 * @param entry   Audit entry to verify
 * @return        true if HMAC is valid, false otherwise
 */
bool audit_logger_verify_entry(AuditLoggerHandle* handle, const void* entry);

/**
 * Get the timestamp from an audit entry.
 *
 * @param entry  Audit entry
 * @return       ISO 8601 timestamp string (null-terminated)
 */
const char* audit_entry_get_timestamp(const void* entry);

/**
 * Get the event type from an audit entry.
 *
 * @param entry  Audit entry
 * @return       Event type string (null-terminated)
 */
const char* audit_entry_get_event_type(const void* entry);

/**
 * Get the details from an audit entry.
 *
 * @param entry  Audit entry
 * @return       Details string (null-terminated)
 */
const char* audit_entry_get_details(const void* entry);

/**
 * Get the HMAC from an audit entry.
 *
 * @param entry  Audit entry
 * @return       HMAC hex string (null-terminated)
 */
const char* audit_entry_get_hmac(const void* entry);

/**
 * Destroy an audit entry.
 *
 * @param entry  Audit entry to destroy
 */
void audit_entry_destroy(void* entry);

/**
 * Destroy the audit logger and release all resources.
 *
 * @param handle  Logger handle to destroy
 */
void audit_logger_destroy(AuditLoggerHandle* handle);

// ============================================================================
// Initialization Helper
// ============================================================================

/**
 * Initialize all security components at once.
 *
 * Creates a secure allocator, crypto module with random key,
 * and audit logger with random HMAC key.
 *
 * @param allocator_out  Output: allocator handle
 * @param crypto_out     Output: crypto module handle
 * @param logger_out     Output: audit logger handle
 */
void security_core_init(
    SecureAllocatorHandle** allocator_out,
    CryptoModuleHandle** crypto_out,
    AuditLoggerHandle** logger_out
);

#ifdef __cplusplus
}
#endif

#endif /* PHOENIX_SECURITY_CORE_H */
