//! Phoenix Engine Security Core
//!
//! A secure Rust library providing:
//! - Secure memory allocation with automatic zeroization
//! - AES-256-GCM encryption/decryption
//! - Audit logging with timestamps and HMAC integrity
//!
//! # Safety
//! This library minimizes unsafe code and follows Rust security best practices.

pub mod ffi;

use aes_gcm::{
    aead::{Aead, KeyInit, OsRng},
    Aes256Gcm, Nonce,
};
use hmac::{Hmac, Mac};
use rand::RngCore;
use sha2::Sha256;
use std::sync::Mutex;
use time::{format_description, OffsetDateTime};
use zeroize::{Zeroize, Zeroizing};

type HmacSha256 = Hmac<Sha256>;

// ============================================================================
// SecureAllocator - Memory-safe allocation with automatic zeroization
// ============================================================================

/// Secure allocator that ensures memory is zeroized before deallocation
/// to prevent sensitive data leakage.
pub struct SecureAllocator {
    allocations: Mutex<Vec<*mut u8>>,
}

impl SecureAllocator {
    /// Create a new secure allocator instance
    pub fn new() -> Self {
        SecureAllocator {
            allocations: Mutex::new(Vec::new()),
        }
    }

    /// Allocate secure memory of the given size
    /// Returns a Zeroizing wrapper that automatically clears memory on drop
    pub fn allocate(&self, size: usize) -> Zeroizing<Vec<u8>> {
        let mut buffer = Zeroizing::new(vec![0u8; size]);
        // Fill with random data for additional security
        OsRng.fill_bytes(&mut buffer[..]);
        buffer
    }

    /// Allocate and initialize with specific data
    pub fn allocate_with(&self, data: &[u8]) -> Zeroizing<Vec<u8>> {
        let mut buffer = Zeroizing::new(vec![0u8; data.len()]);
        buffer[..data.len()].copy_from_slice(data);
        buffer
    }

    /// Securely clear and deallocate memory
    /// Note: Zeroizing handles this automatically on drop
    pub fn deallocate(&self, _buffer: &mut Zeroizing<Vec<u8>>) {
        // Zeroizing automatically zeroizes on drop
        // This method is provided for explicit control
    }
}

impl Default for SecureAllocator {
    fn default() -> Self {
        Self::new()
    }
}

// ============================================================================
// CryptoModule - AES-256-GCM Encryption
// ============================================================================

/// Encryption error types
#[derive(Debug, Clone)]
pub enum CryptoError {
    EncryptionFailed(String),
    DecryptionFailed(String),
    InvalidKeySize(String),
    InvalidNonceSize(String),
}

impl std::fmt::Display for CryptoError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            CryptoError::EncryptionFailed(msg) => write!(f, "Encryption failed: {}", msg),
            CryptoError::DecryptionFailed(msg) => write!(f, "Decryption failed: {}", msg),
            CryptoError::InvalidKeySize(msg) => write!(f, "Invalid key size: {}", msg),
            CryptoError::InvalidNonceSize(msg) => write!(f, "Invalid nonce size: {}", msg),
        }
    }
}

impl std::error::Error for CryptoError {}

/// AES-256-GCM encryption module
pub struct CryptoModule {
    key: Zeroizing<Vec<u8>>,
}

impl CryptoModule {
    /// AES-256 key size in bytes
    pub const KEY_SIZE: usize = 32;
    /// Nonce size for GCM mode
    pub const NONCE_SIZE: usize = 12;

    /// Create a new crypto module with the given key
    /// Key must be exactly 32 bytes for AES-256
    pub fn new(key: &[u8]) -> Result<Self, CryptoError> {
        if key.len() != Self::KEY_SIZE {
            return Err(CryptoError::InvalidKeySize(format!(
                "Expected {} bytes, got {}",
                Self::KEY_SIZE,
                key.len()
            )));
        }

        let mut key_buffer = Zeroizing::new(vec![0u8; Self::KEY_SIZE]);
        key_buffer.copy_from_slice(key);

        Ok(CryptoModule { key: key_buffer })
    }

    /// Generate a random secure key
    pub fn generate_key() -> Zeroizing<Vec<u8>> {
        let mut key = Zeroizing::new(vec![0u8; Self::KEY_SIZE]);
        OsRng.fill_bytes(&mut key[..]);
        key
    }

    /// Encrypt data using AES-256-GCM
    /// Returns: (nonce, ciphertext) - nonce must be stored with ciphertext for decryption
    pub fn encrypt(&self, plaintext: &[u8]) -> Result<(Vec<u8>, Vec<u8>), CryptoError> {
        let cipher = Aes256Gcm::new_from_slice(&self.key)
            .map_err(|e| CryptoError::EncryptionFailed(e.to_string()))?;

        let mut nonce_bytes = [0u8; Self::NONCE_SIZE];
        OsRng.fill_bytes(&mut nonce_bytes);
        let nonce = Nonce::from_slice(&nonce_bytes);

        let ciphertext = cipher
            .encrypt(nonce, plaintext)
            .map_err(|e| CryptoError::EncryptionFailed(e.to_string()))?;

        Ok((nonce_bytes.to_vec(), ciphertext))
    }

    /// Decrypt data using AES-256-GCM
    /// Requires the nonce that was used during encryption
    pub fn decrypt(&self, nonce: &[u8], ciphertext: &[u8]) -> Result<Vec<u8>, CryptoError> {
        if nonce.len() != Self::NONCE_SIZE {
            return Err(CryptoError::InvalidNonceSize(format!(
                "Expected {} bytes, got {}",
                Self::NONCE_SIZE,
                nonce.len()
            )));
        }

        let cipher = Aes256Gcm::new_from_slice(&self.key)
            .map_err(|e| CryptoError::DecryptionFailed(e.to_string()))?;

        let nonce = Nonce::from_slice(nonce);

        let plaintext = cipher
            .decrypt(nonce, ciphertext)
            .map_err(|e| CryptoError::DecryptionFailed(e.to_string()))?;

        Ok(plaintext)
    }

    /// Encrypt and return base64-encoded result (nonce + ciphertext)
    pub fn encrypt_to_base64(&self, plaintext: &[u8]) -> Result<String, CryptoError> {
        let (nonce, ciphertext) = self.encrypt(plaintext)?;
        let mut combined = nonce;
        combined.extend_from_slice(&ciphertext);
        Ok(base64_encode(&combined))
    }

    /// Decrypt from base64-encoded input
    pub fn decrypt_from_base64(&self, encoded: &str) -> Result<Vec<u8>, CryptoError> {
        let combined = base64_decode(encoded)
            .map_err(|e| CryptoError::DecryptionFailed(format!("Base64 decode failed: {}", e)))?;

        if combined.len() < Self::NONCE_SIZE {
            return Err(CryptoError::DecryptionFailed("Data too short".to_string()));
        }

        let nonce = &combined[..Self::NONCE_SIZE];
        let ciphertext = &combined[Self::NONCE_SIZE..];

        self.decrypt(nonce, ciphertext)
    }
}

// Simple base64 encoding (using hex as fallback to avoid extra dependency)
fn base64_encode(data: &[u8]) -> String {
    hex::encode(data)
}

fn base64_decode(s: &str) -> Result<Vec<u8>, String> {
    hex::decode(s).map_err(|e| e.to_string())
}

// ============================================================================
// AuditLogger - Secure audit logging with HMAC integrity
// ============================================================================

/// Audit log entry
#[derive(Debug, Clone)]
pub struct AuditEntry {
    pub timestamp: String,
    pub event_type: String,
    pub details: String,
    pub hmac: String,
}

/// Audit logger with HMAC integrity verification
pub struct AuditLogger {
    hmac_key: Zeroizing<Vec<u8>>,
    log_file: Option<String>,
    entries: Mutex<Vec<AuditEntry>>,
}

impl AuditLogger {
    /// Create a new audit logger with the given HMAC key
    pub fn new(hmac_key: &[u8]) -> Self {
        let mut key = Zeroizing::new(vec![0u8; hmac_key.len()]);
        key.copy_from_slice(hmac_key);

        AuditLogger {
            hmac_key: key,
            log_file: None,
            entries: Mutex::new(Vec::new()),
        }
    }

    /// Generate a secure HMAC key
    pub fn generate_key() -> Zeroizing<Vec<u8>> {
        let mut key = Zeroizing::new(vec![0u8; 32]);
        OsRng.fill_bytes(&mut key[..]);
        key
    }

    /// Set the log file path
    pub fn set_log_file(&mut self, path: &str) {
        self.log_file = Some(path.to_string());
    }

    /// Compute HMAC for data
    fn compute_hmac(&self, data: &str) -> String {
        let mut mac = HmacSha256::new_from_slice(&self.hmac_key)
            .expect("HMAC can take key of any size");
        mac.update(data.as_bytes());
        let result = mac.finalize();
        hex::encode(result.into_bytes())
    }

    /// Get current timestamp in ISO format
    fn get_timestamp() -> String {
        let format = format_description::well_known::Iso8601::DEFAULT;
        OffsetDateTime::now_utc()
            .format(&format)
            .unwrap_or_else(|_| "unknown".to_string())
    }

    /// Log an audit event
    pub fn log(&self, event_type: &str, details: &str) -> AuditEntry {
        let timestamp = Self::get_timestamp();

        // Create data for HMAC (timestamp + event_type + details)
        let data_for_hmac = format!("{}|{}|{}", timestamp, event_type, details);
        let hmac = self.compute_hmac(&data_for_hmac);

        let entry = AuditEntry {
            timestamp,
            event_type: event_type.to_string(),
            details: details.to_string(),
            hmac,
        };

        // Add to in-memory log
        if let Ok(mut entries) = self.entries.lock() {
            entries.push(entry.clone());
        }

        // Write to file if configured
        if let Some(ref log_file) = self.log_file {
            self.write_to_file(log_file, &entry);
        }

        entry
    }

    /// Write entry to log file
    fn write_to_file(&self, path: &str, entry: &AuditEntry) {
        use std::fs::OpenOptions;
        use std::io::Write;

        let log_line = format!(
            "{}|{}|{}|{}\n",
            entry.timestamp, entry.event_type, entry.details, entry.hmac
        );

        if let Ok(mut file) = OpenOptions::new().create(true).append(true).open(path) {
            let _ = file.write_all(log_line.as_bytes());
        }
    }

    /// Verify the integrity of an audit entry
    pub fn verify_entry(&self, entry: &AuditEntry) -> bool {
        let data_for_hmac = format!("{}|{}|{}", entry.timestamp, entry.event_type, entry.details);
        let expected_hmac = self.compute_hmac(&data_for_hmac);
        entry.hmac == expected_hmac
    }

    /// Get all logged entries
    pub fn get_entries(&self) -> Vec<AuditEntry> {
        self.entries.lock().map(|e| e.clone()).unwrap_or_default()
    }

    /// Verify all entries in the log
    pub fn verify_all(&self) -> bool {
        self.entries
            .lock()
            .map(|entries| entries.iter().all(|e| self.verify_entry(e)))
            .unwrap_or(false)
    }
}

// ============================================================================
// Module exports
// ============================================================================

/// Initialize the security core with default secure keys
pub fn init() -> (SecureAllocator, CryptoModule, AuditLogger) {
    let allocator = SecureAllocator::new();
    let crypto_key = CryptoModule::generate_key();
    let crypto = CryptoModule::new(&crypto_key).expect("Failed to create crypto module");
    let audit_key = AuditLogger::generate_key();
    let logger = AuditLogger::new(&audit_key);

    (allocator, crypto, logger)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_secure_allocator() {
        let allocator = SecureAllocator::new();
        let buffer = allocator.allocate(1024);
        assert_eq!(buffer.len(), 1024);
        // Buffer is automatically zeroized on drop
    }

    #[test]
    fn test_crypto_encrypt_decrypt() {
        let key = CryptoModule::generate_key();
        let crypto = CryptoModule::new(&key).unwrap();

        let plaintext = b"Hello, Phoenix Engine!";
        let (nonce, ciphertext) = crypto.encrypt(plaintext).unwrap();
        let decrypted = crypto.decrypt(&nonce, &ciphertext).unwrap();

        assert_eq!(plaintext.to_vec(), decrypted);
    }

    #[test]
    fn test_audit_logger() {
        let key = AuditLogger::generate_key();
        let logger = AuditLogger::new(&key);

        let entry = logger.log("LOGIN", "user=admin ip=192.168.1.1");

        assert!(logger.verify_entry(&entry));
        assert_eq!(entry.event_type, "LOGIN");
    }

    #[test]
    fn test_hmac_tampering_detection() {
        let key = AuditLogger::generate_key();
        let logger = AuditLogger::new(&key);

        let mut entry = logger.log("TEST", "original data");

        // Tamper with the entry
        entry.details = "tampered data".to_string();

        // Should fail verification
        assert!(!logger.verify_entry(&entry));
    }
}
