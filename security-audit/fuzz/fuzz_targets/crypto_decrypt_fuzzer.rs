//! Crypto Decrypt Fuzzer
//! Tests AES-256-GCM decryption with malformed inputs

#![no_main]

use libfuzzer_sys::fuzz_target;
use phoenix_security_core::CryptoModule;

fuzz_target!(|data: &[u8]| {
    // Need at least key + some encrypted data
    if data.len() < 32 + 16 {
        return;
    }
    
    let key = &data[0..32];
    let encrypted_data = &data[32..];
    
    // Create crypto module
    if let Ok(crypto) = CryptoModule::new(key) {
        // Fuzz decryption with arbitrary data (should handle errors gracefully)
        // Format: [nonce_len (4 bytes)][nonce][ciphertext]
        
        // Test 1: Try direct decrypt (expects properly formatted data)
        if encrypted_data.len() >= 4 {
            let nonce_len = u32::from_le_bytes([
                encrypted_data[0],
                encrypted_data[1],
                encrypted_data[2],
                encrypted_data[3],
            ]) as usize;
            
            // Only try if nonce_len is reasonable
            if nonce_len == 12 && encrypted_data.len() >= 4 + nonce_len {
                let nonce = &encrypted_data[4..4 + nonce_len];
                let ciphertext = &encrypted_data[4 + nonce_len..];
                let _ = crypto.decrypt(nonce, ciphertext);
            }
        }
        
        // Test 2: Try base64 decode (hex in this implementation)
        if let Ok(encoded) = std::str::from_utf8(encrypted_data) {
            let _ = crypto.decrypt_from_base64(encoded);
        }
    }
});
